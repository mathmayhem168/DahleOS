#include "persist.h"
#include "../drivers/ata.h"
#include "../shell/commands.h"
#include "../fs/fs.h"
#include "../libc/mem.h"

/* Set by persist_probe(); gates all save/load calls. */
static int disk_present = 0;

/*
 * Staging buffers at file scope — must NOT be on the stack
 * (boot.asm stack is only 16 KiB; these are 3 KB + 26 KB).
 */
static uint8_t alias_buf[6  * 512];   /* 3072  bytes */
static uint8_t fs_buf   [51 * 512];   /* 26112 bytes */

static uint32_t checksum(const uint8_t *p, uint32_t len) {
    uint32_t s = 0;
    for (uint32_t i = 0; i < len; i++) s += p[i];
    return s;
}

static int write_header(uint32_t flags,
                        uint32_t alias_csum,
                        uint32_t fs_csum) {
    static uint8_t hdr_buf[512];
    memset(hdr_buf, 0, 512);
    persist_header_t *h = (persist_header_t *)hdr_buf;
    h->magic          = PERSIST_MAGIC;
    h->version        = PERSIST_VERSION;
    h->flags          = flags;
    h->alias_checksum = alias_csum;
    h->fs_checksum    = fs_csum;
    return ata_write_sectors(PERSIST_LBA_HEADER, 1, hdr_buf);
}

/* ------------------------------------------------------------------ */

int persist_probe(void) {
    disk_present = ata_detect();
    if (!disk_present) return 0;

    static uint8_t hdr_buf[512];
    if (ata_read_sectors(PERSIST_LBA_HEADER, 1, hdr_buf) != ATA_OK) return 0;

    persist_header_t *h = (persist_header_t *)hdr_buf;
    return (h->magic == PERSIST_MAGIC && h->version == PERSIST_VERSION) ? 1 : 0;
}

/* ---- Aliases ---- */

int persist_save_aliases(void) {
    if (!disk_present) return ATA_ERR_NODRIVE;

    memset(alias_buf, 0, sizeof(alias_buf));

    int count = alias_get_count();
    memcpy(alias_buf, &count, sizeof(int));
    memcpy(alias_buf + sizeof(int),
           alias_get_table_ptr(),
           alias_get_table_size());

    uint32_t csum = checksum(alias_buf, sizeof(alias_buf));

    int r = ata_write_sectors(PERSIST_LBA_ALIASES, 6, alias_buf);
    if (r != ATA_OK) return r;

    /* read back current header so we preserve fs_checksum */
    static uint8_t hdr_buf[512];
    ata_read_sectors(PERSIST_LBA_HEADER, 1, hdr_buf);
    persist_header_t *h = (persist_header_t *)hdr_buf;
    uint32_t fs_csum = (h->magic == PERSIST_MAGIC) ? h->fs_checksum : 0;
    uint32_t flags   = (h->magic == PERSIST_MAGIC) ? h->flags        : 0;

    return write_header(flags | PERSIST_FLAG_ALIASES, csum, fs_csum);
}

int persist_load_aliases(void) {
    if (!disk_present) return ATA_ERR_NODRIVE;

    /* read header to get expected checksum */
    static uint8_t hdr_buf[512];
    if (ata_read_sectors(PERSIST_LBA_HEADER, 1, hdr_buf) != ATA_OK) return ATA_ERR_ERROR;
    persist_header_t *h = (persist_header_t *)hdr_buf;
    if (h->magic != PERSIST_MAGIC) return ATA_ERR_ERROR;
    if (!(h->flags & PERSIST_FLAG_ALIASES)) return ATA_OK;   /* no alias data saved yet */

    if (ata_read_sectors(PERSIST_LBA_ALIASES, 6, alias_buf) != ATA_OK) return ATA_ERR_ERROR;

    if (checksum(alias_buf, sizeof(alias_buf)) != h->alias_checksum)
        return ATA_ERR_ERROR;   /* corrupt — leave defaults in place */

    int count;
    memcpy(&count, alias_buf, sizeof(int));
    alias_set_count(count);
    alias_set_table(alias_buf + sizeof(int), alias_get_table_size());
    return ATA_OK;
}

/* ---- Filesystem ---- */

int persist_save_fs(void) {
    if (!disk_present) return ATA_ERR_NODRIVE;

    memset(fs_buf, 0, sizeof(fs_buf));

    int pool_used = fs_get_pool_used();
    int cwd       = fs_get_cwd();
    memcpy(fs_buf,              &pool_used, sizeof(int));
    memcpy(fs_buf + sizeof(int), &cwd,      sizeof(int));
    memcpy(fs_buf + 2 * sizeof(int),
           fs_get_pool_ptr(),
           fs_get_pool_size());

    uint32_t csum = checksum(fs_buf, sizeof(fs_buf));

    int r = ata_write_sectors(PERSIST_LBA_FS, 51, fs_buf);
    if (r != ATA_OK) return r;

    static uint8_t hdr_buf[512];
    ata_read_sectors(PERSIST_LBA_HEADER, 1, hdr_buf);
    persist_header_t *h = (persist_header_t *)hdr_buf;
    uint32_t alias_csum = (h->magic == PERSIST_MAGIC) ? h->alias_checksum : 0;
    uint32_t flags      = (h->magic == PERSIST_MAGIC) ? h->flags           : 0;

    return write_header(flags | PERSIST_FLAG_FS, alias_csum, csum);
}

int persist_load_fs(void) {
    if (!disk_present) return ATA_ERR_NODRIVE;

    static uint8_t hdr_buf[512];
    if (ata_read_sectors(PERSIST_LBA_HEADER, 1, hdr_buf) != ATA_OK) return ATA_ERR_ERROR;
    persist_header_t *h = (persist_header_t *)hdr_buf;
    if (h->magic != PERSIST_MAGIC) return ATA_ERR_ERROR;
    if (!(h->flags & PERSIST_FLAG_FS)) return ATA_ERR_ERROR;   /* no fs data yet */

    if (ata_read_sectors(PERSIST_LBA_FS, 51, fs_buf) != ATA_OK) return ATA_ERR_ERROR;

    if (checksum(fs_buf, sizeof(fs_buf)) != h->fs_checksum)
        return ATA_ERR_ERROR;

    int pool_used, cwd;
    memcpy(&pool_used, fs_buf,              sizeof(int));
    memcpy(&cwd,       fs_buf + sizeof(int), sizeof(int));
    fs_load_state(pool_used, cwd,
                  fs_buf + 2 * sizeof(int),
                  fs_get_pool_size());
    return ATA_OK;
}
