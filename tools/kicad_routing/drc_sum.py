import sys, re, collections
s=open(sys.argv[1]).read()
blocks=s.split('\n[')
c=collections.Counter()
show=('shorting_items','clearance','solder_mask_bridge','track_dangling','via_dangling','hole_clearance','hole_to_hole','holes_co_located','copper_edge_clearance','starved_thermal','tracks_crossing','courtyards_overlap')
lines=[]
for bl in blocks[1:]:
    m=re.match(r'([a-z_]+)\]',bl)
    if not m: continue
    k=m.group(1); c[k]+=1
    if k in show:
        ls=[l.strip() for l in bl.splitlines()]
        locs=[l for l in ls if l.startswith('@(')]
        if k=='courtyards_overlap' and any(('LED' in l or 'SW' in l) for l in locs): continue
        act=re.search(r'actual ([\d.]+) mm',bl)
        lines.append('%s %s| %s'%(k,act.group(1) if act else '',' | '.join(locs[:2])))
print(dict(c))
for l in lines: print(l[:260])
