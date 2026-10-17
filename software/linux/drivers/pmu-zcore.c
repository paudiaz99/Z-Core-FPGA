// SPDX-License-Identifier: GPL-2.0-only
/* Fixed total and User-mode counters: counting only. */
#include <linux/init.h>
#include <linux/perf_event.h>
#include <asm/csr.h>

static struct pmu zpmu;

static u64 counter(int idx, bool user_only)
{
	u32 hi, lo, again;
#define READ_CASE(n) case n: do { \
	if (user_only) { hi = csr_read(0xfe0 + n); lo = csr_read(0xfc0 + n); \
		again = csr_read(0xfe0 + n); } \
	else { hi = csr_read(0xb80 + n); lo = csr_read(0xb00 + n); \
		again = csr_read(0xb80 + n); } \
	} while (hi != again); break
	switch (idx) {
	READ_CASE(0); READ_CASE(2); READ_CASE(3); READ_CASE(4);
	READ_CASE(5); READ_CASE(6); READ_CASE(7); READ_CASE(8);
	READ_CASE(9); READ_CASE(10); READ_CASE(11); READ_CASE(12);
	default: return 0;
	}
#undef READ_CASE
	return ((u64)hi << 32) | lo;
}

static int event_init(struct perf_event *e)
{
	int idx = -1;
	if (e->attr.type == PERF_TYPE_HARDWARE) {
		switch (e->attr.config) {
		case PERF_COUNT_HW_CPU_CYCLES: idx = 0; break;
		case PERF_COUNT_HW_INSTRUCTIONS: idx = 2; break;
		case PERF_COUNT_HW_BRANCH_MISSES: idx = 7; break;
		default: return -ENOENT;
		}
	} else if (e->attr.type == zpmu.type) {
		if (e->attr.config == 0 || (e->attr.config >= 2 && e->attr.config <= 12))
			idx = e->attr.config;
	} else {
		return -ENOENT;
	}
	if (idx < 0) return -EINVAL;
	if (is_sampling_event(e) || e->attr.exclude_user || e->attr.exclude_hv || e->attr.exclude_idle || e->attr.exclude_host ||
	    e->attr.exclude_guest) return -EOPNOTSUPP;
	e->hw.idx = idx;
	e->hw.state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
	return 0;
}

static void update(struct perf_event *e)
{
	u64 old, now;
	if (e->hw.state & PERF_HES_STOPPED) return;
	do {
		old = local64_read(&e->hw.prev_count);
		now = counter(e->hw.idx, e->attr.exclude_kernel);
	} while (local64_cmpxchg(&e->hw.prev_count, old, now) != old);
	local64_add(now - old, &e->count);
}
static void start(struct perf_event *e, int flags)
{
	local64_set(&e->hw.prev_count, counter(e->hw.idx, e->attr.exclude_kernel));
	e->hw.state = 0;
}
static void stop(struct perf_event *e, int flags)
{
	update(e);
	e->hw.state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
}
static int add(struct perf_event *e, int flags)
{
	e->hw.state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
	if (flags & PERF_EF_START) start(e, flags);
	return 0;
}
static void del(struct perf_event *e, int flags) { stop(e, flags); }

PMU_FORMAT_ATTR(event, "config:0-7");
static struct attribute *format_attrs[] = { &format_attr_event.attr, NULL };
static const struct attribute_group format_group = { .name = "format", .attrs = format_attrs };
#define EVENT(name, num) PMU_EVENT_ATTR_STRING(name, name##_attr, "event=" #num)
EVENT(cycles, 0); EVENT(instructions, 2); EVENT(icache_hits, 3);
EVENT(dcache_hits, 4); EVENT(load_requests, 5); EVENT(store_requests, 6);
EVENT(branch_misses, 7); EVENT(pipeline_flushes, 8); EVENT(icache_misses, 9);
EVENT(memory_fetches, 10); EVENT(uncached_accesses, 11); EVENT(dcache_misses, 12);
static struct attribute *event_attrs[] = {
	&cycles_attr.attr.attr, &instructions_attr.attr.attr,
	&icache_hits_attr.attr.attr, &dcache_hits_attr.attr.attr,
	&load_requests_attr.attr.attr, &store_requests_attr.attr.attr,
	&branch_misses_attr.attr.attr, &pipeline_flushes_attr.attr.attr,
	&icache_misses_attr.attr.attr, &memory_fetches_attr.attr.attr,
	&uncached_accesses_attr.attr.attr, &dcache_misses_attr.attr.attr, NULL
};
static const struct attribute_group event_group = { .name = "events", .attrs = event_attrs };
static const struct attribute_group *groups[] = { &format_group, &event_group, NULL };
static int __init init(void)
{
	int ret;
	csr_write(0x306, 0x1ffd);
	zpmu = (struct pmu) {
		.task_ctx_nr = perf_hw_context,
		.capabilities = PERF_PMU_CAP_NO_INTERRUPT,
		.event_init = event_init, .add = add, .del = del,
		.start = start, .stop = stop, .read = update, .attr_groups = groups,
	};
	ret = perf_pmu_register(&zpmu, "cpu", PERF_TYPE_RAW);
	if (!ret) pr_info("Z-Core PMU: 12 fixed 64-bit counters, counting only, User-mode filtering\n");
	return ret;
}
device_initcall(init);
