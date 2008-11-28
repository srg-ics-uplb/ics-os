;**********************************************************************************
;  Name: startup.asm - kernel entrypoint module
;  Copyright: 
;  Author: Joseph Emmanuel DL Dayo
;  Date: 11/03/04 04:31
;  Description: This module receives control from GRUB after being loaded into 
;		memory. It sets up the GDT and IDT table which are both needed
;		in order to work in a protected mode environment. Other initialization
;		functions include enableing the A20 line and initializing the segment
;		registers or selectors.


;DEX Operating system startup file
;MULTI-BOOT compliant. Loadable by the GRUB boot loader

MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_FLAGS equ 0x10002

global _startup
extern edata
extern end
global _reset_gdtr


section .text

[BITS 32]
_startup:   
jmp not_multiboot
;Multiboot Header information
align 4,db 0

mb_header:
mb_magic         dd MULTIBOOT_MAGIC                       ;magic
mb_flags         dd MULTIBOOT_FLAGS                       ;bit 16 is set	
mb_checksum      dd - (MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)  ;compute checksum
mb_header_addr   dd mb_header
;mb_load_addr     dd _startup
;mb_load_end_addr dd _end
;mb_bss_end_addr  dd 0x200000
;mb_entry_addr    dd multiboot
mb_header_end:

multiboot:
;Multi boot compliant code 
     mov [multiboothdr], ebx   ;record the location of the multiboot information
                                ;structure
     mov byte [0xb8000],'.'

     ;the null descriptor	
     mov word  [gdt],0		
     mov word  [gdt+2],0	
     mov dword [gdt+4],0	

     ;the linear selector
     mov word [gdt1],0xFFFF		; limit 0xFFFFF
     mov word [gdt1+2],0		; base 0
     mov byte [gdt1+4],0
     mov byte [gdt1+5],0x92		; present, ring 0, data, expand-up, writable
     mov byte [gdt1+6],0xCF                 ; page-granular, 32-bit
     mov byte [gdt1+7],0


     xor ebx,ebx
     mov ebx,0x0000000
     mov eax,ebx

;the System Code selector
     mov word [gdt2],0xFFFF		; limit 0xFFFFF
     mov word [gdt2+2],ax		; base 0
     shr eax,16
     mov byte [gdt2+4],al
     mov byte [gdt2+5],0x9A		; present, ring 0, data, expand-up, writable
     mov byte [gdt2+6],0xCF                 ; page-granular, 32-bit
     mov byte [gdt2+7],ah

;the stack selector                       
     mov eax,0x80000 
     mov word [gdt3+2],ax
     shr eax,16
     mov byte [gdt3+4],al       
     mov word [gdt3],0xFFFF		; limit 0xFFFFF
     mov byte [gdt3+5],0x92		; present, ring 0, data, expand-up, writable
     mov byte [gdt3+6],0xCF               ; page-granular, 32-bit
     mov byte [gdt3+7],ah


;set up the data segment selector which is also linear in nature       
      
     mov eax, 0x0000
     mov word [gdt4+2],ax
     shr eax,16
     mov byte [gdt4+4],al       
     mov word [gdt4],0xFFFF		; limit 0xFFFFF
     mov byte [gdt4+5],0x92		; present, ring 0, data, expand-up, writable
     mov byte [gdt4+6],0xCF               ; page-granular, 32-bit
     mov byte [gdt4+7],ah

     mov byte [0xb8002],'.'
     cli                             ;interrupts disabled
     lgdt [gdtr]                     ;gdtr should have a valid value

jmp  SYS_CODE_SEL2:not_multiboot

not_multiboot:

     mov byte [0xb8004],'.' 
     mov ax,LINEAR_SEL
     mov es,ax
     mov ss,ax
     mov ds,ax
     mov gs,ax
     mov fs,ax
     ;the linear code selector
     mov eax,0x00000000
     mov word [es:gdt7],0xFFFF		; 1MB kernel memory length
     mov word [es:gdt7+2],ax
     shr eax,16
     mov byte [es:gdt7+4],al       
      
        mov byte [es:gdt7+5],0x9A		; present, ring 0, data, expand-up, writable
        mov byte [es:gdt7+6],0xCF               ; page-granular, 32-bit
        mov byte [es:gdt7+7],ah     

;refresh the Code Segment selector since we are now going to go into
;linear mode

        
        ;the linear stack selector                       
        mov eax,0x00000 
        mov word [es:gdt3+2],ax
        shr eax,16
        mov byte [es:gdt3+4],al       
        mov word [es:gdt3],0xFFFF		; limit 0xFFFFF
        mov byte [es:gdt3+5],0x92		; present, ring 0, data, expand-up, writable
        mov byte [es:gdt3+6],0xCF               ; page-granular, 32-bit
        mov byte [es:gdt3+7],ah


