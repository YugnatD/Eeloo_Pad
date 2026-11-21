"""stitch.py in out : add GND stitching vias to weakly tied GND fill islands (<=1 via/TH tie, or large with <4)."""
import sys, math; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from viaspot import free
b=pcbnew.LoadBoard(sys.argv[1]); out=sys.argv[2]
g=b.FindNet('GND')
def fill():
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    isl=[]; byL={pcbnew.F_Cu:[],pcbnew.B_Cu:[]}
    for z in b.Zones():
        if z.GetNetname()!='GND': continue
        for L in byL:
            if not z.IsOnLayer(L): continue
            ps=z.GetFilledPolysList(L)
            for i in range(ps.OutlineCount()):
                one=pcbnew.SHAPE_POLY_SET(); one.AddOutline(ps.COutline(i))
                for h in range(ps.HoleCount(i)): one.AddHole(ps.CHole(i,h))
                isl.append((L,one,abs(one.Area())/1e12)); byL[L].append(one)
    return isl,byL
added=0
for it in range(3):
    isl,byL=fill()
    ties=[t.GetPosition() for t in b.GetTracks() if t.Type()==pcbnew.PCB_VIA_T and t.GetNetname()=='GND']+[p.GetPosition() for f in b.GetFootprints() for p in f.Pads() if p.GetNetname()=='GND' and p.HasHole()]
    new=0
    for L,one,a in isl:
        if a<20: continue
        n=sum(1 for q in ties if one.Contains(q))
        want = 1 if n==0 else 0
        if n<=1 and a>=20: want=max(1,int(a//300)+1)-n
        elif a>800 and n<4: want=4-n
        if want<=0: continue
        other=byL[pcbnew.B_Cu if L==pcbnew.F_Cu else pcbnew.F_Cu]
        bb=one.BBox(); cands=[]
        step=FromMM(1.0)
        y=bb.GetTop()
        while y<bb.GetBottom():
            x=bb.GetLeft()
            while x<bb.GetRight():
                q=V(int(x),int(y))
                c=pcbnew.SHAPE_CIRCLE(q,FromMM(0.35))
                if one.Contains(q) and not one.Collide(c,0) is False:
                    # via disc fully inside this island and inside an other-layer GND fill
                    inside=all(one.Contains(V(int(q.x+FromMM(0.35)*math.cos(k*math.pi/4)),int(q.y+FromMM(0.35)*math.sin(k*math.pi/4)))) for k in range(8))
                    if inside and any(all(o.Contains(V(int(q.x+FromMM(0.35)*math.cos(k*math.pi/4)),int(q.y+FromMM(0.35)*math.sin(k*math.pi/4)))) for k in range(8)) for o in other):
                        cands.append(q)
                x+=step
            y+=step
        placed=[]
        for q in cands:
            if len(placed)>=want: break
            if any((q-p).EuclideanNorm()<FromMM(8) for p in placed+ties if one.Contains(p) or p in placed): continue
            if not free(b,'GND',M(q.x),M(q.y),r=0.3,clr=0.2): continue
            v=pcbnew.PCB_VIA(b); v.SetPosition(q); v.SetWidth(FromMM(0.6)); v.SetDrill(FromMM(0.3)); v.SetNet(g); b.Add(v)
            placed.append(q); new+=1
        if placed: print('  island %s %.0f mm2 ties=%d +%d'%('F' if L==pcbnew.F_Cu else 'B',a,n,len(placed)))
    added+=new
    if new==0: break
print('added',added)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
