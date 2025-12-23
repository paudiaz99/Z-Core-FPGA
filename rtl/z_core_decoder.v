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

module z_core_decoder (
    input [31:0] inst,
    output [6:0] op,
    output [4:0] rs1,
    output [4:0] rs2,
    output [4:0] rd,
    output [31:0] Iimm,
    output [31:0] Simm,
    output [31:0] Uimm,
    output [31:0] Bimm,
    output [31:0] Jimm,
    output [2:0] funct3,
    output [6:0] funct7
);

    // Decode Operation
    assign op = inst[6:0];

    // Decode Registers
    assign rs1 = inst[19:15];
    assign rs2 = inst[24:20];
    assign rd = inst[11:7];

    // Decode Funct
    assign funct3 = inst[14:12];
    assign funct7 = inst[31:25];

    // Decode Immediates
    // I-type: imm[11:0] = inst[31:20], sign-extended
    assign Iimm = {{21{inst[31]}}, inst[30:20]};
    // S-type: imm[11:5|4:0] = inst[31:25|11:7], sign-extended
    assign Simm = {{21{inst[31]}}, inst[30:25], inst[11:7]};
    // B-type: imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7], sign-extended
    assign Bimm = {{20{inst[31]}}, inst[7], inst[30:25], inst[11:8], 1'b0};
    // U-type: imm[31:12] = inst[31:12], lower 12 bits zero
    assign Uimm = {inst[31:12], {12{1'b0}}};
    // J-type: imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12], sign-extended
    assign Jimm = {{12{inst[31]}}, inst[19:12], inst[20], inst[30:21], 1'b0};

endmodule