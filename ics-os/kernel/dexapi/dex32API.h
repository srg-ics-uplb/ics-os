/*******************************************************************
DEX32 API (Application Programmers Interface)
This is the code that manages system calls from user mode programs (Level 3)
currently applications make sys calls using interrupt 0x30h (User Interrupt Gate) 
although a user procedure call is in the works

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
********************************************************************/
#ifndef __dex32API_h_

#define __dex32API_h_

//handles dex32 system calls, supports a maximum of
//5 parameters per call with a single DWORD return value
//this function is called using interrupt 0x30 by user-mode
//applications
#define API_MAXSYSCALLS 0x100

#define API_REQUIRE_INTS 0x1
typedef struct _api_systemcall 
{
    DWORD access_check;
    int flags;
    void *function_ptr;
} api_systemcall;


api_systemcall api_syscalltable[API_MAXSYSCALLS];

int api_addsystemcall(DWORD function_number, void *function_ptr, 
                        DWORD access_check, DWORD flags);
void api_init();
int api_removesystemcall(DWORD function_number);
DWORD api_syscall(DWORD fxn,DWORD val,DWORD val2,
                   DWORD val3,DWORD val4,DWORD val5);

#endif
