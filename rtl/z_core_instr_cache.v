// **************************************************
//      Synchronous Instruction Cache (1-cycle latency, 1 instr/cycle throughput)
//
//  - Single packed array  {tag, data}  -> single inferred BRAM, atomic update
//  - Sideband valid bits in registers   -> cheap clear-on-reset
//  - Same-cycle write-data RAW bypass   -> safe under simultaneous fill+read
//  - Outputs (data_out, valid_d, addr_rd_d) are registered and correspond to
//    the addr_rd presented on the PREVIOUS clock edge.
//
//  The fetch FSM keeps throughput at 1 instr/cycle by presenting addr_rd
//  combinationally from the next-PC mux every cycle; this module returns the
//  result one cycle later, while a new lookup is already in flight.
// **************************************************

module z_core_instr_cache #(
    parameter DATA_WIDTH  = 32,
    parameter ADDR_WIDTH  = 32,
    parameter CACHE_DEPTH = 256
) (
    input  wire                    clk,
    input  wire                    rstn,

    // Read port (lookup result appears 1 cycle later in *_d / data_out)
    input  wire [ADDR_WIDTH-1:0]   addr_rd,
    output wire [ADDR_WIDTH-1:0]   addr_rd_d,
    output wire [DATA_WIDTH-1:0]   data_out,
    output wire                    valid,        // alias of valid_d
    output wire                    cache_hit,    // alias of valid_d
    output wire                    cache_miss,   // alias of !valid_d

    // Write / fill port
    input  wire                    wen,
    input  wire [ADDR_WIDTH-1:0]   addr_wr,
    input  wire [DATA_WIDTH-1:0]   data_in
);

    localparam CACHE_ADDR_WIDTH = $clog2(CACHE_DEPTH);
    localparam CACHE_TAG_WIDTH  = ADDR_WIDTH - 2 - CACHE_ADDR_WIDTH;
    localparam LINE_WIDTH       = CACHE_TAG_WIDTH + DATA_WIDTH;  // {tag, data}

    // --------------------------------------------------------------------
    //  Address decode
    // --------------------------------------------------------------------
    wire [CACHE_TAG_WIDTH-1:0]  tag_rd  = addr_rd[ADDR_WIDTH-1 -: CACHE_TAG_WIDTH];
    wire [CACHE_ADDR_WIDTH-1:0] idx_rd  = addr_rd[CACHE_ADDR_WIDTH+1:2];
    wire [CACHE_TAG_WIDTH-1:0]  tag_wr  = addr_wr[ADDR_WIDTH-1 -: CACHE_TAG_WIDTH];
    wire [CACHE_ADDR_WIDTH-1:0] idx_wr  = addr_wr[CACHE_ADDR_WIDTH+1:2];

    // --------------------------------------------------------------------
    //  Storage
    //    - mem (single packed array, BRAM-inferred): {tag, data}
    //    - valid_arr (registers, cheap to reset)
    // --------------------------------------------------------------------
    (* ramstyle = "no_rw_check" *)
    reg [LINE_WIDTH-1:0] mem [0:CACHE_DEPTH-1];

    reg [CACHE_DEPTH-1:0] valid_arr;

    // --------------------------------------------------------------------
    //  Read pipeline registers (updated every clock)
    // --------------------------------------------------------------------
    reg [LINE_WIDTH-1:0]  line_q;
    reg [CACHE_TAG_WIDTH-1:0] tag_q;
    reg [ADDR_WIDTH-1:0]  addr_rd_q;
    reg                   valid_at_rd_q;

    // RAW bypass: if a fill writes the same index we read this cycle,
    // line_q comes back stale next cycle - latch the freshly-written
    // line and select it via mux.
    reg                   bypass_valid_q;
    reg [LINE_WIDTH-1:0]  bypass_line_q;

    always @(posedge clk) begin
        if (!rstn) begin
            valid_arr      <= {CACHE_DEPTH{1'b0}};
            bypass_valid_q <= 1'b0;
            valid_at_rd_q  <= 1'b0;
            addr_rd_q      <= {ADDR_WIDTH{1'b0}};
            tag_q          <= {CACHE_TAG_WIDTH{1'b0}};
        end else begin
            // ---- Sync read (data appears on outputs next cycle) ----
            line_q         <= mem[idx_rd];
            tag_q          <= tag_rd;
            addr_rd_q      <= addr_rd;
            // valid_at_rd_q reflects whether idx_rd was valid at the
            // moment of the read (with same-cycle fill considered valid)
            valid_at_rd_q  <= valid_arr[idx_rd] || (wen && (idx_wr == idx_rd));

            // ---- RAW bypass capture ----
            bypass_valid_q <= wen && (idx_wr == idx_rd);
            bypass_line_q  <= {tag_wr, data_in};

            // ---- Sync write (fill) ----
            if (wen) begin
                mem[idx_wr]       <= {tag_wr, data_in};
                valid_arr[idx_wr] <= 1'b1;
            end
        end
    end

    // --------------------------------------------------------------------
    //  Output mux (combinational from registered state)
    // --------------------------------------------------------------------
    wire [LINE_WIDTH-1:0]      eff_line = bypass_valid_q ? bypass_line_q : line_q;
    wire [CACHE_TAG_WIDTH-1:0] eff_tag  = eff_line[LINE_WIDTH-1 -: CACHE_TAG_WIDTH];
    wire [DATA_WIDTH-1:0]      eff_data = eff_line[DATA_WIDTH-1:0];

    wire valid_d = valid_at_rd_q && (eff_tag == tag_q);

    assign data_out   = eff_data;
    assign addr_rd_d  = addr_rd_q;
    assign valid      = valid_d;
    assign cache_hit  = valid_d;
    assign cache_miss = !valid_d;

endmodule
