import pcbnew, sys
sys.argv=[sys.argv[0]]+sys.argv[1:]
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
# U11 bottom border duplicates the U10 module drawing
u=b.FindFootprintByReference('U11') if False else None
if u is None: pass
else:
  pass
u=b.FindFootprintByReference('U11')
for g in list(u.GraphicalItems()):
    if g.GetClass()=='PCB_SHAPE' and g.GetLayer()==pcbnew.F_SilkS and g.GetShape()==pcbnew.SHAPE_T_SEGMENT:
        ys=(M(g.GetStart().y),M(g.GetEnd().y))
        if min(ys)>208.9: u.Remove(g)
exec(open('silk_common.py').read())
for ref in sys.argv[3:]:
    f=b.FindFootprintByReference(ref); r=f.Reference()
    r.SetTextSize(V(FromMM(0.8),FromMM(0.8))); r.SetTextThickness(FromMM(0.12))
    ok=place(f,allow_center=True)
    print(ref,'placed' if ok else 'NO SPOT')
b.Save(out)
