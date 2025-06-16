#ifndef APP_H_
#define APP_H_

void api_putch(int c);
void api_end(void);
void api_putstr(char *);
int api_open_win(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstr_win(int win, int x, int y, int col, int len, char *str);
void api_boxfill_win(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
void api_end(void);
void api_point(int win, int x, int y, int col);
void api_close_win(int win);
int api_getkey(int mode);

#endif
