/*

Copyright (c) 2025 Pau Díaz Cuesta

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

*/

// ****************************************************
//   Z-Core Physical Memory Attributes (PMA) Checker
//
//   Pure combinational decoder. Given a physical address and an
//   access type, reports the inherent attributes of the region
//   (cacheable, R/W/X permissions, main-memory vs. I/O) and
//   raises access_fault when the requested access type is not
//   supported by the region (or when the address is vacant).
//
//   Single-cycle: outputs are valid the same cycle the inputs
//   are presented. No registers, no clock.
//
//   Region table is platform-specific and lives in
//   z_core_pma_map.vh. Swap headers per build target to retarget
//   the address map (e.g. FPGA + SDRAM) without editing this file.
// ****************************************************

module z_core_pma #(
    parameter ADDR_WIDTH = 32
)(
    input  wire [ADDR_WIDTH-1:0] addr,
    // access_type: 2'b00 = instruction fetch, 2'b01 = load, 2'b10 = store
    input  wire [1:0]            access_type,

    output wire                  cacheable,
    output wire                  readable,
    output wire                  writable,
    output wire                  executable,
    output wire                  is_io,
    output wire                  access_fault
);

`include "rtl/z_core_pma_map.vh"

localparam ACCESS_FETCH = 2'b00;
localparam ACCESS_LOAD  = 2'b01;
localparam ACCESS_STORE = 2'b10;

// Pack the per-region constants from the include into a small
// internal array so the decode loop is parametric over PMA_NUM_REGIONS.
// Using fixed-size arrays sized for the current map; if more regions
// are needed, extend the include and bump PMA_NUM_REGIONS.
wire [31:0] region_base  [0:PMA_NUM_REGIONS-1];
wire [31:0] region_mask  [0:PMA_NUM_REGIONS-1];
wire [4:0]  region_perms [0:PMA_NUM_REGIONS-1];

assign region_base[0]  = PMA_REGION0_BASE;
assign region_mask[0]  = PMA_REGION0_MASK;
assign region_perms[0] = PMA_REGION0_PERMS;
assign region_base[1]  = PMA_REGION1_BASE;
assign region_mask[1]  = PMA_REGION1_MASK;
assign region_perms[1] = PMA_REGION1_PERMS;
assign region_base[2]  = PMA_REGION2_BASE;
assign region_mask[2]  = PMA_REGION2_MASK;
assign region_perms[2] = PMA_REGION2_PERMS;
assign region_base[3]  = PMA_REGION3_BASE;
assign region_mask[3]  = PMA_REGION3_MASK;
assign region_perms[3] = PMA_REGION3_PERMS;
assign region_base[4]  = PMA_REGION4_BASE;
assign region_mask[4]  = PMA_REGION4_MASK;
assign region_perms[4] = PMA_REGION4_PERMS;
assign region_base[5]  = PMA_REGION5_BASE;
assign region_mask[5]  = PMA_REGION5_MASK;
assign region_perms[5] = PMA_REGION5_PERMS;

// Per-region match: address falls inside the region's range.
wire [PMA_NUM_REGIONS-1:0] match;
genvar gi;
generate
    for (gi = 0; gi < PMA_NUM_REGIONS; gi = gi + 1) begin : g_match
        assign match[gi] = ((addr & ~region_mask[gi]) == region_base[gi]);
    end
endgenerate

// Priority selection: last (highest-indexed) matching region wins.
// Higher-indexed regions override lower ones. Vacant addresses produce
// all-zero perms.
reg [4:0] selected_perms;
integer ri;
always @* begin
    selected_perms = 5'b0;
    for (ri = 0; ri < PMA_NUM_REGIONS; ri = ri + 1) begin
        if (match[ri])
            selected_perms = region_perms[ri];
    end
end

assign cacheable  = selected_perms[0];
assign readable   = selected_perms[1];
assign writable   = selected_perms[2];
assign executable = selected_perms[3];
assign is_io      = selected_perms[4];

// Access fault: requested access type is not supported by this region
// (or the region is vacant -> all perms zero -> always faults).
reg fault;
always @* begin
    case (access_type)
        ACCESS_FETCH: fault = ~executable;
        ACCESS_LOAD:  fault = ~readable;
        ACCESS_STORE: fault = ~writable;
        default:      fault = 1'b1;
    endcase
end

assign access_fault = fault;

endmodule
