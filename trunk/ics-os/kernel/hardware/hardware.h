/*
  Name: miscellanous hardware management
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This module handles functions that get information about various
  hardware devices like the CPU.
  
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
#define CPUFEATURE_FPU   1
#define CPUFEATURE_VME   2
#define CPUFEATURE_DE    4
#define CPUFEATURE_PSE   8
#define CPUFEATURE_TSC   16
#define CPUFEATURE_MSR   32
#define CPUFEATURE_PAE   64
#define CPUFEATURE_MCE   128
#define CPUFEATURE_CXS   256
#define CPUFEATURE_APIC  512
#define CPUFEATURE_AMDFASTSYSCALL 1024
#define CPUFEATURE_INTELFASTSYSCALL 2048
#define CPUFEATURE_MTRR   4096
#define CPUFEATURE_PGE    8192
#define CPUFEATURE_MCA    16384
#define CPUFEATURE_CMOV   32768
#define CPUFEATURE_PAT    65536
#define CPUFEATURE_PSE36  131072
#define CPUFEATURE_PSN    262144
#define CPUFEATURE_CF     524288
#define CPUFEATURE_DTS    2097152
#define CPUFEATURE_ACPI   4194304 
#define CPUFEATURE_MMX    8388608
#define CPUFEATURE_FXSAVE 16777216
#define CPUFEATURE_SSE    33554432
#define CPUFEATURE_SSE2     67108864
#define CPUFEATURE_SNOOP    134217728
#define CPUFEATURE_ACC      536870912
#define CPUFEATURE_IA64     1073741824
#define CPUFEATURE_HTT      268435456
#define CPUFEATURE_PBE      2147483648

//AMD FEATURES
#define CPUFEATURE_3DNOW    2147483648 
#define CPUFEATURE_3DNOWEXT 1073741824
#define CPUFEATURE_AA64      536870912

typedef struct _hardwareinfo {
 DWORD a,b,c,d,p;
} hardwareinfo;

/*Based on the Intel Pentium Processor Software developer's manual*/
typedef struct _hardware_cpuinfo {
char  manufacturer[13]; //The manufacturer of the CPU
char  modelstring[49];  //The name of the CPU assigned by the manufacturer e.g. "Pentium(r)"
DWORD type, family, model, stepping;
DWORD feature;
} hardware_cpuinfo;

 hardware_cpuinfo hardware_mycpu; //set by the kernel on startup

extern void getcpuid(DWORD eax,DWORD *a, DWORD *b, DWORD *c, DWORD *d);

void hardware_printinfo(hardware_cpuinfo *cpuinfo);
void hardware_getcpuinfo(hardware_cpuinfo *cpuinfo);
void getcpubrand(char *s);
void getcpumodel(char *s);

