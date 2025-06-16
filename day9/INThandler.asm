;中断处理程序
[BITS 32]

GLOBAL asm_INT_handler21, asm_INT_handler27, asm_INT_handler2c

EXTERN INT_handler21, INT_handler27, INT_handler2c

asm_INT_handler21:
    PUSH    ES          ;处理中断前
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    INT_handler21       ;调用中断处理函数
    POP     EAX                 ;中断返回
    POPAD   
    POP     DS
    POP     ES
    IRETD

asm_INT_handler27:
    PUSH    ES          ;处理中断前
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    INT_handler27       ;调用中断处理函数
    POP     EAX                 ;中断返回
    POPAD   
    POP     DS
    POP     ES
    IRETD

asm_INT_handler2c:
    PUSH    ES          ;处理中断前
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    INT_handler2c       ;调用中断处理函数
    POP     EAX                 ;中断返回
    POPAD   
    POP     DS
    POP     ES
    IRETD

