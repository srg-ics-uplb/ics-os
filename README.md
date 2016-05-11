# CMSC125 project

## Developers:
[Matthew Marcos](https://github.com/matthewmarcos94)
[Ma. Alyssa Chelseah Blaquera](https://github.com/alyssachelseah)

## How to run
1. ```cd ics-os```
1. ```make && sudo make install```
1. ```qemu-system-i386 -fda ics-os-floppy.img```


## How to edit files:
```bash
# Where the console commands go
/ICS-OS/ics-os/kernel/console/console.c
```

### File IO
- vfs_core.c

```C

// fread
// fseek
// fclose
// openfilex
// fgets

// pcb_FILE *f;
// f = fopen("history.hs", "a");
// fprintf(f, "%s", str);
// fclose(f);

```

### Buffer IO

- dexio.c

```C

```



## About

Modern real-world operating systems are too complex to be taught to undergraduates and other instructional operating systems are not complete and usable and do not work on real hardware. By providing students with a _not so complex_ working operating system to play with, they will be able to appreciate and understand deeper the concepts underlying an operating system.

Thus, this project aims to develop a simple yet operational instructional operating system for teaching undergraduate operating systems courses. ICS-OS is a fork of <a href='http://sourceforge.net/projects/dex-os'>DEX-OS</a> by Joseph Dayo.

## Downloads

Latest floppy image: <a href='https://github.com/srg-ics-uplb/ics-os/raw/master/ics-os/ics-os-floppy.img'>ics-os-floppy.img</a>

The source code in .tar.gz format is availabe in the <a href='https://github.com/srg-ics-uplb/ics-os/releases'>releases</a> section.

## Development and Support
This project is maintained and used by the <a href='http://ics.uplb.edu.ph'>Institute of Computer Science</a>, <a href='http://www.uplb.edu.ph'>University of the Philippines Los Banos</a> for <a href='http://ics.uplb.edu.ph/courses/ugrad/cmsc/125'>CMSC 125</a>.

Dont forget to check the <a href="http://github.com/srg-ics-uplb/ics-os/wiki">Wiki</a>.

You can ask questions <a href="https://groups.google.com/forum/#!forum/ics-os">here</a>.
