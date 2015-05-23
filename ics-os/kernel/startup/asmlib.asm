;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;stores the current cursor position

[bits 32]

SECTION .data
global CsrX
global CsrY


CsrX:
;CsrX:	db 0

CsrY:
;CsrY:	db 0

global attb
attb:
;attb:   db 0

idtr:   dw 2047     ;idt limit
        dd 0x2000   


testvalue dd 0xFAFAFAFA

;extern memset
extern curp
extern kernelp
extern pagedir1
extern servicedirloc
extern tlb_address
extern pcibiosentry
extern pcibios


SECTION .text

;int pci_finddevice(WORD deviceid,WORD vendorid, WORD index,char ;*busnumber, WORD *devnumber)

global pci_finddevice
pci_finddevice:
	push ebp
	mov ebp,esp

	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xB1
	mov al, 2
	mov ecx,[ebp+8]
	mov edx,[ebp+12]
	mov esi,[ebp+16]
	
	call far [pcibios]

	mov ecx, [ebp+20]	
	mov [ecx],bh
	mov ecx, [ebp+24]
	mov [ecx],bl

	shr eax, 8
	and eax, 0xf

	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;int pci_findclass(WORD classcode, WORD index,char 
;*busnumber, WORD *devnumber)

global pci_findclass
pci_findclass:
	push ebp
	mov ebp,esp

	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xB1
	mov al, 3
	mov ecx,[ebp+8]
	mov esi,[ebp+12]
	
	call far [pcibios]

	mov ecx, [ebp+16]	
	mov [ecx],bh
	mov ecx, [ebp+20]
	mov [ecx],bl

	shr eax, 8
	and eax, 0xf

	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;char _pci_readconfigbyte(char bus, char devfunc, WORD register)
global pci_readconfigbyte
pci_readconfigbyte:
	push ebp
	mov ebp,esp
	
	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xb1
	mov al, 8
	mov bh, [ebp+8]
	mov bl, [ebp+12]
	mov edi, [ebp+16]

	call far [pcibios]

	mov eax,ecx
	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;word _pci_readconfigword(char bus, char devfunc, WORD register)
global pci_readconfigword
pci_readconfigword:
	push ebp
	mov ebp,esp
	
	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xb1
	mov al, 9
	mov bh, [ebp+8]
	mov bl, [ebp+12]
	mov edi, [ebp+16]

	call far [pcibios]

	mov eax,ecx
	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;word _pci_readconfigdword(char bus, char devfunc, WORD register)
global pci_readconfigdword
pci_readconfigdword:
	push ebp
	mov ebp,esp
	
	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xb1
	mov al, 10
	mov bh, [ebp+8]
	mov bl, [ebp+12]
	mov edi, [ebp+16]

	call far [pcibios]

	mov eax,ecx
	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;int _pci_writeconfigbyte(char bus, char devfunc, WORD register, char data)
global pci_writeconfigbyte
pci_writeconfigbyte:
	push ebp
	mov ebp,esp
	
	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xb1
	mov al, 10
	mov bh, [ebp+8]
	mov bl, [ebp+12]
	mov edi, [ebp+16]
	mov ecx, [ebp+20]
	
	call far [pcibios]

	shr eax,8
	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;int _pci_writeconfigword(char bus, char devfunc, WORD register, WORD data)
global pci_writeconfigword
pci_writeconfigword:
	push ebp
	mov ebp,esp
	
	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xb1
	mov al, 11
	mov bh, [ebp+8]
	mov bl, [ebp+12]
	mov edi, [ebp+16]
	mov ecx, [ebp+20]
	
	call far [pcibios]

	shr eax,8
	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

;int _pci_writeconfigdword(char bus, char devfunc, WORD register, DWORD data)
global pci_writeconfigdword
pci_writeconfigdword:
	push ebp
	mov ebp,esp
	
	push ecx
	push edx
	push esi
	push ebx

	mov ah, 0xb1
	mov al, 12
	mov bh, [ebp+8]
	mov bl, [ebp+12]
	mov edi, [ebp+16]
	mov ecx, [ebp+20]
	
	call far [pcibios]

	shr eax,8
	pop ebx	
	pop esi
	pop edx
	pop ecx
	pop ebp
	ret

global pcibios_call

;DWORD pcibios_call(function number)
pcibios_call:
	push ebp
	mov ebp,esp
	push ebx
	push ecx

	mov eax, 0x49435024
	mov bl,0

	call far [pcibiosentry]

	cmp al,0x80
	je pci_error

	cmp al,0x81
	je pci_error

	mov eax,ebx
	add eax,edx
	jmp pci_done
	
	pci_error
	mov eax,-1
	
	pci_done:
		
	pop ecx	
	pop ebx
	pop ebp
	ret



