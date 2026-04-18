# ============================================================
#  DahleOS – Makefile
#
#  Prerequisites (macOS – install once with Homebrew):
#    brew install i686-elf-gcc i686-elf-binutils nasm qemu
#
#  Targets:
#    make        – compile  (produces os.bin)
#    make run    – compile + boot in QEMU
#    make iso    – build bootable ISO  (needs grub + xorriso)
#    make clean  – remove build artefacts
# ============================================================

CC  := i686-elf-gcc
LD  := i686-elf-ld
AS  := nasm

CFLAGS  := -m32 -ffreestanding -fno-pic -fno-stack-protector \
            -Wall -Wextra -Werror -c -I.
ASFLAGS := -f elf32
LDFLAGS := -T linker.ld -m elf_i386

# ---- sources ------------------------------------------------
C_SRC := \
    kernel/kernel.c     \
    cpu/gdt.c           \
    cpu/idt.c           \
    cpu/isr.c           \
    cpu/irq.c           \
    cpu/timer.c         \
    drivers/port.c      \
    drivers/ata.c       \
    drivers/screen.c    \
    drivers/gui.c       \
    drivers/font.c      \
    drivers/keyboard.c  \
    libc/string.c       \
    libc/mem.c          \
    fs/fs.c             \
    storage/persist.c   \
    shell/commands.c    \
    shell/shell.c

# ASM objects get an _s suffix to avoid colliding with same-named .c objects
# e.g. cpu/gdt.asm → cpu/gdt_s.o   (cpu/gdt.c → cpu/gdt.o, no conflict)
ASM_SRC := \
    boot/boot.asm   \
    cpu/gdt.asm     \
    cpu/idt.asm     \
    cpu/isr.asm     \
    cpu/irq.asm

C_OBJS   := $(C_SRC:.c=.o)
ASM_OBJS := $(ASM_SRC:.asm=_s.o)
OBJS     := $(ASM_OBJS) $(C_OBJS)

# ---- rules --------------------------------------------------
.PHONY: all run iso clean

all: os.bin
	@echo ""
	@echo "  Build OK  →  os.bin"
	@echo "  Run with: make run"
	@echo ""

os.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%_s.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

disk.img:
	@test -f disk.img || dd if=/dev/zero of=disk.img bs=512 count=2048

run: os.bin disk.img
	qemu-system-i386 -kernel os.bin -vga std \
	    -drive file=disk.img,format=raw,if=ide,index=0,media=disk

iso: os.bin
	mkdir -p iso/boot/grub
	cp os.bin iso/boot/os.bin
	printf 'set timeout=0\nset default=0\nset gfxmode=800x600x32\nset gfxpayload=keep\nmenuentry "DahleOS" { multiboot /boot/os.bin }\n' \
	    > iso/boot/grub/grub.cfg
	grub-mkrescue -o dahleos.iso iso
	@echo "ISO → dahleos.iso"

clean:
	rm -f $(OBJS) os.bin dahleos.iso disk.img
	rm -rf iso
