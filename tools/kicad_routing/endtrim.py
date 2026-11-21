"""endtrim.py in out NET [NET...]: cut free track ends back to the last same-net contact (no DRC report needed)."""
import sys; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from clusters import clusters
src,out=sys.argv[1],sys.argv[2]; nets=sys.argv[3:]
b=pcbnew.LoadBoard(src)
def touches(net,L,pt,ex):
    c=pcbnew.SHAPE_CIRCLE(pt,FromMM(0.005))
    for u in b.GetTracks():
        if u.GetNetname()==net and u.m_Uuid.AsString()!=ex and u.IsOnLayer(L) and u.GetEffectiveShape(L).Collide(c,0): return True
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetname()==net and p.IsOnLayer(L) and p.GetEffectiveShape(L).Collide(c,0): return True
    return False
tot=0
for net in nets:
    changed=True
    while changed:
        changed=False
        for t in [t for t in b.GetTracks() if t.GetNetname()==net and t.Type()==pcbnew.PCB_TRACE_T]:
            L=t.GetLayer(); u=t.m_Uuid.AsString()
            for D,O,setD in ((t.GetStart(),t.GetEnd(),t.SetStart),(t.GetEnd(),t.GetStart(),t.SetEnd)):
                if touches(net,L,D,u): continue
                N=max(20,int(M((O-D).EuclideanNorm())/0.05)); P=None
                for k in range(1,N+1):
                    q=V(int(D.x+(O.x-D.x)*k/N),int(D.y+(O.y-D.y)*k/N))
                    if touches(net,L,q,u): P=q; break
                n0=len(clusters(b,net))
                if P is None or (P-O).EuclideanNorm()<FromMM(0.02):
                    b.Remove(t)
                    if len(clusters(b,net))>n0: b.Add(t); continue
                    print('removed',net,round(M(D.x),2),round(M(D.y),2)); tot+=1; changed=True; break
                old=V(D.x,D.y); setD(P)
                if len(clusters(b,net))>n0: setD(old); continue
                print('cut',net,round(M(D.x),2),round(M(D.y),2),'->',round(M(P.x),2),round(M(P.y),2)); tot+=1; changed=True; break
            if changed: break
print('fixed',tot)
b.Save(out)
