/*
 *  STAR ASSAULT — Z-Core RISC-V Space Shooter
 *
 *  160x120 VGA framebuffer, 4x upscaled to 640x480
 *  GPIO bit 8 = left, bit 9 = right, auto-fire
 *  FPS counter top-right, score top-left
 *
 *  Rendering strategy: draw HUD during vblank (tear-free),
 *  then clear + draw game area top-to-bottom ahead of scan.
 */

#include "libs/uart.h"
#include "libs/vga.h"

#define GPIO_LOW     (*((volatile unsigned int *)0x04001000))
#define GPIO_DIR_LOW (*((volatile unsigned int *)0x04001008))

/* Entity limits (tuned for 12 KB RAM) */
#define MAX_BULLETS   4
#define MAX_ENEMIES   6
#define MAX_STARS    20
#define MAX_EXPL      4

#define SHIP_W  7
#define SHIP_H  6
#define ENM_W   7
#define ENM_H   5

#define HUD_H   14
#define GAME_TOP HUD_H
#define SHIP_Y  (VGA_HEIGHT - SHIP_H - 4)

/* ───── RNG ───── */
static unsigned int seed = 54321;
static int rng(int max) {
    seed = seed * 1103515245 + 12345;
    return (int)((seed >> 16) % (unsigned int)max);
}

/* ───── Cycle counter for FPS ───── */
static inline unsigned int rdcycle(void) {
    unsigned int v;
    asm volatile("csrr %0, mcycle" : "=r"(v));
    return v;
}

/* ═══════════════════════════════════
          SPRITE DATA (7 px wide)
   ═══════════════════════════════════ */

static const unsigned char ship_spr[SHIP_H] = {
    0x08, /*  ...#...  */
    0x1C, /*  ..###..  */
    0x3E, /*  .#####.  */
    0x7F, /*  ####### */
    0x5D, /*  #.###.#  */
    0x14  /*  ..#.#..  */
};

static const unsigned char enm1_spr[ENM_H] = {
    0x2A, /*  .#.#.#.  */
    0x7F, /*  ####### */
    0x3E, /*  .#####.  */
    0x55, /*  #.#.#.#  */
    0x22  /*  .#...#.  */
};

static const unsigned char enm2_spr[ENM_H] = {
    0x08, /*  ...#...  */
    0x3E, /*  .#####.  */
    0x7F, /*  ####### */
    0x3E, /*  .#####.  */
    0x08  /*  ...#...  */
};

/* 3x5 digit font */
static const unsigned char font3x5[10][5] = {
    {7,5,5,5,7},{2,2,2,2,2},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}
};

/* ═══════════════════════════════════
              GAME STATE
   ═══════════════════════════════════ */

static struct { int x, y; unsigned char active; }            bullets[MAX_BULLETS];
static struct { int x, y; unsigned char active, type, hp; }  enemies[MAX_ENEMIES];
static struct { int x, y; unsigned char timer; }             expls[MAX_EXPL];
static struct { unsigned char x, y, spd; }                   stars[MAX_STARS];

static int ship_x, score, hi_score, lives, frame;
static int fire_cd, spawn_cd, invuln;

/* ═══════════════════════════════════
            DRAWING HELPERS
   ═══════════════════════════════════ */

static void draw_spr(int sx, int sy, const unsigned char *d,
                     int w, int h, unsigned char col) {
    for (int r = 0; r < h; r++) {
        int yy = sy + r;
        if ((unsigned)yy >= (unsigned)VGA_HEIGHT) continue;
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

static void draw_num(int x, int y, int num, unsigned char col) {
    char buf[6];
    int n = 0;
    if (num <= 0) { buf[n++] = 0; }
    else { while (num > 0 && n < 6) { buf[n++] = (char)(num % 10); num /= 10; } }
    for (int i = n - 1; i >= 0; i--) {
        const unsigned char *g = font3x5[(int)buf[i]];
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++)
                if (g[r] & (4 >> c))
                    vga_set_pixel(x + c, y + r, col);
        x += 4;
    }
}

static void draw_num_2x(int x, int y, int num, unsigned char col) {
    char buf[6];
    int n = 0;
    if (num <= 0) { buf[n++] = 0; }
    else { while (num > 0 && n < 6) { buf[n++] = (char)(num % 10); num /= 10; } }
    for (int i = n - 1; i >= 0; i--) {
        const unsigned char *g = font3x5[(int)buf[i]];
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++)
                if (g[r] & (4 >> c))
                    vga_fill_rect(x + c * 2, y + r * 2, 2, 2, col);
        x += 8;
    }
}

