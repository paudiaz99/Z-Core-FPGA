module z_core_data_cache #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter CACHE_ENTRIES = 8192,
    parameter ASSOCIATIVITY = 2,
    parameter CACHE_LINE_SIZE = 4,
    parameter CACHE_DEPTH = CACHE_ENTRIES / ASSOCIATIVITY,
    parameter CACHE_ADDR_WIDTH = $clog2(CACHE_DEPTH),
    parameter CACHE_TAG_WIDTH = ADDR_WIDTH - 2 - CACHE_ADDR_WIDTH
)(
    input wire clk,
    input wire rstn,
    input wire wen,
    input wire cs,
    input wire [3:0] strb,
    input wire refill_complete,
    input wire pipeline_enable,

    input wire [ADDR_WIDTH-1:0] addr,
    input wire [DATA_WIDTH-1:0] data_in,

    output reg [DATA_WIDTH-1:0] data_out,
    output wire cache_hit_comb,
    output reg dirty_writeback_enabled,
    output reg [ADDR_WIDTH-1:0] dirty_writeback_addr,
    output reg [DATA_WIDTH-1:0] dirty_writeback_data,
    output reg [3:0] dirty_writeback_strb,
    output reg request_refill,
    output wire cache_flush_done   // low while post-reset invalidation runs; stall mem ops until high
);

// Wide arrays → BRAM inference (no_rw_check suppresses the read-during-write check)
(* ramstyle = "no_rw_check" *)
reg [DATA_WIDTH-1:0] data_way0 [CACHE_DEPTH-1:0];
(* ramstyle = "no_rw_check" *)
reg [DATA_WIDTH-1:0] data_way1 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg [CACHE_TAG_WIDTH-1:0] tags_way0 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg [CACHE_TAG_WIDTH-1:0] tags_way1 [CACHE_DEPTH-1:0];

// Unpacked 1-bit arrays (NOT packed vectors) so Quartus infers M9K instead of
// a 4096:1 read mux. No reset on contents -> cleared by the flush counter below.
(* ramstyle = "M9K" *)
reg valid_bits_way0 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg valid_bits_way1 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg dirty_bits_way0 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg dirty_bits_way1 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg lru_bits_way0 [CACHE_DEPTH-1:0];
(* ramstyle = "M9K" *)
reg lru_bits_way1 [CACHE_DEPTH-1:0];

// Hardware flush: walks every index after reset clearing valid/dirty/lru,
// since M9K contents cannot be reset in a single cycle.
reg [CACHE_ADDR_WIDTH:0] flush_cnt;
wire flush_running = ~flush_cnt[CACHE_ADDR_WIDTH];
assign cache_flush_done = ~flush_running;

reg [DATA_WIDTH-1:0] refill_buffer_data;
reg refill_buffer_set;
reg refill_wen;
reg [CACHE_ADDR_WIDTH-1:0] refill_index_q;
reg [CACHE_TAG_WIDTH-1:0]  refill_tag_q;

// Pipeline Registers
wire [CACHE_TAG_WIDTH-1:0] tag_0_r;
wire [CACHE_TAG_WIDTH-1:0] tag_1_r;
wire valid_0_r;
wire valid_1_r;
wire dirty_0_r;
wire dirty_1_r;
wire lru_0_r;
wire lru_1_r;
reg [CACHE_TAG_WIDTH-1:0] tag_way_0_q;
reg [CACHE_TAG_WIDTH-1:0] tag_way_1_q;
reg valid_way_0_q;
reg valid_way_1_q;
reg dirty_way_0_q;
reg dirty_way_1_q;
reg lru_way_0_q;
reg lru_way_1_q;
reg [ADDR_WIDTH-1:0] data_addr_q;

wire [CACHE_TAG_WIDTH-1:0] tag_addr = addr[ADDR_WIDTH-1:CACHE_ADDR_WIDTH+2];
wire [CACHE_ADDR_WIDTH-1:0] index_addr = addr[CACHE_ADDR_WIDTH+1:2];

