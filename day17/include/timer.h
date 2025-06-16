#ifndef _TIMER_H_
#define _TIMER_H_

#include "fifo.h"
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define MAX_TIMER 500
#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

/*
 *这里的定时器设置为100Hz，即0.01s一次中断，1秒有100次中断发生
 *如何验证？ 这里可以在定时器中加上一个count技术进行测试，可以测得每秒count加100。
 *由此，在使用定时器时可以根据测得的标准值来设定定时器
 */

extern struct TIMERCTL timerctl;

struct TIMER{
    unsigned int flags;     //使用标志位
    unsigned int timeout;   //用于记录距离超时还有多长时间
    struct FIFO32 *fifo;
    unsigned char data;     //data数据写入管道，用于便是属于哪一个定时器中断
    struct TIMER *next;
};

struct TIMERCTL{
    unsigned int count;
    unsigned int next_time;      //记录下一个超时时刻
    unsigned using_;             //记录活动定时器的个数
    struct TIMER timer[MAX_TIMER], *timers;
};

void init_pit(void);
void timer_settimer(struct TIMER *timer, unsigned int timeout);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void INT_handler20(int *esp);
#endif
