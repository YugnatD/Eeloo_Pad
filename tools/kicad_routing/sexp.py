import re
def match(s,i):
    d=0; q=False
    for j in range(i,len(s)):
        c=s[j]
        if c=='"' and s[j-1]!='\\': q=not q
        elif not q:
            if c=='(': d+=1
            elif c==')':
                d-=1
                if d==0: return j+1
    raise ValueError
def libsym(lib,name):
    s=open('/usr/share/kicad/symbols/%s.kicad_sym'%lib).read()
    i=s.find('\n\t(symbol "%s"\n'%name)
    if i<0: raise KeyError(name)
    return s[i+1:match(s,i+1)], s
def pins(block):
    out=[]
    for m in re.finditer(r'\(pin (\w+) (\w+)\s*\(at ([-\d.]+) ([-\d.]+) ([-\d.]+)\)\s*\(length ([-\d.]+)\)(.*?)\(name "([^"]*)"(.*?)\(number "([^"]*)"',block,re.S):
        out.append(dict(type=m.group(1),x=float(m.group(3)),y=float(m.group(4)),a=float(m.group(5)),name=m.group(8),num=m.group(10)))
    return out
