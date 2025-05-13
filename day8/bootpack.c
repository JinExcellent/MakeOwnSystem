#include <stdio.h>
#include "bootpack.h"
#include "desctbl.h"
#include "graphic.h"
#include "fifo.h"
#include "keyboard.h"
#include "mouse.h"
#include "int.h"
#include "asmfunc.h"
#include "memory.h"

int main(void) {
    struct BootInfo *binfo = (struct BootInfo *)ADR_BOOTINFO;     //这里根据给出的首地址，再通过结构体中成员变量的分布来逐一初始化
    struct MouseDec mdec;
    struct MemMan *memman = (struct MemMan *)MEMMAN_ADDR;
    char mcursor[256];
    char s[40];

    init_gdtidt();
    init_pic(); // GDT/IDT完成初始化，开放CPU中断

    io_sti();     //这里是终端表初始化完毕后开始设置允许中断
                //问题：计算机开机时中断标志的值为多少？***********

    //初始化从中断处理后接受数据的缓冲区
    fifo8_init(&keyfifo, KEY_FIFO_BUF_SIZE, keybuf);
    fifo8_init(&mousefifo, MOUSE_FIFO_BUF_SIZE, mousebuf);

    //在初始化完毕中断处理缓冲区后再向pic发送设备中断信号
    io_out8(PIC0_IMR, 0xf9);                   // 开放PIC1以及键盘中断
    io_out8(PIC1_IMR, 0xef);                   // 开放鼠标中断           

    init_keyboard();                           //初始化鼠标电路，启动鼠标模式

    init_palette();                            //初始化调色板
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    int mx = (binfo->scrnx - 16) / 2;          //定位绘画鼠标的坐标
    int my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    put_block8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    //启动鼠标
    enable_mouse();
    
    //内存检测
    unsigned int memtotal = memtest(0x0400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00010000, 0x0009e000); 
    sprintf(s, "memory %dMB, free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sprintf(s, "frees: %d, maxfrees: %dKB", memman->frees, memman->maxfrees);
    put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 48, COL8_FFFFFF, s);
    
    sprintf(s, "block size: %dKB[%X]:", memman->free[0].size, memman->free[0].addr);
    put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 64, COL8_FFFFFF, s);

    unsigned char key;

    for (;;) {
        io_cli();
        if((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0){
            io_stihlt();
        }else{
            if(fifo8_status(&keyfifo)){
                key = (unsigned char)fifo8_get(&keyfifo);      
                
                //在从缓冲区中取到数据后再设置中断标志，其原因是如果该句和取数据语序颠倒，可能会造成开启中断后，后面的中断数据会覆盖要读的数据
                io_sti();
                sprintf(s, "%02X", key);
                //这里每次显示或打印鼠标之前都冲洗一次屏幕，如果没有冲洗，后写入的数据会与之前的进行重叠
                box_fill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            }else if(fifo8_status(&mousefifo)) {
                key = (unsigned char)fifo8_get(&mousefifo);

                io_sti();
                 
                if(mouse_decode(&mdec, key)){
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);

                    if((mdec.btn & 0x01)){
                        s[1] = 'L';
                    }
                    if((mdec.btn & 0x02)){
                        s[3] = 'R';
                    }
                    if((mdec.btn & 0x04)){
                        s[2] = 'C';
                    }
                    box_fill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    put_fonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    box_fill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
                   
                    mx += mdec.x;
                    my += mdec.y;
                   
                    //可以通过修改数据来理解以下坐标设置语句的含义
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }

                    sprintf(s, "(%3d, %3d)", mx, my);
                    box_fill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 隐藏坐标
                    put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // 显示坐标
                    put_block8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); // 描画鼠标
                }
            }
        }
  }

  return 0;
}
