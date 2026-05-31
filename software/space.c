/*
 *  STAR ASSAULT v2 — Z-Core RISC-V Space Shooter
 *
 *  320x200 VGA framebuffer, Bresenham stretched to 640x480
 *  GPIO bit 8 = left, bit 9 = right, auto-fire
 *
 *  v2 additions: wave system, power-ups (MULTI/RAPID/SHIELD),
 *  enemy movement patterns (zigzag, diver), difficulty scaling
 */

#include "libs/uart.h"
#include "libs/vga.h"


#define GPIO_LOW     (*((volatile unsigned int *)0x04001000))
#define GPIO_DIR_LOW (*((volatile unsigned int *)0x04001008))

/* ─── Entity limits ─── */
#define MAX_BULLETS   8
#define MAX_ENEMIES   8
#define MAX_STARS    30
#define MAX_EXPL      4
#define MAX_PUPS      3

/* ─── Power-up types ─── */
#define PUP_MULTI    0   /* 3-way spread shot */
#define PUP_RAPID    1   /* 2x fire rate       */
#define PUP_SHIELD   2   /* absorb one hit      */
#define PUP_DROP_PCT 20  /* % drop chance on kill */

/* ─── Wave timing ─── */
#define WAVE_PAUSE   90  /* frames between waves */

/* ─── Sprite dimensions ─── */
#define SHIP_W  7
#define SHIP_H  6
#define ENM_W   7
#define ENM_H   5

/* ─── Screen layout ─── */
#define HUD_H    14
#define GAME_TOP HUD_H
#define SHIP_Y   (VGA_HEIGHT - SHIP_H - 4)

/* ═══════════════ RNG / PERF ═══════════════ */

static unsigned int rng_seed = 54321;
static int rng(int max) {
    rng_seed = rng_seed * 1103515245u + 12345;
    return (int)((rng_seed >> 16) % (unsigned int)max);
}

static inline unsigned int rdcycle(void) {
    unsigned int v;
    asm volatile("csrr %0, mcycle" : "=r"(v));
    return v;
}

/* ═══════════════ SPRITE DATA ═══════════════ */

/* 7-wide sprites: bit (w-1-col) set = pixel on */
static const unsigned char ship_spr[SHIP_H] = {
    0x08, /*  ...#...  */
    0x1C, /*  ..###..  */
    0x3E, /*  .#####.  */
    0x7F, /*  ####### */
    0x5D, /*  #.###.#  */
    0x14  /*  ..#.#..  */
};

static const unsigned char enm1_spr[ENM_H] = {   /* type 0: alien */
    0x2A, /*  .#.#.#.  */
    0x7F, /*  ####### */
    0x3E, /*  .#####.  */
    0x55, /*  #.#.#.#  */
    0x22  /*  .#...#.  */
};

static const unsigned char enm2_spr[ENM_H] = {   /* type 1: zigzagger */
    0x08, /*  ...#...  */
    0x3E, /*  .#####.  */
    0x7F, /*  ####### */
    0x3E, /*  .#####.  */
    0x08  /*  ...#...  */
};

static const unsigned char enm3_spr[ENM_H] = {   /* type 2: diver */
    0x08, /*  ...#...  */
    0x1C, /*  ..###..  */
    0x2A, /*  .#.#.#.  */
    0x3E, /*  .#####.  */
    0x1C  /*  ..###..  */
};

/* ─── 3x5 digit font ─── */
static const unsigned char font_d[10][5] = {
    {7,5,5,5,7},{2,2,2,2,2},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}
};

/* ─── 3x5 letter bitmaps: W, A, V, E ─── */
static const unsigned char font_l[4][5] = {
    {5,5,7,5,5},  /* W */
    {2,5,7,5,5},  /* A */
    {5,5,5,2,2},  /* V */
    {7,4,6,4,7},  /* E */
};

/* ═══════════════ GAME STATE ═══════════════ */

static struct { int x, y; unsigned char active; }
    bullets[MAX_BULLETS];

