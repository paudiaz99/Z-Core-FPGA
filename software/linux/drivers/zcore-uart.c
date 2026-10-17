// SPDX-License-Identifier: GPL-2.0-only
/* Z-Core AXI UART: no IRQ output, one-byte receive holding register. */
#include <linux/console.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>

#define TX 0x00
#define RX 0x04
#define STATUS 0x08
#define DIVISOR 0x10
#define TX_EMPTY BIT(0)
#define RX_VALID BIT(2)
#define POLL_NS 5000000
#define TX_BUDGET_NS 1500000
#define TX_BURST 16

static struct uart_port zport;
static struct hrtimer poll_timer;
static bool rx_enabled;
static void put_char(struct uart_port *p, unsigned char c)
{
	unsigned int timeout = 1000000;
	while (!(readl(p->membase + STATUS) & TX_EMPTY) && --timeout)
		cpu_relax();
	if (timeout)
		writel(c, p->membase + TX);
}
static void early_write(struct console *con, const char *s, unsigned int n)
{
	struct earlycon_device *dev = con->data;
	uart_console_write(&dev->port, s, n, put_char);
}
static int __init early_setup(struct earlycon_device *dev, const char *opt)
{
	if (!dev->port.membase)
		return -ENODEV;
	dev->con->write = early_write;
	return 0;
}
OF_EARLYCON_DECLARE(zcore, "zcore,uart", early_setup);

static bool tx_ready_before(struct uart_port *p, ktime_t deadline)
{
	while (ktime_before(ktime_get(), deadline)) {
		if (readl(p->membase + STATUS) & TX_EMPTY)
			return true;
		cpu_relax();
	}
	return false;
}
static void transmit(struct uart_port *p)
{
	u8 ch;
	ktime_t deadline;
	if (!p->x_char && (uart_tx_stopped(p) ||
			  kfifo_is_empty(&p->state->port.xmit_fifo)))
		return;
	deadline = ktime_add_ns(ktime_get(), TX_BUDGET_NS);
	uart_port_tx_limited(p, ch, TX_BURST, tx_ready_before(p, deadline),
		writel(ch, p->membase + TX), ({}));
}
static enum hrtimer_restart poll(struct hrtimer *timer)
{
	unsigned long flags;
	bool received = false;
	uart_port_lock_irqsave(&zport, &flags);
	if (rx_enabled && (readl(zport.membase + STATUS) & RX_VALID)) {
		u8 c = readl(zport.membase + RX);
		zport.icount.rx++;
		if (!uart_handle_sysrq_char(&zport, c))
			tty_insert_flip_char(&zport.state->port, c, TTY_NORMAL);
		received = true;
	}
	transmit(&zport);
	uart_port_unlock_irqrestore(&zport, flags);
	if (received)
		tty_flip_buffer_push(&zport.state->port);
	hrtimer_forward_now(timer, ns_to_ktime(POLL_NS));
	return HRTIMER_RESTART;
}
static unsigned int tx_empty(struct uart_port *p)
{
	return readl(p->membase + STATUS) & TX_EMPTY ? TIOCSER_TEMT : 0;
}
static void noop(struct uart_port *p) {}
static void set_mctrl(struct uart_port *p, unsigned int m) {}
static unsigned int get_mctrl(struct uart_port *p)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}
static void stop_rx(struct uart_port *p) { rx_enabled = false; }
static int startup(struct uart_port *p)
{
	rx_enabled = true;
	hrtimer_start(&poll_timer, ns_to_ktime(POLL_NS), HRTIMER_MODE_REL);
	return 0;
}
static void shutdown(struct uart_port *p) { hrtimer_cancel(&poll_timer); }
static void set_termios(struct uart_port *p, struct ktermios *termios,
			const struct ktermios *old)
{
	unsigned long flags;
	termios->c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
	termios->c_cflag |= CS8 | CLOCAL;
	tty_termios_encode_baud_rate(termios, 115200, 115200);
	uart_port_lock_irqsave(p, &flags);
	writel(27, p->membase + DIVISOR);
	uart_update_timeout(p, termios->c_cflag, 115200);
	uart_port_unlock_irqrestore(p, flags);
}
static const char *type(struct uart_port *p) { return "zcore-uart"; }
static int request_port(struct uart_port *p) { return 0; }
static void config_port(struct uart_port *p, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		p->type = PORT_GENERIC;
}
static int verify_port(struct uart_port *p, struct serial_struct *s)
{
	return -EINVAL;
}
static const struct uart_ops ops = {
	.tx_empty = tx_empty, .set_mctrl = set_mctrl, .get_mctrl = get_mctrl,
	.stop_tx = noop, .start_tx = transmit, .stop_rx = stop_rx,
	.startup = startup, .shutdown = shutdown, .set_termios = set_termios,
	.type = type, .release_port = noop, .request_port = request_port,
	.config_port = config_port, .verify_port = verify_port,
};
static struct uart_driver driver;
static void console_write(struct console *co, const char *s, unsigned int n)
{
	unsigned long flags;
	bool locked = false;
	local_irq_save(flags);
	if (oops_in_progress)
		locked = uart_port_trylock(&zport);
	else {
		uart_port_lock(&zport);
		locked = true;
	}
	uart_console_write(&zport, s, n, put_char);
	if (locked)
		uart_port_unlock(&zport);
	local_irq_restore(flags);
}
static int console_setup(struct console *co, char *options)
{
	if (!zport.membase)
		return -ENODEV;
	return uart_set_options(&zport, co, 115200, 'n', 8, 'n');
}
static struct console console = {
	.name = "ttyZC", .write = console_write, .device = uart_console_device,
	.setup = console_setup, .flags = CON_PRINTBUFFER, .index = -1,
	.data = &driver,
};
static struct uart_driver driver = {
	.owner = THIS_MODULE, .driver_name = "zcore-uart", .dev_name = "ttyZC",
	.nr = 1, .cons = &console,
};
static int probe(struct platform_device *pdev)
{
	struct resource *res;
	zport.membase = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(zport.membase))
		return PTR_ERR(zport.membase);
	zport.mapbase = res->start;
	zport.dev = &pdev->dev;
	zport.iotype = UPIO_MEM32;
	zport.uartclk = 50000000;
	zport.fifosize = 1;
	zport.flags = UPF_BOOT_AUTOCONF;
	zport.ops = &ops;
	hrtimer_init(&poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	poll_timer.function = poll;
	return uart_add_one_port(&driver, &zport);
}
static void remove_port(struct platform_device *pdev)
{
	uart_remove_one_port(&driver, &zport);
}
static const struct of_device_id matches[] = {
	{ .compatible = "zcore,uart" }, {}
};
static struct platform_driver platform = {
	.probe = probe, .remove = remove_port,
	.driver = { .name = "zcore-uart", .of_match_table = matches },
};
static int __init init(void)
{
	int ret = uart_register_driver(&driver);
	if (ret)
		return ret;
	ret = platform_driver_register(&platform);
	if (ret)
		uart_unregister_driver(&driver);
	return ret;
}
device_initcall(init);
MODULE_LICENSE("GPL");
