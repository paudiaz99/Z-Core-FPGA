// ****************************************************
//   Z-Core Physical Memory Attributes (PMA) Region Map
//   Default memory map (matches z_core_top_model.v)
// ****************************************************
//
// To target a different platform (e.g. an FPGA build with relocated RAM
// or an additional SDRAM region), copy this file to a sibling header
// (e.g. z_core_pma_map_fpga.vh) and select it in that target's flist.
//
// Each region is described by three constants:
//   PMA_REGION<i>_BASE   : 32-bit base address (must be aligned to mask+1)
//   PMA_REGION<i>_MASK   : 32-bit mask of address bits that vary within
//                          the region; an address `a` matches when
//                          (a & ~MASK) == BASE.
//   PMA_REGION<i>_PERMS  : {is_io, executable, writable, readable, cacheable}
//                          5 bits, one per attribute.
//
// Vacant addresses (no region match) report all-zero perms, which raises
// an access fault on any access.
//
// Permission bit encoding (LSB first):
//   bit 0 : cacheable   (1 = data/I-cache may cache; 0 = bypass)
//   bit 1 : readable    (loads/fetches allowed)
//   bit 2 : writable    (stores allowed)
//   bit 3 : executable  (instruction fetches allowed)
//   bit 4 : is_io       (informational; main memory = 0, MMIO = 1)

`ifndef Z_CORE_PMA_MAP_VH
`define Z_CORE_PMA_MAP_VH

localparam integer PMA_NUM_REGIONS = 6;

// ---- Region 0: RAM (main memory, 64 MB) ----
//   0x0000_0000 - 0x03FF_FFFF
localparam [31:0] PMA_REGION0_BASE  = 32'h0000_0000;
localparam [31:0] PMA_REGION0_MASK  = 32'h03FF_FFFF;
localparam [4:0]  PMA_REGION0_PERMS = 5'b0_1_1_1_1; // !io, X, W, R, !C

// ---- Region 1: UART (MMIO, 4 KB) ----
//   0x0400_0000 - 0x0400_0FFF
localparam [31:0] PMA_REGION1_BASE  = 32'h0400_0000;
localparam [31:0] PMA_REGION1_MASK  = 32'h0000_0FFF;
localparam [4:0]  PMA_REGION1_PERMS = 5'b1_0_1_1_0; // io, !X, W, R, !C

// ---- Region 2: GPIO (MMIO, 4 KB) ----
//   0x0400_1000 - 0x0400_1FFF
localparam [31:0] PMA_REGION2_BASE  = 32'h0400_1000;
localparam [31:0] PMA_REGION2_MASK  = 32'h0000_0FFF;
localparam [4:0]  PMA_REGION2_PERMS = 5'b1_0_1_1_0; // io, !X, W, R, !C

// ---- Region 3: Timer (MMIO, 4 KB) ----
//   0x0400_2000 - 0x0400_2FFF
localparam [31:0] PMA_REGION3_BASE  = 32'h0400_2000;
localparam [31:0] PMA_REGION3_MASK  = 32'h0000_0FFF;
localparam [4:0]  PMA_REGION3_PERMS = 5'b1_0_1_1_0; // io, !X, W, R, !C

// ---- Region 4: VGA (MMIO, 4 KB) ----
//   0x0400_3000 - 0x0400_3FFF
localparam [31:0] PMA_REGION4_BASE  = 32'h0400_3000;
localparam [31:0] PMA_REGION4_MASK  = 32'h0000_0FFF;
localparam [4:0]  PMA_REGION4_PERMS = 5'b1_0_1_1_0; // io, !X, W, R, !C

// ---- Region 5: SDRAM (cacheable, 64 MB) ----
//   0x1000_0000 - 0x13FF_FFFF
localparam [31:0] PMA_REGION5_BASE  = 32'h1000_0000;
localparam [31:0] PMA_REGION5_MASK  = 32'h03FF_FFFF;
localparam [4:0]  PMA_REGION5_PERMS = 5'b0_1_1_1_1; // !io, X, W, R, C

`endif // Z_CORE_PMA_MAP_VH
