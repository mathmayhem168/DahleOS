; cpu/gdt.asm – reload segment registers after lgdt
[GLOBAL gdt_flush]
gdt_flush:
    mov  eax, [esp+4]
    lgdt [eax]
    mov  ax, 0x10       ; kernel data selector
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax
    jmp  0x08:.done     ; far jump → reload CS with kernel code selector
.done:
    ret
