#!/usr/bin/env python3
"""Small, explicit no-MMU adjustments to pinned application sources."""
import argparse
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('app', choices=['htop', 'lscpu', 'vim'])
p.add_argument('source', type=Path)
a = p.parse_args()

def unsupported_fork(path):
    text = path.read_text()

    if 'fork()' in text:
        text = '#include <errno.h>\n' + text.replace('fork()', '(errno = ENOSYS, (pid_t)-1)')
        path.write_text(text)

if a.app == 'htop':
    for name in ['OpenFilesScreen.c', 'TraceScreen.c', 'linux/SystemdMeter.c']:
        unsupported_fork(a.source / name)
    path = a.source / 'linux/SystemdMeter.c'
    text = path.read_text()
    text = text.replace('#include <dlfcn.h>', '#ifndef BUILD_STATIC\n#include <dlfcn.h>\n#endif')
    path.write_text(text)
elif a.app == 'lscpu':

    for path in (a.source / 'lib').glob('*.c'):
        unsupported_fork(path)
    path = a.source / 'sys-utils/lscpu-cputype.c'
    text = path.read_text()

    text = '\n'.join(line for line in text.split('\n')
                     if not ('DEF_PAT_CPUTYPE' in line and
                             ('"mvendorid"' in line or '"uarch"' in line)))
    path.write_text(text)
elif a.app == 'vim':

    unsupported_fork(a.source / 'src/os_unix.c')
