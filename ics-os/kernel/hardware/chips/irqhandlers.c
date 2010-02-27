/*
  Name: IRQ and interrupt management module
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This module handles registration of interrupt handlers and also manages
  the Programmable interrupt controller.
  
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

#define IRQ_TIMER 1
#define IRQ_CASCADE 4   /* FOR SLAVE PIC */
#define IRQ_KEYBOARD 2
#define IRQ_FDC 64
#define IRQ_MOUSE 16  //added by jach

typedef struct _idtr {
   WORD limit;
   idtentry *location;
}idtr;

/*Used by the irqwrappers to determine the device drivers
  that uses this irq's so that they could be notified whenever
  an IRQ is triggered.*/
  
typedef struct _irq_attachments {
   int deviceid;
   void (*irq_handler)(); //the device's irq handler
   struct _irq_attachments *next,*prev;
} irq_attachments;

irq_attachments *irq_attachlist[16];

//ticks gets incremented whenever the timer interrupt gets called.
unsigned int ticks=0;

/*These wrappers link to irqwrap.asm
 These must be in assembly since interrupt handlers are
 required to terminate with an IRET instruction*/

extern void timerwrapper(void);
extern void loadregisters(void);
extern void kbdwrapper(void);
extern void mousewrapper(void);
extern void pfwrapper(void);
extern void switchprocess(void);
extern void syscallwrapper(void);
extern void gpfwrapper(void);
extern void copwrapper(void);
extern void CPUintwrapper(void);
extern void div_wrapper(void);
extern void invalidtsswrapper(void);

extern void irq1wrapper(void);
extern void irq2wrapper(void);
extern void irq3wrapper(void);
extern void irq4wrapper(void);
extern void irq5wrapper(void);
extern void irq6wrapper(void);
extern void irq7wrapper(void);
extern void irq8wrapper(void);
extern void irq9wrapper(void);
extern void irq10wrapper(void);
extern void irq11wrapper(void);
extern void irq12wrapper(void);

//functions for setting and getting the CR0 register*/
extern DWORD getCR0();
extern DWORD setCR0(DWORD);

idtr intloc;

/*program8259 is the main function used for programming
  the 8259 controller. The 8259 controller is responsible
  for controlling the IRQ assignments of various devices in the computer

  This function remaps the IRQs from 0-7 to 20-27
  and 8-15 to 28-2F respectively since Interrupts 0-15 are reserved by
  Intel corp.

  The paremeter b holds the IRQ bits of the devices that you want to enable
  See Kernel32.c where this function is used*/
void program8259(unsigned char b)
 {
   unsigned char b1=0xFF;
   unsigned char b2=0xFF;

   //remap the IRQs
   outportb(0x20,0x11);
   outportb(0xA0,0x11);
   outportb(0x21,0x20); //IRQ0-IRQ7 -> interrupts 0x20-0x27
   outportb(0xA1,0x28); //IRQ8-IRQ15 -> interrupts 0x28-0x2F
   outportb(0x21,4);
   outportb(0xA1,2);
   outportb(0x21,1);
   outportb(0xA1,1);
   b1^=b;
   outportb(0x21,b1);
   //b2^=IRQ_MOUSE;
   //outportb(0xA1,b2);
   outportb(0xA1, inportb(0xA1) & ~0x10);
 };


void dex32_irqcntl(unsigned char b)
 {
   unsigned char b1=0xFF;
   b1^=b;
   outportb(0x21,b1);	
   outportb(0xA1,0xFF);	
 };

void CPUint()
 {
  stopints();
  printf("unhandled Interrupt 0-0x19..\n");
  while (1) {};
  startints();
 };

void unhandled(int irq_num)
 {
   stopints();
   textcolor(RED);
   println("This interrupt is unhandled....",attb);
 //  while (1) {}; //temporarily disabled infinite loop for unhandled interrupts
   startints();
 };



void fdctimer(void); //found in floppy.c
void fdcwrapper(void); //found in floppy.c

void seg_error()
 {
   stopints();
   printf("Segmentation error\n");
   printf("system halted.\n");
   while(1) {};
 };

void stack_error()
 {
   stopints();
   printf("Stack segment error\n");
   printf("system halted.\n");
   while(1) {};
 };


void invalidtss()
  {
    stopints();
    printf("Invalid Task State Segment.\n");
    printf("system halted.\n");
    while (1) {};
  };

