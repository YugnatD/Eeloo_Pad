"""cellfix.py in out LEDref [LEDref...]: clean VBUS feed for one top-row LED cell (LED pad1 + cap -> spine y=40.45)."""
import sys, os; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters, desc
from keepouts import add_keepouts
from smooth import smooth
from safetrim import safe_trim
src,out=sys.argv[1],sys.argv[2]; refs=sys.argv[3:]; Y=40.45
b=pcbnew.LoadBoard(src)
RP=dict(clr=0.2,margin=0.05,grid=0.1)
width={}
for t in b.GetTracks():
    if t.Type()==pcbnew.PCB_TRACE_T: width[t.GetNetname()]=max(width.get(t.GetNetname(),0),M(t.GetWidth()))
def region(g,m):
    xs=[];ys=[]
    for it in g:
        bb=it.GetBoundingBox(); xs+=[M(bb.GetLeft()),M(bb.GetRight())]; ys+=[M(bb.GetTop()),M(bb.GetBottom())]
    return (max(14.45,min(xs)-m),max(9.95,min(ys)-m),min(366.35,max(xs)+m),min(238.05,max(ys)+m))
def route(net,terms,reg,ws,vc=40):
    n0=len(clusters(b,net))
    for w in ws:
        R=Router(b,reg,**RP).build(w=w); add_keepouts(R,b)
        r,_=R.route_net(net,terms,w=w,via_cost=vc,turn_cost=1.5)
        if r and len(clusters(b,net))<n0: smooth(R,b,net,R.new_items); return w
        for t in R.new_items: b.Remove(t)
    return None
def connect(net, ws):
    global n_; n_=net
    while True:
        cl=sorted(clusters(b,net),key=len)
        if len(cl)<2: return True
        g=cl[0]; ok=False
        for m in (8,15,25):
            reg=region(g,m)
            if os.environ.get('RYMIN') and n_!='/VBUS': reg=(reg[0],max(reg[1],float(os.environ['RYMIN'])),reg[2],reg[3])
            ins=lambda it:(M(it.GetBoundingBox().GetRight())>reg[0]+0.5 and M(it.GetBoundingBox().GetLeft())<reg[2]-0.5 and M(it.GetBoundingBox().GetBottom())>reg[1]+0.5 and M(it.GetBoundingBox().GetTop())<reg[3]-0.5)
            others=[it for h in cl[1:] for it in h if ins(it)]
            if others and route(net,[g,others],reg,ws): ok=True; break
        if not ok: print('   FAIL',net,desc(g)[:50]); return False
spine=lambda: [t for t in b.GetTracks() if t.GetNetname()=='/VBUS' and t.Type()==pcbnew.PCB_TRACE_T and abs(M(t.GetStart().y)-Y)<0.01 and abs(M(t.GetEnd().y)-Y)<0.01 and M(t.GetWidth())>0.55]
allok=True
for ref in refs:
    f=b.FindFootprintByReference(ref); x=M(f.GetPosition().x)
    lp=[p for p in f.Pads() if p.GetNumber()=='1'][0]
    cap=[p for g in b.GetFootprints() if g.GetReference().startswith('C') for p in g.Pads() if p.GetNetname()=='/VBUS' and abs(M(p.GetPosition().x)-x)<2 and 34<M(p.GetPosition().y)<36.5][0]
    spids={t.m_Uuid.AsString() for t in spine()}
    # remove VBUS copper of this cell (not the spine)
    for t in list(b.GetTracks()):
        if t.GetNetname()!='/VBUS' or t.m_Uuid.AsString() in spids: continue
        pts=[t.GetPosition()] if t.Type()==pcbnew.PCB_VIA_T else [t.GetStart(),t.GetEnd()]
        if all(x-9.4<=M(p.x)<=x+9.4 and 23<=M(p.y)<=40.2 for p in pts): b.Remove(t)
    # rip other nets crossing the feed column
    box=pcbnew.SHAPE_RECT(V(FromMM(x-3.6),FromMM(33.5)),FromMM(5.2),FromMM(7.2))
    ripped=set()
    for t in list(b.GetTracks()):
        n=t.GetNetname()
        if n in ('/VBUS','GND',''): continue
        import os
        fr=os.environ.get('FRIP','').split(',')
        if t.Type()==pcbnew.PCB_TRACE_T and t.GetLayer()!=pcbnew.B_Cu and n not in fr: continue
        L=t.GetLayer() if t.Type()==pcbnew.PCB_TRACE_T else pcbnew.B_Cu
        if os.environ.get('NORIP') or n in os.environ.get('KEEP','').split(','): continue
        if t.GetEffectiveShape(L).Collide(box,0): ripped.add(n); b.Remove(t)
    w1=route('/VBUS',[[lp],[cap]],region([lp,cap],3),(0.6,0.5,0.4))
    g=[c for c in clusters(b,'/VBUS') if any(it.m_Uuid.AsString()==lp.m_Uuid.AsString() for it in c)][0]
    near=sorted(spine(),key=lambda t:min(abs(M(t.GetStart().x)-x),abs(M(t.GetEnd().x)-x)) if not (min(M(t.GetStart().x),M(t.GetEnd().x))<=x<=max(M(t.GetStart().x),M(t.GetEnd().x))) else 0)
    SR=float(os.environ.get('SR','16'))
    if os.environ.get('ANYV'):
        reg=(x-SR,29,x+SR,float(os.environ.get('YMAX','50')))
        gid={it.m_Uuid.AsString() for it in g}
        ins=lambda it:(M(it.GetBoundingBox().GetRight())>reg[0]+0.5 and M(it.GetBoundingBox().GetLeft())<reg[2]-0.5 and M(it.GetBoundingBox().GetBottom())>reg[1]+0.5 and M(it.GetBoundingBox().GetTop())<reg[3]-0.5)
        oth=[it for c in clusters(b,'/VBUS') for it in c if it.m_Uuid.AsString() not in gid and ins(it)]
        w2=route('/VBUS',[g,oth],reg,(0.6,0.5,0.4))
    else:
        w2=route('/VBUS',[g,near[:2]],(x-SR,29,x+SR,44),(0.6,0.5,0.4))
    print(f'{ref}: link={w1} spine={w2} ripped={sorted(ripped)}',flush=True)
    for n in sorted(ripped):
        r=connect(n,[w for w in (width.get(n,0.2),0.4,0.3,0.25,0.2) if w<=width.get(n,0.2)+1e-6]); allok&=r
    allok&=connect('/VBUS',(0.6,0.5,0.4))
    allok&= w2 is not None
print('safe_trim',safe_trim(b,{'/VBUS'}|set()))
if allok: pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out); print('SAVED')
else: print('NOT saved')