/* vx: signed horizontal speed (zigzag/diver); type: 0/1/2 */
static struct { int x, y; signed char vx; unsigned char active, type, hp; }
    enemies[MAX_ENEMIES];

static struct { int x, y; unsigned char timer; }
    expls[MAX_EXPL];

static struct { short x; unsigned char y, spd; }   /* x was unsigned char → short: fixes 0..255 clamp */
    stars[MAX_STARS];

static struct { short x, y; unsigned char type, active; }
    pups[MAX_PUPS];

static int ship_x, score, hi_score, lives, frame;
static int fire_cd, invuln;

/* Wave state */
static int wave_num, wave_spawn_left, wave_spawn_cd, wave_pause;

/* Power-up state */
static int pup_weapon;        /* 0=none 1=MULTI 2=RAPID */
static int pup_weapon_timer;  /* frames remaining        */
static int pup_shield;        /* 1=shield active         */

/* Shadow state for dirty-rect rendering (prev-frame positions) */
static struct { short x; unsigned char y; } shad_stars[MAX_STARS];
static struct { int x, y; unsigned char active; } shad_bul[MAX_BULLETS];
static struct { int x, y; unsigned char active; } shad_enm[MAX_ENEMIES];
static struct { int x, y; unsigned char active; } shad_expl[MAX_EXPL];
static struct { short x, y; unsigned char active; } shad_pup[MAX_PUPS];
static int shad_ship_x;
static int shad_wave_pause;

/* ═══════════════ DRAWING HELPERS ═══════════════ */

static void draw_spr(int sx, int sy, const unsigned char *d,
                     int w, int h, unsigned char col)
{
    for (int r = 0; r < h; r++) {
        int yy = sy + r;
        if (yy < GAME_TOP || (unsigned)yy >= (unsigned)VGA_HEIGHT) continue;
        unsigned char row = d[r];
        for (int c = 0; c < w; c++) {
            if (row & (1 << (w - 1 - c))) {
                int xx = sx + c;
                if ((unsigned)xx < (unsigned)VGA_WIDTH)
                    vga_set_pixel(xx, yy, col);
            }
        }
    }
}

static void draw_num(int x, int y, int num, unsigned char col)
{
    char buf[6]; int n = 0;
    if (num <= 0) { buf[n++] = 0; }
    else { while (num > 0 && n < 6) { buf[n++] = (char)(num % 10); num /= 10; } }
    for (int i = n - 1; i >= 0; i--) {
        const unsigned char *g = font_d[(int)buf[i]];
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++)
                if (g[r] & (4 >> c))
                    vga_set_pixel(x + c, y + r, col);
        x += 4;
    }
}

static void draw_num_2x(int x, int y, int num, unsigned char col)
{
    char buf[6]; int n = 0;
    if (num <= 0) { buf[n++] = 0; }
    else { while (num > 0 && n < 6) { buf[n++] = (char)(num % 10); num /= 10; } }
    for (int i = n - 1; i >= 0; i--) {
        const unsigned char *g = font_d[(int)buf[i]];
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++)
                if (g[r] & (4 >> c))
                    vga_fill_rect(x + c * 2, y + r * 2, 2, 2, col);
        x += 8;
    }
}

static void draw_letter(int x, int y, int idx, unsigned char col)
{
    const unsigned char *g = font_l[idx];
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 3; c++)
            if (g[r] & (4 >> c))
                vga_set_pixel(x + c, y + r, col);
}

static void draw_expl(int cx, int cy, int t)
{
    static const unsigned char cols[] = { 0xFF, 0xFC, 0xEC, 0xE0 };
    unsigned char c = cols[t < 4 ? t : 3];
    int s = t + 1;
    for (int i = -s; i <= s; i++) {
        int px, py;
        px = cx + i; py = cy;
        if ((unsigned)px < (unsigned)VGA_WIDTH &&
            (unsigned)py < (unsigned)VGA_HEIGHT && py >= GAME_TOP)
            vga_set_pixel(px, py, c);
        px = cx; py = cy + i;
        if ((unsigned)px < (unsigned)VGA_WIDTH &&
            (unsigned)py < (unsigned)VGA_HEIGHT && py >= GAME_TOP)
            vga_set_pixel(px, py, c);
    }
    if (s > 1) {
        int d = s - 1;
        for (int k = 0; k < 4; k++) {
            int ddx = (k & 1) ? d : -d;
            int ddy = (k & 2) ? d : -d;
            int ppx = cx + ddx, ppy = cy + ddy;
            if ((unsigned)ppx < (unsigned)VGA_WIDTH &&
                (unsigned)ppy < (unsigned)VGA_HEIGHT && ppy >= GAME_TOP)
                vga_set_pixel(ppx, ppy, c);
        }
    }
}

