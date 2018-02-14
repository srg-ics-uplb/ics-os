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
