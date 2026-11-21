"""Rip-up & retry: ripup.py in out NET REF.pad REF.pad [margin]
Removes other-net tracks/vias crossing the box around the two pads, routes NET between them,
then reroutes every ripped net. Saves only if everything is connected again."""
import sys, os; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters, desc
from keepouts import add_keepouts
from smooth import smooth
src,out,net,pa,pb=sys.argv[1:6]; mg=float(sys.argv[6]) if len(sys.argv)>6 else 2.0
b=pcbnew.LoadBoard(src)
RP=dict(clr=0.2, margin=0.05, grid=0.1)
def pad(s):
    r,n=s.split('.'); return [p for p in b.FindFootprintByReference(r).Pads() if p.GetNumber()==n][0]
A,B=pad(pa),pad(pb)
x0=min(M(A.GetPosition().x),M(B.GetPosition().x))-mg; x1=max(M(A.GetPosition().x),M(B.GetPosition().x))+mg
y0=min(M(A.GetPosition().y),M(B.GetPosition().y))-mg; y1=max(M(A.GetPosition().y),M(B.GetPosition().y))+mg
box=pcbnew.SHAPE_RECT(V(FromMM(x0),FromMM(y0)),FromMM(x1-x0),FromMM(y1-y0))
width={}
for t in b.GetTracks():
    if t.Type()==pcbnew.PCB_TRACE_T: width[t.GetNetname()]=max(width.get(t.GetNetname(),0),M(t.GetWidth()))
ripped=set()
for t in list(b.GetTracks()):
    n=t.GetNetname()
    if n in (net,'GND',''): continue
    if os.environ.get('RIP') and n not in os.environ['RIP'].split(','): continue
    L=t.GetLayer() if t.Type()!=pcbnew.PCB_VIA_T else pcbnew.F_Cu
    if t.GetEffectiveShape(L).Collide(box,0): ripped.add(n); b.Remove(t)
print('ripped',sorted(ripped))
def region(groups,m):
    xs=[];ys=[]
    for g in groups:
        for it in g:
            bb=it.GetBoundingBox(); xs+=[M(bb.GetLeft()),M(bb.GetRight())]; ys+=[M(bb.GetTop()),M(bb.GetBottom())]
    return (max(14.45,min(xs)-m),max(9.95,min(ys)-m),min(366.35,max(xs)+m),min(238.05,max(ys)+m))
def connect(n, ws):
    while True:
        cl=sorted(clusters(b,n),key=len)
        if len(cl)<2: return True
        g=cl[0]; reg=region([g],20)
        ins=lambda it:(M(it.GetBoundingBox().GetLeft())>reg[0]+1 and M(it.GetBoundingBox().GetRight())<reg[2]-1 and M(it.GetBoundingBox().GetTop())>reg[1]+1 and M(it.GetBoundingBox().GetBottom())<reg[3]-1)
        others=[it for h in cl[1:] for it in h if ins(it)]
        ok=False
        for w in ws:
            R=Router(b,reg,**RP).build(w=w); add_keepouts(R,b)
            r,_=R.route_net(n,[g,others],w=w,via_cost=30,turn_cost=1.0)
            if r and len(clusters(b,n))<len(cl): smooth(R,b,n,R.new_items); ok=True; break
            for t in R.new_items: b.Remove(t)
        if not ok: print('  FAIL',n,desc(g)[:50]); return False
wl=lambda n:[x for x in (width.get(n,0.2),0.4,0.3,0.25,0.2) if x<=width.get(n,0.2)+1e-6]
import os
order=sorted(ripped, key=lambda n:(n=='/VBUS',n))
seq=([net]+order) if os.environ.get('ORDER','first')=='first' else (order[:-1]+[net]+order[-1:] if os.environ.get('ORDER')=='beforevbus' else order+[net])
allok=True
for n in seq:
    r=connect(n, wl(n) if n!='/VBUS' else (0.5,0.4,0.3)); print('  route',n,r); allok&=r
if allok:
    pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out); print('SAVED',out)
else: print('NOT saved')
