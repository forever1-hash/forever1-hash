# -*- coding: utf-8 -*-
"""粉色圣诞树 · 龙卷开场

结构说明（保持原有骨架）：
  * proj()      透视投影，模块级，不再每帧重建
  * layout(w,h) 接收 screen.get_size() 的两个参数
  * build_*     只在启动时生成一次素材
  * 主循环顺序   天空 -> 投影/分桶 -> 粒子层 -> 流星 -> 冲击环 -> 星形 -> 叠加辉光 -> 文字
"""

import math
import random
import sys

import pygame

# ---------------------------------------------------------------- 顶部常量
BASE_W, BASE_H = 1280, 800       # 设计分辨率
WIDTH, HEIGHT = BASE_W, BASE_H   # 当前窗口尺寸，由 layout() 维护
S = 1.0                          # 相对设计稿的缩放系数
CX, CY = BASE_W // 2, BASE_H // 2

TR_H = 264.0                     # 树的世界高度（底 -> 尖）
TR_R = 118.0                     # 树底半径（约为高度的 0.45，主流审美的胖树形）
BASE_K = 1.60                    # 世界坐标 -> 像素的基准比例
K = BASE_K                       # 由 layout() 按 S 重算

FPS = 60
BG = (0, 0, 0)

AUTO_SPIN = 0.20                 # 自动旋转角速度
T_METEOR = 4.2                   # 流星出现时刻
T_IMPACT = 5.6                   # 流星命中时刻

BUCKETS = 56                     # 深度分桶数量

PINK = [
    (255, 170, 216),
    (255, 126, 190),
    (250, 206, 234),
    (238, 100, 174),
    (255, 236, 247),
]


# ------------------------------------------------- 每帧视图参数（供 proj 读取）
# proj() 提到模块级后不再持有闭包，逐帧的三角函数/相机参数放在这里更新。
VIEW = {
    "cy": 1.0, "sy": 0.0,        # cos(yaw), sin(yaw)
    "cp": 1.0, "sp": 0.0,        # cos(pitch), sin(pitch)
    "cam_d": 1.0,
    "target_y": 0.0,
    "kz": 1.0,
}


def proj(x, y, z):
    """透视投影：返回 (屏幕x, 屏幕y, 透视缩放, 深度) 或 None（在相机后面）。"""
    cy_ = VIEW["cy"]
    sy_ = VIEW["sy"]
    cp = VIEW["cp"]
    sp = VIEW["sp"]
    cam_d = VIEW["cam_d"]
    target_y = VIEW["target_y"]
    kz = VIEW["kz"]

    x1 = x * cy_ + z * sy_
    z1 = -x * sy_ + z * cy_
    y2 = y * cp - z1 * sp
    z2 = y * sp + z1 * cp
    zc = cam_d + z2
    if zc < 40:
        return None
    s = cam_d / zc
    dep = 0.5 - z2 / (2.7 * TR_R)
    dep = 0.0 if dep < 0 else (1.0 if dep > 1 else dep)
    return CX + x1 * kz * s, CY - (y2 - target_y) * kz * s, s, dep


# ---------------------------------------------------------------- 布局与素材
def layout(w, h):
    """调用处是 layout(*screen.get_size())，因此固定接收 (w, h) 两个参数。"""
    global WIDTH, HEIGHT, S, CX, CY, K
    WIDTH, HEIGHT = max(1, int(w)), max(1, int(h))
    S = min(WIDTH / float(BASE_W), HEIGHT / float(BASE_H))
    CX, CY = WIDTH // 2, HEIGHT // 2
    K = BASE_K * S


def make_sky(w, h):
    """纯黑夜空 + 细碎银白星尘。"""
    surf = pygame.Surface((w, h))
    surf.fill((0, 0, 0))
    rng = random.Random(11)
    for _ in range(int(w * h / 2600)):
        x = rng.randrange(w)
        y = rng.randrange(h)
        v = rng.randint(70, 190)
        surf.set_at((x, y), (v, v, min(255, v + 14)))
    # 少量稍大一点的星
    for _ in range(int(w * h / 26000) + 8):
        x = rng.randrange(w)
        y = rng.randrange(h)
        pygame.draw.circle(surf, (215, 220, 240), (x, y), 1)
    return surf


