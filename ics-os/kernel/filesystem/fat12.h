/*
  Name: FAT filesystem driver
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This is the implementation of the FAT filesystem based on the
  "Microsoft Extensible Firmware Initiative FAT32 File System Specification"
  version 1.03 released by Microsoft Corporation on December 6,2000
  
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

#ifndef FAT_H
#define FAT_H

#include "../vfs/vfs_core.h"

typedef unsigned char	uint8_t;	/* or #include <stdint.h> */
typedef unsigned short	uint16_t;	/* Note: multi-byte values are little-endian */
typedef unsigned long	uint32_t;

#define FAT12_EOC 0x0FFF
#define FAT16_EOC 0xFFFF
#define FAT32_EOC 0x0FFFFFFF

#define FAT12_INVC 0xFF7
#define FAT12_END_CLUSTER 0x0FF8
#define FDIRECTORY 16
#pragma packed(1)


#define ATTR_READ_ONLY   	0x01
#define ATTR_HIDDEN 	0x02
#define ATTR_SYSTEM 	0x04
#define ATTR_VOLUME_ID 	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE  	0x20
#define ATTR_LONG_NAME 	(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK	(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)
#define LAST_LONG_ENTRY 0x40

#define FAT12_FSIZE 1
#define FAT12_FNAME 2

#define FAT12_FAT12 1
#define FAT12_FAT16 2
#define FAT12_FAT32 3


typedef struct __attribute__((packed)) _fat_long_entry
{
 BYTE order;     /*The order of this entry in the sequence of long dir entries associated
                   with the short dir entry at the end of the long dir set.
                   If masked with 0x40 (LAST_LONG_ENTRY), this indicates the entry
                   is the last long dir entry in a set of long dir entries.
                   All valid sets of long dir entries must begin with an entry having this mask.*/
 char name1[10]; /*Characters 1-5 of the long-name sub-component in this dir entry.*/
 BYTE attr;      /*Attributes - must be ATTR_LONG_NAME*/
 BYTE type;      /*If zero, indicates a directory entry that is a sub-component of a
                   long name.  NOTE: Other values reserved for future extensions.
                   Non-zero implies other dirent types.*/
 BYTE chksum;    /*Checksum of name in the short dir entry at the end of the long dir set.*/
 char name2[12]; /*Characters 6-11 of the long-name sub-component in this dir entry.*/
 WORD reserved;  /*Must be ZERO. This is an artifact of the FAT "first cluster" and
                   must be zero for compatibility with existing disk utilities.
                    It's meaningless in the context of a long dir entry.*/
  char name3[4]; /*Characters 12-13 of the long-name sub-component in this dir entry.*/

} dos_long_entry;


typedef struct __attribute__((packed)) _dos_time                 /* Warning: this struct must be packed */
{
	unsigned two_secs : 5;  /* low 5 bits: 2-second increments */
	unsigned minutes : 6;   /* middle 6 bits: minutes */
	unsigned hours : 5;     /* high 5 bits: hours (0-23) */
} dos_time;                              /* 2 bytes total */

typedef struct __attribute__((packed)) _dos_date                 /* Warning: this struct must be packed */
{
	unsigned date : 5;      /* low 5 bits: date (1-31) */
	unsigned month : 4;     /* middle 4 bits: month (1-12) */
	unsigned year : 7;      /* high 7 bits: year - 1980 */
} dos_date;                              /* 2 bytes total */


typedef struct __attribute__((packed)) fat_bootsector			/* Warning: this struct must be packed */
{
    BYTE  jump[3];               /* 16-bit JMP to boot code, or 8-bit JMP + NOP */
    BYTE  oem_id[8];             /* e.g. 'MSWIN4.0' */
  	WORD  bytes_per_sector;	/* usu. =512 */
    BYTE  sectors_per_cluster;
  	WORD  num_boot_sectors;	/* usu. =1 */
    BYTE  num_fats;              /* usu. =2 */
  	WORD num_root_dir_ents;
	WORD total_sectors;		/* 16-bit; 0 if num sectors > 65535 */
    BYTE  media_ID_byte;         /* usu. =0F0h */
	WORD sectors_per_fat;
	WORD sectors_per_track;
	WORD heads;
	DWORD hidden_sectors;	/* =LBA partition start */
	DWORD total_sectors_large;	/* 32-bit; 0 if num sectors < 65536 */
	
	/*********************FAT12/16 only portion******************************/
	BYTE drvnum;
	BYTE reserved1;
	BYTE bootsig;
	char vol_id[4];
	char vol_label[11];
	char filesystype[8];
    BYTE  boot_code[448];
    BYTE  magic[2];              /* 55h, 0AAh */
} BPB;                              /* 512 bytes total */

typedef struct __attribute__((packed)) fat_bootsector32			/* Warning: this struct must be packed */
{
    BYTE  jump[3];               /* 16-bit JMP to boot code, or 8-bit JMP + NOP */
    BYTE  oem_id[8];             /* e.g. 'MSWIN4.0' */
  	WORD  bytes_per_sector;	/* usu. =512 */
    BYTE  sectors_per_cluster;
  	WORD  num_boot_sectors;	/* usu. =1 */
    BYTE  num_fats;              /* usu. =2 */
  	WORD num_root_dir_ents;
	WORD total_sectors;		/* 16-bit; 0 if num sectors > 65535 */
    BYTE  media_ID_byte;         /* usu. =0F0h */
	WORD sectors_per_fat;
	WORD sectors_per_track;
	WORD heads;
	DWORD hidden_sectors;	/* =LBA partition start */
	DWORD total_sectors_large;	/* 32-bit; 0 if num sectors < 65536 */
	
	/*********************FAT12/16 only portion******************************/
	DWORD fatsz32;
	WORD  extflags;
	WORD  fsversion;
	DWORD rootcluster;
	WORD  fsinfo;
	WORD  bkbootsec;
	char  reserved[12];
	BYTE  drvnum;
    BYTE  boot_code[445];
    BYTE  magic[2];              /* 55h, 0AAh */
} BPB32;                              /* 512 bytes total */


