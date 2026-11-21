import sys, time; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from router import Router
from clusters import clusters, desc
from trim import trim
from safetrim import safe_trim
import os
RP=dict(clr=0.2, margin=0.05, grid=float(os.environ.get('GRID','0.1')))
from keepouts import add_keepouts
from smooth import smooth
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
leds=[b.FindFootprintByReference(f'LED{i}') for i in range(1,26)]
width={}
for t in list(b.GetTracks()):
    if t.Type()==pcbnew.PCB_TRACE_T: width[t.GetNetname()]=max(width.get(t.GetNetname(),0),M(t.GetWidth()))
nets=set(p.GetNetname() for f in leds for p in f.Pads())
for t in list(b.GetTracks()):
    Ls=[pcbnew.F_Cu,pcbnew.B_Cu] if t.Type()==pcbnew.PCB_VIA_T else [t.GetLayer()]
    bad=False
    for f in leds:
        if (f.GetPosition()-t.GetStart()).EuclideanNorm()>FromMM(60) and (f.GetPosition()-t.GetEnd()).EuclideanNorm()>FromMM(60): continue
        cut=pcbnew.SHAPE_RECT(V(f.GetPosition().x-FromMM(1.8),f.GetPosition().y-FromMM(1.6)),FromMM(3.6),FromMM(3.2))
        for L in Ls:
            sh=t.GetEffectiveShape(L)
            if sh.Collide(cut,FromMM(0.5)): bad=True
            for p in f.Pads():
                if p.IsOnLayer(L) and p.GetNetname()!=t.GetNetname() and p.GetEffectiveShape(L).Collide(sh,FromMM(0.15)): bad=True
        if bad: break
    if bad: nets.add(t.GetNetname()); b.Remove(t)
nets.discard('GND')
order=sorted(nets,key=lambda n:(n in ('/VBUS','+3.3V'), n))
print('nets',len(order), flush=True)
def region(groups, m):
    xs=[]; ys=[]
    for g in groups:
        for it in g:
            bb=it.GetBoundingBox(); xs+=[M(bb.GetLeft()),M(bb.GetRight())]; ys+=[M(bb.GetTop()),M(bb.GetBottom())]
    return (max(14.45,min(xs)-m),max(9.95,min(ys)-m),min(366.35,max(xs)+m),min(238.05,max(ys)+m))
def route(net, terms, w, reg):
    R=Router(b,reg,**RP).build(w=w); add_keepouts(R,b)
    ok,_=R.route_net(net,terms,w=w,via_cost=30,turn_cost=1.0)
    if not ok:
        for t in R.new_items: b.Remove(t)
        return False
    rec=[(t.GetLayer(),t.GetStart(),t.GetEnd(),t.GetWidth()) for t in R.new_items if t.Type()==pcbnew.PCB_TRACE_T]
    n0=len(clusters(b,net))
    out=smooth(R,b,net,R.new_items)
    if len(clusters(b,net))>n0:
        for t in out:
            if t.Type()==pcbnew.PCB_TRACE_T: b.Remove(t)
        for L,s_,e_,wd in rec:
            t=pcbnew.PCB_TRACK(b); t.SetStart(s_); t.SetEnd(e_); t.SetWidth(wd); t.SetLayer(L); t.SetNet(b.FindNet(net)); b.Add(t)
        print('   smoothing reverted', net)
    return True
failed=[]
for net in order:
    cl=sorted(clusters(b,net),key=len,reverse=True)
    if len(cl)<2: continue
    t0=time.time()
    if net!='/VBUS':
        w0=width.get(net,0.2); ok=False
        for w in [w0]+[x for x in (0.4,0.3,0.25) if x<w0-1e-6]:
            if route(net,cl,w,region(cl,15)): ok=True; break
        print(f'{net:35s} cl={len(cl):3d} w={w} ok={ok} {time.time()-t0:.0f}s', flush=True)
        if not ok: failed.append(net)
        continue
    # VBUS: island by island, smallest first, to everything else
    tried=set()
    while True:
        cl=sorted(clusters(b,net),key=len)
        if len(cl)<2: break
        cand=[g for g in cl[:-1] if desc(g) not in tried]
        if not cand: break
        g=cand[0]; reg=region([g],25)
        inside=lambda it:(M(it.GetBoundingBox().GetLeft())>reg[0]+1 and M(it.GetBoundingBox().GetRight())<reg[2]-1 and M(it.GetBoundingBox().GetTop())>reg[1]+1 and M(it.GetBoundingBox().GetBottom())<reg[3]-1)
        others=[it for h in cl if h is not g for it in h if inside(it)]
        ok=False; n0=len(cl)
        if others:
            for w in (0.6,0.5,0.4):
                if route(net,[g,others],w,reg):
                    if len(clusters(b,net))<n0: ok=True; break
                    print('   route did not merge, removing'); 
                    break
        print(f'  VBUS {desc(g)[:45]:45s} w={w} ok={ok}', flush=True)
        if not ok: tried.add(desc(g)); failed.append('VBUS '+desc(g)[:40])
before={n:len(clusters(b,n)) for n in order}
b.Save(out.replace(".kicad_pcb","_pretrim.kicad_pcb"))
print("pretrim multi",[(n,before[n]) for n in order if before[n]>1], flush=True)
print('safe_trim removed',safe_trim(b,nets), flush=True)
after={n:len(clusters(b,n)) for n in order}
print('failed',failed)
print('trim broke',[n for n in order if after[n]>before[n]])
print('remaining multi-cluster',[(n,after[n]) for n in order if after[n]>1])
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
