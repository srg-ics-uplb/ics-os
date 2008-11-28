/*
pcibios.c

Date: Thu, 08 Feb 1996 19:20:15 GMT
From: Tim Eccles <Tim@tile.demon.co.uk>

*******/

// PCIBIOS.C

#include <dos.h>
#include "pcibios.h"

BOOL ReadPciBiosInfo(BYTE& HardwareFlags,
                        BYTE& MajorVersion,
                        BYTE& MinorVersion,
                        BYTE& MaxBus)
        {
        union REGS regs;

        regs.x.ax = 0xb101;             // check PCI BIOS present
        int86(0x1a, &regs, &regs);
                                        // also ah=0
        HardwareFlags = regs.h.al;      //      al=hardware characteristics
        MajorVersion  = regs.h.bh;      //      bh=PCI major version
        MinorVersion  = regs.h.bl;      //      bl=PCI minor version
        MaxBus        = regs.h.cl;      //      cl=number of last bus in system
                                        //      edi=addr of protected mode entry point
        return ((regs.x.cflag==0)       // carry clear
                &&(regs.x.dx==0x4350)); // partial check of
                                        //     EDX = 2049_4350 = 'ICP'
        }


BOOL ReadPciConfigByte(BYTE Bus,BYTE Function,BYTE Device,BYTE Index,BYTE& PciByte)
        {
        union REGS regs;

        regs.x.ax = 0xb108;             // read config byte
        regs.h.bh = Bus;
        regs.h.bl = (Device<<3) | Function;
        regs.x.di = Index;
        int86(0x1a, &regs, &regs);

        PciByte = regs.h.cl;
        return (regs.x.cflag==0);
        }

BOOL WritePciConfigByte(BYTE Bus,BYTE Function,BYTE Device,BYTE Index,BYTE PciByte)
        {
        union REGS regs;

        regs.x.ax = 0xb10b;             // write config byte
        regs.h.bh = Bus;
        regs.h.bl = (Device<<3) | Function;
        regs.x.di = Index;
        regs.h.cl = PciByte;
        int86(0x1a, &regs, &regs);

        return (regs.x.cflag==0);
        }

BOOL ReadPciConfigWord(BYTE Bus,BYTE Function,BYTE Device,BYTE Index,WORD& PciWord)
        {
        union REGS regs;

        regs.x.ax = 0xb109;             // read config word
        regs.h.bh = Bus;
        regs.h.bl = (Device<<3) | Function;
        regs.x.di = Index;
        int86(0x1a, &regs, &regs);

        PciWord = regs.x.cx;
        return (regs.x.cflag==0);
        }

BOOL WritePciConfigWord(BYTE Bus,BYTE Function,BYTE Device,BYTE Index,WORD PciWord)
        {
        union REGS regs;

        regs.x.ax = 0xb10c;             // write config word
        regs.h.bh = Bus;
        regs.h.bl = (Device<<3) | Function;
        regs.x.di = Index;
        regs.x.cx = PciWord;
        int86(0x1a, &regs, &regs);

        return (regs.x.cflag==0);
        }
