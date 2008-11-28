/*
pcibios.h
Software to display Vendor ID's/Device ID's on the PCI bus


Date: Thu, 08 Feb 1996 19:20:15 GMT
From: Tim Eccles <Tim@tile.demon.co.uk>

*******/

// pcibios.h

#ifndef _pcibios_h_
#define _pcibios_h_

#define FALSE           0
#define TRUE            1

typedef unsigned int    BOOL;
typedef unsigned char   BYTE;   // 8 bits
typedef unsigned short  WORD;   // 16 bits
typedef unsigned long   DWORD;  // 32 bits

#define PCI_CFG_VENDOR_ID           0x00
#define PCI_CFG_DEVICE_ID           0x02
#define PCI_CFG_COMMAND             0x04
#define PCI_CFG_STATUS              0x06
#define PCI_CFG_REV_ID              0x08
#define PCI_CFG_CLASS_PI            0x09
#define PCI_CFG_CLASS_SUB           0x0a
#define PCI_CFG_CLASS_BASE          0x0b
#define PCI_CFG_CACHE_SIZE          0x0c
#define PCI_CFG_LATENCY_TIMER       0x0d
#define PCI_CFG_HEADER_TYPE         0x0e
#define PCI_CFG_BIST                0x0f
#define PCI_CFG_BASE                0x10
#define PCI_CFG_BASE_0              0x10
#define PCI_CFG_BASE_1              0x14
#define PCI_CFG_BASE_2              0x18
#define PCI_CFG_BASE_3              0x1c
#define PCI_CFG_BASE_4              0x20
#define PCI_CFG_BASE_5              0x24
#define PCI_CFG_RESERVED_28         0x28
#define PCI_CFG_RESERVED_2c         0x2c
#define PCI_CFG_ROM_BASE            0x30
#define PCI_CFG_RESERVED_34         0x34
#define PCI_CFG_RESERVED_38         0x38
#define PCI_CFG_INT_LINE            0x3c
#define PCI_CFG_INT_PIN             0x3d
#define PCI_CFG_MIN_GNT             0x3e
#define PCI_CFG_MAX_LAT             0x3f

#define PCI_CFG_MEM_RTR_BASE        PCI_CFG_BASE_0
#define PCI_CFG_IO_RTR_BASE         PCI_CFG_BASE_1
#define PCI_CFG_LOCAL_BASE          PCI_CFG_BASE_2

BOOL ReadPciBiosInfo(BYTE *HardwareFlags,
 						 	BYTE *MajorVersion,
                     BYTE *MinorVersion,
                     BYTE *MaxBus);
BOOL ReadPciConfigWord  (BYTE Bus,
                         BYTE Function,
                         BYTE Device,
                         BYTE Index,
                         WORD *PciWord);
BOOL WritePciConfigWord (BYTE Bus,
                         BYTE Function,
                         BYTE Device,
                         BYTE Index,
                         WORD PciWord);
BOOL ReadPciConfigByte  (BYTE Bus,
                         BYTE Function,
                         BYTE Device,
                         BYTE Index,
                         BYTE* PciByte);
BOOL WritePciConfigByte (BYTE Bus,
                         BYTE Function,
                         BYTE Device,
                         BYTE Index,
                         BYTE PciByte);

#endif  // _pcibios_h_

