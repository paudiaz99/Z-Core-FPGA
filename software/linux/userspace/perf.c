/* SPDX-License-Identifier: GPL-2.0-only */
/* Small no-MMU counting frontend to Linux perf_event_open, not upstream perf. */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/perf_event.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const struct { const char *name; unsigned csr; } events[] = {
    {"cycles",0}, {"instructions",2}, {"icache-hits",3}, {"dcache-hits",4},
    {"load-requests",5}, {"store-requests",6}, {"branch-misses",7},
    {"pipeline-flushes",8}, {"icache-misses",9}, {"memory-fetches",10},
    {"uncached-accesses",11}, {"dcache-misses",12}
};
static volatile sig_atomic_t interrupted;
static void on_signal(int sig) { interrupted = sig; }
static void usage(void) {
    puts("Z-Core count-only perf frontend (Linux perf_event_open)\n"
         "  perf list\n  perf stat [-a] [-u] [-e events] [--timeout milliseconds] -- command [args]\n"
         "Events: cycles,instructions,icache-hits,dcache-hits,load-requests,\n"
         "store-requests,branch-misses,pipeline-flushes,icache-misses,\n"
         "memory-fetches,uncached-accesses,dcache-misses (or cpu/event=N/, rHEX).\n"
         "-u counts User-mode only; no hardware sampling. Without -a, the\n"
         "task measurement includes this frontend's launch/wait overhead.");
}
int main(int argc, char **argv) {
    int selected[32], fds[32], n=0, all=0, user_only=0, timeout=0, pos=2, status=0;
    char *spec=NULL;
    if (argc<2 || !strcmp(argv[1],"--help")) { usage(); return argc<2; }
    if (!strcmp(argv[1],"list")) {
        for (unsigned i=0;i<sizeof(events)/sizeof(events[0]);i++)
            printf("%-20s cpu/event=%u/\n",events[i].name,events[i].csr);
        return 0;
    }
    if (strcmp(argv[1],"stat")) { usage(); return 1; }
    while (pos<argc) {
        if (!strcmp(argv[pos],"--")) { pos++; break; }
        if (!strcmp(argv[pos],"-a")) { all=1; pos++; }
        else if (!strcmp(argv[pos],"-u") || !strcmp(argv[pos],"--user")) { user_only=1; pos++; }
        else if (!strcmp(argv[pos],"-e") && pos+1<argc) { spec=argv[pos+1]; pos+=2; }
        else if (!strcmp(argv[pos],"--timeout") && pos+1<argc) { timeout=atoi(argv[pos+1]); pos+=2; }
        else if (argv[pos][0]=='-') { usage(); return 1; }
        else break;
    }
    if (pos==argc && timeout<=0) { usage(); return 1; }
    char *copy=strdup(spec?spec:"cycles,instructions,branch-misses"), *save;
    for (char *s=strtok_r(copy,",",&save);s;s=strtok_r(NULL,",",&save)) {
        int found=-1; unsigned raw;
        for (unsigned i=0;i<sizeof(events)/sizeof(events[0]);i++)
            if (!strcmp(s,events[i].name)) found=i;
        if (sscanf(s,"cpu/event=%u/",&raw)==1 || (s[0]=='r' && sscanf(s+1,"%x",&raw)==1))
            for (unsigned i=0;i<sizeof(events)/sizeof(events[0]);i++)
                if (events[i].csr==raw) found=i;
        if (found<0 || n==32) { fprintf(stderr,"Unsupported event: %s\n",s); free(copy); return 1; }
        selected[n++]=found;
    }
    free(copy);
    if (!n) return 1;
    int opened=0;
    for (int i=0;i<n;i++) {
        struct perf_event_attr attr={0};
        attr.size=sizeof(attr); attr.type=PERF_TYPE_RAW;
        attr.config=events[selected[i]].csr; attr.disabled=1;
        attr.inherit=!all; attr.exclude_kernel=user_only;
        attr.read_format=PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING;
        fds[i]=syscall(__NR_perf_event_open,&attr,all?-1:0,all?0:-1,-1,0);
        if (fds[i]<0) {
            fprintf(stderr,"perf_event_open(%s): %s\n",events[selected[i]].name,strerror(errno));
            for (int j=0;j<opened;j++) close(fds[j]);
            return 1;
        }
        opened++;
    }
    struct timespec begin,end;
    clock_gettime(CLOCK_MONOTONIC,&begin);
    signal(SIGINT,on_signal); signal(SIGTERM,on_signal);
    for (int i=0;i<n;i++)
        if (ioctl(fds[i],PERF_EVENT_IOC_ENABLE,0)<0) { perror("enable"); return 1; }
    if (pos<argc) {
        pid_t child=vfork();
        if (child==0) { execvp(argv[pos],argv+pos); _exit(127); }
        if (child<0) { perror("vfork"); status=1; }
        else while (waitpid(child,&status,0)<0 && errno==EINTR) {
            if (interrupted) kill(child,interrupted);
        }
    } else {
        struct timespec delay={timeout/1000,(timeout%1000)*1000000L};
        while (!interrupted && nanosleep(&delay,&delay)<0 && errno==EINTR) {}
    }
    clock_gettime(CLOCK_MONOTONIC,&end);
    int failed=0;
    for (int i=0;i<n;i++)
        if (ioctl(fds[i],PERF_EVENT_IOC_DISABLE,0)<0) {
            perror("disable"); failed=1;
        }
    for (int i=0;i<n;i++) {
        struct { uint64_t count,enabled,running; } result;
        if (read(fds[i],&result,sizeof(result))!=sizeof(result)) {
            perror("counter read"); failed=1;
        } else fprintf(stderr,"%20llu  %-20s (running %llu / enabled %llu ns)\n",
            (unsigned long long)result.count,events[selected[i]].name,
            (unsigned long long)result.running,(unsigned long long)result.enabled);
        close(fds[i]);
    }
    long long ns=(end.tv_sec-begin.tv_sec)*1000000000LL+end.tv_nsec-begin.tv_nsec;
    fprintf(stderr,"%lld.%03lld seconds elapsed\n",ns/1000000000LL,(ns/1000000LL)%1000);
    if (failed) return 1;
    if (pos<argc) return WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
    return status;
}
