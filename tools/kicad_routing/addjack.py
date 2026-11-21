"""Add 12V jack input + LMR33630 5V buck + USB/jack priority P-FET + hub reset on VBUS_USB
to the root sheet Eeloo_Pad.kicad_sch (labels / power symbols on short wire stubs)."""
import re, uuid, math, sys
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from sexp import match, libsym, pins

SCH = sys.argv[1]
s = open(SCH).read()
ROOT = re.search(r'\(kicad_sch.*?\(uuid "([^"]+)"\)', s, re.S).group(1)
U = lambda: str(uuid.uuid4())
F = lambda v: ('%.4f' % v).rstrip('0').rstrip('.')

# ---------------------------------------------------------------- library symbols
def flat(lib, name, newname=None):
    blk, libtxt = libsym(lib, name)
    ext = re.search(r'\(extends "([^"]+)"\)', blk)
    if not ext:
        out = blk.replace('(symbol "%s"' % name, '(symbol "%s:%s"' % (lib, name), 1)
        return out, pins(blk)
    parent = ext.group(1)
    pb, _ = libsym(lib, parent)
    out = pb.replace('(symbol "%s"' % parent, '(symbol "%s:%s"' % (lib, name), 1)
    out = out.replace('(symbol "%s_' % parent, '(symbol "%s_' % name)
    for prop in ('Value', 'Footprint', 'Datasheet', 'Description', 'ki_keywords', 'ki_fp_filters'):
        m = re.search(r'\(property "%s" "((?:[^"\\]|\\.)*)"' % prop, blk)
        if m:
            out = re.sub(r'(\(property "%s" ")((?:[^"\\]|\\.)*)(")' % prop, lambda mm: mm.group(1) + m.group(1).replace('\\', '\\\\') + mm.group(3), out, count=1)
    return out, pins(pb)

need = [('Connector', 'Barrel_Jack'), ('Device', 'D_Schottky'), ('Device', 'D_TVS'), ('Transistor_FET', 'AO3401A'),
        ('power', '+12V'), ('Regulator_Switching', 'LMR33630ADDA'), ('Device', 'R'), ('Device', 'C'), ('Device', 'L'),
        ('Device', 'Polyfuse'), ('power', 'GND'), ('power', 'PWR_FLAG')]
PINS = {}
li = s.find('(lib_symbols'); le = match(s, li)
libsec = s[li:le]
add_lib = ''
for lib, name in need:
    txt, pp = flat(lib, name)
    PINS['%s:%s' % (lib, name)] = pp
    if '(symbol "%s:%s"' % (lib, name) not in libsec:
        add_lib += '\n' + '\n'.join('\t\t' + l for l in txt.replace('\t', '\t').splitlines())
if add_lib:
    s = s[:le - 1].rstrip() + add_lib + '\n\t)' + s[le:]

# ---------------------------------------------------------------- geometry
def xform(X, Y, r, x, y):
    c, sn = round(math.cos(math.radians(r)), 6), round(math.sin(math.radians(r)), 6)
    return (X + x * c - y * sn, Y - (x * sn + y * c))
def outward(r, a):
    ang = math.radians(a + 180 + r)
    return (round(math.cos(ang)), -round(math.sin(ang)))   # sheet coords (y down)

ITEMS = []
pwr_n = [221]; flg_n = [4]
def prop(name, val, x, y, hide=False, ang=0, just=None):
    return ('\t\t(property "%s" "%s"\n\t\t\t(at %s %s %d)\n%s\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n'
            '\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n%s\t\t\t)\n\t\t)\n') % (
        name, val, F(x), F(y), ang, '\t\t\t(hide yes)\n' if hide else '', ('\t\t\t\t(justify %s)\n' % just) if just else '')
