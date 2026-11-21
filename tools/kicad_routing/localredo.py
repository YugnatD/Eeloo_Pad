"""localredo.py in out 'NET@x,y,r[@vc]' ... : rip NET copper fully inside circle (x,y,r) (pads untouched),
reconnect with the router (no via in SMD pads), smooth; revert that spec if it fails."""
import sys; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters, desc
from keepouts import add_keepouts
from smooth import smooth
src,out=sys.argv[1],sys.argv[2]; specs=sys.argv[3:]
b=pcbnew.LoadBoard(src)
RP=dict(clr=0.2,margin=0.05,grid=0.1)
width={}
for t in b.GetTracks():
    if t.Type()==pcbnew.PCB_TRACE_T: width[t.GetNetname()]=max(width.get(t.GetNetname(),0),M(t.GetWidth()))
def region(groups,m):
    xs=[];ys=[]
    for g in groups:
        for it in g:
            bb=it.GetBoundingBox(); xs+=[M(bb.GetLeft()),M(bb.GetRight())]; ys+=[M(bb.GetTop()),M(bb.GetBottom())]
    return (max(14.45,min(xs)-m),max(9.95,min(ys)-m),min(366.35,max(xs)+m),min(238.05,max(ys)+m))
res=[]
for sp in specs:
    parts=sp.split('@'); net=parts[0]; x,y,r=[float(v) for v in parts[1].split(',')]; vc=float(parts[2]) if len(parts)>2 else 60
    C=V(FromMM(x),FromMM(y)); R2=FromMM(r)
    inside=lambda p:(p-C).EuclideanNorm()<=R2
    n0=len(clusters(b,net))
    # widths used locally
    loc=[t for t in b.GetTracks() if t.GetNetname()==net and (inside(t.GetPosition()) if t.Type()==pcbnew.PCB_VIA_T else (inside(t.GetStart()) and inside(t.GetEnd())))]
    wl=sorted({round(M(t.GetWidth()),3) for t in loc if t.Type()==pcbnew.PCB_TRACE_T},reverse=True) or [width.get(net,0.2)]
    for t in loc: b.Remove(t)
    ok=True; added=[]
    while True:
        cl=sorted(clusters(b,net),key=len)
        if len(cl)<=n0: break
        g=cl[0]; done=False
        for m in (4,8,15):
            reg=region([g],m); reg=(min(reg[0],x-r-2),min(reg[1],y-r-2),max(reg[2],x+r+2),max(reg[3],y+r+2))
            ins=lambda it:(M(it.GetBoundingBox().GetRight())>reg[0]+0.5 and M(it.GetBoundingBox().GetLeft())<reg[2]-0.5 and M(it.GetBoundingBox().GetBottom())>reg[1]+0.5 and M(it.GetBoundingBox().GetTop())<reg[3]-0.5)
            oth=[it for h in cl[1:] for it in h if ins(it)]
            if not oth: continue
            for w in [wl[0]]+[v for v in (0.4,0.3,0.25,0.2) if v<wl[0]-1e-6]:
                Rt=Router(b,reg,**RP).build(w=w); add_keepouts(Rt,b)
                rr,_=Rt.route_net(net,[g,oth],w=w,via_cost=vc,turn_cost=1.5)
                if rr and len(clusters(b,net))<len(cl):
                    added+=smooth(Rt,b,net,Rt.new_items); done=True; break
                for t in Rt.new_items: b.Remove(t)
            if done: break
        if not done: ok=False; break
    if not ok:
        for t in [t for t in b.GetTracks() if any(t.m_Uuid.AsString()==a.m_Uuid.AsString() for a in added)]: b.Remove(t)
        for t in loc: b.Add(t)
    nv=sum(1 for a in added if a.Type()==pcbnew.PCB_VIA_T)
    res.append((sp,ok,len(loc),len(added),nv)); print(f'{sp:45s} ok={ok} removed={len(loc)} added={len(added)} vias={nv}',flush=True)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
