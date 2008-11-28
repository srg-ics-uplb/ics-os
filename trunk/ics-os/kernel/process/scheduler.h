/*
Name: Dex32 Default Round-Robin Scheduler
Author: Joseph Emmanuel Dayo
Description: This module is the default round-robin scheduler that is
				 initially used by the operating system
				 
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
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../devmgr/dex32_devmgr.h"

extern PCB386 *sched_phead;
extern int ps_schedid;
extern devmgr_scheduler_extension ps_scheduler;
extern int sched_attach(devmgr_generic *cur);
extern int sched_dequeue(PCB386 *ptr);
extern void sched_enqueue(PCB386 *process);
extern PCB386 *sched_getcurrentprocess();
extern PCB386 *sched_getnextprocess(PCB386 *before);
extern void sched_scheduler_install();
extern PCB386 *scheduler(PCB386 *lastprocess);
extern void ps_scheduler_install();
#endif
