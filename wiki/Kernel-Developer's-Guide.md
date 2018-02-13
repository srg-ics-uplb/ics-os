(Work in progress...)

# 1. Introduction
This guide is for software developers who would like to work on the source code of
ics-os. The suggested development platform is a linux box with the following tools installed.

  * make
  * gcc(v4.8)/tcc
  * GNU binutils (ld, strip)
  * mount
  * bochs/qemu
  * nasm
  * git

Instructions for installing the above packages differ from one linux distribution to another. Consult the documentation for the distribution that you use. (NOTE: This guide assumes a **64-bit Ubuntu 16.04** development machine).

For Ubuntu users, the following commands will install the required packages.
```
$sudo apt-get update
$sudo apt-get install build-essential nasm qemu-kvm tcc git gcc-multilib
```

The main target audience for this guide are students learning systems programming and OS kernel programming.

# 2. Obtaining the Source Code
```
$git clone https://github.com/srg-ics-uplb/ics-os.git
```

# 3. Building the Source Code
Building the source code for the kernel and the distribution disk is accomplished using ` make `. Make sure you perform steps 2-4 every time you make changes in the source code.

  1) Next, go inside the directory of the extracted source.
```
$cd ics-os/ics-os
```
  2) Remove binary files.
```
$make clean
```
  3) Build the kernel.
```
$make
```
  4) Create the distribution floppy image. Make sure that you have root privileges(use the `su` or `sudo` command).
```
$sudo make install
```
  5) Test the distribution floppy image. This does not require root privileges.
```
$qemu-system-i386 -fda ics-os-floppy.img -boot a
```

# 4. Source Code Directory Structure
Top level directories.

| **Directory** | **Description** |
|:--------------|:----------------|
|`apps/`        |Executables of application programs |
|`apps-old/`    |Executables of old application programs|
|`base/`        |Contains files that will be on the root directory of the floppy distribution|
|`boot/`        |Contains files for grub|
|`contrib/`     |Sources for applications|
|`kernel/`      |Kernel sources directory|
|`lib/`         |Binaries of extension modules|
|`mnt/`         |Temporary folder for mounting the floppy image when creating the distribution|
|`sdk/`         |Libraries for application development|

Kernel source directories.

