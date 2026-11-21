import pcbnew, sys
from pcbnew import FromMM, ToMM as M, VECTOR2I as V
src,out,lib=sys.argv[1:4]
b=pcbnew.LoadBoard(src)
CX,CY=270.5,207.0
def mm(x,y): return V(FromMM(x),FromMM(y))
# ---- footprint (local coordinates, origin = hole centre)
fp=pcbnew.FOOTPRINT(b)
fp.SetFPID(pcbnew.LIB_ID('Eeloo_Pad','Schneider_XB2_22.5mm_PanelMount'))
fp.SetLibDescription('Panel mounting for Schneider Harmony XB2 22mm pushbutton (e.g. XB2-BS542): 22.5mm hole, head 40mm, rear body ~30x40mm, PCB used as panel (1-6mm)')
fp.SetKeywords('Schneider XB2 22mm pushbutton emergency stop panel hole')
pad=pcbnew.PAD(fp); pad.SetAttribute(pcbnew.PAD_ATTRIB_NPTH); pad.SetShape(pcbnew.PAD_SHAPE_CIRCLE)
pad.SetSize(mm(22.5,22.5)); pad.SetDrillSize(mm(22.5,22.5)); pad.SetLayerSet(pad.UnplatedHoleMask()); pad.SetNumber(''); fp.Add(pad)
def circle(layer,r,w):
    s=pcbnew.PCB_SHAPE(fp,pcbnew.SHAPE_T_CIRCLE); s.SetCenter(mm(0,0)); s.SetEnd(mm(r,0)); s.SetLayer(layer); s.SetWidth(FromMM(w)); fp.Add(s)
def rect(layer,w,h,lw):
    s=pcbnew.PCB_SHAPE(fp,pcbnew.SHAPE_T_RECTANGLE); s.SetStart(mm(-w/2,-h/2)); s.SetEnd(mm(w/2,h/2)); s.SetLayer(layer); s.SetWidth(FromMM(lw)); fp.Add(s)
circle(pcbnew.F_CrtYd,20.25,0.05)        # mushroom head 40mm
circle(pcbnew.F_SilkS,15.5,0.15)         # bezel / clamping collar outline
circle(pcbnew.F_Fab,20.0,0.1)
circle(pcbnew.Dwgs_User,11.25,0.1)
rect(pcbnew.B_CrtYd,32,42,0.05)          # rear body + contact block keep-out
rect(pcbnew.B_Fab,30,40,0.1)
fp.SetReference('H13'); fp.SetValue('XB2-BS542 22.5mm')
fp.Reference().SetPosition(mm(0,-17.2)); fp.Reference().SetLayer(pcbnew.F_Fab)
fp.Value().SetPosition(mm(0,21.5)); fp.Value().SetLayer(pcbnew.F_Fab)
t=pcbnew.PCB_TEXT(fp); t.SetText('STAGE'); t.SetLayer(pcbnew.F_SilkS); t.SetPosition(mm(0,-17.3)); t.SetTextSize(mm(1.5,1.5)); t.SetTextThickness(FromMM(0.25)); fp.Add(t)
# keep tracks/vias out from under the clamping collar (both sides)
z=pcbnew.ZONE(fp); z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
z.SetDoNotAllowPads(False); z.SetDoNotAllowZoneFills(False); z.SetDoNotAllowFootprints(False)
ls=pcbnew.LSET(); ls.AddLayer(pcbnew.F_Cu); ls.AddLayer(pcbnew.B_Cu); z.SetLayerSet(ls)
import math
pts=[mm(16.0*math.cos(2*math.pi*k/48),16.0*math.sin(2*math.pi*k/48)) for k in range(48)]
z.AppendCorner(pts[0],-1)
for p_ in pts[1:]: z.AppendCorner(p_,0)
z.SetZoneName('XB2 collar keepout'); fp.Add(z)
fp.SetAttributes(pcbnew.FP_BOARD_ONLY | pcbnew.FP_EXCLUDE_FROM_BOM | pcbnew.FP_EXCLUDE_FROM_POS_FILES)
pcbnew.FootprintSave(lib, fp)
b.Add(fp); fp.SetPosition(mm(CX,CY))
# ---- move J46 to the back, just below the rear body
j=b.FindFootprintByReference('J46')
j.SetOrientationDegrees(0); j.SetPosition(mm(CX-1.25,CY+24.5))
if not j.IsFlipped(): j.Flip(j.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT if hasattr(pcbnew,'FLIP_DIRECTION_LEFT_RIGHT') else False)
for p in j.Pads(): print('J46 pad',p.GetNumber(),p.GetNetname(),round(M(p.GetPosition().x),2),round(M(p.GetPosition().y),2))
b.Save(out)