void double_fault()
  {
  stopints();
    printf("Double fault.\n");
    printf("system halted.\n");
    while (1) {};
  };

void boundscheck()
  {
    stopints();
    printf("bounds check error.\n");
    printf("system halted.\n");
    while (1) {};
  };


void nocoprocessor()
  {
    char temp[255];
    setCR0(0x80000011);
  };

void breakpoint()
  {
    stopints();
    printf("breakpoint reached.\n");
    printf("system halted.\n");
    while (1) {};
  };
void overflow()
  {
    stopints();
    printf("over flow error.\n");
    printf("system halted.\n");
    while (1) {};
  };

void coprocessor_segment_overrun()
  {
   stopints();
   printf("Coprocessor segment overrun.\n");
   printf("System Halted.\n");
   while (1);   
  };
  
void coprocessor()
  {
    stopints();
    printf("Floating Point error.\n");
    printf("system halted.\n");
    while (1) {};
  };

void invalid_opcode()
  {
    stopints();
    printf("invalid opcode error.\n");
    printf("system halted.\n");
    while (1) {};

  };

void debug_error()
  {
    stopints();
    printf("debug called.\n");
    printf("system halted.\n");
    while (1) {};

  };

void ungetcx(char c)
 {
  char temp[2];
  temp[0]=c;
  temp[1]=0;
  sendtokeyb(temp,&_q);
 };


int ts_enabled=1;

void disable_taskswitching()
{  
   DWORD flags;
   if (ts_enabled)
   {
      storeflags(&flags);
      stopints();
      setinterruptvector(0x20,dex_idtbase,0x8E,
      timerwrapper,SYS_CODE_SEL);
      ts_enabled=0;
      refreshpages();
      restoreflags(flags);
   };   
};

void enable_taskswitching()
{
    DWORD flags;
    if (!ts_enabled)
    {
        storeflags(&flags);
        stopints();
        setinterruptvector(0x20,dex_idtbase,0x85,0,SCHED_TSS);
        ts_enabled=1;
        refreshpages();
        restoreflags(flags);
    };
};

/*
====================================================================================
irq_init:

Initializes the IRQ handler table.
====================================================================================
*/
void irq_init()
{
    int i;
    for (i=0; i<16 ; i++)
        irq_attachlist[i] = 0;
};


/*
====================================================================================
irq_activate:

This function gets called only when an IRQ fires and should not be called
through any other way by any other module.
====================================================================================
*/

void irq_activate(int irqnum)
{
    irq_attachments *ptr = irq_attachlist[irqnum];
    //irq was activated, now we loop through all the handlers for this IRQ
    while(ptr!=0)
    {
        //call the handler for this device
        ptr->irq_handler();
        ptr = ptr->next;
    };

    //EOI for slave 
    if (irqnum > 0x27){
      outportb(0xA0, 0x20);
    }
 
    //reactive the programmer interrupt controller since it gets
    //temporarily disabled once an IRQ fires
    outportb(0x20,0x20);
};


/*
====================================================================================
irq_addhandler:

This function adds an event handler to a particular irq, usually used by
device drivers to hook onto irq's
parameters:
    deviceid   = the deviceid of the caller
    irq_number = the IRQ number
    handler    = a pointer to the event handler of the IRQ which is supplied by the
                 device driver.   
returns: 
    -1 if unsuccessful
     0 if handler was the first 
     1 if handler was not the first to be hooked onto the irq
====================================================================================
*/
int irq_addhandler(int deviceid,int irq_number,void (*handler)())
{
   irq_attachments *irqhand = (irq_attachments*)malloc(sizeof(irq_attachments));
   irqhand->deviceid    = deviceid;
   irqhand->irq_handler = handler;

   //attach to irq handlers list using the attach to head method
   //check if nothing is attached yet
   if (irq_attachlist[irq_number]==0) 
   {
      irq_attachlist[irq_number] = irqhand;
      irqhand->next = 0;
      irqhand->prev = 0;                 
      return 0;
   };
        
   irqhand->next = irq_attachlist[irq_number];
   irq_attachlist[irq_number]->prev = irqhand;
   irq_attachlist[irq_number]= irqhand;
   irqhand->prev = 0;    
   return 1;
};


