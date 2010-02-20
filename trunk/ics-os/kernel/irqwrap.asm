;This file handles all the wrappers for the interrupt handlers

[bits 32]

section .text

extern time_handler
extern kbd_irq
extern mouse_irq
extern pagefaulthandler
extern fdchandler
extern api_syscall
extern GPFhandler
extern CPUint
extern nocoprocessor
extern divide_error
extern exc_invalidtss
extern irq_activate

;this function handles the normal DEX32 system calls
global syscallwrapper
syscallwrapper:
push edi
push esi
push edx
push ecx
push ebx
push eax
call api_syscall
add esp,24
iret



global CPUintwrapper
CPUintwrapper:
cli
 call CPUint
sti
iret


global fdcwrapper
fdcwrapper:
cli
pusha
call fdchandler
popa
sti
iret

global copwrapper
copwrapper:
cli
call nocoprocessor
sti
iret

global timerwrapper
timerwrapper:
cli
push ebp
push gs
push fs
push es
push ss
push ds
pusha

call time_handler

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
sti
iret


global kbdwrapper
kbdwrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

call kbd_irq

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp

iret


global mousewrapper
mousewrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

call mouse_irq
;needed to renable interrupts
mov al,0x20
out 0xA0,al
out 0x20,al


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp

iret


global irq1wrapper
irq1wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 1
call irq_activate
add esp,4

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret


global irq2wrapper
irq2wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 2
call irq_activate
add esp,4

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq3wrapper
irq3wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 3
call irq_activate
add esp,4

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq4wrapper
irq4wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 4
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq5wrapper
irq5wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 5
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq6wrapper
irq6wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 6
call irq_activate
add esp,4

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq7wrapper
irq7wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 7
call irq_activate
add esp,4

popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq8wrapper
irq8wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 8
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq9wrapper
irq9wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 9
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq10wrapper
irq10wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 10
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq11wrapper
irq11wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 11
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global irq12wrapper
irq12wrapper:
push ebp
push gs
push fs
push es
push ss
push ds
pusha

push 12
call irq_activate
add esp,4


popa
pop ds
pop ss
pop es
pop fs
pop gs
pop ebp
iret

global div_wrapper
div_wrapper:
cli 
push ebp
mov ebp,esp
mov eax,[ebp+8]
push eax
call divide_error
add esp,4
pop ebp
sti
iret

global gpfwrapper
gpfwrapper:
cli 
push ebp
mov ebp,esp
mov eax,[ebp+8]
push eax
call GPFhandler
add esp,4
pop ebp
sti
iret

global invalidtsswrapper
invalidtsswrapper:
cli 
push ebp
mov ebp,esp
mov eax,[ebp+8]
push eax
call exc_invalidtss
add esp,4
pop ebp
sti
iret


global pfwrapper
pfwrapper:
cli
push ebp
mov ebp,esp
mov eax,[ebp+8]
push eax
mov eax,cr2
push eax
call pagefaulthandler
add esp,8
sti
iret

SYS_CODE_SEL2   equ     10000b
SYS_STACK_SEL   equ     11000b
SYS_DATA_SEL    equ    100000b
