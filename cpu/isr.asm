; =============================================================
;  cpu/isr.asm  –  CPU exception stubs (vectors 0-31)
;  You will never need to edit this file.
; =============================================================

[EXTERN isr_handler]

; Exception with NO CPU-pushed error code → push dummy 0
%macro ISR_NOERR 1
[GLOBAL isr%1]
isr%1:
    cli
    push dword 0        ; dummy error code
    push dword %1       ; interrupt number
    jmp  isr_common
%endmacro

; Exception WITH CPU-pushed error code (CPU already pushed it)
%macro ISR_ERR 1
[GLOBAL isr%1]
isr%1:
    cli
    push dword %1       ; interrupt number (error code already on stack)
    jmp  isr_common
%endmacro

ISR_NOERR  0    ; #DE  divide by zero
ISR_NOERR  1    ; #DB  debug
ISR_NOERR  2    ;      NMI
ISR_NOERR  3    ; #BP  breakpoint
ISR_NOERR  4    ; #OF  overflow
ISR_NOERR  5    ; #BR  bound range
ISR_NOERR  6    ; #UD  invalid opcode
ISR_NOERR  7    ; #NM  device not available
ISR_ERR    8    ; #DF  double fault
ISR_NOERR  9    ;      coprocessor overrun
ISR_ERR   10    ; #TS  invalid TSS
ISR_ERR   11    ; #NP  segment not present
ISR_ERR   12    ; #SS  stack-segment fault
ISR_ERR   13    ; #GP  general protection fault
ISR_ERR   14    ; #PF  page fault
ISR_NOERR 15
ISR_NOERR 16    ; #MF  x87 FP error
ISR_ERR   17    ; #AC  alignment check
ISR_NOERR 18    ; #MC  machine check
ISR_NOERR 19    ; #XM  SIMD FP exception
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30    ; #SX  security exception
ISR_NOERR 31

; --- Common stub ---
isr_common:
    pusha                   ; edi esi ebp esp ebx edx ecx eax  (edi at lowest addr)
    mov  ax, ds
    push eax                ; save ds

    mov  ax, 0x10           ; switch to kernel data segment
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    push esp                ; arg: registers_t *
    call isr_handler
    add  esp, 4

    pop  eax                ; restore ds
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    popa
    add  esp, 8             ; discard int_no + err_code
    iret
