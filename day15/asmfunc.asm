[BITS 32]

;相当于创建了一个c可调用的全局函数
GLOBAL io_hlt               ;void io_hlt(void);
GLOBAL io_cli, io_sti, io_stihlt
GLOBAL io_in8, io_in16, io_in32
GLOBAL io_out8, io_out16, io_out32
GLOBAL io_load_eflags, io_store_eflags 
GLOBAL io_load_cr0, io_store_cr0
GLOBAL memtest_sub
GLOBAL load_tr, far_jmp

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

 
 far_jmp:                ;void farjmp(int eip, int cs);
    JMP FAR [ESP+4]     ;eip:cs
    RET



