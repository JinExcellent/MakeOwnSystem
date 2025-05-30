#ifndef _FIFO_H_
#define _FIFO_H_

#define FLAGS_OVERRUN 0x0001

struct FIFO32 {
  int *buf;
  int size, free, flags;
  int next_r, next_w;
  struct TASK *task;
};

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

#endif // _FIFO_H_
