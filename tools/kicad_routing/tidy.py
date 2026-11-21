"""tidy.py in out : remove exact duplicate segments, split segments at T-junctions (geometry unchanged), enlarge small silk texts."""
import sys; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from collections import defaultdict
b=pcbnew.LoadBoard(sys.argv[1])
tr=[t for t in b.GetTracks() if t.Type()==pcbnew.PCB_TRACE_T]
# 1) duplicates
seen={}; dup=0
for t in tr:
    k=(t.GetNetname(),t.GetLayer())+tuple(sorted([(t.GetStart().x,t.GetStart().y),(t.GetEnd().x,t.GetEnd().y)]))
    if k in seen:
        o=seen[k]
        keep,drop=(o,t) if o.GetWidth()>=t.GetWidth() else (t,o)
        b.Remove(drop); seen[k]=keep; dup+=1
    else: seen[k]=t
print('duplicates removed',dup)
# 2) T-junction split
tr=[t for t in b.GetTracks() if t.Type()==pcbnew.PCB_TRACE_T]
pts=defaultdict(list)
for t in tr:
    for p in (t.GetStart(),t.GetEnd()): pts[(t.GetNetname(),t.GetLayer())].append((p.x,p.y))
for t in b.GetTracks():
    if t.Type()==pcbnew.PCB_VIA_T:
        for L in (pcbnew.F_Cu,pcbnew.B_Cu): pts[(t.GetNetname(),L)].append((t.GetPosition().x,t.GetPosition().y))
split=0
tol=FromMM(0.002)
work=list(tr)
while work:
    t=work.pop()
    a,c=t.GetStart(),t.GetEnd(); L=t.GetLayer(); n=t.GetNetname()
    if n in ('',): continue
    ax,ay,cx,cy=a.x,a.y,c.x,c.y; ln2=(cx-ax)**2+(cy-ay)**2
    if ln2==0: continue
    for (px,py) in pts[(n,L)]:
        if (px,py)==(ax,ay) or (px,py)==(cx,cy): continue
        if not (min(ax,cx)-tol<=px<=max(ax,cx)+tol and min(ay,cy)-tol<=py<=max(ay,cy)+tol): continue
        cross=abs((cx-ax)*(py-ay)-(cy-ay)*(px-ax))/ln2**0.5
        if cross>tol: continue
        u=((px-ax)*(cx-ax)+(py-ay)*(cy-ay))/ln2
        if u<=1e-6 or u>=1-1e-6: continue
        q=V(int(px),int(py))
        t2=pcbnew.PCB_TRACK(b); t2.SetStart(q); t2.SetEnd(c); t2.SetWidth(t.GetWidth()); t2.SetLayer(L); t2.SetNet(t.GetNet()); b.Add(t2)
        t.SetEnd(q); split+=1; work.append(t); work.append(t2); break
print('T splits',split)
# 3) silk text >= 1.0 mm
small=0
refs={f'LED{i}' for i in range(1,26)}|{'R121','Q2','R12','F1','D12'}
for f in b.GetFootprints():
    items=[]
    if f.GetReference() in refs: items.append(f.Reference())
    if f.GetReference()=='U10':
        items+=[g for g in f.GraphicalItems() if isinstance(g,pcbnew.PCB_TEXT) and g.GetText() in ('3V3','5V','D-','D+','1 GND')]
    for tx in items:
        if tx.GetLayer() in (pcbnew.F_SilkS,pcbnew.B_SilkS) and tx.GetTextHeight()<FromMM(0.99):
            tx.SetTextSize(V(FromMM(1.0),FromMM(1.0))); tx.SetTextThickness(FromMM(0.15)); small+=1
print('silk texts enlarged',small)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(sys.argv[2])