static void draw_expl(int cx, int cy, int t) {
    static const unsigned char cols[] = { 0xFF, 0xFC, 0xEC, 0xE0 };
    unsigned char c = cols[t < 4 ? t : 3];
    int s = t + 1;
    for (int i = -s; i <= s; i++) {
        int px, py;
        px = cx + i; py = cy;
        if ((unsigned)px < (unsigned)VGA_WIDTH && (unsigned)py < (unsigned)VGA_HEIGHT)
            vga_set_pixel(px, py, c);
        px = cx; py = cy + i;
        if ((unsigned)px < (unsigned)VGA_WIDTH && (unsigned)py < (unsigned)VGA_HEIGHT)
            vga_set_pixel(px, py, c);
    }
    if (s > 1) {
        int d = s - 1;
        for (int k = 0; k < 4; k++) {
            int dx = (k & 1) ? d : -d;
            int dy = (k & 2) ? d : -d;
            int px = cx + dx, py = cy + dy;
            if ((unsigned)px < (unsigned)VGA_WIDTH && (unsigned)py < (unsigned)VGA_HEIGHT)
                vga_set_pixel(px, py, c);
        }
    }
}

static void hline(int x, int y, int w, unsigned char c) {
    if ((unsigned)y >= (unsigned)VGA_HEIGHT) return;
    VGA_FB_ADDR = (unsigned)(y * VGA_WIDTH + x);
    for (int i = 0; i < w && x + i < VGA_WIDTH; i++)
        VGA_FB_DATA = c;
}

static void clear_game_area(void) {
    VGA_FB_ADDR = (unsigned)(GAME_TOP * VGA_WIDTH);
    for (int i = GAME_TOP * VGA_WIDTH; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_FB_DATA = VGA_BLACK;
}

/* ═══════════════════════════════════
           INIT / RESET
   ═══════════════════════════════════ */

static void init_stars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = (unsigned char)rng(VGA_WIDTH);
        stars[i].y = (unsigned char)(GAME_TOP + rng(VGA_HEIGHT - GAME_TOP));
        stars[i].spd = (i & 1) ? 2 : 1;
    }
}

static void reset_game(void) {
    ship_x = VGA_WIDTH / 2 - SHIP_W / 2;
    score = 0; lives = 3; frame = 0;
    fire_cd = 0; spawn_cd = 60; invuln = 0;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
    for (int i = 0; i < MAX_EXPL; i++)    expls[i].timer = 0;
    init_stars();
    vga_fill(VGA_BLACK);
}

/* ═══════════════════════════════════
             UPDATE LOGIC
   ═══════════════════════════════════ */

static void update(void) {
    unsigned int btn = (GPIO_LOW >> 8) & 0x03;

    if ((btn & 1) && ship_x > 1)                       ship_x -= 2;
    if ((btn & 2) && ship_x < VGA_WIDTH - SHIP_W - 1)  ship_x += 2;

    if (fire_cd > 0) fire_cd--;
    if (fire_cd == 0) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].active = 1;
                bullets[i].x = ship_x + SHIP_W / 2;
                bullets[i].y = SHIP_Y - 2;
                fire_cd = 6;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y -= 3;
        if (bullets[i].y < GAME_TOP) bullets[i].active = 0;
    }

    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].y += stars[i].spd;
        if (stars[i].y >= VGA_HEIGHT) {
            stars[i].y = (unsigned char)GAME_TOP;
            stars[i].x = (unsigned char)rng(VGA_WIDTH);
        }
    }

    if (--spawn_cd <= 0) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) continue;
            enemies[i].active = 1;
            enemies[i].x = rng(VGA_WIDTH - ENM_W);
            enemies[i].y = GAME_TOP - (int)ENM_H;
            int diff = score / 50;
            enemies[i].type = (unsigned char)(rng(100) < diff * 12 ? 1 : 0);
            enemies[i].hp   = (unsigned char)(enemies[i].type + 1);
            spawn_cd = 45 - diff * 3;
            if (spawn_cd < 12) spawn_cd = 12;
            break;
        }
        if (spawn_cd <= 0) spawn_cd = 30;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemies[i].y += 1 + enemies[i].type;
        if (enemies[i].y > VGA_HEIGHT) { enemies[i].active = 0; continue; }

        if (invuln > 0) continue;
        if (enemies[i].y + ENM_H > SHIP_Y &&
            enemies[i].y < SHIP_Y + SHIP_H &&
            enemies[i].x + ENM_W > ship_x &&
            enemies[i].x < ship_x + SHIP_W) {
            for (int j = 0; j < MAX_EXPL; j++) {
                if (!expls[j].timer) {
                    expls[j].x = enemies[i].x + ENM_W / 2;
                    expls[j].y = enemies[i].y + ENM_H / 2;
                    expls[j].timer = 8;
                    break;
                }
            }
            enemies[i].active = 0;
            lives--;
            invuln = 90;
            if (lives <= 0) {
                if (score > hi_score) hi_score = score;
                vga_fill(VGA_RED);
                uart_puts("GAME OVER  Score: ");
                uart_putint(score);
                uart_puts("  Hi: ");
                uart_putint(hi_score);
                uart_puts("\r\n");
                for (volatile int d = 0; d < 4000000; d++);
                reset_game();
                return;
            }
        }
    }

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
                    for (int j = 0; j < MAX_EXPL; j++) {
                        if (!expls[j].timer) {
                            expls[j].x = enemies[e].x + ENM_W / 2;
                            expls[j].y = enemies[e].y + ENM_H / 2;
                            expls[j].timer = 8;
                            break;
                        }
                    }
                    enemies[e].active = 0;
                    score += (enemies[e].type + 1) * 10;
                }
                break;
            }
        }
    }

    for (int i = 0; i < MAX_EXPL; i++)
        if (expls[i].timer > 0) expls[i].timer--;

    if (invuln > 0) invuln--;

    GPIO_LOW = (unsigned int)((lives & 0x07) | (((score / 10) & 0x1F) << 3));

    frame++;
}

