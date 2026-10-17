/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/

#ifndef CORE_PORTME_H
#define CORE_PORTME_H
#ifndef HAS_FLOAT 
#define HAS_FLOAT 1
#endif
#ifndef HAS_TIME_H
#define HAS_TIME_H 1
#endif
#ifndef USE_CLOCK
#define USE_CLOCK 0
#endif
#ifndef HAS_STDIO
#define HAS_STDIO 1
#endif
#ifndef HAS_PRINTF
#define HAS_PRINTF 1
#endif

#if defined(_MSC_VER)
#include <windows.h>
typedef size_t CORE_TICKS;
#elif HAS_TIME_H
#include <time.h>
typedef clock_t CORE_TICKS;
#else
#error "Please define type of CORE_TICKS and implement start_time, end_time get_time and time_in_secs functions!"
#endif

#ifndef COMPILER_VERSION 
 #ifdef __GNUC__
 #define COMPILER_VERSION "GCC"__VERSION__
 #else
 #define COMPILER_VERSION "Please put compiler version here (e.g. gcc 4.1)"
 #endif
#endif
#ifndef COMPILER_FLAGS 
 #define COMPILER_FLAGS FLAGS_STR
#endif
#ifndef MEM_LOCATION 
 #define MEM_LOCATION "Please put data memory location here\n\t\t\t(e.g. code in flash, data on heap etc)"
 #define MEM_LOCATION_UNSPEC 1
#endif


typedef signed short ee_s16;
typedef unsigned short ee_u16;
typedef signed int ee_s32;
typedef double ee_f32;
typedef unsigned char ee_u8;
typedef unsigned int ee_u32;
typedef ee_u32 ee_ptr_int;
typedef size_t ee_size_t;
#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x) - 1) & ~3))


#ifndef SEED_METHOD
#define SEED_METHOD SEED_ARG
#endif


#ifndef MEM_METHOD
#define MEM_METHOD MEM_MALLOC
#endif





#ifndef MULTITHREAD
#define MULTITHREAD 1
#endif



#ifndef USE_PTHREAD
#define USE_PTHREAD 0
#endif



#ifndef USE_FORK
#define USE_FORK 0
#endif



#ifndef USE_SOCKET
#define USE_SOCKET 0
#endif


#ifndef MAIN_HAS_NOARGC 
#define MAIN_HAS_NOARGC 0
#endif


#ifndef MAIN_HAS_NORETURN
#define MAIN_HAS_NORETURN 0
#endif



extern ee_u32 default_num_contexts;

#if (MULTITHREAD>1)
#if USE_PTHREAD
	#include <pthread.h>
	#define PARALLEL_METHOD "PThreads"
#elif USE_FORK
	#include <unistd.h>
	#include <errno.h>
	#include <sys/wait.h>
	#include <sys/shm.h>
	#include <string.h>
	#define PARALLEL_METHOD "Fork"
#elif USE_SOCKET
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <errno.h>
	#define PARALLEL_METHOD "Sockets"
#else
	#define PARALLEL_METHOD "Proprietary"
	#error "Please implement multicore functionality in core_portme.c to use multiple contexts."
#endif
#endif

typedef struct CORE_PORTABLE_S {
#if (MULTITHREAD>1)
	#if USE_PTHREAD
	pthread_t thread;
	#elif USE_FORK
	pid_t pid;
	int shmid;
	void *shm;
	#elif USE_SOCKET
	pid_t pid;
	int sock;
	struct sockaddr_in sa;
	#endif
#endif
	ee_u8	portable_id;
} core_portable;

void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);

#if (SEED_METHOD==SEED_VOLATILE)
 #if (VALIDATION_RUN || PERFORMANCE_RUN || PROFILE_RUN)
  #define RUN_TYPE_FLAG 1
 #else
  #if (TOTAL_DATA_SIZE==1200)
   #define PROFILE_RUN 1
  #else
   #define PERFORMANCE_RUN 1
  #endif
 #endif
#endif

#endif
