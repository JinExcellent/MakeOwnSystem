#include "console.h"
#include "bootpack.h"

void console_task(struct Sheet *sheet, unsigned int memtotal){
    struct TIMER *timer;
    struct TASK *task = task_now();
    int cursor_x = 16, cursor_y = 28, cursor_c = -1;
    int data ,fifobuf[128];
    char s[30], cmdline[30];
    struct MemMan *memman = (struct MemMan *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    char *p; //指向找到的文件在内存中的地址
    int *fat = (int *)memman_alloc_4K(memman, 4 * 2880);   //每个fat表项占32位
    fifo32_init(&task->fifo, 128, fifobuf, task);

    //光标定时器
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settimer(timer, 50);
    
    
    //解压FAT表
    //FAT12中第一个FAT表通常在第一个扇区
    //可以使用 “sudo fsck.fat -v name.img”来查看相应镜像文件中相应的FAT表信息
    file_read_fat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));
    //提示符
    put_font8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);
    while (1) {
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
                    if(cursor_c >= 0)
                        cursor_c = COL8_000000;
                }else{
                    timer_init(timer, &task->fifo, 1);
                    if(cursor_c >= 0)
                        cursor_c = COL8_FFFFFF;
                }
                timer_settimer(timer, 50);
            }
            if(data == 2)
                cursor_c = COL8_FFFFFF;
            if(data == 3){
                box_fill8(sheet->buf, cursor_x, COL8_000000, cursor_x, 28, cursor_x + 1, 43);
                cursor_c = -1;
            }
            //键盘数据
            if(256 <= data && data <= 511){
                if(data == 8 + 256){     //退格键码值为8
                    if(cursor_x > 16){
                        put_font8_asc_sht(sheet, cursor_x - 8, cursor_y, COL8_FFFFFF, COL8_000000, " ", 2);
                        cursor_x -= 8;
                    }
                }else if(data == 10 + 256){     //回车编码10
                    put_font8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1); //先用空格将光标擦除
                    cmdline[cursor_x / 8 - 2] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);
                    //执行指令
                    if(strcmp(cmdline, "mem") == 0){
                        sprintf(s, "total %dMB", memtotal / (1024 * 1024));
                        put_font8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, sizeof(s));
                        cursor_y = cons_newline(cursor_y, sheet);

                        sprintf(s, "free %dMB", memman_total(memman) / 1024);
                        put_font8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, sizeof(s));
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);

                    }else if(strcmp(cmdline, "cls") == 0){
                        for(int y = 28; y < 28 + 128; y++){
                            for(int x = 8; x < 8 + 240; x++)
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    }else if(strcmp(cmdline, "dir") == 0){
                        for(int x = 0; x < 224; x++){
                            if(finfo[x].name[0] == 0x00){
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                if (!(finfo[x].type & 0x18)) {
                                    sprintf(s, "filename.ext %7d", finfo[x].size);
                                    for (int y = 0; y < 8; y++) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    s[9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    
                                    put_font8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }   
                        }
                        cursor_y = cons_newline(cursor_y, sheet);

                    }else if(strncmp(cmdline, "type ", 5) == 0){
                    //准备文件名
                        for(int i = 0; i < 11; i++)
                            s[i] = ' ';
                        for(int i = 5, j = 0; j < 11 && cmdline[i] != 0; i++){
                            if(cmdline[i] == '.' && j <= 8){
                                j = 8;
                            }else{
                                s[j] = cmdline[i];      //在这里写入文件名
                                if('a' <= s[j] && s[j] <= 'z')
                                    s[j] -= 0x20;
                                j++;
                            }
                        }
                        //寻找文件
                        int x;
                        for(x = 0; x < 224;){
                            if(finfo[x].name[0] == 0x00){
                                  break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                               for(int i = 0; i < 11; i++){
                                    if(finfo[x].name[i] != s[i]){
                                        goto type_next_file;
                                    }
                               }
                               break;
                            }
            type_next_file:
                            x++;
                        }
                        
                        if(x < 224 && finfo[x].name[0] != 0x00){
                        //找到文件
                            p = (char *)memman_alloc(memman, finfo[x].size);
                            file_load_file(finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
                            cursor_x = 8;
                            for(int i = 0; i < finfo[x].size; i++){
                                s[0] = p[i];
                                s[1] = 0;
                                if(s[0] == 0x09){   //制表符
                                    while(1){
                                        put_font8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1); 
                                        cursor_x +=8;
                                        //到达窗口最右边的情况
                                        if(cursor_x == 8 + 240){
                                            cursor_x = 8;
                                            cursor_y = cons_newline(cursor_y, sheet);
                                        }
                                        if(((cursor_x - 8) & 0x1f) == 0)    //32 = 0x10,当为32的倍数时，条件成立（即能被32整除）
                                            break;
                                    }
                                }else if(s[0] == 0x0a){     //换行
                                    cursor_x = 8;
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }else if(s[0] == 0x0d){     //回车
                            
                                }else{
                                    put_font8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                                    cursor_x += 8;
                                    if(cursor_x == 8 + 240){
                                        cursor_x = 8;
                                        cursor_y = cons_newline(cursor_y, sheet);
                                    }
                                }
                            }
                            memman_free_4K(memman, (int)p, finfo[x].size);
                        }else{      //没有找到文件
                            put_font8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "not found file.", 20);
                            cursor_y = cons_newline(cursor_y, sheet);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);

                    }else if(cmdline[0] != 0){
                        put_font8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "bad command.", 20);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    put_font8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor_x = 16;
                }else{
                    //一般字符
                    if(cursor_c < 240){
                        s[0] = data - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 2] = data - 256;
                        put_font8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            if(cursor_c >= 0)
                box_fill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 1, cursor_y + 15);     //绘制光标
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 1, cursor_y + 16);
        }
    }
}


int cons_newline(int cursor_y, struct Sheet *sheet){
    int x, y;
    
    if(cursor_y < 28 + 112){
        cursor_y += 16;
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
    return cursor_y;
}




