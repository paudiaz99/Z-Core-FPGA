module z_core_instr_cache #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter CACHE_DEPTH = 256
) (
    input wire clk,
    input wire rstn,
    input wire wen,
    input wire [ADDR_WIDTH-1:0] addr_rd,
    input wire [ADDR_WIDTH-1:0] addr_wr,
    input wire [DATA_WIDTH-1:0] data_in,
    output wire [DATA_WIDTH-1:0] data_out,

    output wire valid,
    output wire cache_hit,
    output wire cache_miss
);

// **************************************************
//      Dual-Port Instruction Cache (256x32)
//      Port A: Asynchronous Read (Fetch)
//      Port B: Synchronous Write (Memory Fill)
// **************************************************

localparam CACHE_ADDR_WIDTH = $clog2(CACHE_DEPTH);
localparam CACHE_TAG_WIDTH = ADDR_WIDTH - 2 - CACHE_ADDR_WIDTH;

reg [DATA_WIDTH-1:0] instr_cache [CACHE_DEPTH-1:0];
reg [CACHE_TAG_WIDTH-1:0] instr_cache_tag [CACHE_DEPTH-1:0];
reg [CACHE_DEPTH-1:0] instr_cache_valid;

// Port A: Read Logic
wire [CACHE_TAG_WIDTH-1:0] tag_rd = addr_rd[ADDR_WIDTH-1:ADDR_WIDTH-CACHE_TAG_WIDTH];
wire [CACHE_ADDR_WIDTH-1:0] index_rd = addr_rd[CACHE_ADDR_WIDTH+1:2];

assign data_out = instr_cache[index_rd];
assign cache_hit = (instr_cache_tag[index_rd] == tag_rd) && instr_cache_valid[index_rd];
assign cache_miss = !((instr_cache_tag[index_rd] == tag_rd) && instr_cache_valid[index_rd]);
assign valid = (instr_cache_tag[index_rd] == tag_rd) && instr_cache_valid[index_rd];

// Port B: Write Logic
wire [CACHE_TAG_WIDTH-1:0] tag_wr = addr_wr[ADDR_WIDTH-1:ADDR_WIDTH-CACHE_TAG_WIDTH];
wire [CACHE_ADDR_WIDTH-1:0] index_wr = addr_wr[CACHE_ADDR_WIDTH+1:2];

always @(posedge clk) begin
    if (!rstn) begin
        instr_cache_valid <= {CACHE_DEPTH{1'b0}};
    end else if (wen) begin
        instr_cache[index_wr] <= data_in;
        instr_cache_tag[index_wr] <= tag_wr;
        instr_cache_valid[index_wr] <= 1'b1;
    end
end

endmodule
