/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025–2026 Pau Díaz Cuesta
 *
 * Z-Core write-buffer / D-cache stress benchmark (original project code).
 */

#include <stdint.h>
#include <string.h>

#ifndef PIX32
#define PIX32 1
#endif

#ifndef VGA_BLIT
#define VGA_BLIT 1
#endif

#ifndef VGA_SUBSTEPS
#define VGA_SUBSTEPS 3
#endif

#include "uart.h"
#include "printf.h"
#include "timer.h"
#if VGA_BLIT && !defined(WB3D_HOST)
#include "vga.h"
#endif

#define WB_ENTRIES        8
#define DCACHE_ENTRIES    8192
#define DCACHE_WAYS       2
#define DCACHE_LINE       4
#define DCACHE_SETS       (DCACHE_ENTRIES / DCACHE_WAYS)
#define DCACHE_BYTES      (DCACHE_ENTRIES * DCACHE_LINE)
#define SET_STRIDE        (DCACHE_SETS * DCACHE_LINE)


#include <stdlib.h>
static unsigned char linux_scratch[0x240000] __attribute__((aligned(16384)));
#define SCRATCH ((uintptr_t)linux_scratch)

#define SCR_W        320
#define SCR_H        200
#define NPIX         (SCR_W * SCR_H)

static uint8_t  *const color_buf = (uint8_t  *)(SCRATCH + 0x00000000u);

static uint32_t *const depth_buf = (uint32_t *)(SCRATCH + 0x00040000u);

#define FILL_WORDS   (256 * 1024 / 4)
static uint32_t *const fill_buf  = (uint32_t *)(SCRATCH + 0x00080000u);

static uint16_t frame_tag = 1;

static uint32_t *const conf_a    = (uint32_t *)(SCRATCH + 0x00100000u);
static uint32_t *const conf_b    = (uint32_t *)(SCRATCH + 0x00100000u + SET_STRIDE);
static uint32_t *const conf_c    = (uint32_t *)(SCRATCH + 0x00100000u + 2 * SET_STRIDE);

static uint32_t *const merge_a   = (uint32_t *)(SCRATCH + 0x00180000u);
static uint32_t *const merge_b   = (uint32_t *)(SCRATCH + 0x00180000u + SET_STRIDE);
static uint32_t *const merge_c   = (uint32_t *)(SCRATCH + 0x00180000u + 2 * SET_STRIDE);

static uint32_t *const fwd_a     = (uint32_t *)(SCRATCH + 0x00200000u);
static uint32_t *const fwd_b     = (uint32_t *)(SCRATCH + 0x00200000u + SET_STRIDE);
static uint32_t *const fwd_c     = (uint32_t *)(SCRATCH + 0x00200000u + 2 * SET_STRIDE);

_Static_assert(SET_STRIDE == 16384, "set stride changed: re-derive from the RTL");


#ifdef WB3D_HOST
#define FENCE() __asm__ volatile("" ::: "memory")
static inline uint64_t instret64(void) { return 0; }
#else
#define FENCE() __asm__ volatile("fence" ::: "memory")

