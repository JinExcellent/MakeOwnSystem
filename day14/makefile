IMG := MirrorProcess.img
AS := nasm
GXX := gcc 
LD := ld
CFLAGS := -Wall -m32  
CFLAGS += -fno-pie -no-pie 
CFLAGS += -ffunction-sections -nostdinc -nostdlib -fno-builtin -ffreestanding
CFLAGS += -I ./include -I ../libc/include
CFLAGS += -g
QEMU_FLAGS += -no-reboot -d in_asm -m size=128		#这里给QEMU分配了128MB内存
#OBJS := IPL.bin OS.bin
K_OBJS := bootpack.bin asmfunc.bin Stab.bin hankaku.bin desctbl.bin graphic.bin int.bin INThandler.bin fifo.bin keyboard.bin mouse.bin memory.bin sheet.bin window.bin timer.bin task.bin
L_OBJS := ../libc/stdio/stdio.bin ../libc/string/strcmp.bin ../libc/string/strncmp.bin ../libc/stdlib/rand.bin ../libc/ssp/ssp.bin

IPL.bin:
	$(AS) -f bin IPL.asm -o IPL.bin -l IPL.lst

OS.bin:
	$(AS) -f bin OS.asm -o OS.bin -l OS.lst

hankaku.bin: hankaku.asm
	$(AS) -f elf $< -o $@

%.bin: %.asm
	$(AS) -f elf $< -o $@ -l $(subst .asm,.lst,$<)

%.bin: %.c
	$(GXX) $(CFLAGS) -c $< -o $@

kernel.sys: ${K_OBJS} ${L_OBJS}
	$(LD) -m elf_i386 --oformat binary -o kernel.sys -T kernel.ld $^

test_os.sys: OS.bin kernel.sys
	cat OS.bin > test_os.sys
	cat kernel.sys >> test_os.sys

image: IPL.bin test_os.sys
	dd	if=/dev/zero of=$(IMG) bs=512 count=2880
	dd	if=IPL.bin of=$(IMG) bs=512 count=1 conv=notrunc
	dd	if=test_os.sys of=$(IMG) seek=33 bs=512 conv=notrunc

all: ${OBJS} test_os.sys image

qemu:
	qemu-system-i386 -fda $(IMG) 
	#qemu-system-i386 -fda $(IMG) -gdb tcp:1234 -S

clean:
	rm -f *.bin *.lst *.sys $(IMG)

.PHONY:
	all
