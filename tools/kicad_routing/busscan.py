import sys, pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
b=pcbnew.LoadBoard(sys.argv[1]); X0,X1=58.0,312.0; W=0.6
def obstacles(L):
    o=[]
    for t in b.GetTracks():
        if t.GetNetname()=='/VBUS' or not t.IsOnLayer(L): continue
        bb=t.GetBoundingBox()
        if M(bb.GetBottom())>30 and M(bb.GetTop())<46: o.append(t.GetEffectiveShape(L))
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetname()=='/VBUS': continue
            bb=p.GetBoundingBox()
            if M(bb.GetBottom())>30 and M(bb.GetTop())<46:
                if p.IsOnLayer(L): o.append(p.GetEffectiveShape(L))
                elif p.HasHole(): o.append(p.GetEffectiveHoleShape())
    return o
OB={L:obstacles(L) for L in (pcbnew.F_Cu,pcbnew.B_Cu)}
def free(L,x,y,w=W,clr=0.2):
    s=pcbnew.SHAPE_SEGMENT(V(FromMM(x),FromMM(y)),V(FromMM(x+0.1),FromMM(y)),FromMM(w))
    return not any(o.Collide(s,FromMM(clr)) for o in OB[L])
def via_ok(x,y):
    c=pcbnew.SHAPE_CIRCLE(V(FromMM(x),FromMM(y)),FromMM(0.3))
    return not any(o.Collide(c,FromMM(0.2)) for L in OB for o in OB[L])
def plan(y):
    xs=[round(X0+i*0.1,2) for i in range(int((X1-X0)/0.1)+1)]
    fb=[free(pcbnew.B_Cu,x,y) for x in xs]; ff=[free(pcbnew.F_Cu,x,y) for x in xs]
    # blocked B intervals
    iv=[]; i=0
    while i<len(xs):
        if not fb[i]:
            j=i
            while j<len(xs) and not fb[j]: j+=1
            iv.append((xs[i],xs[j-1])); i=j
        else: i+=1
    res=[]
    for a,c in iv:
        # need F free from a-0.8 to c+0.8 and vias at a-0.8, c+0.8 (B free there)
        va,vc=round(a-0.9,2),round(c+0.9,2)
        fok=all(free(pcbnew.F_Cu,x,y) for x in [round(va+k*0.1,2) for k in range(int((vc-va)/0.1)+1)])
        res.append((a,c,fok,via_ok(va,y),via_ok(vc,y)))
    return res
if __name__=='__main__':
    for y in [float(v) for v in sys.argv[2].split(',')]:
        r=plan(y); bad=[x for x in r if not (x[2] and x[3] and x[4])]
        print(y,'B gaps',len(r),'unsolvable',len(bad),bad[:6])
