#ifndef VGA_H
#define VGA_H

#define VGA_BASE       0x04003000
#define VGA_FB_ADDR    (*((volatile unsigned int *)(VGA_BASE + 0x00)))
#define VGA_FB_DATA    (*((volatile unsigned int *)(VGA_BASE + 0x04)))
#define VGA_FB_STATUS  (*((volatile unsigned int *)(VGA_BASE + 0x08)))

#define VGA_WIDTH      160
#define VGA_HEIGHT     120

/* 8-bit color: RRRGGGBB */
#define VGA_RGB(r,g,b) ((unsigned char)(((r)<<5)|((g)<<2)|(b)))

#define VGA_BLACK      0x00
#define VGA_WHITE      0xFF
#define VGA_RED        0xE0
#define VGA_GREEN      0x1C
#define VGA_BLUE       0x03
#define VGA_YELLOW     0xFC
#define VGA_CYAN       0x1F
#define VGA_MAGENTA    0xE3
#define VGA_DARK_GRAY  0x49
#define VGA_LIGHT_GRAY 0xB6

static inline void vga_set_pixel(int x, int y, unsigned char color) {
    VGA_FB_ADDR = (unsigned int)(y * VGA_WIDTH + x);
    VGA_FB_DATA = color;
}

static inline void vga_fill(unsigned char color) {
    VGA_FB_ADDR = 0;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_FB_DATA = color;
}

static inline void vga_fill_rect(int x0, int y0, int w, int h, unsigned char color) {
    for (int y = y0; y < y0 + h && y < VGA_HEIGHT; y++) {
        VGA_FB_ADDR = (unsigned int)(y * VGA_WIDTH + x0);
        for (int x = 0; x < w && (x0 + x) < VGA_WIDTH; x++)
            VGA_FB_DATA = color;
    }
}

static inline void vga_wait_vsync(void) {
    while (!(VGA_FB_STATUS & 0x01))
        ;
    while (VGA_FB_STATUS & 0x01)
        ;
}

#endif /* VGA_H */
