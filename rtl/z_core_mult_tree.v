// ============================================================================
// Fast Tree Multiplier - Educational Implementation
// Patterson & Hennessy Figure 3.7 Style
// 32-bit x 32-bit = 64-bit multiplication
//
// Uses 62 32-bit adders organized in a tree (31 pairs for 64-bit additions)
// This is the educational version showing explicit tree structure.
//
// RISC-V M Extension: MUL, MULH, MULHSU, MULHU
// ============================================================================

module full_adder (
    input a,
    input b,
    input cin,
    output sum,
    output cout
);

assign sum = a ^ b ^ cin;
assign cout = (a & b) | (b & cin) | (cin & a); 

endmodule

module adder_32b (
    input [31:0] op1,
    input [31:0] op2,
    input cin,
    output [31:0] result,
    output cout
);

wire [32:0] carry;
assign carry[0] = cin;

// Ripple carry adder using the full_adder module
generate
    genvar i;
    for (i = 0; i < 32; i = i + 1) begin : adder_gen
        full_adder u_full_adder (
            .a(op1[i]),
            .b(op2[i]),
            .cin(carry[i]),
            .sum(result[i]),
            .cout(carry[i+1])
        );
    end
endgenerate

assign cout = carry[32];

endmodule

module z_core_mult_tree (
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
// Partial Products: PP[i] = b if a[i]=1, else 0
// Expanded to 64-bit at position [i+31:i]
// ============================================================================
wire [63:0] pp [0:31];
genvar i;
generate
    for (i = 0; i < 32; i = i + 1) begin : gen_pp
        assign pp[i] = a[i] ? ({32'b0, b} << i) : 64'b0;
    end
endgenerate

// ============================================================================
// Tree Level 1: 16 additions (32 adder_32b)
// ============================================================================
wire [63:0] L1 [0:15];
generate
    for (i = 0; i < 16; i = i + 1) begin : L1_add
        wire c;
        adder_32b lo (.op1(pp[i<<1][31:0]), .op2(pp[(i<<1)+1][31:0]), 
                      .cin(1'b0), .result(L1[i][31:0]), .cout(c));
        adder_32b hi (.op1(pp[i<<1][63:32]), .op2(pp[(i<<1)+1][63:32]),
                      .cin(c), .result(L1[i][63:32]), .cout());
    end
endgenerate

// ============================================================================
// Tree Level 2: 8 additions (16 adder_32b)
// ============================================================================
wire [63:0] L2 [0:7];
generate
    for (i = 0; i < 8; i = i + 1) begin : L2_add
        wire c;
        adder_32b lo (.op1(L1[i<<1][31:0]), .op2(L1[(i<<1)+1][31:0]),
                      .cin(1'b0), .result(L2[i][31:0]), .cout(c));
        adder_32b hi (.op1(L1[i<<1][63:32]), .op2(L1[(i<<1)+1][63:32]),
                      .cin(c), .result(L2[i][63:32]), .cout());
    end
endgenerate

// ============================================================================
// Tree Level 3: 4 additions (8 adder_32b)
// ============================================================================
wire [63:0] L3 [0:3];
generate
    for (i = 0; i < 4; i = i + 1) begin : L3_add
        wire c;
        adder_32b lo (.op1(L2[i<<1][31:0]), .op2(L2[(i<<1)+1][31:0]),
                      .cin(1'b0), .result(L3[i][31:0]), .cout(c));
        adder_32b hi (.op1(L2[i<<1][63:32]), .op2(L2[(i<<1)+1][63:32]),
                      .cin(c), .result(L3[i][63:32]), .cout());
    end
endgenerate

// ============================================================================
// Tree Level 4: 2 additions (4 adder_32b)
// ============================================================================
wire [63:0] L4 [0:1];
generate
    for (i = 0; i < 2; i = i + 1) begin : L4_add
        wire c;
        adder_32b lo (.op1(L3[i<<1][31:0]), .op2(L3[(i<<1)+1][31:0]),
                      .cin(1'b0), .result(L4[i][31:0]), .cout(c));
        adder_32b hi (.op1(L3[i<<1][63:32]), .op2(L3[(i<<1)+1][63:32]),
                      .cin(c), .result(L4[i][63:32]), .cout());
    end
endgenerate

// ============================================================================
// Tree Level 5: Final addition (2 adder_32b)
// ============================================================================
wire [63:0] prod;
wire cf;
adder_32b final_lo (.op1(L4[0][31:0]), .op2(L4[1][31:0]),
                    .cin(1'b0), .result(prod[31:0]), .cout(cf));
adder_32b final_hi (.op1(L4[0][63:32]), .op2(L4[1][63:32]),
                    .cin(cf), .result(prod[63:32]), .cout());

// ============================================================================
// Sign Correction
// ============================================================================
assign result = res_neg ? (~prod + 64'd1) : prod;

// Hardware: 62 adder_32b = 1984 full_adders + 3 negations

endmodule
