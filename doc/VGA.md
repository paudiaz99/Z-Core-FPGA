# VGA Controller

The Z-Core VGA Controller provides a simple interface for video output on the DE10-Lite board. It uses a 160x120 internal framebuffer, which is hardware-upscaled (4x) to a standard 640x480 @ 60 Hz VGA signal.

## Features

- **Resolution**: 160x120 pixels (upscaled to 640x480).
- **Color Depth**: 8-bit color (3-3-2 RGB format).
- **Interface**: AXI-Lite slave.
- **Hardware**: Uses on-chip M9K RAM for the framebuffer.

## Register Map

Base Address: `0x04003000`

| Offset | Name | Type | Description |
|--------|------|------|-------------|
| `0x00` | `FB_ADDR` | R/W | Framebuffer write address (0 to 19199). |
| `0x04` | `FB_DATA` | W | Write pixel color to current address. Auto-increments `FB_ADDR`. |
| `0x08` | `FB_STATUS` | R | Status bits. Bit 0: `in_vblank` (1 if in vertical blanking). |

### Color Format (8-bit RGB 3:3:2)

The controller uses an 8-bit color byte per pixel:

| Bits | Description |
|------|-------------|
| `[7:5]` | Red (3 bits) |
| `[4:2]` | Green (3 bits) |
| `[1:0]` | Blue (2 bits) |

## C API (`vga.h`)

The library `software/libs/vga.h` provides helper functions to interact with the VGA controller.

### Constants

```c
#define VGA_WIDTH      160
#define VGA_HEIGHT     120

/* Predefined colors */
#define VGA_BLACK      0x00
#define VGA_WHITE      0xFF
#define VGA_RED        0xE0
#define VGA_GREEN      0x1C
#define VGA_BLUE       0x03
```

### Functions

#### `vga_set_pixel(int x, int y, unsigned char color)`
Sets a single pixel at coordinates (x, y).
- `x`: 0..159
- `y`: 0..119
- `color`: 8-bit RGB332

#### `vga_fill(unsigned char color)`
Fills the entire screen with a single color.

#### `vga_fill_rect(int x, int y, int w, int h, unsigned char color)`
Fills a rectangular area with a color.

#### `vga_wait_vsync(void)`
Blocks execution until the start of the next vertical blanking period. Useful for flicker-free animations.