;set up the data segment selector which is also linear in nature       
      
        mov eax, 0x0000
        mov word [es:gdt4+2],ax
        shr eax,16
        mov byte [es:gdt4+4],al       
        mov word [es:gdt4],0xFFFF		; limit 0xFFFFF
        mov byte [es:gdt4+5],0x92		; present, ring 0, data, expand-up, writable
        mov byte [es:gdt4+6],0xCF               ; page-granular, 32-bit
        mov byte [es:gdt4+7],ah

;Set up a video memory selector
        mov eax,0x0B8000
        mov word [es:gdt5+2],ax
        shr eax,16
        mov byte [es:gdt5+4],al       
        mov word [es:gdt5],0xFFFF		; limit 0xFFFFF
        mov byte [es:gdt5+5],0x92		; present, ring 0, data, expand-up, writable
        mov byte [es:gdt5+6],0xCF               ; page-granular, 32-bit
        mov byte [es:gdt5+7],ah
        
        mov eax,0x01000000
        mov word [es:gdt6],0x0244		; 1MB kernel memory length
        mov word [es:gdt6+2],ax
        shr eax,16
      
        mov byte [es:gdt6+4],al       
        mov byte [es:gdt6+5],0x92		; present, ring 0, data, expand-up, writable
        mov byte [es:gdt6+6],0xCF               ; page-granular, 32-bit
        mov byte [es:gdt6+7],ah     

        mov byte [0xb8006],'.'
;Set the PE bit to get into protected mode
        mov eax,cr0
	or  al,1
	mov cr0,eax

        
jmp SYS_CODE_SEL:linearcode
linearcode:

        
        xor eax,eax
        mov ax,SYS_STACK_SEL
        mov ss,ax
        mov esp,0x9FFFE
        xor eax,eax
        mov ax,SYS_DATA_SEL 
        mov ds,ax

    
call enable_A20
;main code
			mov edi, 0xB8000
         mov esi, string
         mov ah, 0x0F
        .charLoop:
         lodsb
         stosw
         or al, al
         jnz .charLoop
         jmp short $


;this procedure handles the activation of the A20 line, it uses the standard method
;of enabling the A20 line
enable_A20:
	cli

	call    a20wait
	mov     al,0xAD
	out     0x64,al

	call    a20wait
	mov     al,0xD0
	out     0x64,al

	call    a20wait2
	in      al,0x60
	push    eax

	call    a20wait
	mov     al,0xD1
	out     0x64,al

	call    a20wait
	pop     eax
	or      al,2
	out     0x60,al

        call    a20wait
	mov     al,0xAE
	out     0x64,al

	call    a20wait
	ret

a20wait:
.l0:	mov     ecx,65536
.l1:	in      al,0x64
	test    al,2
	jz      .l2
	loop    .l1
	jmp     .l0
.l2:	ret


a20wait2:
.l0:	mov     ecx,65536
.l1:	in      al,0x64
	test    al,1
	jnz     .l2
        loop    .l1
	jmp     .l0
.l2:	ret

_reset_gdtr:
lgdt [gdtr]                     ;gdtr should have a valid value
ret

SECTION .data

string		db		'os is cool!',0

global multiboothdr        
multiboothdr dd 0; If this was loaded by a multiboot compliant system,
                             ; this should point to the multiboot structure.

gdtr:	dw 2047                	; GDT limit of 256 descriptors
	dd 0x01000              ; (GDT located at 0x01000)

NULL_SEL        equ         0b 
LINEAR_SEL	equ	 1000b
SYS_CODE_SEL2   equ     10000b
SYS_STACK_SEL   equ     11000b
SYS_DATA_SEL    equ    100000b
TEXT_VIDEO_SEL  equ    101000b 
KERNEL_MEM_SEL  equ    110000b
SYS_CODE_SEL    equ    111000b

APM32_CODE_SEL  equ   1000000b
APM16_CODE_SEL  equ   1001000b
APMDATA_SEL     equ   1010000b
TS_STACK_SEL    equ   1011000b
 

gdt             equ     0x01000
gdt1            equ     0x01000+8
gdt2            equ     0x01000+16
gdt3            equ     0x01000+24 
gdt4            equ     0x01000+32
gdt5            equ     0x01000+40 
gdt6            equ     0x01000+48
gdt7            equ     0x01000+56

gdtAPM32        equ     0x01000+64
gdtAPM16        equ     0x01000+72
gdtDATA         equ     0x01000+80
gdt8            equ     0x01000+88
