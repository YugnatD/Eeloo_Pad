import re,sys
def nets(f):
    s=open(f).read(); s=s[s.find('(nets'):]
    out={}
    for blk in re.split(r'\n\t\t\(net\n',s)[1:]:
        name=re.search(r'\(name "([^"]*)"\)',blk).group(1)
        out[name]=re.findall(r'\(ref "([^"]+)"\)\s*\(pin "([^"]+)"\)',blk)
    return out
