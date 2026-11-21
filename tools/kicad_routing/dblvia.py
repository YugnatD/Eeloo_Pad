import pcbnew, sys, math
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
NETS={'/VBUS','+12V','/V5_DC','Net-(F1-Pad1)','/VBUS_USB','Net-(F2-Pad1)','Net-(D11-A)'}
tracks=list(b.GetTracks())
pads=[p for f in b.GetFootprints() for p in f.Pads()]
def clear(net,pos,r,layers,skip=()):
    c=pcbnew.SHAPE_CIRCLE(pos,r)
    for t in b.GetTracks():
        if t.GetNetname()==net or t in skip: continue
        for L in layers:
            if t.IsOnLayer(L) and t.GetEffectiveShape(L).Collide(c,FromMM(0.2)): return False
    for p in pads:
        if p.GetNetname()==net and net: continue
        for L in layers:
            if p.IsOnLayer(L) and p.GetEffectiveShape(L).Collide(c,FromMM(0.2)): return False
        if p.HasHole() and (p.GetPosition()-pos).EuclideanNorm() < max(p.GetDrillSize().x,p.GetDrillSize().y)//2+FromMM(0.15+0.3): return False
    return True
def seg_clear(net,a,c,w,L):
    n=max(2,int(M(int((c-a).EuclideanNorm()))/0.1))
    for k in range(n+1):
        p=V(a.x+(c.x-a.x)*k//n,a.y+(c.y-a.y)*k//n)
        if not clear(net,p,w//2,(L,)): return False
    return True
added=0; skipped=[]
vias=[t for t in tracks if t.Type()==pcbnew.PCB_VIA_T and t.GetNetname() in NETS]
for v in vias:
    net=v.GetNetname(); P=v.GetPosition()
    att={pcbnew.F_Cu:[],pcbnew.B_Cu:[]}
    for t in tracks:
        if t.Type()==pcbnew.PCB_TRACE_T and t.GetNetname()==net and t.GetEffectiveShape(t.GetLayer()).Collide(pcbnew.SHAPE_CIRCLE(P,FromMM(0.3)),0):
            att[t.GetLayer()].append(t)
    wmax=max([M(t.GetWidth()) for L in att for t in att[L]] or [0])
    if wmax<0.5: continue
    # already doubled? another via of same net within 1.2 mm
    if any(o.m_Uuid.AsString()!=v.m_Uuid.AsString() and o.Type()==pcbnew.PCB_VIA_T and o.GetNetname()==net and (o.GetPosition()-P).EuclideanNorm()<FromMM(1.2) for o in b.GetTracks()): continue
    done=False
    for L1 in (pcbnew.F_Cu,pcbnew.B_Cu):
        L2=pcbnew.B_Cu if L1==pcbnew.F_Cu else pcbnew.F_Cu
        for t in att[L1]:
            far=t.GetEnd() if (t.GetStart()-P).EuclideanNorm()<(t.GetEnd()-P).EuclideanNorm() else t.GetStart()
            d=far-P; L=d.EuclideanNorm()
            if L<FromMM(1.0): continue
            for dist in (0.9,1.1,1.4):
                Q=V(P.x+int(d.x*FromMM(dist)/L),P.y+int(d.y*FromMM(dist)/L))
                w2=max([t2.GetWidth() for t2 in att[L2]] or [t.GetWidth()])
                if not clear(net,Q,FromMM(0.3),(pcbnew.F_Cu,pcbnew.B_Cu)): continue
                if any(o.Type()==pcbnew.PCB_VIA_T and o.m_Uuid.AsString()!=v.m_Uuid.AsString() and (o.GetPosition()-Q).EuclideanNorm()<FromMM(0.3+0.3+0.25) for o in b.GetTracks()): continue
                if not seg_clear(net,P,Q,w2,L2): continue
                nv=pcbnew.PCB_VIA(b); nv.SetPosition(Q); nv.SetWidth(FromMM(0.6)); nv.SetDrill(FromMM(0.3)); nv.SetNet(v.GetNet()); b.Add(nv)
                st=pcbnew.PCB_TRACK(b); st.SetStart(P); st.SetEnd(Q); st.SetWidth(w2); st.SetLayer(L2); st.SetNet(v.GetNet()); b.Add(st)
                added+=1; done=True; break
            if done: break
        if done: break
    if not done: skipped.append((net.split('/')[-1],round(M(P.x),1),round(M(P.y),1)))
print('vias doubled',added,'not possible',len(skipped),skipped)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
