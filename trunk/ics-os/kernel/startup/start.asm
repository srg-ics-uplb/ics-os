      ; multi-boot mini kernel - [ start.asm ]
      ;
      ; (c) 2002, NeuralDK
      section .text
      bits 32
      ; multi boot header defines
      MBOOT_PAGE_ALIGN   equ 1 << 0
      MBOOT_MEM_INFO     equ 1 << 1
      MBOOT_AOUT_KLUDGE  equ 1 << 16
      MBOOT_MAGIC equ 0x1BADB002
      MBOOT_FLAGS equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_AOUT_KLUDGE
      CHECKSUM    equ -(MBOOT_MAGIC + MBOOT_FLAGS)
      STACK_SIZE  equ 0x1000
      ; defined in the linker script
      extern textEnd
      extern dataEnd
      extern bssEnd
      global  start
      global _start
      entry:
          jmp start
          ; The Multiboot header
      align 4, db 0
      mBootHeader:
          dd MBOOT_MAGIC
          dd MBOOT_FLAGS
          dd CHECKSUM
          ; fields used if MBOOT_AOUT_KLUDGE is set in
          ; MBOOT_HEADER_FLAGS
          dd mBootHeader                     ; these are PHYSICAL addresses
          dd entry                           ; start of kernel .text (code) section
          dd dataEnd                         ; end of kernel .data section
          dd bssEnd                          ; end of kernel BSS
          dd entry                           ; kernel entry point (initial EIP)
       start:
      _start:
          mov edi, 0xB8000
          mov esi, string
          mov ah, 0x0F
        .charLoop:
          lodsb
          stosw
          or al, al
          jnz .charLoop
          jmp short $
      section .data
      string          db "ndk is alive!", 0
      section .bss
          align 4, db 0
          common stack 0x1000
          resb 0x4000

