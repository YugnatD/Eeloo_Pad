import sys; sys.path.insert(0,'.')
import pcbnew
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out=sys.argv[1],sys.argv[2]
b=pcbnew.LoadBoard(src)
sws=[f for f in b.GetFootprints() if f.GetFPIDAsString().endswith('SW_MX_Transparent_1.00u_PCB')]
def swcenter(f): return [p for p in f.Pads() if p.GetAttribute()==pcbnew.PAD_ATTRIB_NPTH and M(p.GetDrillSize().x)>3.5][0].GetPosition()
centers=[(f.GetReference(),swcenter(f)) for f in sws]
log=[]
for i in range(1,26):
    old=b.FindFootprintByReference(f'LED{i}'); op=old.GetPosition()
    ref,c=min(centers,key=lambda rc:(rc[1]-op).EuclideanNorm())
    d=op-c
    if abs(M(d.x))<0.5 and 3.5<M(d.y)<6: tgt=V(c.x,c.y+FromMM(5.08)); under=ref
    else: tgt=op; under=None
    nets={p.GetNumber():p.GetNet() for p in old.Pads()}
    # remove tracks/vias touching old pads
    for t in list(b.GetTracks()):
        for p in old.Pads():
            L=t.GetLayer() if t.Type()!=pcbnew.PCB_VIA_T else pcbnew.F_Cu
            if p.IsOnLayer(L) and t.GetNetname()==p.GetNetname() and p.GetEffectiveShape(L).Collide(t.GetEffectiveShape(L),0):
                b.Remove(t); break
    new=pcbnew.FootprintLoad('lib','LED_SK6812MINI-E_3.2x2.8mm')
    new.SetFPID(pcbnew.LIB_ID('Eeloo_Pad','LED_SK6812MINI-E_3.2x2.8mm'))
    new.SetReference(old.GetReference()); new.SetValue(old.GetValue()); new.SetPath(old.GetPath())
    new.SetSheetname(old.GetSheetname()); new.SetSheetfile(old.GetSheetfile())
    for fld in old.GetFields():
        n=fld.GetName()
        if n in ('Reference','Value','Footprint'): continue
        new.SetField(n, fld.GetText())
        nf=new.GetField(n)
        if nf is not None: nf.SetVisible(False); nf.SetLayer(pcbnew.F_Fab)
    new.SetAttributes(old.GetAttributes())
    b.Remove(old); b.Add(new)
    new.SetPosition(tgt); new.SetOrientationDegrees(0)
    new.Flip(tgt, pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    for p in new.Pads(): p.SetNet(nets[p.GetNumber()])
    rel=sorted((p.GetNumber(),round(M(p.GetPosition().x-tgt.x),3),round(M(p.GetPosition().y-tgt.y),3),new.GetLayerName()) for p in new.Pads())
    log.append((f'LED{i}',under,round(M(tgt.x),3),round(M(tgt.y),3),rel[0]))
for l in log: print(l)
b.Save(out)
