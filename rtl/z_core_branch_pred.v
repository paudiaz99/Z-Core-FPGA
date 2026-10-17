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
    parameter TABLE_DEPTH = 128,
    parameter GLOBAL_HISTORY_DEPTH = 5
)(
    input clk,
    input rstn,
    input branch_taken,
    input is_branch,
    input is_jump,
    input [ADDR_WIDTH-1:0] inst_addr_wr,
    input [ADDR_WIDTH-1:0] branch_target_wr,
    input [ADDR_WIDTH-1:0] inst_addr_rd,
    input [GLOBAL_HISTORY_DEPTH-1:0] ghr_snapshot,

    output wire branch_taken_pred,
    output wire [ADDR_WIDTH-1:0] branch_target_pred,
    output wire [GLOBAL_HISTORY_DEPTH-1:0] ghr_out
);

localparam STRONG_TAKEN = 3, WEAK_TAKEN = 2, WEAK_NOT_TAKEN = 1, STRONG_NOT_TAKEN = 0;
localparam STRONG_GLOBAL_HISTORY = 3, WEAK_GLOBAL_HISTORY = 2, WEAK_LOCAL = 1, STRONG_LOCAL = 0;
localparam BRANCH_TARGET_BUFF_ADDR_WIDTH = $clog2(TABLE_DEPTH);
localparam BRANCH_TABLE_TAG_WIDTH = ADDR_WIDTH - 2 - BRANCH_TARGET_BUFF_ADDR_WIDTH;
integer i_btb, i_pht, i_ghr;

wire is_branch_or_jump = is_branch || is_jump;

reg [GLOBAL_HISTORY_DEPTH-1:0] global_history_shift_reg;
reg [1:0] tournament_state [TABLE_DEPTH-1:0];

reg [ADDR_WIDTH-1:0] branch_target_buffer [TABLE_DEPTH-1:0];
reg [BRANCH_TABLE_TAG_WIDTH-1:0] branch_table_tag [TABLE_DEPTH-1:0];
reg [1:0] branch_history_table [TABLE_DEPTH-1:0];
reg branch_table_valid [TABLE_DEPTH-1:0];
reg branch_table_is_jump [TABLE_DEPTH-1:0];

wire [BRANCH_TABLE_TAG_WIDTH-1:0] tag_wr = inst_addr_wr[ADDR_WIDTH-1:ADDR_WIDTH-BRANCH_TABLE_TAG_WIDTH];
wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_wr_pc = inst_addr_wr[BRANCH_TARGET_BUFF_ADDR_WIDTH+1:2];

wire [BRANCH_TABLE_TAG_WIDTH-1:0] tag_rd = inst_addr_rd[ADDR_WIDTH-1:ADDR_WIDTH-BRANCH_TABLE_TAG_WIDTH];
wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_rd_pc = inst_addr_rd[BRANCH_TARGET_BUFF_ADDR_WIDTH+1:2];

wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_local_wr  = addr_wr_pc;
wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_global_wr = addr_wr_pc ^ ghr_snapshot;
wire [BRANCH_TARGET_BUFF_ADDR_WIDTH-1:0] addr_rd = tournament_state[addr_rd_pc][1]
                                                ? (addr_rd_pc ^ global_history_shift_reg)
                                                : addr_rd_pc;

wire local_pred  = branch_history_table[addr_local_wr][1];
wire global_pred = branch_history_table[addr_global_wr][1];
wire local_ok    = (local_pred  == branch_taken);
wire global_ok   = (global_pred == branch_taken);

function [1:0] pht_next;
    input [1:0] state;
    input taken;
    begin
        case (state)
            STRONG_TAKEN:     pht_next = taken ? STRONG_TAKEN     : WEAK_TAKEN;
            WEAK_TAKEN:       pht_next = taken ? STRONG_TAKEN     : WEAK_NOT_TAKEN;
            WEAK_NOT_TAKEN:   pht_next = taken ? WEAK_TAKEN       : STRONG_NOT_TAKEN;
            STRONG_NOT_TAKEN: pht_next = taken ? WEAK_NOT_TAKEN   : STRONG_NOT_TAKEN;
            default:          pht_next = WEAK_NOT_TAKEN;
        endcase
    end