static inline uint64_t instret64(void)
{
    uint32_t hi, lo, hi2;
    do {
        __asm__ volatile("csrr %0, instreth" : "=r"(hi));
        __asm__ volatile("csrr %0, instret"  : "=r"(lo));
        __asm__ volatile("csrr %0, instreth" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

#endif

static void fill32(uint32_t *p, uint32_t v, uint32_t words)
{
    for (uint32_t i = 0; i < words; i++) p[i] = v;
}

static uint32_t fnv1a(const void *buf, uint32_t bytes)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < bytes; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}

static inline int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}


typedef int32_t fx;
#define FX_ONE   65536
#define FX(f)    ((fx)((f) * 65536.0))

#include "sintab.h"

static inline fx fx_sin(int32_t a) { return sintab[(uint32_t)a & 255]; }
static inline fx fx_cos(int32_t a) { return sintab[((uint32_t)a + 64) & 255]; }
static inline fx fx_mul(fx a, fx b) { return (fx)(((int64_t)a * (int64_t)b) >> 16); }

#ifndef FOCAL_PIX
#define FOCAL_PIX  235
#endif

#ifndef VIEW_Y_OFS
#define VIEW_Y_OFS 31
#endif
#ifndef CAM_DIST_F
#define CAM_DIST_F 8.5
#endif
#define CAM_DIST   FX(CAM_DIST_F)

typedef struct { int32_t x, y; int32_t z; } vtx;

static const fx cube_v[8][3] = {
    { -FX_ONE, -FX_ONE, -FX_ONE }, {  FX_ONE, -FX_ONE, -FX_ONE },
    {  FX_ONE,  FX_ONE, -FX_ONE }, { -FX_ONE,  FX_ONE, -FX_ONE },
    { -FX_ONE, -FX_ONE,  FX_ONE }, {  FX_ONE, -FX_ONE,  FX_ONE },
    {  FX_ONE,  FX_ONE,  FX_ONE }, { -FX_ONE,  FX_ONE,  FX_ONE },
};

static const uint8_t cube_i[12][3] = {
    {0,2,1},{0,3,2}, {4,5,6},{4,6,7}, {0,1,5},{0,5,4},
    {2,3,7},{2,7,6}, {0,4,7},{0,7,3}, {1,2,6},{1,6,5},
};

#define NOBJ 10

#include "palette.h"

#define CUBE_HALF   FX(0.62)
#define BOX_X       FX(3.30)
#define BOX_Y       FX(1.90)
#define BOX_ZMIN    FX(-2.40)
#define BOX_ZMAX    FX( 2.40)
#define SUBSTEPS    8

#define SPAWN_NX    3
#define SPAWN_NY    2
#define SPAWN_NZ    2

typedef struct {
    fx      px, py, pz;
    fx      vx, vy, vz;
    int32_t yaw;
    int32_t yaw_vel;
} obj_t;

static obj_t   obj[NOBJ];
static uint32_t collisions;

static uint32_t rng_state;
static inline uint32_t rnd(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

_Static_assert(SPAWN_NX * SPAWN_NY * SPAWN_NZ >= NOBJ,
               "spawn lattice has fewer cells than NOBJ");
#define ROT_SPAN  (((2 * CUBE_HALF) * 181) / 128)
_Static_assert((2 * (BOX_X - CUBE_HALF)) / SPAWN_NX >= ROT_SPAN,
               "spawn cells narrower than a spinning cube in x");
_Static_assert((2 * (BOX_Y - CUBE_HALF)) / SPAWN_NY >= 2 * CUBE_HALF,
               "spawn cells narrower than a cube in y");
_Static_assert((BOX_ZMAX - BOX_ZMIN - 2 * CUBE_HALF) / SPAWN_NZ >= ROT_SPAN,
               "spawn cells narrower than a spinning cube in z");

static void world_reset(void)
{
    rng_state  = 0x13572468u;
    collisions = 0;

    const fx x0 = -(BOX_X - CUBE_HALF), xc = (2 * (BOX_X - CUBE_HALF)) / SPAWN_NX;
    const fx y0 = -(BOX_Y - CUBE_HALF), yc = (2 * (BOX_Y - CUBE_HALF)) / SPAWN_NY;
    const fx z0 = BOX_ZMIN + CUBE_HALF;
    const fx zc = (BOX_ZMAX - BOX_ZMIN - 2 * CUBE_HALF) / SPAWN_NZ;

    const fx jx = xc - 2 * CUBE_HALF;
    const fx jy = yc - 2 * CUBE_HALF;
    const fx jz = zc - 2 * CUBE_HALF;

    for (int i = 0; i < NOBJ; i++) {
        int cx =  i % SPAWN_NX;
        int cy = (i / SPAWN_NX) % SPAWN_NY;
        int cz =  i / (SPAWN_NX * SPAWN_NY);

        obj[i].px = x0 + cx * xc + CUBE_HALF + (jx ? (fx)(rnd() % (uint32_t)jx) : 0);
        obj[i].py = y0 + cy * yc + CUBE_HALF + (jy ? (fx)(rnd() % (uint32_t)jy) : 0);
        obj[i].pz = z0 + cz * zc + CUBE_HALF + (jz ? (fx)(rnd() % (uint32_t)jz) : 0);

        obj[i].vx = (fx)(rnd() % 7200u) - 3600;
        obj[i].vy = (fx)(rnd() % 7200u) - 3600;
        obj[i].vz = (fx)(rnd() % 7200u) - 3600;

        obj[i].yaw     = 0;
        obj[i].yaw_vel = (int32_t)(rnd() % 96u) + 24;
    }
}

static inline void wall(fx *p, fx *v, fx lo, fx hi)
{
    if (*p < lo) { *p = lo; *v = -*v; }
    else if (*p > hi) { *p = hi; *v = -*v; }
}

static inline fx fx_abs(fx a) { return a < 0 ? -a : a; }

typedef struct { fx nx, nz; fx depth; int is_y; } hit_t;

static int obb_overlap(int i, int j,
                       fx ci, fx si, fx cj, fx sj, hit_t *out)
{
    const fx h  = CUBE_HALF;
    const fx dx = obj[j].px - obj[i].px;
    const fx dy = obj[j].py - obj[i].py;
    const fx dz = obj[j].pz - obj[i].pz;

    fx ov_y = 2 * h - fx_abs(dy);
    if (ov_y <= 0) return 0;

    const fx ax[4] = {  ci, -si,  cj, -sj };
    const fx az[4] = {  si,  ci,  sj,  cj };

    fx best = 0x7FFFFFFF; fx bnx = 0, bnz = 0;

    for (int k = 0; k < 4; k++) {
        const fx nx = ax[k], nz = az[k];

        fx ri = fx_mul(h, fx_abs(fx_mul(nx, ci) + fx_mul(nz, si)) +
                          fx_abs(fx_mul(nx, -si) + fx_mul(nz, ci)));
        fx rj = fx_mul(h, fx_abs(fx_mul(nx, cj) + fx_mul(nz, sj)) +
                          fx_abs(fx_mul(nx, -sj) + fx_mul(nz, cj)));

        fx dist = fx_abs(fx_mul(dx, nx) + fx_mul(dz, nz));
        fx ov   = ri + rj - dist;
        if (ov <= 0) return 0;
        if (ov < best) { best = ov; bnx = nx; bnz = nz; }
    }

    if (ov_y < best) {
        out->is_y = 1; out->depth = ov_y; out->nx = 0; out->nz = 0;
    } else {
        out->is_y = 0; out->depth = best;
        if (fx_mul(dx, bnx) + fx_mul(dz, bnz) < 0) { bnx = -bnx; bnz = -bnz; }
        out->nx = bnx; out->nz = bnz;
    }
    return 1;
}

static void resolve(int i, int j, const hit_t *h)
{
    if (h->is_y) {
        fx half = h->depth >> 1;
        if (obj[j].py < obj[i].py) half = -half;
        obj[i].py -= half; obj[j].py += half;
        fx t = obj[i].vy; obj[i].vy = obj[j].vy; obj[j].vy = t;
        return;
    }

    fx half = h->depth >> 1;
    fx sx = fx_mul(h->nx, half), sz = fx_mul(h->nz, half);
    obj[i].px -= sx; obj[i].pz -= sz;
    obj[j].px += sx; obj[j].pz += sz;

    fx vin = fx_mul(obj[i].vx, h->nx) + fx_mul(obj[i].vz, h->nz);
    fx vjn = fx_mul(obj[j].vx, h->nx) + fx_mul(obj[j].vz, h->nz);
    fx d   = vjn - vin;
    obj[i].vx += fx_mul(d, h->nx); obj[i].vz += fx_mul(d, h->nz);
    obj[j].vx -= fx_mul(d, h->nx); obj[j].vz -= fx_mul(d, h->nz);
}

static void walls_for(int i, fx c, fx sn)
{
    const fx hx = fx_mul(CUBE_HALF, fx_abs(c) + fx_abs(sn));
    wall(&obj[i].px, &obj[i].vx, -(BOX_X - hx),      BOX_X - hx);
    wall(&obj[i].py, &obj[i].vy, -(BOX_Y - CUBE_HALF), BOX_Y - CUBE_HALF);
    wall(&obj[i].pz, &obj[i].vz, BOX_ZMIN + hx,      BOX_ZMAX - hx);
}

#ifndef RESOLVE_ITERS
#define RESOLVE_ITERS 8
#endif

static void physics_step(void)
{
    fx cs[NOBJ], sn[NOBJ];

    for (int i = 0; i < NOBJ; i++) {
        obj[i].px += obj[i].vx;
        obj[i].py += obj[i].vy;
        obj[i].pz += obj[i].vz;
        obj[i].yaw += obj[i].yaw_vel;

        int idx = (obj[i].yaw >> 8) & 255;
        cs[i] = fx_cos(idx);
        sn[i] = fx_sin(idx);
        walls_for(i, cs[i], sn[i]);
    }

    for (int pass = 0; pass < RESOLVE_ITERS; pass++) {
        for (int i = 0; i < NOBJ; i++) walls_for(i, cs[i], sn[i]);

        int touched = 0;
        for (int i = 0; i < NOBJ; i++) {
            for (int j = i + 1; j < NOBJ; j++) {
                hit_t h;
                if (obb_overlap(i, j, cs[i], sn[i], cs[j], sn[j], &h)) {
                    resolve(i, j, &h);
                    touched = 1;
                    if (pass == 0) collisions++;
                }
            }
        }
        if (!touched) break;
    }
}

static inline void span_body(uint8_t *cb, uint32_t *zb, int idx, int len,
                             int32_t z, int32_t dzdx, uint8_t col)
{
    const uint32_t tag = (uint32_t)frame_tag << 16;

    for (int i = 0; i < len; i++) {
        uint16_t zv  = (uint16_t)clamp32(z >> 16, 0, 65535);
        uint32_t cur = zb[idx + i];

        uint16_t old = ((cur >> 16) == frame_tag) ? (uint16_t)cur : 0xFFFF;

        if (zv < old) {
            zb[idx + i] = tag | zv;
            cb[idx + i] = col;
        }
        z += dzdx;
    }
}

typedef void (*span_fn)(uint8_t *cb, uint32_t *zb, int idx, int len,
                        int32_t z, int32_t dzdx, uint8_t col);

#include "span_bloat.h"

static uint32_t span_rr;

static void draw_tri(uint8_t *cb, uint32_t *zb, vtx a, vtx b, vtx c,
                     uint8_t col, int bloat)
{
    int32_t area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (area <= 0) return;

    int32_t minx = a.x < b.x ? (a.x < c.x ? a.x : c.x) : (b.x < c.x ? b.x : c.x);
    int32_t maxx = a.x > b.x ? (a.x > c.x ? a.x : c.x) : (b.x > c.x ? b.x : c.x);
    int32_t miny = a.y < b.y ? (a.y < c.y ? a.y : c.y) : (b.y < c.y ? b.y : c.y);
    int32_t maxy = a.y > b.y ? (a.y > c.y ? a.y : c.y) : (b.y > c.y ? b.y : c.y);

    minx = clamp32(minx, 0, SCR_W - 1); maxx = clamp32(maxx, 0, SCR_W - 1);
    miny = clamp32(miny, 0, SCR_H - 1); maxy = clamp32(maxy, 0, SCR_H - 1);
    if (minx > maxx || miny > maxy) return;

    const int32_t s0x = b.y - c.y, s0y = c.x - b.x;
    const int32_t s1x = c.y - a.y, s1y = a.x - c.x;
    const int32_t s2x = a.y - b.y, s2y = b.x - a.x;

    int32_t w0r = (b.x - minx) * (c.y - miny) - (b.y - miny) * (c.x - minx);
    int32_t w1r = (c.x - minx) * (a.y - miny) - (c.y - miny) * (a.x - minx);
    int32_t w2r = (a.x - minx) * (b.y - miny) - (a.y - miny) * (b.x - minx);

    const int32_t inv_area = (int32_t)((1u << 30) / (uint32_t)area);

    const int32_t dznx = s0x * a.z + s1x * b.z + s2x * c.z;
    const int32_t dzny = s0y * a.z + s1y * b.z + s2y * c.z;
    const int32_t dzdx = (int32_t)(((int64_t)dznx * inv_area) >> 14);
    const int32_t dzdy = (int32_t)(((int64_t)dzny * inv_area) >> 14);

    int64_t zn_row = (int64_t)w0r * a.z + (int64_t)w1r * b.z + (int64_t)w2r * c.z;
    int32_t z_row  = (int32_t)((zn_row * inv_area) >> 14);

    for (int32_t y = miny; y <= maxy; y++) {
        int32_t w0 = w0r, w1 = w1r, w2 = w2r;

        int32_t xs = -1, xe = -1;
        for (int32_t x = minx; x <= maxx; x++) {
            if ((w0 | w1 | w2) >= 0) { if (xs < 0) xs = x; xe = x; }
            else if (xs >= 0) break;
            w0 += s0x; w1 += s1x; w2 += s2x;
        }

        if (xs >= 0) {
            int32_t z0 = z_row + (xs - minx) * dzdx;
            int  idx = (int)(y * SCR_W + xs);
            int  len = (int)(xe - xs + 1);
            if (bloat) span_tbl[span_rr++ & (SPAN_TBL_N - 1)](cb, zb, idx, len, z0, dzdx, col);
            else       span_body(cb, zb, idx, len, z0, dzdx, col);
        }

        w0r += s0y; w1r += s1y; w2r += s2y;
        z_row += dzdy;
    }
}



#define VIEW_PITCH  14

static void render_frame(uint8_t *cb, uint32_t *zb, int32_t ang, int bloat)
{
    const fx sy = fx_sin(ang),        cy = fx_cos(ang);
    const fx sx = fx_sin(VIEW_PITCH), cx = fx_cos(VIEW_PITCH);

    for (int n = 0; n < NOBJ; n++) {
        vtx sv[8];

        const int  oi = (obj[n].yaw >> 8) & 255;
        const fx   oc = fx_cos(oi), os_ = fx_sin(oi);

        for (int i = 0; i < 8; i++) {
            fx lx = fx_mul(cube_v[i][0], CUBE_HALF);
            fx ly = fx_mul(cube_v[i][1], CUBE_HALF);
            fx lz = fx_mul(cube_v[i][2], CUBE_HALF);

            fx x = fx_mul(lx, oc) - fx_mul(lz, os_) + obj[n].px;
            fx y = ly                                + obj[n].py;
            fx z = fx_mul(lx, os_) + fx_mul(lz, oc) + obj[n].pz;

            fx x1 =  fx_mul(x, cy) + fx_mul(z, sy);
            fx z1 = -fx_mul(x, sy) + fx_mul(z, cy);
            fx y2 =  fx_mul(y, cx) - fx_mul(z1, sx);
            fx z2 =  fx_mul(y, sx) + fx_mul(z1, cx) + CAM_DIST;
            if (z2 < FX(0.5)) z2 = FX(0.5);

            int32_t xn = (int32_t)(((int64_t)x1 * FOCAL_PIX) >> 16);
            int32_t yn = (int32_t)(((int64_t)y2 * FOCAL_PIX) >> 16);
            int32_t zq8 = z2 >> 8;
            if (zq8 < 1) zq8 = 1;

            sv[i].x = SCR_W / 2 + ((xn << 8) / zq8);
            sv[i].y = SCR_H / 2 + VIEW_Y_OFS - ((yn << 8) / zq8);
            sv[i].z = clamp32(zq8 * 20 - 12000, 0, 65535);
        }

        for (int t = 0; t < 12; t++)
            draw_tri(cb, zb, sv[cube_i[t][0]], sv[cube_i[t][1]], sv[cube_i[t][2]],
                     face_col[n % NHUE][t], bloat);
    }
}

static void clear_scene(void)
{
    fill32((uint32_t *)color_buf, 0x00000000u, NPIX / 4);

    if (++frame_tag == 0) {
        frame_tag = 1;
        fill32(depth_buf, 0x00000000u, NPIX);
    }
}

typedef struct {
    const char *name;
    uint64_t    cyc;
    uint64_t    ins;
    uint32_t    csum;
    uint32_t    units;
    const char *unit_name;
    int         pass;
    const char *note;
} result_t;

static result_t results[8];
static int nres;

static void record(const char *name, uint64_t cyc, uint64_t ins, uint32_t csum,
                   uint32_t units, const char *unit_name, int pass, const char *note)
{
    results[nres].name = name;      results[nres].cyc = cyc;
    results[nres].ins = ins;        results[nres].csum = csum;
    results[nres].units = units;    results[nres].unit_name = unit_name;
    results[nres].pass = pass;      results[nres].note = note;
    nres++;
}

static void phase_fill(void)
{
    uint32_t expect = 0;
    FENCE();
    uint64_t c0 = cycles64(), i0 = instret64();

#if PIX32
    for (uint32_t i = 0; i < FILL_WORDS; i++) fill_buf[i] = 0xA5A50000u + i;
    uint32_t units = FILL_WORDS;
    const char *un = "words";
    const char *note = "32-bit stores: 1 dirty eviction per pixel";
#else
    {
        uint8_t *p = (uint8_t *)fill_buf;
        for (uint32_t i = 0; i < FILL_WORDS * 4; i++) p[i] = (uint8_t)(i * 31u);
    }
    uint32_t units = FILL_WORDS * 4;
    const char *un = "bytes";
    const char *note = "8-bit stores: 1 dirty eviction per 4 pixels";
#endif

    FENCE();
    uint64_t c1 = cycles64(), i1 = instret64();

    expect = fnv1a(fill_buf, FILL_WORDS * 4);
    record("1 FILL   stream", c1 - c0, i1 - i0, expect, units, un, 1, note);
}

#define FRAMES 4

static uint32_t scene_checksum(void)
{
    uint32_t h = fnv1a(color_buf, NPIX);
    for (int i = 0; i < NPIX; i++) {
        uint32_t d = depth_buf[i];
        uint16_t v = ((d >> 16) == frame_tag) ? (uint16_t)d : 0xFFFF;
        h ^= v; h *= 16777619u;
    }
    return h;
}

static uint64_t phys_cyc, phys_ins;

static uint32_t render_phase(int bloat, uint64_t *cyc, uint64_t *ins)
{
    world_reset();
    clear_scene();
    span_rr  = 0;
    phys_cyc = 0; phys_ins = 0;

    uint64_t rc = 0, ri = 0;
    FENCE();

    for (int f = 0; f < FRAMES; f++) {
        uint64_t a0 = cycles64(), b0 = instret64();
        for (int t = 0; t < SUBSTEPS; t++) physics_step();
        uint64_t a1 = cycles64(), b1 = instret64();
        phys_cyc += a1 - a0; phys_ins += b1 - b0;

        uint64_t c0 = cycles64(), i0 = instret64();
        render_frame(color_buf, depth_buf, f * 11, bloat);
        FENCE();
        uint64_t c1 = cycles64(), i1 = instret64();
        rc += c1 - c0; ri += i1 - i0;
    }

    *cyc = rc; *ins = ri;
    return scene_checksum() ^ collisions;
}

#define CONF_N   (SET_STRIDE / 4)
#define MERGE_N  (SET_STRIDE / 4)
#define FWD_N    (SET_STRIDE / 4)

static void phase_conflict(void)
{
    const uint32_t N = CONF_N;

    fill32(conf_a, 0, N); fill32(conf_b, 0, N); fill32(conf_c, 0, N);
    FENCE();

    uint64_t c0 = cycles64(), i0 = instret64();
    for (uint32_t r = 0; r < 16; r++)
        for (uint32_t i = 0; i < N; i++) {
            conf_a[i] += i + r;
            conf_b[i] += conf_a[i];
            conf_c[i] += conf_b[i];
        }
    FENCE();
    uint64_t c1 = cycles64(), i1 = instret64();

    int pass = 1;
    for (uint32_t i = 0; i < N; i += 257) {
        uint32_t a = 0, b = 0, cc = 0;
        for (uint32_t r = 0; r < 16; r++) { a += i + r; b += a; cc += b; }
        if (conf_a[i] != a || conf_b[i] != b || conf_c[i] != cc) { pass = 0; break; }
    }

    record("4 CONFLICT thrash", c1 - c0, i1 - i0,
           fnv1a(conf_c, N * 4), N * 16 * 3, "accesses", pass,
           "3 lines/set in a 2-way cache: dirty eviction every iteration");
}

static void phase_merge(void)
{
    const uint32_t N = MERGE_N;

    fill32(merge_a, 0, N); fill32(merge_b, 1, N); fill32(merge_c, 2, N);
    FENCE();

    uint32_t sink = 0;
    uint64_t c0 = cycles64(), i0 = instret64();
    for (uint32_t i = 0; i < N; i++)
        for (uint32_t r = 0; r < 16; r++) {
            merge_a[i] = r + i;
            sink += merge_b[i] + merge_c[i];
        }
    FENCE();
    uint64_t c1 = cycles64(), i1 = instret64();

    int pass = (sink == N * 16u * 3u);
    for (uint32_t i = 0; i < N; i += 129)
        if (merge_a[i] != 15u + i) { pass = 0; break; }

    record("5 MERGE  tail", c1 - c0, i1 - i0, sink, N * 16, "iterations", pass,
           "every buffer push is the same address: do_merge on all but the first");
}

static void phase_forward(void)
{
    const uint32_t N = FWD_N;

    fill32(fwd_a, 0, N); fill32(fwd_b, 0, N); fill32(fwd_c, 0, N);
    FENCE();

    uint32_t bad = 0, sink = 0;
    uint64_t c0 = cycles64(), i0 = instret64();
    for (uint32_t r = 0; r < 16; r++)
        for (uint32_t i = 0; i < N; i++) {
            uint32_t v = (r << 20) ^ (i * 2654435761u);
            fwd_a[i] = v;
            fwd_b[i] = v + 1;
            fwd_c[i] = v + 2;
            uint32_t rb = fwd_a[i];
            if (rb != v) bad++;
            sink += rb;
        }
    FENCE();
    uint64_t c1 = cycles64(), i1 = instret64();

    record("6 FORWARD ld-fwd", c1 - c0, i1 - i0, sink, N * 16, "readbacks",
           bad == 0, bad == 0 ? "every evicted store read back correctly (fwd or drained)"
                              : "STALE LOADS -- load_forward path is broken");
}

int main(void)
{
    uart_puts("\r\n");
    uprintf("=============================================================\n");
    uprintf(" wb3d -- Z-Core write-buffer benchmark @ %u MHz\n", (unsigned)(CPU_HZ / 1000000UL));
    uprintf("=============================================================\n");
    uprintf(" write buffer : %u entries\n", WB_ENTRIES);
    uprintf(" D-cache      : %u B, %u-way, %u B line, %u sets\n",
            DCACHE_BYTES, DCACHE_WAYS, DCACHE_LINE, DCACHE_SETS);
    uprintf(" set aliasing : every %u B\n", SET_STRIDE);
    uprintf(" scene        : %ux%u, %u cubes, %u frames, working set %u B\n",
            SCR_W, SCR_H, NOBJ, FRAMES, NPIX * 3);
    uprintf(" physics      : %u substeps/frame, %u pair-tests/step\n",
            SUBSTEPS, (NOBJ * (NOBJ - 1)) / 2);
    uprintf(" build        : -O2, PIX32=%u\n", PIX32);
    uprintf("-------------------------------------------------------------\n");

    uprintf(" [1/6] fill ...\n");
    phase_fill();

    uprintf(" [2/6] render, tight loop ...\n");
    uint64_t c2, i2;
    uint32_t csum_tight = render_phase(0, &c2, &i2);
    record("2 RENDER tight", c2, i2, csum_tight, NPIX * FRAMES, "px-slots", 1,
           "small inner loop: I-cache hits, buffer drains in the background");
    record("2b PHYSICS collide", phys_cyc, phys_ins, collisions,
           (uint32_t)((NOBJ * (NOBJ - 1)) / 2) * SUBSTEPS * FRAMES, "pair-tests", 1,
           "O(N^2) AABB + elastic resolve; ALU-bound, timed apart from raster");

    uprintf(" [3/6] render, I-cache hostile ...\n");
    uint64_t c3, i3;
    uint32_t csum_bloat = render_phase(1, &c3, &i3);
    record("3 RENDER bloat", c3, i3, csum_bloat, NPIX * FRAMES, "px-slots",
           csum_bloat == csum_tight,
           csum_bloat == csum_tight ? "identical work via 256 span copies (~64 KB)"
                                    : "CHECKSUM DIVERGED from phase 2 -- data corruption");

    uprintf(" [4/6] conflict thrash ...\n");
    phase_conflict();

    uprintf(" [5/6] tail merge ...\n");
    phase_merge();

    uprintf(" [6/6] load forwarding ...\n");
    phase_forward();

    uprintf("\n-------------------------------------------------------------\n");
    uprintf(" Results\n");
    uprintf("-------------------------------------------------------------\n");
    uprintf(" %-18s %12s %10s %6s %8s\n", "phase", "cycles", "instr", "IPC", "cyc/op");

    int all_pass = 1;
    for (int i = 0; i < nres; i++) {
        result_t *r = &results[i];
        uint32_t ipc  = r->cyc ? (uint32_t)((r->ins * 100u) / r->cyc) : 0;
        uint32_t cpo  = r->units ? (uint32_t)((r->cyc * 100u) / r->units) : 0;
        uprintf(" %-18s %12llu %10llu  %u.%02u %5u.%02u\n",
                r->name, (unsigned long long)r->cyc, (unsigned long long)r->ins,
                ipc / 100, ipc % 100, cpo / 100, cpo % 100);
        if (!r->pass) all_pass = 0;
    }

    uprintf("\n Detail\n");
    for (int i = 0; i < nres; i++) {
        result_t *r = &results[i];
        uprintf("  %-18s %-4s  %llu %s, csum 0x%08x\n"
                "      %s\n",
                r->name, r->pass ? "PASS" : "FAIL",
                (unsigned long long)r->units, r->unit_name, r->csum, r->note);
    }

    if (c2 && c3) {
        uint32_t ratio = (uint32_t)((c3 * 100u) / c2);
        uprintf("\n Drain-prediction A/B (phase 3 vs phase 2)\n");
        uprintf("  cycles   x%u.%02u\n", ratio / 100, ratio % 100);
        uprintf("  Identical rasterization work, physics excluded. The delta is\n"
                "  opportunistic draining (instruction_hit_pred[2]) being suppressed\n"
                "  by I-cache misses, leaving only forced drains on full_out.\n");
    }

    uprintf("\n=============================================================\n");
    uprintf(" %s\n", all_pass ? " ALL PHASES PASS"
                              : " *** FAILURE -- see FAIL rows above ***");
    uprintf("=============================================================\n");

#if VGA_BLIT && !defined(WB3D_HOST)


    uprintf("\n animating on VGA -- press KEY[0] to reset\n");
    world_reset();

    uint64_t fps_mark = cycles64();
    uint32_t fps_frames = 0;

    uint64_t t_phys = 0, t_clear = 0, t_rend = 0, t_blit = 0;

    for (int32_t ang = 0; ; ang++) {
        uint64_t a = cycles64();
        for (int t = 0; t < VGA_SUBSTEPS; t++) physics_step();

        uint64_t b = cycles64();
        clear_scene();

        uint64_t c = cycles64();
        render_frame(color_buf, depth_buf, ang, 0);

        uint64_t d = cycles64();
        while (!(VGA_FB_STATUS & 0x01))
            ;
        VGA_FB_ADDR = 0;
        {
            const uint32_t *src = (const uint32_t *)color_buf;
            for (int i = 0; i < NPIX / 4; i++) VGA_FB_DATA4 = src[i];
        }
        uint64_t e = cycles64();

        t_phys += b - a; t_clear += c - b; t_rend += d - c; t_blit += e - d;

        if (++fps_frames == 16) {
            uint64_t now = cycles64();
            uint64_t dc  = now - fps_mark;
            uint32_t cfp = (uint32_t)(dc / 16u);
            uint32_t f100 = cfp ? (uint32_t)((100ull * CPU_HZ) / cfp) : 0;
            uprintf(" [VGA] %u.%02u fps  %u cyc/frame | phys %u%% clear %u%% "
                    "rast %u%% blit %u%% | %u collisions\n",
                    f100 / 100, f100 % 100, cfp,
                    (uint32_t)(100 * t_phys  / dc), (uint32_t)(100 * t_clear / dc),
                    (uint32_t)(100 * t_rend  / dc), (uint32_t)(100 * t_blit  / dc),
                    collisions);
            fps_mark   = now;
            fps_frames = 0;
            t_phys = t_clear = t_rend = t_blit = 0;
        }
    }
#endif

#ifndef WB3D_HOST
#endif
    return all_pass ? 0 : 1;
}
