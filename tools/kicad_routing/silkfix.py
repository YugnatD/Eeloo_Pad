"""Silkscreen cleanup: merge the shared border of side-by-side 14-seg displays, clip the jack
outline at the board edge, and move flagged reference texts to the nearest free spot."""
import pcbnew, sys, re
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src, out, drc = sys.argv[1:4]
b = pcbnew.LoadBoard(src)
F = {f.GetReference(): f for f in b.GetFootprints()}
SILK = (pcbnew.F_SilkS, pcbnew.B_SilkS)
def silk_shapes(f):
    return [g for g in f.GraphicalItems() if g.GetClass() == 'PCB_SHAPE' and g.GetLayer() in SILK]

# 1) displays side by side: drop the right display's left border
for a, c in (('LED26', 'LED27'), ('LED28', 'LED29'), ('LED30', 'LED31'), ('LED32', 'LED33'), ('LED34', 'LED35'), ('LED36', 'LED37')):
    A, C = F[a], F[c]
    if A.GetPosition().x > C.GetPosition().x: A, C = C, A
    xa = max(max(g.GetStart().x, g.GetEnd().x) for g in silk_shapes(A))
    xc = min(min(g.GetStart().x, g.GetEnd().x) for g in silk_shapes(C))
    if M(xc - xa) > 1.0: continue
    for g in silk_shapes(C):
        s, e = g.GetStart(), g.GetEnd()
        if g.GetShape() == pcbnew.SHAPE_T_SEGMENT and s.x == e.x == xc:
            C.Remove(g); continue
        if g.GetShape() == pcbnew.SHAPE_T_SEGMENT and s.y == e.y and min(s.x, e.x) == xc:
            nx = xa + FromMM(0.3)
            if s.x == xc: g.SetStart(V(nx, s.y))
            else: g.SetEnd(V(nx, e.y))
    print('display border merged', A.GetReference(), C.GetReference())

# 2) jack outline clipped at the right board edge
edge = b.GetBoardEdgesBoundingBox()
xmax = edge.GetRight() - FromMM(0.35)
J = F['J45']
for g in silk_shapes(J):
    s, e = g.GetStart(), g.GetEnd()
    if g.GetShape() != pcbnew.SHAPE_T_SEGMENT: continue
    if s.x > xmax and e.x > xmax: J.Remove(g); continue
    if s.x > xmax: g.SetStart(V(xmax, s.y + (e.y - s.y) * (s.x - xmax) // max(1, s.x - e.x)))
    if e.x > xmax: g.SetEnd(V(xmax, e.y + (s.y - e.y) * (e.x - xmax) // max(1, e.x - s.x)))

# 3) move flagged references
txt = open(drc).read()
flag = set()
for bl in txt.split('\n[')[1:]:
    if not bl.startswith(('silk_overlap', 'silk_over_copper', 'silk_edge_clearance')): continue
    flag |= set(re.findall(r'Reference field of ([A-Z]+\d+)', bl))
print('references to move:', sorted(flag))
ib = edge.GetLeft() + FromMM(0.6), edge.GetTop() + FromMM(0.6), edge.GetRight() - FromMM(0.6), edge.GetBottom() - FromMM(0.6)
def obstacles(side_front, skip_ref):
    obs = []
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.HasHole() or p.IsOnLayer(pcbnew.F_Cu if side_front else pcbnew.B_Cu):
                bb = p.GetBoundingBox(); bb.Inflate(FromMM(0.15)); obs.append(bb)
        for g in silk_shapes(f):
            if (g.GetLayer() == pcbnew.F_SilkS) == side_front:
                if g.GetShape() == pcbnew.SHAPE_T_SEGMENT:
                    # thin bbox per segment (split long diagonals into pieces)
                    s, e = g.GetStart(), g.GetEnd(); n = max(1, int(M((e - s).EuclideanNorm()) / 1.0))
                    for k in range(n):
                        a_ = V(s.x + (e.x - s.x) * k // n, s.y + (e.y - s.y) * k // n); c_ = V(s.x + (e.x - s.x) * (k + 1) // n, s.y + (e.y - s.y) * (k + 1) // n)
                        bb = pcbnew.BOX2I(V(min(a_.x, c_.x), min(a_.y, c_.y)), V(abs(c_.x - a_.x) + 1, abs(c_.y - a_.y) + 1)); bb.Inflate(g.GetWidth() // 2 + FromMM(0.1)); obs.append(bb)
                else:
                    bb = g.GetBoundingBox(); bb.Inflate(FromMM(0.1)); obs.append(bb)
        if f.GetReference() != skip_ref:
            r = f.Reference()
            if r.IsVisible() and (r.GetLayer() == pcbnew.F_SilkS) == side_front:
                bb = r.GetBoundingBox(); bb.Inflate(FromMM(0.1)); obs.append(bb)
    return obs
moved = 0
for ref in sorted(flag):
    f = F[ref]; r = f.Reference()
    front = r.GetLayer() == pcbnew.F_SilkS
    obs = obstacles(front, ref)
    cb = f.GetCourtyard(pcbnew.F_CrtYd if not f.IsFlipped() else pcbnew.B_CrtYd).BBox()
    if cb.GetWidth() == 0: cb = f.GetBoundingBox(False, False)
    cx, cy = cb.Centre().x, cb.Centre().y
    orig_pos, orig_ang = r.GetPosition(), r.GetTextAngleDegrees()
    best = None
    for ang in (orig_ang, (orig_ang + 90) % 180):
        r.SetTextAngleDegrees(ang)
        r.SetPosition(V(cx, cy)); tb = r.GetBoundingBox(); hw, hh = tb.GetWidth() // 2, tb.GetHeight() // 2
        for d in [FromMM(x) for x in (0.15, 0.5, 1.0, 1.5, 2.0, 3.0)]:
            cands = [(cx, cb.GetTop() - hh - d), (cx, cb.GetBottom() + hh + d), (cb.GetLeft() - hw - d, cy), (cb.GetRight() + hw + d, cy),
                     (cb.GetLeft() - hw - d, cb.GetTop() - hh - d), (cb.GetRight() + hw + d, cb.GetTop() - hh - d),
                     (cb.GetLeft() - hw - d, cb.GetBottom() + hh + d), (cb.GetRight() + hw + d, cb.GetBottom() + hh + d)]
            for (x, y) in cands:
                r.SetPosition(V(int(x), int(y))); tb = r.GetBoundingBox()
                if tb.GetLeft() < ib[0] or tb.GetTop() < ib[1] or tb.GetRight() > ib[2] or tb.GetBottom() > ib[3]: continue
                if any(tb.Intersects(o) for o in obs): continue
                dist = (x - cx) ** 2 + (y - cy) ** 2 + (0 if ang == orig_ang else FromMM(1.0) ** 2)
                if best is None or dist < best[0]: best = (dist, int(x), int(y), ang)
            if best: break
        if best and best[3] == orig_ang: break
    if best:
        r.SetTextAngleDegrees(best[3]); r.SetPosition(V(best[1], best[2])); moved += 1
        print('  %-6s -> (%.2f, %.2f) angle %d' % (ref, M(best[1]), M(best[2]), best[3]))
    else:
        r.SetTextAngleDegrees(orig_ang); r.SetPosition(orig_pos); print('  %-6s no free spot' % ref)
print('moved', moved, '/', len(flag))
b.Save(out)
