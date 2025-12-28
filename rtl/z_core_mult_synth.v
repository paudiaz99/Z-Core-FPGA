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
// Synthesis-Optimized Multiplier
// 32-bit x 32-bit = 64-bit multiplication
//
// Uses Verilog * operator for synthesis tool optimization.
// Allows the tool to use DSP blocks, Booth encoding, or other optimizations.
//
// RISC-V M Extension: MUL, MULH, MULHSU, MULHU
// ============================================================================

module z_core_mult_synth (
    input [31:0] op1,
    input [31:0] op2,
    input        op1_signed,
    input        op2_signed,
    output [63:0] result
);

// ============================================================================
// Sign Handling
// ============================================================================
wire op1_neg = op1_signed & op1[31];
wire op2_neg = op2_signed & op2[31];
wire [31:0] a = op1_neg ? (~op1 + 32'd1) : op1;
wire [31:0] b = op2_neg ? (~op2 + 32'd1) : op2;
wire res_neg = op1_neg ^ op2_neg;

// ============================================================================
// Unsigned Multiplication - Let synthesis tool optimize
// ============================================================================
wire [63:0] prod = a * b;

// ============================================================================
// Sign Correction
// ============================================================================
assign result = res_neg ? (~prod + 64'd1) : prod;

endmodule
