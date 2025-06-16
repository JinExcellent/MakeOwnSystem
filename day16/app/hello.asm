[BITS 32]
MOV EDX, 2
MOV EBX, msg   ;这里写入字符串时没有指定段地址，是因为应用程序使用的是动态分配的内存,需要在程序写入到内存之后，再将程序在内存中的首地址进行回传(cmd_app函数)再进行使用     
INT 0x40

MOV EDX, 4
INT 0x40

msg:
    DB "hello" ,0



;[BITS 32]
    ;MOV AL, 'h'
    ;INT 0x40
    ;RETF

;[BITS 32]

;MOV ECX, msg
;MOV EDX, 1
;putloop:
    ;MOV AL, [CS:ECX]        ;疑问：我在向ECX写入字符串时，是如何确定ECX是在CS这个段中的？
    ;CMP AL, 0
    ;JE fin
    ;INT 0x40
    ;ADD ECX, 1
    ;JMP putloop

;fin:
    ;RETF

;msg:
    ;DB "hello", 0
;RETF
