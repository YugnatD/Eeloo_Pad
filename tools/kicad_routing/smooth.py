"""Line-of-sight smoothing of routes made by router.Router (removes A* staircases).
smooth(R, b, net, items) -> new item list. Chains of same-layer segments between fixed points
(vias, branch points, chain ends) are rebuilt greedily with octilinear (0/45/90 deg) moves
checked against the router obstacle mask (other nets + keepouts + board edge)."""
import pcbnew, math
from pcbnew import FromMM, ToMM as M, VECTOR2I as V

LAYERS = {'F': pcbnew.F_Cu, 'B': pcbnew.B_Cu}

def _free(R, blk, L, a, c):
    m = blk[('t', L)]
    n = max(2, int(math.hypot(c[0] - a[0], c[1] - a[1]) / (R.res * 0.5)) + 1)
    for k in range(n + 1):
        x = a[0] + (c[0] - a[0]) * k / n; y = a[1] + (c[1] - a[1]) * k / n
        px, py = R._px(FromMM(x), FromMM(y)); ix, iy = int(round(px)), int(round(py))
        if iy < 0 or ix < 0 or iy >= m.shape[0] or ix >= m.shape[1] or m[iy, ix]: return False
    return True

def _octi(a, c):
    dx, dy = c[0] - a[0], c[1] - a[1]
    if abs(dx) < 1e-6 or abs(dy) < 1e-6 or abs(abs(dx) - abs(dy)) < 1e-6: return [[a, c]]
    d = min(abs(dx), abs(dy)); sx = math.copysign(1, dx); sy = math.copysign(1, dy)
    m1 = (a[0] + sx * d, a[1] + sy * d); m2 = (c[0] - sx * d, c[1] - sy * d)
    return [[a, m1, c], [a, m2, c]]

def smooth(R, b, net, items):
    blk = R.blocked(net)
    segs = [t for t in items if t.Type() == pcbnew.PCB_TRACE_T]
    vias = [t for t in items if t.Type() == pcbnew.PCB_VIA_T]
    key = lambda p: (round(M(p.x), 4), round(M(p.y), 4))
    out = list(vias)
    nobj0 = b.FindNet(net)
    for L, lid in LAYERS.items():
        ss = [t for t in segs if t.GetLayer() == lid]
        if not ss: continue
        # split new segments at T-junctions (an endpoint of any own-net segment, new or existing,
        # or a via lying inside a new segment) so that the junction becomes a fixed vertex
        allown = [t for t in b.GetTracks() if t.GetNetname() == net and (t.Type() == pcbnew.PCB_VIA_T or t.GetLayer() == lid)]
        pts = set()
        for t in allown:
            if t.Type() == pcbnew.PCB_VIA_T: pts.add(key(t.GetPosition()))
            else: pts.add(key(t.GetStart())); pts.add(key(t.GetEnd()))
        tj = set()
        changed = True
        while changed:
            changed = False
            for t in list(ss):
                a, c = key(t.GetStart()), key(t.GetEnd())
                for p in pts:
                    if p == a or p == c: continue
                    if abs((c[0]-a[0])*(p[1]-a[1]) - (c[1]-a[1])*(p[0]-a[0])) > 1e-6: continue
                    if min(a[0],c[0])-1e-6 <= p[0] <= max(a[0],c[0])+1e-6 and min(a[1],c[1])-1e-6 <= p[1] <= max(a[1],c[1])+1e-6:
                        t2 = pcbnew.PCB_TRACK(b); t2.SetStart(V(FromMM(p[0]), FromMM(p[1]))); t2.SetEnd(t.GetEnd())
                        t2.SetWidth(t.GetWidth()); t2.SetLayer(lid); t2.SetNet(nobj0); b.Add(t2)
                        t.SetEnd(V(FromMM(p[0]), FromMM(p[1]))); ss.append(t2); tj.add(p); changed = True; break
                if changed: break
        w = ss[0].GetWidth()
        adj = {}
        for t in ss:
            a, c = key(t.GetStart()), key(t.GetEnd())
            adj.setdefault(a, []).append((c, t)); adj.setdefault(c, []).append((a, t))
        fixed = {p for p, l in adj.items() if len(l) != 2} | {key(v.GetPosition()) for v in vias} | tj
        # vertices touched by existing own-net copper (pads, old tracks/vias) stay fixed too
        for p in list(adj):
            if p in fixed: continue
            c = pcbnew.SHAPE_CIRCLE(V(FromMM(p[0]), FromMM(p[1])), FromMM(0.01))
            for t in allown:
                if t.m_Uuid.AsString() in {u.m_Uuid.AsString() for u in ss}: continue
                if t.GetEffectiveShape(lid).Collide(c, 0): fixed.add(p); break
            else:
                for f in b.GetFootprints():
                    if any(pd.GetNetname() == net and pd.IsOnLayer(lid) and pd.GetEffectiveShape(lid).Collide(c, 0) for pd in f.Pads()): fixed.add(p); break
        used = set(); chains = []
        for s in list(adj):
            if s not in fixed: continue
            for nb, t in adj[s]:
                if t.m_Uuid.AsString() in used: continue
                ch = [s]; cur, tt = nb, t; used.add(t.m_Uuid.AsString()); ch.append(cur)
                while cur not in fixed:
                    nxt = [(q, u) for q, u in adj[cur] if u.m_Uuid.AsString() not in used]
                    if not nxt: break
                    q, u = nxt[0]; used.add(u.m_Uuid.AsString()); cur = q; ch.append(cur)
                chains.append(ch)
        for t in ss:
            if t.m_Uuid.AsString() not in used: chains.append([key(t.GetStart()), key(t.GetEnd())]); used.add(t.m_Uuid.AsString())
        for t in ss: b.Remove(t)
        nobj = b.FindNet(net)
        for ch in chains:
            i = 0; pts = [ch[0]]
            while i < len(ch) - 1:
                done = False
                for j in range(len(ch) - 1, i, -1):
                    for path in _octi(ch[i], ch[j]):
                        if all(_free(R, blk, L, p, q) for p, q in zip(path, path[1:])):
                            pts.extend(path[1:]); i = j; done = True; break
                    if done: break
                if not done: pts.append(ch[i + 1]); i += 1
            # merge collinear
            q = [pts[0]]
            for k in range(1, len(pts) - 1):
                a, p, c = q[-1], pts[k], pts[k + 1]
                if abs((p[0] - a[0]) * (c[1] - p[1]) - (p[1] - a[1]) * (c[0] - p[0])) > 1e-9: q.append(p)
            q.append(pts[-1])
            for a, c in zip(q, q[1:]):
                if a == c: continue
                t = pcbnew.PCB_TRACK(b); t.SetStart(V(FromMM(a[0]), FromMM(a[1]))); t.SetEnd(V(FromMM(c[0]), FromMM(c[1])))
                t.SetWidth(w); t.SetLayer(lid); t.SetNet(nobj); b.Add(t); out.append(t)
    return out
