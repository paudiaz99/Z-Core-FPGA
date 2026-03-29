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

module z_core_alu (
    input [31:0] alu_in1,
    input [31:0] alu_in2,
    input [4:0] alu_inst_type,
    output [31:0] alu_out,
    output reg alu_branch
);

// Instructions
localparam INST_ADD = 5'd0;  // Used For Multiple Instructions
localparam INST_SUB = 5'd1;
localparam INST_SLL = 5'd2;  // Both SLL and SLLI
localparam INST_SLT = 5'd3;  // Both SLT and SLTI
localparam INST_SLTU = 5'd4; // Both SLTU and SLTIU
localparam INST_XOR = 5'd5;  // Both XOR and XORI
localparam INST_SRL = 5'd6;  // Both SRL and SRLI
localparam INST_SRA = 5'd7;  // Both SRA and SRAI
localparam INST_OR = 5'd8;   // Both OR and ORI
localparam INST_AND = 5'd9;  // Both AND and ANDI
localparam INST_BEQ = 5'd10;
localparam INST_BNE = 5'd11;
localparam INST_BLT = 5'd12;
localparam INST_BGE = 5'd13;
localparam INST_BLTU = 5'd14;
localparam INST_BGEU = 5'd15;
localparam INST_MUL = 5'd16;
localparam INST_MULH = 5'd17;
localparam INST_MULHSU = 5'd18;
localparam INST_MULHU = 5'd19;

// ##################################################
//       MULTIPLIER UNIT (uses z_core_mult_unit)
// ##################################################

wire [63:0] multiplier_result;

// Signedness control for RISC-V M extension:
// MUL:    don't care (lower 32 bits same for all)
// MULH:   signed × signed (both signed)
// MULHSU: signed × unsigned (op1 signed, op2 unsigned)
// MULHU:  unsigned × unsigned (both unsigned)
wire mul_op1_signed = (alu_inst_type == INST_MULH) || (alu_inst_type == INST_MULHSU);
wire mul_op2_signed = (alu_inst_type == INST_MULH);

z_core_mult_unit mult_unit (
    .op1(alu_in1),
    .op2(alu_in2),
    .op1_signed(mul_op1_signed),
    .op2_signed(mul_op2_signed),
    .result(multiplier_result)
);


// ##################################################
//       ALU Result - Continuous Assignment Mux
// ##################################################

// Internal wires for each operation result
wire [31:0] add_result  = alu_in1 + alu_in2;
wire [31:0] sub_result  = alu_in1 - alu_in2;
wire [31:0] sll_result  = alu_in1 << alu_in2[4:0];
wire [31:0] slt_result  = ($signed(alu_in1) < $signed(alu_in2)) ? 32'd1 : 32'd0;
wire [31:0] sltu_result = (alu_in1 < alu_in2) ? 32'd1 : 32'd0;
wire [31:0] xor_result  = alu_in1 ^ alu_in2;
wire [31:0] srl_result  = alu_in1 >> alu_in2[4:0];
wire [31:0] sra_result  = $signed(alu_in1) >>> alu_in2[4:0];
wire [31:0] or_result   = alu_in1 | alu_in2;
wire [31:0] and_result  = alu_in1 & alu_in2;
wire [31:0] mul_result  = multiplier_result[31:0];
wire [31:0] mulh_result = multiplier_result[63:32];

// Output mux using continuous assignment
assign alu_out = (alu_inst_type == INST_ADD)    ? add_result  :
                 (alu_inst_type == INST_SUB)    ? sub_result  :
                 (alu_inst_type == INST_SLL)    ? sll_result  :
                 (alu_inst_type == INST_SLT)    ? slt_result  :
                 (alu_inst_type == INST_SLTU)   ? sltu_result :
                 (alu_inst_type == INST_XOR)    ? xor_result  :
                 (alu_inst_type == INST_SRL)    ? srl_result  :
                 (alu_inst_type == INST_SRA)    ? sra_result  :
                 (alu_inst_type == INST_OR)     ? or_result   :
                 (alu_inst_type == INST_AND)    ? and_result  :
                 (alu_inst_type == INST_MUL)    ? mul_result  :
                 (alu_inst_type == INST_MULH)   ? mulh_result :
                 (alu_inst_type == INST_MULHSU) ? mulh_result :
                 (alu_inst_type == INST_MULHU)  ? mulh_result :
                 32'd0;

// ##################################################
//       Branch Logic - Procedural Block
// ##################################################

always @(*) begin
    case (alu_inst_type) 
        INST_BEQ:  alu_branch = (alu_in1 == alu_in2);
        INST_BNE:  alu_branch = (alu_in1 != alu_in2);
        INST_BLT:  alu_branch = ($signed(alu_in1) < $signed(alu_in2));
        INST_BGE:  alu_branch = ($signed(alu_in1) >= $signed(alu_in2));
        INST_BLTU: alu_branch = (alu_in1 < alu_in2);
        INST_BGEU: alu_branch = (alu_in1 >= alu_in2);
        default:   alu_branch = 1'b0;
    endcase
end

endmodule