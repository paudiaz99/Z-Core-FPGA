/* SPDX-License-Identifier: GPL-2.0-or-later */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "doomdef.h"
#include "d_main.h"
#include "m_argv.h"
int linux_frame_limit;
static struct termios saved;
static int have_tty;
static void restore(void) { if(have_tty) tcsetattr(0,TCSANOW,&saved); }
static void stop(int sig) { restore(); _exit(128+sig); }
int main(int argc,char **argv) {
    if(argc>1 && !strcmp(argv[1],"--help")) {
        puts("DOOM for Z-Core Linux: VGA display, WASD movement, space fire, E use, Esc menu.\n"
             "--frames N stops after N frames (bring-up check). Ctrl-C quits."); return 0;
    }
    for(int i=1;i+1<argc;i++) if(!strcmp(argv[i],"--frames")) linux_frame_limit=atoi(argv[i+1]);
    myargc=argc; myargv=argv;
    setbuf(stdout,NULL);
    if(tcgetattr(0,&saved)==0) {
        struct termios raw=saved; have_tty=1; atexit(restore);
        raw.c_lflag &= ~(ICANON|ECHO);
        raw.c_cc[VMIN]=0; raw.c_cc[VTIME]=0; tcsetattr(0,TCSANOW,&raw);
    }
    signal(SIGINT,stop); signal(SIGTERM,stop);
    if(chdir("/usr/share/doom")) { perror("DOOM data directory"); return 1; }
    D_DoomMain(); return 0;
}
