#include "memory.h"
#include "sheet.h"

struct Shtctl *shtctl_init(struct MemMan *man, unsigned char *vram, int xsize, int ysize){
    struct Shtctl *ctl = (struct Shtctl *)memman_alloc_4K(man, sizeof(struct Shtctl));

    if(ctl == 0){
        return ctl;
    }
    
    if(!(ctl->map = (unsigned char *)memman_alloc_4K(man, xsize * ysize))){
        memman_free_4K(man, (unsigned int)ctl, sizeof(struct Shtctl) );
    }

    ctl->vram = vram;
    ctl->top = -1;
    ctl->xsize = xsize;
    ctl->ysize = ysize;

    for(int i = 0; i < MAX_SHEETS; i++){
        ctl->sheets0[i].flags = 0;
        ctl->sheets0[i].ctl = ctl;
    }

    return ctl;
}

struct Sheet *sheet_alloc(struct Shtctl *ctl){
    struct Sheet *sht;

    for(int i = 0; i < MAX_SHEETS; i++){
        if(ctl->sheets0[i].flags == 0){
            sht = &(ctl->sheets0[i]);
            sht->flags = SHEET_USE;
            sht->height = -1;

            return sht;
        }
    }
    return 0;
}

void sheet_updown(struct Sheet *sht, int height){
    int h, old = sht->height;
    struct Shtctl *ctl = sht->ctl;

    //处理超过指定高度
    if(height > ctl->top + 1)
       height = ctl->top + 1; 
    if(height < -1)
        height = -1;

    sht->height = height;

    //设置完指定高度后需要在sheets中重新排序
    if(old > height){
    //新设置的高度小，则将老高度所在位置进行覆盖，从后部找到一个合适的位置进行插入
        if(height >= 0){
            for(h = old; h > height; h--){
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            
            ctl->sheets[height] = sht;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
        }else{
            //隐藏页面
            if(ctl->top > old){
                for(h = old; h < ctl->top; h++){
                    ctl->sheets[h] = ctl->sheets[h+1];
                    ctl->sheets[h]->height = h;
                }
            }
            //在删除一个页面后或者是要删除的页面是最顶的页面
            ctl->top--;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
        //sheet_refresh(ctl);
    }else if(old < height){
       if(old >= 0){
            for(h = old; h < height; h++){
                ctl->sheets[h] = ctl->sheets[h+1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
       }else{
            //将页面从隐藏转为显示
            for(h = ctl->top; h >= height; h--){
                ctl->sheets[h+1] = ctl->sheets[h];
                ctl->sheets[h+1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;     //新增页面，top++
       }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }
}

/*void sheet_refreshsub(struct Shtctl *ctl, int vx0, int vy0, int vx1, int vy1){
    int h, bx, by, vx, vy;
    unsigned char *buf, c, *vram = ctl->varm;
    struct Sheet *sht;

    //从底向上依次刷新
    for(h = 0; h <= ctl->top; h++){
        sht = ctl->sheets[h];
        buf = sht->buf;
        for(by = 0; by < sht->bysize; by++){
            vy = sht->vy0 + by;
            for(bx = 0; bx < sht->bxsize; bx++){
                vx = sht->vx0 + bx;
                if(vx0 <= vx && vx < vx1 && vy0 <= vy && vy< vy1){
                    c = buf[by * sht->bxsize + bx];
                    if(c != sht->col_inv)
                        vram[vy * ctl->xsize + vx] = c;            
                }
            }
        }
    }
    return;
}
*/

void sheet_refreshmap(struct Shtctl *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    unsigned char *map = ctl->map;
    
    //处理鼠标超出屏幕时不显示 
    if(vx0 < 0)
        vx0 = 0;
    if(vy0 < 0)
        vy0 = 0;
    if(vx1 > ctl->xsize)
        vx1 = ctl->xsize;
    if(vy1 > ctl->ysize)
        vy1 = ctl->ysize;

    for (int h = h0; h <= ctl->top; h++) {
        struct Sheet *sht = ctl->sheets[h];
        unsigned char *buf = sht->buf;
        //指针减法所得结果为两个指针之间所隔的元素个数
        unsigned char *sid = sht - (ctl->sheets0);  
        // 使用vx0 ~ vy1，对bx0 ~ by1进行倒推
        int bx0 = vx0 - sht->vx0;
        int by0 = vy0 - sht->vy0;
        int bx1 = vx1 - sht->vx0;
        int by1 = vy1 - sht->vy0;

        if (bx0 < 0) {
          bx0 = 0;
        }
        if (by0 < 0) {
          by0 = 0;
        }
        if (bx1 > sht->bxsize) {
          bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
          by1 = sht->bysize;
    }

    for (int by = by0; by < by1; by++) {
        int vy = sht->vy0 + by;
        for (int bx = bx0; bx < bx1; bx++) {
            int vx = sht->vx0 + bx;
            unsigned char c = buf[by * sht->bxsize + bx];
            if (c != sht->col_inv) {
                map[vy * ctl->xsize + vx] = sid;    //将色号替换为图层号码
            }
        }
    }
  }
}

void sheet_refreshsub(struct Shtctl *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    unsigned char *buf, *vram = ctl->vram, *map = ctl->map;
    struct Sheet *sht;
    //处理鼠标超出屏幕时不显示 
    if(vx0 < 0)
        vx0 = 0;
    if(vy0 < 0)
        vy0 = 0;
    if(vx1 > ctl->xsize)
        vx1 = ctl->xsize;
    if(vy1 > ctl->ysize)
        vy1 = ctl->ysize;

    for (int h = h0; h <= h1; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        unsigned char sid = sht - ctl->sheets0;

        // 使用vx0 ~ vy1，对bx0 ~ by1进行倒推
        int bx0 = vx0 - sht->vx0;
        int by0 = vy0 - sht->vy0;
        int bx1 = vx1 - sht->vx0;
        int by1 = vy1 - sht->vy0;

        if (bx0 < 0) {
          bx0 = 0;
        }
        if (by0 < 0) {
          by0 = 0;
        }
        if (bx1 > sht->bxsize) {
          bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
          by1 = sht->bysize;
    }

    for (int by = by0; by < by1; by++) {
        int vy = sht->vy0 + by;
        for (int bx = bx0; bx < bx1; bx++) {
            int vx = sht->vx0 + bx;
            if (map[vy * ctl->xsize + vx] == sid) {
                vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
            }
        }
    }
  }
}

void sheet_refresh(struct Sheet *sht, int bx0, int by0, int bx1, int by1){
    if(sht->height >= 0){
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
}

void sheet_slide(struct Sheet *sht, int vx0, int vy0){
    //根据新的坐标重新绘制界面
    struct Shtctl *ctl = sht->ctl;
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;

    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if(sht->height >= 0){
        sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

void sheet_free(struct Sheet *sht){
    if(sht->height >= 0){
        sheet_updown(sht, -1);
    }
    sht->flags = 0;
    return;
}

void sheet_setbuf(struct Sheet *sht, unsigned char *buf, int xsize, int ysize, int col_inv){
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;

    return;
}






















