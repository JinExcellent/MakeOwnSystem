#include "fifo.h"

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#define PORT_KEYDAT 0x0060          //键盘端口地址
#define PORT_KEYSTA 0x0064          //控制端口
#define PORT_KEYCMD 0x0064          //命令端口

#define KEYSTA_SEND_NOTREADY 0x02   //检测位，用于检查键盘控制电路是否准备好
#define KEYCMD_WRITE_MODE 0x60      //键盘模式设定指令
#define KBC_MODE 0x47               //设定键盘利用鼠标模式
#define KEYCMD_LED 0xed
#define KEY_FIFO_BUF_SIZE 32

static char keytable0[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9',  '0', '-', '=',  0,
    0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '[', ']', 0,    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0,   '\\', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0,   '*',  0,   ' ', 0,    0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    '7', '8', '9',  '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.', 0,   0,    0,   0,   0,    0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,
    0,   0,   0,   0,   0,   0,   0,   0};

static char keytable1[] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0};


extern struct FIFO32 *keyfifo;
extern unsigned char keybuf[KEY_FIFO_BUF_SIZE];

void wait_KBC_sendready(void);
void INT_handler21(int *esp);
void init_keyboard(struct FIFO32 *fifo, int data0);
#endif // _KEYBOARD_H_
