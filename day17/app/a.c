#include "app.h"
#include <stdio.h>

int main(void){
    /*char ch[10];*/

    /*ch[10] = 'a';*/
    /*api_putch(ch[10]);*/
    /*ch[11] = 'b';*/
    /*api_putch(ch[11]);*/
    /*ch[-1] = 'c';   在QEMU的问题才导致这种访问奏效*/
    /*api_putch(ch[-1]);*/

    //窗口显示文字
    //char *buf;
    int win;
    char buf[150*50];
    //api_initmalloc();
    //buf = api_malloc(150 * 50);
    
    //if(buf){
    //    api_putstr("alloced");
        win = api_open_win(buf, 150, 50, -1, "test1");
        api_boxfill_win(win, 8, 36, 141, 44, 3);
        api_putstr_win(win, 8, 28, 0, 15, "hello world");
   // }else{
  //      api_putstr("no alloc");
    //}
    
   /* while(1){*/
        /*if(api_getkey(1) == 0x0a)*/
            /*break;*/
    /*}*/
    /*api_close_win(win);*/
    api_end();


    return 0;
}
