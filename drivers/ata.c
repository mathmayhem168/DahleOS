#include "ata.h"
#include "port.h"

static int ata_wait_bsy(void) {
    int t = 0x100000;
    while (--t) {
        uint8_t s = port_byte_in(ATA_PRIMARY_BASE + ATA_REG_STATUS);
        if (!(s & ATA_SR_BSY)) {
            if (s & ATA_SR_ERR) return ATA_ERR_ERROR;
            return ATA_OK;
        }
    }
    return ATA_ERR_TIMEOUT;
}

static int ata_wait_drq(void) {
    int t = 0x100000;
    while (--t) {
        uint8_t s = port_byte_in(ATA_PRIMARY_BASE + ATA_REG_STATUS);
        if (s & ATA_SR_ERR) return ATA_ERR_ERROR;
        if (!(s & ATA_SR_BSY) && (s & ATA_SR_DRQ)) return ATA_OK;
    }
    return ATA_ERR_TIMEOUT;
}

/* Reading the alternate status port 4 times gives ~400 ns settling time. */
static void ata_400ns_delay(void) {
    port_byte_in(ATA_PRIMARY_CTRL);
    port_byte_in(ATA_PRIMARY_CTRL);
    port_byte_in(ATA_PRIMARY_CTRL);
    port_byte_in(ATA_PRIMARY_CTRL);
}

static void ata_select_lba28(uint32_t lba, uint8_t count) {
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_LBA_LO,  (uint8_t)(lba));
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_LBA_HI,  (uint8_t)(lba >> 16));
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_DRIVE,
                  ATA_DRIVE_MASTER | ((lba >> 24) & 0x0F));
}

int ata_detect(void) {
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_DRIVE, 0xA0);
    ata_400ns_delay();

    uint8_t s = port_byte_in(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (s == 0xFF) return 0;   /* floating bus — no drive */

    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_CMD, ATA_CMD_IDENTIFY);
    ata_400ns_delay();

    s = port_byte_in(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (s == 0x00) return 0;

    if (ata_wait_bsy() != ATA_OK) return 0;

    s = port_byte_in(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if ((s & ATA_SR_ERR) || !(s & ATA_SR_DRQ)) return 0;

    /* drain IDENTIFY data sector */
    for (int i = 0; i < 256; i++)
        port_word_in(ATA_PRIMARY_BASE + ATA_REG_DATA);

    return 1;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void *buf) {
    if (ata_wait_bsy() != ATA_OK) return ATA_ERR_TIMEOUT;
    ata_select_lba28(lba, count);
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_CMD, ATA_CMD_READ_PIO);

    uint16_t *ptr = (uint16_t *)buf;
    for (int s = 0; s < (int)count; s++) {
        ata_400ns_delay();
        int r = ata_wait_bsy();  if (r != ATA_OK) return r;
        r     = ata_wait_drq();  if (r != ATA_OK) return r;
        for (int i = 0; i < 256; i++)
            ptr[s * 256 + i] = port_word_in(ATA_PRIMARY_BASE + ATA_REG_DATA);
    }
    return ATA_OK;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void *buf) {
    if (ata_wait_bsy() != ATA_OK) return ATA_ERR_TIMEOUT;
    ata_select_lba28(lba, count);
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_CMD, ATA_CMD_WRITE_PIO);

    const uint16_t *ptr = (const uint16_t *)buf;
    for (int s = 0; s < (int)count; s++) {
        ata_400ns_delay();
        int r = ata_wait_drq();  if (r != ATA_OK) return r;
        for (int i = 0; i < 256; i++)
            port_word_out(ATA_PRIMARY_BASE + ATA_REG_DATA, ptr[s * 256 + i]);
    }
    port_byte_out(ATA_PRIMARY_BASE + ATA_REG_CMD, ATA_CMD_FLUSH);
    ata_wait_bsy();
    return ATA_OK;
}
