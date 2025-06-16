#ifndef _INT_H_
#define _INT_H_

#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR  0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR  0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1

void init_pic(void);

void INT_handler27(int *esp);
int *INT_handler0d(int *esp);
int *INT_handler0c(int *esp);

void asm_INT_handler20(void);
void asm_INT_handler21(void);
void asm_INT_handler27(void);
void asm_INT_handler2c(void);
void asm_INT_handler0d(int *esp);
void asm_INT_handler0c(int *esp);

#endif // _INT_H_
