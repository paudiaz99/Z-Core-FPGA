# VGA Controller

The Z-Core VGA Controller provides a simple interface for video output on the DE10-Lite board. It uses a 320×200 internal framebuffer, which is hardware-upscaled 2× to 640×400 and letterboxed inside a standard 640×480 @ 60 Hz VGA signal.

## Features

- **Resolution**: 320×200 pixels (upscaled 2× to 640×400, letterboxed in 640×480).
- **Color Depth**: 8-bit color (RGB 3:3:2 format).
- **Interface**: AXI-Lite slave.
- **Hardware**: Uses on-chip M9K RAM for the framebuffer.

## Register Map

Base Address: `0x0400_3000`

| Offset | Name | Type | Description |
|--------|------|------|-------------|
| `0x00` | `FB_ADDR` | R/W | Framebuffer write address (0 to 63999). |
| `0x04` | `FB_DATA` | W | Write pixel color (RGB332) to current address. Auto-increments `FB_ADDR`. |
| `0x08` | `FB_STATUS` | R | Status bits. Bit 0: `in_vblank` (1 if in vertical blanking period). |

Pixel address formula: `addr = y * 320 + x`, where `x ∈ [0, 319]` and `y ∈ [0, 199]`.

### Color Format (8-bit RGB 3:3:2)

| Bits | Description |
|------|-------------|
| `[7:5]` | Red (3 bits) |
| `[4:2]` | Green (3 bits) |
| `[1:0]` | Blue (2 bits) |

Convenience macro: `VGA_RGB(r, g, b)` where `r ∈ [0, 7]`, `g ∈ [0, 7]`, `b ∈ [0, 3]`.

## C API (`vga.h`)

The header `software/libs/vga.h` is a header-only library providing inline helper functions and color constants.

### Constants

```c
#define VGA_WIDTH      320
#define VGA_HEIGHT     200

/* Predefined colors */
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

/* Pack r[0..7], g[0..7], b[0..3] into an RGB332 byte */
#define VGA_RGB(r, g, b)  ((unsigned char)(((r)<<5)|((g)<<2)|(b)))
```

### Functions

#### `vga_set_pixel(int x, int y, unsigned char color)`
Sets a single pixel at coordinates (x, y).
- `x`: 0..319
- `y`: 0..199
- `color`: 8-bit RGB332 byte

#### `vga_fill(unsigned char color)`
Fills the entire 320×200 framebuffer with a single color.

#### `vga_fill_rect(int x0, int y0, int w, int h, unsigned char color)`
Fills a rectangular region starting at (x0, y0) with the given width, height, and color. Clips automatically at screen boundaries.

#### `vga_wait_vsync(void)`
Blocks execution until the start of the next vertical blanking period (waits for `FB_STATUS` bit 0 to go high, then low). Use before updating the framebuffer to avoid tearing.

## Usage Example

```c
#include "libs/vga.h"
#include "libs/uart.h"

void main(void) {
    uart_set_baud(27);

    // Fill background dark gray
    vga_fill(VGA_DARK_GRAY);

    // Draw a red rectangle
    vga_fill_rect(10, 10, 100, 50, VGA_RED);

    // Draw individual pixels
    for (int x = 0; x < 320; x++)
        vga_set_pixel(x, 100, VGA_RGB(0, x >> 5, 3));  // gradient line

    // Animation loop — update each vertical blank
    while (1) {
        vga_wait_vsync();
        // update framebuffer here
    }
}
```

## Notes

- Writing to `FB_DATA` auto-increments `FB_ADDR`, so sequential pixel writes do not need to update the address register each time.
- For full-screen clears or frame copies, writing all 64,000 pixels sequentially without updating `FB_ADDR` is the fastest path.
- DOOM uses this controller at 320×200 with its own software renderer writing directly to the framebuffer through the VGA registers.
