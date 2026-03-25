OUTPUT_FORMAT(elf32-i386)
ENTRY(start)

SECTIONS {
    . = 0x00100000;      /* GRUB loads us at 1 MiB */

    code = .;
    .text   ALIGN(4096) : { *(.text)   }
    data = .;
    .rodata ALIGN(4096) : { *(.rodata*) }
    .data   ALIGN(4096) : { *(.data)   }
    bss  = .;
    .bss    ALIGN(4096) : { *(COMMON) *(.bss) }
    end  = .;
}
