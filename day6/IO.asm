;[FORMAT "WCOFF"]       ;该指令为Windows特有的伪指令，在linux端下需要在nasm编译时通过-f选项指定文件输出格式为elf32
[BITS 32]

;该成勋相当于创建了一个c可调用的全局函数
GLOBAL io_hlt               ;void io_hlt(void);
GLOBAL io_cli, io_sti, io_stihlt
GLOBAL io_in8, io_in16, io_in32
GLOBAL io_out8, io_out16, io_out32
GLOBAL io_load_eflags, io_store_eflags 
;GLOBAL write_mem            ;void write_mem(int addr, int data);


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
    PUSHFD
    POP EAX
    RET

io_store_eflags:
    MOV EAX, [ESP+4]
    PUSH EAX
    POPFD
    RET


;这里需要自己去查阅相关的函数栈传参细节
;write_mem:                  
;    MOV ECX,[ESP+4]
    ;MOV AL,[ESP+8]
    ;MOV [ECX],AL
    ;RET