/* 
====================================================================================
This procedure "setdefaulthandlers()" sets up the IDT (Interrupt Descriptor Table)
--The Interrupt Descriptor Table (IDT) is like the IVT in x86 real mode only that an 
entry in an IDT contains more elements like security bits, and uses
selectors unlike segments in real mode. For details on the workings of the 
IDT, please consult an Intel 386+ programmer's Manual.
====================================================================================  
*/
void setdefaulthandlers()
  {
  int i;
  for (i=0;i<0x20;i++)
       setinterruptvector(i,dex_idtbase,0x8E,CPUintwrapper,SYS_CODE_SEL);
  for (i=0x20;i<255;i++)
       setinterruptvector(i,dex_idtbase,0x8E,unhandled,SYS_CODE_SEL);


  irq_init();
       
   /************************* Install the IRQ handlers ***************************/    
   //install timer handler
   setinterruptvector(0x20,dex_idtbase,0x85,0,SCHED_TSS);
   
   setinterruptvector(0x21,dex_idtbase,0x8E,
   irq1wrapper,SYS_CODE_SEL);
   
   setinterruptvector(0x22,dex_idtbase,0x8E,
   irq2wrapper,SYS_CODE_SEL);
   
   setinterruptvector(0x23,dex_idtbase,0x8E,
   irq3wrapper,SYS_CODE_SEL);

   setinterruptvector(0x24,dex_idtbase,0x8E,
   irq4wrapper,SYS_CODE_SEL);

   setinterruptvector(0x25,dex_idtbase,0x8E,
   irq5wrapper,SYS_CODE_SEL);
   
   setinterruptvector(0x26,dex_idtbase,0x8E,
   irq6wrapper,SYS_CODE_SEL);

   setinterruptvector(0x27,dex_idtbase,0x8E,
   irq7wrapper,SYS_CODE_SEL);

   setinterruptvector(0x28,dex_idtbase,0x8E,
   irq8wrapper,SYS_CODE_SEL);
   
   setinterruptvector(0x29,dex_idtbase,0x8E,
   irq9wrapper,SYS_CODE_SEL);

   setinterruptvector(0x2A,dex_idtbase,0x8E,
   irq10wrapper,SYS_CODE_SEL);
   
   setinterruptvector(0x2B,dex_idtbase,0x8E,
   irq11wrapper,SYS_CODE_SEL);

 


   /************************** Install the CPU exception handlers *****************/
   //install the GPF handler
   setinterruptvector(0x0D,dex_idtbase,0x8E,
   gpfwrapper,SYS_CODE_SEL);

   //install the page fault handler
   setinterruptvector(14,dex_idtbase,0x85,
   0,PF_TSS);

   
     //install some error handlers
   setinterruptvector(0x0,dex_idtbase,0x8E,
   div_wrapper,SYS_CODE_SEL);

   setinterruptvector(11,dex_idtbase,0x8E,
   seg_error,SYS_CODE_SEL);

   setinterruptvector(12,dex_idtbase,0x8E,
   stack_error,SYS_CODE_SEL);

   setinterruptvector(10,dex_idtbase,0x8E,
   invalidtsswrapper,SYS_CODE_SEL);

   setinterruptvector(4,dex_idtbase,0x8E,
   overflow,SYS_CODE_SEL);

   setinterruptvector(9,dex_idtbase,0x8E,
   coprocessor_segment_overrun,SYS_CODE_SEL);

   setinterruptvector(16,dex_idtbase,0x8E,
   copwrapper,SYS_CODE_SEL);

   setinterruptvector(7,dex_idtbase,0x8E,
   copwrapper,SYS_CODE_SEL);

   setinterruptvector(5,dex_idtbase,0x8E,
   boundscheck,SYS_CODE_SEL);

   setinterruptvector(8,dex_idtbase,0x8E,
   double_fault,SYS_CODE_SEL);

   setinterruptvector(6,dex_idtbase,0x8E,
   fdcwrapper,SYS_CODE_SEL);

   setinterruptvector(3,dex_idtbase,0x8E,
   breakpoint,SYS_CODE_SEL);

   setinterruptvector(1,dex_idtbase,0x8E,
   debug_error,SYS_CODE_SEL);

   setinterruptvector(0x30,dex_idtbase,0xEE,
   syscallwrapper,SYS_CODE_SEL);
   
   setinterruptvector(0x31,dex_idtbase,0x8E,
   syscallwrapper,SYS_CODE_SEL);

   intloc.limit=2047;
   intloc.location=dex_idtbase;
   loadregisters();     //load the idtr register with data
};

