/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
static int fd=-1;
static byte palette332[256], frame[320*200];
static unsigned frames;
static int game_frame_written;
extern int linux_frame_limit;
void I_InitGraphics(void) {
    fd=open("/dev/zcore-vga",O_WRONLY);
    if(fd<0) I_Error("Cannot open /dev/zcore-vga");
    usegamma=1;
}
void I_ShutdownGraphics(void) { if(fd>=0) { close(fd); fd=-1; } }
void I_SetPalette(byte *p) {
    for(int i=0;i<256;i++) {
        byte r=gammatable[usegamma][*p++],g=gammatable[usegamma][*p++],b=gammatable[usegamma][*p++];
        palette332[i]=(r&0xe0)|((g>>3)&0x1c)|(b>>6);
    }
}
void I_UpdateNoBlit(void) {}
void I_FinishUpdate(void) {
    for(int i=0;i<320*200;i++) frame[i]=palette332[screens[0][i]];
    if(write(fd,frame,sizeof(frame))!=sizeof(frame)) I_Error("VGA frame write failed");
    if(++frames==1) { printf("DOOM: first VGA frame written\n"); fflush(stdout); }
    if(gamestate==GS_LEVEL && !game_frame_written) {
        game_frame_written=1;
        printf("DOOM: first gameplay VGA frame written\n"); fflush(stdout);
    }
    if(linux_frame_limit>0 && frames>=(unsigned)linux_frame_limit) I_Quit();
}
void I_WaitVBL(int n) { usleep(n*1000000/70); }
void I_ReadScreen(byte *p) { memcpy(p,screens[0],320*200); }
