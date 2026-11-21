import pcbnew, sys, time
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from router import Router
from clusters import clusters
from trim import trim
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]
VC=float(sys.argv[3]) if len(sys.argv)>3 else 150
DP=float(sys.argv[4]) if len(sys.argv)>4 else 1.15
LMAX=float(sys.argv[5]) if len(sys.argv)>5 else 1.4
b=pcbnew.LoadBoard(src)
F={f.GetReference():f for f in b.GetFootprints()}
nets=set()
for ic in ('IC2','IC3'):
    for p in F[ic].Pads():
        n=int(p.GetNumber())
        if n in (12,22,23,24): continue
        nets.add(p.GetNetname())
nets.discard('GND'); nets.discard('+3.3V')
anchor_pads=[p for r in ('IC2','IC3','J13') for p in F[r].Pads()]
def on_anchor(pt,layer):
    return any(p.IsOnLayer(layer) and p.HitTest(pt) for p in anchor_pads)
allpads={}
for f in b.GetFootprints():
    for p in f.Pads(): allpads.setdefault(p.GetNetname(),[]).append(p)
def on_pad(net,pt,layer):
    return any(p.IsOnLayer(layer) and p.HitTest(pt) for p in allpads.get(net,[]))
def protected(net):
    its=[t for t in b.GetTracks() if t.GetNetname()==net]
    prot=set(); ends=[]
    for t in its:
        if t.Type()!=pcbnew.PCB_TRACE_T: continue
        s,e,L=t.GetStart(),t.GetEnd(),t.GetLayer()
        a1,a2=on_anchor(s,L),on_anchor(e,L)
        if a1 or a2 or (on_pad(net,s,L) and on_pad(net,e,L)):
            prot.add(t.m_Uuid.AsString()); ends.append(e if a1 else s)
        # INT escapes around J13
        if 'INT' in net and 304<M(s.x)<312.5 and 99.5<M(s.y)<103: prot.add(t.m_Uuid.AsString()); ends+= [s,e]
    for t in its:
        if t.Type()==pcbnew.PCB_VIA_T and any((t.GetPosition()-q).EuclideanNorm()<FromMM(0.05) for q in ends): prot.add(t.m_Uuid.AsString())
    return its,prot
def geom(t):
    if t.Type()==pcbnew.PCB_VIA_T: return ('v',t.GetPosition(),t.GetWidth(pcbnew.F_Cu),t.GetDrillValue())
    return ('t',t.GetLayer(),t.GetStart(),t.GetEnd(),t.GetWidth())
def make(g,net):
    if g[0]=='v':
        v=pcbnew.PCB_VIA(b); v.SetPosition(g[1]); v.SetWidth(g[2]); v.SetDrill(g[3]); v.SetNet(b.FindNet(net)); b.Add(v); return v
    t=pcbnew.PCB_TRACK(b); t.SetLayer(g[1]); t.SetStart(g[2]); t.SetEnd(g[3]); t.SetWidth(g[4]); t.SetNet(b.FindNet(net)); b.Add(t); return t
def stats(items):
    v=sum(1 for t in items if t.Type()==pcbnew.PCB_VIA_T); l=sum(M(t.GetLength()) for t in items if t.Type()==pcbnew.PCB_TRACE_T); return v,l
t0=time.time()
R=Router(b,(14.45,9.95,366.35,238.05)).build(w=0.2)
R.dirpref={'F':(1.0,DP,1.0+(DP-1)/2),'B':(DP,1.0,1.0+(DP-1)/2)} if DP>1 else None
print('router built',round(time.time()-t0),'s',flush=True)
def total_vias(): return sum(1 for t in b.GetTracks() if t.Type()==pcbnew.PCB_VIA_T)
print('vias before',total_vias())
changed=set()
for pas in (1,2,3):
    order=[]
    for n in nets:
        its,prot=protected(n)
        free_=[t for t in its if t.m_Uuid.AsString() not in prot]
        order.append((stats(free_)[0],n))
    order.sort(reverse=True)
    acc=0
    for ov,n in order:
        its,prot=protected(n)
        old=[t for t in its if t.m_Uuid.AsString() not in prot]
        if not old: continue
        o_v,o_l=stats(old); saved=[geom(t) for t in old]
        for t in old: R.remove_owner(t.m_Uuid.AsString()); b.Remove(t)
        cl=clusters(b,n)
        ok,_=R.route_net(n,cl,w=0.2,via_cost=VC,turn_cost=2.0,max_exp=8_000_000) if len(cl)>1 else (True,None)
        new=list(R.new_items) if len(cl)>1 else []
        n_v,n_l=stats(new)
        good= ok and ((n_v<o_v and n_l<=o_l*LMAX+3) or (n_v==o_v and n_l<o_l*0.93))
        if good:
            acc+=1; changed.add(n); print('  %-40s vias %d->%d  len %.0f->%.0f  ACCEPT'%(n.split('/')[-1],o_v,n_v,o_l,n_l),flush=True)
        else:
            for t in new: R.remove_owner(t.m_Uuid.AsString()); b.Remove(t)
            for g in saved: R.add_item(make(g,n),n)
            print('  %-40s vias %d->%s  keep'%(n.split('/')[-1],o_v,n_v if ok else 'FAIL'),flush=True)
    print('pass',pas,'accepted',acc,'vias now',total_vias(),round(time.time()-t0),'s',flush=True)
print('trim',trim(b,nets))
open(out+'.changed','w').write('\n'.join(sorted(changed)))
print('vias after',total_vias())
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
