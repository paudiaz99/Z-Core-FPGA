# ==============================================================================
# Z-Core SDC Constraints
# Target: Intel MAX 10 (10M50DAF484C7G) on DE10-Lite
# Clock: 50 MHz (20 ns period)
# ==============================================================================

# ==============================================================================
# 1. Clock Definition
# ==============================================================================
create_clock -name MAX10_CLK1_50 -period 20.000 [get_ports {MAX10_CLK1_50}]
derive_clock_uncertainty

# ==============================================================================
# 2. I/O Constraints
# ==============================================================================

# UART — asynchronous serial, metastability handled internally
set_false_path -from [get_ports {uart_rx}]
set_output_delay -clock MAX10_CLK1_50 -max 8.0 [get_ports {uart_tx}]
set_output_delay -clock MAX10_CLK1_50 -min 0.0 [get_ports {uart_tx}]

# GPIO — low-speed bidirectional I/O
set_false_path -from [get_ports {gpio_pins[*]}]
set_output_delay -clock MAX10_CLK1_50 -max 8.0 [get_ports {gpio_pins[*]}]
set_output_delay -clock MAX10_CLK1_50 -min 0.0 [get_ports {gpio_pins[*]}]

# LEDs — display only, no timing requirement
set_false_path -to [get_ports {LEDR[*]}]

# Reset — asynchronous
set_false_path -from [get_ports {KEY[*]}]
