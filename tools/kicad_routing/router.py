"""Grid router for 2-layer KiCad boards (pcbnew python).
Obstacle raster at `res`, A* on a `grid` lattice with 45deg moves, via/turn costs,
optional per-layer direction preference, and removable obstacles (owner = item uuid)."""
import math, heapq
import numpy as np
import pcbnew
from pcbnew import FromMM, ToMM
from PIL import Image, ImageDraw

LAYERS = {'F': pcbnew.F_Cu, 'B': pcbnew.B_Cu}
# consecutive entries are 45deg apart -> turn size = index distance mod 8
DIRS = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]


def _blockany(m, s):
    H, W = m.shape
    h, w = (H + s - 1) // s, (W + s - 1) // s
    p = np.zeros((h * s, w * s), bool)
    p[:H, :W] = m
    return p.reshape(h, s, w, s).any(axis=(1, 3))


class Router:
    def __init__(self, board, region, res=0.1, grid=0.2, clr=0.2, margin=0.05, via_d=0.6, via_drill=0.3, edge_clr=0.5, hole_clr=0.3, no_via_in_pad=True):
        self.no_via_in_pad = no_via_in_pad
        self.b = board; self.res = res; self.grid = grid; self.clr = clr; self.margin = margin
        self.via_d = via_d; self.via_drill = via_drill; self.edge_clr = edge_clr; self.hole_clr = hole_clr
        self.x0, self.y0, x1, y1 = region
        self.W = int(math.ceil((x1 - self.x0) / res)) + 1; self.H = int(math.ceil((y1 - self.y0) / res)) + 1
        self.step = int(round(grid / res))
        self.items = []
        self.cnt = {(k, L): np.zeros((self.H, self.W), np.int32) for k in 'tv' for L in 'FB'}
        self.region_poly = region
        self.failed = []
        self.dirpref = None

    # ---------- raster helpers
    def _px(self, x_nm, y_nm):
        return ((ToMM(x_nm) - self.x0) / self.res, (ToMM(y_nm) - self.y0) / self.res)

    def _draw_polyset(self, ps):
        if ps.OutlineCount() == 0: return None
        bb = ps.BBox()
        ax, ay = self._px(bb.GetLeft(), bb.GetTop()); bx, by = self._px(bb.GetRight(), bb.GetBottom())
        ix0 = max(0, int(math.floor(ax)) - 1); iy0 = max(0, int(math.floor(ay)) - 1)
        ix1 = min(self.W, int(math.ceil(bx)) + 2); iy1 = min(self.H, int(math.ceil(by)) + 2)
        if ix1 <= ix0 or iy1 <= iy0: return None
        img = Image.new('L', (ix1 - ix0, iy1 - iy0), 0); d = ImageDraw.Draw(img)
        for i in range(ps.OutlineCount()):
            ol = ps.COutline(i)
            pts = [self._px(ol.CPoint(j).x, ol.CPoint(j).y) for j in range(ol.PointCount())]
            pts = [(x - ix0, y - iy0) for x, y in pts]
            if len(pts) >= 3: d.polygon(pts, fill=1)
            for h in range(ps.HoleCount(i)):
                hl = ps.CHole(i, h)
                hp = [self._px(hl.CPoint(j).x, hl.CPoint(j).y) for j in range(hl.PointCount())]
                hp = [(x - ix0, y - iy0) for x, y in hp]
                if len(hp) >= 3: d.polygon(hp, fill=0)
        patch = np.array(img, np.int32)
        if not patch.any(): return None
        return (ix0, iy0, patch)

    def _add(self, net, L, kind, r, owner=None):
        if r is None: return
        ix0, iy0, patch = r
        self.cnt[(kind, L)][iy0:iy0 + patch.shape[0], ix0:ix0 + patch.shape[1]] += patch
        self.items.append((net, L, kind, ix0, iy0, patch, owner))

    def remove_owner(self, owner):
        keep = []
        for e in self.items:
            if e[6] == owner:
                net, L, kind, ix0, iy0, patch, _ = e
                self.cnt[(kind, L)][iy0:iy0 + patch.shape[0], ix0:ix0 + patch.shape[1]] -= patch
            else:
                keep.append(e)
        self.items = keep

    def _infl(self, item, layer, amount):
        ps = pcbnew.SHAPE_POLY_SET()
        item.TransformShapeToPolygon(ps, layer, FromMM(amount), FromMM(0.005), pcbnew.ERROR_OUTSIDE)
        return ps

    def add_item(self, it, net=None):
        """register a copper item (track / via) as obstacle for other nets"""
        net = net or it.GetNetname(); w = self.w; own = it.m_Uuid.AsString()
        for L, lid in LAYERS.items():
            if it.IsOnLayer(lid):
                self._add(net, L, 't', self._draw_polyset(self._infl(it, lid, self.clr + w / 2 + self.margin)), own)
                self._add(net, L, 'v', self._draw_polyset(self._infl(it, lid, self.clr + self.via_d / 2 + self.margin)), own)

    def build(self, w=0.2):
        self.w = w
        ti = self.clr + w / 2 + self.margin
        vi = self.clr + self.via_d / 2 + self.margin
        rx0, ry0, rx1, ry1 = self.region_poly
        inreg = lambda bb: not (ToMM(bb.GetRight()) < rx0 - 2 or ToMM(bb.GetLeft()) > rx1 + 2 or ToMM(bb.GetBottom()) < ry0 - 2 or ToMM(bb.GetTop()) > ry1 + 2)
        for f in self.b.GetFootprints():
            if not inreg(f.GetBoundingBox(False, False)): continue
            for p in f.Pads():
                net = p.GetNetname()
                for L, lid in LAYERS.items():
                    if p.IsOnLayer(lid) and p.GetAttribute() != pcbnew.PAD_ATTRIB_NPTH:
                        self._add(net, L, 't', self._draw_polyset(self._infl(p, lid, ti)))
                        self._add(net, L, 'v', self._draw_polyset(self._infl(p, lid, vi)))
                if self.no_via_in_pad and p.GetAttribute() == pcbnew.PAD_ATTRIB_SMD:
                    for L, lid in LAYERS.items():
                        if p.IsOnLayer(lid):
                            self._add('__padvia__', L, 'v', self._draw_polyset(self._infl(p, lid, self.via_d / 2 + 0.2 + self.margin)))
                if p.HasHole():
                    for L in 'FB':
                        hs = pcbnew.SHAPE_POLY_SET(); p.TransformHoleToPolygon(hs, FromMM(self.hole_clr + w / 2 + self.margin), FromMM(0.005), pcbnew.ERROR_OUTSIDE)
                        self._add('__hole__' if p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH else net, L, 't', self._draw_polyset(hs))
                        hs = pcbnew.SHAPE_POLY_SET(); p.TransformHoleToPolygon(hs, FromMM(self.hole_clr + self.via_d / 2 + self.margin), FromMM(0.005), pcbnew.ERROR_OUTSIDE)
                        self._add('__hole__', L, 'v', self._draw_polyset(hs))
            for g in f.GraphicalItems():
                for L, lid in LAYERS.items():
                    if g.GetLayer() == lid and g.GetClass() == 'PCB_SHAPE':
                        self._add('__gfx__', L, 't', self._draw_polyset(self._infl(g, lid, ti)))
                        self._add('__gfx__', L, 'v', self._draw_polyset(self._infl(g, lid, vi)))
        for t in self.b.GetTracks():
            if not inreg(t.GetBoundingBox()): continue
            net = t.GetNetname(); own = t.m_Uuid.AsString()
            for L, lid in LAYERS.items():
                if t.IsOnLayer(lid):
                    self._add(net, L, 't', self._draw_polyset(self._infl(t, lid, ti)), own)
                    self._add(net, L, 'v', self._draw_polyset(self._infl(t, lid, vi)), own)
        ob = pcbnew.SHAPE_POLY_SET(); self.b.GetBoardPolygonOutlines(ob, False)
        for kind, extra in (('t', w / 2), ('v', self.via_d / 2)):
            o2 = pcbnew.SHAPE_POLY_SET(ob); o2.Inflate(-FromMM(self.edge_clr + extra + self.margin), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, FromMM(0.01))
            img = Image.new('L', (self.W, self.H), 1); d = ImageDraw.Draw(img)
            for i in range(o2.OutlineCount()):
                ol = o2.Outline(i); d.polygon([self._px(ol.CPoint(j).x, ol.CPoint(j).y) for j in range(ol.PointCount())], fill=0)
                for h in range(o2.HoleCount(i)):
                    hl = o2.Hole(i, h); d.polygon([self._px(hl.CPoint(j).x, hl.CPoint(j).y) for j in range(hl.PointCount())], fill=1)
            m = np.array(img, np.int32)
            for L in 'FB': self.cnt[(kind, L)] += m
        return self

    def blocked(self, net):
        own = {k: np.zeros((self.H, self.W), np.int32) for k in self.cnt}
        for (n, L, kind, ix0, iy0, patch, _) in self.items:
            if n == net:
                own[(kind, L)][iy0:iy0 + patch.shape[0], ix0:ix0 + patch.shape[1]] += patch
        return {k: (self.cnt[k] - own[k]) > 0 for k in self.cnt}

    def pads_of(self, net):
        return [(f.GetReference(), p) for f in self.b.GetFootprints() for p in f.Pads() if p.GetNetname() == net]

    def _mask_of_items(self, items, w=0.2):
        """start/goal cells: pads shrunk by w/2+res, tracks = centre line, vias = small disc"""
        m = {L: np.zeros((self.H, self.W), bool) for L in 'FB'}
        er = int(math.ceil((w / 2) / self.res)) + 1
        for it in items:
            for L, lid in LAYERS.items():
                if not it.IsOnLayer(lid): continue
                if it.Type() == pcbnew.PCB_TRACE_T:
                    a = self._px(it.GetStart().x, it.GetStart().y); c = self._px(it.GetEnd().x, it.GetEnd().y)
                    img = Image.new('L', (self.W, self.H), 0); ImageDraw.Draw(img).line([a, c], fill=1, width=1)
                    m[L] |= np.array(img, bool)
                elif it.Type() == pcbnew.PCB_VIA_T:
                    cx, cy = self._px(it.GetPosition().x, it.GetPosition().y)
                    r = max(1, int(0.15 / self.res))
                    y0_, y1_ = max(0, int(cy) - r), min(self.H, int(cy) + r + 1); x0_, x1_ = max(0, int(cx) - r), min(self.W, int(cx) + r + 1)
                    if y1_ > y0_ and x1_ > x0_: m[L][y0_:y1_, x0_:x1_] = True
                else:
                    rr = self._draw_polyset(self._infl(it, lid, 0.0))
                    if not rr: continue
                    ix0, iy0, patch = rr; pm = patch.astype(bool)
                    for _ in range(er):
                        q = pm.copy(); q[1:, :] &= pm[:-1, :]; q[:-1, :] &= pm[1:, :]; q[:, 1:] &= pm[:, :-1]; q[:, :-1] &= pm[:, 1:]; pm = q
                    if not pm.any():
                        pm = np.zeros_like(pm); pm[pm.shape[0] // 2, pm.shape[1] // 2] = True
                    m[L][iy0:iy0 + pm.shape[0], ix0:ix0 + pm.shape[1]] |= pm
        return m

    def route_net(self, net, terminals=None, w=0.2, via_cost=25.0, turn_cost=0.6, layer_cost=None, max_exp=12_000_000, hweight=1.4):
        if terminals is None: terminals = [[p] for _, p in self.pads_of(net)]
        self.hweight = hweight; self.w_cur = w
        blk = self.blocked(net)
        tm = [self._mask_of_items(t, w) for t in terminals]
        conn = {L: tm[0][L].copy() for L in 'FB'}
        todo = list(range(1, len(terminals)))
        results = []; self.new_items = []
        while todo:
            goal = {L: np.zeros((self.H, self.W), bool) for L in 'FB'}
            for i in todo:
                for L in 'FB': goal[L] |= tm[i][L]
            path = self._astar(conn, goal, blk, via_cost, turn_cost, layer_cost or {'F': 1.0, 'B': 1.0}, max_exp)
            if path is None:
                self.failed = list(todo)
                return None, results
            (gx, gy, gL) = path[-1]
            hit = [i for i in todo if _blockany(tm[i][gL], self.step)[gy // self.step, gx // self.step]]
            i = hit[0] if hit else todo[0]
            todo.remove(i)
            segs, vias = self._commit(net, path, w)
            results.append((segs, vias))
            for L in 'FB': conn[L] |= tm[i][L]
            nm = self._mask_of_items(self._last_items, w)
            for L in 'FB': conn[L] |= nm[L]
            blk = self.blocked(net)
        return True, results

    def _astar(self, start, goal, blk, via_cost, turn_cost, lc, max_exp):
        s = self.step; H, W = self.H, self.W
        nx, ny = len(range(0, W, s)), len(range(0, H, s))
        def seg_ok(L, ax, ay, bx, by):
            n = max(abs(bx - ax), abs(by - ay))
            for k in range(1, n):
                x = ax + (bx - ax) * k // n; y = ay + (by - ay) * k // n
                if blk[('t', L)][y, x] and not (start[L][y, x] or goal[L][y, x]): return False
            return True
        gpts = []
        for L in 'FB':
            ys, xs = np.nonzero(_blockany(goal[L], s))
            gpts += list(zip(xs.tolist(), ys.tolist()))
        if not gpts: return None
        garr = np.array(gpts)
        if len(gpts) > 2000: garr = garr[::max(1, len(gpts) // 2000)]
        def h(cx, cy):
            d = np.abs(garr - (cx, cy)); dx = d[:, 0]; dy = d[:, 1]
            return float(np.min(np.maximum(dx, dy) + (math.sqrt(2) - 1) * np.minimum(dx, dy)))
        gblk = {L: _blockany(goal[L], s) for L in 'FB'}
        sblk = {L: _blockany(start[L], s) for L in 'FB'}
        gs = {}; heap = []
        for L in 'FB':
            ys, xs = np.nonzero(sblk[L])
            for cx, cy in zip(xs.tolist(), ys.tolist()):
                gs[(cx, cy, L)] = 0.0
                heapq.heappush(heap, (self.hweight * h(cx, cy), 0.0, cx, cy, L, -1, None))
        came = {}; exp = 0; sq2 = math.sqrt(2); dp = self.dirpref
        while heap:
            f, g, cx, cy, L, d, parent = heapq.heappop(heap)
            key = (cx, cy, L)
            if key in came: continue
            came[key] = parent
            ix, iy = cx * s, cy * s
            if gblk[L][cy, cx] and parent is not None:
                path = [key]; k = parent
                while k is not None: path.append(k); k = came[k]
                path.reverse()
                return [(px * s, py * s, pl) for (px, py, pl) in path]
            exp += 1
            if exp > max_exp: return None
            for di, (dx, dy) in enumerate(DIRS):
                mx, my = cx + dx, cy + dy
                if not (0 <= mx < nx and 0 <= my < ny): continue
                jx, jy = mx * s, my * s
                if jy >= H or jx >= W: continue
                inown = start[L][jy, jx] or goal[L][jy, jx] or gblk[L][my, mx] or sblk[L][my, mx]
                if blk[('t', L)][jy, jx] and not inown: continue
                if (mx, my, L) in came: continue
                if not seg_ok(L, ix, iy, jx, jy): continue
                step = (sq2 if dx and dy else 1.0) * lc[L]
                if dp: step *= dp[L][2] if (dx and dy) else (dp[L][0] if dx else dp[L][1])
                tc = 0.0
                if d >= 0 and di != d:
                    turn = min((di - d) % 8, (d - di) % 8)
                    if turn >= 3: continue
                    tc = turn_cost * turn
                ng = g + step + tc
                nk = (mx, my, L)
                if ng < gs.get(nk, 1e18):
                    gs[nk] = ng
                    heapq.heappush(heap, (ng + self.hweight * h(mx, my), ng, mx, my, L, di, key))
            oL = 'B' if L == 'F' else 'F'
            if not blk[('v', 'F')][iy, ix] and not blk[('v', 'B')][iy, ix]:
                nk = (cx, cy, oL); ng = g + via_cost
                if nk not in came and ng < gs.get(nk, 1e18):
                    gs[nk] = ng
                    heapq.heappush(heap, (ng + self.hweight * h(cx, cy), ng, cx, cy, oL, -1, key))
        return None

    def _commit(self, net, path, w):
        pts = [(self.x0 + x * self.res, self.y0 + y * self.res, L) for (x, y, L) in path]
        segs = []; vias = []; cur = [pts[0]]
        for p in pts[1:]:
            if p[2] != cur[-1][2]:
                if len(cur) > 1: segs.append((cur[-1][2], cur))
                vias.append((p[0], p[1])); cur = [p]
            else: cur.append(p)
        if len(cur) > 1: segs.append((cur[-1][2], cur))
        out = []
        for L, poly in segs:
            q = [poly[0]]
            for i in range(1, len(poly) - 1):
                a, b_, c = q[-1], poly[i], poly[i + 1]
                if abs((b_[0] - a[0]) * (c[1] - b_[1]) - (b_[1] - a[1]) * (c[0] - b_[0])) > 1e-9: q.append(b_)
            q.append(poly[-1])
            out.append((L, [(round(x, 4), round(y, 4)) for x, y, _ in q]))
        nobj = self.b.FindNet(net); new = []
        for L, poly in out:
            for a, c in zip(poly, poly[1:]):
                t = pcbnew.PCB_TRACK(self.b); t.SetStart(pcbnew.VECTOR2I(FromMM(a[0]), FromMM(a[1]))); t.SetEnd(pcbnew.VECTOR2I(FromMM(c[0]), FromMM(c[1])))
                t.SetWidth(FromMM(w)); t.SetLayer(LAYERS[L]); t.SetNet(nobj); self.b.Add(t); new.append(t)
        for x, y in vias:
            v = pcbnew.PCB_VIA(self.b); v.SetPosition(pcbnew.VECTOR2I(FromMM(x), FromMM(y))); v.SetWidth(FromMM(self.via_d)); v.SetDrill(FromMM(self.via_drill)); v.SetNet(nobj); self.b.Add(v); new.append(v)
        vi = self.clr + self.via_d / 2 + self.margin
        for it in new:
            own = it.m_Uuid.AsString()
            for L, lid in LAYERS.items():
                if it.IsOnLayer(lid):
                    self._add(net, L, 't', self._draw_polyset(self._infl(it, lid, self.clr + self.w / 2 + self.margin)), own)
                    self._add(net, L, 'v', self._draw_polyset(self._infl(it, lid, vi)), own)
        self._last_items = new
        self.new_items.extend(new)
        return out, [(round(x, 4), round(y, 4)) for x, y in vias]
