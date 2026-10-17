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

module z_core_reg_file(
    // Inputs
    input clk,
    input [4:0] rd,
    input [31:0] rd_in,
    input [4:0] rs1,
    input [4:0] rs2,
    input write_enable,
    input reset,

    // Outputs
    output [31:0] rs1_out,
    output [31:0] rs2_out
);

    // Register Iniialization

    reg [31:0] reg_r1_q;
    reg [31:0] reg_r2_q;
    reg [31:0] reg_r3_q;
    reg [31:0] reg_r4_q;
    reg [31:0] reg_r5_q;
    reg [31:0] reg_r6_q;
    reg [31:0] reg_r7_q;
    reg [31:0] reg_r8_q;
    reg [31:0] reg_r9_q;
    reg [31:0] reg_r10_q;
    reg [31:0] reg_r11_q;
    reg [31:0] reg_r12_q;
    reg [31:0] reg_r13_q;
    reg [31:0] reg_r14_q;
    reg [31:0] reg_r15_q;
    reg [31:0] reg_r16_q;
    reg [31:0] reg_r17_q;
    reg [31:0] reg_r18_q;
    reg [31:0] reg_r19_q;
    reg [31:0] reg_r20_q;
    reg [31:0] reg_r21_q;
    reg [31:0] reg_r22_q;
    reg [31:0] reg_r23_q;
    reg [31:0] reg_r24_q;
    reg [31:0] reg_r25_q;
    reg [31:0] reg_r26_q;
    reg [31:0] reg_r27_q;
    reg [31:0] reg_r28_q;
    reg [31:0] reg_r29_q;
    reg [31:0] reg_r30_q;
    reg [31:0] reg_r31_q;


     /* Synchronous read */

    always @(posedge clk) begin
        if(reset) begin
            reg_r1_q <= 32'b0;
            reg_r2_q <= 32'b0;
            reg_r3_q <= 32'b0;
            reg_r4_q <= 32'b0;
            reg_r5_q <= 32'b0;
            reg_r6_q <= 32'b0;
            reg_r7_q <= 32'b0;
            reg_r8_q <= 32'b0;
            reg_r9_q <= 32'b0;
            reg_r10_q <= 32'b0;
            reg_r11_q <= 32'b0;
            reg_r12_q <= 32'b0;
            reg_r13_q <= 32'b0;
            reg_r14_q <= 32'b0;
            reg_r15_q <= 32'b0;
            reg_r16_q <= 32'b0;
            reg_r17_q <= 32'b0;
            reg_r18_q <= 32'b0;
            reg_r19_q <= 32'b0;
            reg_r20_q <= 32'b0;
            reg_r21_q <= 32'b0;
            reg_r22_q <= 32'b0;
            reg_r23_q <= 32'b0;
            reg_r24_q <= 32'b0;
            reg_r25_q <= 32'b0;
            reg_r26_q <= 32'b0;
            reg_r27_q <= 32'b0;
            reg_r28_q <= 32'b0;
            reg_r29_q <= 32'b0;
            reg_r30_q <= 32'b0;
            reg_r31_q <= 32'b0;
        end
        else if (write_enable) begin
            if(rd == 5'h1) reg_r1_q <= rd_in;
            if(rd == 5'h2) reg_r2_q <= rd_in;
            if(rd == 5'h3) reg_r3_q <= rd_in;
            if(rd == 5'h4) reg_r4_q <= rd_in;
            if(rd == 5'h5) reg_r5_q <= rd_in;
            if(rd == 5'h6) reg_r6_q <= rd_in;
            if(rd == 5'h7) reg_r7_q <= rd_in;
            if(rd == 5'h8) reg_r8_q <= rd_in;
            if(rd == 5'h9) reg_r9_q <= rd_in;
            if(rd == 5'hA) reg_r10_q <= rd_in;
            if(rd == 5'hB) reg_r11_q <= rd_in;
            if(rd == 5'hC) reg_r12_q <= rd_in;
            if(rd == 5'hD) reg_r13_q <= rd_in;
            if(rd == 5'hE) reg_r14_q <= rd_in;
            if(rd == 5'hF) reg_r15_q <= rd_in;
            if(rd == 5'h10) reg_r16_q <= rd_in;
            if(rd == 5'h11) reg_r17_q <= rd_in;
            if(rd == 5'h12) reg_r18_q <= rd_in;
            if(rd == 5'h13) reg_r19_q <= rd_in;
            if(rd == 5'h14) reg_r20_q <= rd_in;
            if(rd == 5'h15) reg_r21_q <= rd_in;
            if(rd == 5'h16) reg_r22_q <= rd_in;
            if(rd == 5'h17) reg_r23_q <= rd_in;
            if(rd == 5'h18) reg_r24_q <= rd_in;
            if(rd == 5'h19) reg_r25_q <= rd_in;
            if(rd == 5'h1A) reg_r26_q <= rd_in;
            if(rd == 5'h1B) reg_r27_q <= rd_in;
            if(rd == 5'h1C) reg_r28_q <= rd_in;
            if(rd == 5'h1D) reg_r29_q <= rd_in;
            if(rd == 5'h1E) reg_r30_q <= rd_in;
            if(rd == 5'h1F) reg_r31_q <= rd_in;
        end
    end

    /* Asynchronous read */

    reg [31:0] rs1_reg;
    reg [31:0] rs2_reg;

    always @(*) begin

        case(rs1)
            5'h0: rs1_reg <= 32'd0;
            5'h1: rs1_reg <= reg_r1_q;
            5'h2: rs1_reg <= reg_r2_q;
            5'h3: rs1_reg <= reg_r3_q;
            5'h4: rs1_reg <= reg_r4_q;
            5'h5: rs1_reg <= reg_r5_q;
            5'h6: rs1_reg <= reg_r6_q;
            5'h7: rs1_reg <= reg_r7_q;
            5'h8: rs1_reg <= reg_r8_q;
            5'h9: rs1_reg <= reg_r9_q;
            5'hA: rs1_reg <= reg_r10_q;
            5'hB: rs1_reg <= reg_r11_q;
            5'hC: rs1_reg <= reg_r12_q;
            5'hD: rs1_reg <= reg_r13_q;
            5'hE: rs1_reg <= reg_r14_q;
            5'hF: rs1_reg <= reg_r15_q;
            5'h10: rs1_reg <= reg_r16_q;
            5'h11: rs1_reg <= reg_r17_q;
            5'h12: rs1_reg <= reg_r18_q;
            5'h13: rs1_reg <= reg_r19_q;
            5'h14: rs1_reg <= reg_r20_q;
            5'h15: rs1_reg <= reg_r21_q;
            5'h16: rs1_reg <= reg_r22_q;
            5'h17: rs1_reg <= reg_r23_q;
            5'h18: rs1_reg <= reg_r24_q;
            5'h19: rs1_reg <= reg_r25_q;
            5'h1A: rs1_reg <= reg_r26_q;
            5'h1B: rs1_reg <= reg_r27_q;
            5'h1C: rs1_reg <= reg_r28_q;
            5'h1D: rs1_reg <= reg_r29_q;
            5'h1E: rs1_reg <= reg_r30_q;
            5'h1F: rs1_reg <= reg_r31_q;

        endcase

        case(rs2)
            5'h0: rs2_reg <= 32'd0;
            5'h1: rs2_reg <= reg_r1_q;
            5'h2: rs2_reg <= reg_r2_q;
            5'h3: rs2_reg <= reg_r3_q;
            5'h4: rs2_reg <= reg_r4_q;
            5'h5: rs2_reg <= reg_r5_q;
            5'h6: rs2_reg <= reg_r6_q;
            5'h7: rs2_reg <= reg_r7_q;
            5'h8: rs2_reg <= reg_r8_q;
            5'h9: rs2_reg <= reg_r9_q;
            5'hA: rs2_reg <= reg_r10_q;
            5'hB: rs2_reg <= reg_r11_q;
            5'hC: rs2_reg <= reg_r12_q;
            5'hD: rs2_reg <= reg_r13_q;
            5'hE: rs2_reg <= reg_r14_q;
            5'hF: rs2_reg <= reg_r15_q;
            5'h10: rs2_reg <= reg_r16_q;
            5'h11: rs2_reg <= reg_r17_q;
            5'h12: rs2_reg <= reg_r18_q;
            5'h13: rs2_reg <= reg_r19_q;
            5'h14: rs2_reg <= reg_r20_q;
            5'h15: rs2_reg <= reg_r21_q;
            5'h16: rs2_reg <= reg_r22_q;
            5'h17: rs2_reg <= reg_r23_q;
            5'h18: rs2_reg <= reg_r24_q;
            5'h19: rs2_reg <= reg_r25_q;
            5'h1A: rs2_reg <= reg_r26_q;
            5'h1B: rs2_reg <= reg_r27_q;
            5'h1C: rs2_reg <= reg_r28_q;
            5'h1D: rs2_reg <= reg_r29_q;
            5'h1E: rs2_reg <= reg_r30_q;
            5'h1F: rs2_reg <= reg_r31_q;
        endcase
    end

assign rs1_out = rs1_reg;
assign rs2_out = rs2_reg;

endmodule