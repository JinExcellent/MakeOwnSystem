[BITS 32]

  GLOBAL asm_hrb_api

  EXTERN hrb_api

asm_hrb_api:
  STI
  PUSH    DS
  PUSH    ES
  PUSHAD
  PUSHAD
  MOV     AX, SS
  MOV     DS, AX
  MOV     ES, AX
  CALL    hrb_api
  CMP     EAX, 0
  JNE     .end_app
  ADD     ESP, 32
  POPAD
  POP     ES
  POP     DS
  IRETD

.end_app:
  MOV     ESP, [EAX]
  POPAD
  RET

    ;讲解这里为何使用两次pushad
    ;假设中断前 ESP = 0x0010FFFC

    ;ESP = 0x0010FFBC ─┬─ 第二次 PUSHAD 保存的寄存器 (给 hrb_api 使用)
                 ;│
    ;ESP = 0x0010FFDC ─┬─ 第一次 PUSHAD 保存的寄存器 (用来恢复现场)
                 ;│
    ;ESP = 0x0010FFFC ─┘ 中断前 ESP

    ;从栈结构图中可以得知，两次pushad操作移动了两次栈指针，并没有第二次会覆盖第一次这种情况
    ;第一次PUSHAD：是为了处理中断前，先把当前寄存器状态全部保存起来，便于稍后恢复，确保中断透明。
    ;第二次PUSHAD：是为了调用一个 C 函数（hrb_api），而把寄存器的值当作参数压入栈中供它使用。