global invtlb
invtlb:
	INVLPG [tlb_address]
ret

global repinword
repinword:
	push      ebp
	mov       ebp,esp
	push      edi
	push	  eax
	push	  ecx
	push	  edx
	mov	  ax,word   [ebp+8]
	mov	  es,ax
	mov	  edi,dword [ebp+12]
	mov	  ecx,dword [ebp+16]
	mov	  dx, word  [ebp+20]
	cld	
	rep   insw	
	pop	  edx
	pop	  ecx
	pop	  eax
	pop       edi
	pop       ebp
	ret 

global repindword
repindword:
	push      ebp
	mov       ebp,esp
	push      edi
	push	  eax
	push	  ecx
	push	  edx
	mov	  ax,word   [ebp+8]
	mov	  es,ax
	mov	  edi,dword [ebp+12]
	mov	  ecx,dword [ebp+16]
	mov	  dx, word  [ebp+20]
	cld	
	rep       insd	
	pop	  edx
	pop	  ecx
	pop	  eax
	pop       edi
	pop       ebp
	ret 

global repoutdword
repoutdword:
	push      ebp
	mov       ebp,esp
	push      esi
   	push	  eax
   	push	  ecx
   	push	  edx
   	push	  esi
   	push	  ds
   	mov	  ax,word [ebp+8]
   	mov	  ds,ax
   	mov	  esi,dword [ebp+12]
   	mov	  ecx,dword  [ebp+16]
   	mov	  dx,word [ebp+20]
   	cld	
   	rep       outsd	
   	pop	  ds
   	pop	  esi
   	pop	  edx
   	pop	  ecx
   	pop	  eax
	pop       esi
	pop       ebp
	ret 

global repoutword
repoutword:
	push      ebp
	mov       ebp,esp
	push      esi
   	push	  eax
   	push	  ecx
   	push	  edx
   	push	  esi
   	push	  ds
   	mov	  ax,word [ebp+8]
   	mov	  ds,ax
   	mov	  esi,dword [ebp+12]
   	mov	  ecx,dword  [ebp+16]
   	mov	  dx,word [ebp+20]
   	cld	
   	rep       outsw	
   	pop	  ds
   	pop	  esi
   	pop	  edx
   	pop	  ecx
   	pop	  eax
	pop       esi
	pop       ebp
	ret 


