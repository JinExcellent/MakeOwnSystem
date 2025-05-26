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
#include "sheet.h"
#include "window.h"
#include "timer.h"

//unsigned char buf_test[100000];

int main(void) {
    struct BootInfo *binfo = (struct BootInfo *)ADR_BOOTINFO;     //这里根据给出的首地址，再通过结构体中成员变量的分布来逐一初始化
    struct MouseDec mdec;
    struct MemMan *memman = (struct MemMan *)MEMMAN_ADDR;
    char s[40];
    struct FIFO32 fifo;
    int fifobuf[128];
    struct TIMER *timer, *timer2, *timer3;


    init_gdtidt();
    init_pic(); // GDT/IDT完成初始化，开放CPU中断

    io_sti();     //这里是终端表初始化完毕后开始设置允许中断
                //问题：计算机开机时中断标志的值为多少？***********

    //初始化从中断处理后接受数据的缓冲区
    fifo32_init(&fifo, 128, fifobuf);
    
    init_keyboard(&fifo, 256);                           //初始化鼠标电路，启动鼠标模式
     //启动鼠标
    enable_mouse(&fifo, 512, &mdec);

    init_pit();     //初始化中断定时器
    
    //在初始化完毕中断处理缓冲区后再向pic发送设备中断信号
    io_out8(PIC0_IMR, 0xf8);                   // 开启PIT和PIC1以及键盘中断
    io_out8(PIC1_IMR, 0xef);                   // 开放鼠标中断           

    timer = timer_alloc();
    timer2 = timer_alloc();
    timer3 = timer_alloc();

    timer_init(timer, &fifo, 10);
    timer_init(timer2, &fifo, 3);
    timer_init(timer3, &fifo, 1);

    timer_settimer(timer, 1000);
    timer_settimer(timer2, 300);
    timer_settimer(timer3, 50);

    init_palette();                            //初始化调色板
    
    //内存检测
    unsigned int memtotal = memtest(0x0400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00010000, 0x0009e000); 
    memman_free(memman,0x00400000, memtotal - 0x00400000);
    
    init_palette();                            //初始化调色板
    struct Shtctl *shtctl;
    struct Sheet *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);

    buf_win = (unsigned char *) memman_alloc_4K(memman, 160 * 52);
    buf_back = (unsigned char *)memman_alloc_4K(memman, binfo->scrnx * binfo->scrny);       

    /**********问题：为什么不使用分配的内存就会黑屏？反而鼠标缓存却可以使用自己设定的缓存空间？？？？
     *
     *回答：其原因是在程序中声明的数组使用的该程序的栈段空间，而这个空间一般不会很大（在作为显存的临时空间肯定不够）
     * 栈段的空间大小可以又链接器进行显示设定。
     * 如果不想使用动态分配存储空间，可以声明一个全局变量（保存在data段），但会怎加程序的总体大小。
     * data段（Data Segment） 的大小是由 编译期确定、链接期固化 的，
     * 其大小主要取决于程序中 显式初始化的全局/静态变量 的存储需求
     */
    
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    sheet_setbuf(sht_win, buf_win, 160, 52, -1);
        
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    
    make_window8(buf_win, 160, 52, "counter");
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);

    sheet_slide(sht_back, 0 ,0);

    int mx = (binfo->scrnx - 16) / 2;          //定位绘画鼠标的坐标
    int my = (binfo->scrny - 28 - 16) / 2;
    
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);
    
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);

    sprintf(s, "(%d, %d)", mx, my);
    put_fonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    
    sprintf(s, "memory %dMB, free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    put_fonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);
    /*sprintf(s, "frees: %d, maxfrees: %dKB", memman->frees, memman->maxfrees);*/
    /*put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 48, COL8_FFFFFF, s);*/
    
    /*sprintf(s, "block size: %dKB[%X]:", memman->free[0].size, memman->free[0].addr);*/
    /*put_fonts8_asc(binfo->vram, binfo->scrnx, 0, 64, COL8_FFFFFF, s);*/

    int key;
    int cursor_x = 8, cursor_c = COL8_FFFFFF;
    
    while (1) {
        io_cli();
        if(fifo32_status(&fifo) == 0){
            io_stihlt();
        }else{
            key = fifo32_get(&fifo);      

            io_sti();
            //sprintf(s, "%0d", key);
            //put_font8_asc_sht(sht_win, 45, 24, COL8_000000, COL8_C6C6C6, s, 10);
            if(256 <= key && key <= 511){   //键盘接收数据范围 
                //在从缓冲区中取到数据后再设置中断标志，其原因是如果该句和取数据语序颠倒，可能会造成开启中断后，后面的中断数据会覆盖要读的数据
                sprintf(s, "%02X", key - 256);
                put_font8_asc_sht(sht_back,0, 16, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                if(key < 256 + 0x54){
                    if(keytable[key - 256] != 0 && cursor_x < 144){
                        s[0] = keytable[key - 256];
                        s[1] = 0;
                        put_font8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                        cursor_x += 8;
                    }
                }
                //删除字符
                if(key == 256 + 0x0e && cursor_x > 8){
                    put_font8_asc_sht(sht_win, cursor_x - 8, 28, COL8_000000, COL8_FFFFFF, " ", 2);
                    cursor_x -= 8;
                }
                //处理完字符后再显示光标
                box_fill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 1, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 1, 44);
            }else if(512 <= key && key <= 767) {    //鼠标接收数据范围
                if(mouse_decode(&mdec, key - 512)){
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
                    
                    put_font8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                    
                    mx += mdec.x;
                    my += mdec.y;
                   
                    //可以通过修改数据来理解以下坐标设置语句的含义
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }

                    sprintf(s, "(%3d, %3d)", mx, my);
                    put_font8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, sizeof(s));

                    sheet_slide(sht_mouse, mx, my); // 描画鼠标
                    if((mdec.btn & 0x01) != 0)      //如果鼠标单击左键，则绘制窗口
                        sheet_slide(sht_win, mx - 80, my - 8);
                }
            }else if(key == 10){
                put_fonts8_asc(buf_back, binfo->scrnx, 0, 64, COL8_000000, "10[sec]");
                    sheet_refresh(sht_back, 0, 64, 56, 80);
            }else if(key == 3){
                put_fonts8_asc(buf_back, binfo->scrnx, 0, 80, COL8_000000, "3[sec]");
                    sheet_refresh(sht_back, 0, 80, 56, 96);
            //光标定时器
            }else if(key <= 1){
                if(key != 0){
                    timer_init(timer3, &fifo, 0);
                    cursor_c = COL8_000000;
                }else{
                    timer_init(timer3, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                box_fill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 1, 43);     //绘制光标
                timer_settimer(timer3, 50);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 1, 44);
            }
        }
    }
    return 0;
}
