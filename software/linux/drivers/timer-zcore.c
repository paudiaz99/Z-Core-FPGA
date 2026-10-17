// SPDX-License-Identifier: GPL-2.0-only
/* Single-hart Z-Core timer, MTIP (cause 7), 50 MHz. */
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>
#include <asm/csr.h>
#include <asm/timex.h>

static void __iomem *base;
static DEFINE_PER_CPU(struct clock_event_device, events);
static u64 notrace cycles(void)
{
	u32 hi, lo;
	do {
		hi = readl_relaxed(base + 4);
		lo = readl_relaxed(base);
	} while (hi != readl_relaxed(base + 4));
	return ((u64)hi << 32) | lo;
}
static void compare(u64 value)
{
	writel(~0U, base + 12);
	writel(value >> 32, base + 16);
	writel((u32)value, base + 12);
}
static u64 read_clock(struct clocksource *cs) { return cycles(); }
static struct clocksource source = {
	.name = "zcore-timer", .rating = 300, .read = read_clock,
	.mask = CLOCKSOURCE_MASK(64), .flags = CLOCK_SOURCE_IS_CONTINUOUS,
};
static int shutdown(struct clock_event_device *ce)
{
	csr_clear(CSR_IE, IE_TIE);
	compare(~0ULL);
	return 0;
}
static int next_event(unsigned long delta, struct clock_event_device *ce)
{
	u64 target = cycles() + delta;
	compare(target);
	csr_set(CSR_IE, IE_TIE);
	return cycles() >= target ? -ETIME : 0;
}
static irqreturn_t interrupt(int irq, void *data)
{
	struct clock_event_device *ce = this_cpu_ptr(&events);
	csr_clear(CSR_IE, IE_TIE);
	compare(~0ULL);
	ce->event_handler(ce);
	return IRQ_HANDLED;
}
static int __init timer_init(struct device_node *np)
{
	struct clock_event_device *ce = this_cpu_ptr(&events);
	unsigned int irq = irq_of_parse_and_map(np, 0);
	int ret;
	if (!irq)
		return -EINVAL;
	base = of_iomap(np, 0);
	if (!base)
		return -ENOMEM;
	compare(~0ULL);
	writel(0x0b, base + 8);
	clint_time_val = base;
	ret = request_percpu_irq(irq, interrupt, "zcore-timer", &events);
	if (ret)
		goto fail;
	ret = clocksource_register_hz(&source, riscv_timebase);
	if (ret) {
		free_percpu_irq(irq, &events);
		goto fail;
	}
	sched_clock_register(cycles, 64, riscv_timebase);
	ce->name = "zcore-timer";
	ce->rating = 300;
	ce->features = CLOCK_EVT_FEAT_ONESHOT;
	ce->set_next_event = next_event;
	ce->set_state_shutdown = shutdown;
	ce->set_state_oneshot = shutdown;
	ce->cpumask = cpumask_of(0);
	ce->irq = irq;
	clockevents_config_and_register(ce, riscv_timebase, 500, 0x7fffffff);
	enable_percpu_irq(irq, 0);
	return 0;
fail:
	writel(0, base + 8);
	clint_time_val = NULL;
	iounmap(base);
	return ret;
}
TIMER_OF_DECLARE(zcore_timer, "zcore,timer", timer_init);
