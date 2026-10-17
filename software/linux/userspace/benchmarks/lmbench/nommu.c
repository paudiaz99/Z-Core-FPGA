/* SPDX-License-Identifier: GPL-2.0-only */
/* Sequential lmbench harness: no fork, single worker only. */
#include "bench.h"
static uint64 wall_us(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64)t.tv_sec*1000000+t.tv_nsec/1000;
}
void benchmp(benchmp_f init,benchmp_f run,benchmp_f clean,int enough,
             int parallel,int warmup,int repetitions,void *cookie) {
    static result_t results;
    iter_t n=1;
    uint64 elapsed;
    if(parallel!=1) { fprintf(stderr,"no-MMU lmbench variant supports -P 1 only\n"); exit(1); }
    fprintf(stderr,"lmbench no-MMU variant: sequential worker, unchanged test kernel\n");
    if(repetitions<1 || repetitions>TRIES) { fprintf(stderr,"-N must be 1..%d\n",TRIES); exit(1); }
    if(init) init(0,cookie);
    do {
        if(init) init(n,cookie);
        uint64 t=wall_us(); run(n,cookie); elapsed=wall_us()-t;
        if(clean) clean(n,cookie);
        if(elapsed >= (uint64)(enough>0?enough:50000)) break;
        if(n>100000000) break;
        n*=2;
    } while(1);
    if(warmup>0) {
        uint64 end=wall_us()+(uint64)warmup*1000;
        do { if(init) init(n,cookie); run(n,cookie); if(clean) clean(n,cookie); } while(wall_us()<end);
    }
    insertinit(&results);
    for(int i=0;i<repetitions;i++) {
        if(init) init(n,cookie);
        uint64 t=wall_us(); run(n,cookie); elapsed=wall_us()-t;
        if(clean) clean(n,cookie);
        insertsort(elapsed,n,&results);
    }
    set_results(&results);
    if(clean) clean(0,cookie);
}
