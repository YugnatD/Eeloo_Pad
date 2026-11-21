"""shorten.py in out drc.txt : cut DRC-dangling track ends back to the last same-net copper they touch;
remove dangling vias that then touch copper on one layer only (connectivity-checked)."""
import sys, re; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from clusters import clusters
src,out,drc=sys.argv[1:4]
b=pcbnew.LoadBoard(src); rep=open(drc).read()
def touches(net, L, pt, excl):
    c=pcbnew.SHAPE_CIRCLE(pt, FromMM(0.01))
    for u in b.GetTracks():
        if u.GetNetname()==net and u.m_Uuid.AsString() not in excl and u.IsOnLayer(L) and u.GetEffectiveShape(L).Collide(c,0): return True
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetname()==net and p.IsOnLayer(L) and p.GetEffectiveShape(L).Collide(c,0): return True
    return False
done=0
for blk in re.split(r'\n(?=\[)', rep):
    if not (blk.startswith('[track_dangling]') or blk.startswith('[via_dangling]')): continue
    m=re.search(r'@\(([\d.]+) mm, ([\d.]+) mm\): (Track|Via) \[([^\]]+)\]', blk)
    x,y,kind,net=float(m.group(1)),float(m.group(2)),m.group(3),m.group(4)
    if net=='GND': continue
    c=V(FromMM(x),FromMM(y))
    if kind=='Via':
        v=[t for t in b.GetTracks() if t.Type()==pcbnew.PCB_VIA_T and t.GetNetname()==net and (t.GetPosition()-c).EuclideanNorm()<FromMM(0.02)]
        if v:
            n0=len(clusters(b,net)); b.Remove(v[0])
            if len(clusters(b,net))>n0: b.Add(v[0])
            else: done+=1; print('via removed',net,x,y)
        continue
    for t in [t for t in b.GetTracks() if t.Type()==pcbnew.PCB_TRACE_T and t.GetNetname()==net]:
        if (t.GetStart()-c).EuclideanNorm()<FromMM(0.02): D,O,setD=t.GetStart(),t.GetEnd(),t.SetStart
        elif (t.GetEnd()-c).EuclideanNorm()<FromMM(0.02): D,O,setD=t.GetEnd(),t.GetStart(),t.SetEnd
        else: continue
        L=t.GetLayer(); ex={t.m_Uuid.AsString()}; N=200; P=None
        for k in range(N+1):
            q=V(int(D.x+(O.x-D.x)*k/N), int(D.y+(O.y-D.y)*k/N))
            if touches(net,L,q,ex): P=q; break
        if P is None or (P-O).EuclideanNorm()<FromMM(0.02):
            n0=len(clusters(b,net)); b.Remove(t)
            if len(clusters(b,net))>n0: b.Add(t)
            else: done+=1; print('track removed',net,x,y)
        else:
            setD(P); done+=1; print('track shortened',net,x,y,'->',round(M(P.x),3),round(M(P.y),3))
        break
print('fixed',done)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
