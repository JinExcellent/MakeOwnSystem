#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#include "sheet.h"
#include "graphic.h"
#include "fileinfo.h"
#include <stdio.h>
#include "bootpack.h"
#include "timer.h"

struct CONSOLE{
    struct Sheet *sht;
    int cursor_x, cursor_y, cursor_c;
    struct TIMER *timer;
};

void cons_newline(struct CONSOLE *cons);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
void console_task(struct Sheet *sheet, unsigned int memtotal);
void cons_putstr(struct CONSOLE *cons, char *s);
void cons_putnstr(struct CONSOLE *cons, char *s, int len);
#endif
