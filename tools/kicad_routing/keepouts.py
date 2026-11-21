import pcbnew
from pcbnew import FromMM
def add_keepouts(R, b, margin=0.05):
    """Add every rule area (board + footprints) that forbids tracks / vias as router obstacles."""
    zs = list(b.Zones()) + [z for f in b.GetFootprints() for z in f.Zones()]
    n = 0
    for z in zs:
        if not z.GetIsRuleArea(): continue
        for L, lid in (('F', pcbnew.F_Cu), ('B', pcbnew.B_Cu)):
            if not z.IsOnLayer(lid): continue
            for kind, forbid, extra in (('t', z.GetDoNotAllowTracks(), R.w / 2), ('v', z.GetDoNotAllowVias(), R.via_d / 2)):
                if not forbid: continue
                ps = pcbnew.SHAPE_POLY_SET(z.Outline()); ps.Inflate(FromMM(extra + margin), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, FromMM(0.01))
                r = R._draw_polyset(ps)
                if r: R._add('__keepout__', L, kind, r); n += 1
    return n
