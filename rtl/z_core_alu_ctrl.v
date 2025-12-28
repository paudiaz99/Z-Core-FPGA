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


module z_core_alu_ctrl(
    input [6:0] alu_op,
    input [2:0] alu_funct3,
    input [6:0] alu_funct7,
    output reg [4:0] alu_inst_type
);

// R-Type Instructions
localparam R_INST = 7'b0110011;

// I-Type Instructions
localparam I_INST = 7'b0010011;
localparam I_LOAD_INST = 7'b0000011;
localparam JALR_INST = 7'b1100111;

// S/B-Type Instructions
localparam S_INST = 7'b0100011;
localparam B_INST = 7'b1100011;

// J/U-Type Instructions
localparam JAL_INST = 7'b1101111;
localparam LUI_INST = 7'b0110111;
localparam AUIPC_INST = 7'b0010111;

// Function3 Codes
localparam F3_ADD_SUB_LB_JALR_SB_BEQ_MUL = 3'b000;
localparam F3_SLL_LH_SH_BNE_MULH = 3'b001;
localparam F3_SLT_LW_SW_MULHSU = 3'b010;
localparam F3_SLTU_MULHU = 3'b011;
localparam F3_XOR_LBU_BLT_DIV = 3'b100;
localparam F3_SRL_SRA_LHU_BGE_DIVU = 3'b101;
localparam F3_OR_BLTU_REM = 3'b110;
localparam F3_AND_BGEU_REMU = 3'b111;

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
localparam INST_DIV = 5'd20;
localparam INST_DIVU = 5'd21;
localparam INST_REM = 5'd22;
localparam INST_REMU = 5'd23;


always @(*) begin
    case(alu_op)
        R_INST: begin
            case(alu_funct3)
                F3_ADD_SUB_LB_JALR_SB_BEQ_MUL: begin
                    if (alu_funct7[5] == 1'b1) begin
                        alu_inst_type = INST_SUB; // SUB
                    end else if(alu_funct7[0] == 1'b1) begin
                        alu_inst_type = INST_MUL; // MUL
                    end else begin
                        alu_inst_type = INST_ADD; // ADD
                    end
                end
                F3_SLL_LH_SH_BNE_MULH: alu_inst_type = alu_funct7[0] ? INST_MULH : INST_SLL; // SLL or MULH
                F3_SLT_LW_SW_MULHSU: alu_inst_type = alu_funct7[0] ? INST_MULHSU : INST_SLT; // SLT or MULHSU
                F3_SLTU_MULHU: alu_inst_type = alu_funct7[0] ? INST_MULHU : INST_SLTU; // SLTU or MULHU
                F3_XOR_LBU_BLT_DIV: alu_inst_type = alu_funct7[0] ? INST_DIV : INST_XOR; // XOR
                F3_SRL_SRA_LHU_BGE_DIVU: alu_inst_type = alu_funct7[0] ? INST_DIVU : (alu_funct7[5] ? INST_SRA : INST_SRL); // SRL or SRA
                F3_OR_BLTU_REM: alu_inst_type = alu_funct7[0] ? INST_REM : INST_OR; // OR
                F3_AND_BGEU_REMU: alu_inst_type = alu_funct7[0] ? INST_REMU : INST_AND; // AND
                default: alu_inst_type = 5'bxxxxx; // Invalid
            endcase
        end
        I_INST: begin
            case(alu_funct3)
                F3_ADD_SUB_LB_JALR_SB_BEQ_MUL: alu_inst_type = INST_ADD; // ADDI
                F3_SLL_LH_SH_BNE_MULH: alu_inst_type = INST_SLL; // SLLI
                F3_SLT_LW_SW_MULHSU: alu_inst_type = INST_SLT; // SLTI
                F3_SLTU_MULHU: alu_inst_type = INST_SLTU; // SLTIU
                F3_XOR_LBU_BLT_DIV: alu_inst_type = INST_XOR; // XORI
                F3_SRL_SRA_LHU_BGE_DIVU: alu_inst_type = alu_funct7[5] ? INST_SRA : INST_SRL; // SRL or SRA
                F3_OR_BLTU_REM: alu_inst_type = INST_OR; // ORI
                F3_AND_BGEU_REMU: alu_inst_type = INST_AND; // ANDI
                default: alu_inst_type = 5'bxxxxx; // Invalid
            endcase
        end
        I_LOAD_INST: alu_inst_type = INST_ADD; // Load uses ADD for address calculation
        S_INST: alu_inst_type = INST_ADD; // Store uses ADD for address calculation
        B_INST: begin
            case(alu_funct3)
                F3_ADD_SUB_LB_JALR_SB_BEQ_MUL: alu_inst_type = INST_BEQ; // BEQ
                F3_SLL_LH_SH_BNE_MULH: alu_inst_type = INST_BNE; // BNE
                F3_XOR_LBU_BLT_DIV: alu_inst_type = INST_BLT; // BLT
                F3_OR_BLTU_REM: alu_inst_type = INST_BLTU; // BLTU
                F3_SRL_SRA_LHU_BGE_DIVU: alu_inst_type = INST_BGE; // BGE
                F3_AND_BGEU_REMU: alu_inst_type = INST_BGEU; // BGEU
                default: alu_inst_type = 5'bxxxxx; // Invalid
            endcase
        end
        JALR_INST: alu_inst_type = INST_ADD; // JALR uses ADD
        JAL_INST: alu_inst_type = INST_ADD; // JAL uses ADD
        LUI_INST: alu_inst_type = INST_ADD; // LUI uses ADD
        AUIPC_INST: alu_inst_type = INST_ADD; // AUIPC uses ADD
        default: alu_inst_type = 5'bxxxxx; // Invalid
    endcase

end

endmodule