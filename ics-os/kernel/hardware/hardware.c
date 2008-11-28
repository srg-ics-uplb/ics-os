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
void hardware_printinfo(hardware_cpuinfo *cpuinfo)
{
    DWORD f;
    int isAMDcpu = 0;
    printf("Manufacturer    : %s\n",cpuinfo->manufacturer);
    printf("Product String  : %s\n",cpuinfo->modelstring);
    printf("CPU model       : Model %d, Family %d, Type %d, Stepping %d.\n",
                                    cpuinfo->model,
                                    cpuinfo->family,
                                    cpuinfo->type,
                                    cpuinfo->stepping);  
    printf("Features    :\n");
    
    if (strcmp(cpuinfo->manufacturer,"AuthenticAMD")==0) isAMDcpu = 1;
    
    f = cpuinfo->feature;
    if ( f & CPUFEATURE_FPU) printf("* FPU\n");
    if ( f & CPUFEATURE_VME) printf("* Virtual Mode Extensions\n");
    if ( f & CPUFEATURE_DE)  printf("* Debugging Extensions\n");
    if ( f & CPUFEATURE_PSE) printf("* Page Size Extensions\n");
    if ( f & CPUFEATURE_TSC) printf("* Time stamp counter\n");
    if ( f & CPUFEATURE_MSR) printf("* Model-specific registers\n");
    if ( f & CPUFEATURE_PAE) printf("* Physical Address Extensions\n");
    if ( f & CPUFEATURE_MCE) printf("* Machine Check Exceptions\n");
    if ( f & CPUFEATURE_CXS)  printf("* Compare and exchange 8-byte\n");
    if ( f & CPUFEATURE_APIC) printf("* On-chip APIC\n");
    if ( f & CPUFEATURE_AMDFASTSYSCALL && isAMDcpu) printf("* aFSC\n");
    if ( f & CPUFEATURE_INTELFASTSYSCALL) printf("* iFSC\n");
    if ( f & CPUFEATURE_MTRR) printf("* MTRR\n");
    if ( f & CPUFEATURE_PGE) printf("* PGE\n");
    if ( f & CPUFEATURE_MCA) printf("* MCA\n");
    if ( f & CPUFEATURE_CMOV) printf("* CMOV\n");
    if ( f & CPUFEATURE_PAT) printf("* Page Attribute Table\n");
    if ( f & CPUFEATURE_PSE36) printf("* 36-bit Page Size Extensions\n");
    if ( f & CPUFEATURE_PSN) printf("* Processor Serial Nnumber\n");
    if ( f & CPUFEATURE_CF) printf("* CLFLUSH\n");
    if ( f & CPUFEATURE_DTS) printf("* Debug Trace and EMON Store MSRs\n");
    if ( f & CPUFEATURE_ACPI) printf("* Thermal Control MSR\n");
    if ( f & CPUFEATURE_MMX) printf("* MMX\n");
    if ( f & CPUFEATURE_FXSAVE) printf("* Fast floating point save/restore\n");
    if ( f & CPUFEATURE_SSE && !isAMDcpu) printf("* Intel SSE\n");
    if ( f & CPUFEATURE_SSE && isAMDcpu) printf("* AMD 3Dnow Professional (SSE)\n");
    if ( f & CPUFEATURE_SSE2) printf("* Intel SSE2\n");
    if ( f & CPUFEATURE_SNOOP) printf("* Slef-Snoop\n");
    if ( f & CPUFEATURE_ACC && !isAMDcpu) printf("* Thermal Interrupt & Status MSRs.\n");
    if ( f & CPUFEATURE_IA64 && !isAMDcpu) printf("* Intel IA64\n");
    if ( f & CPUFEATURE_3DNOW && isAMDcpu) printf("* AMD 3Dnow!\n");
    if ( f & CPUFEATURE_3DNOWEXT && isAMDcpu) printf("* AMD Extended 3Dnow!\n");
    if ( f & CPUFEATURE_HTT) printf("* HyperThreading Technology\n");
    printf("\n");

};

void hardware_getcpuinfo(hardware_cpuinfo *cpuinfo)
{
    DWORD max_ins, type;
    hardwareinfo hw;
    
    memset(cpuinfo,0,sizeof(hardware_cpuinfo));
    getcpubrand(cpuinfo->manufacturer);    
    //get feature information
    getcpuid(1,&hw.a,&hw.b,&hw.c,&hw.d);
    cpuinfo->feature = hw.d;
    getcpumodel(cpuinfo->modelstring);

    //get model, family, type and stepping information
    type = hw.a;
    cpuinfo->stepping = type & 0xF;
    cpuinfo->model =    (type & 0xF0)  >> 4;
    cpuinfo->family =   (type & 0xF00) >> 8;
    cpuinfo->type =     (type & 0x3000) >> 12;  
};

void getcpubrand(char *s)
{
   hardwareinfo hw; //used for getcpuid() information
   getcpuid(0,&hw.a,&hw.b,&hw.d,&hw.c); //get manufacturer string
   hw.p=0;
   strcpy(s,(char*)&hw.b);
;};

void getcpumodel(char *s)
{
   hardwareinfo hw; //used for getcpuid() information
   getcpuid(0x80000002,&hw.a,&hw.b,&hw.c,&hw.d); //get manufacturer string
   hw.p=0;
   strcpy(s,(char*)&hw);
   getcpuid(0x80000003,&hw.a,&hw.b,&hw.c,&hw.d); //get manufacturer string
   hw.p=0;
   strcat(s,(char*)&hw);
   getcpuid(0x80000004,&hw.a,&hw.b,&hw.c,&hw.d); //get manufacturer string
   hw.p=0;
   strcat(s,(char*)&hw);


;};
