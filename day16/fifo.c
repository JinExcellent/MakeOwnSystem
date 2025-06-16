#include "fifo.h"
#include "task.h"

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->next_r = 0;
    fifo->next_w = 0;
    //当有数据写入队列时，唤醒指定的任务，如果不想唤醒任务，可以指定为0
    fifo->task = task;
}

int fifo32_put(struct FIFO32 *fifo, int data) {
    if (fifo->free == 0) {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }

    fifo->buf[fifo->next_w++] = data;
    if (fifo->next_w == fifo->size) {
        fifo->next_w = 0;
    }
    fifo->free--;
    if(fifo->task != 0){
        if(fifo->task->flags != 2)        //如果任务处于休眠态，则唤醒(重复唤醒会造成任务重复注册)
            task_run(fifo->task, -1, 0);    //仅唤醒任务，不做其它改变
    }
    return 0;
}

int fifo32_get(struct FIFO32 *fifo) {
    if (fifo->free == fifo->size) {
        return -1;
    }

    int data = fifo->buf[fifo->next_r++];
    if (fifo->next_r == fifo->size) {
        fifo->next_r = 0;
    }
    fifo->free++;
    return data;
}

int fifo32_status(struct FIFO32 *fifo) {
    return fifo->size - fifo->free;
}
