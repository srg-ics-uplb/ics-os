/*
dumppci.c

Put online by David Cary near
  http://www.rdrop.com/~cary/html/device_driver.html#pci
.


Date: Thu, 08 Feb 1996 19:20:15 GMT
From: Tim Eccles <Tim@tile.demon.co.uk>
Subject: Re: utility to check for the existence of PCI
X-Mailing-List: <pci-sig@znyx.com> archive/latest/2148
To: Mailing List Recipients <pci-sig-request@znyx.com>

In message <199602022222.OAA01057@dynip40.efn.org> Rick Bronson writes:
> 
> Hi,
>   Does anyone have a utility to check for the existence of PCI and if
> it exists show all Vendor ID's/Device ID's for each PCI card?
> 
>   Thanks
> 
>   Rick
> 

Here are some simple routines - they worked OK for me.  This stuff
was compiled with an old Borland C++ compiler.  Hope it saves someone
some typing.

There are three bits - the original names are PCIBIOS.H, PCIBIOS.C,
and DUMPPCI.C

DUMPPCI.C is the mainline

*******/


// DumpPCI.C

#include <stdio.h>
#include "pcibios.h"

int MaxDevice = 3;      // 15 or 20 on a real PCI bus
const int Bus = 0;
const int Function = 0;

BOOL DumpBiosInfo(void)
        {
        BYTE    HardwareFlags,
                MajorVersion,
                MinorVersion,
                MaxBus;

        BOOL PciPresent = ReadPciBiosInfo(HardwareFlags,
                                MajorVersion, MinorVersion, MaxBus);
        if (PciPresent)
                {
                printf("\nPCI BIOS matches PCI Spec %d.%d",
                                        MajorVersion,MinorVersion);
                printf("\n%d PCI buses and bridged buses installed.",MaxBus+1);
                printf("\nConfiguration access mechanisms: ");
                printf("1%ssupported. ",((HardwareFlags>>0)&1)?" ":" NOT ");
                printf("2%ssupported. ",((HardwareFlags>>1)&1)?" ":" NOT ");
                printf("\nSpecial Cycle mechanisms: ");
                printf("1%ssupported. ",((HardwareFlags>>4)&1)?" ":" NOT ");
                printf("2%ssupported. ",((HardwareFlags>>5)&1)?" ":" NOT ");
                if (HardwareFlags & 1)
                        MaxDevice = 20;         // mechanism 1: AD lines 11..31
                if (HardwareFlags & 2)
                        MaxDevice = 15;         // mechanism 2: AD lines 16..31
                }
           else printf("\nCould not detect PCI BIOS on this computer.");
        return PciPresent;
        }

