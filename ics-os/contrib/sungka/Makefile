CC=gcc
ICSOS_ROOT=../..
SDK=../../sdk
CFLAGS=-m32 -nostdlib -fno-builtin -static
LIBS=$(SDK)/tccsdk.c $(SDK)/libtcc1.c $(SDK)/crt1.c
EXE=sungka.exe

$(EXE): sungka.c
	$(CC)  $(CFLAGS) -o${EXE} sungka.c $(LIBS)

install: $(EXE)
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall:
	rm $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm $(EXE)
