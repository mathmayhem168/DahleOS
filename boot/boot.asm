; =============================================================
;  boot/boot.asm  —  Multiboot entry point
; =============================================================
;  MBOOT_VIDEO (bit 2) asks GRUB/QEMU to set up a linear
;  pixel framebuffer before handing control to the kernel.
;  The four extra DWORDs after the checksum specify the mode.
;  Bit 16 is NOT set, so the loader uses ELF headers for load
;  addresses — no address fields are required here.
; =============================================================

MBOOT_PAGE_ALIGN  equ 1 << 0
MBOOT_MEM_INFO    equ 1 << 1
MBOOT_VIDEO       equ 1 << 2
MBOOT_MAGIC       equ 0x1BADB002
MBOOT_FLAGS       equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_VIDEO
MBOOT_CHECKSUM    equ -(MBOOT_MAGIC + MBOOT_FLAGS)

[BITS 32]
[GLOBAL mboot]

mboot:
    dd  MBOOT_MAGIC
    dd  MBOOT_FLAGS
    dd  MBOOT_CHECKSUM
    ; Video-mode fields (present because MBOOT_VIDEO is set):
    dd  0       ; mode_type : 0 = linear graphics framebuffer
    dd  800     ; preferred width  in pixels
    dd  600     ; preferred height in pixels
    dd  32      ; preferred colour depth in bits-per-pixel

[GLOBAL start]
[EXTERN kmain]

start:
    cli
    mov  esp, _stack_top
    push ebx        ; multiboot_info_t *  (loader puts it in EBX)
    call kmain
    jmp  $

section .bss
    resb 16384
_stack_top:
