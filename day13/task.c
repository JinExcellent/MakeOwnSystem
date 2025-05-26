#include "task.h"
#include "desctbl.h"

struct TIMER *task_timer;
struct TASKCTL *taskctl;
int mt_tr;

//任务切换通过时钟定时器进行
void taskswitch(void){
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];    
    struct TASK *now_task = tl->tasks[tl->now];

    tl->now++;
    if(tl->now == tl->running)
        tl->now = 0;
    
    if(taskctl->lv_change){
        task_switchsub();
        tl= &taskctl->level[taskctl->now_lv];
    }

    struct TASK *new_task = tl->tasks[tl->now];
    timer_settimer(task_timer, new_task->priority);
    //当只有一个任务时，避免自身切换
    if(new_task != now_task)
        far_jmp(0, new_task->sel);

    return;
}

//切换level
void task_switchsub(){
    int i;

    for(i = 0; i < MAX_TASKLEVELS; i++){
        if(taskctl->level[i].running > 0)
            break;
    }

    taskctl->now_lv = i;
    taskctl->lv_change = 0;
}

struct TASK *task_init(struct MemMan *memman){
    struct TASK *task;
    struct SegmentDescriptor *gdt = (struct SegmentDescriptor *) ADR_GDT;
    taskctl = (struct TASKCTL *)memman_alloc_4K(memman, sizeof(struct TASKCTL));
    
    for(int i = 0; i < MAX_TASKS; i++){
       taskctl->tasks0[i].flags = 0;
       taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
       set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }
    
    for(int i = 0; i < MAX_TASKLEVELS; i++){
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }

    //主任务初始化
    task = task_alloc();
    task->flags = 2;        //任务活动标记
    task->priority = 2;     //0.02s
    task->level = 0;
    task_add(task);
    task_switchsub();
    load_tr(task->sel);
    
    task_timer = timer_alloc();
    timer_settimer(task_timer, task->priority);

    return task;
}

struct TASK *task_alloc(void){
    struct TASK *task;

    for(int i = 0; i < MAX_TASKS; i++){
        if(taskctl->tasks0[i].flags == 0){
            task = &taskctl->tasks0[i];
            task->flags = 1;    //使用标志
            task->tss.eflags = 0x00000202; // IF = 1
            task->tss.eax = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.ebx = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;

              return task;
        }
    }
    return 0;
}

void task_run(struct TASK *task, int level, int priority){
    if(level < 0)
        level = task->level;        //在唤醒任务不改变层级
    if(priority > 0)
        task->priority =priority;

    //改变活动中的任务的level，在改变后flags = 1，因为上层队列中的任务优先级高
    if(task->flags == 2 && task->level != level)
        task_remove(task);
    
    if(task->flags != 2){
        task->level = level;
        task_add(task);
    }
    taskctl->lv_change = 1;
}

void task_add(struct TASK *task){
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    tl->tasks[tl->running] = task;
    tl->running++;
    task->flags = 2;
}

struct TASK *task_now(){
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

void task_sleep(struct TASK *task){
    if(task->flags == 2){
        struct TASK *now_task = task_now();
        task_remove(task);
        if(task == now_task){
            task_switchsub();
            now_task = task_now();
            far_jmp(0, now_task->sel);
        }   
    }
}

void task_remove(struct TASK *task){
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    int i;

    for(i = 0; i < tl->running; i++){
        if(tl->tasks[i] == task)
            break;
    }

    tl->running--;
    if(i < tl->now)
        tl->now--;      //欲删除任务在正在运行任务之后，需要调整now

    if(tl->now >= tl->running)
        tl->now = 0;

    task->flags = 1;
    //移动队列中的任务
    for(; i < tl->running; i++)
        tl->tasks[i] = tl->tasks[i+1];
}