def make_glow(size=220):
    """径向柔光（供 BLEND_ADD 使用）：颜色随半径衰减，避免出现实心色块。"""
    surf = pygame.Surface((size, size), pygame.SRCALPHA)
    c = size / 2.0
    pivot = (int(c), int(c))
    for i in range(1, int(c) + 1):
        f = i / c
        k = (1.0 - f) ** 2.4
        col = (int(96 * k), int(34 * k), int(74 * k))
        if col == (0, 0, 0):
            break
        pygame.draw.circle(surf, col + (255,), pivot, i, 1)
    return surf


def cone_radius(v):
    """锥面半径：v=0 是底面、v=1 是树尖，底部带一点喇叭口。"""
    return max(4.0, TR_R * (1.0 - v) ** 1.10 * (1.0 + 0.16 * math.exp(-v * 12.0)))


def build_points():
    """粒子点集：底盘光环 + 螺旋主体 + 圆锥浮尘 + 亮白星屑。

    元组顺序与主循环一致：(x, y, z, size, color, app, dur, r0, th0)
    r0/th0 是该粒子的最终半径与角度，故 x = cos(th0)*r0、z = sin(th0)*r0。
    出场（龙卷）阶段由主循环把半径从 20 拉到 r0。
    """
    rng = random.Random(20241225)
    pts = []

    def push(y, r0, th0, size, color, app, dur):
        pts.append((math.cos(th0) * r0, y, math.sin(th0) * r0,
                    size, color, app, dur, r0, th0))

    cone_r = cone_radius

    def timing():
        return 0.05 + 2.4 * rng.random(), 0.8 + 1.0 * rng.random()

    # 1) 树干亮芯：沿中轴的一条细亮线，让树有厚度
    for _ in range(200):
        v = rng.random()
        app, dur = timing()
        push(TR_H * v, cone_r(v) * 0.14 * rng.random(), rng.random() * math.tau,
             0.9 + 1.6 * rng.random(), (255, 236, 248), app, dur)

    # 2) 锥体实心填充：建立连续、密实的轮廓
    for _ in range(4300):
        v = rng.random() ** 1.30
        y = TR_H * v
        r0 = cone_r(v) * rng.random() ** 0.62
        size = 0.9 + 2.0 * rng.random() ** 2
        color = PINK[rng.randrange(len(PINK))]
        app, dur = timing()
        push(y, r0, rng.random() * math.tau, size, color, app, dur)

    # 3) 表面螺旋：贴在锥面上的一层亮带，绕出玫瑰纹理
    arms = 3
    twist = 8.0
    for i in range(2000):
        v = rng.random() ** 1.15
        y = TR_H * v
        base = cone_r(v)
        th0 = (i % arms) * math.tau / arms + v * twist + rng.gauss(0.0, 0.18)
        r0 = base * (0.86 + 0.14 * rng.random())
        size = 1.0 + 2.2 * rng.random() ** 2
        color = PINK[rng.randrange(len(PINK))]
        app, dur = timing()
        push(y, r0, th0, size, color, app, dur)

    # 4) 亮白星屑
    for _ in range(260):
        v = rng.random() ** 1.15
        y = TR_H * v
        r0 = cone_r(v) * (0.55 + 0.45 * rng.random())
        app, dur = timing()
        push(y, r0, rng.random() * math.tau, 1.8 + 2.2 * rng.random(), (255, 250, 255), app, dur)

    # 5) 顶端小结点：星星下面那一小团
    for _ in range(80):
        app, dur = timing()
        push(TR_H - 6 + 12 * rng.random(), 14.0 * rng.random() ** 0.6,
             rng.random() * math.tau, 1.2 + 2.0 * rng.random(),
             (255, 226, 240), app, dur)

    # 6) 底盘光环 + 地面漩涡：白尘从树底向外旋开（参考图里最抢眼的一层）
    for i in range(300):
        th0 = i / 300.0 * math.tau + rng.gauss(0.0, 0.010)
        app, dur = timing()
        push(-1.0 + 2.0 * rng.random(), TR_R * 1.16 * (0.99 + 0.02 * rng.random()),
             th0, 1.0 + 1.6 * rng.random(), (255, 232, 248), app, dur)

    for i in range(1800):
        u = rng.random() ** 0.75
        r0 = TR_R * (1.15 + 2.6 * u)
        th0 = (-2.4 * math.log(1.0 + u * 3.0)
               + (i % 3) * math.tau / 3.0 + rng.gauss(0.0, 0.30))
        y = -2.0 + 5.0 * rng.random() * (1.0 - 0.6 * u)
        size = 0.8 + 1.5 * rng.random() ** 2
        v = int(244 - 74 * u)
        app, dur = timing()
        push(y, r0, th0, size, (v, int(v * 0.95), int(v * 0.99)), app, dur)


    return pts


