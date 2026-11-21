import pcbnew, sys, math
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]; refs=sys.argv[3:]
b=pcbnew.LoadBoard(src)
exec(open('silk_common.py').read())
J1=b.FindFootprintByReference('J1').GetCourtyard(pcbnew.F_CrtYd).BBox()
for ref in refs:
    f=b.FindFootprintByReference(ref); r=f.Reference()
    p0,a0,s0,t0=r.GetPosition(),r.GetTextAngleDegrees(),r.GetTextSize(),r.GetTextThickness()
    front=r.GetLayer()==pcbnew.F_SilkS
    obs=obstacles(front,ref)
    if ref!='J1': obs.append(J1)
    cb=f.GetCourtyard(pcbnew.F_CrtYd if not f.IsFlipped() else pcbnew.B_CrtYd).BBox(); cx,cy=cb.Centre().x,cb.Centre().y
    best=None
    for size in (1.0,0.8):
        r.SetTextSize(V(FromMM(size),FromMM(size))); r.SetTextThickness(FromMM(0.15 if size==1.0 else 0.12))
        for ang in (0,90):
            r.SetTextAngleDegrees(ang); r.SetPosition(V(cx,cy)); tb=r.GetBoundingBox(); hw,hh=tb.GetWidth()//2,tb.GetHeight()//2
            for d in [FromMM(x) for x in (0.1,0.4,0.8,1.2,1.8,2.5,3.5)]:
                for x,y in ((cx,cb.GetTop()-hh-d),(cx,cb.GetBottom()+hh+d),(cb.GetLeft()-hw-d,cy),(cb.GetRight()+hw+d,cy),(cb.GetLeft()-hw-d,cb.GetTop()-hh-d),(cb.GetRight()+hw+d,cb.GetTop()-hh-d),(cb.GetLeft()-hw-d,cb.GetBottom()+hh+d),(cb.GetRight()+hw+d,cb.GetBottom()+hh+d)):
                    r.SetPosition(V(int(x),int(y))); tb=r.GetBoundingBox()
                    if tb.GetLeft()<ib[0] or tb.GetTop()<ib[1] or tb.GetRight()>ib[2] or tb.GetBottom()>ib[3]: continue
                    if any(tb.Intersects(o) for o in obs): continue
                    sc=(x-cx)**2+(y-cy)**2+(FromMM(1.5)**2 if size<1 else 0)
                    if best is None or sc<best[0]: best=(sc,int(x),int(y),ang,size)
            if best and best[4]==size: pass
        if best: break
    if best:
        r.SetTextSize(V(FromMM(best[4]),FromMM(best[4]))); r.SetTextThickness(FromMM(0.15 if best[4]==1.0 else 0.12))
        r.SetTextAngleDegrees(best[3]); r.SetPosition(V(best[1],best[2]))
        print('%-5s -> (%.2f,%.2f) ang %d size %.1f  (%.1f mm from centre)'%(ref,M(best[1]),M(best[2]),best[3],best[4],M(int(math.sqrt(best[0])))))
    else:
        r.SetPosition(p0); r.SetTextAngleDegrees(a0); r.SetTextSize(s0); r.SetTextThickness(t0); print(ref,'unchanged (no spot)')
b.Save(out)
