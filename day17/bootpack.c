#include "bootpack.h"

int main(void) {
    struct BootInfo *binfo = (struct BootInfo *)ADR_BOOTINFO;     //这里根据给出的首地址，再通过结构体中成员变量的分布来逐一初始化
    struct MouseDec mdec;
    struct MemMan *memman = (struct MemMan *)MEMMAN_ADDR;
    char s[40];
    struct FIFO32 fifo;     //主任务中所有事件共用的管道
    int fifobuf[128];
    struct TIMER *timer;


    init_gdtidt();
    init_pic(); // GDT/IDT完成初始化，开放CPU中断

    io_sti();     //这里是终端表初始化完毕后开始设置允许中断
                //问题：计算机开机时中断标志的值为多少？***********

    //初始化从中断处理后接受数据的缓冲区
    fifo32_init(&fifo, 128, fifobuf, 0);

   
    init_keyboard(&fifo, 256);                           //初始化鼠标电路，启动鼠标模式
     //启动鼠标
    enable_mouse(&fifo, 512, &mdec);

    init_pit();     //初始化中断定时器
    
    //在初始化完毕中断处理缓冲区后再向pic发送设备中断信号
    io_out8(PIC0_IMR, 0xf8);                   // 开启PIT和PIC1以及键盘中断
    io_out8(PIC1_IMR, 0xef);                   // 开放鼠标中断           

    //内存检测
    unsigned int memtotal = memtest(0x0400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00010000, 0x0009e000); 
    memman_free(memman,0x00400000, memtotal - 0x00400000);
    
    init_palette();                            //初始化调色板
    struct Shtctl *shtctl;
    struct Sheet *sht_back, *sht_mouse, *sht_win, *sht_cons;
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
   
    //设置主任务
    /*关于以下两句放错位置导致程序BUG的原因记录*/
    /*
     *  以下两句放置在64行的io_sti之后会导致程序黑屏
     *      原因：是由于在没有内存初始化时就通过内存分配进行主任务初始化
     *  当放置在180行的while循环之前，会出现任务切换时出现闪屏
     *      原因：其它任务都先于主任务定义，导致在其它任务在切换到主任务时会出错
     */
    struct TASK *task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);

    //sht_back
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4K(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    //sht_cons
    sht_cons = sheet_alloc(shtctl); 
    buf_cons = (unsigned char *)memman_alloc_4K(memman, 256*165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    
    struct TASK *task_cons = task_alloc();
    task_cons->tss.esp  = memman_alloc_4K(memman, 64 * 1024) + 64 * 1024 - 12;  //分配栈空间并计算出栈底地址
    task_cons->tss.eip = (int)&console_task;        //任务的程序入口
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int)sht_cons;      //利用栈机制向执行程序传递参数
    *((int *) (task_cons->tss.esp + 8)) = memtotal;
    task_run(task_cons, 2, 2);
    
    //sht_win
    int cursor_x = 8, cursor_c = COL8_FFFFFF;

    sht_win = sheet_alloc(shtctl);
    buf_win = (unsigned char *) memman_alloc_4K(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 160, 52, -1);
    make_window8(buf_win, 160, 52, "counter", 1);
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
 
    //光标定时器
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settimer(timer, 50);

    //sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->scrnx - 16) / 2;          //定位绘画鼠标的坐标
    int my = (binfo->scrny - 28 - 16) / 2;
    
    
    sheet_slide(sht_back, 0 ,0);
    sheet_slide(sht_cons, 32, 4);
    sheet_slide(sht_win, 10, 400);
    sheet_slide(sht_mouse, mx, my);
    
    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_win, 2);
    sheet_updown(sht_mouse, 3);

    
    int key;
    int key_to = 0, key_shift = 0;     //用于窗口光标切换
    int key_ctl = 0;
    int key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
    struct FIFO32 keycmd;
    int keycmd_buf[32];

    fifo32_init(&keycmd, 32, keycmd_buf, 0);

    // 避免与键盘状态冲突
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    *((int *)0x0fe4) = (int)shtctl;

    while (1) {
        io_cli();
        if(fifo32_status(&fifo) == 0){
            task_sleep(task_a);
            io_sti();   //如果设置成stihlt会使cpu不断加电断电，也就造成了窗口的不断闪烁
        }else{
            //在从缓冲区中取到数据后再设置中断标志，其原因是如果该句和取数据语序颠倒，可能会造成开启中断后，后面的中断数据会覆盖要读的数据
            key = fifo32_get(&fifo);      
            io_sti();
            if(256 <= key && key <= 511){   //键盘接收数据范围 
                if(key < 256 + 0x80){           //0x80为键盘编码边界值
                    if(key_shift == 0)          
                        s[0] = keytable0[key - 256];
                    else
                        s[0] = keytable1[key - 256];
                }else{
                    s[0] = 0;
                }
                if('A' <= s[0] && s[0] <= 'Z'){
                    if(((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)){
                        s[0] += 0x20;    //大写与小写字母之间的偏移值为32
                    }
                }
                if(s[0] != 0){
                    if(key_to == 0){
                        if(cursor_x < 128){
                            s[1] = 0;
                            put_font8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    }else   //发送给命令窗口
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                }
                //删除字符
                if(key == 256 + 0x0e){
                    if(key_to == 0){
                        if(cursor_x > 8){   
                            put_font8_asc_sht(sht_win, cursor_x - 8, 28, COL8_000000, COL8_FFFFFF, " ", 2);
                            cursor_x -= 8;
                        }
                    }else{ 
                        fifo32_put(&task_cons->fifo, 8 + 256);
                    }
                }
                //TAB键
                if(key == 256 + 0x0f){
                   if(key_to == 0){
                        key_to = 1;
                        make_title8(buf_win, sht_win->bxsize, "task_a", 0);
                        make_title8(buf_cons, sht_cons->bxsize, "console", 1);
                        cursor_c = -1;
                        box_fill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 1, 43);
                        fifo32_put(&task_cons->fifo, 2);    //向指令窗口发送2表示开启光标
                   }else {
                        key_to = 0;
                        make_title8(buf_win, sht_win->bxsize, "task_a", 1);
                        make_title8(buf_cons, sht_cons->bxsize, "console", 0);
                        cursor_c = COL8_000000;
                        fifo32_put(&task_cons->fifo, 3);    //向指令窗口发送3表示关闭光标
                   }
                    sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }
                if(key == 256 + 0x2a){   //left shift ON
                    key_shift |= 1;   
                    sprintf(s, "left shift ON:%d", key_shift);
                    put_font8_asc_sht(sht_back, 0, 250, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                }
                if(key == 256 + 0x36){   //right shift ON
                    key_shift |= 2;
                    sprintf(s, "right shift ON:%d", key_shift);
                    put_font8_asc_sht(sht_back, 0, 270, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                }
                if(key == 256 + 0xaa){   //left shift OFF
                    key_shift &= ~1;
                    sprintf(s, "left shift OFF:%d", key_shift);
                    put_font8_asc_sht(sht_back, 0, 250, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                }
                if(key == 256 + 0xb6){   //right shift OFF
                    key_shift &= ~2;
                    sprintf(s, "right shift OFF:%d", key_shift);
                    put_font8_asc_sht(sht_back, 0, 270, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                }

                if(key == 256 + 0x1d){  //left Ctrl ON
                    key_ctl |= 1;
                    sprintf(s, "left ctl ON:%d", key_ctl);
                    put_font8_asc_sht(sht_back, 0, 289, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                }

                if(key == 256 + 0x9d){  //left Ctrl OFF
                    key_ctl &= ~1;
                    sprintf(s, "left ctl OFF:%d", key_ctl);
                    put_font8_asc_sht(sht_back, 0, 289, COL8_FFFFFF, COL8_008484, s, sizeof(s));
                }

                //CapsLock
                if(key == 256 + 0x3a){
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                //NumLock 
                if(key == 256 + 0x45){
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                //ScrollLock
                if(key == 256 + 0x46){
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                //ctl + c 
                if(key == 256 + 0x2e && key_ctl != 0 && task_cons->tss.ss0 != 0){   
                    struct CONSOLE *cons = (struct CONSOLE *) *((int *)0x0fec);
                    cons_putstr(cons, "\nbreak(key).\n");
                    io_cli();
                    task_cons->tss.eax = (int)&(task_cons->tss.esp0);
                    task_cons->tss.eip = (int)asm_end_app;
                    io_sti();
                }

                // 键盘成功接收到数据
                if (key == 256 + 0xfa) {
                    keycmd_wait = -1;
                }
                if (key == 256 + 0xfe) {
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
                
                //回车键
                if(key == 256 + 0x1c){
                    if(key_to != 0)
                        fifo32_put(&task_cons->fifo, 10 + 256);
                }
                //处理完字符后再显示光标
                if(cursor_c >= 0){
                    box_fill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 1, 43);
                }
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 1, 44);
            }else if(512 <= key && key <= 767) {    //鼠标接收数据范围
                if(mouse_decode(&mdec, key - 512)){
                    mx += mdec.x;
                    my += mdec.y;
                   
                    //可以通过修改数据来理解以下坐标设置语句的含义
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }

                    sheet_slide(sht_mouse, mx, my); // 描画鼠标
                    if((mdec.btn & 0x01) != 0)      //如果鼠标单击左键，则绘制窗口
                        sheet_slide(sht_win, mx - 80, my - 8);
                }
            }else if(key <= 1){
               //光标定时器
                if(key != 0){
                    timer_init(timer, &fifo, 0);
                    if(cursor_c >= 0)
                        cursor_c = COL8_000000;
                }else{
                    timer_init(timer, &fifo, 1);
                    if(cursor_c >= 0)
                        cursor_c = COL8_FFFFFF;
                }
                timer_settimer(timer, 50);
                if(cursor_c >= 0){
                    box_fill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 1, 43);     //绘制光标
                    sheet_refresh(sht_win, cursor_x, 28, cursor_x + 1, 44);
                }
            }
        }
    }
    return 0;
}
