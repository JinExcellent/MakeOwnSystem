#include "timer.h"
#include "asmfunc.h"

struct TIMERCTL timerctl;


void init_pit(void){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    timerctl.using_ = 0;

    for(int i = 0; i < MAX_TIMER; i++)
        timerctl.timer[i].flags = 0;
    
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data){
    timer->fifo = fifo;
    timer->data = data;
}

struct TIMER *timer_alloc(void){
    for(int i = 0; i < MAX_TIMER; i++){
        if(timerctl.timer[i].flags == 0){
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }

    return 0;
}

void timer_free(struct TIMER *timer){
    timer->flags = 0;
    return;
}

void timer_settimer(struct TIMER *timer, unsigned int timeout){
    int i, j;
    int eflags;

    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    
    eflags = io_load_eflags();
    io_cli();

    for(i = 0; i < timerctl.using_; i++){
        if(timer->timeout <= timerctl.timers[i]->timeout)
            break;
    }
    
    for(j = timerctl.using_; j > i; j--)
        timerctl.timers[j] = timerctl.timers[j - 1];

    timerctl.using_++;
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(eflags);
    
    return;
}
