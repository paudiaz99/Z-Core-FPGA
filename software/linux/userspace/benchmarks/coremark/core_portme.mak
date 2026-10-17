# Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Original Author: Shay Gal-on


OUTFLAG= -o
CC = gcc
PORT_CFLAGS = -O2
FLAGS_STR = "$(PORT_CFLAGS) $(XCFLAGS) $(XLFLAGS) $(LFLAGS_END)"
CFLAGS = $(PORT_CFLAGS) -I$(PORT_DIR) -I. -DFLAGS_STR=\"$(FLAGS_STR)\"
LFLAGS_END += -lrt
PORT_SRCS = $(PORT_DIR)/core_portme.c

LOAD = echo Loading done
RUN =

OEXT = .o
EXE = .exe

ifdef SEPARATE_COMPILE

LD		= gcc
OBJOUT 	= -o
LFLAGS 	=
OFLAG 	= -o
COUT 	= -c
PORT_OBJS = $(PORT_DIR)/core_portme$(OEXT)
PORT_CLEAN = *$(OEXT)

$(OPATH)%$(OEXT) : %.c
	$(CC) $(CFLAGS) $(XCFLAGS) $(COUT) $< $(OBJOUT) $@

endif

ifdef PGO
 ifeq (,$(findstring $(PGO),gen))
  PGO_STAGE=build_pgo_gcc
  CFLAGS+=-fprofile-use
 endif
 PORT_CLEAN+=*.gcda *.gcno gmon.out
endif

.PHONY: port_prebuild
port_prebuild: $(PGO_STAGE)

.PHONY: build_pgo_gcc
build_pgo_gcc:
	$(MAKE) PGO=gen XCFLAGS="$(XCFLAGS) -fprofile-generate -DTOTAL_DATA_SIZE=1200" ITERATIONS=10 gen_pgo_data REBUILD=1

.PHONY: port_postbuild
port_postbuild:

.PHONY: port_postrun
port_postrun:

.PHONY: port_prerun
port_prerun:

.PHONY: port_postload
port_postload:

.PHONY: port_preload
port_preload:

OPATH = ./
MKDIR = mkdir -p

PERL=/usr/bin/perl
