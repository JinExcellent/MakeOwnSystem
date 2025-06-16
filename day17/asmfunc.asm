[BITS 32]

;相当于创建了一个c可调用的全局函数
GLOBAL io_hlt               ;void io_hlt(void);
GLOBAL io_cli, io_sti, io_stihlt
GLOBAL io_in8, io_in16, io_in32
GLOBAL io_out8, io_out16, io_out32
GLOBAL io_load_eflags, io_store_eflags 
GLOBAL io_load_cr0, io_store_cr0
GLOBAL memtest_sub
GLOBAL load_tr, far_jmp, far_call
GLOBAL asm_cons_putchar
GLOBAL start_app
GLOBAL asm_end_app

EXTERN cons_putchar

[SECTION .text]         ;向进程的.text段中写入函数指令
io_hlt:
    HLT
    RET

io_cli:
    CLI
    RET

io_sti:
    STI
    RET

io_stihlt:
    STI
    HLT
    RET

io_in8:
    MOV EDX,[ESP+4]
    MOV EAX,0
    IN AL,DX
    RET

io_in16:
    MOV EDX,[ESP+4]
    MOV EAX,0
    IN AX,DX
    RET

io_in32:
    MOV EDX,[ESP+4]
    IN EAX,DX
    RET

io_out8:
    MOV EDX,[ESP+4]
    MOV AL,[ESP+8]
    OUT DX,AL
    RET

io_out16:
    MOV EDX,[ESP+4]
    MOV AX,[ESP+8]
    OUT DX,AX
    RET

io_out32:
    MOV EDX,[ESP+4]
    MOV EAX,[ESP+8]
    OUT DX,EAX
    RET

io_load_eflags:
    PUSHFD      ;压入到内存栈
    POP EAX
    RET

io_store_eflags:
    MOV EAX, [ESP+4]
    PUSH EAX
    POPFD       ;从栈弹出到ELFIG寄存器中
    RET

io_load_cr0:
    MOV EAX, CR0
    RET

io_store_cr0:
    MOV EAX, [ESP+4]
    MOV CR0, EAX
    RET

memtest_sub:
    PUSH EDI
    PUSH ESI
    PUSH EBX
    MOV ESI, 0xaa55aa55
    MOV EDI, 0x55aa55aa
    MOV EAX, [ESP+12+4]          


    ;取函数第一个参数,这里取参数+12的原因是该函数调用前已将三个寄存器值保存在栈中，导致栈顶指针向下移动了12个字节
    ;完整的函数调用栈图：
    
    ;...                         <- 高地址
    ;[esp+24] -> end
    ;[esp+20] -> start
    ;[esp+16] -> 返回地址
    ;[esp+12] -> 保存的 EDI
    ;[esp+8 ] -> 保存的 ESI
    ;[esp+4 ] -> 保存的 EBX     <- 当前栈顶指针位置



 mts_loop:
    MOV EBX, EAX
    ADD EBX, 0xffc
    MOV EDX, [EBX]
    MOV [EBX], ESI
    
    XOR DWORD [EBX], 0Xffffffff
    CMP EDI, [EBX]
    JNE mts_fin
    
    XOR DWORD [EBX], 0xffffffff
    CMP ESI, [EBX]
    JNE mts_fin
    MOV [EBX], EDX
    
    ADD EAX, 0x1000
    CMP EAX, [ESP+12+8]
    JBE mts_loop

    POP EBX
    POP ESI
    POP EDI

    RET 

 mts_fin:
    MOV [EBX], EDX
    POP EBX
    POP ESI
    POP EDI
    RET

 
 load_tr:       ;void load_tr(int tr);
    LTR [ESP+4]
    RET

;JMP FAR用于跨段跳转（即跳转到另一个代码段），它会同时修改 CS（代码段寄存器）和 EIP（指令指针） 
 far_jmp:                ;void farjmp(int eip, int cs);
    JMP FAR [ESP+4]     ;eip:cs 先读取第一个参数的四字节，在向后读取两个字节的段号
    RET

 far_call:              ;void far_call(int eip, int cs);
    CALL FAR [ESP+4]
    RET

 asm_cons_putchar:
    STI                     ;在显示字符时允许cpu响应其它中断(这个API是被写入到中断表中的，而cpu在处理中断时默认为CLI)
    PUSHAD                  
    PUSH 1
    AND EAX, 0xff           ;高位置为0并将已存入的字符入栈
    PUSH EAX
    PUSH DWORD [0x0fec]     ;将指定地址中的数据入栈
    CALL cons_putchar
    ADD ESP, 12             ;调用完函数后返回后，清空栈
    POPAD
    IRETD

;关于这段指令的详细讲解，见书444页笔记
start_app:      ;void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
    PUSHAD
    MOV     EAX, [ESP+36]   ; EIP
    MOV     ECX, [ESP+40]   ; CS
    MOV     EDX, [ESP+44]   ; ESP
    MOV     EBX, [ESP+48]   ; DS/SS
    MOV     EBP, [ESP+52]   ; tss.esp0
    MOV     [EBP], ESP
    MOV     [EBP+4], SS
    MOV     ES, BX
    MOV     DS, BX
    MOV     FS, BX
    MOV     GS, BX

    OR      ECX, 3
    OR      EBX, 3
    PUSH    EBX
    PUSH    EDX
    PUSH    ECX
    PUSH    EAX
    RETF 

asm_end_app:
    MOV     ESP, [EAX]          ;EAX中保存的任务的tss结构体重的esp0值
    MOV     DWORD [EAX+4], 0    ;将tss结构体中的ss0写入0
    POPAD
    RET
