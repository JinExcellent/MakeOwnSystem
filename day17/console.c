#include "console.h"
#include "cmd.h"
#include "elf.h"
#include <string.h>

void console_task(struct Sheet *sheet, unsigned int memtotal){
    struct TIMER *timer;
    struct CONSOLE cons;
    struct TASK *task = task_now();
    int data ,fifobuf[128];
    char s[30], cmdline[30];
    struct MemMan *memman = (struct MemMan *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    int *fat = (int *)memman_alloc_4K(memman, 4 * 2880);   //每个fat表项占32位
    
    cons.sht = sheet, cons.cursor_x = 8, cons.cursor_y = 28, cons.cursor_c = -1;
    *((int *)0x0fec) = (int) &cons;
    fifo32_init(&task->fifo, 128, fifobuf, task);

    //光标定时器
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settimer(timer, 50);
    
    
    //解压FAT表
    //FAT12中第一个FAT表通常在第二个扇区
    //可以使用 “sudo fsck.fat -v name.img”来查看相应镜像文件中相应的FAT表信息
    file_read_fat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));
    //提示符
    cons_putchar(&cons, '>' , 1);
    while(1){
        io_cli();
        if(fifo32_status(&task->fifo) == 0){
            task_sleep(task);
            io_sti();
        }else {
            data = fifo32_get(&task->fifo);
            io_sti();
            if(data <= 1){
               //光标定时器
                if(data != 0){
                    timer_init(timer, &task->fifo, 0);
                    if(cons.cursor_c >= 0)
                        cons.cursor_c = COL8_FFFFFF;
                }else{
                    timer_init(timer, &task->fifo, 1);
                    if(cons.cursor_c >= 0)
                        cons.cursor_c = COL8_000000;
                }
                timer_settimer(timer, 50);
            }
            if(data == 2)   //光标ON
                cons.cursor_c = COL8_FFFFFF;
            if(data == 3){  //光标OFF
                box_fill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cursor_x, 28, cons.cursor_x + 1, 43);
                cons.cursor_c = -1;
            }
            //键盘数据
            if(256 <= data && data <= 511){
                if(data == 8 + 256){     //退格键码值为8
                    if(cons.cursor_x > 16){
                        put_font8_asc_sht(cons.sht, cons.cursor_x - 8, cons.cursor_y, cons.cursor_c, COL8_000000, " ", 2);
                        cons.cursor_x -= 8;
                    }
                }else if(data == 10 + 256){     //回车编码10            
                    cons_putchar(&cons, ' ', 0);
                    cmdline[cons.cursor_x/8 - 2] = 0;
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);     //运行命令
                    
                    cons_putchar(&cons, '>', 1);
                }else{      //一般字符
                    if(cons.cursor_x < 240){
                        cmdline[cons.cursor_x / 8 - 2] = data - 256;
                        cons_putchar(&cons, data - 256, 1);
                    }
                }
                if(cons.cursor_c >= 0)
                    box_fill8(sheet->buf, sheet->bxsize, cons.cursor_c, cons.cursor_x, 28, cons.cursor_x + 1, 43);     //绘制光标
                sheet_refresh(cons.sht, cons.cursor_x, cons.cursor_y, cons.cursor_x + 1, cons.cursor_y + 16);
            }
        }
    }
}

