"""List signal nets involved in copper DRC errors (clearance/short/dangling/crossing)."""
import sys,re
s=open(sys.argv[1]).read(); keep=set(open(sys.argv[2]).read().split()) if len(sys.argv)>2 else None
bad=set()
for bl in s.split('\n[')[1:]:
    k=bl.split(']')[0]
    if k in ('clearance','shorting_items','track_dangling','via_dangling','tracks_crossing','hole_clearance','hole_to_hole','holes_co_located','solder_mask_bridge'):
        for n in re.findall(r'\[([^\]]+)\]',bl):
            if n!='GND' and (keep is None or n in keep): bad.add(n)
print(' '.join(sorted(bad)))
