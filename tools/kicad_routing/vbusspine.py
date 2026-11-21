import sys; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters, desc
from keepouts import add_keepouts
from smooth import smooth
from safetrim import safe_trim
src,out=sys.argv[1],sys.argv[2]; Y=40.45
b=pcbnew.LoadBoard(src); net=b.FindNet('/VBUS')
# 1) remove VBUS copper in the top-row band
rm=0
for t in list(b.GetTracks()):
    if t.GetNetname()!='/VBUS': continue
    pts=[t.GetPosition()] if t.Type()==pcbnew.PCB_VIA_T else [t.GetStart(),t.GetEnd()]
    if all(58<=M(p.x)<=312 and 29.0<=M(p.y)<=41.6 for p in pts): b.Remove(t); rm+=1
print('removed',rm)
# 2) spine pieces (B) with F hops
def trk(a,c,L,w=0.6):
    t=pcbnew.PCB_TRACK(b); t.SetStart(V(FromMM(a),FromMM(Y))); t.SetEnd(V(FromMM(c),FromMM(Y))); t.SetWidth(FromMM(w)); t.SetLayer(L); t.SetNet(net); b.Add(t)
def via(x):
    v=pcbnew.PCB_VIA(b); v.SetPosition(V(FromMM(x),FromMM(Y))); v.SetWidth(FromMM(0.6)); v.SetDrill(FromMM(0.3)); v.SetNet(net); b.Add(v)
pieces=[[(60.0,72.4,'B'),(72.4,75.8,'F'),(75.8,171.6,'B')],[(173.6,212.2,'B')],[(216.6,222.9,'B'),(222.9,225.9,'F'),(225.9,233.5,'B')],[(270.1,310.5,'B')]]
for pc in pieces:
    for a,c,L in pc: trk(a,c,pcbnew.B_Cu if L=='B' else pcbnew.F_Cu)
    for (a,c,L),(a2,c2,L2) in zip(pc,pc[1:]):
        if L!=L2: via(c)
b.Save(out.replace('.kicad_pcb','_spine.kicad_pcb'))
# 3a) units: LED pad1 + its decoupling cap pad1, linked locally
RP=dict(clr=0.2,margin=0.05,grid=0.1)
def region(g,m):
    xs=[];ys=[]
    for it in g:
        bb=it.GetBoundingBox(); xs+=[M(bb.GetLeft()),M(bb.GetRight())]; ys+=[M(bb.GetTop()),M(bb.GetBottom())]
    return (max(14.45,min(xs)-m),max(9.95,min(ys)-m),min(366.35,max(xs)+m),min(238.05,max(ys)+m))
def try_route(terms, reg, ws=(0.6,0.5,0.4), vc=40):
    n0=len(clusters(b,'/VBUS'))
    for w in ws:
        R=Router(b,reg,**RP).build(w=w); add_keepouts(R,b)
        r,_=R.route_net('/VBUS',terms,w=w,via_cost=vc,turn_cost=1.5)
        if r and len(clusters(b,'/VBUS'))<n0: smooth(R,b,'/VBUS',R.new_items); return w
        for t in R.new_items: b.Remove(t)
    return None
top=[f for f in b.GetFootprints() if f.GetReference().startswith('LED') and abs(M(f.GetPosition().y)-32.004)<0.1]
spine=lambda: [t for t in b.GetTracks() if t.GetNetname()=='/VBUS' and t.Type()==pcbnew.PCB_TRACE_T and abs(M(t.GetStart().y)-Y)<0.01 and abs(M(t.GetEnd().y)-Y)<0.01]
for f in sorted(top,key=lambda f:f.GetPosition().x):
    x=M(f.GetPosition().x)
    lp=[p for p in f.Pads() if p.GetNumber()=='1'][0]
    caps=[p for g in b.GetFootprints() if g.GetReference().startswith('C') for p in g.Pads() if p.GetNetname()=='/VBUS' and abs(M(p.GetPosition().x)-x)<2 and 34<M(p.GetPosition().y)<36.5]
    unit=[lp]+caps[:1]
    w1=None
    if caps:
        w1=try_route([[lp],[caps[0]]], region(unit,3))
    # unit -> spine directly below
    g=[c for c in clusters(b,'/VBUS') if any(it.m_Uuid.AsString()==lp.m_Uuid.AsString() for it in c)][0]
    sp=[t for t in spine() if min(M(t.GetStart().x),M(t.GetEnd().x))-0.5<=x<=max(M(t.GetStart().x),M(t.GetEnd().x))+0.5]
    w2=None
    if sp:
        reg=(x-7,29,x+7,42.5)
        w2=try_route([g,sp],reg)
    print(f'  {f.GetReference():6s} x={x:7.2f} cap={caps[0].GetParentFootprint().GetReference() if caps else None} link={w1} spine={w2}',flush=True)
# 3b) everything left: smallest island first to the rest
tried=set()
while True:
    cl=sorted(clusters(b,'/VBUS'),key=len)
    if len(cl)<2: break
    cand=[g for g in cl[:-1] if desc(g) not in tried]
    if not cand: break
    g=cand[0]; ok=False
    for m in (8,15,25):
        reg=region(g,m)
        ins=lambda it:(M(it.GetBoundingBox().GetLeft())>reg[0]+1 and M(it.GetBoundingBox().GetRight())<reg[2]-1 and M(it.GetBoundingBox().GetTop())>reg[1]+1 and M(it.GetBoundingBox().GetBottom())<reg[3]-1)
        others=[it for h in cl if h is not g for it in h if ins(it)]
        if not others: continue
        for w in (0.6,0.5,0.4):
            R=Router(b,reg,**RP).build(w=w); add_keepouts(R,b)
            r,_=R.route_net('/VBUS',[g,others],w=w,via_cost=40,turn_cost=1.5)
            if r and len(clusters(b,'/VBUS'))<len(cl): smooth(R,b,'/VBUS',R.new_items); ok=True; break
            for t in R.new_items: b.Remove(t)
        if ok: break
    print(f'  {desc(g)[:50]:50s} m={m} w={w} ok={ok}',flush=True)
    if not ok: tried.add(desc(g))
print('VBUS islands',len(clusters(b,'/VBUS')))
print('safe_trim',safe_trim(b,{'/VBUS'}))
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
