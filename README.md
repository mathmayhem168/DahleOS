# DahleOS

A 32-bit x86 bare-metal operating system running on QEMU. Written in C and assembly.

---

## Prerequisites

Install once on macOS with Homebrew:

```bash
brew install i686-elf-gcc i686-elf-binutils nasm qemu
```

---

## Building and Running

```bash
make          # compile → produces os.bin
make run      # compile + boot in QEMU
make iso      # build a bootable ISO (requires grub + xorriso)
make clean    # remove all build artifacts
```

---

## Architecture

```
boot/       Bootloader (NASM) — enters 32-bit protected mode
cpu/        GDT, IDT, ISR, IRQ handlers, PIT timer
drivers/    Screen (VESA framebuffer), keyboard (PS/2), GUI widgets, font, ATA PIO
fs/         In-memory virtual filesystem (mkdir, touch, rm, ls, cd)
include/    Shared types (types.h)
kernel/     Kernel entry point
libc/       Freestanding string and memory utilities
shell/      Interactive shell, command table, all built-in commands
storage/    Persistence layer — saves/loads state to disk via ATA
tools/      Developer scripts (commit helper)
```

---

## Shell Commands

| Command      | Description |
|--------------|-------------|
| `help`       | List all available commands |
| `about`      | Display OS info and version |
| `dahle`      | Launch the 3-window GUI desktop |
| `matrix`     | Falling-character animation (BETA) |
| `ls`         | List files in the current directory |
| `mkdir`      | Create a directory |
| `touch`      | Create a file |
| `rm`         | Remove a file or directory |
| `cd`         | Change directory |
| `alias`      | Define a command alias |
| `savelist`   | List all saved aliases |
| `upper`      | Convert input to uppercase |
| `lower`      | Convert input to lowercase |
| `save`       | Persist aliases and filesystem to disk |
| `load`       | Load aliases and filesystem from disk |

---

## Hardware Specifics

| Property   | Value |
|------------|-------|
| Architecture | x86 32-bit protected mode |
| Video | 800×600×32 VESA framebuffer |
| Keyboard | PS/2 (interrupt-driven) |
| Disk | ATA PIO — `disk.img`, 1 MiB |
| Timer | PIT at 100 Hz |

---

## Disk Layout

Persistent state is stored at fixed LBA offsets in `disk.img`:

| LBA | Contents |
|-----|----------|
| 0 | Header |
| 1–6 | Aliases |
| 7–57 | Filesystem pool |

On boot, `persist_probe()` loads from disk if data exists, otherwise initialises defaults via `fs_init()`.

---

## GUI Development

See [`docs/GUI_GUIDE.md`](docs/GUI_GUIDE.md) for a complete guide to building graphical interfaces using the DahleOS framebuffer widget library.

---

## Contributing / Workflow

See [`github-guide/GUIDE.md`](github-guide/GUIDE.md) for commit conventions, branching, and tagging.
Use [`tools/commit.sh`](tools/commit.sh) as the single entry point for all commits — it enforces the conventional commits format and runs a build check before committing.