static void hline(int x, int y, int w, unsigned char c)
{
    if ((unsigned)y >= (unsigned)VGA_HEIGHT) return;
    VGA_FB_ADDR = (unsigned)(y * VGA_WIDTH + x);
    for (int i = 0; i < w && x + i < VGA_WIDTH; i++)
        VGA_FB_DATA = c;
}

/* Erase a rectangle, clamped to the game area [GAME_TOP, VGA_HEIGHT). */
static void erase_rect(int x0, int y0, int w, int h)
{
    int y1 = y0 < GAME_TOP ? GAME_TOP : y0;
    int y2 = y0 + h < VGA_HEIGHT ? y0 + h : VGA_HEIGHT;
    int x1 = x0 < 0 ? 0 : x0;
    int x2 = x0 + w < VGA_WIDTH ? x0 + w : VGA_WIDTH;
    if (x1 >= x2 || y1 >= y2) return;
    for (int y = y1; y < y2; y++) {
        VGA_FB_ADDR = (unsigned)(y * VGA_WIDTH + x1);
        for (int x = x1; x < x2; x++) VGA_FB_DATA = VGA_BLACK;
    }
}

/* ═══════════════ INIT / WAVE ═══════════════ */

static void init_stars(void)
{
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x   = (short)rng(VGA_WIDTH);
        stars[i].y   = (unsigned char)(GAME_TOP + rng(VGA_HEIGHT - GAME_TOP));
        stars[i].spd = (unsigned char)((i % 3) + 1);  /* speeds 1, 2, 3 */
    }
}

static void start_wave(int w)
{
    wave_num = w;
    int count = 5 + w * 2;
    if (count > 22) count = 22;
    wave_spawn_left = count;
    wave_spawn_cd   = 0;
    wave_pause      = 0;
}

static void reset_game(void)
{
    ship_x = VGA_WIDTH / 2 - SHIP_W / 2;
    score = 0; lives = 3; frame = 0;
    fire_cd = 0; invuln = 0;
    pup_weapon = 0; pup_weapon_timer = 0; pup_shield = 0;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
    for (int i = 0; i < MAX_EXPL;   i++) expls[i].timer    = 0;
    for (int i = 0; i < MAX_PUPS;   i++) pups[i].active    = 0;
    init_stars();
    start_wave(1);
    vga_fill(VGA_BLACK);

    /* Shadow state: skip all erases on first rendered frame */
    for (int i = 0; i < MAX_STARS; i++) {
        shad_stars[i].x = stars[i].x;
        shad_stars[i].y = 255; /* off-screen sentinel */
    }
    for (int i = 0; i < MAX_BULLETS; i++) shad_bul[i].active  = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) shad_enm[i].active  = 0;
    for (int i = 0; i < MAX_EXPL;   i++) shad_expl[i].active = 0;
    for (int i = 0; i < MAX_PUPS;   i++) shad_pup[i].active  = 0;
    shad_ship_x    = ship_x;
    shad_wave_pause = 0;
}