/* ═══════════════════════════════════
               RENDER
   ═══════════════════════════════════ */

static void render_hud(int fps) {
    /*
     * Drawn FIRST after vsync — during vblank the monitor
     * isn't scanning, so these writes are tear-free.
     */

    /* Clear HUD area */
    VGA_FB_ADDR = 0;
    for (int i = 0; i < GAME_TOP * VGA_WIDTH; i++)
        VGA_FB_DATA = VGA_BLACK;

    /* Score icon (small 3x3 star in yellow) */
    vga_set_pixel(2, 2, VGA_YELLOW);
    vga_set_pixel(1, 3, VGA_YELLOW);
    vga_set_pixel(2, 3, VGA_YELLOW);
    vga_set_pixel(3, 3, VGA_YELLOW);
    vga_set_pixel(2, 4, VGA_YELLOW);

    /* Score number (2x size) */
    draw_num_2x(6, 1, score, VGA_WHITE);

    /* High score (small, right of score) */
    if (hi_score > 0) {
        draw_num(6, 8, hi_score, VGA_DARK_GRAY);
    }

    /* FPS (normal size, top-right) */
    draw_num(VGA_WIDTH - 12, 2, fps, VGA_GREEN);

    /* Lives as small ship outlines */
    for (int i = 0; i < lives && i < 5; i++) {
        vga_set_pixel(VGA_WIDTH - 14 + i * 5, 9, VGA_CYAN);
        vga_set_pixel(VGA_WIDTH - 15 + i * 5, 10, VGA_CYAN);
        vga_set_pixel(VGA_WIDTH - 14 + i * 5, 10, VGA_CYAN);
        vga_set_pixel(VGA_WIDTH - 13 + i * 5, 10, VGA_CYAN);
    }

    /* Separator line */
    hline(0, GAME_TOP - 1, VGA_WIDTH, VGA_RGB(1, 1, 0));
}

static void render_game(void) {
    /* Clear game area only (rows GAME_TOP..119) */
    clear_game_area();

    /* ── Stars (parallax) ── */
    for (int i = 0; i < MAX_STARS; i++) {
        unsigned char c = (stars[i].spd == 1) ? VGA_DARK_GRAY : VGA_LIGHT_GRAY;
        if (stars[i].spd == 2 && (frame & 1))
            c = VGA_WHITE;
        vga_set_pixel(stars[i].x, stars[i].y, c);
    }

    /* ── Bullets ── */
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        for (int dy = 0; dy < 4; dy++) {
            int yy = bullets[i].y + dy;
            if ((unsigned)yy >= (unsigned)GAME_TOP && (unsigned)yy < (unsigned)VGA_HEIGHT)
                vga_set_pixel(bullets[i].x, yy,
                    dy < 2 ? VGA_WHITE : VGA_YELLOW);
        }
    }

    /* ── Enemies ── */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        unsigned char ec;
        const unsigned char *sp;
        if (enemies[i].type) {
            ec = VGA_MAGENTA; sp = enm2_spr;
        } else {
            ec = VGA_RED; sp = enm1_spr;
        }
        draw_spr(enemies[i].x, enemies[i].y, sp, ENM_W, ENM_H, ec);
    }

    /* ── Explosions ── */
    for (int i = 0; i < MAX_EXPL; i++)
        if (expls[i].timer > 0)
            draw_expl(expls[i].x, expls[i].y, 8 - expls[i].timer);

    /* ── Ship (always drawn — color dims during invuln) ── */
    {
        unsigned char ship_col = VGA_CYAN;
        unsigned char cockpit_col = VGA_WHITE;
        if (invuln > 0 && (frame & 8))  {
            ship_col = VGA_RGB(0, 2, 1);
            cockpit_col = VGA_RGB(2, 2, 1);
        }

        /* Engine exhaust (always visible, colour varies) */
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
        vga_set_pixel(ship_x + 3, SHIP_Y,     cockpit_col);
        vga_set_pixel(ship_x + 3, SHIP_Y + 1, cockpit_col);
    }
}

/* ═══════════════════════════════════
               MAIN
   ═══════════════════════════════════ */

static void wait_vblank_start(void) {
    while (VGA_FB_STATUS & 0x01)    /* if already in vblank, wait it out */
        ;
    while (!(VGA_FB_STATUS & 0x01)) /* wait for vblank to begin          */
        ;
}

int main(void) {
    GPIO_DIR_LOW = 0xFF;
    uart_puts("Star Assault - Z-Core RV32IM\r\n");

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

        /* HUD first (during vblank — tear-free) then game area */
        render_hud(fps);
        render_game();
    }

    return 0;
}
