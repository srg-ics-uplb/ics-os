all:	miniKernel.bin

miniKernel.bin:
	nasm -f elf start.asm
	gcc	-Xlinker -T -Xlinker minilscript.txt \
			-ffreestanding	\
			-fno-builtin -nostdlib	\
			-nostartfiles -s start.o \
			-o miniKernel	