void cons_putchar(struct CONSOLE *cons, int chr, char move){
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if(s[0] == 0x09){   //制表符
        while(1){
            //这里设的是tab=4
            put_font8_asc_sht(cons->sht, cons->cursor_x, cons->cursor_y, COL8_FFFFFF, COL8_000000, " ", 1); 
            cons->cursor_x +=8;
            //到达窗口最右边的情况
            if(cons->cursor_x == 8 + 240){
                cons_newline(cons);
            }
            if(((cons->cursor_x - 8) & 0x1f) == 0)    //32 = 0x10,当为32的倍数时，条件成立（即能被32整除）
                break;
        }
    }else if(s[0] == 0x0a){     //换行
        cons_newline(cons);
    }else if(s[0] == 0x0d){     //回车

    }else{  //普通字符
        put_font8_asc_sht(cons->sht, cons->cursor_x, cons->cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
        if(move != 0){
            cons->cursor_x += 8;
            if(cons->cursor_x == 8 + 240)
            cons_newline(cons);
        }
    }
}


void cons_newline(struct CONSOLE *cons){
    int x, y;
    struct Sheet *sheet = cons->sht;

    if(cons->cursor_y < 28 + 112){
        cons->cursor_y += 16;
    }else {
        //进行滚动窗口
        for(y = 28; y < 28 + 112; y++){
            for(x = 8; x < 8 + 240; x++){
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for(y = 28 + 112; y < 28 + 128; y++){
            for(x = 8; x < 8 + 240; x++){
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    cons->cursor_x = 8;     //思考这里为何要重新初始化位8
    return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal){
    if(strcmp(cmdline, "mem") == 0)
        cmd_mem(cons, memtotal);
    else if(strcmp(cmdline, "cls") == 0)
        cmd_cls(cons);
    else if(strcmp(cmdline, "dir") == 0)
        cmd_dir(cons);
    else if(strncmp(cmdline, "type ", 5) == 0)
        cmd_type(cons, fat, cmdline);
    else if(strcmp(cmdline, "hlt") == 0)
        cmd_hlt(cons,fat);
    else if(cmdline != 0){
        if(cmd_app(cons, fat, cmdline) == 0){
            cons_putstr(cons, "bad command\n");
        }
    }
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline){
    struct MemMan *memman = (struct MemMan *)MEMMAN_ADDR;
    struct FILEINFO *finfo;
    struct SegmentDescriptor *gdt = (struct SegmentDescriptor *) ADR_GDT;
    char name[18], *p, *q;
    struct TASK *task = task_now();

    int i;
    for(i = 0; i < 13; i++){
        if(cmdline[i] <= ' ')       //当遇到空格和回车跳出循环
            break;

        name[i] = cmdline[i];
    }
    name[i] = 0;    //文件名后加入空字符

    finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    if(finfo == 0 && name[i -1] != '.'){
        name[i] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        
        //加上文件后缀后继续寻找
        finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    }

    if(finfo){
        p = (char *)memman_alloc_4K(memman, finfo->size);
        *((int *)0xfe8) = (int)p;      
        file_load_file(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        Elf32_Ehdr *elfhdr = (Elf32_Ehdr *)p;

        if(elf32_validate(elfhdr)){
            q = (char *) memman_alloc_4K(memman, 64 * 1024);
            *((int *)0xfe8) = (int) q;      //向该地址传入程序在内存中的首地址

            set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
            set_segmdesc(gdt + 1004, 64 * 1024 - 1, (int)q, AR_DATA32_RW + 0x60);

            for(int i = 0; i < elfhdr->e_shnum; i++){
                Elf32_Shdr *shdr = (Elf32_Shdr *)(p + elfhdr->e_shoff + sizeof(Elf32_Shdr) * i);
                //判断程序是否包含程序内容相关的节以及是否被加载到程序的内存空间中
                if(shdr->sh_type != SHT_PROGBITS || !(shdr->sh_flags & SHF_ALLOC))
                    continue;
                
                //将代码段中的节区内容复制到数据段中
                for(int i = 0; i < shdr->sh_size; i++){
                    q[shdr->sh_addr + i] = p[shdr->sh_addr + i];
                }
            }
        
            start_app(0, 1003 * 8, 64 * 1024, 1004 * 8, &(task->tss.esp0));
            memman_free_4K(memman, (int)q, 64 * 1024);
        }else {
            cons_putstr(cons, "ELF file format error.");
        }
        memman_free_4K(memman, (int)p, finfo->size);
        cons_newline(cons);

        return 1;
    }
    return 0;
}
 
//显示字符串的API
void cons_putstr(struct CONSOLE *cons, char *s){
    for(; *s != 0; s++)
        cons_putchar(cons, *s, 1);

    return;
}

void cons_putnstr(struct CONSOLE *cons, char *s, int len){
    for(int i = 0; i < len; i++)
        cons_putchar(cons, s[i], 1);

    return;
}