#pragma packed()

typedef struct fat_bootsctor BOOTSECTOR;

#pragma packed(1)
typedef struct __attribute__((packed)) _fat_dirent               	/* Warning: this struct must be packed */
{
        uint8_t  name[8];              /* ALL-CAPS, pad right with spaces */
        uint8_t  ext[3];               /* ALL-CAPS, pad right with spaces */
        uint8_t  attrib;               /* attribute byte */
        uint8_t  reserved;             /* =0 */
        uint8_t  ctime_ms;             /* file creation time, 10ms units */
		uint16_t ctime;                /* file creation time, in DOS format */
		dos_date cdate;              	/* file creation date, in DOS format */
		uint16_t adate;              	/* DOS date of last file access */
		uint16_t st_clust_msw;       	/* high 16 bits of starting cluster (FAT32) */
		uint16_t mtime;              	/* DOS time of last file modification */
		dos_date mdate;              	/* DOS date of last file modification */
		uint16_t st_clust;           	/* starting cluster */
        uint32_t file_size;          	/* in bytes */
} fatdirentry;                         /* 32 bytes total */


typedef struct __attribute__((packed)) _attrib                   /* Warning: this struct must be packed */
{
	int read_only : 1;      /* b0 */
	int hidden : 1;
	int system : 1;
	int volume_label : 1;
	int directory : 1;
	int archive : 1;
	int reserved : 2;       /* b6, b7 */
} fat_attrib;                              /* 1 byte total */
#pragma packed()

extern int fat_deviceid;

/*---------------------------------Function Prototypes Here----------------------------------*/
int clustertoblock(BPB *bpbblock,int cluster);
void readBPB(BPB *bpbblock,int id);
void interpretBPB(BPB *bpbblock);
int loadroot(fatdirentry **dir,const BPB *bpbblock,int id);
void strtofile12(const char *str,char *s);
void file12tostr(fatdirentry *dir,char *str);
DWORD fat_writefileEX(vfs_node *f,char *bufr,int start,int end,int id);
DWORD fat_createfileEX(vfs_node *f,int id);
DWORD fat_addsectorsEX(vfs_node *f,DWORD sectors /*sectors to add*/,int id);
DWORD fat_openfileEX(vfs_node *f,char *bufr,int start,int end,int id);
int fat_getfreeblocks();
DWORD fat_getbytesperblock();
void fat_getfileblocks(file_PCB *f,DWORD *sectinfo,int id);
DWORD getdirsectorsize(fatdirentry *dir,BPB *bpbblock,int func,BYTE *fat,int id);
DWORD update_dirs(BPB *bpbblock,vfs_node *tdir,BYTE *fat,int id);
DWORD update_fats(BPB *bpbblock,BYTE *fat,int id);
DWORD update_dirs_fats(BPB *bpbblock,BYTE *fat,vfs_node *tdir,int id);
int fat_get_fat_type(int devid, BPB *bpb);
int fat_getsectorsizeEX(vfs_node *f,int id);
void fat_rewritefileEX(vfs_node *f,int id);
DWORD fat_modifyattb(vfs_node *f, DWORD attb,int id);
DWORD fat_addsectors(vfs_node *f,BPB *bpbblock,
                       DWORD sectors /*sectors to add*/,int id);
DWORD fat_deletefile(vfs_node *f,int id);
DWORD fat_rewritefile(vfs_node *f,BPB *bpbblock,int id);
DWORD fat_createfile(vfs_node *f,BPB *bpbblock,int id);
int fat_get_eoc(int fat_type);
DWORD openfile(const char *fname,char *bufr,int id);
void fat_remount(); //performs an auto remount
int fat_mountdirectory(vfs_node *directory, int id);
int fat_mount_root(vfs_node *mountpoint,int id);
char *unicodetoascii(WORD *unicodestr,char *targ,int length);
int fat_mount(vfs_node *mountpoint,fatdirentry *buf2,BPB *bpb,int id);
DWORD get_sector_fromcluster(DWORD cluster,BPB *bpbblock,int func,BYTE *fat,int id);
void writecluster(int cluster,int value,BYTE *fat,int);
int obtain_next_cluster(int cluster,void *fat,int fat_type,BPB *bpbblock,int id);
int obtaincluster(int cluster,BYTE *fat);
int obtainfreecluster(BYTE *fat,int maxentries);
int writefile12(fatdirentry *dir,BPB *bpbblock,char *fname,char *buf);
int fillsectorinfo(fatdirentry *dir,BPB *bpbblock,DWORD *sectinfo,int id);
int readfile12(fatdirentry *dir,BPB *bpbblock,char *buf,DWORD cluster);
int writefile12EX2(fatdirentry *dir,BPB *bpbblock,char *buf,int se,int start,int end,int id);
void loadfat(BPB *bpbblock,void *fat,int id);
int loadfile12EX2(fatdirentry *dir,BPB *bpbblock,char *buf,int se,int start,int end,int id);
int loadfile12(fatdirentry *dir,BPB *bpbblock,char *buf,int id);
int loaddirectory(fatdirentry *dir,char **buf,int id);
int fat_register();

#endif
