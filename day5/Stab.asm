[BITS 32]

GLOBAL load_gdtr, load_idtr

[SECTION .text]
load_gdtr:              ;void load_gdtr(int limit, int addr);
    MOV AX,[ESP+4]
    MOV [ESP+6],AX
    LGDT    [ESP+6]
    RET


load_idtr:             ;void load_idtr(int limit, int addr);
    MOV AX,[ESP+4]
    MOV [ESP+6],AX
    LGDT    [ESP+6]
    RET

   