wire [CACHE_TAG_WIDTH-1:0] tag_addr_q = data_addr_q[ADDR_WIDTH-1:CACHE_ADDR_WIDTH+2];
wire [CACHE_ADDR_WIDTH-1:0] index_addr_q = data_addr_q[CACHE_ADDR_WIDTH+1:2];

// 2-Set Associative Cache - Set One and Set Two Hits
wire set_one_hit = tag_0_r == tag_addr_q && valid_0_r;
wire set_two_hit = tag_1_r == tag_addr_q && valid_1_r;

wire valid_bit_set_one = valid_0_r;
wire valid_bit_set_two = valid_1_r;

assign cache_hit_comb = (((set_one_hit && valid_bit_set_one) || (set_two_hit && valid_bit_set_two)) && cs) && ~flush_running;
wire set_to_write_on_hit = (set_one_hit && valid_bit_set_one) ? 1'b0 : 1'b1;

// Dirty Bit Checks
wire dirty_bit_set_one = dirty_0_r;
wire dirty_bit_set_two = dirty_1_r;


// LRU Bit Checks
wire lru_bit_set_one = lru_0_r;
wire lru_bit_set_two = lru_1_r;

wire lru_bit_set_to_write = lru_bit_set_one ? 0 : 1;

// Empty Slot Checks
wire empty_slot = !valid_bit_set_one || !valid_bit_set_two;
wire empty_slot_set_one = !valid_0_r;
wire empty_slot_to_write = empty_slot_set_one ? 0 : 1;

// Final set to write on miss
wire set_to_write_on_miss = empty_slot ? empty_slot_to_write : lru_bit_set_to_write;

