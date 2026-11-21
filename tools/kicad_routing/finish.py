import sys, re, math; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters, desc
from keepouts import add_keepouts
from smooth import smooth
src, out, drc = sys.argv[1], sys.argv[2], sys.argv[3]
b = pcbnew.LoadBoard(src)
RP = dict(clr=0.2, margin=0.05, grid=0.1)
rep = open(drc).read()
cw = [(float(x), float(y)) for blk in re.split(r'\n(?=\[)', rep) if blk.startswith('[connection_width]') for x, y in re.findall(r'@\(([\d.]+) mm, ([\d.]+) mm\): Track', blk)]
print('cw spots', len(cw))
# 1) heal micro gaps (endpoints 0 < d < 0.3 mm, same net & layer)
healed = 0
tr = [t for t in b.GetTracks() if t.Type() == pcbnew.PCB_TRACE_T and t.GetNetname() not in ('GND', '')]
from collections import defaultdict
ep = defaultdict(list)
for t in tr:
    for p in (t.GetStart(), t.GetEnd()): ep[(t.GetNetname(), t.GetLayer())].append((M(p.x), M(p.y)))
for (net, L), pts in ep.items():
    s = set((round(x, 4), round(y, 4)) for x, y in pts)
    lst = sorted(s)
    for i, a in enumerate(lst):
        for c in lst[i + 1:]:
            if c[0] - a[0] > 0.3: break
            d = math.hypot(c[0] - a[0], c[1] - a[1])
            if 0.001 < d < 0.3 and any(abs(gx - (a[0]+c[0])/2) < 1.5 and abs(gy - (a[1]+c[1])/2) < 1.5 for gx, gy in cw):
                t = pcbnew.PCB_TRACK(b); t.SetStart(V(FromMM(a[0]), FromMM(a[1]))); t.SetEnd(V(FromMM(c[0]), FromMM(c[1])))
                w = min(x.GetWidth() for x in tr if x.GetNetname() == net and x.GetLayer() == L and ((round(M(x.GetStart().x),4),round(M(x.GetStart().y),4)) in (a, c) or (round(M(x.GetEnd().x),4),round(M(x.GetEnd().y),4)) in (a, c)))
                t.SetWidth(w); t.SetLayer(L); t.SetNet(b.FindNet(net)); b.Add(t); healed += 1
print('healed gaps', healed)
# 2) remove DRC-dangling tracks / vias (iterative, connectivity-checked)
def item_at(kind, x, y, net):
    best = None
    for t in b.GetTracks():
        if t.GetNetname() != net: continue
        if kind == 'Via' and t.Type() == pcbnew.PCB_VIA_T and abs(M(t.GetPosition().x) - x) < 0.01 and abs(M(t.GetPosition().y) - y) < 0.01: return t
        if kind == 'Track' and t.Type() == pcbnew.PCB_TRACE_T:
            for p in (t.GetStart(), t.GetEnd()):
                if abs(M(p.x) - x) < 0.01 and abs(M(p.y) - y) < 0.01: best = t
    return best
rem = 0
for kind, x, y, net in re.findall(r'@\(([\d.]+) mm, ([\d.]+) mm\): (Track|Via) \[([^\]]+)\]', ''.join(re.findall(r'\[(?:track|via)_dangling\][^\[]*(?:\[[^\]]*\][^\[]*)*?(?=\n\[|\Z)', rep))) and []:
    pass
