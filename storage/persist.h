#ifndef PERSIST_H
#define PERSIST_H
#include "../include/types.h"
#include "../drivers/ata.h"

#define PERSIST_MAGIC    0xDA41E05u
#define PERSIST_VERSION  1u

/* Disk sector layout */
#define PERSIST_LBA_HEADER   0u   /* 1 sector  — header/magic  */
#define PERSIST_LBA_ALIASES  1u   /* 6 sectors — alias table   */
#define PERSIST_LBA_FS       7u   /* 51 sectors — fs pool      */

/* Written-data flags stored in header.flags (set when data has been saved). */
#define PERSIST_FLAG_ALIASES (1u << 0)
#define PERSIST_FLAG_FS      (1u << 1)

/*
 * Save-list flags stored in header.savelist.
 * When a bit is set the corresponding category is auto-saved on every
 * mutation and included in explicit `save` runs.
 * Default (first boot, no header): all categories enabled.
 */
#define SAVELIST_ALIASES (1u << 0)   /* alias table          */
#define SAVELIST_FS      (1u << 1)   /* filesystem pool      */
#define SAVELIST_ALL     (SAVELIST_ALIASES | SAVELIST_FS)

/* Names used by the `savelist` shell command (must match bit order above). */
#define SAVELIST_COUNT   2
extern const char *savelist_names[SAVELIST_COUNT];   /* {"aliases","fs"} */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;            /* which data slots contain valid data    */
    uint32_t alias_checksum;
    uint32_t fs_checksum;
    uint32_t savelist;         /* which categories are auto-saved        */
    uint8_t  _pad[488];        /* pad to 512 bytes (was 492, minus 4)    */
} __attribute__((packed)) persist_header_t;

/*
 * Probe for a valid disk and saved data.
 * Always call this first — it also sets the internal disk_present flag
 * that gates all subsequent save calls, and loads the stored savelist.
 * Returns 1 if saved data exists, 0 if drive present but no data (or no drive).
 */
int persist_probe(void);

/* Serialize alias_table → sectors 1-6.
   No-op (returns ATA_OK) if SAVELIST_ALIASES is not enabled or no disk. */
int persist_save_aliases(void);

/* Deserialize sectors 1-6 → alias_table.  Silently aborts on bad checksum. */
int persist_load_aliases(void);

/* Serialize fs pool → sectors 7-57.
   No-op (returns ATA_OK) if SAVELIST_FS is not enabled or no disk. */
int persist_save_fs(void);

/* Deserialize sectors 7-57 → fs pool.  Silently aborts on bad checksum. */
int persist_load_fs(void);

/* ---- Save-list API ---- */

/* Return the current savelist bitmask (SAVELIST_* flags). */
uint32_t persist_savelist_get(void);

/*
 * Replace the savelist bitmask and immediately write it to the disk header.
 * No-op if no disk present.
 */
void persist_savelist_set(uint32_t flags);

/* Returns 1 if the given category flag is currently in the save list. */
int persist_savelist_enabled(uint32_t flag);

#endif
