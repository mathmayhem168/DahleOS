# Changelog

All notable changes to DahleOS are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versioning follows [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

> Changes on `main` not yet tagged as a release go here.

---

## [v0.2.3] — 2026-05-01
### Added
- `matrix` command (BETA): falling-character animation in the VESA framebuffer

---

## [v0.2.2] — 2026-04-29
### Added
- `savelist` command: list all saved aliases and their targets

---

## [v0.2.1] — 2026-04-29
### Added
- ATA PIO persistence driver (`drivers/ata.c`)
- `storage/persist.c`: save/load aliases and filesystem pool to disk
- `save` and `load` shell commands
- Auto-save: aliases saved on every alias command, fs on mkdir/touch/rm/cd
- Boot: `persist_probe()` loads from disk if data exists, else defaults to `fs_init()`
- Disk layout: LBA 0=header, 1–6=aliases, 7–57=fs pool (1 MiB disk.img)

---

## [v0.1.6] — 2026-04-17
### Added
- `alias` command: define and resolve command aliases

---

## [v0.1.5] — 2026-04-17
### Added
- `upper` and `lower` commands: convert shell output to upper/lower case

---

## [v0.1.3] — 2026-04-02
### Added
- Basic virtual filesystem (`fs/fs.c`): mkdir, touch, rm, ls, cd
- Filesystem pool in memory with simple path resolution

---

## [v0.1.2] — 2026-03-30
### Added
- `about` command: display OS name, version, and author info

---

## [v0.1.1] — 2026-03-25
### Added
- Initial shell command infrastructure (`shell/commands.c`, `shell/shell.h`)
- Polished OS information display

---

## [v0.0.1] — 2026-03-22
### Added
- Bootloader (`boot/boot.asm`), GDT, IDT, ISR, IRQ, PIT timer
- VESA framebuffer driver (800×600×32), font renderer
- PS/2 keyboard driver
- Basic GUI widget library (`drivers/gui.c`, `drivers/gui.h`) — teal/cyan theme
- Interactive shell with command table
- `libc/string.c`, `libc/mem.c` (freestanding, no stdlib)
- Makefile for macOS cross-compilation (i686-elf-gcc + NASM)
