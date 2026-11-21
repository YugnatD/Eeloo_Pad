import pcbnew, sys
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
from viaspot import free
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
for spec in (sys.argv[3].split(',') if len(sys.argv)>3 and sys.argv[3] else []):
    r,n=spec.split('.')
    for p in b.FindFootprintByReference(r).Pads():
        if p.GetNumber()==n: p.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
def analyse():
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    isl=[]
    for z in b.Zones():
        if z.GetNetname()!='GND': continue
        for L in (pcbnew.F_Cu,pcbnew.B_Cu):
            if not z.IsOnLayer(L): continue
            ps=z.GetFilledPolysList(L)
            for i in range(ps.OutlineCount()):
                one=pcbnew.SHAPE_POLY_SET(); one.AddOutline(ps.COutline(i))
                for h in range(ps.HoleCount(i)): one.AddHole(ps.CHole(i,h))
                isl.append((L,one,abs(one.Area())))
    items=[t for t in b.GetTracks() if t.GetNetname()=='GND']+[p for f in b.GetFootprints() for p in f.Pads() if p.GetNetname()=='GND']
    n=len(isl); par=list(range(n+len(items)))
    def f(a):
        while par[a]!=a: par[a]=par[par[a]]; a=par[a]
        return a
    def u(a,c): par[f(a)]=f(c)
    for j,it in enumerate(items):
        for k,(L,one,a) in enumerate(isl):
            if not it.IsOnLayer(L): continue
            ip=pcbnew.SHAPE_POLY_SET(); it.TransformShapeToPolygon(ip,L,FromMM(0.03),FromMM(0.01),pcbnew.ERROR_OUTSIDE)
            if not one.BBox().Intersects(ip.BBox()): continue
            x=pcbnew.SHAPE_POLY_SET(one); x.BooleanIntersection(ip)
            if x.OutlineCount(): u(n+j,k)
    b.BuildConnectivity(); cn=b.GetConnectivity()
    idx={it.m_Uuid.AsString():j for j,it in enumerate(items)}
    for j,it in enumerate(items):
        for o in cn.GetConnectedItems(it):
            k=idx.get(o.m_Uuid.AsString())
            if k is not None: u(n+j,n+k)
    main=f(max(range(n),key=lambda k:isl[k][2]))
    return isl,f,main,n,items
for it_ in range(30):
    isl,f,main,n,items=analyse()
    lost=[k for k in range(n) if f(k)!=main]
    # only islands holding at least one GND item matter (others are removed by the filler)
    anchored={f(n+j) for j in range(len(items))}
    lost=[k for k in lost if f(k) in anchored]
    print('pass',it_,'lost anchored islands',len(lost))
    if not lost: break
    fixed=0
    for k in sorted(lost,key=lambda k:-isl[k][2]):
        if f(k)==main: continue
        L,one,a=isl[k]
        er=pcbnew.SHAPE_POLY_SET(one); er.Inflate(-FromMM(0.35),pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS,FromMM(0.01))
        others=[q for q in range(n) if isl[q][0]!=L and f(q)==main]
        bb=one.BBox(); done=False; y=bb.GetTop()
        while y<=bb.GetBottom() and not done:
            x=bb.GetLeft()
            while x<=bb.GetRight():
                p=V(int(x),int(y))
                if er.Contains(p):
                    ok=False
                    for q in others:
                        e2=pcbnew.SHAPE_POLY_SET(isl[q][1]); 
                        if e2.Contains(p):
                            e2.Inflate(-FromMM(0.35),pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS,FromMM(0.01))
                            ok=e2.Contains(p); break
                    if ok and free(b,'GND',M(p.x),M(p.y),clr=0.22):
                        v=pcbnew.PCB_VIA(b); v.SetPosition(p); v.SetWidth(FromMM(0.6)); v.SetDrill(FromMM(0.3)); v.SetNet(b.FindNet('GND')); b.Add(v)
                        print('  via at',round(M(p.x),2),round(M(p.y),2)); fixed+=1; done=True; break
                x+=FromMM(0.4)
            y+=FromMM(0.4)
        if done: break   # re-analyse after each via
        else: print('  no spot island area %.1f at (%.1f,%.1f) %s'%(a/1e12,M(bb.Centre().x),M(bb.Centre().y),'F' if L==pcbnew.F_Cu else 'B'))
    if not fixed: break
pcbnew.ZONE_FILLER(b).Fill(b.Zones()); b.Save(out)
