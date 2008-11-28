/*
  Name: Exception handling module
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This module provides exception handlers for the operating system. There are
  exception handlers for GPFs, page faults and divide by zero. The page fault handler also
  handles demand loading requests.
  
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

#define PAGE_FAULT 1
#define GENERAL_PROTECTION_FAULT 2
#define DIVIDE_ERROR 3
#define INVALID_TSS 4

void divide_error(DWORD address);
void GPFhandler(DWORD address);
DWORD pagefaulthandler(DWORD location,DWORD fault_info);
void exc_showdump(DWORD location,int type,DWORD pf_info);
void exc_invalidtss(DWORD address);
void exc_recover();
