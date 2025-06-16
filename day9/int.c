#include "bootpack.h"
#include "graphic.h"
#include "int.h"
#include "asmfunc.h"
#include "fifo.h"
#include "mouse.h"
#include "keyboard.h"
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
