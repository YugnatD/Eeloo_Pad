"""Place the 12V-input / buck / USB-priority footprints on the PCB (copy), set nets from the
schematic netlist, link footprints to their schematic symbols, re-net F1.2 / R123.1."""
import pcbnew, sys, re
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from nets import nets
src, out, netf, sch = sys.argv[1:5]
b = pcbnew.LoadBoard(src)
N = nets(netf)
pin_net = {}
for name, nodes in N.items():
    for r, p in nodes: pin_net[(r, p)] = name
s = open(sch).read()
def sym(ref):
    i = s.find('(property "Reference" "%s"' % ref); a = s.rfind('\n\t(symbol\n', 0, i); blk = s[a:s.find('\n\t)\n', i)]
    g = lambda k: re.search(r'\(property "%s" "((?:[^"\\]|\\.)*)"' % k, blk).group(1)
    return dict(uuid=re.search(r'\n\t\t\(uuid "([^"]+)"\)', blk).group(1), fp=g('Footprint'), val=g('Value'), desc=g('Description'))
def net(name):
    n = b.FindNet(name)
    if n is None:
        n = pcbnew.NETINFO_ITEM(b, name); b.Add(n)
    return n
FPDIR = '/usr/share/kicad/footprints/'
# ref: (x, y, rot, side)
PL = {
    'J45': (352.5, 106.0, 180, 'F'),
    'D13': (354.6, 96.0, 0, 'F'),
    'F2': (362.6, 95.3, 90, 'F'),
    'D11': (362.6, 88.4, -90, 'F'),
    'D12': (358.3, 88.6, 90, 'F'),
    'C112': (354.6, 88.0, 90, 'F'),
    'C113': (352.0, 88.0, 90, 'F'),
    'U12': (353.6, 80.2, 0, 'F'),
    'L2': (362.2, 80.3, -90, 'F'),
    'Q2': (355.84, 70.6, 90, 'F'),
    'R133': (353.0, 74.6, 0, 'F'),
    'R132': (357.4, 74.6, 0, 'F'),
    # back side passives
    'C114': (351.2, 84.6, 0, 'B'),
    'C115': (356.6, 78.9, 90, 'B'),
    'C116': (356.6, 82.6, 90, 'B'),
    'C117': (360.5, 79.0, 90, 'B'),
    'C118': (363.0, 79.0, 90, 'B'),
    'C119': (358.9, 92.4, 0, 'B'),
    'C120': (360.6, 83.6, 0, 'B'),
    'R130': (350.9, 92.4, 0, 'B'),
    'R131': (354.4, 92.4, 0, 'B'),
    'R134': (351.7, 55.5, 0, 'B'),
}
for ref, (x, y, rot, side) in PL.items():
    old = b.FindFootprintByReference(ref)
    if old: b.Remove(old)
    info = sym(ref)
    lib, name = info['fp'].split(':')
    f = pcbnew.FootprintLoad(FPDIR + lib + '.pretty', name)
    f.SetFPID(pcbnew.LIB_ID(lib, name))
    f.SetReference(ref); f.SetValue(info['val'])
    f.SetPath(pcbnew.KIID_PATH('/' + info['uuid']))
    try: f.SetLibDescription(info['desc'])
    except Exception: pass
    b.Add(f)
    f.SetPosition(V(FromMM(x), FromMM(y)))
    if side == 'B': f.Flip(f.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT if hasattr(pcbnew, 'FLIP_DIRECTION_LEFT_RIGHT') else False)
    f.SetOrientationDegrees(rot)
    for p in f.Pads():
        k = (ref, p.GetNumber())
        if k in pin_net and not pin_net[k].startswith('unconnected'):
            p.SetNet(net(pin_net[k]))
# re-net existing pads
for ref, num in (('F1', '2'), ('R123', '1')):
    p = [q for q in b.FindFootprintByReference(ref).Pads() if q.GetNumber() == num][0]
    p.SetNet(net(pin_net[(ref, num)])); print(ref, num, '->', pin_net[(ref, num)])
b.Save(out)
for ref in PL:
    f = b.FindFootprintByReference(ref)
    print(ref, f.GetValue(), [(p.GetNumber(), p.GetNetname()) for p in f.Pads()][:5])