/* Returns 1 if an enemy was placed, 0 if all slots are full. */
static int spawn_enemy(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) continue;

        enemies[i].active = 1;
        enemies[i].y      = GAME_TOP - ENM_H;

        int r = rng(100);
        if (wave_num >= 3 && r < 20) {
            /* Type 2: diagonal diver, targets player at spawn time */
            enemies[i].type = 2;
            enemies[i].hp   = 2;
            enemies[i].x    = rng(VGA_WIDTH - ENM_W);
            int dx = ship_x + SHIP_W / 2 - (enemies[i].x + ENM_W / 2);
            enemies[i].vx   = (signed char)((dx >= 0) ? 2 : -2);
        } else if (wave_num >= 2 && r < 55) {
            /* Type 1: zigzag bouncing off walls */
            enemies[i].type = 1;
            enemies[i].hp   = 1;
            enemies[i].x    = rng(VGA_WIDTH - ENM_W);
            enemies[i].vx   = (signed char)((rng(2) == 0) ? 1 : -1);
        } else {
            /* Type 0: straight drop */
            enemies[i].type = 0;
            enemies[i].hp   = 1;
            enemies[i].x    = rng(VGA_WIDTH - ENM_W);
            enemies[i].vx   = 0;
        }
        return 1;
    }
    return 0;
}

/* ═══════════════ UPDATE HELPERS ═══════════════ */

static void add_expl(int cx, int cy)
{
    for (int j = 0; j < MAX_EXPL; j++) {
        if (!expls[j].timer) {
            expls[j].x = cx; expls[j].y = cy; expls[j].timer = 8;
            return;
        }
    }
}

static void fire_bullet(int x, int y)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = 1;
            bullets[i].x = x;
            bullets[i].y = y;
            return;
        }
    }
}

/* ═══════════════ UPDATE ═══════════════ */

