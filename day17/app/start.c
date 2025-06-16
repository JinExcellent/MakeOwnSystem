#include <stdlib.h>
#include "app.h"

int main(){
    char *buf;
    int x, y;
    
    api_initmalloc();
    buf = api_malloc(150 * 100);
    /*char buf[150*150];*/
    int win = api_open_win(buf, 150, 100, -1, "test1");
    api_boxfill_win(win, 6, 26, 143, 93, 0);
   /* for(int i = 0; i < 10; i++){*/
        /*x = (rand() % 137) + 6;*/
        /*y = (rand() %67) + 26;*/
        /*api_point(win, x, y, 3);*/
    /*}*/

    /*api_refresh_win(win, 6, 26, 144, 94);*/
    api_end();
 
    return 0;
}