global repoutbyte
repoutbyte:
   ;	
   ;	void repoutbyte(unsigned short int bufSeg, unsigned int bufOff,
   ;	
	push      ebp
	mov       ebp,esp
	push      esi
   	push	  eax
   	push	  ecx
   	push	  edx
   	push	  esi
   	push	  ds
   	mov	  ax,word [ebp+8]
   	mov	  ds,ax
   	mov	  esi,dword [ebp+12]
   	mov	  ecx,dword  [ebp+16]
   	mov	  dx,word [ebp+20]
   	cld	
   	rep       outsb	
   	pop	  ds
   	pop	  esi
   	pop	  edx
   	pop	  ecx
   	pop	  eax
	pop       esi
	pop       ebp
	ret 


global reptrans
reptrans:
   ;	
   ;	 reptrans(unsigned short int bufSeg, unsigned int bufOff,
   ;	
	push      ebp
	mov       ebp,esp
	push      edi
	push	  eax
	push	  ecx
	push	  edx
	mov	  ax,word   [ebp+8]
	mov	  es,ax
	mov	  edi,dword [ebp+12]
	mov	  ecx,dword [ebp+16]
	mov	  dx, word  [ebp+20]
	cld	
	rep   insb	
	pop	  edx
	pop	  ecx
	pop	  eax
	pop       edi
	pop       ebp
	ret 


global getcpuid
getcpuid:
   ;	void getcpuid(DWORD initial,DWORD *eax,DWORD *ebx,
	push      ebp
	mov       ebp,esp
	add       esp,-16
	push      ebx
	mov	 eax,[ebp+8]
	cpuid	
	mov	 [ebp-4],eax
	mov	 [ebp-8],ebx
	mov	 [ebp-12],ecx
	mov	 [ebp-16],edx
	mov       eax,[ebp-4]
	mov       edx,[ebp+12]
	mov       [edx],eax
	mov       ecx,[ebp-8]
	mov       eax,[ebp+16]
	mov       [eax],ecx
	mov       edx,[ebp+20]
	mov       ecx,[ebp-12]
	mov       [edx],ecx
	mov       eax,[ebp+24]
	mov       edx,[ebp-16]
	mov       [eax],edx
	pop       ebx
	mov       esp,ebp
	pop       ebp
	ret 


global storeflags
storeflags:
push      ebp
mov       ebp,esp
push      ecx
pushf	
pop	 eax
mov	  [ebp-4],eax
mov       eax,[ebp-4]
mov       edx,[ebp+8]
mov       [edx],eax
pop       ecx
pop       ebp
ret 

global restoreflags
restoreflags:
push      ebp
mov       ebp,esp
mov	  eax,[ebp+8]
push	  eax
popf	
pop       ebp
ret 


global loadtsr

; loadstr(WORD sel)
loadtsr:
       mov ax,SCHED_TSS
       ltr ax
       ret 

global switchuserprocess

switchuserprocess:
        jmp USER_TSS:0
ret	


global switchprocess

switchprocess:
        jmp SYS_TSS:0
ret	



mempop:
   ;	
   ;	unsigned int mempop(unsigned int *s)
   ;	
	push      ebp
	mov       ebp,esp
	mov       eax,dword [ebp+8]
   	mov       edx,dword [eax]
	test      edx,edx
	ja        short @9x
	xor       eax,eax
	pop       ebp
	ret 
   
@9x:
	mov       edx,dword [eax+4*edx]
   	dec       dword [eax]
   	mov       eax,edx
	pop       ebp
	ret 





global loadregisters

loadregisters:
lidt [idtr]  ;load em'
ret


global offcomputer

offcomputer:
   push ebp
   mov ebp,esp
   mov edx,[ebp+8]

   mov ax,APM_CS32
   mov es,ax

   mov ah,0x53
   mov al,0x07
   mov bx,1
   mov cx,3
  
   call [es:edx]
   pop ebp
ret



global setpagedir
;setpagedir(pagedir *dir)

setpagedir:
      push ebp
      mov ebp,esp
      push eax
      mov eax,[ebp+8]
      mov cr3,eax
      pop eax
      pop ebp
ret     

global refreshpages
refreshpages:
          

      mov eax,cr3
      mov cr3,eax
  
ret



refreshpagesA:
       cli
       mov eax,cr0
       and eax,0x7FFFFFFF
       mov cr0,eax  
       mov       eax,cr0
       or        eax,0x80000000
       mov       cr0,eax
       sti
ret                  

global disablepaging
disablepaging:
       mov eax,cr0
       and eax,0x7FFFFFFF
       mov cr0,eax
       jmp        SYS_CODE_SEL:here3
       here3:          
       ret

global enablepaging
enablepaging:

       mov        eax,0x80000011 
       mov        cr0,eax
       ;done, referesh the prefetch cache

       jmp        SYS_CODE_SEL:here2
       here2:          
ret

global getCR0
getCR0:
  mov eax,cr0
ret

global setCR0
setCR0:
push ebp
mov ebp,esp
mov eax,[ebp+8]
mov cr0,eax
pop ebp
ret

;global setinterruptvector
setinterruptvector:
   ;	
   ;	void  setinterruptvector(unsigned int x,idtd *t,unsigned char attr,
   ;	

	push      ebp
	mov       ebp,esp
	mov       edx,dword [ebp+12]
	mov       eax,dword [ebp+8]
	mov       ecx,dword [ebp+20]
	mov       word [edx+8*eax],cx
   	shr       ecx,16
	mov       word [edx+8*eax+6],cx
   	xor       ecx,ecx
	mov       cl,byte [ebp+24]
	mov       word [edx+8*eax+2],cx
	mov       byte [edx+8*eax+4],0
   	mov       cl,byte [ebp+16]
	mov       byte [edx+8*eax+5],cl
	pop       ebp
	ret 



xtoa:
?live1@0:

@1:
	push      ebp
	mov       ebp,esp
	push      ecx
	push      ebx
	push      esi
	push      edi
	mov       edi,dword [ebp+8]
	mov       ebx,dword [ebp+12]
	mov       eax,dword [ebp+20]
	test      eax,eax
	je        short @2
	mov       byte [ebx],45
	inc       ebx
	neg       edi
@2:
	mov       esi,ebx
@3:
	mov       eax,edi
	xor       edx,edx
	div       dword [ebp+16]
	mov       dword [ebp-4],edx
	xor       edx,edx
	mov       ecx,dword [ebp+16]
	mov       eax,edi
	div       ecx
	mov       ecx,dword [ebp-4]
	mov       edi,eax
	cmp       ecx,9
	jbe       short @4
	mov       al,byte [ebp-4]
	add       al,87
	mov       byte [ebx],al
	inc       ebx
	jmp       short @5
@4:
	mov       dl,byte [ebp-4]
	add       dl,48
	mov       byte [ebx],dl
	inc       ebx
@5:
	test      edi,edi
	ja        short @3
	mov       byte [ebx],0
	dec       ebx
@7:
	mov       al,byte [ebx]
	mov       dl,byte [esi]
	mov       byte [ebx],dl
	mov       byte [esi],al
	dec       ebx
	inc       esi
	cmp       ebx,esi
	ja        short @7
@9:
	pop       edi
	pop       esi
	pop       ebx
	pop       ecx
	pop       ebp
	ret 

;This has the exact same prototype as its C equivalent
;This converts an integer into a string format
; char *  itoa ( int val, char *buf, int radix )

global itoa

itoa:
@10:
	push      ebp
	mov       ebp,esp
	push      ebx
        push      ecx
        push      edx  
	mov       edx,dword [ebp+16]
	mov       ebx,dword [ebp+12]
	mov       eax,dword [ebp+8]
	cmp       edx,10
	jne       short @11
	test      eax,eax
	jge       short @11
	push      dword 1
	push      edx
	push      ebx
	push      eax
	call      xtoa
	add       esp,16
	jmp       short @12
@11:
	push      dword 0
	push      edx
	push      ebx
	push      eax
	call      xtoa
	add       esp,16
@12:
	mov       eax,ebx
@14:
@13:
        pop       edx
        pop       ecx  
	pop       ebx
	pop       ebp
	ret 
	
	
ultoa:
   ;	
   ;	char *ultoa (
   ;	
	push      ebp
	mov       ebp,esp
	push      ebx
	mov       ebx,dword [ebp+12]
	push      dword 0
	mov       eax,dword [ebp+16]
	push      eax
	push      ebx
	mov       edx,dword [ebp+8]
	push      edx
	call      xtoa
	add       esp,16
	mov       eax,ebx
	pop       ebx
	pop       ebp
	ret 


global _textcolor

;void textcolor(unsigned char c)
;_textcolor:
textcolor:

   ;	
   ;	void stextcolor(unsigned char c)
   ;	
	push      ebp
	mov       ebp,esp
	and       byte [attb],112
	mov       al,byte [ebp+8]
	or        byte [attb],al
	pop       ebp
	ret 

global _textbackground
;_textbackground:
textbackground:
   ;	
   ;	void stextbackground(unsigned char c)
   ;	
	push      ebp
	mov       ebp,esp
	mov       eax,dword [ebp+8]
	and       byte [attb],-113
	shl       al,4
	or        byte [attb],al
	pop       ebp
	ret 





;       move the cursor
;	void gotoxy(char X,char Y)
global gotoxy
;gotoxy:
gotoxy:

@70:
	push      ebp
	mov       ebp,esp
	mov       al,byte  [ebp+8]
	mov       byte  [CsrX],al
	mov       dl,byte [ebp+12]
	mov       byte  [CsrY],dl
	push      edx
	push      eax
	call      move_cursor
	add       esp,8
@71:
	pop       ebp
	ret 
	
outb:
   ;	
   ;	 void  outb (unsigned short int port,unsigned char data)
   ;	
	push      ebp
	mov       ebp,esp
	mov	  al,byte [ebp+12]
     	mov	  dx,word [ebp+8]
   	out	  dx,al
   	pop       ebp
	ret 


MOVECURSOR: 
;BX = New cursor pos 
PUSH EDX 
PUSH EAX 
PUSH EBX 
MOV DX, 0x3D4 ;Hardware Cursor RegIndex Port 
MOV AL, 0xF ;Make Hardware Cursor Low reg available at 0x3D5 
OUT DX, AL 
INC DX ;Hardware Cursor RegData Port 
MOV AL, BL 
OUT DX, AL ;Write Actual Cursor Low reg 
DEC DX ;Hardware Cursor RegIndex Port 
MOV AL, 0xE ;Make Hardware Cursor High reg available at 0x3D5 
OUT DX, AL 
INC DX ;Hardware Cursor RegData Port 
MOV AL, BH 
OUT DX, AL ;Write Actual Cursor High reg 
POP EBX 
POP EAX 
POP EDX 
RET 

global move_cursor
move_cursor:
;move_cursor:

   ;	
   ;	void update_cursor(unsigned char row,unsigned int col)
   ;	
	push      ebp
	mov       ebp,esp
	push      ecx
	push      ebx
	movzx     eax,word [ebp+8]
	mov       edx,eax
	shl       edx,4
	lea       edx,[edx+4*edx]
	add       dx,word [ebp+12]
	mov       word [ebp-2],dx
	movzx	  ebx,word [ebp-2]
	call	  MOVECURSOR
	pop       ebx
	pop       ecx
	pop       ebp
	ret 

	


clrscrC:
        push ebp
        mov ebp,esp
        push es
        push eax
        push ebx
        push edx
        mov edx,[ebp+8]
        mov ax,LINEAR_SEL
        mov es,ax         ;Use the linear selector for this one
        mov eax,0xB8000   ;The  linear address of the video memory
        mov ebx,80*25   ;The size of the video memory
     clr_C:
        inc eax
        inc eax
        dec ebx
       
        mov byte [es:eax],dl
        mov byte [es:eax+1],0 
        cmp ebx,0
        jne clr_C
                ;reset the position of the cursor to the top left
        mov byte [CsrX],0
        mov byte [CsrY],0
        pop edx
        pop ebx
        pop eax
        pop es  
        pop ebp
	ret 	


;The clrscr function using memset
clrscr2:
	push      dword 4000
	push      dword 0
	push      dword 753664
	push      dword LINEAR_SEL
	call      memset
	add       esp,16
	ret 

scrollup:
   ;	
   ;	void scrollup()

	push      dword 3840
	push      dword 753824
	push      dword 753664
	call      linmemmove
	add       esp,12
	ret 



memset:
	push      ebp
	mov       ebp,esp
	push      ebx
	push      eax
	mov       edx,dword [ebp+20]
	jmp       short @_24
   
@_23:
	mov	 ax,word [ebp+8]
   	mov	 gs,ax
   	mov	 ebx,dword [ebp+12]
   	mov	 al,byte [ebp+16]
   	mov	 byte [gs:ebx],al
	inc      dword [ebp+12]
@_24:
	mov       eax,edx
	add       edx,-1
	test      eax,eax
	jne       short @_23
 
        pop       eax
	pop       ebx
	pop       ebp
	ret 

linmemmove:

   ;	
   ;	void memmove (
   ;	
	push      ebp
	mov       ebp,esp
	push      ebx
	push      esi
	
	mov       ax,LINEAR_SEL
	mov       es,ax
	        
	mov       eax,dword  [ebp+8]
	mov       edx,dword  [ebp+12]
 
	cmp       edx,eax
	mov       esi,eax
	mov       ecx,dword  [ebp+16]
	jae       short @304
	mov       ebx,ecx
	add       ebx,edx
	cmp       eax,ebx
	jb        short @301
	jmp       short @304
	
@303:
	mov       bl,byte [es:edx]
	mov       byte  [es:eax],bl
   
	inc       eax
   	inc       edx
@304:
	mov       ebx,ecx
	add       ecx,-1
	test      ebx,ebx
	jne       short @303
   	jmp       short @305
@301:
	mov       ebx,ecx
	add       eax,ebx
	dec       eax
   	add       edx,ebx
	dec       edx
	jmp       short @307
@306:
	mov       bl,byte [es:edx]
	mov       byte [es:eax],bl
   	dec       eax
   	dec       edx
@307:
	mov       ebx,ecx
	add       ecx,-1
	test      ebx,ebx
	jne       short @306
@305:
	mov       eax,esi
	pop       esi
	pop       ebx
	pop       ebp
	ret 


NULL_SEL        equ         0b 
LINEAR_SEL	equ	 1000b
SYS_CODE_SEL2   equ     10000b
SYS_STACK_SEL   equ     11000b
SYS_DATA_SEL    equ    100000b
TEXT_VIDEO_SEL  equ    101000b 
KERNEL_MEM_SEL  equ    110000b
SYS_CODE_SEL    equ    111000b
SYS_TSS         equ 0x80
SCHED_TSS       equ 0x78
USER_CODE       equ 0x88
USER_DATA       equ 0x90
USER_STACK      equ 0x98
USER_TSS        equ 0xA0
SYS_SCHED_SEL   equ 0xA8
SYS_ERROR_TSS   equ 0xB0
APM_CS32        equ 0xC0
APM_DS          equ 0xC8
APM_CS16        equ 0xD0
KEYB_TSS        equ 0xD8
MOUSE_TSS	equ 0xE0