def build_star_shape():
    """五角星顶点（单位半径），主循环用 draw.polygon 填充。"""
    verts = []
    n = 5
    for i in range(n * 2):
        ang = math.pi / 2.0 + i * math.pi / n
        r = 1.0 if i % 2 == 0 else 0.40
        verts.append((math.cos(ang) * r, math.sin(ang) * r))
    return verts


def make_font(size):
    """正文用衬线斜体，贴近参考图；找不到字体时退回默认字体。"""
    for name in ("georgia", "timesnewroman", "constania", "dejavuserif"):
        try:
            f = pygame.font.SysFont(name, size, italic=True)
            if f is not None:
                return f
        except Exception:
            continue
    return pygame.font.Font(None, size)


# ---------------------------------------------------------- 缠绕光条 + 礼物盒
RIBBONS = (
    # (起始相位, 缠绕总角, 起点高度比, 终点高度比, 线宽, 亮度)
    (0.20, 13.0, 0.03, 0.96, 2, 1.15),
    (2.60, 15.0, 0.06, 0.92, 1, 0.95),
)

GIFTS = (
    # (x, z, 宽, 高, 深, 自转角度)：手摆的几堆，不是等距圆环
    # 左后一堆三个，靠着树脚
    (-148, -104, 28, 24, 26, -0.62),
    (-96, -150, 36, 32, 34, 0.34),
    (-64, -186, 24, 20, 22, 0.90),
    # 正前一个单独放着
    (-6, -170, 30, 26, 28, 0.15),
    # 右前两个靠一起
    (86, -156, 34, 30, 32, -0.30),
    (130, -116, 24, 20, 22, 0.88),
    # 两侧各一个，朝向歪着
    (-178, -24, 28, 24, 26, -0.95),
    (172, -34, 30, 26, 28, 0.42),
)


def make_pool(w, h):
    """地面光池：把 radial glow 压扁成椭圆，垫在树脚当反光。"""
    gw = max(40, int(w * 0.72))
    gh = max(16, int(h * 0.22))
    return pygame.transform.smoothscale(make_glow(240), (gw, gh))


def draw_ribbons(layer, t, k_fade):
    """绕着树身向上缠绕的细光条（加色混合，靠 bloom 出光晕）。"""
    for phase, twist, v0, v1, wid, gain in RIBBONS:
        n = 96
        prev = None
        for i in range(n + 1):
            f = i / float(n)
            v = v0 + (v1 - v0) * f
            r0 = cone_radius(v) * 1.05
            th = phase + v * twist + t * 0.4
            p = proj(math.cos(th) * r0, TR_H * v, math.sin(th) * r0)
            if p is None:
                prev = None
                continue
            cur = (int(p[0]), int(p[1]))
            if prev is not None:
                a = min(1.0, k_fade * gain * (0.45 + 0.55 * p[3]))
                pygame.draw.line(layer, (int(255 * a), int(196 * a), int(238 * a)),
                                 prev, cur, wid)
            prev = cur


