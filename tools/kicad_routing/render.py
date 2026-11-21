import pcbnew,sys
from pcbnew import ToMM as M
from PIL import Image,ImageDraw
src,out=sys.argv[1],sys.argv[2]; hl=sys.argv[3] if len(sys.argv)>3 else '+3.3V'
x0,y0,x1,y1=[float(v) for v in sys.argv[4].split(',')] if len(sys.argv)>4 else (14,9,367,239)
S=float(sys.argv[5]) if len(sys.argv)>5 else 5
b=pcbnew.LoadBoard(src)
W,H=int((x1-x0)*S),int((y1-y0)*S); im=Image.new('RGB',(W,H),(20,20,20)); d=ImageDraw.Draw(im)
P=lambda x,y:((x-x0)*S,(y-y0)*S)
for t in b.GetTracks():
    if t.Type()==pcbnew.PCB_VIA_T: continue
    c=(220,60,60) if t.GetLayer()==pcbnew.F_Cu else (70,120,240)
    if t.GetNetname()==hl: c=(255,230,0)
    d.line([P(M(t.GetStart().x),M(t.GetStart().y)),P(M(t.GetEnd().x),M(t.GetEnd().y))],fill=c,width=max(1,int(M(t.GetWidth())*S)))
for f in b.GetFootprints():
    for p in f.Pads():
        bb=p.GetBoundingBox(); c=(120,120,120)
        if p.GetNetname()==hl: c=(0,255,120)
        elif p.GetNetname()=='GND': c=(90,70,40)
        d.rectangle([P(M(bb.GetLeft()),M(bb.GetTop())),P(M(bb.GetRight()),M(bb.GetBottom()))],outline=c)
    if S>=8:
        x,y=M(f.GetPosition().x),M(f.GetPosition().y); d.text(P(x,y),f.GetReference(),fill=(200,200,200))
for t in b.GetTracks():
    if t.Type()==pcbnew.PCB_VIA_T:
        x,y=M(t.GetPosition().x),M(t.GetPosition().y); r=0.3*S
        c=(255,230,0) if t.GetNetname()==hl else (200,200,200)
        d.ellipse([P(x,y)[0]-r,P(x,y)[1]-r,P(x,y)[0]+r,P(x,y)[1]+r],outline=c)
im.save(out)
