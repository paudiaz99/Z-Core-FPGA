/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdint.h>
#include <stdio.h>
static uint64_t counter(unsigned n) {
    uint32_t hi,lo,again;
#define C(n) case n: do { __asm__ volatile("csrr %0, %1" : "=r"(hi) : "i"(0xc80+n)); \
    __asm__ volatile("csrr %0, %1" : "=r"(lo) : "i"(0xc00+n)); \
    __asm__ volatile("csrr %0, %1" : "=r"(again) : "i"(0xc80+n)); } while(hi!=again); break
    switch(n) { C(0); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10); C(11); C(12); default:return 0; }
    return ((uint64_t)hi<<32)|lo;
}
int main(void) {
    static const char *names[]={"cycles","time (not implemented)","instructions","icache-hits",
        "dcache-hits","load-requests","store-requests","branch-misses","pipeline-flushes",
        "icache-misses","memory-fetches","uncached-accesses","dcache-misses"};
    puts("System-wide counters since reset (sequential 64-bit snapshots)");
    for(unsigned n=0;n<13;n++) if(n!=1)
        printf("%-20s %llu\n",names[n],(unsigned long long)counter(n));
    return 0;
}
