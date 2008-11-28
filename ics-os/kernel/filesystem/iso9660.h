/*
  Name: iso9660.h
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 07/02/04 07:36
  Description: The module that handles the ISO9660 filesystem on most CD-ROMS
  
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
#ifndef ISO9660_H
#define IS09660_H

#include "../dextypes.h"

#define ISO9660_ATTBDIRECTORY 2
#define ISO9660_ATTBLASTFILE 0x80
#define ISO9660_SECTORSIZE 2048

typedef struct __attribute__((packed)) _iso9660_rootdirectory {
    BYTE size;
    BYTE sectors_in_extended;
    DWORD first_sector_le; //little endian
    DWORD first_sector_be; //big endian
    DWORD length_low;
    DWORD length_high;
    BYTE years,month,day,hour,minute,second,GMT;
    BYTE flags;
    BYTE interleave_size;
    BYTE interleave_gap;
    DWORD vol_seq_num;
   } iso9660_rootdirectory;

typedef struct __attribute__((packed)) _iso9660_volumedescriptor {
        BYTE ident;
        BYTE descriptors[6];
        BYTE reserved1;
        BYTE system_ident[32];
        BYTE volume_ident[32];
        BYTE reserved2[8];
        DWORD total_sectors_low;
        DWORD total_secotrs_high;
        BYTE escape_sequence[32];
        DWORD vol_set_size;
        DWORD vol_seq_number;
        WORD sector_size_le;
        WORD sector_size_be;
        DWORD path_table_length_low;
        DWORD path_table_length_high;
        DWORD first_sector1;
        DWORD first_sector2;
        DWORD Bfirst_sector1;
        DWORD Bfirst_sector2;
        iso9660_rootdirectory rootdirrec;
        BYTE vol_set_id[128];
        BYTE publisher_id[128];
        BYTE data_preparer_id[128];
        BYTE application_id[128];
        BYTE copyright_file_id[37];
        BYTE abstract_file_id[37];
        BYTE bibliographical_file_id[37];
        BYTE time_vol_creation[17];
        BYTE time_mod[17];
        BYTE time_exp[17];
        BYTE time_effective[17];
        BYTE reserved4[2];
        BYTE reserved5[512];
        BYTE reserved6[653];
   }  iso9660_volumedescriptor;

typedef struct __attribute__((packed)) _iso9660_path_table {
        BYTE name_length;   
        BYTE num_sectors;
        DWORD first_dir_sector;        
        WORD record_number;
        BYTE NAME[1];
        } iso9660_path_table;
 
               
typedef struct __attribute__((packed)) _iso9660_directory {
    BYTE size;
    BYTE sectors_in_extended;
    DWORD first_sector_le; //little endian
    DWORD first_sector_be; //big endian
    DWORD length_le; //little endian
    DWORD length_be; //big endian
    BYTE years,month,day,hour,minute,second,GMT;
    BYTE flags;
    BYTE interleave_size;
    BYTE interleave_gap;
    WORD vol_seq_num_le;
    WORD vol_seq_num_be;
    BYTE ident_length;
    char ident[30];
   } iso9660_directory;

   /*******************Prototype definition here*******************************/   
   int iso9660_loaddirectory(iso9660_directory *dirinfo, void **buffer,int id);
   int iso9660_mountroot(vfs_node *node,int id);
   char *iso9660_convertname(const char *identifier, char *targ);
   char *iso9660_iso_unicodetoascii(WORD *unicodestr,char *targ,int length);
   int iso9660_getbytesperblock();
   int iso9660_openfile(vfs_node *f,char *buffer,int start,int end,int id);
   int iso9660_unmount(vfs_node *directory,int id);
   int iso9660_mountdirectory(vfs_node *directory, int id);
   int iso9660_isjoliet(iso9660_volumedescriptor *vol);
   void iso9660_mount(vfs_node *mountpoint,char *dirbuffer, int devid);
   void iso9660_init();
   
#endif
