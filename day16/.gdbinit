set architecture i386
target remote localhost:1234
add-symbol-file kernel.elf 0x00280000