static void update(void)
{
    /* ── Player movement ── */
    unsigned int btn = (GPIO_LOW >> 8) & 0x03;
    if ((btn & 1) && ship_x > 1)                       ship_x -= 2;
    if ((btn & 2) && ship_x < VGA_WIDTH - SHIP_W - 1)  ship_x += 2;

    /* ── Auto-fire ── */
    if (fire_cd > 0) fire_cd--;
    int fire_rate = (pup_weapon == PUP_RAPID + 1) ? 3 : 6;
    if (fire_cd == 0) {
        int bx = ship_x + SHIP_W / 2;
        int by = SHIP_Y - 2;
        if (pup_weapon == PUP_MULTI + 1) {
            fire_bullet(bx - 2, by);
            fire_bullet(bx,     by);
            fire_bullet(bx + 2, by);
            fire_cd = fire_rate + 2;
        } else {
            fire_bullet(bx, by);
            fire_cd = fire_rate;
        }
    }

    /* ── Bullet movement ── */
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y -= 3;
        if (bullets[i].y < GAME_TOP) bullets[i].active = 0;
    }

    /* ── Stars ── */
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].y += stars[i].spd;
        if (stars[i].y >= VGA_HEIGHT) {
            stars[i].y = (unsigned char)GAME_TOP;
            stars[i].x = (short)rng(VGA_WIDTH);
        }
    }

    /* ── Power-up weapon timer ── */
    if (pup_weapon_timer > 0 && --pup_weapon_timer == 0)
        pup_weapon = 0;

    /* ── Power-up movement and collection ── */
    for (int i = 0; i < MAX_PUPS; i++) {
        if (!pups[i].active) continue;
        pups[i].y += 1;
        if ((int)pups[i].y >= VGA_HEIGHT) { pups[i].active = 0; continue; }

        /* AABB pickup check */
        if ((int)pups[i].x + 3 > ship_x &&
            (int)pups[i].x     < ship_x + SHIP_W &&
            (int)pups[i].y + 3 > SHIP_Y &&
            (int)pups[i].y     < SHIP_Y + SHIP_H) {
            pups[i].active = 0;
            switch (pups[i].type) {
                case PUP_MULTI:
                    pup_weapon = PUP_MULTI + 1;
                    pup_weapon_timer = 300;
                    break;
                case PUP_RAPID:
                    pup_weapon = PUP_RAPID + 1;
                    pup_weapon_timer = 300;
                    break;
                case PUP_SHIELD:
                    pup_shield = 1;
                    break;
            }
        }
    }

    /* ── Wave / spawn ── */
    if (wave_pause > 0) {
        if (--wave_pause == 0)
            start_wave(wave_num + 1);
    } else {
        /* Spawn next enemy when cooldown expires */
        if (wave_spawn_left > 0) {
            if (wave_spawn_cd > 0) {
                wave_spawn_cd--;
            } else if (spawn_enemy()) {
                wave_spawn_left--;
                int cd = 40 - wave_num * 3;
                if (cd < 8) cd = 8;
                wave_spawn_cd = cd;
            }
        }

        /* Wave complete when all spawned and all dead */
        if (wave_spawn_left == 0) {
            int any = 0;
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (enemies[i].active) { any = 1; break; }
            if (!any) {
                score += wave_num * 50;   /* wave clear bonus */
                wave_pause = WAVE_PAUSE;
            }
        }
    }

    /* ── Enemy movement + player collision ── */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        /* Vertical speed: base + type + wave boost */
        int spd = 1 + (int)enemies[i].type;
        int wb  = wave_num / 3;
        if (wb > 2) wb = 2;
        enemies[i].y += spd + wb;

        /* Horizontal movement by type */
        if (enemies[i].type == 1) {
            /* Zigzag: bounce off screen edges */
            enemies[i].x += enemies[i].vx;
            if (enemies[i].x <= 0)               { enemies[i].x = 0;              enemies[i].vx =  1; }
            if (enemies[i].x >= VGA_WIDTH - ENM_W) { enemies[i].x = VGA_WIDTH - ENM_W; enemies[i].vx = -1; }
        } else if (enemies[i].type == 2) {
            /* Diver: fixed diagonal, clamp at edges */
            enemies[i].x += enemies[i].vx;
            if (enemies[i].x < 0)               enemies[i].x = 0;
            if (enemies[i].x > VGA_WIDTH - ENM_W) enemies[i].x = VGA_WIDTH - ENM_W;
        }

        if (enemies[i].y > VGA_HEIGHT) { enemies[i].active = 0; continue; }

        /* Player hit */
        if (invuln == 0 &&
            enemies[i].y + ENM_H > SHIP_Y     && enemies[i].y < SHIP_Y + SHIP_H &&
            enemies[i].x + ENM_W > ship_x     && enemies[i].x < ship_x + SHIP_W) {
            add_expl(enemies[i].x + ENM_W / 2, enemies[i].y + ENM_H / 2);
            enemies[i].active = 0;
            if (pup_shield) {
                pup_shield = 0;
                invuln = 30;                /* brief grace after shield breaks */
            } else {
                lives--;
                invuln = 60;
                if (lives <= 0) {
                    if (score > hi_score) hi_score = score;
                    vga_fill(VGA_RED);
                    uart_puts("GAME OVER  Score: ");
                    uart_putint(score);
                    uart_puts("  Hi: ");
                    uart_putint(hi_score);
                    uart_puts("  Wave: ");
                    uart_putint(wave_num);
                    uart_puts("\r\n");
                    for (volatile int d = 0; d < 4000000; d++);
                    reset_game();
                    return;
                }
            }
        }
    }

    /* ── Bullet-enemy collision ── */
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!enemies[e].active) continue;
            if (bullets[b].x >= enemies[e].x &&
                bullets[b].x <  enemies[e].x + ENM_W &&
                bullets[b].y >= enemies[e].y &&
                bullets[b].y <  enemies[e].y + ENM_H) {
                bullets[b].active = 0;
                if (--enemies[e].hp <= 0) {
                    add_expl(enemies[e].x + ENM_W / 2, enemies[e].y + ENM_H / 2);
                    score += (enemies[e].type + 1) * 10;
                    /* Power-up drop */
                    if (rng(100) < PUP_DROP_PCT) {
                        for (int j = 0; j < MAX_PUPS; j++) {
                            if (!pups[j].active) {
                                pups[j].active = 1;
                                pups[j].x = (short)(enemies[e].x + ENM_W / 2);
                                pups[j].y = (short)(enemies[e].y + ENM_H / 2);
                                pups[j].type = (unsigned char)rng(3);
                                break;
                            }
                        }
                    }
                    enemies[e].active = 0;
                }
                break;
            }
        }
    }

    /* ── Explosion decay ── */
    for (int i = 0; i < MAX_EXPL; i++)
        if (expls[i].timer > 0) expls[i].timer--;

    if (invuln > 0) invuln--;

    /* GPIO: lives on bits[2:0], score/10 on bits[7:3] */
    GPIO_LOW = (unsigned int)((lives & 7) | (((score / 10) & 0x1F) << 3));
    frame++;
}