void DumpDevice(int Device)
        {
        WORD    VendorId,
                DeviceId,
                Command,
                Status,
                Zero28;
        BYTE    RevisionId,
                ClassPi,
                ClassSub,
                ClassBase,
                CacheSize,
                LatencyTimer,
                HeaderType,
                BIST,
                IntLine,
                IntPin,
                MinGnt,
                MaxLat;

        printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        printf("\nDevice %d...  ",Device);
        if (!ReadPciConfigWord(Bus,Function,Device,PCI_CFG_RESERVED_28,Zero28))
                {
                printf("failed to read configuration registers.");
                return;
                }
        if (Zero28!=0)          // 0x28,2c,34,38 MUST be 0. usually 0xff if no device.
                {
                printf("not installed.");
                return;
                }
        ReadPciConfigWord(Bus,Function,Device,PCI_CFG_VENDOR_ID,VendorId);
        ReadPciConfigWord(Bus,Function,Device,PCI_CFG_DEVICE_ID,DeviceId);
        printf(" Vendor ID = %#x.  Device ID = %#x.",VendorId,DeviceId);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_REV_ID ,RevisionId);
        printf(" Revision ID = %#x.",RevisionId);
        ReadPciConfigWord(Bus,Function,Device,PCI_CFG_COMMAND,Command);
        ReadPciConfigWord(Bus,Function,Device,PCI_CFG_STATUS ,Status);
        printf("\nCommand = %#x. Status = %#x.",Command,Status);
        printf("\n Cmd   I/O Space          : %d.",(Command>>0) & 1);
        printf("   Cmd   Memory Space       : %d.",(Command>>1) & 1);
        printf("\n Cmd   Bus Master         : %d.",(Command>>2) & 1);
        printf("   Cmd   Special Cycles     : %d.",(Command>>3) & 1);
        printf("\n Cmd   Mem Write&Inv.     : %d.",(Command>>4) & 1);
        printf("   Cmd   VGA Palette Snoop  : %d.",(Command>>5) & 1);
        printf("\n Cmd   Parity Error Detect: %d.",(Command>>6) & 1);
        printf("   Cmd   Wait Cycle Control : %d.",(Command>>7) & 1);
        printf("\n Cmd   SERR# Enable       : %d.",(Command>>8) & 1);
        printf("   Cmd   Fast Bk2Bk Ena.    : %d.",(Command>>9) & 1);
        printf("\n Status   Fast Bk2Bk Capable   : %d.",(Status>> 7) & 1);
        printf("   Status   Data Parity Detected : %d.",(Status>> 8) & 1);
        printf("\n Status   DEVSEL Timing        : %d.",(Status>> 9) & 3);
        printf("   Status   Signalled Tgt Abrt   : %d.",(Status>>11) & 1);
        printf("\n Status   Received Tgt Abrt    : %d.",(Status>>12) & 1);
        printf("   Status   Received Mstr Abrt   : %d.",(Status>>13) & 1);
        printf("\n Status   Signalled Sys Err    : %d.",(Status>>14) & 1);
        printf("   Status   Detected Par Error   : %d.",(Status>>15) & 1);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_CLASS_PI,ClassPi);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_CLASS_SUB,ClassSub);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_CLASS_BASE,ClassBase);
        printf("\nClass Code = %02x,%02x,%02x. ",ClassBase,ClassSub,ClassPi);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_CACHE_SIZE,CacheSize);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_LATENCY_TIMER,LatencyTimer);
        printf("\nCache Line Size = %d. Latency Timer = %d.",CacheSize,LatencyTimer);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_HEADER_TYPE,HeaderType);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_BIST,BIST);
        printf(" Header Type = %#2x. BIST = %#2x.",HeaderType, BIST);
        for (int BaseNum=0;BaseNum<=5;BaseNum++)
                {
                WORD BaseLo,BaseHi;
                ReadPciConfigWord(Bus,Function,Device,PCI_CFG_BASE+BaseNum*4  ,BaseLo);
                ReadPciConfigWord(Bus,Function,Device,PCI_CFG_BASE+BaseNum*4+2,BaseHi);
                DWORD BaseAddr = BaseLo | (((DWORD)BaseHi)<<16);
                if ((BaseNum & 1)==0)
                        printf("\n");
                printf("\tBase Address %d = %#lx.",BaseNum,BaseAddr);
                }
        WORD RomLo,RomHi;
        ReadPciConfigWord(Bus,Function,Device,PCI_CFG_ROM_BASE  ,RomLo);
        ReadPciConfigWord(Bus,Function,Device,PCI_CFG_ROM_BASE+2,RomHi);
        DWORD RomAddr = RomLo | (((DWORD)RomHi)<<16);
        printf("\n\tEPROM Base Address = %#lx.",RomAddr);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_INT_LINE,IntLine);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_INT_PIN,IntPin);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_MIN_GNT,MinGnt);
        ReadPciConfigByte(Bus,Function,Device,PCI_CFG_MAX_LAT,MaxLat);
        printf("\nInt Line = %d. Int Pin = %d.",IntLine,IntPin);
        printf(" Min Gnt = %d. Max Lat = %d.",MinGnt,MaxLat);
        }

#define BANNER  "PCI BIOS Dump. TE. Version: "##__DATE__##", "##__TIME__
int main(int argc,char* argv[])
        {
        printf(BANNER);
        printf("    syntax: DUMPPCI [slot]");
        if (DumpBiosInfo())
                {
                if (argc<2)
                        {
                        for (int Device=0;Device<=MaxDevice;Device++)
                                DumpDevice(Device);
                        }
                   else {
                        int Device;
                        sscanf(argv[1],"%d",&Device);   // no error checking!
                        DumpDevice(Device);
                        }
                return 0;
                }
           else return 1;
        }
