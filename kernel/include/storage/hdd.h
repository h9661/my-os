#ifndef HDD_H
#define HDD_H

#include "../common/types.h"

/* ATA/IDE Controller Ports */
#define ATA_PRIMARY_BASE        0x1F0
#define ATA_SECONDARY_BASE      0x170

/* Primary ATA/IDE Ports */
#define ATA_REG_DATA            0x1F0   /* Data register */
#define ATA_REG_FEATURES        0x1F1   /* Features/Error register */
#define ATA_REG_SECTOR_COUNT    0x1F2   /* Sector count register */
#define ATA_REG_LBA_LOW         0x1F3   /* LBA low byte */
#define ATA_REG_LBA_MID         0x1F4   /* LBA mid byte */
#define ATA_REG_LBA_HIGH        0x1F5   /* LBA high byte */
#define ATA_REG_DRIVE_HEAD      0x1F6   /* Drive/Head register */
#define ATA_REG_STATUS          0x1F7   /* Status register (read) */
#define ATA_REG_COMMAND         0x1F7   /* Command register (write) */
#define ATA_REG_ALT_STATUS      0x3F6   /* Alternate status register */
#define ATA_REG_DEV_CONTROL     0x3F6   /* Device control register */

/* Secondary ATA/IDE Ports */
#define ATA_SEC_REG_DATA        0x170
#define ATA_SEC_REG_FEATURES    0x171
#define ATA_SEC_REG_SECTOR_COUNT 0x172
#define ATA_SEC_REG_LBA_LOW     0x173
#define ATA_SEC_REG_LBA_MID     0x174
#define ATA_SEC_REG_LBA_HIGH    0x175
#define ATA_SEC_REG_DRIVE_HEAD  0x176
#define ATA_SEC_REG_STATUS      0x177
#define ATA_SEC_REG_COMMAND     0x177
#define ATA_SEC_REG_ALT_STATUS  0x376
#define ATA_SEC_REG_DEV_CONTROL 0x376

/* ATA Commands */
#define ATA_CMD_READ_SECTORS    0x20    /* Read sectors */
#define ATA_CMD_WRITE_SECTORS   0x30    /* Write sectors */
#define ATA_CMD_IDENTIFY        0xEC    /* Identify device */
#define ATA_CMD_FLUSH_CACHE     0xE7    /* Flush cache */

/* Status Register Bits */
#define ATA_STATUS_ERR          0x01    /* Error occurred */
#define ATA_STATUS_DRQ          0x08    /* Data request ready */
#define ATA_STATUS_SRV          0x10    /* Service request */
#define ATA_STATUS_DF           0x20    /* Drive fault */
#define ATA_STATUS_RDY          0x40    /* Drive ready */
#define ATA_STATUS_BSY          0x80    /* Drive busy */

/* Error Register Bits */
#define ATA_ERROR_AMNF          0x01    /* Address mark not found */
#define ATA_ERROR_TK0NF         0x02    /* Track 0 not found */
#define ATA_ERROR_ABRT          0x04    /* Command aborted */
#define ATA_ERROR_MCR           0x08    /* Media change request */
#define ATA_ERROR_IDNF          0x10    /* ID not found */
#define ATA_ERROR_MC            0x20    /* Media changed */
#define ATA_ERROR_UNC           0x40    /* Uncorrectable data error */
#define ATA_ERROR_BBK           0x80    /* Bad block detected */

/* Drive Selection */
#define ATA_DRIVE_MASTER        0xA0    /* Master drive */
#define ATA_DRIVE_SLAVE         0xB0    /* Slave drive */

/* LBA Mode bit */
#define ATA_LBA_MODE            0x40    /* LBA addressing mode */

/* Sector size */
#define HDD_SECTOR_SIZE         512     /* Standard sector size */

/* Maximum sectors per operation */
#define HDD_MAX_SECTORS         256     /* Maximum sectors in one operation */

/* HDD Types */
typedef enum {
    HDD_TYPE_UNKNOWN = 0,
    HDD_TYPE_ATA,
    HDD_TYPE_ATAPI
} hdd_type_t;

/* HDD Drive Information */
typedef struct {
    bool present;                       /* Drive is present */
    hdd_type_t type;                    /* Drive type */
    uint16_t base_port;                 /* Base I/O port */
    uint8_t drive_select;               /* Drive selection byte */
    uint32_t total_sectors;             /* Total number of sectors */
    uint32_t total_size_mb;             /* Total size in MB */
    char model[41];                     /* Model string (null-terminated) */
    char serial[21];                    /* Serial number (null-terminated) */
    char firmware[9];                   /* Firmware revision (null-terminated) */
    bool lba_supported;                 /* LBA addressing supported */
    bool dma_supported;                 /* DMA transfer supported */
} hdd_drive_info_t;

/* HDD Controller Information */
typedef struct {
    hdd_drive_info_t primary_master;    /* Primary master drive */
    hdd_drive_info_t primary_slave;     /* Primary slave drive */
    hdd_drive_info_t secondary_master;  /* Secondary master drive */
    hdd_drive_info_t secondary_slave;   /* Secondary slave drive */
    uint8_t drives_detected;            /* Number of drives detected */
} hdd_controller_t;

/* HDD Operation Result */
typedef enum {
    HDD_SUCCESS = 0,
    HDD_ERROR_NOT_READY,
    HDD_ERROR_TIMEOUT,
    HDD_ERROR_DRIVE_FAULT,
    HDD_ERROR_SEEK_ERROR,
    HDD_ERROR_BAD_SECTOR,
    HDD_ERROR_UNSUPPORTED,
    HDD_ERROR_INVALID_DRIVE,
    HDD_ERROR_INVALID_SECTOR,
    HDD_ERROR_BUFFER_NULL
} hdd_result_t;

/* Function prototypes */

/* Initialization and detection */
void hdd_initialize(void);
bool hdd_detect_drives(void);
void hdd_display_info(void);

/* Drive identification */
hdd_result_t hdd_identify_drive(uint16_t base_port, uint8_t drive_select, hdd_drive_info_t* info);
void hdd_parse_identify_data(uint16_t* identify_data, hdd_drive_info_t* info);

/* Low-level I/O operations */
hdd_result_t hdd_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer);
hdd_result_t hdd_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer);

/* High-level operations */
hdd_result_t hdd_read_sector(uint8_t drive, uint32_t lba, void* buffer);
hdd_result_t hdd_write_sector(uint8_t drive, uint32_t lba, const void* buffer);

/* Get drive size */
hdd_result_t hdd_get_drive_size(uint8_t drive, uint32_t* total_sectors);

/* Utility functions */
bool hdd_wait_ready(uint16_t base_port);
bool hdd_wait_drq(uint16_t base_port);
uint8_t hdd_get_status(uint16_t base_port);
uint8_t hdd_get_error(uint16_t base_port);
void hdd_select_drive(uint16_t base_port, uint8_t drive_select);
void hdd_soft_reset(uint16_t base_port);
const char* hdd_get_error_string(hdd_result_t error);

/* Drive access macros */
#define HDD_PRIMARY_MASTER      0
#define HDD_PRIMARY_SLAVE       1
#define HDD_SECONDARY_MASTER    2
#define HDD_SECONDARY_SLAVE     3

#endif /* HDD_H */
