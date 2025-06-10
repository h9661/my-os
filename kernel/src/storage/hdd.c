#include "../../include/storage/hdd.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"

/* Global HDD controller state */
static hdd_controller_t hdd_controller;

/* Port I/O helper functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Simple delay function */
static void io_delay(void) {
    inb(0x80);  /* Read from unused port for delay */
}

/* Initialize HDD subsystem */
void hdd_initialize(void) {
    terminal_writeline("Initializing HDD subsystem...");
    
    /* Clear controller structure */
    memset(&hdd_controller, 0, sizeof(hdd_controller_t));
    
    /* Reset both ATA controllers */
    hdd_soft_reset(ATA_PRIMARY_BASE);
    hdd_soft_reset(ATA_SECONDARY_BASE);
    
    /* Detect drives */
    if (hdd_detect_drives()) {
        terminal_writeline("HDD subsystem initialized successfully!");
        hdd_display_info();
    } else {
        terminal_writeline("No HDD drives detected.");
    }
}

/* Detect all available drives */
bool hdd_detect_drives(void) {
    bool drives_found = false;
    
    terminal_writeline("Scanning for HDD drives...");
    
    /* Check Primary Master */
    if (hdd_identify_drive(ATA_PRIMARY_BASE, ATA_DRIVE_MASTER, 
                          &hdd_controller.primary_master) == HDD_SUCCESS) {
        hdd_controller.drives_detected++;
        drives_found = true;
        terminal_writeline("✅ Primary Master drive detected");
    }
    
    /* Check Primary Slave */
    if (hdd_identify_drive(ATA_PRIMARY_BASE, ATA_DRIVE_SLAVE, 
                          &hdd_controller.primary_slave) == HDD_SUCCESS) {
        hdd_controller.drives_detected++;
        drives_found = true;
        terminal_writeline("✅ Primary Slave drive detected");
    }
    
    /* Check Secondary Master */
    if (hdd_identify_drive(ATA_SECONDARY_BASE, ATA_DRIVE_MASTER, 
                          &hdd_controller.secondary_master) == HDD_SUCCESS) {
        hdd_controller.drives_detected++;
        drives_found = true;
        terminal_writeline("✅ Secondary Master drive detected");
    }
    
    /* Check Secondary Slave */
    if (hdd_identify_drive(ATA_SECONDARY_BASE, ATA_DRIVE_SLAVE, 
                          &hdd_controller.secondary_slave) == HDD_SUCCESS) {
        hdd_controller.drives_detected++;
        drives_found = true;
        terminal_writeline("✅ Secondary Slave drive detected");
    }
    
    return drives_found;
}

/* Identify a specific drive */
hdd_result_t hdd_identify_drive(uint16_t base_port, uint8_t drive_select, hdd_drive_info_t* info) {
    uint16_t identify_data[256];
    int i;
    
    if (!info) {
        return HDD_ERROR_BUFFER_NULL;
    }
    
    /* Initialize drive info structure */
    memset(info, 0, sizeof(hdd_drive_info_t));
    info->base_port = base_port;
    info->drive_select = drive_select;
    
    /* Select drive */
    hdd_select_drive(base_port, drive_select);
    
    /* Wait for drive to be ready */
    if (!hdd_wait_ready(base_port)) {
        return HDD_ERROR_NOT_READY;
    }
    
    /* Send IDENTIFY command */
    outb(base_port + 7, ATA_CMD_IDENTIFY);  /* Command register */
    
    /* Check if drive exists by reading status */
    uint8_t status = inb(base_port + 7);
    if (status == 0) {
        return HDD_ERROR_INVALID_DRIVE;  /* No drive */
    }
    
    /* Wait for data to be ready */
    if (!hdd_wait_drq(base_port)) {
        return HDD_ERROR_TIMEOUT;
    }
    
    /* Check for errors */
    status = hdd_get_status(base_port);
    if (status & ATA_STATUS_ERR) {
        return HDD_ERROR_DRIVE_FAULT;
    }
    
    /* Read identify data (256 words = 512 bytes) */
    for (i = 0; i < 256; i++) {
        identify_data[i] = inw(base_port);  /* Data register */
    }
    
    /* Parse the identify data */
    hdd_parse_identify_data(identify_data, info);
    info->present = true;
    
    return HDD_SUCCESS;
}/* Parse IDENTIFY command data */
void hdd_parse_identify_data(uint16_t* identify_data, hdd_drive_info_t* info) {
    int i;
    
    /* Get device type */
    if (identify_data[0] & 0x8000) {
        info->type = HDD_TYPE_ATAPI;
    } else {
        info->type = HDD_TYPE_ATA;
    }
    
    /* Extract model string (words 27-46) */
    for (i = 0; i < 20; i++) {
        info->model[i * 2] = (identify_data[27 + i] >> 8) & 0xFF;
        info->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
    }
    info->model[40] = '\0';
    
    /* Extract serial number (words 10-19) */
    for (i = 0; i < 10; i++) {
        info->serial[i * 2] = (identify_data[10 + i] >> 8) & 0xFF;
        info->serial[i * 2 + 1] = identify_data[10 + i] & 0xFF;
    }
    info->serial[20] = '\0';
    
    /* Extract firmware revision (words 23-26) */
    for (i = 0; i < 4; i++) {
        info->firmware[i * 2] = (identify_data[23 + i] >> 8) & 0xFF;
        info->firmware[i * 2 + 1] = identify_data[23 + i] & 0xFF;
    }
    info->firmware[8] = '\0';
    
    /* Check LBA support (bit 9 of word 49) */
    info->lba_supported = (identify_data[49] & 0x0200) != 0;
    
    /* Check DMA support (bit 8 of word 49) */
    info->dma_supported = (identify_data[49] & 0x0100) != 0;
    
    /* Get total sectors (words 60-61 for LBA28) */
    if (info->lba_supported) {
        info->total_sectors = (uint32_t)identify_data[60] | 
                             ((uint32_t)identify_data[61] << 16);
        info->total_size_mb = (info->total_sectors * HDD_SECTOR_SIZE) / (1024 * 1024);
    } else {
        /* For non-LBA drives, calculate from CHS parameters */
        uint16_t cylinders = identify_data[1];
        uint16_t heads = identify_data[3];
        uint16_t sectors = identify_data[6];
        info->total_sectors = cylinders * heads * sectors;
        info->total_size_mb = (info->total_sectors * HDD_SECTOR_SIZE) / (1024 * 1024);
    }
}