wire [DATA_WIDTH-1:0] mask = {8{strb[0]}} << 0 | {8{strb[1]}} << 8 | {8{strb[2]}} << 16 | {8{strb[3]}} << 24;
integer j = 0;
// Write
always @(posedge clk) begin
    if (!rstn) begin
        flush_cnt <= {(CACHE_ADDR_WIDTH+1){1'b0}};
        dirty_writeback_enabled <= 1'b0;
        data_out <= 32'b0;
        request_refill <= 1'b0;
        refill_index_q <= {CACHE_ADDR_WIDTH{1'b0}};
        refill_tag_q   <= {CACHE_TAG_WIDTH{1'b0}};
    end else if(flush_running) begin
        valid_bits_way0[flush_cnt[CACHE_ADDR_WIDTH-1:0]] <= 1'b0;
        valid_bits_way1[flush_cnt[CACHE_ADDR_WIDTH-1:0]] <= 1'b0;
        dirty_bits_way0[flush_cnt[CACHE_ADDR_WIDTH-1:0]] <= 1'b0;
        dirty_bits_way1[flush_cnt[CACHE_ADDR_WIDTH-1:0]] <= 1'b0;
        lru_bits_way0[flush_cnt[CACHE_ADDR_WIDTH-1:0]]   <= 1'b0;
        lru_bits_way1[flush_cnt[CACHE_ADDR_WIDTH-1:0]]   <= 1'b0;
        flush_cnt <= flush_cnt + 1'b1;
    end else if(refill_complete) begin
        request_refill <= 1'b0;
        if (refill_buffer_set == 1'b0) begin
            data_way0[refill_index_q] <= (refill_wen ? data_in & ~mask | refill_buffer_data & mask : data_in);
            tags_way0[refill_index_q] <= refill_tag_q;
            valid_bits_way0[refill_index_q] <= 1'b1;
            dirty_bits_way0[refill_index_q] <= refill_wen;
            lru_bits_way0[refill_index_q] <= 1'b0;
            lru_bits_way1[refill_index_q] <= 1'b1;
        end else begin
            data_way1[refill_index_q] <= (refill_wen ? data_in & ~mask | refill_buffer_data & mask : data_in);
            tags_way1[refill_index_q] <= refill_tag_q;
            valid_bits_way1[refill_index_q] <= 1'b1;
            dirty_bits_way1[refill_index_q] <= refill_wen;
            lru_bits_way1[refill_index_q] <= 1'b0;
            lru_bits_way0[refill_index_q] <= 1'b1;
        end
    end else if (!cs) begin
        dirty_writeback_enabled <= dirty_writeback_enabled ? 1'b0 : dirty_writeback_enabled;
    end else if(cs && wen) begin
        if(cache_hit_comb) begin
            if (set_to_write_on_hit == 1'b0) begin
                data_way0[index_addr_q] <= data_way0[index_addr_q] & ~mask | data_in & mask;
                tags_way0[index_addr_q] <= tag_addr_q;
                valid_bits_way0[index_addr_q] <= 1'b1;
                dirty_bits_way0[index_addr_q] <= 1'b1;
                lru_bits_way0[index_addr_q] <= 1'b0;
                lru_bits_way1[index_addr_q] <= 1'b1;
            end else begin
                data_way1[index_addr_q] <= data_way1[index_addr_q] & ~mask | data_in & mask;
                tags_way1[index_addr_q] <= tag_addr_q;
                valid_bits_way1[index_addr_q] <= 1'b1;
                dirty_bits_way1[index_addr_q] <= 1'b1;
                lru_bits_way1[index_addr_q] <= 1'b0;
                lru_bits_way0[index_addr_q] <= 1'b1;
            end
        end else if(!request_refill) begin
            refill_index_q <= index_addr_q;
            refill_tag_q   <= tag_addr_q;
            dirty_writeback_enabled <= set_to_write_on_miss ? dirty_1_r : dirty_0_r;
            dirty_writeback_addr <= set_to_write_on_miss ? {tag_1_r, index_addr_q, 2'b00} : {tag_0_r, index_addr_q, 2'b00};
            dirty_writeback_data <= set_to_write_on_miss ? data_way1[index_addr_q] : data_way0[index_addr_q];
            dirty_writeback_strb <= 4'b1111;
            refill_buffer_data <= data_in & mask;
            refill_buffer_set <= set_to_write_on_miss;
            refill_wen <= 1'b1;
            request_refill <= 1'b1;
        end
    end else if(cs && !wen) begin
        if(cache_hit_comb) begin
            data_out <= (set_to_write_on_hit ? data_way1[index_addr_q] : data_way0[index_addr_q]) & mask;
        end else if(!request_refill) begin
            refill_index_q <= index_addr_q;
            refill_tag_q   <= tag_addr_q;
            dirty_writeback_enabled <= set_to_write_on_miss ? dirty_1_r : dirty_0_r;
            dirty_writeback_addr <= set_to_write_on_miss ? {tag_1_r, index_addr_q, 2'b00} : {tag_0_r, index_addr_q, 2'b00};
            dirty_writeback_data <= set_to_write_on_miss ? data_way1[index_addr_q] : data_way0[index_addr_q];
            dirty_writeback_strb <= 4'b1111;
            request_refill <= 1'b1;
            refill_buffer_set <= set_to_write_on_miss;
            refill_wen <= 1'b0;
            data_out <= 32'b0;
        end
    end
end


// ----------------------------------------------------------------------
// Same-cycle write-during-read bypass conditions for the M9K-inferred
// metadata arrays. With (* ramstyle="no_rw_check" *) the M9K returns
// OLD data on a read at the same index that's being written this cycle.
// The next access at that index would then see stale dirty/lru/tag
// values, mis-classify a victim as clean, and silently drop dirty data.
//
// Two write paths can collide with the pipeline_enable read:
//   (1) refill_complete writes data/tag/valid/dirty/lru at refill_index_q.
//   (2) cs && wen && cache_hit_comb writes dirty/lru at index_addr_q
//       (transitions clean->dirty, flips LRU). tag/valid don't change
//       on a hit, so they don't need bypassing on this path.
// ----------------------------------------------------------------------

// (1) refill_complete same-index detection
wire refill_to_same_index_w0 =
    refill_complete && pipeline_enable && (index_addr == refill_index_q) &&
    (refill_buffer_set == 1'b0);
wire refill_to_same_index_w1 =
    refill_complete && pipeline_enable && (index_addr == refill_index_q) &&
    (refill_buffer_set == 1'b1);

// (2) hit-write same-index detection (dirty/lru race only)
wire hit_write_to_same_index_w0 =
    pipeline_enable && cs && wen && cache_hit_comb &&
    (set_to_write_on_hit == 1'b0) &&
    (index_addr == index_addr_q);
wire hit_write_to_same_index_w1 =
    pipeline_enable && cs && wen && cache_hit_comb &&
    (set_to_write_on_hit == 1'b1) &&
    (index_addr == index_addr_q);


always @(posedge clk) begin
    if (pipeline_enable) begin
        tag_way_0_q <= tags_way0[index_addr];
        tag_way_1_q <= tags_way1[index_addr];
    end
end

always @(posedge clk) begin
    if (!rstn) begin
        data_addr_q <= 32'b0;
    end else if (pipeline_enable) begin
        valid_way_0_q <= valid_bits_way0[index_addr];
        valid_way_1_q <= valid_bits_way1[index_addr];
        dirty_way_0_q <= dirty_bits_way0[index_addr];
        dirty_way_1_q <= dirty_bits_way1[index_addr];
        lru_way_0_q   <= lru_bits_way0[index_addr];
        lru_way_1_q   <= lru_bits_way1[index_addr];
        data_addr_q   <= addr;
    end
end

// Refill-complete bypass shadow latches: capture the bypass select /
// replacement values at the same edge that the M9K read is latched.
// These are then consulted by the combinational mux below in cycle N+1.
reg                          refill_bypass_w0_q;
reg                          refill_bypass_w1_q;
reg                          refill_bypass_wen_q;
reg [CACHE_TAG_WIDTH-1:0]    refill_bypass_tag_q;

always @(posedge clk) begin
    if (!rstn) begin
        refill_bypass_w0_q  <= 1'b0;
        refill_bypass_w1_q  <= 1'b0;
        refill_bypass_wen_q <= 1'b0;
    end else if (pipeline_enable) begin
        refill_bypass_w0_q  <= refill_to_same_index_w0;
        refill_bypass_w1_q  <= refill_to_same_index_w1;
        refill_bypass_wen_q <= refill_wen;
        refill_bypass_tag_q <= refill_tag_q;
    end
end

// Hit-write bypass shadow latches: capture the same-cycle hit-write
// condition. Only dirty/lru transition on a hit-write, so we only
// need this path for those two output muxes.
reg                          hit_bypass_w0_q;
reg                          hit_bypass_w1_q;

always @(posedge clk) begin
    if (!rstn) begin
        hit_bypass_w0_q <= 1'b0;
        hit_bypass_w1_q <= 1'b0;
    end else if (pipeline_enable) begin
        hit_bypass_w0_q <= hit_write_to_same_index_w0;
        hit_bypass_w1_q <= hit_write_to_same_index_w1;
    end
end


assign tag_0_r   = refill_bypass_w0_q ? refill_bypass_tag_q : tag_way_0_q;
assign tag_1_r   = refill_bypass_w1_q ? refill_bypass_tag_q : tag_way_1_q;
assign valid_0_r = refill_bypass_w0_q ? 1'b1 : valid_way_0_q;
assign valid_1_r = refill_bypass_w1_q ? 1'b1 : valid_way_1_q;
assign dirty_0_r = refill_bypass_w0_q ? refill_bypass_wen_q
                 : hit_bypass_w0_q    ? 1'b1
                 :                      dirty_way_0_q;
assign dirty_1_r = refill_bypass_w1_q ? refill_bypass_wen_q
                 : hit_bypass_w1_q    ? 1'b1
                 :                      dirty_way_1_q;
assign lru_0_r   = refill_bypass_w0_q ? 1'b0
                 : refill_bypass_w1_q ? 1'b1
                 : hit_bypass_w0_q    ? 1'b0
                 : hit_bypass_w1_q    ? 1'b1
                 :                      lru_way_0_q;
assign lru_1_r   = refill_bypass_w1_q ? 1'b0
                 : refill_bypass_w0_q ? 1'b1
                 : hit_bypass_w1_q    ? 1'b0
                 : hit_bypass_w0_q    ? 1'b1
                 :                      lru_way_1_q;

endmodule
