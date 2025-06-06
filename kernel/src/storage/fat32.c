#include "../../include/storage/fat32.h"
#include "../../include/storage/hdd.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"

/* Forward declarations for internal helper functions */
static fat32_result_t fat32_read_cluster(uint32_t cluster, uint8_t* buffer);
static fat32_result_t fat32_write_cluster(uint32_t cluster, const uint8_t* buffer);
static uint32_t fat32_get_first_cluster(const fat32_dir_entry_t* entry);
static void fat32_set_first_cluster(fat32_dir_entry_t* entry, uint32_t cluster);
static fat32_result_t fat32_parse_path(const char* path, char components[][12], int* component_count);
static fat32_result_t fat32_find_entry_in_directory(uint32_t dir_cluster, const char* name_83,
                                                   fat32_dir_entry_t* entry, uint32_t* entry_cluster, uint32_t* entry_offset);

/* Global FAT32 volume information */
static fat32_volume_t fat32_volume;

/* Current working directory cluster */
static uint32_t fat32_current_directory = 0;

/* Temporary sector buffer for various operations */
static uint8_t fat32_sector_buffer[FAT32_SECTOR_SIZE];

/* Debug function for printing messages */
static void fat32_debug(const char* message) {
#ifdef FAT32_DEBUG
    terminal_writestring("FAT32: ");
    terminal_writeline(message);
#endif
}

/* Convert cluster number to LBA (Logical Block Address) */
uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    /* Make sure the cluster number is valid */
    if (cluster < 2) {
        return 0;  /* Invalid cluster number */
    }
    
    /* Calculate LBA from cluster number */
    return fat32_volume.cluster_begin_lba + 
           ((cluster - 2) * fat32_volume.sectors_per_cluster);
}

/* Get next cluster in the chain from FAT */
uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset, fat_sector, ent_offset;
    uint32_t next_cluster;
    
    /* Calculate position in FAT */
    fat_offset = cluster * 4;  /* 4 bytes per entry in FAT32 */
    fat_sector = fat32_volume.fat_begin_lba + (fat_offset / FAT32_SECTOR_SIZE);
    ent_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    /* Read the FAT sector */
    if (hdd_read_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
        return FAT32_EOC;  /* Error reading FAT */
    }
    
    /* Extract the cluster value */
    next_cluster = *((uint32_t*)&fat32_sector_buffer[ent_offset]) & FAT32_CLUSTER_MASK;
    
    /* Check if it's end of chain */
    if (next_cluster >= FAT32_EOC) {
        return FAT32_EOC;
    }
    
    return next_cluster;
}

