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
