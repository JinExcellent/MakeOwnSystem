#include "fifo.h"
#include "keyboard.h"
#include "asmfunc.h"
#include "int.h"

struct FIFO32 *keyfifo;
int keydata0;
unsigned char keybuf[KEY_FIFO_BUF_SIZE];

void wait_KBC_sendready(void){
    while (1) {
        if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            break;
    }
}

void init_keyboard(struct FIFO32 *fifo, int data0){
    keyfifo = fifo;
    keydata0 = data0;

    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);

    return;
}

void INT_handler21(int *esp) {
    io_out8(PIC0_OCW2, 0x61);

    int data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, data + keydata0);

    return;
}


