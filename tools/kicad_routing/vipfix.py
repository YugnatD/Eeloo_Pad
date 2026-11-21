"""vipfix.py in out : move vias out of same-net SMD pads (dog-bone). Checks clearance of via, dog-bone and re-pointed tracks."""
import sys, math; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from clusters import clusters
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
CLR=0.2
ol=pcbnew.SHAPE_POLY_SET(); b.GetBoardPolygonOutlines(ol,False)
def edge_ok(pt, r):
    # distance to every outline/hole edge >= 0.5 + r
    c=V(int(pt.x),int(pt.y))
    if not ol.Contains(c): return False
    for i in range(ol.OutlineCount()):
        for h in range(ol.HoleCount(i)):
            if ol.CHole(i,h).PointInside(c): return False
        if ol.COutline(i).SquaredDistance(c,True)**0.5 < FromMM(0.5+r): return False
        for h in range(ol.HoleCount(i)):
            if ol.CHole(i,h).SquaredDistance(c,True)**0.5 < FromMM(0.5+r): return False
    return True
def seg_edge_ok(a, c, w):
    n=max(2,int(M((c-a).EuclideanNorm())/0.1))
    return all(edge_ok(V(int(a.x+(c.x-a.x)*k/n),int(a.y+(c.y-a.y)*k/n)), w/2) for k in range(n+1))
def shape_ok(sh, L, net, ignore):
    for t in b.GetTracks():
        if t.m_Uuid.AsString() in ignore: continue
        if t.IsOnLayer(L) and t.GetNetname()!=net and t.GetEffectiveShape(L).Collide(sh, FromMM(CLR)): return False
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.IsOnLayer(L) and p.GetNetname()!=net and p.GetEffectiveShape(L).Collide(sh, FromMM(CLR)): return False
            if p.HasHole() and p.GetNetname()!=net and p.GetEffectiveHoleShape().Collide(sh, FromMM(0.25)): return False
    for z in list(b.Zones())+[z for f in b.GetFootprints() for z in f.Zones()]:
        if z.GetIsRuleArea() and z.IsOnLayer(L) and (z.GetDoNotAllowTracks() or z.GetDoNotAllowVias()):
            if z.Outline().Collide(sh.Centre() if hasattr(sh,'Centre') else V(0,0), 0): return False
    return True
smd=[(f,p) for f in b.GetFootprints() for p in f.Pads() if p.GetAttribute()==pcbnew.PAD_ATTRIB_SMD and p.GetNetname() not in ('',) and min(M(p.GetSize().x),M(p.GetSize().y))<1.5]
vias=[t for t in b.GetTracks() if t.Type()==pcbnew.PCB_VIA_T]
todo=[]
for v in vias:
    for f,p in smd:
        if v.GetNetname()==p.GetNetname() and p.HitTest(v.GetPosition()): todo.append((v,f,p)); break
print('vias in SMD pads',len(todo))
moved=0; fail=[]
for v,f,p in todo:
    net=v.GetNetname(); vp=v.GetPosition(); pc=p.GetPosition()
    padL=pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu; othL=pcbnew.B_Cu if padL==pcbnew.F_Cu else pcbnew.F_Cu
    # tracks attached to the via
    att=[t for t in b.GetTracks() if t.Type()==pcbnew.PCB_TRACE_T and t.GetNetname()==net and ((t.GetStart()-vp).EuclideanNorm()<FromMM(0.31) or (t.GetEnd()-vp).EuclideanNorm()<FromMM(0.31))]
    oth=[t for t in att if t.GetLayer()==othL]
    if net=='GND' and not oth:
        # stitching via inside a GND pad: just move it out, no tracks needed
        pass
    padpoly=pcbnew.SHAPE_POLY_SET(); p.TransformShapeToPolygon(padpoly,padL,0,FromMM(0.005),pcbnew.ERROR_OUTSIDE)
    best=None
    ign={v.m_Uuid.AsString()}|{t.m_Uuid.AsString() for t in att}
    for dist in [0.65+0.05*k for k in range(0,24)]:
        for a in range(0,360,15):
            # direction measured from pad centre; start point = pad edge along that direction
            dx,dy=math.cos(math.radians(a)),math.sin(math.radians(a))
            q=V(int(pc.x+FromMM(dx*(M(p.GetSize().x)/2+dist) if abs(dx)>1e-9 else 0)), int(pc.y+FromMM(dy*(M(p.GetSize().y)/2+dist) if abs(dy)>1e-9 else 0)))
            # via ring >= 0.2 from the pad copper
            c=pcbnew.SHAPE_CIRCLE(q,FromMM(0.3))
            if padpoly.Collide(c, FromMM(0.2)): continue
            if any(pp.GetAttribute()==pcbnew.PAD_ATTRIB_SMD and pp.GetNetname()==net and pp.GetEffectiveShape(pcbnew.F_Cu if pp.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu).Collide(c,FromMM(0.2)) for ff in b.GetFootprints() for pp in ff.Pads()): continue
            if not edge_ok(q,0.3): continue
            if not (shape_ok(c,pcbnew.F_Cu,net,ign) and shape_ok(c,pcbnew.B_Cu,net,ign)): continue
            if any(t.Type()==pcbnew.PCB_VIA_T and t.m_Uuid.AsString()!=v.m_Uuid.AsString() and (t.GetPosition()-q).EuclideanNorm()<FromMM(0.85) for t in b.GetTracks()): continue
            dog=pcbnew.SHAPE_SEGMENT(vp,q,FromMM(0.3))
            if not shape_ok(dog,padL,net,ign) or not seg_edge_ok(vp,q,0.3): continue
            ok=True
            for t in oth:
                o=t.GetEnd() if (t.GetStart()-vp).EuclideanNorm()<(t.GetEnd()-vp).EuclideanNorm() else t.GetStart()
                if not shape_ok(pcbnew.SHAPE_SEGMENT(o,q,t.GetWidth()),othL,net,ign) or not seg_edge_ok(o,q,M(t.GetWidth())): ok=False; break
            if not ok: continue
            best=q; break
        if best is not None: break
    if best is None: fail.append((f.GetReference(),p.GetNumber(),net,round(M(vp.x),2),round(M(vp.y),2))); continue
    n0=len(clusters(b,net)) if net!='GND' else 0
    saved=[]
    for t in oth:
        if (t.GetStart()-vp).EuclideanNorm()<(t.GetEnd()-vp).EuclideanNorm(): saved.append((t,'s',t.GetStart())); t.SetStart(best)
        else: saved.append((t,'e',t.GetEnd())); t.SetEnd(best)
    w=max([M(t.GetWidth()) for t in att]+[0.3]) if att else 0.3
    w=min(w, M(min(p.GetSize().x,p.GetSize().y)))
    d=pcbnew.PCB_TRACK(b); d.SetStart(vp); d.SetEnd(best); d.SetWidth(FromMM(max(0.25,w))); d.SetLayer(padL); d.SetNet(v.GetNet()); b.Add(d)
    v.SetPosition(best)
    if net!='GND' and len(clusters(b,net))>n0:
        v.SetPosition(vp); b.Remove(d)
        for t,k,o0 in saved: (t.SetStart if k=='s' else t.SetEnd)(o0)
        fail.append((f.GetReference(),p.GetNumber(),net,'connectivity')); continue
    moved+=1
print('moved',moved,'failed',len(fail))
for x in fail: print('  FAIL',x)
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
