[BITS 32]

GLOBAL api_putch, api_end

api_putch:      ;void api_putch(int c)
    MOV EDX, 1
    MOV AL, [ESP + 4]
    INT 0x40
    RET

api_end:
    MOV EDX, 4
    INT 0x40
