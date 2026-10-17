/*
Copyright (c) 2025–2026 Pau Díaz Cuesta

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

module z_core_write_buffer#(
     parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter ENTRIES = 8
)(
    // Inputs
    input clk,
    input rstn,
    
    input [DATA_WIDTH-1:0] write_back_data,
    input [ADDR_WIDTH-1:0] write_back_addr,
    input [3:0] write_back_strb,

    input write_enable, // This will be triggered by control unit
    input read_enable, // Read enable represents when the bus is free and we can write to memory. As of now, it will be simulated.

    input [ADDR_WIDTH-1:0] load_address, // Every load will check wb
    input load_check,

    // Outputs
    output reg [DATA_WIDTH-1:0] wb_out_data,
    output reg [ADDR_WIDTH-1:0] wb_out_address,
    output reg [3:0]            wb_out_strb,
    output reg wb_out_valid,
    output full_out,
    output empty_out,

    output reg [DATA_WIDTH-1:0] load_forward_out,
    output reg load_forward_valid_out,
    output reg [ADDR_WIDTH-1:0] load_forward_addr_out,
    output reg [3:0] load_forward_strb_out,
    output reg address_pending_out

);

    reg [DATA_WIDTH-1:0] write_buffer_data [ENTRIES-1:0];
    reg [ADDR_WIDTH-1:0] write_buffer_address [ENTRIES-1:0];
    reg write_buffer_valid [ENTRIES-1:0];

    reg [$clog2(ENTRIES)-1:0] read_pointer;
    reg [$clog2(ENTRIES)-1:0] write_pointer;
    wire [$clog2(ENTRIES)-1:0] tail_pointer = (write_pointer == 0) ? {$clog2(ENTRIES){1'b1}} : (write_pointer - 1'b1);
    reg [$clog2(ENTRIES):0] elem_count;

    wire empty = elem_count == {$clog2(ENTRIES){1'b0}};
    wire full = elem_count == ENTRIES;

    integer i, j, k, m;

    wire merge_write = write_buffer_address[tail_pointer] == write_back_addr && write_buffer_valid[tail_pointer];
    wire pop = read_enable && !empty;
    wire tail_being_popped = pop && (elem_count == 1);
    wire do_merge = merge_write && !tail_being_popped;

    wire write_accepted = write_enable && (!full || read_enable || merge_write);

    always @(posedge clk) begin
        if (~rstn) begin
            for(i = 0; i < ENTRIES; i = i + 1) begin
                write_buffer_data[i] <= {DATA_WIDTH{1'b0}};
                write_buffer_address[i] <= {ADDR_WIDTH{1'b0}};
                write_buffer_valid[i] <= 1'b0;
            end
            read_pointer   <= {$clog2(ENTRIES){1'b0}};
            write_pointer  <= {$clog2(ENTRIES){1'b0}};
            elem_count     <= 0;
            wb_out_data    <= {DATA_WIDTH{1'b0}};
            wb_out_address <= {ADDR_WIDTH{1'b0}};
            wb_out_valid   <= 1'b0;
        end else begin
            wb_out_valid <= 1'b0; // Default to 1'b0

            if(read_enable || write_enable) begin
                if(read_enable) begin
                    wb_out_data    <= (empty && write_enable) ? write_back_data : write_buffer_data[read_pointer];
                    wb_out_address <= (empty && write_enable) ? write_back_addr : write_buffer_address[read_pointer];
                    wb_out_strb    <= 4'b1111;
                    wb_out_valid   <= (empty && write_enable) ? 1'b1            : write_buffer_valid[read_pointer];
                    read_pointer   <= (empty && ~write_enable) ? read_pointer : read_pointer + 1'b1;
                    elem_count     <= ((write_enable && ~merge_write) || (empty && ~write_enable)) ? elem_count : elem_count - 1'b1;
                    write_buffer_valid[read_pointer] <= 1'b0;
                end 
                if(write_enable) begin
                    if(do_merge) begin
                        write_buffer_data[tail_pointer]    <= write_back_data;
                        write_buffer_address[tail_pointer] <= write_back_addr;
                        write_buffer_valid[tail_pointer]   <= 1'b1;
                        write_pointer                        <= write_pointer;
                        // Merging adds no entry, but a simultaneous head
                        // pop still removes one (the tail is not popped).
                        elem_count                           <= pop ? elem_count - 1'b1 : elem_count;
                    end else begin
                        write_buffer_data[write_pointer]    <= ((empty && read_enable) || (full && ~read_enable)) ? write_buffer_data[write_pointer] : write_back_data;
                        write_buffer_address[write_pointer] <= ((empty && read_enable) || (full && ~read_enable)) ? write_buffer_address[write_pointer] : write_back_addr;
                        write_buffer_valid[write_pointer]   <= ((empty && read_enable) || (full && ~read_enable)) ? write_buffer_valid[write_pointer] : 1'b1;
                        write_pointer                      <= (full && ~read_enable) ? write_pointer : write_pointer + 1'b1;
                        elem_count                         <= (read_enable || (full && ~read_enable)) ? elem_count : elem_count + 1'b1;
                    end
                end
            end else begin
                read_pointer <= read_pointer;
                write_pointer <= write_pointer;
                elem_count <= elem_count;
                wb_out_valid <= 1'b0;
                wb_out_data <= {DATA_WIDTH{1'b0}};
                wb_out_address <= {ADDR_WIDTH{1'b0}};
                wb_out_strb <= 4'b0000;
            end
        end

    end

    // Forwarding: This is what makes write buffer expensive
    always @(*) begin
        load_forward_out = {DATA_WIDTH{1'b0}};
        load_forward_addr_out = {ADDR_WIDTH{1'b0}};
        load_forward_valid_out = 1'b0;
        load_forward_strb_out = 4'b0000;

        if (load_check) begin
            for (k = 0; k < ENTRIES; k = k + 1) begin
                j = (read_pointer + k) % ENTRIES;
                if (k < elem_count &&
                    write_buffer_valid[j] &&
                    write_buffer_address[j] == load_address) begin
                    load_forward_out = write_buffer_data[j];
                    load_forward_addr_out = write_buffer_address[j];
                    load_forward_strb_out = 4'b1111;
                    load_forward_valid_out = 1'b1;
                end
            end
            if (write_accepted && write_back_addr == load_address) begin
                load_forward_out = write_back_data;
                load_forward_addr_out = write_back_addr;
                load_forward_strb_out = 4'b1111;
                load_forward_valid_out = 1'b1;
            end
        end
    end

    always @(*) begin
        address_pending_out = 1'b0;
        if (load_check) begin
            for (m = 0; m < ENTRIES; m = m + 1) begin
                if (write_buffer_valid[m] && write_buffer_address[m] == load_address)
                    address_pending_out = 1'b1;
            end
            if (write_enable && (!full || merge_write) && write_back_addr == load_address)
                address_pending_out = 1'b1;
        end
    end

    assign full_out = full;
    assign empty_out = empty;


endmodule