/* Set next cluster in the chain in FAT */
fat32_result_t fat32_set_next_cluster(uint32_t cluster, uint32_t next_cluster) {
    uint32_t fat_offset, fat_sector, ent_offset;
    uint32_t value;
    
    /* Calculate position in FAT */
    fat_offset = cluster * 4;  /* 4 bytes per entry in FAT32 */
    fat_sector = fat32_volume.fat_begin_lba + (fat_offset / FAT32_SECTOR_SIZE);
    ent_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    /* Read the FAT sector */
    if (hdd_read_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    /* Preserve top 4 bits which are reserved */
    value = (*((uint32_t*)&fat32_sector_buffer[ent_offset]) & 0xF0000000) | 
            (next_cluster & FAT32_CLUSTER_MASK);
    
    /* Update the cluster value */
    *((uint32_t*)&fat32_sector_buffer[ent_offset]) = value;
    
    /* Write the FAT sector back */
    if (hdd_write_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* If we have multiple FATs, update all of them */
    for (uint8_t i = 1; i < fat32_volume.num_fats; i++) {
        fat_sector = fat32_volume.fat_begin_lba + (i * fat32_volume.fat_size) + 
                     (fat_offset / FAT32_SECTOR_SIZE);
        
        if (hdd_write_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
            fat32_debug("Warning: Failed to update additional FAT copy");
            /* Continue anyway since the primary FAT was updated */
        }
    }
    
    return FAT32_SUCCESS;
}

/* Allocate a new cluster and optionally link it to a chain */
fat32_result_t fat32_allocate_cluster(uint32_t* cluster) {
    uint32_t current_cluster = fat32_volume.next_free_cluster;
    uint32_t start_cluster = current_cluster;
    bool found = false;
    
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    /* Start searching from the next_free_cluster hint */
    while (!found) {
        /* Check if this cluster is free */
        if (fat32_get_next_cluster(current_cluster) == FAT32_FREE_CLUSTER) {
            found = true;
            break;
        }
        
        /* Move to the next cluster */
        current_cluster++;
        
        /* Wrap around if we reach the end */
        if (current_cluster >= fat32_volume.total_clusters + 2) {
            current_cluster = 2;  /* First valid cluster number */
        }
        
        /* If we've checked all clusters and found none, the disk is full */
        if (current_cluster == start_cluster) {
            return FAT32_ERROR_DISK_FULL;
        }
    }
    
    /* Mark the cluster as end-of-chain */
    if (fat32_set_next_cluster(current_cluster, FAT32_EOC) != FAT32_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Update the next free cluster hint */
    fat32_volume.next_free_cluster = current_cluster + 1;
    if (fat32_volume.next_free_cluster >= fat32_volume.total_clusters + 2) {
        fat32_volume.next_free_cluster = 2;
    }
    
    /* Update free clusters count */
    if (fat32_volume.free_clusters > 0) {
        fat32_volume.free_clusters--;
    }
    
    /* Return the allocated cluster */
    *cluster = current_cluster;
    return FAT32_SUCCESS;
}

/* Free a cluster chain starting from the given cluster */
fat32_result_t fat32_free_cluster_chain(uint32_t start_cluster) {
    uint32_t current_cluster = start_cluster;
    uint32_t next_cluster;
    
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    /* Invalid starting cluster */
    if (current_cluster < 2) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    /* Follow the chain and mark each cluster as free */
    while (current_cluster != FAT32_EOC && current_cluster >= 2) {
        /* Get the next cluster before freeing this one */
        next_cluster = fat32_get_next_cluster(current_cluster);
        
        /* Mark this cluster as free */
        if (fat32_set_next_cluster(current_cluster, FAT32_FREE_CLUSTER) != FAT32_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
        
        /* Update free clusters count */
        fat32_volume.free_clusters++;
        
        /* Move to the next cluster */
        current_cluster = next_cluster;
    }
    
    /* Update the next free cluster hint if needed */
    if (start_cluster < fat32_volume.next_free_cluster) {
        fat32_volume.next_free_cluster = start_cluster;
    }
    
    return FAT32_SUCCESS;
}

/* Calculate checksum for a short filename (used for LFN entries) */
uint8_t fat32_calculate_checksum(const char* short_name) {
    uint8_t checksum = 0;
    
    for (int i = 0; i < 11; i++) {
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + short_name[i];
    }
    
    return checksum;
}

/* Convert a filename to 8.3 format */
void fat32_filename_to_83(const char* filename, char* shortname) {
    int i, j;
    int len = strlen(filename);
    bool has_extension = false;
    int ext_pos = 0;
    
    /* Initialize with spaces */
    for (i = 0; i < 11; i++) {
        shortname[i] = ' ';
    }
    
    /* Find the extension (last dot) */
    for (i = len - 1; i >= 0; i--) {
        if (filename[i] == '.') {
            has_extension = true;
            ext_pos = i;
            break;
        }
    }
    
    /* Copy the main part (up to 8 chars) */
    for (i = 0, j = 0; i < (has_extension ? ext_pos : len) && j < 8; i++) {
        char c = filename[i];
        
        /* Skip dots and spaces */
        if (c == '.' || c == ' ') {
            continue;
        }
        
        /* Convert to uppercase */
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
        
        shortname[j++] = c;
    }
    
    /* Copy the extension (up to 3 chars) */
    if (has_extension) {
        for (i = ext_pos + 1, j = 8; i < len && j < 11; i++) {
            char c = filename[i];
            
            /* Skip dots and spaces */
            if (c == '.' || c == ' ') {
                continue;
            }
            
            /* Convert to uppercase */
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
            
            shortname[j++] = c;
        }
    }
}

/* Convert an 8.3 directory entry to a filename */
void fat32_83_to_filename(const fat32_dir_entry_t* entry, char* filename) {
    int i, j = 0;
    
    /* Copy the name part (ignoring trailing spaces) */
    for (i = 0; i < FAT32_SFN_NAME_SIZE; i++) {
        if (entry->name[i] != ' ') {
            filename[j++] = entry->name[i];
        }
    }
    
    /* Add a dot if there's an extension */
    if (entry->ext[0] != ' ') {
        filename[j++] = '.';
        
        /* Copy the extension part */
        for (i = 0; i < FAT32_SFN_EXT_SIZE; i++) {
            if (entry->ext[i] != ' ') {
                filename[j++] = entry->ext[i];
            }
        }
    }
    
    /* Null-terminate the string */
    filename[j] = '\0';
}

/* Initialize the FAT32 file system on the specified drive */
fat32_result_t fat32_initialize(uint8_t drive) {
    /* Check if the drive is valid */
    if (drive > HDD_SECONDARY_SLAVE) {
        fat32_debug("Invalid drive number");
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear volume information */
    memset(&fat32_volume, 0, sizeof(fat32_volume_t));
    fat32_volume.drive = drive;
    
    /* Read the boot sector */
    if (hdd_read_sector(drive, 0, &fat32_volume.boot_sector) != HDD_SUCCESS) {
        fat32_debug("Failed to read boot sector");
        return FAT32_ERROR_READ_FAILED;
    }
    
    /* Validate the file system signature */
    if (fat32_volume.boot_sector.boot_signature != FAT32_SIGNATURE) {
        fat32_debug("Invalid boot signature");
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Check for the "FAT32" string in the file system type field */
    if (memcmp(fat32_volume.boot_sector.fs_type, FAT32_FILE_SYSTEM_TYPE, 8) != 0) {
        fat32_debug("Not a FAT32 file system");
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Fill in volume information from the boot sector */
    fat32_volume.bytes_per_sector = fat32_volume.boot_sector.bytes_per_sector;
    fat32_volume.sectors_per_cluster = fat32_volume.boot_sector.sectors_per_cluster;
    fat32_volume.num_fats = fat32_volume.boot_sector.num_fats;
    fat32_volume.root_dir_first_cluster = fat32_volume.boot_sector.root_cluster;
    
    /* Calculate important sector locations */
    fat32_volume.fat_begin_lba = fat32_volume.boot_sector.reserved_sectors;
    fat32_volume.fat_size = fat32_volume.boot_sector.sectors_per_fat_32;
    
    /* Calculate beginning of data area (clusters) */
    fat32_volume.cluster_begin_lba = fat32_volume.fat_begin_lba + 
                                    (fat32_volume.num_fats * fat32_volume.fat_size);
    
    /* Calculate total sectors */
    if (fat32_volume.boot_sector.total_sectors_16 != 0) {
        fat32_volume.total_sectors = fat32_volume.boot_sector.total_sectors_16;
    } else {
        fat32_volume.total_sectors = fat32_volume.boot_sector.total_sectors_32;
    }
    
    /* Calculate data sectors and total clusters */
    fat32_volume.data_sectors = fat32_volume.total_sectors - fat32_volume.cluster_begin_lba;
    fat32_volume.total_clusters = fat32_volume.data_sectors / fat32_volume.sectors_per_cluster;
    
    /* Get volume label */
    memcpy(fat32_volume.volume_label, fat32_volume.boot_sector.volume_label, 11);
    fat32_volume.volume_label[11] = '\0';
    
    /* Try to read the FSInfo sector */
    fat32_fsinfo_t fsinfo;
    if (fat32_volume.boot_sector.fs_info != 0) {
        if (hdd_read_sector(drive, fat32_volume.boot_sector.fs_info, &fsinfo) == HDD_SUCCESS) {
            /* Validate FSInfo signatures */
            if (fsinfo.lead_signature == 0x41615252 && 
                fsinfo.structure_signature == 0x61417272 &&
                fsinfo.trail_signature == 0xAA550000) {
                
                fat32_volume.free_clusters = fsinfo.free_cluster_count;
                fat32_volume.next_free_cluster = fsinfo.next_free_cluster;
                
                if (fat32_volume.next_free_cluster < 2 || 
                    fat32_volume.next_free_cluster >= fat32_volume.total_clusters + 2) {
                    fat32_volume.next_free_cluster = 2;
                }
            }
        }
    }
    
    /* Default values if FSInfo wasn't available or valid */
    if (fat32_volume.next_free_cluster < 2) {
        fat32_volume.next_free_cluster = 2;
    }
    
    /* Mark as initialized */
    fat32_volume.initialized = true;
    
    /* Set current directory to root */
    fat32_current_directory = fat32_volume.root_dir_first_cluster;
    
    /* Debug information */
    fat32_debug("FAT32 file system initialized successfully");
    
    return FAT32_SUCCESS;
}

/* Shutdown the file system and update FSInfo */
void fat32_shutdown(void) {
    fat32_fsinfo_t fsinfo;
    
    if (!fat32_volume.initialized) {
        return;
    }
    
    /* Update FSInfo sector if available */
    if (fat32_volume.boot_sector.fs_info != 0) {
        /* Read current FSInfo */
        if (hdd_read_sector(fat32_volume.drive, fat32_volume.boot_sector.fs_info, &fsinfo) == HDD_SUCCESS) {
            /* Update free cluster count and next free cluster hint */
            fsinfo.free_cluster_count = fat32_volume.free_clusters;
            fsinfo.next_free_cluster = fat32_volume.next_free_cluster;
            
            /* Write back the FSInfo sector */
            hdd_write_sector(fat32_volume.drive, fat32_volume.boot_sector.fs_info, &fsinfo);
        }
    }
    
    /* Mark as uninitialized */
    fat32_volume.initialized = false;
    
    fat32_debug("FAT32 file system shut down");
}

/* Get volume information */
fat32_result_t fat32_get_volume_info(fat32_volume_t* info) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (info == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Copy volume information */
    memcpy(info, &fat32_volume, sizeof(fat32_volume_t));
    
    return FAT32_SUCCESS;
}

/* Check if a drive has a valid FAT32 file system */
fat32_result_t fat32_check_filesystem(uint8_t drive) {
    fat32_boot_sector_t boot_sector;
    
    /* Check if the drive is valid */
    if (drive > HDD_SECONDARY_SLAVE) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Read the boot sector */
    if (hdd_read_sector(drive, 0, &boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    /* Validate the file system signature */
    if (boot_sector.boot_signature != FAT32_SIGNATURE) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Check for the "FAT32" string in the file system type field */
    if (memcmp(boot_sector.fs_type, FAT32_FILE_SYSTEM_TYPE, 8) != 0) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    return FAT32_SUCCESS;
}

/* Format a drive as FAT32 */
fat32_result_t fat32_format_drive(uint8_t drive, const char* volume_label) {
    fat32_debug("FAT32 format operation starting");
    
    if (volume_label == NULL) {
        volume_label = "NEW VOLUME ";
    }
    
    /* For this implementation, we'll use default values for formatting
     * In a full implementation, we would query the HDD controller for drive size
     * For now, assume a reasonably sized drive (e.g., 1GB+)
     */
    uint32_t total_sectors = 2097152;  /* 1GB worth of 512-byte sectors */
    
    /* Calculate cluster size based on assumed drive size */
    uint8_t sectors_per_cluster = 8;  /* 4KB clusters for 1GB drive */
    
    /* Calculate FAT32 parameters */
    uint16_t reserved_sectors = 32;  /* Standard for FAT32 */
    uint8_t num_fats = 2;           /* Standard - 2 copies */
    
    /* Calculate sectors per FAT */
    uint32_t data_sectors = total_sectors - reserved_sectors;
    uint32_t tmp1 = data_sectors / sectors_per_cluster;
    uint32_t tmp2 = (tmp1 * 4 + (FAT32_SECTOR_SIZE - 1)) / FAT32_SECTOR_SIZE;
    uint32_t sectors_per_fat = (data_sectors - (num_fats * tmp2)) / sectors_per_cluster;
    sectors_per_fat = (sectors_per_fat * 4 + (FAT32_SECTOR_SIZE - 1)) / FAT32_SECTOR_SIZE;
    
    /* Create boot sector */
    fat32_boot_sector_t boot_sector;
    memset(&boot_sector, 0, sizeof(fat32_boot_sector_t));
    
    /* Jump instruction */
    boot_sector.jump_boot[0] = 0xEB;
    boot_sector.jump_boot[1] = 0x58;
    boot_sector.jump_boot[2] = 0x90;
    
    /* OEM Name */
    memcpy(boot_sector.oem_name, "MSWIN4.1", 8);
    
    /* BIOS Parameter Block */
    boot_sector.bytes_per_sector = FAT32_SECTOR_SIZE;
    boot_sector.sectors_per_cluster = sectors_per_cluster;
    boot_sector.reserved_sectors = reserved_sectors;
    boot_sector.num_fats = num_fats;
    boot_sector.root_entry_count = 0;  /* 0 for FAT32 */
    boot_sector.total_sectors_16 = 0;  /* 0 for FAT32 */
    boot_sector.media_type = 0xF8;     /* Fixed disk */
    boot_sector.sectors_per_fat_16 = 0; /* 0 for FAT32 */
    boot_sector.sectors_per_track = 63;
    boot_sector.num_heads = 255;
    boot_sector.hidden_sectors = 0;
    boot_sector.total_sectors_32 = total_sectors;
    
    /* FAT32 Extended BPB */
    boot_sector.sectors_per_fat_32 = sectors_per_fat;
    boot_sector.ext_flags = 0;
    boot_sector.fs_version = 0;
    boot_sector.root_cluster = 2;  /* Root directory starts at cluster 2 */
    boot_sector.fs_info = 1;       /* FSInfo at sector 1 */
    boot_sector.backup_boot = 6;   /* Backup boot sector at sector 6 */
    
    /* Extended fields */
    boot_sector.drive_number = 0x80;   /* Hard disk */
    boot_sector.boot_signature = FAT32_SIGNATURE;
    boot_sector.volume_id = 0x12345678; /* Random volume ID */
    
    /* Volume label */
    memset(boot_sector.volume_label, ' ', 11);
    int label_len = strlen(volume_label);
    if (label_len > 11) label_len = 11;
    memcpy(boot_sector.volume_label, volume_label, label_len);
    
    /* File system type */
    memcpy(boot_sector.fs_type, FAT32_FILE_SYSTEM_TYPE, 8);
    
    /* Write boot sector */
    if (hdd_write_sector(drive, 0, &boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Write backup boot sector */
    if (hdd_write_sector(drive, 6, &boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Create and write FSInfo sector */
    fat32_fsinfo_t fsinfo;
    memset(&fsinfo, 0, sizeof(fat32_fsinfo_t));
    
    fsinfo.lead_signature = 0x41615252;
    fsinfo.structure_signature = 0x61417272;
    fsinfo.free_cluster_count = 0xFFFFFFFF;  /* Unknown */
    fsinfo.next_free_cluster = 3;  /* Start searching from cluster 3 */
    fsinfo.trail_signature = 0xAA550000;
    
    if (hdd_write_sector(drive, 1, &fsinfo) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Initialize FAT tables */
    uint8_t fat_buffer[FAT32_SECTOR_SIZE];
    memset(fat_buffer, 0, FAT32_SECTOR_SIZE);
    
    /* Write first sector of first FAT */
    uint32_t* fat_entries = (uint32_t*)fat_buffer;
    fat_entries[0] = 0x0FFFFFF8;  /* Media type + EOC */
    fat_entries[1] = 0x0FFFFFFF;  /* EOC for cluster 1 */
    fat_entries[2] = 0x0FFFFFFF;  /* EOC for root directory */
    
    uint32_t first_fat_sector = reserved_sectors;
    if (hdd_write_sector(drive, first_fat_sector, fat_buffer) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Clear rest of first FAT */
    memset(fat_buffer, 0, FAT32_SECTOR_SIZE);
    for (uint32_t i = 1; i < sectors_per_fat; i++) {
        if (hdd_write_sector(drive, first_fat_sector + i, fat_buffer) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    /* Copy first FAT to second FAT */
    uint32_t second_fat_sector = first_fat_sector + sectors_per_fat;
    for (uint32_t i = 0; i < sectors_per_fat; i++) {
        uint8_t sector_buffer[FAT32_SECTOR_SIZE];
        if (hdd_read_sector(drive, first_fat_sector + i, sector_buffer) != HDD_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        if (hdd_write_sector(drive, second_fat_sector + i, sector_buffer) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    /* Create empty root directory */
    uint8_t cluster_buffer[sectors_per_cluster * FAT32_SECTOR_SIZE];
    memset(cluster_buffer, 0, sectors_per_cluster * FAT32_SECTOR_SIZE);
    
    uint32_t root_dir_sector = first_fat_sector + (num_fats * sectors_per_fat);
    for (uint8_t i = 0; i < sectors_per_cluster; i++) {
        if (hdd_write_sector(drive, root_dir_sector + i, 
                           cluster_buffer + (i * FAT32_SECTOR_SIZE)) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    fat32_debug("FAT32 format completed successfully");
    return FAT32_SUCCESS;
}

/* Open a file by path */
fat32_result_t fat32_open_file(const char* path, fat32_file_t* file) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL || file == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12]; /* Max 16 path components, 11 chars each for 8.3 + null */
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    /* Start from root directory */
    uint32_t current_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    /* Navigate through each path component */
    for (int i = 0; i < component_count; i++) {
        result = fat32_find_entry_in_directory(current_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        /* For all but the last component, it must be a directory */
        if (i < component_count - 1) {
            if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
                return FAT32_ERROR_NOT_DIRECTORY;
            }
        }
        
        current_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* The final entry should be a file (not a directory) */
    if (entry.attributes & FAT32_ATTR_DIRECTORY) {
        return FAT32_ERROR_IS_DIRECTORY;
    }
    
    /* Initialize the file handle */
    file->is_open = true;
    file->is_directory = false;
    file->first_cluster = fat32_get_first_cluster(&entry);
    file->current_cluster = file->first_cluster;
    file->current_sector = 0;
    file->position = 0;
    file->size = entry.file_size;
    
    /* Convert 8.3 name to normal filename */
    fat32_83_to_filename(&entry, file->name);
    
    return FAT32_SUCCESS;
}

/* Close a file */
fat32_result_t fat32_close_file(fat32_file_t* file) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (file == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (!file->is_open) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear file information */
    file->is_open = false;
    
    return FAT32_SUCCESS;
}

/* Read data from a file */
fat32_result_t fat32_read_file(fat32_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (file == NULL || buffer == NULL || bytes_read == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (!file->is_open) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (file->is_directory) {
        return FAT32_ERROR_IS_DIRECTORY;
    }
    
    *bytes_read = 0;
    
    /* Check if position is at end of file */
    if (file->position >= file->size) {
        return FAT32_ERROR_EOF;
    }
    
    /* Limit read size to remaining file size */
    uint32_t remaining = file->size - file->position;
    if (size > remaining) {
        size = remaining;
    }
    
    uint8_t* dest = (uint8_t*)buffer;
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
    static uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8]; /* Buffer for one cluster */
    
    while (size > 0 && file->current_cluster != FAT32_EOC) {
        /* Read the current cluster */
        if (fat32_read_cluster(file->current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        
        /* Calculate offset within cluster */
        uint32_t cluster_offset = file->position % bytes_per_cluster;
        uint32_t bytes_to_read = bytes_per_cluster - cluster_offset;
        
        if (bytes_to_read > size) {
            bytes_to_read = size;
        }
        
        /* Copy data from cluster buffer to destination */
        memcpy(dest, cluster_buffer + cluster_offset, bytes_to_read);
        
        dest += bytes_to_read;
        file->position += bytes_to_read;
        *bytes_read += bytes_to_read;
        size -= bytes_to_read;
        
        /* Move to next cluster if we've read the entire current cluster */
        if ((file->position % bytes_per_cluster) == 0) {
            file->current_cluster = fat32_get_next_cluster(file->current_cluster);
        }
    }
    
    return FAT32_SUCCESS;
}

/* Write data to a file */
fat32_result_t fat32_write_file(fat32_file_t* file, const void* buffer, uint32_t size, uint32_t* bytes_written) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (file == NULL || buffer == NULL || bytes_written == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (!file->is_open) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (file->is_directory) {
        return FAT32_ERROR_IS_DIRECTORY;
    }
    
    *bytes_written = 0;
    
    if (size == 0) {
        return FAT32_SUCCESS;
    }
    
    const uint8_t* src = (const uint8_t*)buffer;
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
    static uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8]; /* Buffer for one cluster */
    
    /* If file is empty or we're at the end, we may need to allocate clusters */
    if (file->first_cluster == 0 || file->current_cluster == FAT32_EOC) {
        uint32_t new_cluster;
        fat32_result_t result = fat32_allocate_cluster(&new_cluster);
        if (result != FAT32_SUCCESS) {
            return result;
        }
        
        if (file->first_cluster == 0) {
            file->first_cluster = new_cluster;
        } else {
            /* Link to existing chain - need to find the last cluster */
            uint32_t last_cluster = file->first_cluster;
            while (fat32_get_next_cluster(last_cluster) != FAT32_EOC) {
                last_cluster = fat32_get_next_cluster(last_cluster);
            }
            fat32_set_next_cluster(last_cluster, new_cluster);
        }
        
        file->current_cluster = new_cluster;
    }
    
    while (size > 0) {
        /* Calculate offset within cluster */
        uint32_t cluster_offset = file->position % bytes_per_cluster;
        uint32_t bytes_to_write = bytes_per_cluster - cluster_offset;
        
        if (bytes_to_write > size) {
            bytes_to_write = size;
        }
        
        /* If we're not writing a full cluster or starting from middle, read existing data */
        if (cluster_offset != 0 || bytes_to_write < bytes_per_cluster) {
            if (fat32_read_cluster(file->current_cluster, cluster_buffer) != FAT32_SUCCESS) {
                return FAT32_ERROR_READ_FAILED;
            }
        }
        
        /* Copy new data into cluster buffer */
        memcpy(cluster_buffer + cluster_offset, src, bytes_to_write);
        
        /* Write the cluster back */
        if (fat32_write_cluster(file->current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
        
        src += bytes_to_write;
        file->position += bytes_to_write;
        *bytes_written += bytes_to_write;
        size -= bytes_to_write;
        
        /* Update file size if we've extended it */
        if (file->position > file->size) {
            file->size = file->position;
        }
        
        /* Move to next cluster if we've filled the current one */
        if ((file->position % bytes_per_cluster) == 0 && size > 0) {
            uint32_t next_cluster = fat32_get_next_cluster(file->current_cluster);
            
            if (next_cluster >= FAT32_EOC) {
                /* Need to allocate a new cluster */
                uint32_t new_cluster;
                fat32_result_t result = fat32_allocate_cluster(&new_cluster);
                if (result != FAT32_SUCCESS) {
                    return result;
                }
                
                /* Link to current cluster */
                fat32_set_next_cluster(file->current_cluster, new_cluster);
                file->current_cluster = new_cluster;
            } else {
                file->current_cluster = next_cluster;
            }
        }
    }
    
    return FAT32_SUCCESS;
}

/* Seek to a position in a file */
fat32_result_t fat32_seek_file(fat32_file_t* file, uint32_t position) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (file == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (!file->is_open) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (position > file->size) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Reset to beginning of file */
    file->position = 0;
    file->current_cluster = file->first_cluster;
    
    if (position == 0) {
        return FAT32_SUCCESS;
    }
    
    /* Calculate which cluster contains the target position */
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
    uint32_t target_cluster_index = position / bytes_per_cluster;
    
    /* Follow the cluster chain to the target cluster */
    for (uint32_t i = 0; i < target_cluster_index && file->current_cluster != FAT32_EOC; i++) {
        file->current_cluster = fat32_get_next_cluster(file->current_cluster);
    }
    
    if (file->current_cluster == FAT32_EOC) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Set the position */
    file->position = position;
    
    return FAT32_SUCCESS;
}

/* Create a new file */
fat32_result_t fat32_create_file(const char* path, fat32_file_t* file) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL || file == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Navigate to parent directory */
    uint32_t parent_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < component_count - 1; i++) {
        result = fat32_find_entry_in_directory(parent_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        parent_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Check if file already exists */
    result = fat32_find_entry_in_directory(parent_cluster, components[component_count - 1], &entry, NULL, NULL);
    if (result == FAT32_SUCCESS) {
        return FAT32_ERROR_FILE_EXISTS;
    }
    
    /* Find a free directory entry in the parent directory */
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    uint32_t current_cluster = parent_cluster;
    uint32_t entry_cluster = 0;
    uint32_t entry_offset = 0;
    bool found_free = false;
    
    while (current_cluster != FAT32_EOC && !found_free) {
        if (fat32_read_cluster(current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        
        uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
        uint32_t entries_per_cluster = bytes_per_cluster / FAT32_DIR_ENTRY_SIZE;
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + (i * FAT32_DIR_ENTRY_SIZE));
            
            if (dir_entry->name[0] == 0x00 || dir_entry->name[0] == (char)0xE5) {
                /* Found free entry */
                entry_cluster = current_cluster;
                entry_offset = i * FAT32_DIR_ENTRY_SIZE;
                found_free = true;
                break;
            }
        }
        
        if (!found_free) {
            uint32_t next_cluster = fat32_get_next_cluster(current_cluster);
            if (next_cluster >= FAT32_EOC) {
                /* Need to allocate a new cluster for directory */
                uint32_t new_cluster;
                result = fat32_allocate_cluster(&new_cluster);
                if (result != FAT32_SUCCESS) {
                    return result;
                }
                
                fat32_set_next_cluster(current_cluster, new_cluster);
                
                /* Clear the new cluster */
                memset(cluster_buffer, 0, bytes_per_cluster);
                if (fat32_write_cluster(new_cluster, cluster_buffer) != FAT32_SUCCESS) {
                    return FAT32_ERROR_WRITE_FAILED;
                }
                
                entry_cluster = new_cluster;
                entry_offset = 0;
                found_free = true;
            } else {
                current_cluster = next_cluster;
            }
        }
    }
    
    if (!found_free) {
        return FAT32_ERROR_DISK_FULL;
    }
    
    /* Create the directory entry */
    memset(&entry, 0, sizeof(fat32_dir_entry_t));
    memcpy(entry.name, components[component_count - 1], 11);
    entry.attributes = FAT32_ATTR_ARCHIVE;
    entry.file_size = 0;
    fat32_set_first_cluster(&entry, 0); /* File starts with no clusters */
    
    /* Write the entry */
    if (fat32_read_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    memcpy(cluster_buffer + entry_offset, &entry, sizeof(fat32_dir_entry_t));
    
    if (fat32_write_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Initialize the file handle */
    file->is_open = true;
    file->is_directory = false;
    file->first_cluster = 0; /* No clusters allocated yet */
    file->current_cluster = 0;
    file->current_sector = 0;
    file->position = 0;
    file->size = 0;
    
    /* Convert 8.3 name to normal filename */
    fat32_83_to_filename(&entry, file->name);
    
    return FAT32_SUCCESS;
}

/* Delete a file */
fat32_result_t fat32_delete_file(const char* path) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Navigate to parent directory */
    uint32_t parent_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < component_count - 1; i++) {
        result = fat32_find_entry_in_directory(parent_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        parent_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Find the file to delete */
    uint32_t entry_cluster, entry_offset;
    result = fat32_find_entry_in_directory(parent_cluster, components[component_count - 1], 
                                          &entry, &entry_cluster, &entry_offset);
    if (result != FAT32_SUCCESS) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Don't delete directories with this function */
    if (entry.attributes & FAT32_ATTR_DIRECTORY) {
        return FAT32_ERROR_IS_DIRECTORY;
    }
    
    /* Free the file's cluster chain */
    uint32_t first_cluster = fat32_get_first_cluster(&entry);
    if (first_cluster != 0) {
        result = fat32_free_cluster_chain(first_cluster);
        if (result != FAT32_SUCCESS) {
            return result;
        }
    }
    
    /* Mark the directory entry as deleted */
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    if (fat32_read_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + entry_offset);
    dir_entry->name[0] = (char)0xE5; /* Mark as deleted */
    
    if (fat32_write_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    return FAT32_SUCCESS;
}

/* Rename a file */
fat32_result_t fat32_rename_file(const char* old_path, const char* new_path) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (old_path == NULL || new_path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char old_components[16][12], new_components[16][12];
    int old_count, new_count;
    
    /* Parse both paths */
    fat32_result_t result = fat32_parse_path(old_path, old_components, &old_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    result = fat32_parse_path(new_path, new_components, &new_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (old_count == 0 || new_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* For simplicity, only support renaming within the same directory */
    if (old_count != new_count) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    for (int i = 0; i < old_count - 1; i++) {
        if (memcmp(old_components[i], new_components[i], 11) != 0) {
            return FAT32_ERROR_INVALID_PARAMETER;
        }
    }
    
    /* Navigate to parent directory */
    uint32_t parent_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < old_count - 1; i++) {
        result = fat32_find_entry_in_directory(parent_cluster, old_components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        parent_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Check if new name already exists */
    result = fat32_find_entry_in_directory(parent_cluster, new_components[new_count - 1], &entry, NULL, NULL);
    if (result == FAT32_SUCCESS) {
        return FAT32_ERROR_FILE_EXISTS;
    }
    
    /* Find the old file */
    uint32_t entry_cluster, entry_offset;
    result = fat32_find_entry_in_directory(parent_cluster, old_components[old_count - 1], 
                                          &entry, &entry_cluster, &entry_offset);
    if (result != FAT32_SUCCESS) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Update the name */
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    if (fat32_read_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + entry_offset);
    memcpy(dir_entry->name, new_components[new_count - 1], 11);
    
    if (fat32_write_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    return FAT32_SUCCESS;
}

/* Get file size */
fat32_result_t fat32_get_file_size(const char* path, uint32_t* size) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL || size == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Start from root directory */
    uint32_t current_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    /* Navigate through each path component */
    for (int i = 0; i < component_count; i++) {
        result = fat32_find_entry_in_directory(current_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        /* For all but the last component, it must be a directory */
        if (i < component_count - 1) {
            if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
                return FAT32_ERROR_NOT_DIRECTORY;
            }
            current_cluster = fat32_get_first_cluster(&entry);
        }
    }
    
    /* Return the file size */
    *size = entry.file_size;
    
    return FAT32_SUCCESS;
}

/* Open a directory */
fat32_result_t fat32_open_directory(const char* path, fat32_file_t* dir) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (dir == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Special case for root directory */
    if (path == NULL || path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        dir->is_open = true;
        dir->is_directory = true;
        dir->first_cluster = fat32_volume.root_dir_first_cluster;
        dir->current_cluster = fat32_volume.root_dir_first_cluster;
        dir->current_sector = 0;
        dir->position = 0;
        dir->size = 0; /* Directories don't have a fixed size */
        strcpy(dir->name, "/");
        
        return FAT32_SUCCESS;
    }
    
    char components[16][12]; /* Max 16 path components, 11 chars each for 8.3 + null */
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    /* Start from root directory */
    uint32_t current_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    /* Navigate through each path component */
    for (int i = 0; i < component_count; i++) {
        result = fat32_find_entry_in_directory(current_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        /* Each component must be a directory */
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        current_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Initialize the directory handle */
    dir->is_open = true;
    dir->is_directory = true;
    dir->first_cluster = current_cluster;
    dir->current_cluster = current_cluster;
    dir->current_sector = 0;
    dir->position = 0;
    dir->size = 0; /* Directories don't have a fixed size */
    
    /* Set directory name */
    if (component_count > 0) {
        fat32_83_to_filename(&entry, dir->name);
    } else {
        strcpy(dir->name, "/");
    }
    
    return FAT32_SUCCESS;
}

/* Create a new directory */
fat32_result_t fat32_create_directory(const char* path) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Navigate to parent directory */
    uint32_t parent_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < component_count - 1; i++) {
        result = fat32_find_entry_in_directory(parent_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        parent_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Check if directory already exists */
    result = fat32_find_entry_in_directory(parent_cluster, components[component_count - 1], &entry, NULL, NULL);
    if (result == FAT32_SUCCESS) {
        return FAT32_ERROR_FILE_EXISTS;
    }
    
    /* Allocate a cluster for the new directory */
    uint32_t dir_cluster;
    result = fat32_allocate_cluster(&dir_cluster);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    /* Initialize the directory cluster with . and .. entries */
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
    memset(cluster_buffer, 0, bytes_per_cluster);
    
    /* Create . entry (current directory) */
    fat32_dir_entry_t* dot_entry = (fat32_dir_entry_t*)cluster_buffer;
    memcpy(dot_entry->name, ".          ", 11);
    dot_entry->attributes = FAT32_ATTR_DIRECTORY;
    fat32_set_first_cluster(dot_entry, dir_cluster);
    
    /* Create .. entry (parent directory) */
    fat32_dir_entry_t* dotdot_entry = (fat32_dir_entry_t*)(cluster_buffer + FAT32_DIR_ENTRY_SIZE);
    memcpy(dotdot_entry->name, "..         ", 11);
    dotdot_entry->attributes = FAT32_ATTR_DIRECTORY;
    fat32_set_first_cluster(dotdot_entry, parent_cluster);
    
    /* Write the directory cluster */
    if (fat32_write_cluster(dir_cluster, cluster_buffer) != FAT32_SUCCESS) {
        fat32_free_cluster_chain(dir_cluster);
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Find a free entry in parent directory */
    uint32_t current_cluster = parent_cluster;
    uint32_t entry_cluster = 0;
    uint32_t entry_offset = 0;
    bool found_free = false;
    
    while (current_cluster != FAT32_EOC && !found_free) {
        if (fat32_read_cluster(current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            fat32_free_cluster_chain(dir_cluster);
            return FAT32_ERROR_READ_FAILED;
        }
        
        uint32_t entries_per_cluster = bytes_per_cluster / FAT32_DIR_ENTRY_SIZE;
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + (i * FAT32_DIR_ENTRY_SIZE));
            
            if (dir_entry->name[0] == 0x00 || dir_entry->name[0] == (char)0xE5) {
                entry_cluster = current_cluster;
                entry_offset = i * FAT32_DIR_ENTRY_SIZE;
                found_free = true;
                break;
            }
        }
        
        if (!found_free) {
            uint32_t next_cluster = fat32_get_next_cluster(current_cluster);
            if (next_cluster >= FAT32_EOC) {
                /* Allocate new cluster for parent directory */
                uint32_t new_cluster;
                result = fat32_allocate_cluster(&new_cluster);
                if (result != FAT32_SUCCESS) {
                    fat32_free_cluster_chain(dir_cluster);
                    return result;
                }
                
                fat32_set_next_cluster(current_cluster, new_cluster);
                
                /* Clear the new cluster */
                memset(cluster_buffer, 0, bytes_per_cluster);
                if (fat32_write_cluster(new_cluster, cluster_buffer) != FAT32_SUCCESS) {
                    fat32_free_cluster_chain(dir_cluster);
                    return FAT32_ERROR_WRITE_FAILED;
                }
                
                entry_cluster = new_cluster;
                entry_offset = 0;
                found_free = true;
            } else {
                current_cluster = next_cluster;
            }
        }
    }
    
    if (!found_free) {
        fat32_free_cluster_chain(dir_cluster);
        return FAT32_ERROR_DISK_FULL;
    }
    
    /* Create the directory entry */
    memset(&entry, 0, sizeof(fat32_dir_entry_t));
    memcpy(entry.name, components[component_count - 1], 11);
    entry.attributes = FAT32_ATTR_DIRECTORY;
    fat32_set_first_cluster(&entry, dir_cluster);
    
    /* Write the entry */
    if (fat32_read_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        fat32_free_cluster_chain(dir_cluster);
        return FAT32_ERROR_READ_FAILED;
    }
    
    memcpy(cluster_buffer + entry_offset, &entry, sizeof(fat32_dir_entry_t));
    
    if (fat32_write_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        fat32_free_cluster_chain(dir_cluster);
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    return FAT32_SUCCESS;
}

/* Delete a directory */
fat32_result_t fat32_delete_directory(const char* path) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Navigate to parent directory */
    uint32_t parent_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < component_count - 1; i++) {
        result = fat32_find_entry_in_directory(parent_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        parent_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Find the directory to delete */
    uint32_t entry_cluster, entry_offset;
    result = fat32_find_entry_in_directory(parent_cluster, components[component_count - 1], 
                                          &entry, &entry_cluster, &entry_offset);
    if (result != FAT32_SUCCESS) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Make sure it's a directory */
    if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
        return FAT32_ERROR_NOT_DIRECTORY;
    }
    
    /* Check if directory is empty (only . and .. entries) */
    uint32_t dir_cluster = fat32_get_first_cluster(&entry);
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    
    if (fat32_read_cluster(dir_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
    uint32_t entries_per_cluster = bytes_per_cluster / FAT32_DIR_ENTRY_SIZE;
    
    /* Check entries beyond . and .. */
    for (uint32_t i = 2; i < entries_per_cluster; i++) {
        fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + (i * FAT32_DIR_ENTRY_SIZE));
        
        if (dir_entry->name[0] == 0x00) {
            break; /* End of directory */
        }
        
        if (dir_entry->name[0] != (char)0xE5) {
            return FAT32_ERROR_ACCESS_DENIED; /* Directory not empty */
        }
    }
    
    /* Check if there are more clusters in the directory */
    uint32_t next_cluster = fat32_get_next_cluster(dir_cluster);
    if (next_cluster != FAT32_EOC) {
        /* Need to check other clusters too - for simplicity, assume not empty */
        return FAT32_ERROR_ACCESS_DENIED;
    }
    
    /* Free the directory's cluster chain */
    result = fat32_free_cluster_chain(dir_cluster);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    /* Mark the directory entry as deleted */
    if (fat32_read_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + entry_offset);
    dir_entry->name[0] = (char)0xE5; /* Mark as deleted */
    
    if (fat32_write_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    return FAT32_SUCCESS;
}

/* Read a directory entry */
fat32_result_t fat32_read_directory(fat32_file_t* dir, fat32_dir_entry_t* entry, char* long_name) {
    static uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8]; /* Buffer for one cluster */
    static bool buffer_valid = false;
    static uint32_t current_buffered_cluster = 0;
    
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (dir == NULL || entry == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (!dir->is_open || !dir->is_directory) {
        return FAT32_ERROR_NOT_DIRECTORY;
    }
    
    /* Check if we need to load a new cluster */
    if (!buffer_valid || current_buffered_cluster != dir->current_cluster) {
        if (fat32_read_cluster(dir->current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        buffer_valid = true;
        current_buffered_cluster = dir->current_cluster;
    }
    
    /* Calculate entry position within cluster */
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
    uint32_t entries_per_cluster = bytes_per_cluster / FAT32_DIR_ENTRY_SIZE;
    uint32_t entry_in_cluster = (dir->position / FAT32_DIR_ENTRY_SIZE) % entries_per_cluster;
    
    /* Check if we need to move to next cluster */
    if (entry_in_cluster >= entries_per_cluster) {
        uint32_t next_cluster = fat32_get_next_cluster(dir->current_cluster);
        if (next_cluster >= FAT32_EOC) {
            return FAT32_ERROR_EOF;
        }
        
        dir->current_cluster = next_cluster;
        dir->current_sector = 0;
        
        /* Load new cluster */
        if (fat32_read_cluster(dir->current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        current_buffered_cluster = dir->current_cluster;
        entry_in_cluster = 0;
    }
    
    /* Get the directory entry */
    fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + (entry_in_cluster * FAT32_DIR_ENTRY_SIZE));
    
    /* Check for end of directory */
    if (dir_entry->name[0] == 0x00) {
        return FAT32_ERROR_EOF;
    }
    
    /* Skip deleted entries */
    if (dir_entry->name[0] == (char)0xE5) {
        dir->position += FAT32_DIR_ENTRY_SIZE;
        return fat32_read_directory(dir, entry, long_name); /* Recursively read next entry */
    }
    
    /* Skip long filename entries for now */
    if ((dir_entry->attributes & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
        dir->position += FAT32_DIR_ENTRY_SIZE;
        return fat32_read_directory(dir, entry, long_name); /* Recursively read next entry */
    }
    
    /* Copy the entry */
    memcpy(entry, dir_entry, sizeof(fat32_dir_entry_t));
    
    /* Convert 8.3 filename to regular filename */
    if (long_name != NULL) {
        fat32_83_to_filename(entry, long_name);
    }
    
    /* Update position */
    dir->position += FAT32_DIR_ENTRY_SIZE;
    
    return FAT32_SUCCESS;
}

/* Get current directory path */
fat32_result_t fat32_get_current_directory(char* path, uint32_t size) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (size < 2) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* If we're at the root directory, return "/" */
    if (fat32_current_directory == fat32_volume.root_dir_first_cluster) {
        path[0] = '/';
        path[1] = '\0';
        return FAT32_SUCCESS;
    }
    
    /* For non-root directories, we would need to traverse up the directory tree
     * to construct the full path. This is a complex operation that requires
     * following ".." entries up to the root. For now, we'll return a simple
     * indication that we're in a subdirectory. */
    
    if (size < 12) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Return a placeholder showing we're in a subdirectory */
    strcpy(path, "/subdir");
    
    return FAT32_SUCCESS;
}

/* Change current directory */
fat32_result_t fat32_change_directory(const char* path) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    fat32_result_t result;
    fat32_dir_entry_t entry;
    uint32_t target_cluster;
    
    /* Handle special cases */
    if (strcmp(path, "/") == 0 || strcmp(path, "\\") == 0) {
        /* Change to root directory */
        fat32_current_directory = fat32_volume.root_dir_first_cluster;
        return FAT32_SUCCESS;
    }
    
    if (strcmp(path, ".") == 0) {
        /* Stay in current directory */
        return FAT32_SUCCESS;
    }
    
    if (strcmp(path, "..") == 0) {
        /* Go to parent directory */
        if (fat32_current_directory == fat32_volume.root_dir_first_cluster) {
            /* Already at root, can't go up */
            return FAT32_SUCCESS;
        }
        
        /* Find the ".." entry in current directory */
        result = fat32_find_entry_in_directory(fat32_current_directory, "..", &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return result;
        }
        
        fat32_current_directory = fat32_get_first_cluster(&entry);
        return FAT32_SUCCESS;
    }
    
    /* Parse the path and traverse to target directory */
    char components[32][12]; /* Support up to 32 path components */
    int component_count = 0;
    
    result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    /* Start from root or current directory */
    if (path[0] == '/' || path[0] == '\\') {
        target_cluster = fat32_volume.root_dir_first_cluster;
    } else {
        target_cluster = fat32_current_directory;
    }
    
    /* Traverse each component */
    for (int i = 0; i < component_count; i++) {
        result = fat32_find_entry_in_directory(target_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        /* Check if it's a directory */
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        target_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Update current directory */
    fat32_current_directory = target_cluster;
    
    return FAT32_SUCCESS;
}

/* Get free space on the volume */
fat32_result_t fat32_get_free_space(uint32_t* free_bytes) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (free_bytes == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    *free_bytes = fat32_volume.free_clusters * 
                 fat32_volume.sectors_per_cluster * 
                 fat32_volume.bytes_per_sector;
    
    return FAT32_SUCCESS;
}

/* Get total space on the volume */
fat32_result_t fat32_get_total_space(uint32_t* total_bytes) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (total_bytes == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    *total_bytes = fat32_volume.total_clusters * 
                  fat32_volume.sectors_per_cluster * 
                  fat32_volume.bytes_per_sector;
    
    return FAT32_SUCCESS;
}

/* Get file attributes */
fat32_result_t fat32_get_file_attributes(const char* path, uint8_t* attributes) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL || attributes == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Start from root directory */
    uint32_t current_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    /* Navigate through each path component */
    for (int i = 0; i < component_count; i++) {
        result = fat32_find_entry_in_directory(current_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        /* For all but the last component, it must be a directory */
        if (i < component_count - 1) {
            if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
                return FAT32_ERROR_NOT_DIRECTORY;
            }
            current_cluster = fat32_get_first_cluster(&entry);
        }
    }
    
    /* Return the attributes */
    *attributes = entry.attributes;
    
    return FAT32_SUCCESS;
}

/* Set file attributes */
fat32_result_t fat32_set_file_attributes(const char* path, uint8_t attributes) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (path == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    char components[16][12];
    int component_count;
    
    /* Parse the path */
    fat32_result_t result = fat32_parse_path(path, components, &component_count);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    if (component_count == 0) {
        return FAT32_ERROR_INVALID_PATH;
    }
    
    /* Navigate to parent directory */
    uint32_t parent_cluster = fat32_volume.root_dir_first_cluster;
    fat32_dir_entry_t entry;
    
    for (int i = 0; i < component_count - 1; i++) {
        result = fat32_find_entry_in_directory(parent_cluster, components[i], &entry, NULL, NULL);
        if (result != FAT32_SUCCESS) {
            return FAT32_ERROR_NOT_FOUND;
        }
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return FAT32_ERROR_NOT_DIRECTORY;
        }
        
        parent_cluster = fat32_get_first_cluster(&entry);
    }
    
    /* Find the file */
    uint32_t entry_cluster, entry_offset;
    result = fat32_find_entry_in_directory(parent_cluster, components[component_count - 1], 
                                          &entry, &entry_cluster, &entry_offset);
    if (result != FAT32_SUCCESS) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Update the attributes */
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    if (fat32_read_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + entry_offset);
    dir_entry->attributes = attributes;
    
    if (fat32_write_cluster(entry_cluster, cluster_buffer) != FAT32_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    return FAT32_SUCCESS;
}

/* Helper function to read cluster data */
fat32_result_t fat32_read_cluster(uint32_t cluster, uint8_t* buffer) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (cluster < 2 || cluster >= fat32_volume.total_clusters + 2) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    uint32_t lba = fat32_cluster_to_lba(cluster);
    if (lba == 0) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    /* Read all sectors in the cluster */
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_read_sector(fat32_volume.drive, lba + i, 
                           buffer + (i * fat32_volume.bytes_per_sector)) != HDD_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
    }
    
    return FAT32_SUCCESS;
}

/* Helper function to write cluster data */
fat32_result_t fat32_write_cluster(uint32_t cluster, const uint8_t* buffer) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (cluster < 2 || cluster >= fat32_volume.total_clusters + 2) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    uint32_t lba = fat32_cluster_to_lba(cluster);
    if (lba == 0) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    /* Write all sectors in the cluster */
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_write_sector(fat32_volume.drive, lba + i, 
                            buffer + (i * fat32_volume.bytes_per_sector)) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    return FAT32_SUCCESS;
}

/* Get the first cluster from directory entry */
uint32_t fat32_get_first_cluster(const fat32_dir_entry_t* entry) {
    return ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
}

/* Set the first cluster in directory entry */
void fat32_set_first_cluster(fat32_dir_entry_t* entry, uint32_t cluster) {
    entry->first_cluster_high = (uint16_t)(cluster >> 16);
    entry->first_cluster_low = (uint16_t)(cluster & 0xFFFF);
}

/* Convert result code to string */
const char* fat32_result_to_string(fat32_result_t result) {
    switch (result) {
        case FAT32_SUCCESS:
            return "Success";
        case FAT32_ERROR_NOT_INITIALIZED:
            return "File system not initialized";
        case FAT32_ERROR_READ_FAILED:
            return "Read operation failed";
        case FAT32_ERROR_WRITE_FAILED:
            return "Write operation failed";
        case FAT32_ERROR_NOT_FOUND:
            return "File or directory not found";
        case FAT32_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case FAT32_ERROR_EOF:
            return "End of file";
        case FAT32_ERROR_INVALID_CLUSTER:
            return "Invalid cluster number";
        case FAT32_ERROR_NO_FREE_CLUSTER:
            return "No free clusters available";
        case FAT32_ERROR_ACCESS_DENIED:
            return "Access denied";
        case FAT32_ERROR_FILE_EXISTS:
            return "File already exists";
        case FAT32_ERROR_NOT_DIRECTORY:
            return "Not a directory";
        case FAT32_ERROR_IS_DIRECTORY:
            return "Is a directory";
        case FAT32_ERROR_DISK_FULL:
            return "Disk is full";
        case FAT32_ERROR_INVALID_PATH:
            return "Invalid path";
        case FAT32_ERROR_ALREADY_OPEN:
            return "File is already open";
        default:
            return "Unknown error";
    }
}

/* Parse path components - helper function */
static fat32_result_t fat32_parse_path(const char* path, char components[][12], int* component_count) {
    if (path == NULL || components == NULL || component_count == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    *component_count = 0;
    
    /* Skip leading slashes */
    while (*path == '/') path++;
    
    /* Parse each component */
    while (*path && *component_count < 16) { /* Limit depth to 16 */
        char component[256];
        int comp_len = 0;
        
        /* Extract component until '/' or end of string */
        while (*path && *path != '/' && comp_len < 255) {
            component[comp_len++] = *path++;
        }
        component[comp_len] = '\0';
        
        if (comp_len > 0) {
            /* Convert to 8.3 format */
            fat32_filename_to_83(component, components[*component_count]);
            (*component_count)++;
        }
        
        /* Skip trailing slashes */
        while (*path == '/') path++;
    }
    
    return FAT32_SUCCESS;
}

/* Find a directory entry by name in a directory */
static fat32_result_t fat32_find_entry_in_directory(uint32_t dir_cluster, const char* name_83, 
                                                   fat32_dir_entry_t* entry, uint32_t* entry_cluster, 
                                                   uint32_t* entry_offset) {
    uint8_t cluster_buffer[FAT32_SECTOR_SIZE * 8];
    uint32_t current_cluster = dir_cluster;
    uint32_t position = 0;
    
    while (current_cluster != FAT32_EOC) {
        /* Read cluster */
        if (fat32_read_cluster(current_cluster, cluster_buffer) != FAT32_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        
        uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * fat32_volume.bytes_per_sector;
        uint32_t entries_per_cluster = bytes_per_cluster / FAT32_DIR_ENTRY_SIZE;
        
        /* Search entries in this cluster */
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
            fat32_dir_entry_t* dir_entry = (fat32_dir_entry_t*)(cluster_buffer + (i * FAT32_DIR_ENTRY_SIZE));
            
            /* End of directory */
            if (dir_entry->name[0] == 0x00) {
                return FAT32_ERROR_NOT_FOUND;
            }
            
            /* Skip deleted entries and long filename entries */
            if (dir_entry->name[0] == (char)0xE5 || 
                (dir_entry->attributes & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            /* Compare names */
            char entry_name[12];
            memcpy(entry_name, dir_entry->name, 8);
            memcpy(entry_name + 8, dir_entry->ext, 3);
            entry_name[11] = '\0';
            
            if (memcmp(entry_name, name_83, 11) == 0) {
                /* Found it! */
                memcpy(entry, dir_entry, sizeof(fat32_dir_entry_t));
                if (entry_cluster) *entry_cluster = current_cluster;
                if (entry_offset) *entry_offset = i * FAT32_DIR_ENTRY_SIZE;
                return FAT32_SUCCESS;
            }
        }
        
        /* Move to next cluster */
        current_cluster = fat32_get_next_cluster(current_cluster);
        position += bytes_per_cluster;
    }
    
    return FAT32_ERROR_NOT_FOUND;
}
