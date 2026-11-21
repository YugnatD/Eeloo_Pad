SILK = (pcbnew.F_SilkS, pcbnew.B_SilkS)
def silk_shapes(f):
    return [g for g in f.GraphicalItems() if g.GetClass() == 'PCB_SHAPE' and g.GetLayer() in SILK]

edge = b.GetBoardEdgesBoundingBox()
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

def place(f, allow_center=False):
    ref=f.GetReference(); r=f.Reference(); front=r.GetLayer()==pcbnew.F_SilkS
    obs=obstacles(front, ref)
    own=[]
    for g in silk_shapes(f):
        bb=g.GetBoundingBox(); bb.Inflate(FromMM(0.1)); own.append(bb)
    cb=f.GetCourtyard(pcbnew.F_CrtYd if not f.IsFlipped() else pcbnew.B_CrtYd).BBox()
    cx,cy=cb.Centre().x,cb.Centre().y
    best=None
    for ang in (r.GetTextAngleDegrees(),(r.GetTextAngleDegrees()+90)%180):
        r.SetTextAngleDegrees(ang); r.SetPosition(V(cx,cy)); tb=r.GetBoundingBox(); hw,hh=tb.GetWidth()//2,tb.GetHeight()//2
        cands=[]
        if allow_center: cands.append((cx,cy,True))
        for d in [FromMM(x) for x in (0.1,0.4,0.8,1.2,2.0)]:
            cands+=[(cx,cb.GetTop()-hh-d,False),(cx,cb.GetBottom()+hh+d,False),(cb.GetLeft()-hw-d,cy,False),(cb.GetRight()+hw+d,cy,False)]
        for x,y,center in cands:
            r.SetPosition(V(int(x),int(y))); tb=r.GetBoundingBox()
            if tb.GetLeft()<ib[0] or tb.GetTop()<ib[1] or tb.GetRight()>ib[2] or tb.GetBottom()>ib[3]: continue
            pool=[o for o in obs if not any(o==q for q in own)] if center else obs
            if any(tb.Intersects(o) for o in pool): continue
            dd=(x-cx)**2+(y-cy)**2
            if best is None or dd<best[0]: best=(dd,int(x),int(y),ang)
    if best: r.SetTextAngleDegrees(best[3]); r.SetPosition(V(best[1],best[2])); return True
    return False
