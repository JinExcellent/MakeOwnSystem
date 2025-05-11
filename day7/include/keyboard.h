#include "fifo.h"

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#define PORT_KEYDAT 0x0060          //键盘端口地址
#define PORT_KEYSTA 0x0064          //控制端口
#define PORT_KEYCMD 0x0064          //命令端口

#define KEYSTA_SEND_NOTREADY 0x02   //检测位，用于检查键盘控制电路是否准备好
#define KEYCMD_WRITE_MODE 0x60      //键盘模式设定指令
#define KBC_MODE 0x47               //设定键盘利用鼠标模式

#define KEY_FIFO_BUF_SIZE 32

extern struct FIFO8 keyfifo;
extern unsigned char keybuf[KEY_FIFO_BUF_SIZE];

void wait_KBC_sendready(void);
void init_keyboard(void);

#endif // _KEYBOARD_H_