endfunction

function [1:0] tournament_next;
    input [1:0] state;
    input pred_local_ok;
    input pred_global_ok;
    begin
        tournament_next = state;
        if (pred_local_ok != pred_global_ok) begin
            if (pred_global_ok) begin
                case (state)
                    STRONG_LOCAL:          tournament_next = WEAK_LOCAL;
                    WEAK_LOCAL:            tournament_next = WEAK_GLOBAL_HISTORY;
                    WEAK_GLOBAL_HISTORY:   tournament_next = STRONG_GLOBAL_HISTORY;
                    STRONG_GLOBAL_HISTORY: tournament_next = STRONG_GLOBAL_HISTORY;
                    default:               tournament_next = state;
                endcase
            end else begin
                case (state)
                    STRONG_GLOBAL_HISTORY: tournament_next = WEAK_GLOBAL_HISTORY;
                    WEAK_GLOBAL_HISTORY:   tournament_next = WEAK_LOCAL;
                    WEAK_LOCAL:            tournament_next = STRONG_LOCAL;
                    STRONG_LOCAL:          tournament_next = STRONG_LOCAL;
                    default:               tournament_next = state;
                endcase
            end
        end
    end
endfunction

wire [1:0] next_state_local      = pht_next(branch_history_table[addr_local_wr], branch_taken);
wire [1:0] next_state_global     = pht_next(branch_history_table[addr_global_wr], branch_taken);
wire [1:0] next_state_tournament = tournament_next(tournament_state[addr_wr_pc], local_ok, global_ok);

always @(posedge clk) begin
    if (~rstn) begin
        for (i_btb = 0; i_btb < TABLE_DEPTH; i_btb = i_btb + 1) begin
            branch_target_buffer[i_btb] <= {ADDR_WIDTH{1'b0}};
            branch_table_tag[i_btb]     <= {BRANCH_TABLE_TAG_WIDTH{1'b0}};
            branch_table_valid[i_btb]   <= 1'b0;
            branch_table_is_jump[i_btb] <= 1'b0;
        end
    end else if (is_branch_or_jump) begin
        branch_target_buffer[addr_wr_pc] <= branch_target_wr;
        branch_table_tag[addr_wr_pc]     <= tag_wr;
        branch_table_valid[addr_wr_pc]   <= 1'b1;
        branch_table_is_jump[addr_wr_pc] <= is_jump;
    end
end

always @(posedge clk) begin
    if (~rstn) begin
        global_history_shift_reg <= {GLOBAL_HISTORY_DEPTH{1'b0}};
        for (i_pht = 0; i_pht < TABLE_DEPTH; i_pht = i_pht + 1) begin
            branch_history_table[i_pht] <= 2'b01;
            tournament_state[i_pht]     <= 2'b01;
        end
    end else if (is_branch) begin
        global_history_shift_reg[0]          <= branch_taken;
        tournament_state[addr_wr_pc]         <= next_state_tournament;
        branch_history_table[addr_local_wr]  <= next_state_local;
        if (addr_global_wr != addr_local_wr)
            branch_history_table[addr_global_wr] <= next_state_global;
        for (i_ghr = 1; i_ghr < GLOBAL_HISTORY_DEPTH; i_ghr = i_ghr + 1)
            global_history_shift_reg[i_ghr] <= global_history_shift_reg[i_ghr-1];
    end
end

assign branch_taken_pred = branch_table_valid[addr_rd_pc]
                        && (tag_rd == branch_table_tag[addr_rd_pc])
                        && (branch_table_is_jump[addr_rd_pc] || branch_history_table[addr_rd][1]);
assign branch_target_pred = branch_target_buffer[addr_rd_pc];
assign ghr_out = global_history_shift_reg;

endmodule
