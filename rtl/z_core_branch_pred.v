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

module z_core_branch_pred #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter TABLE_DEPTH = 32
)(
    input clk,
    input rstn,
    input branch_taken,
    input is_branch,
    input [ADDR_WIDTH-1:0] inst_addr_wr,
    input [ADDR_WIDTH-1:0] branch_target_wr,
    input [ADDR_WIDTH-1:0] inst_addr_rd,

    output wire branch_taken_pred,
    output wire [ADDR_WIDTH-1:0] branch_target_pred
);

localparam STRONG_TAKEN = 3, WEAK_TAKEN = 2, WEAK_NOT_TAKEN = 1, STRONG_NOT_TAKEN = 0;
localparam BRANCH_TARGET_BUFF_ADDR_WIDTH = $clog2(TABLE_DEPTH);
localparam BRANCH_TABLE_TAG_WIDTH = ADDR_WIDTH - 2 - BRANCH_TARGET_BUFF_ADDR_WIDTH;
integer j;

reg [1:0] next_state;

reg [ADDR_WIDTH-1:0] branch_target_buffer [TABLE_DEPTH-1:0]; // Contains target address.
reg [BRANCH_TABLE_TAG_WIDTH-1:0] branch_table_tag [TABLE_DEPTH-1:0];
reg [1:0] branch_history_table [TABLE_DEPTH-1:0]; // Contains predicted branch bits (current_state)

wire [BRANCH_TABLE_TAG_WIDTH-1:0] tag_wr = inst_addr_wr[ADDR_WIDTH-1:ADDR_WIDTH-BRANCH_TABLE_TAG_WIDTH];
wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_wr = inst_addr_wr[BRANCH_TARGET_BUFF_ADDR_WIDTH+1:2];

wire [BRANCH_TABLE_TAG_WIDTH-1:0] tag_rd = inst_addr_rd[ADDR_WIDTH-1:ADDR_WIDTH-BRANCH_TABLE_TAG_WIDTH];
wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_rd = inst_addr_rd[BRANCH_TARGET_BUFF_ADDR_WIDTH+1:2];


always @* begin
    case (branch_history_table[addr_wr])
        STRONG_TAKEN:       next_state = branch_taken ? STRONG_TAKEN : WEAK_TAKEN;
        WEAK_TAKEN:         next_state = branch_taken ? STRONG_TAKEN : STRONG_NOT_TAKEN;
        WEAK_NOT_TAKEN:     next_state = branch_taken ? STRONG_TAKEN : STRONG_NOT_TAKEN;
        STRONG_NOT_TAKEN:   next_state = branch_taken ? WEAK_NOT_TAKEN : STRONG_NOT_TAKEN;
    endcase;
end

always @(posedge clk) begin
    if(~rstn) begin
        for (j=0; j < TABLE_DEPTH; j=j+1) begin
            branch_history_table[j] <= 2'b01; // Start at Weak Not Taken
            branch_target_buffer[j] <= {ADDR_WIDTH{1'b0}};
            branch_table_tag[j]     <= {BRANCH_TABLE_TAG_WIDTH{1'b0}};
        end
    end else if(is_branch) begin
        branch_target_buffer[addr_wr] <= branch_target_wr;
        branch_history_table[addr_wr] <= next_state;
        branch_table_tag[addr_wr] <= tag_wr;
    end
end

assign branch_taken_pred = branch_history_table[addr_rd][1] && (tag_rd == branch_table_tag[addr_rd]);
assign branch_target_pred = branch_target_buffer[addr_rd];

endmodule