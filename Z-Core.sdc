
create_clock -name MAX10_CLK1_50 -period 20.000 [get_ports {MAX10_CLK1_50}]
derive_clock_uncertainty


set_false_path -from [get_ports {uart_rx}]
set_output_delay -clock MAX10_CLK1_50 -max 8.0 [get_ports {uart_tx}]
set_output_delay -clock MAX10_CLK1_50 -min 0.0 [get_ports {uart_tx}]

set_false_path -from [get_ports {gpio_pins[*]}]
set_output_delay -clock MAX10_CLK1_50 -max 8.0 [get_ports {gpio_pins[*]}]
set_output_delay -clock MAX10_CLK1_50 -min 0.0 [get_ports {gpio_pins[*]}]

set_false_path -to [get_ports {LEDR[*]}]

set_false_path -from [get_ports {KEY[*]}]
set_false_path -from [get_ports {host_rstn}]