/* Display information about detected drives */
void hdd_display_info(void) {
    char num_str[16];
    
    terminal_writeline("\n=== HDD Drive Information ===");
    
    terminal_writestring("Total drives detected: ");
    int_to_string(hdd_controller.drives_detected, num_str);
    terminal_writeline(num_str);
    
    /* Primary Master */
    if (hdd_controller.primary_master.present) {
        terminal_writeline("\n--- Primary Master ---");
        terminal_writestring("Model: ");
        terminal_writeline(hdd_controller.primary_master.model);
        terminal_writestring("Size: ");
        int_to_string(hdd_controller.primary_master.total_size_mb, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" MB");
        terminal_writestring("Sectors: ");
        int_to_string(hdd_controller.primary_master.total_sectors, num_str);
        terminal_writeline(num_str);
        terminal_writestring("LBA Support: ");
        terminal_writeline(hdd_controller.primary_master.lba_supported ? "Yes" : "No");
    }
    
    /* Primary Slave */
    if (hdd_controller.primary_slave.present) {
        terminal_writeline("\n--- Primary Slave ---");
        terminal_writestring("Model: ");
        terminal_writeline(hdd_controller.primary_slave.model);
        terminal_writestring("Size: ");
        int_to_string(hdd_controller.primary_slave.total_size_mb, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" MB");
        terminal_writestring("Sectors: ");
        int_to_string(hdd_controller.primary_slave.total_sectors, num_str);
        terminal_writeline(num_str);
        terminal_writestring("LBA Support: ");
        terminal_writeline(hdd_controller.primary_slave.lba_supported ? "Yes" : "No");
    }
    
    /* Secondary Master */
    if (hdd_controller.secondary_master.present) {
        terminal_writeline("\n--- Secondary Master ---");
        terminal_writestring("Model: ");
        terminal_writeline(hdd_controller.secondary_master.model);
        terminal_writestring("Size: ");
        int_to_string(hdd_controller.secondary_master.total_size_mb, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" MB");
        terminal_writestring("Sectors: ");
        int_to_string(hdd_controller.secondary_master.total_sectors, num_str);
        terminal_writeline(num_str);
        terminal_writestring("LBA Support: ");
        terminal_writeline(hdd_controller.secondary_master.lba_supported ? "Yes" : "No");
    }
    
    /* Secondary Slave */
    if (hdd_controller.secondary_slave.present) {
        terminal_writeline("\n--- Secondary Slave ---");
        terminal_writestring("Model: ");
        terminal_writeline(hdd_controller.secondary_slave.model);
        terminal_writestring("Size: ");
        int_to_string(hdd_controller.secondary_slave.total_size_mb, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" MB");
        terminal_writestring("Sectors: ");
        int_to_string(hdd_controller.secondary_slave.total_sectors, num_str);
        terminal_writeline(num_str);
        terminal_writestring("LBA Support: ");
        terminal_writeline(hdd_controller.secondary_slave.lba_supported ? "Yes" : "No");
    }
    
    terminal_writeline("===============================\n");
}/* Get pointer to drive info structure */
static hdd_drive_info_t* hdd_get_drive_info(uint8_t drive) {
    switch (drive) {
        case HDD_PRIMARY_MASTER:
            return hdd_controller.primary_master.present ? &hdd_controller.primary_master : NULL;
        case HDD_PRIMARY_SLAVE:
            return hdd_controller.primary_slave.present ? &hdd_controller.primary_slave : NULL;
        case HDD_SECONDARY_MASTER:
            return hdd_controller.secondary_master.present ? &hdd_controller.secondary_master : NULL;
        case HDD_SECONDARY_SLAVE:
            return hdd_controller.secondary_slave.present ? &hdd_controller.secondary_slave : NULL;
        default:
            return NULL;
    }
}