def symbol(lib_id, ref, val, X, Y, r=0, fp='', ds='', desc='', extra=None, refpos=None, valpos=None, inbom=True, just=None):
    pp = PINS[lib_id]
    rp = refpos or (X + 2.54, Y - 1.27); vp = valpos or (X + 2.54, Y + 1.27)
    t = '\t(symbol\n\t\t(lib_id "%s")\n\t\t(at %s %s %d)\n\t\t(unit 1)\n\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n' % (lib_id, F(X), F(Y), r)
    t += '\t\t(in_bom %s)\n\t\t(on_board %s)\n\t\t(in_pos_files %s)\n\t\t(dnp no)\n\t\t(uuid "%s")\n' % (('yes' if inbom else 'no'),) * 3 + (U(),) if False else ''
    t = '\t(symbol\n\t\t(lib_id "%s")\n\t\t(at %s %s %d)\n\t\t(unit 1)\n\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom %s)\n\t\t(on_board %s)\n\t\t(in_pos_files %s)\n\t\t(dnp no)\n\t\t(uuid "%s")\n' % (
        lib_id, F(X), F(Y), r, 'yes' if inbom else 'no', 'yes' if inbom else 'no', 'yes' if inbom else 'no', U())
    hide_ref = ref.startswith('#')
    fa = r if r in (90, 270) else 0
    t += prop('Reference', ref, rp[0], rp[1], hide=hide_ref, just=just, ang=fa)
    t += prop('Value', val, vp[0], vp[1], just=just, ang=fa)
    t += prop('Footprint', fp, X, Y, hide=True)
    t += prop('Datasheet', ds, X, Y, hide=True)
    t += prop('Description', desc, X, Y, hide=True)
    for k, v in (extra or {}).items(): t += prop(k, v, X, Y, hide=True)
    for num in sorted({p['num'] for p in pp}, key=lambda q: (len(q), q)):
        t += '\t\t(pin "%s"\n\t\t\t(uuid "%s")\n\t\t)\n' % (num, U())
    t += '\t\t(instances\n\t\t\t(project "Eeloo_Pad"\n\t\t\t\t(path "/%s"\n\t\t\t\t\t(reference "%s")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n' % (ROOT, ref)
    ITEMS.append(t)
def wire(a, b):
    ITEMS.append('\t(wire\n\t\t(pts\n\t\t\t(xy %s %s) (xy %s %s)\n\t\t)\n\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid "%s")\n\t)\n' % (F(a[0]), F(a[1]), F(b[0]), F(b[1]), U()))
def label(name, x, y, d):
    ang = {(1, 0): 0, (0, -1): 90, (-1, 0): 180, (0, 1): 270}[d]
    just = 'left bottom' if ang in (0, 90) else 'right bottom'
    ITEMS.append('\t(label "%s"\n\t\t(at %s %s %d)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify %s)\n\t\t)\n\t\t(uuid "%s")\n\t)\n' % (name, F(x), F(y), ang, just, U()))
def noconn(x, y):
    ITEMS.append('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n' % (F(x), F(y), U()))
def power(kind, x, y, d):
    if kind == 'GND':
        r = {(0, 1): 0, (1, 0): 90, (0, -1): 180, (-1, 0): 270}[d]; lid = 'power:GND'; ref = '#PWR%03d' % pwr_n[0]; pwr_n[0] += 1
        desc = 'Power symbol creates a global label with name \\"GND\\" , ground'
    elif kind == '+12V':
        r = {(0, -1): 0, (-1, 0): 90, (0, 1): 180, (1, 0): 270}[d]; lid = 'power:+12V'; ref = '#PWR%03d' % pwr_n[0]; pwr_n[0] += 1
        desc = 'Power symbol creates a global label with name \\"+12V\\"'
    else:
        r = {(0, -1): 0, (-1, 0): 90, (0, 1): 180, (1, 0): 270}[d]; lid = 'power:PWR_FLAG'; ref = '#FLG%02d' % flg_n[0]; flg_n[0] += 1
        desc = 'Special symbol for telling ERC where power comes from'
    off = {(0, 1): (0, 5.08), (0, -1): (0, -3.81), (1, 0): (3.81, 0), (-1, 0): (-3.81, 0)}[d]
    symbol(lid, ref, kind if kind != 'FLAG' else 'PWR_FLAG', x, y, r, desc=desc, refpos=(x, y), valpos=(x + off[0], y + off[1]), inbom=False)

STUB = 2.54
PP = {}
def junction(x, y):
    ITEMS.append('\t(junction\n\t\t(at %s %s)\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n\t\t(uuid "%s")\n\t)\n' % (F(x), F(y), U()))
def place(lib_id, ref, val, X, Y, r, nets, fp='', ds='', desc='', extra=None, fields=None):
    """nets: {pin_number: net}; net 'GND' / '+12V' -> power symbol, 'NC' -> no-connect, else label"""
    if fields is None: fields = 'v' if r in (0, 180) and lib_id in ('Device:R', 'Device:C') or (lib_id == 'Device:D_TVS' and r in (90, 270)) else 'h'
    if fields == 'v': rp, vp, j = (X + 2.54, Y - 1.27), (X + 2.54, Y + 1.27), 'left'
    elif fields == 'h': rp, vp, j = (X, Y - 3.81), (X, Y + 3.81), None
    else: rp, vp, j = fields
    symbol(lib_id, ref, val, X, Y, r, fp, ds, desc, extra, refpos=rp, valpos=vp, just=j)
    done = set()
    for p in PINS[lib_id]:
        PP[(ref, p['num'])] = xform(X, Y, r, p['x'], p['y'])
    for p in PINS[lib_id]:
        if p['num'] not in nets or nets[p['num']] == '-': continue
        ex, ey = xform(X, Y, r, p['x'], p['y'])
        key = (round(ex, 3), round(ey, 3))
        if key in done: continue
        done.add(key)
        net = nets[p['num']]
        if net == 'NC': noconn(ex, ey); continue
        d = outward(r, p['a'])
        tx, ty = ex + d[0] * STUB, ey + d[1] * STUB
        wire((ex, ey), (tx, ty))
        if net in ('GND', '+12V'): power(net, tx, ty, d)
        else: label(net, tx, ty, d)

