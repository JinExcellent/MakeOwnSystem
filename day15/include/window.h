#ifndef _WINDOW_H_
#define _WINDOW_H_
#include "sheet.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char* title, char act);

void make_title8(unsigned char *buf, int xsize, char *title, char act);
void make_textbox8(struct Sheet *sht, int x0, int y0, int sx, int sy, int c);
#endif
