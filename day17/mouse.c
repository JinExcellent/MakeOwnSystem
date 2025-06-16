#include "fifo.h"
#include "asmfunc.h"
#include "keyboard.h"
#include "mouse.h"
#include "int.h"

struct FIFO32 *mousefifo;
int mousedata0;

unsigned char mousebuf[MOUSE_FIFO_BUF_SIZE];

void enable_mouse(struct FIFO32 *fifo, int data0, struct MouseDec *mdec) {
    mousefifo = fifo;
    mousedata0 = data0;

    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);        //向键盘控制器的命令端口发送0xd4，则下一个数据自动发送给鼠标
    wait_KBC_sendready();         
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);            //发送0xf4启动鼠标，如果成功，会答复一个0xfa
    mdec->phase = 0;

     return;
}

int mouse_decode(struct MouseDec *mdec, unsigned char dat){
    if(mdec->phase == 0){
        if(dat == 0xfa){
            mdec->phase = 1;
        }
        return 0;
    }else if(mdec->phase == 1){
        //检查第一字节的高两位（范围：0~3）和第四位（8~f）,0x08为鼠标初始状态
        if((dat & 0xc8) == 0x08){       
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    }else if(mdec->phase == 2){
        mdec->buf[1] = dat;
        mdec->phase = 3;

        return 0;
    }else if(mdec->phase == 3){
        mdec->buf[2] = dat;
        mdec->phase = 1;

        mdec->btn = mdec->buf[0] & 0x07;        //获取鼠标按键状态(鼠标按键的状态在低3位)
        mdec->x = mdec->buf[1]; 
        mdec->y = mdec->buf[2];

        //提取鼠标坐标值,由buf[0]中高四位中的后两位来判断是是鼠标的横向坐标还是纵向坐标数据
        if((mdec->buf[0] & 0x10) != 0){
            mdec->x |= 0xffffff00;
        }
        if((mdec->buf[0] & 0x20) != 0){
            mdec->y |= 0xffffff00;
        }
        mdec->y = -(mdec->y);                  //鼠标和屏幕的纵坐标相反(具体可查阅计算机图形学相关资料)
        
        return 1;
    }
    return -1;
}

//为何鼠标和键盘的中断处理都是从键盘的端口号中获取数据呢？*********************************
void INT_handler2c(int *esp) {
    //需要先告知从cpi，再告知主cpi
    io_out8(PIC1_OCW2, 0x64);
    io_out8(PIC0_OCW2, 0x62);

    int data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata0);
    
    return;
}


