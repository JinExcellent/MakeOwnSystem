#include "timer.h"
#include "asmfunc.h"
#include "int.h"
#include "task.h"

struct TIMERCTL timerctl;


void init_pit(void){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    
    timerctl.count = 0;
    timerctl.next_time = 0xffffffff;
    timerctl.using_ = 0;

    for(int i = 0; i < MAX_TIMER; i++)
        timerctl.timer[i].flags = 0;

    struct TIMER *t;
    
    //设定哨兵
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;
    timerctl.timers = t;
    timerctl.next_time = 0xffffffff;
    //timerctl.using_ = 1;

    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data){
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
    int eflags;
    struct TIMER *t, *s;

    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    
    eflags = io_load_eflags();
    io_cli();
    
   // timerctl.using_++;
   
    t = timerctl.timers;
    if(timer->timeout <= t->timeout){
        timerctl.timers = timer;
        timer->next = t;
        timerctl.next_time = timer->timeout;

        io_store_eflags(eflags);
        return;
    }

    while(1){
        s = t;          //使用双指针进行查找
        t = t->next;

        /*if(t == 0) */
            /*break;*/

        if(timer->timeout <= t->timeout){
            s->next = timer;
            timer->next = t;

            io_store_eflags(eflags);
            return;
        }
    }

    return;
}

void INT_handler20(int *esp){
    struct TIMER *timer;
    char ts = 0;        //任务切换标志

    io_out8(PIC0_OCW2, 0x60);
    timerctl.count++;
   
    if(timerctl.next_time > timerctl.count){
        return;     //未到达下一个超时时刻，退出
    }
    
    timer = timerctl.timers;
    while(1){
        if(timerctl.count < timer->timeout)
            break;
        
        timer->flags = TIMER_FLAGS_ALLOC;
        if(timer != task_timer){
            //超时处理
            fifo32_put(timer->fifo, timer->data);
        }else
            ts = 1;     //当任务切换定时器超时后，设定任务切换标志
        timer = timer->next;
    }
    
    timerctl.timers = timer;
    /*timerctl.next_time = timerctl.timers->timeout;*/
    timerctl.next_time = timer->timeout;
    //在所有定时超时处理完毕后再进行可能的任务切换
    if(ts) taskswitch();

    return;
}


