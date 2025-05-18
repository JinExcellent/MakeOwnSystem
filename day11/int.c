#include "bootpack.h"
#include "graphic.h"
#include "int.h"
#include "asmfunc.h"
#include "fifo.h"
#include "mouse.h"
#include "keyboard.h"
#include "timer.h"
#include <stdio.h>

void init_pic(void) {
  // 禁止所有中断
  io_out8(PIC0_IMR, 0xff);
  io_out8(PIC1_IMR, 0xff);

  io_out8(PIC0_ICW1, 0x11); // 边缘触发模式
  io_out8(PIC0_ICW2, 0x20); // IRQ0-7由INT20-27接收
  io_out8(PIC0_ICW3, 1 << 2); // PIC1由IRQ2连接
  io_out8(PIC0_ICW4, 0x01); // 无缓冲区模式

  io_out8(PIC1_ICW1, 0x11); // 边缘触发模式
  io_out8(PIC1_ICW2, 0x28); // IRQ8-15由INT28-2f接收
  io_out8(PIC1_ICW3, 2); // PIC1由IRQ2连接
  io_out8(PIC1_ICW4, 0x01); // 无缓冲区模式

  io_out8(PIC0_IMR, 0xfb); // PIC1以外中断全部禁止
  io_out8(PIC1_IMR, 0xff); // 禁止全部中断
}

void INT_handler20(int *esp){
    int i, j;

    io_out8(PIC0_OCW2, 0x60);
    timerctl.count++;
   
    if(timerctl.next > timerctl.count){
        return;     //未到达下一个超时时刻，退出
    }
   
    for(i = 0; i < timerctl.using_; i++){
        if(timerctl.count < timerctl.timers[i]->timeout)
            break;
        
        //超时处理
        timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    
    timerctl.using_ -= i;
    //这里的程序还需要改进，虽然已大幅度减少if的判断，但又造成了数组移动的时间。并且在没有超时的情况下也会执行以下语句
    for(j = 0; j < timerctl.using_; j++)
        timerctl.timers[j] = timerctl.timers[i + j];     
    if(timerctl.using_ > 0)
        timerctl.next = timerctl.timers[0]->timeout;
    else
        timerctl.next = 0xffffffff;

    return;
}

//为何鼠标和键盘的中断处理都是从键盘的端口号中获取数据呢？*********************************
void INT_handler21(int *esp) {
    io_out8(PIC0_OCW2, 0x61);

    unsigned char data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo, data);
}

void INT_handler2c(int *esp) {
    //需要先告知从cpi，再告知主cpi
    io_out8(PIC1_OCW2, 0x64);
    io_out8(PIC0_OCW2, 0x62);

    unsigned char data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);

}

void INT_handler27(int *esp) {
	io_out8(PIC0_OCW2, 0x67);
}
