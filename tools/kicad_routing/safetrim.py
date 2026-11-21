"""Safe dangling cleanup: removes a track end / via only if it touches nothing on one side
AND the net's island count does not increase. Never use on GND (zones not considered)."""
import pcbnew
from pcbnew import FromMM
from clusters import clusters
def _touch(b, net, item, pt, layer, own):
    c = pcbnew.SHAPE_CIRCLE(pt, FromMM(0.005))
    for t in own:
        if t.m_Uuid.AsString() == item.m_Uuid.AsString(): continue
        if t.IsOnLayer(layer) and t.GetEffectiveShape(layer).Collide(c, 0): return True
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetname() == net and p.IsOnLayer(layer) and p.GetEffectiveShape(layer).Collide(c, 0): return True
    return False
def safe_trim(b, nets):
    removed = 0
    for net in nets:
        if net == 'GND': continue
        changed = True
        while changed:
            changed = False
            own = [t for t in b.GetTracks() if t.GetNetname() == net]
            n0 = len(clusters(b, net))
            for t in own:
                if t.Type() == pcbnew.PCB_TRACE_T:
                    L = t.GetLayer()
                    dang = t.GetLength() < FromMM(0.06) or not _touch(b, net, t, t.GetStart(), L, own) or not _touch(b, net, t, t.GetEnd(), L, own)
                else:
                    lay = [L for L in (pcbnew.F_Cu, pcbnew.B_Cu) if _touch(b, net, t, t.GetPosition(), L, own)]
                    dang = len(lay) < 2
                if not dang: continue
                b.Remove(t)
                if len(clusters(b, net)) > n0: b.Add(t); continue
                removed += 1; changed = True; break
    return removed
