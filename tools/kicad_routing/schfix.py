import re, uuid, sys
sys.path.insert(0,'/tmp/claude-1000/-home-tanguy-Desktop-Eeloo-Pad/2ce1b93e-994f-483b-b4d2-351d89552201/scratchpad/w2')
from sexp import match
U=lambda: str(uuid.uuid4())
PIN={'FJ5241AH':None,'XL-5050RGBC-WS2812B':{'DIN':'input','DOU':'output','VDD':'power_in','GND':'power_in'}}
def retype(txt,symname,mapping):
    out=txt; pos=0
    for hdr in ('\t(symbol "%s"'%symname,'(symbol "Eeloo_Pad:%s"'%symname):
        i=out.find(hdr)
        while i>=0:
            j=match(out,i); blk=out[i:j]
            def rep(m):
                name=re.search(r'\(name "([^"]*)"',m.group(0)).group(1)
                new=(mapping or {}).get(name,'passive') if mapping is not None else 'passive'
                return m.group(0).replace('(pin unspecified','(pin '+new,1)
            nb=re.sub(r'\(pin unspecified \w+[\s\S]*?\(name "[^"]*"',rep,blk)
            out=out[:i]+nb+out[j:]
            i=out.find(hdr,i+len(nb))
    return out
def nc(s,pts):
    add=''.join('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n'%(x,y,U()) for x,y in pts)
    k=s.rfind('\n\t(sheet_instances')
    if k<0: k=s.rfind('\n)')
    return s[:k]+'\n'+add.rstrip('\n')+s[k:]
# library
lib=open('Eeloo_Pad.kicad_sym').read()
for n,m in PIN.items(): lib=retype(lib,n,m)
open('Eeloo_Pad.kicad_sym','w').write(lib)
# control_module: LED pin types, NC flags, orphan GND
s=open('control_module.kicad_sch').read()
s=retype(s,'XL-5050RGBC-WS2812B',PIN['XL-5050RGBC-WS2812B']); s=retype(s,'XL-5050RGBC-WS2812B_1',PIN['XL-5050RGBC-WS2812B'])
i=s.find('(property "Reference" "#PWR034"'); a=s.rfind('\n\t(symbol\n',0,i); e=s.find('\n\t)\n',i)+3
assert '(at 288.29 223.52 0)' in s[a:i]; s=s[:a]+s[e:]
pts=[(129.54,273.05)]+[(31.75,y) for y in (48.26,50.8,53.34,55.88)]+[(66.04,y) for y in (27.94,30.48,33.02,35.56,43.18,45.72,48.26,50.8,58.42,60.96,71.12,73.66,76.2,78.74,83.82,86.36,88.9,91.44)]
s=nc(s,pts); open('control_module.kicad_sch','w').write(s)
# seg module: display pin types
s=open('seg_4x14_module.kicad_sch').read(); s=retype(s,'FJ5241AH',None); open('seg_4x14_module.kicad_sch','w').write(s)
# root: J1 SBU1/SBU2 no-connect
s=open('Eeloo_Pad.kicad_sch').read(); s=nc(s,[(46.99,53.34),(46.99,55.88)]); open('Eeloo_Pad.kicad_sch','w').write(s)
# uC1: R9 DNP
s=open('uC1.kicad_sch').read()
i=s.find('(property "Reference" "R9"'); a=s.rfind('\n\t(symbol\n',0,i)
blk=s[a:i]; nb=blk.replace('(dnp no)','(dnp yes)',1); assert nb!=blk; s=s[:a]+nb+s[i:]
open('uC1.kicad_sch','w').write(s)
print('done')
