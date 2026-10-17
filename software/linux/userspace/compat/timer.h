#ifndef ZCORE_LINUX_TIMER_H
#define ZCORE_LINUX_TIMER_H
#include <stdint.h>
#include <time.h>
#define CPU_HZ 50000000UL
static inline uint32_t cycles32(void) {
    uint32_t lo; __asm__ volatile("csrr %0, cycle" : "=r"(lo)); return lo;
}
static inline uint64_t cycles64(void) {
    uint32_t hi,lo,again;
    do { __asm__ volatile("csrr %0, cycleh" : "=r"(hi));
         __asm__ volatile("csrr %0, cycle" : "=r"(lo));
         __asm__ volatile("csrr %0, cycleh" : "=r"(again)); } while (hi!=again);
    return ((uint64_t)hi<<32)|lo;
}
static inline uint64_t cycles_to_us(uint64_t n) { return n/50; }
static inline uint64_t cycles_to_ms(uint64_t n) { return n/50000; }
static inline void delay_us(uint32_t n) {
    struct timespec t={n/1000000,(n%1000000)*1000L}; nanosleep(&t,0);
}
static inline void delay_ms(uint32_t n) { delay_us(n*1000); }
static inline void delay_cycles(uint64_t n) { delay_us(n/50); }
#endif
