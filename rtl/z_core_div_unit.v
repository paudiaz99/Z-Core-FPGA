// Division Unit based on Patterson & Hennessy "Computer Organization and Design"
// RISC-V Edition, Figure 3.8 - First version of the division hardware
//
// Supports both signed and unsigned division per RISC-V M Extension:
//   - DIVU/REMU: Unsigned division/remainder
//   - DIV/REM:   Signed division/remainder
//
// Algorithm (unsigned core):
// 1. Initialize remainder with dividend (lower 32 bits), divisor in upper 32 bits
// 2. For 33 iterations:
//    a. Subtract divisor from remainder
//    b. If result >= 0: shift quotient left, insert 1
//       If result < 0: restore remainder, shift quotient left, insert 0
//    c. Shift divisor right by 1
// 3. Apply sign correction for signed operations
//
// Signed division handling:
// - Convert operands to absolute values
// - Perform unsigned division
// - Quotient sign: negative if operand signs differ
// - Remainder sign: same sign as dividend

module z_core_div_unit(
    // Input signals
    input clk,
    input rstn,
    input [31:0] dividend,
    input [31:0] divisor,
    input div_start,
    input is_signed,        // 1 = signed (DIV/REM), 0 = unsigned (DIVU/REMU)
    input quotient_or_rem,  // 1 = quotient, 0 = remainder

    output reg div_done,
    output reg div_running,
    output reg [31:0] div_result
);

// States
localparam IDLE   = 3'd0;
localparam SUB    = 3'd1;
localparam SHIFT  = 3'd2;
localparam RESULT = 3'd3;
localparam DONE   = 3'd4;

reg [2:0] state;
reg [31:0] quotient;
reg [64:0] remainder;      // 65 bits: extra bit for borrow detection
reg [64:0] temp_remainder; // 65 bits
reg [64:0] divisor_reg;    // 65 bits
reg [5:0] iteration_count;

// Sign tracking for signed division
reg dividend_neg;    // Original dividend was negative
reg divisor_neg;       // Original divisor was negative
reg is_signed_op;      // Latched signed flag
reg quotient_or_rem_reg; // Latched quotient/remainder selection

// Absolute value of operands for signed division
wire [31:0] abs_dividend = (is_signed && dividend[31]) ? (~dividend + 1) : dividend;
wire [31:0] abs_divisor  = (is_signed && divisor[31])  ? (~divisor + 1)  : divisor;

// Calculate final results with sign correction
wire quotient_neg = dividend_neg ^ divisor_neg;  // Different signs = negative quotient
wire [31:0] signed_quotient  = quotient_neg ? (~quotient + 1) : quotient;
wire [31:0] signed_remainder = dividend_neg ? (~remainder[31:0] + 1) : remainder[31:0];

always @(posedge clk) begin
    if (~rstn) begin
        // Reset all registers
        state <= IDLE;
        div_done <= 1'b0;
        div_running <= 1'b0;
        div_result <= 32'b0;
        quotient <= 32'b0;
        remainder <= 65'b0;
        temp_remainder <= 65'b0;
        divisor_reg <= 65'b0;
        iteration_count <= 6'b0;
        dividend_neg <= 1'b0;
        divisor_neg <= 1'b0;
        is_signed_op <= 1'b0;
        quotient_or_rem_reg <= 1'b0;
    end else begin
        case (state)
            IDLE: begin
                div_done <= 1'b0;
                if (div_start) begin
                    // Latch inputs that might change during division
                    is_signed_op <= is_signed;
                    quotient_or_rem_reg <= quotient_or_rem;
                    
                    // Track original signs for signed operations
                    dividend_neg <= is_signed & dividend[31];
                    divisor_neg  <= is_signed & divisor[31];
                    
                    // Initialize for new division using absolute values
                    // Remainder: absolute dividend in lower 32 bits (65-bit with leading 0)
                    remainder <= {33'b0, abs_dividend};
                    // Divisor: absolute divisor in upper 32 bits (65-bit with leading 0)
                    divisor_reg <= {1'b0, abs_divisor, 32'b0};
                    quotient <= 32'b0;
                    iteration_count <= 6'b0;
                    div_running <= 1'b1;
                    state <= SUB;
                end
            end

            SUB: begin
                // Step 1: Subtract divisor from remainder
                temp_remainder <= remainder;
                remainder <= remainder - divisor_reg;
                state <= SHIFT;
            end

            SHIFT: begin
                // Step 2: Check borrow bit (bit 64) for unsigned subtraction
                if (remainder[64]) begin
                    // Borrow occurred: Restore remainder, shift quotient left with 0
                    remainder <= temp_remainder;
                    quotient <= {quotient[30:0], 1'b0};
                end else begin
                    // No borrow: Keep remainder, shift quotient left with 1
                    quotient <= {quotient[30:0], 1'b1};
                end

                // Step 3: Shift divisor right by 1
                divisor_reg <= {1'b0, divisor_reg[64:1]};

                // Increment iteration counter
                iteration_count <= iteration_count + 1;

                // Check if done (33 iterations for 32-bit division)
                if (iteration_count == 6'd32) begin
                    state <= RESULT;
                end else begin
                    state <= SUB;
                end
            end

            RESULT: begin
                // Set the result with sign correction - this happens BEFORE div_done
                if (quotient_or_rem_reg) begin
                    // Quotient: negate if operand signs differ
                    div_result <= is_signed_op ? signed_quotient : quotient;
                end else begin
                    // Remainder: same sign as dividend
                    div_result <= is_signed_op ? signed_remainder : remainder[31:0];
                end
                state <= DONE;
            end

            DONE: begin
                // Signal completion AFTER result is stable
                div_done <= 1'b1;
                div_running <= 1'b0;
                
                // Immediately return to IDLE to be ready for next division
                state <= IDLE;
            end
        endcase
    end
end

endmodule