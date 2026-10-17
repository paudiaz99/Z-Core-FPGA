#!/usr/bin/env python3
"""Cross-build Linux bFLT applications from the vendored board sources."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cc', required=True)
p.add_argument('--source', type=Path, required=True)
p.add_argument('--target', type=Path, required=True)
a=p.parse_args()
src=a.source.resolve(); target=a.target.resolve()
out=src/'out'; out.mkdir(exist_ok=True)
flags=['-O2','-fPIC','-fno-strict-aliasing','-march=rv32ima_zicsr_zifencei',
       '-mabi=ilp32','-static','-Wl,-elf2flt=-r -s65536','-I'+str(src/'compat')]
bindir=target/'usr/bin'; bindir.mkdir(parents=True,exist_ok=True)
def build(name, files, extra=(), libs=()):
    subprocess.run([a.cc,*flags,*extra,*map(str,files),*libs,'-o',str(out/name)],check=True)
    shutil.copy2(out/name,bindir/name)
    (bindir/name).chmod(0o755)

build('perf',[src/'perf.c'])
build('zcore-counters',[src/'counters.c'])
for name in ['dhrystone','whetstone','stream','ptr-chase','wb3d']:
    path=src/'benchmarks'/name/(name+'.c')
    opts=['-fwrapv'] if name=='stream' else ['-DVGA_BLIT=0'] if name=='wb3d' else []
    installed='fpga-'+name if name in ['dhrystone','whetstone','stream'] else name
    build(installed,[path],opts)
    if name!='wb3d':
        opts={'dhrystone':['-DDHRY_ITERATIONS=2000'], 'whetstone':['-DWHET_LOOPS=2'],
              'stream':['-fwrapv','-DSTREAM_ARRAY_SIZE=16384','-DNTIMES=5'],
              'ptr-chase':['-DMAX_BYTES=65536']}[name]
        if name=='ptr-chase':
            quick=out/'ptr-quick.c'
            quick.write_text(path.read_text().replace('(1u << 20)','(1u << 12)').replace(
                'for (unsigned s = 0; s < NSIZES; s++)',
                'for (unsigned s = 0; s < NSIZES && ws_bytes[s] <= 65536; s++)'))
            path=quick
        build(installed+'-quick',[path],opts)

original=src/'benchmarks/original'
dhry=out/'dhry_1-linux.c'
text=(original/'dhry_1.c').read_text().replace('extern char     *malloc ();',
    '#include <stdlib.h>\n#include <string.h>').replace('extern  int     times ();',
    'extern clock_t times (struct tms *);')
dhry.write_text(text)
build('dhrystone',[dhry,original/'dhry_2.c'],
      ['-I'+str(original),'-std=gnu89','-fno-builtin','-DHZ=100'])
build('whetstone',[original/'whetstone.c'],libs=['-lm'])
build('stream',[original/'stream.c'],['-DSTREAM_ARRAY_SIZE=131072'],libs=['-lm'])
build('stream-quick',[original/'stream.c'],['-DSTREAM_ARRAY_SIZE=16384'],libs=['-lm'])
for obsolete in ['dhrystone-quick','whetstone-quick','lavat']:
    (bindir/obsolete).unlink(missing_ok=True)

cm=src/'benchmarks/coremark'
cmfiles=[cm/n for n in ['core_main.c','core_list_join.c','core_matrix.c','core_state.c','core_util.c','core_portme.c']]
build('coremark',cmfiles,['-I'+str(cm),'-DITERATIONS=0','-DPERFORMANCE_RUN=1',
    '-DMULTITHREAD=1','-DMEM_LOCATION="Linux heap"','-DFLAGS_STR="-O2 RV32IMA ILP32"'])
lm=src/'benchmarks/lmbench'
lmflags=['-I'+str(lm),'-DNO_RPC','-DHAVE_uint=1','-DHAVE_int64_t=1','-DHAVE_uint64_t=1',
         '-include','stdint.h','-include','string.h']
lmlibs=[lm/n for n in ['lib_timing.c','lib_mem.c','lib_stats.c','lib_sched.c','getopt.c','nommu.c']]
for name in ['bw_mem','lat_mem_rd','lat_syscall']:
    build('lmbench-'+name,[lm/(name+'.c'),*lmlibs],lmflags)

doom=src/'doom'; engine=doom/'src'; platform=doom/'linux'
sources=re.search(r'SOURCES_doom := \\\n(.*?)\$\(NULL\)',(engine/'sources.mk').read_text(),re.S).group(1)
files=[engine/n for n in re.findall(r'[a-z_]+\.c',sources) if n not in ['d_main.c','s_sound.c']]
files += [engine/'riscv'/n for n in ['d_main.c','i_net.c','i_sound.c','s_sound.c']]
files += [platform/n for n in ['i_main.c','i_system.c','i_video.c','console.c']]
build('doom',files,['-DNORMALUNIX','-DZCORE_DOOM','-I'+str(engine),'-I'+str(engine/'riscv')])
data=target/'usr/share/doom'; data.mkdir(parents=True,exist_ok=True)
shutil.copy2(doom/'doom1.wad',data/'doom1.wad')
licenses=target/'usr/share/zcore/licenses'; licenses.mkdir(parents=True,exist_ok=True)
(licenses/'lavat').unlink(missing_ok=True)
for source,name in [(doom/'LICENSE','doom'),(cm/'LICENSE.md','coremark'),
                    (lm/'COPYING','lmbench'),(lm/'COPYING-2','lmbench-notice'),
                    (original/'whetstone.c','whetstone-source-notice'),
                    (original/'stream.c','stream-source-notice'),
                    (original/'dhry.h','dhrystone-source-notice')]:
    shutil.copy2(source,licenses/name)