R06 = 'Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder'
C06 = 'Capacitor_SMD:C_0603_1608Metric_Pad1.08x0.95mm_HandSolder'
C12 = 'Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder'
Y0 = 93.98
# --- 12 V input: J45 -> F2 -> D11 wired directly
place('Connector:Barrel_Jack', 'J45', 'Jack 12V 5.5x2.1', 113.03, 96.52, 0, {'1': '-', '2': 'GND'},
      fp='Connector_BarrelJack:BarrelJack_Horizontal', desc='DC barrel jack 5.5x2.1mm, centre +, 12V 2A adapter',
      fields=((113.03, 90.17), (113.03, 102.87), None))
place('Device:Polyfuse', 'F2', '2A', 128.27, Y0, 90, {'1': '-', '2': '-'},
      fp='Fuse:Fuse_1812_4532Metric_Pad1.30x3.40mm_HandSolder', desc='PTC resettable fuse 2A hold, 1812')
place('Device:D_Schottky', 'D11', 'SS34', 139.7, Y0, 180, {'2': '-', '1': '+12V'},
      fp='Diode_SMD:D_SMA', desc='Reverse polarity protection, 40V 3A Schottky')
wire(PP[('J45', '1')], PP[('F2', '1')]); wire(PP[('F2', '2')], PP[('D11', '2')])
label('VIN_DC', 122.555, Y0, (0, -1)) if False else None
place('Device:D_TVS', 'D12', 'SMBJ15CA', 152.4, 104.14, 270, {'1': '+12V', '2': 'GND'},
      fp='Diode_SMD:D_SMB', desc='TVS 15V on 12V input', fields=((149.86, 102.87), (149.86, 105.41), 'left'))
for ref, val, x, fp in (('C112', '10uF', 160.02, C12), ('C113', '10uF', 172.72, C12), ('C114', '100nF', 185.42, C06)):
    place('Device:C', ref, val, x, 104.14, 0, {'1': '+12V', '2': 'GND'}, fp=fp, desc='input cap, 25V X7R' if val == '10uF' else '')
# --- buck 12V -> 5.3V
place('Regulator_Switching:LMR33630ADDA', 'U12', 'LMR33630ADDA', 208.28, 104.14, 0,
      {'2': '+12V', '3': '-', '6': 'BUCK_VCC', '1': 'GND', '9': 'GND', '7': 'BUCK_BOOT', '8': '-', '5': 'BUCK_FB', '4': 'NC'},
      fp='Package_SO:Texas_HSOP-8-1EP_3.9x4.9mm_P1.27mm_ThermalVias', ds='https://www.ti.com/lit/ds/symlink/lmr33630.pdf',
      desc='3.8-36V in, 3A, 400kHz synchronous buck, Vfb=1.0V', fields=((208.28, 92.71), (208.28, 95.25), None))
wire(PP[('U12', '3')], PP[('U12', '2')])                      # EN tied to VIN
place('Device:C', 'C116', '1uF', 193.04, 119.38, 0, {'1': 'BUCK_VCC', '2': 'GND'}, fp=C06, desc='LMR33630 VCC bypass')
place('Device:C', 'C115', '100nF', 228.6, 88.9, 90, {'1': 'BUCK_BOOT', '2': 'BUCK_SW'}, fp=C06, desc='bootstrap')
place('Device:L', 'L2', '8.2uH', 238.76, 101.6, 90, {'1': '-', '2': 'V5_DC'},
      fp='Inductor_SMD:L_Bourns_SRP1038C_10.0x10.0mm', desc='8.2uH, Isat >= 5A (e.g. Bourns SRP1038C-8R2M)')
wire(PP[('U12', '8')], PP[('L2', '1')]); label('BUCK_SW', 223.52, 101.6, (1, 0))
place('Device:R', 'R130', '100k', 242.57, 109.22, 0, {'1': '-', '2': '-'}, fp=R06, desc='FB top: Vout = 1.0V x (1 + 100k/23.2k) = 5.31V')
place('Device:R', 'R131', '23.2k', 242.57, 119.38, 0, {'1': '-', '2': 'GND'}, fp=R06, desc='FB bottom')
wire(PP[('L2', '2')], PP[('R130', '1')]); junction(*PP[('L2', '2')])
fa, fb = PP[('R130', '2')], PP[('R131', '1')]
wire(fa, fb); mid = (fa[0], (fa[1] + fb[1]) / 2); mid = (mid[0], round(mid[1] / 1.27) * 1.27)
junction(*mid); wire(mid, (mid[0] - 3.81, mid[1])); label('BUCK_FB', mid[0] - 3.81, mid[1], (-1, 0))
for i, x in enumerate((251.46, 261.62, 271.78)):
    place('Device:C', 'C%d' % (117 + i), '22uF', x, 111.76, 0, {'1': 'V5_DC', '2': 'GND'}, fp=C12, desc='output cap, 10V X7R')