/* ═══════════════ RENDER ═══════════════ */

static void render_hud(int fps)
{
    /* Clear HUD strip */
    VGA_FB_ADDR = 0;
    for (int i = 0; i < GAME_TOP * VGA_WIDTH; i++) VGA_FB_DATA = VGA_BLACK;

    /* Score star icon */
    vga_set_pixel(2, 2, VGA_YELLOW);
    vga_set_pixel(1, 3, VGA_YELLOW);
    vga_set_pixel(2, 3, VGA_YELLOW);
    vga_set_pixel(3, 3, VGA_YELLOW);
    vga_set_pixel(2, 4, VGA_YELLOW);

    /* Score (2x) and hi-score (1x below) */
    draw_num_2x(6, 1, score, VGA_WHITE);
    if (hi_score > 0) draw_num(6, 8, hi_score, VGA_DARK_GRAY);

    /* Wave number — top-centre */
    {
        int wx = VGA_WIDTH / 2 - 2;
        if (wave_num >= 10) wx -= 2;
        draw_num(wx, 2, wave_num, VGA_RGB(3, 3, 1));
    }

    /* FPS — top-right */
    draw_num(VGA_WIDTH - 12, 2, fps, VGA_GREEN);

    /* Active power-up indicator (below FPS) */
    if (pup_weapon == PUP_MULTI + 1)
        vga_fill_rect(VGA_WIDTH - 9, 8, 3, 3, VGA_CYAN);
    else if (pup_weapon == PUP_RAPID + 1)
        vga_fill_rect(VGA_WIDTH - 9, 8, 3, 3, VGA_GREEN);
    if (pup_shield)
        vga_set_pixel(VGA_WIDTH - 4, 9, VGA_YELLOW);

    /* Lives as small ships */
    for (int i = 0; i < lives && i < 5; i++) {
        vga_set_pixel(VGA_WIDTH - 14 + i * 5, 9,  VGA_CYAN);
        vga_set_pixel(VGA_WIDTH - 15 + i * 5, 10, VGA_CYAN);
        vga_set_pixel(VGA_WIDTH - 14 + i * 5, 10, VGA_CYAN);
        vga_set_pixel(VGA_WIDTH - 13 + i * 5, 10, VGA_CYAN);
    }

    /* Separator line */
    hline(0, GAME_TOP - 1, VGA_WIDTH, VGA_RGB(1, 1, 0));
}

