CC=tcc
ICSOS_ROOT=../..
KERNEL=$(ICSOS_ROOT)/kernel
SDK=../../sdk
CFLAGS=-w -nostdlib -fno-builtin -static -ffreestanding
LIBS=$(SDK)/tccsdk.c $(SDK)/libtcc1.c $(SDK)/crt1.c
EXE=lzozip.exe

$(EXE): lzozip.c
	$(CC) $(CFLAGS) -olzozip.exe lzozip.c minilzo.c $(LIBS)

install: $(EXE)
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall: $(EXE)
	rm $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm $(EXE)


