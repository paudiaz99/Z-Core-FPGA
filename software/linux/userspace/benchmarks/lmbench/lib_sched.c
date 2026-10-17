#include "bench.h"


#if defined(HAVE_SYSMP)
#include <sys/sysmp.h>
#include <sys/sysinfo.h>
#endif

#if defined(HAVE_MPCTL)
#include <sys/mpctl.h>
#endif

#if defined(HAVE_BINDPROCESSOR)
#include <sys/processor.h>
#endif

#if defined(HAVE_PROCESSOR_BIND)
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#endif

#if defined(HAVE_SCHED_SETAFFINITY)
#include <sched.h>
#endif

extern int custom(char* str, int cpu);
extern int reverse_bits(int cpu);
extern int sched_ncpus();
extern int sched_pin(int cpu);

int
handle_scheduler(int childno, int benchproc, int nbenchprocs)
{
	int	cpu = 0;
	char*	sched = getenv("LMBENCH_SCHED");
	
	if (!sched || strcasecmp(sched, "DEFAULT") == 0) {
		return 0;
	} else if (strcasecmp(sched, "SINGLE") == 0) {
		cpu = 0;
	} else if (strcasecmp(sched, "BALANCED") == 0) {
		cpu = childno;
	} else if (strcasecmp(sched, "BALANCED_SPREAD") == 0) {
		cpu = reverse_bits(childno);
	} else if (strcasecmp(sched, "UNIQUE") == 0) {
		cpu = childno * (nbenchprocs + 1) + benchproc;
	} else if (strcasecmp(sched, "UNIQUE_SPREAD") == 0) {
		cpu = reverse_bits(childno * (nbenchprocs + 1) + benchproc);
	} else if (strncasecmp(sched, "CUSTOM ", strlen("CUSTOM ")) == 0) {
		cpu = custom(sched + strlen("CUSTOM"), childno);
	} else if (strncasecmp(sched, "CUSTOM_UNIQUE ", strlen("CUSTOM_UNIQUE ")) == 0) {
		cpu = custom(sched + strlen("CUSTOM_UNIQUE"), 
			     childno * (nbenchprocs + 1) + benchproc);
	} else {
		return;
	}

	return sched_pin(cpu % sched_ncpus());
}

int
reverse_bits(int cpu)
{
	int	i;
	int	nbits;
	int	max = sched_ncpus() - 1;
	int	cpu_reverse = 0;

	for (i = max>>1, nbits = 1; i > 0; i >>= 1, nbits++)
	  ;
	for (i = 0; i < nbits; i++) {
		if (cpu & (1<<i))
			cpu_reverse |= (1<<(nbits-i-1));
	}
	return cpu_reverse;
}

int
custom(char* str, int cpu)
{
	static int nvalues = -1;
	static int* values = NULL;

	if (values == NULL) {
		nvalues = 0;
		values = (int*)malloc(sizeof(int));
	
		while (*str) {
			char* q;
			while (*str && !isdigit(*str)) str++;
			q = str;
			while (*str && isdigit(*str)) str++;
			if (str == q) break;
			*str++ = 0;
			sscanf(q, "%d", &values[nvalues++]);
			values = (int*)realloc((void*)values, (nvalues + 1) * sizeof(int));
		}
	}
	if (nvalues == 0) return 0;
	return values[cpu % nvalues];
}

int
sched_ncpus()
{
#ifdef MP_NPROCS
	return sysmp(MP_NPROCS);
#elif defined(HAVE_MPCTL)
	return mpctl(MPC_GETNUMSPUS_SYS, 0, 0);
#elif defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
	return 1;
}

int
sched_pin(int cpu)
{
	int retval = -1;

#ifdef HAVE_SYSMP
	retval = sysmp(MP_MUSTRUN, cpu);
#elif defined(HAVE_MPCTL)
	retval = mpctl(MPC_SET_PROCESS, cpu, MPC_SELFPID);
#elif defined(HAVE_BINDPROCESSOR)
	retval =  bindprocessor(BINDPROCESS, getpid(), cpu);
#elif defined(HAVE_PROCESSOR_BIND)
	retval = processor_bind(P_PID, P_MYPID, cpu, NULL);
#elif defined(HAVE_SCHED_SETAFFINITY)
	static unsigned long* mask = NULL;
	static unsigned long* cpumask = NULL;
	static int sz = 0;
	static int ncpus = 0;
	int i;
	int j;
	
	if (cpumask == NULL) {
		sz = 1 + (2 * sched_ncpus()) / (8 * sizeof(unsigned long));
		mask = (unsigned long*)malloc(sz * sizeof(unsigned long));
		cpumask = (unsigned long*)malloc(sz * sizeof(unsigned long));
		retval = sched_getaffinity(0, sz * sizeof(unsigned long), cpumask);
		if (retval < 0) perror("sched_getaffinity:");
		if (retval < 0) return retval;

		for (i = 0; i < sz * 8 * sizeof(unsigned long); ++i) {
			int	word = i / (8 * sizeof(unsigned long));
			int	bit = i % (8 * sizeof(unsigned long));
			if (cpumask[word] & (1 << bit)) ncpus++;
		}
	}
	cpu %= ncpus;

	bzero(mask, sz * sizeof(unsigned long));
	for (i = 0, j = 0; i < sz * 8 * sizeof(unsigned long); ++i) {
		int	word = i / (8 * sizeof(unsigned long));
		int	bit = i % (8 * sizeof(unsigned long));
		if (cpumask[word] & (1 << bit)) {
			if (j >= cpu) {
				mask[word] |= (1 << bit);
				break;
			}
			j++;
		}
	}
	retval = sched_setaffinity(0, sz * sizeof(unsigned long), mask);
	if (retval < 0) perror("sched_setaffinity:");
#ifdef _DEBUG
	fprintf(stderr, "sched_pin(%d): pid=%d, returning %d\n", cpu, (int)getpid(), retval);
#endif

#endif
	return retval;
}
