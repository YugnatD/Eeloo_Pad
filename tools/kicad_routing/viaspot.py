import pcbnew, math
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
def free(b, net, x, y, r=0.3, clr=0.2, hole_clr=0.3, layers=(pcbnew.F_Cu,pcbnew.B_Cu)):
    c=pcbnew.SHAPE_CIRCLE(V(FromMM(x),FromMM(y)),FromMM(r))
    for t in b.GetTracks():
        if t.GetNetname()==net: continue
        for L in layers:
            if t.IsOnLayer(L) and t.GetEffectiveShape(L).Collide(c,FromMM(clr)): return False
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetname()==net and net!='': continue
            for L in layers:
                if p.IsOnLayer(L) and p.GetEffectiveShape(L).Collide(c,FromMM(clr)): return False
            if p.HasHole() and r>=0.25:
                d=math.hypot(M(p.GetPosition().x)-x,M(p.GetPosition().y)-y)
                if d < M(max(p.GetDrillSize().x,p.GetDrillSize().y))/2+0.15+hole_clr: return False
    if r>=0.25:
        for t in b.GetTracks():
            if t.Type()==pcbnew.PCB_VIA_T and math.hypot(M(t.GetPosition().x)-x,M(t.GetPosition().y)-y)<0.3+0.15+0.25: return False
    return True
