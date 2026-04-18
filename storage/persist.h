#ifndef PERSIST_H
#define PERSIST_H
#include "../include/types.h"
#include "../drivers/ata.h"

#define PERSIST_MAGIC    0xDA41E05u
#define PERSIST_VERSION  1u

/* Disk sector layout */
#define PERSIST_LBA_HEADER   0u   /* 1 sector  — header/magic */
#define PERSIST_LBA_ALIASES  1u   /* 6 sectors — alias table  */
#define PERSIST_LBA_FS       7u   /* 51 sectors — fs pool     */

#define PERSIST_FLAG_ALIASES (1u << 0)
#define PERSIST_FLAG_FS      (1u << 1)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t alias_checksum;
    uint32_t fs_checksum;
    uint8_t  _pad[492];
} __attribute__((packed)) persist_header_t;

/*
 * Probe for a valid disk and saved data.
 * Always call this first — it also sets the internal disk_present flag
 * that gates all subsequent save calls.
 * Returns 1 if saved data exists, 0 if drive present but no data (or no drive).
 */
int persist_probe(void);

/* Serialize alias_table → sectors 1-6.  No-op if no disk detected. */
int persist_save_aliases(void);

/* Deserialize sectors 1-6 → alias_table.  Silently aborts on bad checksum. */
int persist_load_aliases(void);

/* Serialize fs pool → sectors 7-57.  No-op if no disk detected. */
int persist_save_fs(void);

/* Deserialize sectors 7-57 → fs pool.  Silently aborts on bad checksum. */
int persist_load_fs(void);

#endif
