IMG := haribote.img
AS := nasm
GXX := gcc 
LD := ld
QEMU := qemu-system-i386
QEMU_IMG := $(IMG)

CFLAGS := -Wall -Wno-format -Wno-unused -std=gnu99 -static -m32  
CFLAGS += -fno-pie 
CFLAGS += -ffunction-sections -nostdinc -nostdlib -fno-builtin -ffreestanding
CFLAGS += -I ./include -I ../libc/include

ASM_FLAGS :=

#-d in_asm 显示中断日志，需要时再加,这里给QEMU分配了128MB内存
QEMU_FLAGS += -no-reboot -m size=128

#APP
APP_CFLAGS = -Wall -Wno-format -Wno-unused -std=gnu99 -m32	-fno-pie
APP_LDFLAGS = -m elf_i386 --oformat elf32-i386 -e main

ifdef DEBUG
	QEMU_FLAGS += -gdb tcp::1234 -S
	CFLAGS += -g
	ASM_FLAGS += -g
endif


#kernel objects
#注意，C源文件和汇编文件实现的函数接口都需要编译成二进制文件后再进行链接
K_OBJS := bootpack.bin asmfunc.bin Stab.bin hankaku.bin desctbl.bin graphic.bin int.bin INThandler.bin fifo.bin keyboard.bin mouse.bin memory.bin sheet.bin window.bin timer.bin task.bin file.bin console.bin cmd.bin api.bin asm_api.bin

#lab
L_OBJS := ../libc/stdio/stdio.bin ../libc/string/strcmp.bin ../libc/string/strncmp.bin ../libc/stdlib/rand.bin ../libc/ssp/ssp.bin

#application
APPS := app/hello.hrb app/a.hrb

IPL.bin:
	$(AS) $(ASM_FLAGS) -f bin IPL.asm -o IPL.bin -l IPL.lst

OS.bin:
	$(AS) $(ASM_FLAGS) -f bin OS.asm -o OS.bin -l OS.lst

hankaku.bin: hankaku.asm
	$(AS) -f elf $< -o $@

%.bin: %.asm
	$(AS) $(ASM_FLAGS) -f elf $< -o $@ -l $(subst .asm,.lst,$<)

%.bin: %.c
	$(GXX) $(CFLAGS) -c $< -o $@

%.hrb: %.asm
	$(AS) $(ASM_FLAGS) -f bin $< -o $@  -l $(subst .asm,.lst,$<)

app/a_asm.o:
	$(AS) $(ASM_FLAGS) -f elf app/a_asm.asm -o app/a_asm.o

app/a.hrb: app/a_asm.o
	$(GXX) $(APP_CFLAGS) -c app/a.c -o app/a.o
	$(LD) $(APP_LDFLAGS) app/a.o app/a_asm.o -o app/a.hrb

#生成ELF文件用于GDB调试
kernel.elf: ${K_OBJS} ${L_OBJS}
	$(LD) -m elf_i386 -T kernel.ld -o kernel.elf $^

kernel.sys: ${K_OBJS} ${L_OBJS}
	$(LD) -m elf_i386 --oformat binary -Map kernel.map -o kernel.sys -T kernel.ld $^

haribote.sys: OS.bin kernel.sys
	cat OS.bin > haribote.sys
	cat kernel.sys >> haribote.sys

image: IPL.bin haribote.sys $(APPS)
	dd	if=/dev/zero of=$(IMG) bs=512 count=2880
	dd	if=IPL.bin of=$(IMG) bs=512 count=1 conv=notrunc
	#dd	if=test_os.sys of=$(IMG) seek=33 bs=512 conv=notrunc
	# 向镜像写入文件,自动创建并管理 FAT 文件系统
	# 注意这里的文件要分开写如，这里第一句偷懒，但第二个规则中APPS可能包含多个文件，故必须分开写到根目录中
	mcopy -i $(IMG) haribote.sys makefile IPL.asm ::/
	mcopy -i $(IMG) $(APPS) ::/

all: ${L_OBJS} ${OBJS} ${APPS} haribote.sys image

clean:
	rm -f *.elf
	rm -f *.bin 
	rm -f *.lst 
	rm -f *.sys 
	rm -f $(IMG)
	rm -rf app/*.hrb
	rm -rf app/*.lst
	rm -rf app/*.o

qemu: 
	$(QEMU) -drive file=$(IMG),format=raw,if=floppy $(QEMU_FLAGS)

.PHONY:
	all
