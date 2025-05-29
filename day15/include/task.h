#ifndef _TASK_H_
#define _TASK_H_

#include "timer.h"
#include "memory.h"
#include "fifo.h"

#define MAX_TASKS 1000          //单个最大任务量
#define MAX_TASKS_LV 100        //单个层级的最大任务数
#define MAX_TASKLEVELS 10       //任务队列
#define TASK_GDT0 3             //从GDT3号开始分配，1，2号已经分配（见相应源文件可知）

extern struct TIMER *mt_timer;
extern int mt_tr;
extern struct TIMER *task_timer;
extern struct TASKCTL *taskctl;

struct TSS32 {
  int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
  int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  int es, cs, ss, ds, fs, gs;
  int ldtr, iomap;
};

//struct FIFO32;  //前向声明

struct TASK{
    int sel, flags;         //sel存放GDT编号
    int level, priority;    //任务所在队列层数以及任务优先级
    struct FIFO32 fifo;
    struct TSS32 tss;
};

struct TASKLEVEL{
    int running;    //正在运行任务数
    int now;        //记录当前正在运行的是哪个任务
    struct TASK *tasks[MAX_TASKS_LV];      //正在运行任务队列
};

struct TASKCTL{
    int now_lv;     //现在活动中的level
    int lv_change;  //下次任务切换时是否需要切换level
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK tasks0[MAX_TASKS];
};

void load_tr(int tr);
void far_jmp(int eip, int cs);

void taskswitch(void);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
struct TASK *task_init(struct MemMan *memman);
void task_sleep(struct TASK *task);
struct TASK *task_now();
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_switchsub();
void task_idle();
#endif // _TASK_H_
