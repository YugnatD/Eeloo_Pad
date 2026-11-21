import pcbnew
from pcbnew import FromMM
def trim(b, nets, protect=()):
    """remove dangling track ends and one-layer vias on the given nets (iterative);
    a one-layer via joining >=2 tracks is replaced by short bridges to its centre"""
    total=0
    pads={}
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetname() in nets: pads.setdefault(p.GetNetname(),[]).append(p)
    while True:
        items={}
        for t in b.GetTracks():
            if t.GetNetname() in nets: items.setdefault(t.GetNetname(),[]).append(t)
        rm=[]; done=set()
        for n,its in items.items():
            for t in its:
                if t.m_Uuid.AsString() in protect: continue
                if t.Type()==pcbnew.PCB_VIA_T:
                    c=pcbnew.SHAPE_CIRCLE(t.GetPosition(),FromMM(0.295)); lay=set()
                    for o in its:
                        if o is t or o.Type()==pcbnew.PCB_VIA_T: continue
                        if o.GetEffectiveShape(o.GetLayer()).Collide(c,0): lay.add(o.GetLayer())
                    if any(p.HitTest(t.GetPosition()) for p in pads.get(n,[])): lay|={pcbnew.F_Cu,pcbnew.B_Cu}
                    if len(lay)<2:
                        touching=[o for o in its if o is not t and o.Type()!=pcbnew.PCB_VIA_T and o.GetEffectiveShape(o.GetLayer()).Collide(c,0)]
                        if len(touching)>=2:
                            pos=t.GetPosition()
                            for o in touching:
                                e=min((o.GetStart(),o.GetEnd()),key=lambda q:(q-pos).EuclideanNorm())
                                if (e-pos).EuclideanNorm()>FromMM(0.001):
                                    nt=pcbnew.PCB_TRACK(b); nt.SetStart(e); nt.SetEnd(pos); nt.SetWidth(o.GetWidth()); nt.SetLayer(o.GetLayer()); nt.SetNet(o.GetNet()); b.Add(nt)
                        rm.append(t)
                    continue
                for e in (t.GetStart(),t.GetEnd()):
                    c=pcbnew.SHAPE_CIRCLE(e,max(1,t.GetWidth()//2-FromMM(0.005))); ok=False
                    for o in its:
                        if o is t: continue
                        if o.Type()==pcbnew.PCB_VIA_T:
                            if (o.GetPosition()-e).EuclideanNorm()<FromMM(0.3)+t.GetWidth()//2: ok=True; break
                        elif o.GetLayer()==t.GetLayer() and o.GetEffectiveShape(o.GetLayer()).Collide(c,0): ok=True; break
                    if not ok:
                        for p in pads.get(n,[]):
                            if p.IsOnLayer(t.GetLayer()) and p.GetEffectiveShape(t.GetLayer()).Collide(c,0): ok=True; break
                    if not ok: rm.append(t); break
        if not rm: return total
        for t in rm:
            try: b.Remove(t)
            except Exception: pass
        total+=len(rm)