/* Read sectors from HDD */
hdd_result_t hdd_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    hdd_drive_info_t* drive_info;
    uint16_t base_port;
    int i, j;
    
    /* Validate parameters */
    if (!buffer || sector_count == 0 || sector_count > HDD_MAX_SECTORS) {
        return HDD_ERROR_BUFFER_NULL;
    }
    
    /* Get drive information */
    drive_info = hdd_get_drive_info(drive);
    if (!drive_info) {
        return HDD_ERROR_INVALID_DRIVE;
    }
    
    /* Check sector bounds */
    if (lba >= drive_info->total_sectors || 
        (lba + sector_count) > drive_info->total_sectors) {
        return HDD_ERROR_INVALID_SECTOR;
    }
    
    base_port = drive_info->base_port;
    
    /* Select drive */
    hdd_select_drive(base_port, drive_info->drive_select);
    
    /* Wait for drive ready */
    if (!hdd_wait_ready(base_port)) {
        return HDD_ERROR_NOT_READY;
    }
    
    /* Set up LBA parameters */
    outb(base_port + 2, sector_count);              /* Sector count */
    outb(base_port + 3, lba & 0xFF);                /* LBA low */
    outb(base_port + 4, (lba >> 8) & 0xFF);         /* LBA mid */
    outb(base_port + 5, (lba >> 16) & 0xFF);        /* LBA high */
    outb(base_port + 6, 0xE0 | (drive & 1) << 4 | ((lba >> 24) & 0x0F)); /* Drive/head */
    
    /* Send read command */
    outb(base_port + 7, ATA_CMD_READ_SECTORS);
    
    /* Read each sector */
    for (i = 0; i < sector_count; i++) {
        /* Wait for data ready */
        if (!hdd_wait_drq(base_port)) {
            return HDD_ERROR_TIMEOUT;
        }
        
        /* Check for errors */
        uint8_t status = hdd_get_status(base_port);
        if (status & ATA_STATUS_ERR) {
            return HDD_ERROR_BAD_SECTOR;
        }
        
        /* Read 256 words (512 bytes) */
        for (j = 0; j < 256; j++) {
            buffer[i * 256 + j] = inw(base_port);
        }
    }
    
    return HDD_SUCCESS;
}

/* Write sectors to HDD */
hdd_result_t hdd_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t* buffer) {
    hdd_drive_info_t* drive_info;
    uint16_t base_port;
    int i, j;
    
    /* Validate parameters */
    if (!buffer || sector_count == 0 || sector_count > HDD_MAX_SECTORS) {
        return HDD_ERROR_BUFFER_NULL;
    }
    
    /* Get drive information */
    drive_info = hdd_get_drive_info(drive);
    if (!drive_info) {
        return HDD_ERROR_INVALID_DRIVE;
    }
    
    /* Check sector bounds */
    if (lba >= drive_info->total_sectors || 
        (lba + sector_count) > drive_info->total_sectors) {
        return HDD_ERROR_INVALID_SECTOR;
    }
    
    base_port = drive_info->base_port;
    
    /* Select drive */
    hdd_select_drive(base_port, drive_info->drive_select);
    
    /* Wait for drive ready */
    if (!hdd_wait_ready(base_port)) {
        return HDD_ERROR_NOT_READY;
    }
    
    /* Set up LBA parameters */
    outb(base_port + 2, sector_count);              /* Sector count */
    outb(base_port + 3, lba & 0xFF);                /* LBA low */
    outb(base_port + 4, (lba >> 8) & 0xFF);         /* LBA mid */
    outb(base_port + 5, (lba >> 16) & 0xFF);        /* LBA high */
    outb(base_port + 6, 0xE0 | (drive & 1) << 4 | ((lba >> 24) & 0x0F)); /* Drive/head */
    
    /* Send write command */
    outb(base_port + 7, ATA_CMD_WRITE_SECTORS);
    
    /* Write each sector */
    for (i = 0; i < sector_count; i++) {
        /* Wait for data ready */
        if (!hdd_wait_drq(base_port)) {
            return HDD_ERROR_TIMEOUT;
        }
        
        /* Check for errors */
        uint8_t status = hdd_get_status(base_port);
        if (status & ATA_STATUS_ERR) {
            return HDD_ERROR_BAD_SECTOR;
        }
        
        /* Write 256 words (512 bytes) */
        for (j = 0; j < 256; j++) {
            outw(base_port, buffer[i * 256 + j]);
        }
    }
    
    /* Flush cache to ensure data is written */
    outb(base_port + 7, ATA_CMD_FLUSH_CACHE);
    if (!hdd_wait_ready(base_port)) {
        return HDD_ERROR_TIMEOUT;
    }
    
    return HDD_SUCCESS;
}/* High-level single sector read */
hdd_result_t hdd_read_sector(uint8_t drive, uint32_t lba, void* buffer) {
    return hdd_read_sectors(drive, lba, 1, (uint16_t*)buffer);
}

