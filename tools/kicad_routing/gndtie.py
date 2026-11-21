"""gndtie.py in out REF.pad ... : tie an isolated GND pad to the other layer's GND fill with a short track + via."""
import sys, math; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]; specs=sys.argv[3:]
b=pcbnew.LoadBoard(src); pcbnew.ZONE_FILLER(b).Fill(b.Zones())
exec(open('vipfix.py').read().split("smd=[")[0].split("b=pcbnew.LoadBoard(src)")[1])   # edge_ok, seg_edge_ok, shape_ok
fills={L:[] for L in (pcbnew.F_Cu,pcbnew.B_Cu)}
for z in b.Zones():
    if z.GetNetname()=='GND':
        for L in fills:
            if z.IsOnLayer(L):
                ps=z.GetFilledPolysList(L)
                for i in range(ps.OutlineCount()):
                    one=pcbnew.SHAPE_POLY_SET(); one.AddOutline(ps.COutline(i))
                    for h in range(ps.HoleCount(i)): one.AddHole(ps.CHole(i,h))
                    if abs(one.Area())/1e12>200: fills[L].append(one)
g=b.FindNet('GND')
for sp in specs:
    r,n=sp.split('.'); f=b.FindFootprintByReference(r); p=[p for p in f.Pads() if p.GetNumber()==n][0]
    padL=pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu; othL=pcbnew.B_Cu if padL==pcbnew.F_Cu else pcbnew.F_Cu
    pc=p.GetPosition(); best=None
    padpoly=pcbnew.SHAPE_POLY_SET(); p.TransformShapeToPolygon(padpoly,padL,0,FromMM(0.005),pcbnew.ERROR_OUTSIDE)
    for dist in [0.7+0.1*k for k in range(25)]:
        for a in range(0,360,10):
            q=V(int(pc.x+FromMM(dist*math.cos(math.radians(a)))),int(pc.y+FromMM(dist*math.sin(math.radians(a)))))
            c=pcbnew.SHAPE_CIRCLE(q,FromMM(0.3))
            if padpoly.Collide(c,FromMM(0.2)): continue
            if not edge_ok(q,0.3): continue
            if not (shape_ok(c,pcbnew.F_Cu,'GND',set()) and shape_ok(c,pcbnew.B_Cu,'GND',set())): continue
            if any(t.Type()==pcbnew.PCB_VIA_T and (t.GetPosition()-q).EuclideanNorm()<FromMM(0.85) for t in b.GetTracks()): continue
            ring=[V(int(q.x+FromMM(0.35)*math.cos(k*math.pi/4)),int(q.y+FromMM(0.35)*math.sin(k*math.pi/4))) for k in range(8)]
            if not any(all(ps.Contains(pt) for pt in ring) for ps in fills[othL]): continue
            if not shape_ok(pcbnew.SHAPE_SEGMENT(pc,q,FromMM(0.3)),padL,'GND',set()) or not seg_edge_ok(pc,q,0.3): continue
            best=q; break
        if best is not None: break
    if best is None: print(sp,'no spot'); continue
    t=pcbnew.PCB_TRACK(b); t.SetStart(pc); t.SetEnd(best); t.SetWidth(FromMM(0.3)); t.SetLayer(padL); t.SetNet(g); b.Add(t)
    v=pcbnew.PCB_VIA(b); v.SetPosition(best); v.SetWidth(FromMM(0.6)); v.SetDrill(FromMM(0.3)); v.SetNet(g); b.Add(v)
    print(sp,'tied at',round(M(best.x),2),round(M(best.y),2))
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