| **Directory** | **Description** |
|:--------------|:----------------|
|`console/`     |Sources for the main shell|
|`devmgr/`      |Sources for the device and extension manager|
|`dexapi/`      |Sources for setting up the system call table|
|`docs/`        |Documentation files for kernel|
|`filesystem/`  |Sources for filesystem support (fat12 and iso9660)|
|`grub/`        |Files needed by grub|
|`hardware/`    |Sources for hardware device drivers|
|`iomgr/`       |Input/Output manager code|
|`memory/`      |Memory management routines|
|`mnt/`         |Temporary mount directory|
|`module/`      |Implementation of supported executable file formats (PE, ELF)|
|`process/`     |Process management routines|
|`startup/`     |Contains startup routines after bootloader finishes(enables 32-bit protected mode|
|`stdlib/`      |Standard library routines|
|`vfs/`         |Virtual File System implementation|
|`vmm/`         |Virtual Memory Management implementation|


# 5. A Hello World Example
This section describes an example on how to modify ics-os, specifically the kernel by adding a `hello` command which displays a message. After extracting the sources, open the file `kernel/console/console.c` on a text editor from the top level directory of the extracted source. Locate the function
`int console_execute(const char *str)`. Find the code fragment before the START comment line in the code fragment below. Then insert the code fragment between the START and END comment lines on the location as shown below. Note that the code fragment between the START and END comment lines is not present on the original body of the function.

```
    //check if a pathcut command was executed
    if (u[command_length - 1] == ':') 
                {
                    char temp[512];
                    sprintf(temp,"cd %s",u);            
                    console_execute(temp); 
                }
                else
    /*----------------------START------------------*/
    if (strcmp(u,"hello")==0)
                {
                   printf("Hello World command!\n");
                }
		else
    /*-----------------------END------------------*/
    if (strcmp(u,"fgman")==0)
                {
                    fg_set_state(1);
                }

```

Perform steps 2-4 of Section 3 to build the source. You should see something similar to the figure below after typing `hello` on the command prompt and pressing enter.

![http://ics-os.googlecode.com/svn/trunk/ics-os/kernel/docs/figure01.png](http://ics-os.googlecode.com/svn/trunk/ics-os/kernel/docs/figure01.png)

# 6. Understanding the Kernel Makefile
In order to create the floppy distribution image of ics-os, it uses the `make` utility
to build the sources. For a detailed explanation of this utility, please read the <a href='http://www.gnu.org/software/make/manual/make.html'>GNU Make</a> manual. The primary input to `make` is a makefile. In ics-os, there are two makefiles, `Makefile` and `kernel/Makefile`. A simple makefile is composed of rules with the following syntax.
```
target ... : prerequisites ...
             command
             ...
             ...
```
Shown below are the contents of `kernel/Makefile` (see source code for updated version).
```
CC=gcc
CFLAGS=-w  -nostdlib -fno-builtin -ffreestanding -c
ASM=nasm
ASMFLAGS=-f elf

bzImage: all
	gzip -c -9 Kernel32.bin >  vmdex
	cp vmdex ..

all: obj Kernel32.bin

obj: scheduler.o fat.o iso9660.o devfs.o iomgr.o devmgr_error.o kernel32.o \
		startup.o asmlib.o irqwrap.o 
	strip --strip-debug *.o
		
kernel32.o: kernel32.c build.h
	$(CC) $(CFLAGS) -o kernel32.o kernel32.c 
	
scheduler.o:
	$(CC) $(CFLAGS) -o scheduler.o process/scheduler.c

fat.o:	
	$(CC) $(CFLAGS) -o fat.o filesystem/fat12.c
	
iso9660.o:
	$(CC) $(CFLAGS) -o iso9660.o filesystem/iso9660.c
	
devfs.o:
	$(CC) $(CFLAGS) -o devfs.o filesystem/devfs.c
	
iomgr.o:
	$(CC) $(CFLAGS) -o iomgr.o iomgr/iosched.c
	
devmgr_error.o:
	$(CC) $(CFLAGS) -o devmgr_error.o devmgr/devmgr_error.c

startup.o:
	$(ASM) $(ASMFLAGS) -o startup.o startup/startup.asm
	
asmlib.o:
	$(ASM) $(ASMFLAGS) -o asmlib.o startup/asmlib.asm 

irqwrap.o:
	$(ASM) $(ASMFLAGS) -o irqwrap.o irqwrap.asm
	
Kernel32.bin:
	ld -T lscript.txt -Map mapfile.txt

clean:
	rm -f *.o
	rm -f Kernel32.bin
	rm -f vmdex
```

When you run `make`, what happens is that the target `bzImage` is processed first because it is the very first target. The prerequisite for this target is also a target, `all`. Thus, the target `all` will be processed first before the commands for the target `bzImage` are executed. If you look at the target `all`, notice that the prerequisites are also targets and thus will be processed first. The processing is thus recursive. Majority of the commands for the targets invoke the C compiler and assembler defined as variables at the start of the makefile. For example the command for the `scheduler.o` target becomes `gcc -w -nostdlib -fno-builtin -ffreestanding -c -o scheduler.o process/scheduler.c` when executed.

Let us focus our attention on the `Kernel32.bin` target which is the target for creating the final kernel image. Unlike the other targets, the command for this target invokes the linker `ld`. Detailed information on the `ld` command is available <a href='http://sourceware.org/binutils/docs/ld/index.html'>here</a>. Basically, what a linker does is to combine several input files and archives into a single output file. When you compile a program, the final step is usually to invoke the linker. In the case of ics-os, it is composed of several object files (those targets ending in .o). The single kernel image file (Kernel32.bin) is created by invoking the `ld` command. The file `lscript.txt` is the linker script that describes how the output file is to be created. The contents of lscript.txt is shown below.
```
OUTPUT_FORMAT("elf32-i386")
ENTRY(startup)
SECTIONS {
  .text 0x00100000 :{
    *(.text)
  }
  textEnd = .;
  .data :{
    *(.data)
    *(.rodata)
  }
  dataEnd = .;
  .bss :{
    *(.common)
    *(.bss)
  }
  bssEnd = .;
}
INPUT(startup.o asmlib.o kernel32.o scheduler.o iomgr.o fat.o iso9660.o
      devfs.o irqwrap.o devmgr_error.o)
OUTPUT(Kernel32.bin)
```
The first line of the linker script specifies the type of executable to produce, elf32-i386. There are several executable file formats available but in the case of ics-os, we want to use ELF which is used in linux. The second line specifies `startup` as the entry point for the operating system to begin its execution. The entry point is a symbol (a label in assembly) to jump to. This symbol is defined in the file `kernel/startup/startup.asm`. `SECTIONS` specify the memory area where the instructions(.text) and data(.data) will be placed which in ics-os case is at memory location 0x00100000. The `INPUT` section specifies the input files which are the object files and the `OUTPUT` section specifies the output kernel image file. After linking, the linker generates a map file that summarizes how it created the output file. A portion of the generated mapfile(`mapfile.txt`) is shown below.
```
.text           0x0000000000100000    0x270d4
 *(.text)
 .text          0x0000000000100000      0x2d2 startup.o
                0x00000000001002ca                reset_gdtr
                0x0000000000100000                startup
 *fill*         0x00000000001002d2        0xe 00
 .text          0x00000000001002e0      0x5fe asmlib.o
                0x00000000001003f3                pci_writeconfigdword
                0x00000000001003a7                pci_writeconfigbyte
                0x00000000001005dc                refreshpages

```

The final kernel image `Kernel32.bin` is then gzipped into `vmdex` to conserve space. Control is transferred to this image after GRUB has loaded.



# 7. startup.asm
The file [`kernel/startup/startup.asm`](https://github.com/srg-ics-uplb/ics-os/blob/devel/ics-os/kernel/startup/startup.asm) enables the 32-bit protected mode of x86, enables the A20 line, and transfers control to the `main()` function in `kernel32.c`. [Here](http://www.brokenthorn.com/Resources/OSDev8.html) is a link to a more detailed discussion of protected mode.

# 8. kernel32.c
The file  [`kernel/kernel32.c`](https://github.com/srg-ics-uplb/ics-os/blob/devel/ics-os/kernel/kernel32.c) is the main entry point of the ics-os. The following steps are performed in `main()`

  1. Program IRQ lines for timer, keyboard, and floppy 
  1. Set up the interrupt descriptor table 
  1. Obtain boot device and memory information from GRUB
  1. Initialize memory subsystem 
  1. Sets the current process to the kernel process `_sPCB_`. This structure will be initialized at a later stage
  1. Setup context switch timer 
  1. Initialize bridge manager
  1. Initialize virtual console manager
  1. Initialize kernel virtual console for kernel messages

After the above operations, memory access should be saved. Control is transferred to the `dex32_startup()` function.

  1. Print CPU information
  1. Print available memory
  1. Initialize extension manager
  1. Initialize device manager
  1. Register memory manager and memory allocator
  1. Initialize `malloc()` provider
  1. Initialize ports
  1. Initialize kernel api
  1. Initialize process manager and start the task switcher

The task switcher calls the `dex_init()` function which is essentially the first "process" that is executed. It performs the following operations
  1. Initialize the keyboard
  1. Installs the floppy driver
  1. Initialize the ide driver
  1. Initialize the vga driver
  1. Initialize the I/O manager
  1. Initialize the virtual file system
  1. Initialize the task manager
  1. Initialize the Disk I/O manager
  1. Initialize null block device
  1. Initialize device filesystem driver
  1. Install the FAT12 filesystem driver
  1. Install the  ISO9660 filesystem driver
  1. Mount the floppy device
  1. Initialize module loader
  1. Run foreground manager thread
  1. Create a new instance of console
  1. Start the process dispatcher
