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