def draw_gifts(layer, t, k_fade, far=None):
    """树脚散落的礼物盒：大小、朝向各不相同，像人随手摆的。

    线框 12 棱 + 盒身一圈丝带 + 顶面十字 + 蝴蝶结，脚下压一圈阴影。
    礼物放在地面上、不跟着树自转，所以临时把 yaw 归零再投影；
    far=True 画远侧（先画，会被树挡住），far=False 画近侧。
    """
    keep_cy, keep_sy = VIEW["cy"], VIEW["sy"]
    VIEW["cy"], VIEW["sy"] = 1.0, 0.0
    dim = 0.62 if far else 1.0
    for gx, gz, gw, gh, gd, rot in GIFTS:
        if far is not None and (gz >= 0.0) != far:
            continue
        ca, sa = math.cos(rot), math.sin(rot)

        def P(xx, yy, zz):
            """盒子自身坐标 -> 绕自身中心转 rot -> 投影。"""
            dx, dz = xx - gx, zz - gz
            return proj(gx + dx * ca + dz * sa, yy, gz - dx * sa + dz * ca)

        x0, x1 = gx - gw * 0.5, gx + gw * 0.5
        z0, z1 = gz - gd * 0.5, gz + gd * 0.5
        y0, y1 = 0.0, gh
        cs = {}
        for xi, xx in enumerate((x0, x1)):
            for yi, yy in enumerate((y0, y1)):
                for zi, zz in enumerate((z0, z1)):
                    p = P(xx, yy, zz)
                    if p is not None:
                        cs[(xi, yi, zi)] = (int(p[0]), int(p[1]), p[3])

        # 接地阴影
        bot = [cs[k] for k in ((0, 0, 0), (1, 0, 0), (0, 0, 1), (1, 0, 1)) if k in cs]
        if len(bot) == 4 and k_fade > 0.3:
            xs = [q[0] for q in bot]
            ys = [q[1] for q in bot]
            sw = max(6, int((max(xs) - min(xs)) * 1.25))
            sh = max(3, int(sw * 0.30))
            sx = int((max(xs) + min(xs)) * 0.5 - sw * 0.5)
            sy = int((max(ys) + min(ys)) * 0.5 - sh * 0.32)
            pygame.draw.ellipse(layer, (24, 6, 20, int(110 * k_fade)),
                                (sx, sy, sw, sh))

        edges = []
        for yi in (0, 1):
            for zi in (0, 1):
                edges.append(((0, yi, zi), (1, yi, zi)))
            for xi in (0, 1):
                edges.append(((xi, yi, 0), (xi, yi, 1)))
        for xi in (0, 1):
            for zi in (0, 1):
                edges.append(((xi, 0, zi), (xi, 1, zi)))
        for a, b in edges:
            pa = cs.get(a)
            pb = cs.get(b)
            if pa and pb:
                kk = min(1.0, dim * k_fade * (0.45 + 0.65 * (pa[2] + pb[2]) * 0.5))
                pygame.draw.line(layer, (int(255 * kk), int(212 * kk), int(244 * kk)),
                                 (pa[0], pa[1]), (pb[0], pb[1]), 2)

        mx, mz = (x0 + x1) * 0.5, (z0 + z1) * 0.5
        # 丝带：绕盒身一圈（四个侧面各一条竖带）+ 顶面十字
        bands = (((mx, y1, z0), (mx, y0, z0)), ((mx, y1, z1), (mx, y0, z1)),
                 ((x0, y1, mz), (x0, y0, mz)), ((x1, y1, mz), (x1, y0, mz)),
                 ((mx, y1, z0), (mx, y1, z1)), ((x0, y1, mz), (x1, y1, mz)))
        for a3, b3 in bands:
            pa = P(*a3)
            pb = P(*b3)
            if pa and pb:
                kk = min(1.0, dim * k_fade * (0.5 + 0.6 * (pa[3] + pb[3]) * 0.5))
                pygame.draw.line(layer, (int(255 * kk), int(214 * kk), int(246 * kk)),
                                 (int(pa[0]), int(pa[1])), (int(pb[0]), int(pb[1])), 1)

        # 蝴蝶结：顶部两颗小环 + 中间结
        pc = P(mx, y1 + 3.0, mz)
        ple = P(x0, y1, mz)
        pri = P(x1, y1, mz)
        if pc is not None and ple is not None and pri is not None:
            width_px = abs(pri[0] - ple[0])
            kk = min(1.0, dim * k_fade * (0.55 + 0.5 * pc[2]))
            col = (int(255 * kk), int(224 * kk), int(248 * kk))
            r_loop = max(2, int(width_px * 0.14))
            dx = max(2, int(width_px * 0.17))
            cyy = int(pc[1])
            pygame.draw.circle(layer, col, (int(pc[0]) - dx, cyy), r_loop, 1)
            pygame.draw.circle(layer, col, (int(pc[0]) + dx, cyy), r_loop, 1)
            pygame.draw.circle(layer, col, (int(pc[0]), cyy), max(1, int(r_loop * 0.55)))
    VIEW["cy"], VIEW["sy"] = keep_cy, keep_sy


