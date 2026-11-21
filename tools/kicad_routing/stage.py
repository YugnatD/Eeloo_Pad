import re, uuid, sys
p=sys.argv[1]; s=open(p).read()
U=lambda: str(uuid.uuid4())
i=s.find('(property "Reference" "SW20"'); a=s.rfind('\n\t(symbol\n',0,i); e=s.find('\n\t)\n',i)+4
blk=s[a:e]
inst=re.search(r'\(instances\n\t\t\t\(project "([^"]+)"\n\t\t\t\t\(path "([^"]+)"',blk)
proj,path=inst.group(1),inst.group(2)
X,Y=241.3,231.14
def prop(n,v,x,y,hide=False,just=None):
    return ('\t\t(property "%s" "%s"\n\t\t\t(at %s %s 0)\n%s\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n%s\t\t\t)\n\t\t)\n')%(n,v,x,y,'\t\t\t(hide yes)\n' if hide else '',('\t\t\t\t(justify %s)\n'%just) if just else '')
sym_uuid=U()
t='\n\t(symbol\n\t\t(lib_id "Connector:Conn_01x02_Pin")\n\t\t(at %s %s 180)\n\t\t(mirror x)\n\t\t(unit 1)\n\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid "%s")\n'%(X,Y,sym_uuid)
t+=prop('Reference','J46',X+1.27,Y-1.27,just='right')
t+=prop('Value','XB2-BS542 STAGE',X+1.27,Y+3.81,just='right')
t+=prop('Footprint','Connector_JST:JST_EH_B2B-EH-A_1x02_P2.50mm_Vertical',X,Y,True)
t+=prop('Datasheet','',X,Y,True)
t+=prop('Description','Vers bouton coup-de-poing Schneider XB2-BS542 (contact 1 NF, accrochage, deverrouillage par rotation). STAGE: logique inversee + detection de front dans le firmware',X,Y,True)
t+=prop('Manufacturer_Part_Number','B2B-EH-A',X,Y,True)
t+=prop('Manufacturer_Name','JST',X,Y,True)
t+='\t\t(pin "1"\n\t\t\t(uuid "%s")\n\t\t)\n\t\t(pin "2"\n\t\t\t(uuid "%s")\n\t\t)\n'%(U(),U())
t+='\t\t(instances\n\t\t\t(project "%s"\n\t\t\t\t(path "%s"\n\t\t\t\t\t(reference "J46")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n'%(proj,path)
def wire(a,c): return '\t(wire\n\t\t(pts\n\t\t\t(xy %s %s) (xy %s %s)\n\t\t)\n\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid "%s")\n\t)\n'%(a[0],a[1],c[0],c[1],U())
w=wire((236.22,233.68),(236.22,241.3))
note='\t(text "STAGE = XB2-BS542 deporte (contact NF, accrochage)\\nfirmware: lire STAGE inverse, agir sur le front"\n\t\t(exclude_from_sim no)\n\t\t(at 215.9 246.38 0)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid "%s")\n\t)\n'%U()
s=s[:a]+t.rstrip('\n')+'\n'+w+note.rstrip('\n')+s[e-1:] if False else s[:a]+'\n'+t.lstrip('\n')+w+s[e:]
open(p,'w').write(s)
print('sheet path',path,'symbol uuid',sym_uuid)
open('j46_uuid.txt','w').write(path+'/'+sym_uuid)