/* High-level single sector write */
hdd_result_t hdd_write_sector(uint8_t drive, uint32_t lba, const void* buffer) {
    return hdd_write_sectors(drive, lba, 1, (uint16_t*)buffer);
}

/* Wait for drive to be ready */
bool hdd_wait_ready(uint16_t base_port) {
    int timeout = 1000000;  /* Timeout counter */
    uint8_t status;
    
    while (timeout-- > 0) {
        status = inb(base_port + 7);  /* Status register */
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY)) {
            return true;  /* Drive is ready */
        }
        io_delay();
    }
    
    return false;  /* Timeout */
}

/* Wait for data request */
bool hdd_wait_drq(uint16_t base_port) {
    int timeout = 1000000;  /* Timeout counter */
    uint8_t status;
    
    while (timeout-- > 0) {
        status = inb(base_port + 7);  /* Status register */
        if (status & ATA_STATUS_ERR) {
            return false;  /* Error occurred */
        }
        if (status & ATA_STATUS_DRQ) {
            return true;  /* Data ready */
        }
        io_delay();
    }
    
    return false;  /* Timeout */
}

/* Get drive status */
uint8_t hdd_get_status(uint16_t base_port) {
    return inb(base_port + 7);
}

/* Get error register */
uint8_t hdd_get_error(uint16_t base_port) {
    return inb(base_port + 1);
}

/* Select drive */
void hdd_select_drive(uint16_t base_port, uint8_t drive_select) {
    outb(base_port + 6, drive_select);
    io_delay();  /* Small delay after drive selection */
    io_delay();
    io_delay();
    io_delay();
}

/* Perform soft reset */
void hdd_soft_reset(uint16_t base_port) {
    /* Send reset signal */
    outb(base_port + 0x206, 0x04);  /* Device control register - software reset */
    io_delay();
    
    /* Clear reset signal */
    outb(base_port + 0x206, 0x00);
    io_delay();
    
    /* Wait for reset to complete */
    hdd_wait_ready(base_port);
}

/* Get error description string */
const char* hdd_get_error_string(hdd_result_t error) {
    switch (error) {
        case HDD_SUCCESS:
            return "Operation successful";
        case HDD_ERROR_NOT_READY:
            return "Drive not ready";
        case HDD_ERROR_TIMEOUT:
            return "Operation timeout";
        case HDD_ERROR_DRIVE_FAULT:
            return "Drive fault";
        case HDD_ERROR_SEEK_ERROR:
            return "Seek error";
        case HDD_ERROR_BAD_SECTOR:
            return "Bad sector";
        case HDD_ERROR_UNSUPPORTED:
            return "Operation not supported";
        case HDD_ERROR_INVALID_DRIVE:
            return "Invalid drive";
        case HDD_ERROR_INVALID_SECTOR:
            return "Invalid sector";
        case HDD_ERROR_BUFFER_NULL:
            return "NULL buffer";
        default:
            return "Unknown error";
    }
}/* Get drive size in sectors */
hdd_result_t hdd_get_drive_size(uint8_t drive, uint32_t* total_sectors) {
    hdd_drive_info_t* drive_info;
    
    if (!total_sectors) {
        return HDD_ERROR_BUFFER_NULL;
    }
    
    /* Get drive information */
    drive_info = hdd_get_drive_info(drive);
    if (!drive_info) {
        return HDD_ERROR_INVALID_DRIVE;
    }
    
    *total_sectors = drive_info->total_sectors;
    return HDD_SUCCESS;
}