#ifndef _TIMER_H_
#define _TIMER_H_

#include "fifo.h"
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define MAX_TIMER 500
#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2


extern struct TIMERCTL timerctl;

struct TIMER{
    unsigned int flags;
    unsigned int timeout;   //用于记录距离超时还有多长时间
    struct FIFO8 *fifo;
    unsigned char data;
};

struct TIMERCTL{
    unsigned int count;
    unsigned int next;      //记录下一个超时时刻
    unsigned using_;        //记录活动定时器的个数
    struct TIMER timer[MAX_TIMER], *timers[MAX_TIMER];
};

void init_pit(void);
void timer_settimer(struct TIMER *timer, unsigned int timeout);
void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data);

struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
#endif
