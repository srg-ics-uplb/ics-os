/*
  Name: DEX process dispatcher
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 23/01/04 04:53
  Description: This module is responsible for mapping processes from files, calling
  the appropriate moduleloader and starting them.
  
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

//Constants for the type of module
#define NEW_MODULE  0         //The module will NOT be an exact copy of the parent process when dispatched
#define FORK_MODULE 1         //The module will be an exact copy of the parent process when dispatched

/**
 * A node in the process dispatcher queue which describes a module.
 * The node contains information that process_dispatcher() needs to 
 * create a new process using either forkprocess() or createprocess() 
 * depending on the type
 * of the module. 
 */
typedef struct _createp_queue{
   int handle;                         //A reference to the handle
   int type;                           //Is it a NEW_MODULE or a FORK_MODULE
   char *image;                        //The program image read from the executable
   char *loadaddress;                  //The desired location in memory where the program data will be placed
   char name[255];                     //The name of the module
   char parameter[500];                //Command line arguments
   char workdir[500];                  //The working directory. Will be used as reference in directory access.
   int  mode;                          // TODO: 
   int parent;                         //Process ID of parent
   int dispatched;                     //Has this module been dispatched?
   DWORD processid;                    //The process id for this module
   struct _createp_queue *next;        //Pointer to the next entry
}createp_queue;


createp_queue *pd_head=0;              //The head of the process dispatcher queue which is global
DWORD pd_handlecounter=0;

int pd_busy = 0;                       //lock to access pd_head 
int pd_ready =0;                       

int addmodule(char *name,char *image,char *loadaddress,int mode,char *parameter,char *workdir,int parent);
int pd_forkmodule(int parent);
int pd_ok(int handle);
