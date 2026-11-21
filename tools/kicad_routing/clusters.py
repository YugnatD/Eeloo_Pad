import pcbnew
from pcbnew import ToMM as M
def clusters(b, net):
    b.BuildConnectivity(); c=b.GetConnectivity()
    items=[p for f in b.GetFootprints() for p in f.Pads() if p.GetNetname()==net]+[t for t in b.GetTracks() if t.GetNetname()==net]
    seen=set(); out=[]
    for it in items:
        k=it.m_Uuid.AsString()
        if k in seen: continue
        grp=[it]; seen.add(k)
        for o in c.GetConnectedItems(it):
            if o.GetNetname()!=net: continue
            kk=o.m_Uuid.AsString()
            if kk in seen: continue
            seen.add(kk); grp.append(o.Cast() if hasattr(o,'Cast') else o)
        out.append(grp)
    return out
def desc(g):
    pads=[x for x in g if x.GetClass()=='PAD']
    names=sorted(set('%s.%s'%(p.GetParentFootprint().GetReference(),p.GetNumber()) for p in pads))
    xs=[M(x.GetPosition().x) for x in g]; ys=[M(x.GetPosition().y) for x in g]
    return '(%.0f,%.0f) n=%d %s'%(sum(xs)/len(xs),sum(ys)/len(ys),len(g),' '.join(names[:8]))
