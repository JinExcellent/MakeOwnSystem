#include "api.h"
#include "console.h"
#include "task.h"

//根据标志来调用不同的系统调用函数
// AL为单个字符编码     EBX为字符串地址     ECX为字符串长度     EDX为标志位
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx,int eax){
    struct CONSOLE *cons = (struct CONSOLE *)*((int *)0x0fec);
    struct TASK *task = task_now();
    int cs_base = *((int *) 0xfe8);

    if(edx == 1){
        cons_putchar(cons, eax & 0xff, 1);
    }else if(edx == 2){
        cons_putstr(cons, (char *)ebx + cs_base);
    }else if(edx == 3){
        cons_putnstr(cons, (char *)ebx + cs_base, ecx);
    }else if(edx == 4){
        return &(task->tss.esp0);
    }

    return 0;
}

/*
 *由hrb_api函数最后的return 0；而造成的BUG记录
 *  由于之前忽略了该函数是需要返回一个int *类型的地址，
 *  该函数是由系统调用函数asm_hrb_api来调用的，这hrb_api函数执行完后会向EAX存入返回值
 *  由于我没有加retrun 0；这一句，导致EAX中的值未知，
 *  在返回到系统调用asm_hrb_api函数后会进行判断（这是EAX大概率不会为0,是一个未知的值）导致系统跳到end_app函数去执行
 *  end_app函数会跳转到由[EAX]指定task->tss.esp0去执行（这是在edx=4的情况下会返回一个非0的值），然而由于自己疏忽了，
 *  EAX中的值未知，导致系统会跳转到未知的区域，从而系统崩溃
 *
 * */
