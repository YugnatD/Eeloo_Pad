import pcbnew, sys, time
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from router import Router
from clusters import clusters, desc
from trim import trim
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
REG=(14.45,9.95,366.35,238.05)
POWER=['Net-(F2-Pad1)','Net-(D11-A)','+12V','/BUCK_SW','/V5_DC','/VBUS']
SIG=['/BUCK_BOOT','/BUCK_VCC','/BUCK_FB','/DC_SENSE','/VBUS_USB','/HUB_RESET','/HUB_V33']
RS={}
def router(w):
    if w not in RS:
        RS[w]=Router(b,REG).build(w=w)
    return RS[w]
def sync(new):
    for R in RS.values():
        for t in new: R.add_item(t,t.GetNetname())
fails=[]
for nets,widths in ((POWER,(0.8,0.6,0.4)),(SIG,(0.3,0.2))):
    for n in nets:
        cl=sorted(clusters(b,n),key=len,reverse=True)
        if len(cl)<2: print(n,'connected'); continue
        ok=False
        for w in widths:
            R=router(w); before={t.m_Uuid.AsString() for t in b.GetTracks()}
            ok,_=R.route_net(n,cl,w=w,via_cost=30,turn_cost=1.0)
            new=[t for t in b.GetTracks() if t.m_Uuid.AsString() not in before]
            if ok:
                for R2 in RS.values():
                    if R2 is not R:
                        for t in new: R2.add_item(t,n)
                print('%-16s OK w=%.1f clusters=%d'%(n,w,len(cl)),flush=True); break
            for t in new: R.remove_owner(t.m_Uuid.AsString()); b.Remove(t)
            cl=sorted(clusters(b,n),key=len,reverse=True)
        if not ok: print('%-16s FAILED'%n,[desc(g) for g in cl]); fails.append(n)
print('trim',trim(b,set(POWER+SIG)))
print('FAILS',fails)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
