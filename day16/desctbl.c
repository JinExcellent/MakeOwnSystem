#include "desctbl.h"
#include "int.h"
#include "asmfunc.h"
#include "api.h"

void init_gdtidt(void) {
  struct SegmentDescriptor *gdt = (struct SegmentDescriptor *)ADR_GDT;
  struct GateDescriptor *idt = (struct GateDescriptor *)ADR_IDT;

  for (int i = 0; i <= LIMIT_GDT / 8; i++) {        //由于每个段表项占8字节，为计算出段表项数量，故/8
    set_segmdesc(gdt + i, 0, 0, 0);
  }

  set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);              //系统启动所用的1MB空间
  set_segmdesc(gdt + 2, LIMIT_BOOTPACK, ADR_BOOTPACK, AR_CODE32_ER);        //系统内核所用的段空间
  load_gdtr(LIMIT_GDT, ADR_GDT);

  for (int i = 0; i <= LIMIT_IDT / 8; i++) {       //与段表相同，中断向量表项也占8字节
    set_gatedesc(idt + i, 0, 0, 0);
  }
  load_idtr(LIMIT_IDT, ADR_IDT);

  //*************************************要清楚为何这里都设置为相同的段选择子
  set_gatedesc(idt + 0x0d, (int)asm_INT_handler0d, 2 * 8, AR_INTGATE32);        
  set_gatedesc(idt + 0x20, (int)asm_INT_handler20, 2 * 8, AR_INTGATE32);        
  set_gatedesc(idt + 0x21, (int)asm_INT_handler21, 2 * 8, AR_INTGATE32);
  set_gatedesc(idt + 0x27, (int)asm_INT_handler27, 2 * 8, AR_INTGATE32);
  set_gatedesc(idt + 0x2c, (int)asm_INT_handler2c, 2 * 8, AR_INTGATE32);
  set_gatedesc(idt + 0x40, (int)asm_hrb_api, 2 * 8, AR_INTGATE32 + 0x60);   // +0x60是为了设置成用户段(可查看GDT表权限)
}

void set_segmdesc(struct SegmentDescriptor *sd, unsigned int limit, int base, int ar) {
  if (limit > 0xfffff) {
    ar |= 0x8000; // G_bit = 1,当段大小超过1MB时，采用按页为单位进行分配（4kB）
    limit /= 0x1000;
  }

  sd->limit_low = limit & 0xffff;
  sd->base_low = base & 0xffff;
  sd->base_mid = (base >> 16) & 0xff;
  sd->access_right = ar & 0xff;
  sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) | 0xf0);     //将limit的4位地址位和扩展访问权限一同放入limit_high
  sd->base_high = (base >> 24) & 0xff;
}

void set_gatedesc(struct GateDescriptor *gd, int offset, int selector, int ar) {
  gd->offset_low = offset & 0xffff;
  gd->selector = selector;
  gd->dw_count = (ar >> 8) & 0xff;
  gd->access_right = ar & 0xff;
  gd->offset_high = (offset >> 16) & 0xffff;
}
