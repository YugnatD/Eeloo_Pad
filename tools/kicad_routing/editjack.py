import pcbnew, sys
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
near=lambda p,q: abs(M(p.x)-q[0])<0.005 and abs(M(p.y)-q[1])<0.005
def seg(t,a,c): return (near(t.GetStart(),a) and near(t.GetEnd(),c)) or (near(t.GetStart(),c) and near(t.GetEnd(),a))
rm=[((356.236,68.834),(356.236,67.726)),((355.223,68.739),(356.236,67.726)),((357.926,70.524),(356.236,68.834))]
n=0
for t in list(b.GetTracks()):
    if t.Type()!=pcbnew.PCB_TRACE_T: continue
    if t.GetNetname()=='/VBUS' and any(seg(t,a,c) for a,c in rm): b.Remove(t); n+=1
    elif t.GetNetname()=='/VBUS' and seg(t,(354.001,68.739),(355.223,68.739)):
        if near(t.GetStart(),(355.223,68.739)): t.SetStart(V(FromMM(355.0),FromMM(68.74)))
        else: t.SetEnd(V(FromMM(355.0),FromMM(68.74)))
        n+=1
# R123.1 (now VBUS_USB) : drop the HUB_V33 copper touching it
p=[q for q in b.FindFootprintByReference('R123').Pads() if q.GetNumber()=='1'][0]
for t in list(b.GetTracks()):
    if t.GetNetname()=='/HUB_V33' and t.Type()==pcbnew.PCB_TRACE_T and (p.HitTest(t.GetStart()) or p.HitTest(t.GetEnd())):
        print('rm HUB_V33',(M(t.GetStart().x),M(t.GetStart().y)),(M(t.GetEnd().x),M(t.GetEnd().y))); b.Remove(t); n+=1
print('edited',n)
def track(net,pts,w,L=pcbnew.F_Cu):
    for a,c in zip(pts,pts[1:]):
        t=pcbnew.PCB_TRACK(b); t.SetStart(V(FromMM(a[0]),FromMM(a[1]))); t.SetEnd(V(FromMM(c[0]),FromMM(c[1]))); t.SetWidth(FromMM(w)); t.SetLayer(L); t.SetNet(b.FindNet(net)); b.Add(t)
track('/VBUS_USB',[(356.236,67.726),(355.84,68.122),(355.84,69.66)],0.5)
track('/VBUS',[(356.79,71.5375),(360.0,71.5375),(360.0,70.524)],0.5)
for t in b.GetTracks():
    if t.GetNetname()=='/VBUS' and t.Type()==pcbnew.PCB_TRACE_T and seg(t,(357.926,70.524),(361.757,70.524)):
        if near(t.GetStart(),(357.926,70.524)): t.SetStart(V(FromMM(360.0),FromMM(70.524)))
        else: t.SetEnd(V(FromMM(360.0),FromMM(70.524)))
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
