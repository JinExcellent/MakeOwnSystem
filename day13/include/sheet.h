#ifndef _SHEET_H_
#define _SHEET_H_

#include "memory.h"

#define MAX_SHEETS 256
#define SHEET_USE 1

struct Sheet{
    unsigned char *buf;
    struct Shtctl *ctl;
    //col_inv图层透明色 height图层当前的高度 flags图层当前是否可用
    //bx*by图层大小 vx/vy图层在桌面中的坐标
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
};

struct Shtctl{
    unsigned char *vram, *map;
    int xsize, ysize, top;      //top 最定图层的高度
    struct Sheet *sheets[MAX_SHEETS];       //用于对图层的高度进行升序排序
    struct Sheet sheets0[MAX_SHEETS];
};

struct Shtctl *shtctl_init(struct MemMan *memman, unsigned char *vram, int xsize, int ysize);
struct Sheet *sheet_alloc(struct Shtctl *ctl);
void sheet_setbuf(struct Sheet *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct Sheet *sht, int height);
void sheet_refreshsub(struct Shtctl *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshmap(struct Shtctl *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void sheet_refresh(struct Sheet *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct Sheet *sht, int vx0, int vy0);
void sheet_free(struct Sheet *sht);


#endif
