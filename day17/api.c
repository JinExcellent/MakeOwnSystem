#include "api.h"
#include "console.h"
#include "task.h"

//根据标志来调用不同的系统调用函数
// AL为单个字符编码     EBX为字符串地址     ECX为字符串长度     EDX为系统调用号
// ESI x坐标    EDI y坐标   ECX 标题    EAX透明色
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx,int eax){
    struct CONSOLE *cons = (struct CONSOLE *)*((int *)0x0fec);
    struct Shtctl *shtctl = (struct Shtctl *)*((int *)0x0fe4);
    struct TASK *task = task_now();
    int ds_base = *((int *) 0xfe8);
    struct Sheet *sht;
    char s[15];
    int *reg = &eax + 1;

    if(edx == 1){
        cons_putchar(cons, eax & 0xff, 1);
    }else if(edx == 2){
        cons_putstr(cons, (char *)ebx + ds_base);
    }else if(edx == 3){
        cons_putnstr(cons, (char *)ebx + ds_base, ecx);
    }else if(edx == 4){
        return &(task->tss.esp0);
    }else if(edx == 5){
        sht = sheet_alloc(shtctl);
        sheet_setbuf(sht, (unsigned char *)(ebx + ds_base), esi, edi, eax);
        make_window8((unsigned char *)(ebx + ds_base), esi, edi, (char *)(ecx + ds_base), 0);
        sheet_slide(sht, 100, 50);
        sheet_updown(sht, 3);
        reg[7] = (int)sht;
    }else if(edx == 6){
        sht = (struct Sheet *)(ebx & 0xfffffffe);
        put_fonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *)(ebp + ds_base));
        if(!(ebx & 1))
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
    }else if(edx == 7){
        sht = (struct Sheet *)(ebx & 0xfffffffe);
        box_fill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if(!(ebx & 1))
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
    }else if(edx == 8){
        memman_init((struct MemMan*)(ebx + ds_base));
        ecx &= 0xfffffff0;      //以16字节为单位
        memman_free((struct MemMan*)(ebx + ds_base), eax, ecx);
    }else if(edx == 9){
        ecx = (ecx + 0x0f) & 0xfffffff0;    //以16字节为单位进行取整
        reg[7] = memman_alloc((struct MemMan *)(ebx + ds_base), ecx);  //利用EAX返回所分配的地址
    }else if(edx == 10){
        ecx = (ecx + 0x0f) & 0xfffffff0;
        memman_free((struct MemMan *)(ebx + ds_base), eax, ecx);
    }else if(edx == 11){
        sht = (struct Sheet *)ebx;
        sht->buf[sht->bxsize * edi + esi] = eax;
        sheet_refresh(sht, esi, edi, esi+1, edi+1);
    }else if(edx == 12){
        sheet_free((struct Sheet *)ebx);
    }else if(edx == 13){
        while(1){
            
            io_cli();
            if(fifo32_status(&task->fifo) == 0){
                if(eax != 0){
                    task_sleep(task);
                }else{
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
                    
            }
            int data = fifo32_get(&task->fifo);
            io_sti();
            if(data <= 1){
                timer_init(cons->timer, &task->fifo, 1);
                timer_settimer(cons->timer, 50);
            }
            if(data == 2)   //光标ON
                cons->cursor_c = COL8_FFFFFF;
            if(data == 3)   //OFF
                cons->cursor_c = -1;
            if(256 <= data && data <= 511){     //通过主任务获得的键盘数据
                reg[7] = data - 256;
                return 0;
            }
        }
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