static void render_game(void)
{
    /* ═══ PHASE 1: Erase all objects at previous-frame positions ═══ */

    /* Stars */
    for (int i = 0; i < MAX_STARS; i++) {
        int sx = (int)shad_stars[i].x, sy = (int)shad_stars[i].y;
        if ((unsigned)sy < (unsigned)VGA_HEIGHT && sy >= GAME_TOP &&
            (unsigned)sx < (unsigned)VGA_WIDTH)
            vga_set_pixel(sx, sy, VGA_BLACK);
    }

    /* Power-ups (3×3 bounding box) */
    for (int i = 0; i < MAX_PUPS; i++) {
        if (!shad_pup[i].active) continue;
        erase_rect((int)shad_pup[i].x, (int)shad_pup[i].y, 3, 3);
    }

    /* Bullets (1×4) */
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!shad_bul[i].active) continue;
        erase_rect(shad_bul[i].x, shad_bul[i].y, 1, 4);
    }

    /* Enemies (ENM_W × ENM_H) */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!shad_enm[i].active) continue;
        erase_rect(shad_enm[i].x, shad_enm[i].y, ENM_W, ENM_H);
    }

    /* Explosions (max radius 8 → 19×19 bounding box) */
    for (int i = 0; i < MAX_EXPL; i++) {
        if (!shad_expl[i].active) continue;
        erase_rect(shad_expl[i].x - 9, shad_expl[i].y - 9, 19, 19);
    }

    /* Ship: SHIP_W+2 wide (shield ring ±1), SHIP_H+3 tall (SHIP_Y−1 to SHIP_Y+SHIP_H+1) */
    erase_rect(shad_ship_x - 1, SHIP_Y - 1, SHIP_W + 2, SHIP_H + 3);

    /* Wave announcement */
    if (shad_wave_pause > 0) {
        int cy = VGA_HEIGHT / 2 - 5;
        erase_rect(24, cy - 3, VGA_WIDTH - 48, 18);
    }

    /* ═══ PHASE 2: Draw all objects at current positions ═══ */

    /* Stars */
    for (int i = 0; i < MAX_STARS; i++) {
        unsigned char c;
        if      (stars[i].spd == 3) c = (frame & 1) ? VGA_WHITE : VGA_LIGHT_GRAY;
        else if (stars[i].spd == 2) c = VGA_LIGHT_GRAY;
        else                        c = VGA_DARK_GRAY;
        int sx = (int)stars[i].x, sy = (int)stars[i].y;
        if ((unsigned)sx < (unsigned)VGA_WIDTH &&
            (unsigned)sy < (unsigned)VGA_HEIGHT && sy >= GAME_TOP)
            vga_set_pixel(sx, sy, c);
        shad_stars[i].x = (short)sx;
        shad_stars[i].y = (unsigned char)sy;
    }

    /* Power-up pickups (blinking diamond) */
    for (int i = 0; i < MAX_PUPS; i++) {
        if (!pups[i].active) { shad_pup[i].active = 0; continue; }
        int px = (int)pups[i].x, py = (int)pups[i].y;
        if ((unsigned)py >= (unsigned)VGA_HEIGHT) { shad_pup[i].active = 0; continue; }
        unsigned char col;
        switch (pups[i].type) {
            case PUP_MULTI:  col = VGA_CYAN;   break;
            case PUP_RAPID:  col = VGA_GREEN;  break;
            default:         col = VGA_YELLOW; break;
        }
        vga_set_pixel(px + 1, py, col);
        if (frame & 4) {
            vga_set_pixel(px,     py + 1, col);
            vga_set_pixel(px + 2, py + 1, col);
        }
        vga_set_pixel(px + 1, py + 1, col);
        vga_set_pixel(px + 1, py + 2, col);
        shad_pup[i].x = (short)px; shad_pup[i].y = (short)py; shad_pup[i].active = 1;
    }

    /* Bullets */
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) { shad_bul[i].active = 0; continue; }
        for (int dy = 0; dy < 4; dy++) {
            int yy = bullets[i].y + dy;
            if ((unsigned)yy < (unsigned)VGA_HEIGHT && yy >= GAME_TOP)
                vga_set_pixel(bullets[i].x, yy, dy < 2 ? VGA_WHITE : VGA_YELLOW);
        }
        shad_bul[i].x = bullets[i].x; shad_bul[i].y = bullets[i].y; shad_bul[i].active = 1;
    }

    /* Enemies */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) { shad_enm[i].active = 0; continue; }
        unsigned char ec;
        const unsigned char *sp;
        switch (enemies[i].type) {
            case 1:  ec = VGA_MAGENTA;    sp = enm2_spr; break;
            case 2:  ec = VGA_RED;        sp = enm3_spr; break;
            default: ec = VGA_RGB(7,2,0); sp = enm1_spr; break;
        }
        draw_spr(enemies[i].x, enemies[i].y, sp, ENM_W, ENM_H, ec);
        shad_enm[i].x = enemies[i].x; shad_enm[i].y = enemies[i].y; shad_enm[i].active = 1;
    }

    /* Explosions */
    for (int i = 0; i < MAX_EXPL; i++) {
        if (expls[i].timer > 0) {
            draw_expl(expls[i].x, expls[i].y, 8 - expls[i].timer);
            shad_expl[i].x = expls[i].x; shad_expl[i].y = expls[i].y;
            shad_expl[i].active = 1;
        } else {
            shad_expl[i].active = 0;
        }
    }

    /* Player ship */
    {
        int flicker = invuln > 0 && (frame & 8);
        unsigned char ship_col = flicker ? VGA_RGB(0, 2, 1) : VGA_CYAN;
        unsigned char ckpt_col = flicker ? VGA_RGB(2, 2, 1) : VGA_WHITE;

        if (pup_shield) {
            vga_set_pixel(ship_x - 1,      SHIP_Y + 2,      VGA_YELLOW);
            vga_set_pixel(ship_x + SHIP_W, SHIP_Y + 2,      VGA_YELLOW);
            vga_set_pixel(ship_x + 3,      SHIP_Y - 1,      VGA_YELLOW);
            vga_set_pixel(ship_x + 3,      SHIP_Y + SHIP_H, VGA_YELLOW);
        }

        int ey = SHIP_Y + SHIP_H;
        if ((unsigned)ey < (unsigned)VGA_HEIGHT) {
            unsigned char f1 = (frame & 2) ? VGA_RGB(7,5,0) : VGA_RGB(7,3,0);
            unsigned char f2 = (frame & 2) ? VGA_RGB(7,2,0) : VGA_RGB(6,1,0);
            vga_set_pixel(ship_x + 2, ey, f2);
            vga_set_pixel(ship_x + 3, ey, f1);
            vga_set_pixel(ship_x + 4, ey, f2);
        }
        if ((unsigned)(ey + 1) < (unsigned)VGA_HEIGHT) {
            unsigned char ft = (frame & 4) ? VGA_RGB(5,1,0) : VGA_RGB(3,0,0);
            vga_set_pixel(ship_x + 3, ey + 1, ft);
        }

        draw_spr(ship_x, SHIP_Y, ship_spr, SHIP_W, SHIP_H, ship_col);
        vga_set_pixel(ship_x + 3, SHIP_Y,     ckpt_col);
        vga_set_pixel(ship_x + 3, SHIP_Y + 1, ckpt_col);
        shad_ship_x = ship_x;
    }

    /* Between-wave announcement */
    if (wave_pause > 0 && (frame & 4)) {
        int cy = VGA_HEIGHT / 2 - 5;
        int bx = 24;
        vga_fill_rect(bx, cy - 3, VGA_WIDTH - bx * 2, 18, VGA_RGB(0, 0, 2));
        int lx = VGA_WIDTH / 2 - 10;
        draw_letter(lx,      cy, 0, VGA_YELLOW);  /* W */
        draw_letter(lx + 4,  cy, 1, VGA_YELLOW);  /* A */
        draw_letter(lx + 8,  cy, 2, VGA_YELLOW);  /* V */
        draw_letter(lx + 12, cy, 3, VGA_YELLOW);  /* E */
        draw_num_2x(lx + 18, cy - 1, wave_num + 1, VGA_WHITE);
    }
    shad_wave_pause = wave_pause;
}

/* ═══════════════ MAIN ═══════════════ */

static void wait_vblank_start(void)
{
    while  (VGA_FB_STATUS & 0x01);
    while (!(VGA_FB_STATUS & 0x01));
}

int main(void)
{
    GPIO_DIR_LOW = 0xFF;
    uart_puts("Star Assault v2 - Z-Core RV32IM\r\n");
    uart_puts("Waves | MULTI/RAPID/SHIELD power-ups | 3 enemy types\r\n");

    reset_game();

    unsigned int fps_tick = rdcycle();
    int fps = 60, fps_cnt = 0;

    while (1) {
        wait_vblank_start();

        fps_cnt++;
        unsigned int now = rdcycle();
        if (now - fps_tick >= 50000000u) {
            fps = fps_cnt;
            fps_cnt = 0;
            fps_tick = now;
        }

        update();
        render_hud(fps);
        render_game();
    }

    return 0;
}