place('Device:C', 'C120', '100nF', 281.94, 111.76, 0, {'1': 'V5_DC', '2': 'GND'}, fp=C06)
place('Device:D_Schottky', 'D13', 'SS54', 269.24, Y0 - 2.54, 180, {'2': 'V5_DC', '1': 'VBUS'},
      fp='Diode_SMD:D_SMC', desc='OR-ing diode 12V-buck -> VBUS, 40V 5A Schottky')
# --- USB / jack priority: P-FET between VBUS_USB and VBUS
place('Transistor_FET:AO3401A', 'Q2', 'AO3401A', 134.62, 133.35, 0, {'1': '-', '2': 'VBUS', '3': 'VBUS_USB'},
      desc='USB VBUS switch: off when the 12V jack is present (gate pulled up from +12V)',
      fields=((138.43, 132.08), (138.43, 134.62), 'left'))
place('Device:R', 'R132', '47k', 116.84, 127.0, 0, {'1': '+12V', '2': '-'}, fp=R06)
place('Device:R', 'R133', '100k', 116.84, 139.7, 0, {'1': '-', '2': 'GND'}, fp=R06)
ga, gb = PP[('R132', '2')], PP[('R133', '1')]
wire(ga, gb); g = PP[('Q2', '1')]
junction(ga[0], g[1]); wire((ga[0], g[1]), g); label('DC_SENSE', ga[0] + 2.54, g[1], (1, 0)) if False else None
ITEMS.append('\t(label "DC_SENSE"\n\t\t(at %s %s 0)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "%s")\n\t)\n' % (F(ga[0] + 2.54), F(g[1]), U()))
# --- power flags
def flag_on(net, x, y):
    if net == '+12V':
        power('+12V', x, y, (0, -1)); wire((x, y), (x, y + 2.54)); power('FLAG', x, y + 2.54, (0, 1))
    else:
        power('FLAG', x, y, (0, -1)); wire((x, y), (x, y + 2.54)); label(net, x, y + 2.54, (0, 1))
flag_on('+12V', 166.37, 80.01)
flag_on('VBUS', 281.94, 86.36)
# --- hub held in reset without USB VBUS: R134 15k HUB_RESET -> GND (R123 now from VBUS_USB)
place('Device:R', 'R134', '15k', 114.3, 36.83, 0, {'1': '-', '2': 'GND'}, fp=R06,
      desc='with R123 (10k from VBUS_USB): RESET = 3.0V only when the USB host supplies VBUS')
wire((106.68, 30.48), (114.3, 30.48)); wire((114.3, 30.48), PP[('R134', '1')])
# --- note
ITEMS.append('\t(text "Entree 12V (jack) -> abaisseur 5,3V -> VBUS. Q2 coupe l\'USB quand le jack est branche.\\nLe hub reste en reset sans VBUS USB (R123 depuis VBUS_USB + R134)."\n\t\t(exclude_from_sim no)\n\t\t(at 106.68 73.66 0)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "%s")\n\t)\n' % U())

# ---------------------------------------------------------------- relabel existing nets
def relabel(old, new, x, y):
    global s
    pat = re.compile(r'\(label "%s"\n\t\t\(at %s %s ' % (re.escape(old), re.escape(F(x)), re.escape(F(y))))
    m = pat.search(s)
    assert m, (old, x, y)
    s = s[:m.start()] + '(label "%s"' % new + s[m.start() + len('(label "%s"' % old):]
relabel('VBUS', 'VBUS_USB', 58.42, 25.4)        # F1 pin 2 -> VBUS_USB
relabel('HUB_V33', 'VBUS_USB', 106.68, 22.86)   # R123 top -> VBUS_USB

# ---------------------------------------------------------------- insert before sheet_instances / embedded files end
k = s.rfind('\n\t(sheet_instances')
if k < 0: k = s.rfind('\n\t(embedded_fonts')
if k < 0: k = s.rfind('\n)')
s = s[:k] + '\n' + ''.join(ITEMS).rstrip('\n') + s[k:]
open(SCH, 'w').write(s)
print('items', len(ITEMS), 'pwr up to', pwr_n[0] - 1, 'flg up to', flg_n[0] - 1)
