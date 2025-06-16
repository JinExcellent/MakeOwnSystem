;中断处理程序
[BITS 32]

GLOBAL asm_INT_handler20, asm_INT_handler21, asm_INT_handler27, asm_INT_handler2c,asm_INT_handler0d

EXTERN INT_handler20, INT_handler21, INT_handler27, INT_handler2c,INT_handler0d

;异常处理函数
asm_INT_handler0d:
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX         ;作为INT_handler0d的参数进行传递
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX

    CALL    INT_handler0d
    CMP     EAX, 0
    JNE     .end_app
    POP     EAX
    POPAD   
    POP     DS
    POP     ES
    ADD     ESP, 4      ;这里+4的原因是，调用异常处理函数INT_handler0d为一个c函数，没有ret指令返回（该指令会从栈中弹出返回地址，但c中的return没有此功能），故需要收调整栈指针
    IRETD

.end_app
    MOV     ESP, [EAX]  ;[EAX]中保存的是用INT_handler0d函数传递的tss32.0esp值
    POPAD
    RET


asm_INT_handler20:
    PUSH    ES          ;处理中断前
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    INT_handler20       ;调用中断处理函数
    POP     EAX                 ;中断返回
    POPAD   
    POP     DS
    POP     ES
    IRETD


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