for blk in re.split(r'\n(?=\[)', rep):
    if not (blk.startswith('[track_dangling]') or blk.startswith('[via_dangling]')): continue
    m = re.search(r'@\(([\d.]+) mm, ([\d.]+) mm\): (Track|Via) \[([^\]]+)\]', blk)
    if not m: continue
    x, y, kind, net = float(m.group(1)), float(m.group(2)), m.group(3), m.group(4)
    if net == 'GND': continue
    # walk the dangling chain from that end
    while True:
        t = item_at(kind, x, y, net)
        if t is None: break
        n0 = len(clusters(b, net)); b.Remove(t)
        if len(clusters(b, net)) > n0: b.Add(t); break
        rem += 1
        if t.Type() == pcbnew.PCB_VIA_T: break
        o = t.GetEnd() if abs(M(t.GetStart().x) - x) < 0.01 and abs(M(t.GetStart().y) - y) < 0.01 else t.GetStart()
        x, y = M(o.x), M(o.y)
        others = [u for u in b.GetTracks() if u.GetNetname() == net and u.Type() == pcbnew.PCB_TRACE_T and any(abs(M(p.x) - x) < 0.01 and abs(M(p.y) - y) < 0.01 for p in (u.GetStart(), u.GetEnd()))]
        pads = [p for f in b.GetFootprints() for p in f.Pads() if p.GetNetname() == net and p.HitTest(V(FromMM(x), FromMM(y)))]
        vias = [u for u in b.GetTracks() if u.Type() == pcbnew.PCB_VIA_T and u.GetNetname() == net and abs(M(u.GetPosition().x) - x) < 0.01 and abs(M(u.GetPosition().y) - y) < 0.01]
        if len(others) != 1 or pads or vias: break
        kind = 'Track'
print('dangling removed', rem)
# 3) clearance fixes: remove tracks of other nets too close (<0.2) to LED pads
reroute = set()
leds = [b.FindFootprintByReference(f'LED{i}') for i in range(1, 26)]
for t in list(b.GetTracks()):
    Ls = [pcbnew.F_Cu, pcbnew.B_Cu] if t.Type() == pcbnew.PCB_VIA_T else [t.GetLayer()]
    for f in leds:
        if (f.GetPosition() - t.GetStart()).EuclideanNorm() > FromMM(30): continue
        if any(p.IsOnLayer(L) and p.GetNetname() != t.GetNetname() and p.GetEffectiveShape(L).Collide(t.GetEffectiveShape(L), FromMM(0.199)) for L in Ls for p in f.Pads()):
            reroute.add(t.GetNetname()); b.Remove(t); break
print('clearance reroute', reroute)
# 4) route every changed net still split
nets = reroute | {'Net-(LED1-DOU)', '/VBUS'}
def region(groups, m):
    xs = []; ys = []
    for g in groups:
        for it in g:
            bb = it.GetBoundingBox(); xs += [M(bb.GetLeft()), M(bb.GetRight())]; ys += [M(bb.GetTop()), M(bb.GetBottom())]
    return (max(14.45, min(xs) - m), max(9.95, min(ys) - m), min(366.35, max(xs) + m), min(238.05, max(ys) + m))
for net in sorted(nets):
    cl = sorted(clusters(b, net), key=len)
    while len(cl) > 1:
        g = cl[0]; reg = region([g], 20)
        inside = lambda it: (M(it.GetBoundingBox().GetLeft()) > reg[0] + 1 and M(it.GetBoundingBox().GetRight()) < reg[2] - 1 and M(it.GetBoundingBox().GetTop()) > reg[1] + 1 and M(it.GetBoundingBox().GetBottom()) < reg[3] - 1)
        others = [it for h in cl[1:] for it in h if inside(it)]
        ok = False
        for w in ((0.5, 0.4, 0.3) if net == '/VBUS' else (0.2,)):
            for vc in (30, 10):
                R = Router(b, reg, **RP).build(w=w); add_keepouts(R, b)
                r, _ = R.route_net(net, [g, others], w=w, via_cost=vc, turn_cost=1.0)
                if r and len(clusters(b, net)) < len(cl): smooth(R, b, net, R.new_items); ok = True; break
                for t in R.new_items: b.Remove(t)
            if ok: break
        print(f'  {net} {desc(g)[:40]} ok={ok}', flush=True)
        if not ok: break
        cl = sorted(clusters(b, net), key=len)
print('split nets', [(n, len(clusters(b, n))) for n in sorted(nets) if len(clusters(b, n)) > 1])
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
