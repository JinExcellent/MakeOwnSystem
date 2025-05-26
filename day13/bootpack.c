#include <include/asmfunc.h>
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
#include "task.h"


void task_b_main(struct Sheet *sht_win_b){
    struct FIFO32 fifo;
    int fifobuf[128], count = 0, count0 = 0;
    char s[12];
    int data;

    fifo32_init(&fifo, 128, fifobuf, 0);
    struct TIMER *timer_1s = timer_alloc();
    timer_init(timer_1s, &fifo, 100);
    timer_settimer(timer_1s, 100);

    while (1) {
        count++;

        io_cli();
        if(fifo32_status(&fifo) == 0){
            io_stihlt();    //**********设置成sti时会出现BUG(死机)
                /*sprintf(s, "%d-", fifo.buf[0]);*/
                /*put_font8_asc_sht(sht_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s, 5);*/

        }else {
            data = fifo32_get(&fifo);
            io_sti();
            if(data == 100){
                sprintf(s, "%5d", count);
                put_font8_asc_sht(sht_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s, 10);
                count0 = count;
                timer_settimer(timer_1s, 100);
            }
        }
    }
}

int main(void) {
    struct BootInfo *binfo = (struct BootInfo *)ADR_BOOTINFO;     //这里根据给出的首地址，再通过结构体中成员变量的分布来逐一初始化
    struct MouseDec mdec;
    struct MemMan *memman = (struct MemMan *)MEMMAN_ADDR;
    char s[40];
    struct FIFO32 fifo;
    int fifobuf[128];
    struct TIMER *timer;


    init_gdtidt();
    init_pic(); // GDT/IDT完成初始化，开放CPU中断

    io_sti();     //这里是终端表初始化完毕后开始设置允许中断
                //问题：计算机开机时中断标志的值为多少？***********

    //初始化从中断处理后接受数据的缓冲区
    fifo32_init(&fifo, 128, fifobuf, 0);

   
    init_keyboard(&fifo, 256);                           //初始化鼠标电路，启动鼠标模式
     //启动鼠标
    enable_mouse(&fifo, 512, &mdec);

    init_pit();     //初始化中断定时器
    
    //在初始化完毕中断处理缓冲区后再向pic发送设备中断信号
    io_out8(PIC0_IMR, 0xf8);                   // 开启PIT和PIC1以及键盘中断
    io_out8(PIC1_IMR, 0xef);                   // 开放鼠标中断           

    //内存检测
    unsigned int memtotal = memtest(0x0400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00010000, 0x0009e000); 
    memman_free(memman,0x00400000, memtotal - 0x00400000);
    
    init_palette();                            //初始化调色板
    struct Shtctl *shtctl;
    struct Sheet *sht_back, *sht_mouse, *sht_win, *sht_win_b[3];
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_win_b;

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
   
    //设置主任务
    /*关于以下两句放错位置导致程序BUG的原因记录*/
    /*
     *  以下两句放置在64行的io_sti之后会导致程序黑屏
     *      原因：是由于在没有内存初始化时就通过内存分配进行主任务初始化
     *  当放置在180行的while循环之前，会出现任务切换时出现闪屏
     *      原因：其它任务都先于主任务定义，导致在其它任务在切换到主任务时会出错
     */
    struct TASK *task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);

    //sht_back
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4K(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    //sht_win_b
    struct TASK *task_b[3];

    for(int i = 0; i < 3; i++){
        sht_win_b[i] = sheet_alloc(shtctl);
        buf_win_b = (unsigned char *)memman_alloc_4K(memman, 144 * 52);
        sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1);
        sprintf(s, "TASK:%d", i + 1);
        make_window8(buf_win_b, 144, 52, s, 0);
        task_b[i] = task_alloc();
        task_b[i]->tss.esp  = memman_alloc_4K(memman, 64 * 1024) + 64 * 1024 - 8;  //分配栈空间
        task_b[i]->tss.eip = (int)&task_b_main;        //任务的程序入口
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8;
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        *((int *) (task_b[i]->tss.esp + 4)) = (int)sht_win_b[i];      //利用栈机制向执行程序传递参数
        task_run(task_b[i], 2, i + 1);
    }
    
    //sht_win
    int cursor_x = 8, cursor_c = COL8_FFFFFF;

    sht_win = sheet_alloc(shtctl);
    buf_win = (unsigned char *) memman_alloc_4K(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 160, 52, -1);
    make_window8(buf_win, 160, 52, "counter", 1);
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
 
    //光标定时器
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settimer(timer, 50);

    //sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->scrnx - 16) / 2;          //定位绘画鼠标的坐标
    int my = (binfo->scrny - 28 - 16) / 2;
    
    
    sheet_slide(sht_back, 0 ,0);
    sheet_slide(sht_win_b[0], 168, 56);
    sheet_slide(sht_win_b[1], 8, 116);
    sheet_slide(sht_win_b[2], 168, 116);
    sheet_slide(sht_win, 8, 56);
    sheet_slide(sht_mouse, mx, my);
    
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win_b[0], 1);
    sheet_updown(sht_win_b[1], 2);
    sheet_updown(sht_win_b[2], 3);
    sheet_updown(sht_win, 4);
    sheet_updown(sht_mouse, 5);

    sprintf(s, "(%d, %d)", mx, my);
    put_fonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    
    sprintf(s, "memory %dMB, free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    put_fonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

    
    int key;
    
    while (1) {
        io_cli();
        if(fifo32_status(&fifo) == 0){
            task_sleep(task_a);
            io_sti();   //如果设置成stihlt会使cpu不断加电断电，也就造成了窗口的不断闪烁
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
            }else if(key <= 1){
               //光标定时器
                if(key != 0){
                    timer_init(timer, &fifo, 0);
                    cursor_c = COL8_000000;
                }else{
                    timer_init(timer, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                box_fill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 1, 43);     //绘制光标
                timer_settimer(timer, 50);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 1, 44);
            }
        }
    }
    return 0;
}
