"""Restore given nets in `cur` from `base` geometry (tracks + vias), then refill zones."""
import pcbnew, sys
cur,base,out=sys.argv[1:4]; nets=set(sys.argv[4:])
b=pcbnew.LoadBoard(cur); o=pcbnew.LoadBoard(base)
for t in list(b.GetTracks()):
    if t.GetNetname() in nets: b.Remove(t)
n=0
for t in o.GetTracks():
    if t.GetNetname() not in nets: continue
    net=b.FindNet(t.GetNetname())
    if t.Type()==pcbnew.PCB_VIA_T:
        v=pcbnew.PCB_VIA(b); v.SetPosition(t.GetPosition()); v.SetWidth(t.GetWidth(pcbnew.F_Cu)); v.SetDrill(t.GetDrillValue()); v.SetNet(net); b.Add(v)
    else:
        c=pcbnew.PCB_TRACK(b); c.SetStart(t.GetStart()); c.SetEnd(t.GetEnd()); c.SetWidth(t.GetWidth()); c.SetLayer(t.GetLayer()); c.SetNet(net); b.Add(c)
    n+=1
print('restored items',n,'for',len(nets),'nets')
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
