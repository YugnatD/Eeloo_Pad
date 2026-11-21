"""padfree.py in out REF.pad NET [radius]: reroute NET locally keeping a free area right/below the pad (so GND fill reaches it)."""
import sys; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters
from keepouts import add_keepouts
from smooth import smooth
src,out,ps,N=sys.argv[1:5]; rad=float(sys.argv[5]) if len(sys.argv)>5 else 3.2
b=pcbnew.LoadBoard(src)
r,n=ps.split('.'); p3=[p for p in b.FindFootprintByReference(r).Pads() if p.GetNumber()==n][0]; c=p3.GetPosition()
n0=len(clusters(b,N)); rem=[]
for t in list(b.GetTracks()):
    if t.GetNetname()!=N: continue
    pts=[t.GetPosition()] if t.Type()==pcbnew.PCB_VIA_T else [t.GetStart(),t.GetEnd()]
    if all((q-c).EuclideanNorm()<FromMM(rad) for q in pts): rem.append(t); b.Remove(t)
box=pcbnew.SHAPE_POLY_SET(); ol=pcbnew.SHAPE_LINE_CHAIN()
for dx,dy in ((-0.9,-0.55),(2.2,-0.55),(2.2,2.2),(-0.9,2.2)): ol.Append(V(c.x+FromMM(dx),c.y+FromMM(dy)))
ol.SetClosed(True); box.AddOutline(ol)
cx,cy=M(c.x),M(c.y); ok=False
cl=sorted(clusters(b,N),key=len)
for reg in ((cx-14,cy-10,cx+12,cy+10),(cx-25,cy-18,cx+14,cy+14)):
    reg=(max(14.45,reg[0]),max(9.95,reg[1]),min(366.35,reg[2]),min(238.05,reg[3]))
    Rt=Router(b,reg,clr=0.2,margin=0.05,grid=0.1).build(w=0.2); add_keepouts(Rt,b)
    for L in 'FB':
        for k in 'tv': Rt._add('__keepout__',L,k,Rt._draw_polyset(pcbnew.SHAPE_POLY_SET(box)))
    rr,_=Rt.route_net(N,cl,w=0.2,via_cost=40,turn_cost=1.5)
    if rr and len(clusters(b,N))<=n0: smooth(Rt,b,N,Rt.new_items); ok=True; break
    for t in Rt.new_items: b.Remove(t)
if not ok:
    for t in rem: b.Add(t)
print(ps,N,'removed',len(rem),'ok',ok)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