# ---------------------------------------------------------------- 主程序
def main():
    pygame.init()
    try:
        info = pygame.display.Info()
        desk_w, desk_h = info.current_w or BASE_W, info.current_h or BASE_H
    except Exception:
        desk_w, desk_h = BASE_W, BASE_H

    win_w = max(760, min(BASE_W, int(desk_w * 0.94)))
    win_h = max(520, min(BASE_H, int(desk_h * 0.88)))
    screen = pygame.display.set_mode((win_w, win_h), pygame.RESIZABLE)
    pygame.display.set_caption("粉色圣诞树 · 龙卷开场")
    layout(win_w, win_h)

    clock = pygame.time.Clock()
    font = make_font(max(20, int(46 * max(S, 0.75))))
    sky = make_sky(WIDTH, HEIGHT)
    glow = make_glow()
    pool = make_pool(WIDTH, HEIGHT)
    pts = build_points()
    star = build_star_shape()

    layer = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    buckets = [[] for _ in range(BUCKETS)]

    yaw, pitch, zoom = 0.0, -0.26, 1.0
    t = 0.0
    dragging = False
    is_full = False
    running = True

    while running:
        dt = min(clock.tick(FPS) / 1000.0, 0.05)

        t += dt
        yaw += dt * AUTO_SPIN * min(1.0, t / 2.0)
        if t < 3.0:
            pitch = -0.26 + 0.14 * (t / 3.0)

        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            yaw -= dt * 2.2
        if keys[pygame.K_RIGHT]:
            yaw += dt * 2.2
        if keys[pygame.K_UP]:
            pitch = max(-0.8, pitch - dt * 1.4)
        if keys[pygame.K_DOWN]:
            pitch = min(0.8, pitch + dt * 1.4)
        if keys[pygame.K_PLUS] or keys[pygame.K_EQUALS]:
            zoom = min(1.9, zoom + dt * 0.9)
        if keys[pygame.K_MINUS]:
            zoom = max(0.6, zoom - dt * 0.9)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key == pygame.K_q:
                    running = False
                elif event.key == pygame.K_F11:
                    is_full = not is_full
                    if is_full:
                        screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
                    else:
                        screen = pygame.display.set_mode((win_w, win_h), pygame.RESIZABLE)
                    layout(*screen.get_size())
                    sky = make_sky(WIDTH, HEIGHT)
                    pool = make_pool(WIDTH, HEIGHT)
                    layer = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
                elif event.key == pygame.K_r:
                    t = 0.0
                    yaw, pitch, zoom = 0.0, -0.26, 1.0
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:
                    dragging = True
                elif event.button == 4:
                    zoom = min(1.9, zoom + 0.08)
                elif event.button == 5:
                    zoom = max(0.6, zoom - 0.08)
            elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
                dragging = False
            elif event.type == pygame.MOUSEMOTION and dragging:
                yaw += event.rel[0] * 0.009
                pitch = max(-0.8, min(0.8, pitch + event.rel[1] * 0.008))
            elif event.type == pygame.VIDEORESIZE:
                screen = pygame.display.set_mode(
                    (max(760, event.w), max(520, event.h)), pygame.RESIZABLE)
                layout(*screen.get_size())
                sky = make_sky(WIDTH, HEIGHT)
                layer = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)

        screen.fill(BG)
        sky.set_alpha(int(255 * min(1.0, t / 0.9)))
        screen.blit(sky, (0, 0))

        VIEW["cy"] = math.cos(yaw)
        VIEW["sy"] = math.sin(yaw)
        VIEW["cp"] = math.cos(pitch)
        VIEW["sp"] = math.sin(pitch)
        VIEW["cam_d"] = 2.6 * TR_H
        VIEW["target_y"] = TR_H * 0.42
        VIEW["kz"] = K * zoom

        # ---- 粒子 -> 深度分桶
        for b in buckets:
            b.clear()

        for (x, y, z, size, color, app, dur, r0, th0) in pts:
            if t < app:
                continue
            u = (t - app) / dur
            if u >= 1.0:
                qx, qy, qz = x, y, z
                fresh = 1.0 - min(1.0, (t - app - dur) / 0.35)
            else:
                e = 1.0 - (1.0 - u) ** 2.4
                r_t = 20.0 + (r0 - 20.0) * e
                qy = -4.0 + (y + 4.0) * e
                th_t = th0 - (1.0 - e) * 2.0
                qx = math.cos(th_t) * r_t
                qz = math.sin(th_t) * r_t
                fresh = 1.0

            p = proj(qx, qy, qz)
            if p is None:
                continue
            px, py, s_, dep = p

            if T_IMPACT < t < T_IMPACT + 1.5:
                k = (t - T_IMPACT) * 620.0
                pulse = 1.3 * math.exp(-((k - (TR_H - y)) ** 2) / (2 * 68.0 ** 2))
            else:
                pulse = 0.0

            bright = (0.55 + 0.85 * dep) * (1.0 + pulse)
            c = (min(255, int((color[0] + 255 * fresh * 0.6) * bright)),
                 min(255, int((color[1] + 255 * fresh * 0.6) * bright)),
                 min(255, int((color[2] + 255 * fresh * 0.6) * bright)))

            dsize = max(1, int(round(size * (0.6 + 0.8 * dep) * s_)))
            bi = int((1.0 - dep) * (BUCKETS - 1))
            bi = 0 if bi < 0 else (BUCKETS - 1 if bi >= BUCKETS else bi)
            buckets[bi].append((int(px), int(py), dsize, c))

        layer.fill((0, 0, 0, 0))

        # 树成形后再淡入
        k_fade = 0.0 if t < 2.4 else min(1.0, (t - 2.4) / 1.2)
        # 后半圈礼物盒先画，树叶/粒子会盖在它上面 -> 才有"绕一圈"的遮挡感
        if k_fade > 0.0:
            draw_gifts(layer, t, k_fade, True)

        for bkt in buckets:
            for px, py, dsize, c in bkt:
                layer.fill(c, (px, py, dsize, dsize))

        # ---- 缠绕光条 + 树前方的礼物盒
        if k_fade > 0.0:
            draw_ribbons(layer, t, k_fade)
            draw_gifts(layer, t, k_fade, False)

        # ---- 星星
        tip_y = TR_H + 24
        tip_sx = CX
        tip_sy = CY - (tip_y * VIEW["cp"] - VIEW["target_y"]) * VIEW["kz"]

        if t >= T_IMPACT:
            prog = min(1.0, (t - T_IMPACT) / 0.45)
            e = 1.0 - (1.0 - prog) ** 3
            tw = 27.0 * (0.65 + 0.35 * e) * (1.0 + 0.04 * math.sin(t * 1.8))
        else:
            tw = 0.0

        # ---- 流星
        if T_METEOR < t < T_IMPACT:
            prog = (t - T_METEOR) / (T_IMPACT - T_METEOR)
            e = prog ** 1.35
            sx0, sy0 = CX - 0.58 * WIDTH, tip_sy - 0.10 * HEIGHT
            for j in range(48):
                ep = max(0.0, e - j * 0.011)
                mx = sx0 + (tip_sx - sx0) * ep
                my = sy0 + (tip_sy - sy0) * ep
                k = 1.0 - j / 48.0
                d = max(1, int(4.2 * k * (0.6 + 0.4 * prog)))
                layer.fill((int(255 * k), int(180 * k), int(220 * k)),
                           (int(mx), int(my), d, d))
            mx = sx0 + (tip_sx - sx0) * e
            my = sy0 + (tip_sy - sy0) * e
            layer.fill((255, 230, 245), (int(mx) - 3, int(my) - 3, 6, 6))

        # ---- 冲击环
        if T_IMPACT < t < T_IMPACT + 1.25:
            k = (t - T_IMPACT) / 1.25
            rr = 60.0 + k * TR_R * 6.2
            fade = (1.0 - k) ** 1.8
            n = 200
            for i in range(n):
                a2 = i / n * math.pi * 2
                p = proj(math.cos(a2) * rr, -2, math.sin(a2) * rr)
                if p is None:
                    continue
                px, py, s_, dep = p
                d = max(1, int((2.6 - 1.4 * k) * s_))
                layer.fill((int(255 * fade), int(170 * fade), int(210 * fade)),
                           (int(px), int(py), d, d))

        # ---- 顶端星形（填充多边形 + 外圈柔光）
        if tw > 0.5:
            poly = [(tip_sx + hx * tw, tip_sy - hy * tw) for hx, hy in star]
            halo = [(tip_sx + hx * tw * 1.55, tip_sy - hy * tw * 1.55) for hx, hy in star]
            pygame.draw.polygon(layer, (255, 176, 222, 34), halo)
            pygame.draw.polygon(layer, (255, 255, 255), poly)

        # ---- 地面光池（叠加提亮树脚）
        if k_fade > 0.0:
            ground_sy = CY + VIEW["target_y"] * VIEW["kz"]
            pool_x = CX - pool.get_width() // 2
            pool_y = int(ground_sy) - pool.get_height() // 2
            layer.blit(pool, (pool_x, pool_y), special_flags=pygame.BLEND_ADD)

        screen.blit(layer, (0, 0))
        small = pygame.transform.smoothscale(layer, (WIDTH // 6, HEIGHT // 6))
        screen.blit(pygame.transform.smoothscale(small, (WIDTH, HEIGHT)), (0, 0),
                    special_flags=pygame.BLEND_ADD)

        # ---- 命中瞬间的白闪
        if T_IMPACT <= t < T_IMPACT + 0.32:
            k = max(0.0, 1.0 - (t - T_IMPACT) / 0.32)
            flash = pygame.Surface((WIDTH, HEIGHT))
            flash.fill((255, 235, 245))
            flash.set_alpha(int(42 * k ** 2.6))
            screen.blit(flash, (0, 0))

        # ---- 流星/星星的光晕
        if T_METEOR < t < T_IMPACT:
            prog = (t - T_METEOR) / (T_IMPACT - T_METEOR)
            e2 = prog ** 1.35
            sx0, sy0 = CX - 0.58 * WIDTH, tip_sy - 0.10 * HEIGHT
            gx = sx0 + (tip_sx - sx0) * e2
            gy = sy0 + (tip_sy - sy0) * e2
            gw = max(14, int(150 * (0.4 + 0.6 * prog)))
            g = pygame.transform.smoothscale(glow, (gw, gw))
            screen.blit(g, (int(gx - gw / 2), int(gy - gw / 2)),
                        special_flags=pygame.BLEND_ADD)
        elif t >= T_IMPACT:
            gw = max(60, int(tw * 6.0))
            g = pygame.transform.smoothscale(glow, (gw, gw))
            screen.blit(g, (int(tip_sx - gw / 2), int(tip_sy - gw / 2)),
                        special_flags=pygame.BLEND_ADD)

        # ---- 文字
        pad = int(WIDTH * 0.055)
        if t > T_IMPACT:
            txt = font.render("Merry Christmas", True, (255, 246, 250))
            txt.set_alpha(int(255 * min(1.0, (t - T_IMPACT + 0.3) / 0.9)))
            ty = int(HEIGHT * 0.36)
            screen.blit(txt, (pad, ty))
            tw_ = txt.get_width()
            line = pygame.Surface((int(tw_ * 1.55), 1), pygame.SRCALPHA)
            line.fill((255, 246, 250, 120))
            line.set_alpha(int(255 * min(1.0, (t - T_IMPACT + 0.3) / 0.9)))
            screen.blit(line, (pad, ty + txt.get_height() + int(10 * S)))

        pygame.display.flip()

    pygame.quit()
    sys.exit(0)


if __name__ == "__main__":
    main()
