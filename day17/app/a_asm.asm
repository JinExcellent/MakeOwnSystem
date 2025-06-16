[BITS 32]

GLOBAL api_putch, api_end, api_putstr, api_open_win, api_putstr_win, api_boxfill_win, api_initmalloc, api_malloc, api_free, api_point, api_close_win, api_getkey

api_putch:      ;void api_putch(int c)
    MOV EDX, 1
    MOV AL, [ESP + 4]
    INT 0x40
    RET

api_putstr:
    PUSH EBX
    MOV EDX, 2
    MOV EBX, [ESP + 8]
    INT 0x40
    POP EBX
    RET

api_end:
    MOV EDX, 4
    INT 0x40

api_open_win:   ;int api_open_win(char *buf, int xsize, int ysize, int col_inv, char *title)
    PUSH EDI
    PUSH ESI
    PUSH EBX
    
    MOV EDX, 5
    MOV EBX, [ESP+16]
    MOV ESI, [ESP+20]
    MOV EDI, [ESP+24]
    MOV EAX, [ESP+28]
    MOV ECX, [ESP+32]

    INT 0x40

    POP EBX
    POP ESI
    POP EDI

    RET


api_putstr_win: ;void api_putstr_win(int win, int x, int y, int col, int len, char *ch)
    PUSH EDI
    PUSH ESI
    PUSH EBP
    PUSH EBX
    
    MOV EDX, 6
    MOV EBX, [ESP+20]
    MOV ESI, [ESP+24]
    MOV EDI, [ESP+28]
    MOV EAX, [ESP+32]
    MOV ECX, [ESP+36]
    MOV EBP, [ESP+40]

    INT 0x40

    POP EBX
    POP EBP
    POP ESI
    POP EDI

    RET

api_boxfill_win: ;void api_boxfill_win(int win, int x0, int y0, int x1, int y1, int col)
    PUSH EDI
    PUSH ESI
    PUSH EBP
    PUSH EBX
    
    MOV EDX, 7
    MOV EBX, [ESP+20]
    MOV EAX, [ESP+24]
    MOV ECX, [ESP+28]
    MOV ESI, [ESP+32]
    MOV EDI, [ESP+36]
    MOV EBP, [ESP+40]

    INT 0x40

    POP EBX
    POP EBP
    POP ESI
    POP EDI

    RET

api_initmalloc: ;void api_initmalloc();
    PUSH EBX
    MOV EDX, 8
    MOV EBX, [CS:0x0020]
    MOV EAX, EBX
    ADD EAX, 32*1024
    MOV ECX, [CS:0X0000]
    SUB ECX, EAX
    
    INT 0x40

    POP EBX
    
    RET

api_malloc: ;void api_malloc(int size)
    PUSH EBX
    MOV EDX, 9
    MOV EBX, [CS:0x0020]
    MOV ECX, [ESP+8]
    
    INT 0x40
    
    POP EBX

    RET


api_free:   ;api_free(char *addr, int size)
    PUSH EBX
    MOV EDX, 10
    MOV EBX, [CS:0x0020]
    MOV EAX, [ESP+8]
    MOV ECX, [ESP+12]

    INT 0x40

    POP EBX

    RET

api_point:  ;void api_point(int win, int x, int y, int col)
    PUSH EDI
    PUSH ESI
    PUSH EBX

    MOV EDX, 11
    MOV EBX, [ESP+16]
    MOV ESI, [ESP+20]
    MOV EDI, [ESP+24]
    MOV EAX, [ESP+28]

    INT 0x40

    POP EBX
    POP ESI
    POP EDI

    RET
   
api_close_win:   ;void api_closewin(int win);
    PUSH EBX
    
    MOV EDX, 12
    MOV EBX, [ESP+8]
    INT 0x40

    POP EBX
    RET

api_getkey:     ;int api_getkey(int mode)
    MOV EDX, 15
    MOV EAX, [ESP+4]

    INT 0x40
    RET
