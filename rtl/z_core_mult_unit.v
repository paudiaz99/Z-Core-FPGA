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

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// ============================================================================
// Multiplier Unit Wrapper
// 32-bit x 32-bit = 64-bit multiplication
//
// Change the include and instantiation below to switch implementations:
// - z_core_mult_tree  : Educational tree-based (62 adders)
// - z_core_mult_synth : Synthesis-optimized (* operator)
//
// RISC-V M Extension: MUL, MULH, MULHSU, MULHU
// ============================================================================

// === SELECT IMPLEMENTATION HERE ===
// `define USE_TREE_MULTIPLIER  // Uncomment for educational tree version

//`ifdef USE_TREE_MULTIPLIER
  //  `include "rtl/z_core_mult_tree.v"
//`else
  //  `include "rtl/z_core_mult_synth.v"
//`endif


module z_core_mult_unit (
    input [31:0] op1,
    input [31:0] op2,
    input        op1_signed,
    input        op2_signed,
    output [63:0] result
);

`ifdef USE_TREE_MULTIPLIER
    z_core_mult_tree mult_impl (
        .op1(op1),
        .op2(op2),
        .op1_signed(op1_signed),
        .op2_signed(op2_signed),
        .result(result)
    );
`else
    z_core_mult_synth mult_impl (
        .op1(op1),
        .op2(op2),
        .op1_signed(op1_signed),
        .op2_signed(op2_signed),
        .result(result)
    );
`endif

endmodule
