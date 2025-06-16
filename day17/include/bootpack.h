#ifndef _BOOTPACK_H_
#define _BOOTPACK_H_

#include <stdio.h>
#include <string.h>
#include "desctbl.h"
#include "graphic.h"
#include "fifo.h"
#include "keyboard.h"
#include "mouse.h"
#include "int.h"
#include "asmfunc.h"
#include "memory.h"
#include "sheet.h"
#include "window.h"
#include "timer.h"
#include "task.h"
#include "console.h"
#include "fileinfo.h"



#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG 0x00100000

struct BootInfo{
  char  cyls;
  char  leds;
  char  vmode;
  char  reserve;
  short scrnx;
  short scrny;
  unsigned char *vram;
};


#endif // _BOOTPACK_H_
