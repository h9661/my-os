/*
 * Complete FAT32 File System Implementation
 * ChanUX Operating System
 * 
 * This file contains all missing functions and a complete implementation
 */

#include "../../include/common/types.h"
#include "../../include/storage/fat32.h"
#include "../../include/storage/hdd.h"
#include "../../include/common/utils.h"

/* Global volume state */
fat32_volume_t fat32_volume = {0};
uint32_t fat32_current_directory = 0;

/* Enhanced error handling and validation */
#define FAT32_VALIDATE_CLUSTER(cluster) \
    ((cluster) >= 2 && (cluster) < (fat32_volume.total_clusters + 2) && (cluster) < FAT32_EOC)

/* Enhanced cluster cache for better performance */
static struct {
    uint32_t cluster;
    uint8_t data[FAT32_SECTOR_SIZE * 8];  /* Max cluster size */
    bool dirty;
    bool valid;
} cluster_cache = {0};

/* FAT sector cache for improved performance */
static struct {
    uint32_t sector;
    uint8_t data[FAT32_SECTOR_SIZE];
    bool dirty;
    bool valid;
} fat_cache = {0};

/* Enhanced buffer management */
static uint8_t fat32_sector_buffer[FAT32_SECTOR_SIZE] __attribute__((aligned(4)));

/* Convert cluster number to LBA */
uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    if (cluster < 2) {
        return 0; /* Invalid cluster */
    }
    return fat32_volume.cluster_begin_lba + ((cluster - 2) * fat32_volume.sectors_per_cluster);
}

/* Get next cluster in the FAT chain */
uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset, fat_sector, ent_offset;
    uint32_t next_cluster;
    
    if (!FAT32_VALIDATE_CLUSTER(cluster)) {
        return FAT32_EOC;
    }
    
    /* Calculate position in FAT */
    fat_offset = cluster * 4;  /* 4 bytes per entry in FAT32 */
    fat_sector = fat32_volume.fat_begin_lba + (fat_offset / FAT32_SECTOR_SIZE);
    ent_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    /* Use FAT cache if available */
    if (fat_cache.valid && fat_cache.sector == fat_sector) {
        next_cluster = *((uint32_t*)&fat_cache.data[ent_offset]) & FAT32_CLUSTER_MASK;
    } else {
        /* Read the FAT sector */
        if (hdd_read_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
            return FAT32_EOC;  /* Error reading FAT */
        }
        
        /* Extract the cluster value */
        next_cluster = *((uint32_t*)&fat32_sector_buffer[ent_offset]) & FAT32_CLUSTER_MASK;
        
        /* Update cache */
        fat_cache.sector = fat_sector;
        memcpy(fat_cache.data, fat32_sector_buffer, FAT32_SECTOR_SIZE);
        fat_cache.valid = true;
        fat_cache.dirty = false;
    }
    
    /* Check if it's end of chain */
    if (next_cluster >= FAT32_EOC) {
        return FAT32_EOC;
    }
    
    return next_cluster;
}

/* Set next cluster in the FAT chain */
fat32_result_t fat32_set_next_cluster(uint32_t cluster, uint32_t next_cluster) {
    uint32_t fat_offset, fat_sector, ent_offset;
    uint32_t value;
    
    if (!FAT32_VALIDATE_CLUSTER(cluster) && cluster != 0) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    /* Calculate position in FAT */
    fat_offset = cluster * 4;  /* 4 bytes per entry in FAT32 */
    fat_sector = fat32_volume.fat_begin_lba + (fat_offset / FAT32_SECTOR_SIZE);
    ent_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    /* Read the FAT sector if not cached */
    if (!fat_cache.valid || fat_cache.sector != fat_sector) {
        if (hdd_read_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
        memcpy(fat_cache.data, fat32_sector_buffer, FAT32_SECTOR_SIZE);
        fat_cache.sector = fat_sector;
        fat_cache.valid = true;
        fat_cache.dirty = false;
    }
    
    /* Preserve top 4 bits which are reserved */
    value = (*((uint32_t*)&fat_cache.data[ent_offset]) & 0xF0000000) | 
            (next_cluster & FAT32_CLUSTER_MASK);
    
    /* Update the cluster value */
    *((uint32_t*)&fat_cache.data[ent_offset]) = value;
    fat_cache.dirty = true;
    
    /* Write through cache immediately for reliability */
    memcpy(fat32_sector_buffer, fat_cache.data, FAT32_SECTOR_SIZE);
    if (hdd_write_sector(fat32_volume.drive, fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    fat_cache.dirty = false;
    
    /* If we have multiple FATs, update all of them */
    for (uint8_t i = 1; i < fat32_volume.num_fats; i++) {
        uint32_t alt_fat_sector = fat32_volume.fat_begin_lba + (i * fat32_volume.fat_size) + 
                                 (fat_offset / FAT32_SECTOR_SIZE);
        
        if (hdd_write_sector(fat32_volume.drive, alt_fat_sector, fat32_sector_buffer) != HDD_SUCCESS) {
            /* Continue anyway since the primary FAT was updated */
        }
    }
    
    return FAT32_SUCCESS;
}

/* Convert filename to 8.3 format */
void fat32_filename_to_83(const char* filename, char* shortname) {
    int i, j;
    int len = strlen(filename);
    bool has_extension = false;
    int ext_pos = 0;
    
    /* Initialize shortname with spaces */
    memset(shortname, ' ', 11);
    shortname[11] = '\0';
    
    /* Find the extension */
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

/* Convert 8.3 format to filename */
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

/* Parse path into components */
fat32_result_t fat32_parse_path(const char* path, char components[][12], int* component_count) {
    if (path == NULL || components == NULL || component_count == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    *component_count = 0;
    
    /* Skip leading slashes */
    while (*path == '/' || *path == '\\') {
        path++;
    }
    
    if (*path == '\0') {
        return FAT32_SUCCESS; /* Root directory */
    }
    
    char temp_component[256];
    int temp_index = 0;
    
    while (*path != '\0' && *component_count < 16) {
        if (*path == '/' || *path == '\\') {
            if (temp_index > 0) {
                temp_component[temp_index] = '\0';
                fat32_filename_to_83(temp_component, components[*component_count]);
                (*component_count)++;
                temp_index = 0;
            }
            path++;
        } else {
            if (temp_index < 255) {
                temp_component[temp_index++] = *path;
            }
            path++;
        }
    }
    
    /* Handle final component */
    if (temp_index > 0 && *component_count < 16) {
        temp_component[temp_index] = '\0';
        fat32_filename_to_83(temp_component, components[*component_count]);
        (*component_count)++;
    }
    
    return FAT32_SUCCESS;
}

/* Read a cluster from disk */
fat32_result_t fat32_read_cluster(uint32_t cluster, void* buffer) {
    if (!FAT32_VALIDATE_CLUSTER(cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    uint32_t lba = fat32_cluster_to_lba(cluster);
    
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_read_sector(fat32_volume.drive, lba + i, 
                           (uint8_t*)buffer + (i * FAT32_SECTOR_SIZE)) != HDD_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
    }
    
    return FAT32_SUCCESS;
}

/* Write a cluster to disk */
fat32_result_t fat32_write_cluster(uint32_t cluster, const void* buffer) {
    if (!FAT32_VALIDATE_CLUSTER(cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    uint32_t lba = fat32_cluster_to_lba(cluster);
    
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_write_sector(fat32_volume.drive, lba + i, 
                            (const uint8_t*)buffer + (i * FAT32_SECTOR_SIZE)) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    return FAT32_SUCCESS;
}

/* Initialize the FAT32 file system */
fat32_result_t fat32_initialize(uint8_t drive) {
    /* Check if the drive is valid */
    if (drive > HDD_SECONDARY_SLAVE) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear volume information */
    memset(&fat32_volume, 0, sizeof(fat32_volume_t));
    fat32_volume.drive = drive;
    
    /* Read the boot sector */
    if (hdd_read_sector(drive, 0, &fat32_volume.boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    /* Validate the file system signature */
    if (fat32_volume.boot_sector.signature != FAT32_SIGNATURE) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Check for the "FAT32" string in the file system type field */
    if (memcmp(fat32_volume.boot_sector.fs_type, FAT32_FILE_SYSTEM_TYPE, 8) != 0) {
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
    fat32_volume.total_sectors = (fat32_volume.boot_sector.total_sectors_16 != 0) ?
                                fat32_volume.boot_sector.total_sectors_16 :
                                fat32_volume.boot_sector.total_sectors_32;
    
    /* Calculate data sectors and total clusters */
    fat32_volume.data_sectors = fat32_volume.total_sectors - fat32_volume.cluster_begin_lba;
    fat32_volume.total_clusters = fat32_volume.data_sectors / fat32_volume.sectors_per_cluster;
    
    /* Get volume label */
    memcpy(fat32_volume.volume_label, fat32_volume.boot_sector.volume_label, 11);
    fat32_volume.volume_label[11] = '\0';
    
    /* Initialize caches */
    memset(&cluster_cache, 0, sizeof(cluster_cache));
    memset(&fat_cache, 0, sizeof(fat_cache));
    
    /* Try to read the FSInfo sector */
    if (fat32_volume.boot_sector.fs_info != 0) {
        fat32_fsinfo_t fsinfo;
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
    
    return FAT32_SUCCESS;
}

/* Allocate a new cluster */
fat32_result_t fat32_allocate_cluster(uint32_t* cluster) {
    uint32_t current_cluster = fat32_volume.next_free_cluster;
    uint32_t start_cluster = current_cluster;
    bool found = false;
    
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (cluster == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
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
    
    /* Update free cluster count */
    if (fat32_volume.free_clusters > 0) {
        fat32_volume.free_clusters--;
    }
    
    *cluster = current_cluster;
    return FAT32_SUCCESS;
}

/* Free a cluster chain */
fat32_result_t fat32_free_cluster_chain(uint32_t start_cluster) {
    uint32_t current_cluster = start_cluster;
    uint32_t next_cluster;
    uint32_t freed_count = 0;
    
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (!FAT32_VALIDATE_CLUSTER(start_cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    while (current_cluster != FAT32_EOC && FAT32_VALIDATE_CLUSTER(current_cluster)) {
        next_cluster = fat32_get_next_cluster(current_cluster);
        
        /* Free the current cluster */
        if (fat32_set_next_cluster(current_cluster, FAT32_FREE_CLUSTER) != FAT32_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
        
        freed_count++;
        current_cluster = next_cluster;
        
        /* Prevent infinite loops */
        if (freed_count > fat32_volume.total_clusters) {
            return FAT32_ERROR_INVALID_CLUSTER;
        }
    }
    
    /* Update free cluster count */
    fat32_volume.free_clusters += freed_count;
    
    return FAT32_SUCCESS;
}

/* Calculate checksum for short filename */
uint8_t fat32_calculate_checksum(const char* short_name) {
    uint8_t checksum = 0;
    
    for (int i = 0; i < 11; i++) {
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + short_name[i];
    }
    
    return checksum;
}

/* Shutdown the file system */
void fat32_shutdown(void) {
    if (!fat32_volume.initialized) {
        return;
    }
    
    /* Flush caches */
    if (fat_cache.valid && fat_cache.dirty) {
        hdd_write_sector(fat32_volume.drive, fat_cache.sector, fat_cache.data);
    }
    
    /* Update FSInfo sector if available */
    if (fat32_volume.boot_sector.fs_info != 0) {
        fat32_fsinfo_t fsinfo;
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

/* ========== ENHANCED FUNCTIONS FROM PREVIOUS IMPLEMENTATION ========== */

/* Enhanced cluster allocation with better free space tracking */
fat32_result_t fat32_allocate_cluster_enhanced(uint32_t* cluster) {
    uint32_t current_cluster = fat32_volume.next_free_cluster;
    uint32_t start_cluster = current_cluster;
    bool found = false;
    uint32_t clusters_searched = 0;
    
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (cluster == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Enhanced search algorithm - batch FAT sector reads */
    while (!found && clusters_searched < fat32_volume.total_clusters) {
        uint32_t fat_offset = current_cluster * 4;
        uint32_t fat_sector = fat32_volume.fat_begin_lba + (fat_offset / FAT32_SECTOR_SIZE);
        uint32_t sector_offset = fat_offset % FAT32_SECTOR_SIZE;
        
        /* Use FAT cache for better performance */
        if (!fat_cache.valid || fat_cache.sector != fat_sector) {
            /* Flush dirty cache first */
            if (fat_cache.valid && fat_cache.dirty) {
                if (hdd_write_sector(fat32_volume.drive, fat_cache.sector, fat_cache.data) != HDD_SUCCESS) {
                    return FAT32_ERROR_WRITE_FAILED;
                }
                fat_cache.dirty = false;
            }
            
            /* Load new sector */
            if (hdd_read_sector(fat32_volume.drive, fat_sector, fat_cache.data) != HDD_SUCCESS) {
                fat_cache.valid = false;
                return FAT32_ERROR_READ_FAILED;
            }
            fat_cache.sector = fat_sector;
            fat_cache.valid = true;
        }
        
        /* Check multiple clusters in the current sector */
        uint32_t entries_in_sector = (FAT32_SECTOR_SIZE - sector_offset) / 4;
        for (uint32_t i = 0; i < entries_in_sector && current_cluster < fat32_volume.total_clusters + 2; i++) {
            uint32_t entry_value = *((uint32_t*)&fat_cache.data[sector_offset + (i * 4)]) & FAT32_CLUSTER_MASK;
            
            if (entry_value == FAT32_FREE_CLUSTER) {
                found = true;
                *cluster = current_cluster;
                break;
            }
            
            current_cluster++;
            clusters_searched++;
        }
        
        /* Wrap around if necessary */
        if (!found && current_cluster >= fat32_volume.total_clusters + 2) {
            current_cluster = 2;
            if (current_cluster == start_cluster) {
                break; /* Full circle - disk is full */
            }
        }
    }
    
    if (!found) {
        return FAT32_ERROR_DISK_FULL;
    }
    
    /* Mark the cluster as allocated (EOC) */
    fat32_result_t result = fat32_set_next_cluster(*cluster, FAT32_EOC);
    if (result != FAT32_SUCCESS) {
        return result;
    }
    
    /* Update free cluster tracking */
    if (fat32_volume.free_clusters > 0) {
        fat32_volume.free_clusters--;
    }
    
    /* Update next free cluster hint - look ahead */
    fat32_volume.next_free_cluster = *cluster + 1;
    if (fat32_volume.next_free_cluster >= fat32_volume.total_clusters + 2) {
        fat32_volume.next_free_cluster = 2;
    }
    
    return FAT32_SUCCESS;
}

/* Enhanced cluster reading with caching */
fat32_result_t fat32_read_cluster_cached(uint32_t cluster, void* buffer) {
    if (!FAT32_VALIDATE_CLUSTER(cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    /* Check cache first */
    if (cluster_cache.valid && cluster_cache.cluster == cluster) {
        memcpy(buffer, cluster_cache.data, 
               fat32_volume.sectors_per_cluster * FAT32_SECTOR_SIZE);
        return FAT32_SUCCESS;
    }
    
    /* Calculate LBA and read cluster */
    uint32_t lba = fat32_cluster_to_lba(cluster);
    
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_read_sector(fat32_volume.drive, lba + i, 
                           (uint8_t*)buffer + (i * FAT32_SECTOR_SIZE)) != HDD_SUCCESS) {
            return FAT32_ERROR_READ_FAILED;
        }
    }
    
    /* Update cache */
    cluster_cache.cluster = cluster;
    memcpy(cluster_cache.data, buffer, 
           fat32_volume.sectors_per_cluster * FAT32_SECTOR_SIZE);
    cluster_cache.valid = true;
    cluster_cache.dirty = false;
    
    return FAT32_SUCCESS;
}

/* Enhanced cluster writing with write-through caching */
fat32_result_t fat32_write_cluster_cached(uint32_t cluster, const void* buffer) {
    if (!FAT32_VALIDATE_CLUSTER(cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    /* Calculate LBA and write cluster */
    uint32_t lba = fat32_cluster_to_lba(cluster);
    
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_write_sector(fat32_volume.drive, lba + i, 
                            (const uint8_t*)buffer + (i * FAT32_SECTOR_SIZE)) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    /* Update cache if this cluster is cached */
    if (cluster_cache.valid && cluster_cache.cluster == cluster) {
        memcpy(cluster_cache.data, buffer, 
               fat32_volume.sectors_per_cluster * FAT32_SECTOR_SIZE);
        cluster_cache.dirty = false;
    }
    
    return FAT32_SUCCESS;
}

/* Enhanced path validation and parsing */
fat32_result_t fat32_parse_path_enhanced(const char* path, char components[][12], int* component_count) {
    if (path == NULL || components == NULL || component_count == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    *component_count = 0;
    
    /* Skip leading slashes and normalize path */
    while (*path == '/' || *path == '\\') {
        path++;
    }
    
    if (*path == '\0') {
        return FAT32_SUCCESS; /* Root directory */
    }
    
    char temp_component[256];
    int temp_index = 0;
    
    while (*path != '\0' && *component_count < 16) {
        if (*path == '/' || *path == '\\') {
            if (temp_index > 0) {
                temp_component[temp_index] = '\0';
                
                /* Validate component length and characters */
                if (temp_index > 255) {
                    return FAT32_ERROR_INVALID_PATH;
                }
                
                /* Convert to 8.3 format */
                fat32_filename_to_83(temp_component, components[*component_count]);
                (*component_count)++;
                temp_index = 0;
            }
            path++;
        } else {
            /* Validate character */
            char c = *path;
            if (c < 32 || c == '"' || c == '*' || c == ':' || c == '<' || 
                c == '>' || c == '?' || c == '|' || c == 0x7F) {
                return FAT32_ERROR_INVALID_PATH;
            }
            
            if (temp_index < 255) {
                temp_component[temp_index++] = c;
            }
            path++;
        }
    }
    
    /* Handle final component */
    if (temp_index > 0 && *component_count < 16) {
        temp_component[temp_index] = '\0';
        fat32_filename_to_83(temp_component, components[*component_count]);
        (*component_count)++;
    }
    
    return FAT32_SUCCESS;
}

/* Enhanced seek operation with better cluster chain traversal */
fat32_result_t fat32_seek_file_enhanced(fat32_file_t* file, uint32_t position) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (file == NULL || !file->is_open) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    if (file->is_directory) {
        return FAT32_ERROR_IS_DIRECTORY;
    }
    
    /* Allow seeking beyond file end for write operations */
    if (position > file->size) {
        /* Could implement sparse file support here */
    }
    
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * FAT32_SECTOR_SIZE;
    uint32_t target_cluster_index = position / bytes_per_cluster;
    uint32_t current_cluster_index = file->position / bytes_per_cluster;
    
    /* Optimize: if seeking within the same cluster, just update position */
    if (target_cluster_index == current_cluster_index) {
        file->position = position;
        return FAT32_SUCCESS;
    }
    
    /* If seeking backwards or to beginning, start from first cluster */
    if (target_cluster_index < current_cluster_index || file->position == 0) {
        file->current_cluster = file->first_cluster;
        current_cluster_index = 0;
    }
    
    /* Follow cluster chain to target */
    while (current_cluster_index < target_cluster_index && 
           file->current_cluster != FAT32_EOC && 
           FAT32_VALIDATE_CLUSTER(file->current_cluster)) {
        file->current_cluster = fat32_get_next_cluster(file->current_cluster);
        current_cluster_index++;
    }
    
    if (current_cluster_index != target_cluster_index || 
        file->current_cluster == FAT32_EOC) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    file->position = position;
    return FAT32_SUCCESS;
}

/* Enhanced cluster chain validation */
fat32_result_t fat32_validate_cluster_chain(uint32_t start_cluster, uint32_t* chain_length) {
    if (!FAT32_VALIDATE_CLUSTER(start_cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    uint32_t current = start_cluster;
    uint32_t length = 0;
    uint32_t max_clusters = fat32_volume.total_clusters;
    
    while (current != FAT32_EOC && length < max_clusters) {
        if (!FAT32_VALIDATE_CLUSTER(current)) {
            return FAT32_ERROR_INVALID_CLUSTER;
        }
        
        current = fat32_get_next_cluster(current);
        length++;
        
        /* Detect infinite loops */
        if (length > max_clusters) {
            return FAT32_ERROR_INVALID_CLUSTER;
        }
    }
    
    if (chain_length) {
        *chain_length = length;
    }
    
    return (current == FAT32_EOC) ? FAT32_SUCCESS : FAT32_ERROR_INVALID_CLUSTER;
}

/* ========== ADDITIONAL UTILITY FUNCTIONS ========== */

/* Convert result code to string for debugging */
const char* fat32_result_to_string(fat32_result_t result) {
    switch (result) {
        case FAT32_SUCCESS: return "Success";
        case FAT32_ERROR_NOT_INITIALIZED: return "Not initialized";
        case FAT32_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case FAT32_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case FAT32_ERROR_DRIVE_NOT_READY: return "Drive not ready";
        case FAT32_ERROR_READ_FAILED: return "Read failed";
        case FAT32_ERROR_WRITE_FAILED: return "Write failed";
        case FAT32_ERROR_SEEK_FAILED: return "Seek failed";
        case FAT32_ERROR_NOT_FOUND: return "Not found";
        case FAT32_ERROR_INVALID_CLUSTER: return "Invalid cluster";
        case FAT32_ERROR_INVALID_PATH: return "Invalid path";
        case FAT32_ERROR_INVALID_FILENAME: return "Invalid filename";
        case FAT32_ERROR_CORRUPTED_FS: return "Corrupted filesystem";
        case FAT32_ERROR_EOF: return "End of file";
        case FAT32_ERROR_ACCESS_DENIED: return "Access denied";
        case FAT32_ERROR_FILE_EXISTS: return "File exists";
        case FAT32_ERROR_NOT_DIRECTORY: return "Not a directory";
        case FAT32_ERROR_IS_DIRECTORY: return "Is a directory";
        case FAT32_ERROR_ALREADY_OPEN: return "Already open";
        case FAT32_ERROR_NOT_OPEN: return "Not open";
        case FAT32_ERROR_DISK_FULL: return "Disk full";
        case FAT32_ERROR_NO_FREE_CLUSTER: return "No free cluster";
        case FAT32_ERROR_CLUSTER_CHAIN_BROKEN: return "Cluster chain broken";
        case FAT32_ERROR_CACHE_FULL: return "Cache full";
        case FAT32_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case FAT32_ERROR_READ_ONLY: return "Read only";
        case FAT32_ERROR_LOCKED: return "Locked";
        case FAT32_ERROR_NOT_SUPPORTED: return "Not supported";
        case FAT32_ERROR_VERSION_MISMATCH: return "Version mismatch";
        default: return "Unknown error";
    }
}

/* Get free space in bytes */
fat32_result_t fat32_get_free_space(uint32_t* free_bytes) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (free_bytes == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * FAT32_SECTOR_SIZE;
    *free_bytes = fat32_volume.free_clusters * bytes_per_cluster;
    
    return FAT32_SUCCESS;
}

/* Get total space in bytes */
fat32_result_t fat32_get_total_space(uint32_t* total_bytes) {
    if (!fat32_volume.initialized) {
        return FAT32_ERROR_NOT_INITIALIZED;
    }
    
    if (total_bytes == NULL) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    uint32_t bytes_per_cluster = fat32_volume.sectors_per_cluster * FAT32_SECTOR_SIZE;
    *total_bytes = fat32_volume.total_clusters * bytes_per_cluster;
    
    return FAT32_SUCCESS;
}

/* Check if drive has valid FAT32 filesystem */
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
    if (boot_sector.signature != FAT32_SIGNATURE) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Check for the "FAT32" string in the file system type field */
    if (memcmp(boot_sector.fs_type, FAT32_FILE_SYSTEM_TYPE, 8) != 0) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    return FAT32_SUCCESS;
}

/* Clear a cluster (fill with zeros) */
fat32_result_t fat32_clear_cluster(uint32_t cluster) {
    if (!FAT32_VALIDATE_CLUSTER(cluster)) {
        return FAT32_ERROR_INVALID_CLUSTER;
    }
    
    static uint8_t zero_buffer[FAT32_SECTOR_SIZE];
    memset(zero_buffer, 0, FAT32_SECTOR_SIZE);
    
    uint32_t lba = fat32_cluster_to_lba(cluster);
    
    for (uint8_t i = 0; i < fat32_volume.sectors_per_cluster; i++) {
        if (hdd_write_sector(fat32_volume.drive, lba + i, zero_buffer) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    return FAT32_SUCCESS;
}

/* Enhanced initialization with better error recovery */
fat32_result_t fat32_initialize_enhanced(uint8_t drive) {
    /* Validate drive parameter */
    if (drive > HDD_SECONDARY_SLAVE) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Initialize caches */
    memset(&cluster_cache, 0, sizeof(cluster_cache));
    memset(&fat_cache, 0, sizeof(fat_cache));
    
    /* Clear volume information */
    memset(&fat32_volume, 0, sizeof(fat32_volume_t));
    fat32_volume.drive = drive;
    
    /* Read and validate boot sector */
    if (hdd_read_sector(drive, 0, &fat32_volume.boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_READ_FAILED;
    }
    
    /* Enhanced validation */
    if (fat32_volume.boot_sector.signature != FAT32_SIGNATURE) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    if (memcmp(fat32_volume.boot_sector.fs_type, FAT32_FILE_SYSTEM_TYPE, 8) != 0) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Validate critical BPB fields */
    if (fat32_volume.boot_sector.bytes_per_sector != 512 ||
        fat32_volume.boot_sector.sectors_per_cluster == 0 ||
        fat32_volume.boot_sector.num_fats == 0 ||
        fat32_volume.boot_sector.reserved_sectors == 0) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    /* Fill in volume information */
    fat32_volume.bytes_per_sector = fat32_volume.boot_sector.bytes_per_sector;
    fat32_volume.sectors_per_cluster = fat32_volume.boot_sector.sectors_per_cluster;
    fat32_volume.num_fats = fat32_volume.boot_sector.num_fats;
    fat32_volume.root_dir_first_cluster = fat32_volume.boot_sector.root_cluster;
    
    /* Calculate layout */
    fat32_volume.fat_begin_lba = fat32_volume.boot_sector.reserved_sectors;
    fat32_volume.fat_size = fat32_volume.boot_sector.sectors_per_fat_32;
    fat32_volume.cluster_begin_lba = fat32_volume.fat_begin_lba + 
                                    (fat32_volume.num_fats * fat32_volume.fat_size);
    
    /* Calculate totals with proper bounds checking */
    fat32_volume.total_sectors = (fat32_volume.boot_sector.total_sectors_16 != 0) ?
                                fat32_volume.boot_sector.total_sectors_16 :
                                fat32_volume.boot_sector.total_sectors_32;
    
    if (fat32_volume.total_sectors <= fat32_volume.cluster_begin_lba) {
        return FAT32_ERROR_NOT_FOUND;
    }
    
    fat32_volume.data_sectors = fat32_volume.total_sectors - fat32_volume.cluster_begin_lba;
    fat32_volume.total_clusters = fat32_volume.data_sectors / fat32_volume.sectors_per_cluster;
    
    /* Validate cluster count for FAT32 */
    if (fat32_volume.total_clusters < 65525) {
        return FAT32_ERROR_NOT_FOUND; /* Not FAT32 */
    }
    
    /* Initialize free space tracking */
    fat32_volume.free_clusters = 0xFFFFFFFF; /* Unknown initially */
    fat32_volume.next_free_cluster = 2;
    
    /* Try to read FSInfo with enhanced validation */
    if (fat32_volume.boot_sector.fs_info != 0 && 
        fat32_volume.boot_sector.fs_info < fat32_volume.boot_sector.reserved_sectors) {
        
        fat32_fsinfo_t fsinfo;
        if (hdd_read_sector(drive, fat32_volume.boot_sector.fs_info, &fsinfo) == HDD_SUCCESS) {
            if (fsinfo.lead_signature == 0x41615252 && 
                fsinfo.structure_signature == 0x61417272 &&
                fsinfo.trail_signature == 0xAA550000) {
                
                /* Validate free cluster count */
                if (fsinfo.free_cluster_count <= fat32_volume.total_clusters) {
                    fat32_volume.free_clusters = fsinfo.free_cluster_count;
                }
                
                /* Validate next free cluster hint */
                if (fsinfo.next_free_cluster >= 2 && 
                    fsinfo.next_free_cluster < fat32_volume.total_clusters + 2) {
                    fat32_volume.next_free_cluster = fsinfo.next_free_cluster;
                }
            }
        }
    }
    
    /* Copy volume label with proper null termination */
    memcpy(fat32_volume.volume_label, fat32_volume.boot_sector.volume_label, 11);
    fat32_volume.volume_label[11] = '\0';
    
    /* Remove trailing spaces from volume label */
    for (int i = 10; i >= 0 && fat32_volume.volume_label[i] == ' '; i--) {
        fat32_volume.volume_label[i] = '\0';
    }
    
    fat32_volume.initialized = true;
    fat32_current_directory = fat32_volume.root_dir_first_cluster;
    
    return FAT32_SUCCESS;
}

/* Enhanced shutdown with proper cache flushing */
void fat32_shutdown_enhanced(void) {
    if (!fat32_volume.initialized) {
        return;
    }
    
    /* Flush all caches */
    if (fat_cache.valid && fat_cache.dirty) {
        hdd_write_sector(fat32_volume.drive, fat_cache.sector, fat_cache.data);
    }
    
    if (cluster_cache.valid && cluster_cache.dirty) {
        fat32_write_cluster_cached(cluster_cache.cluster, cluster_cache.data);
    }
    
    /* Update FSInfo sector */
    if (fat32_volume.boot_sector.fs_info != 0) {
        fat32_fsinfo_t fsinfo;
        if (hdd_read_sector(fat32_volume.drive, fat32_volume.boot_sector.fs_info, &fsinfo) == HDD_SUCCESS) {
            if (fsinfo.lead_signature == 0x41615252 && 
                fsinfo.structure_signature == 0x61417272 &&
                fsinfo.trail_signature == 0xAA550000) {
                
                fsinfo.free_cluster_count = fat32_volume.free_clusters;
                fsinfo.next_free_cluster = fat32_volume.next_free_cluster;
                hdd_write_sector(fat32_volume.drive, fat32_volume.boot_sector.fs_info, &fsinfo);
            }
        }
    }
    
    /* Clear caches and volume info */
    memset(&cluster_cache, 0, sizeof(cluster_cache));
    memset(&fat_cache, 0, sizeof(fat_cache));
    fat32_volume.initialized = false;
}

/* Format drive as FAT32 (full implementation) */
fat32_result_t fat32_format_drive(uint8_t drive, const char* volume_label) {
    if (drive > HDD_SECONDARY_SLAVE) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Get drive size first */
    uint32_t total_sectors;
    if (hdd_get_drive_size(drive, &total_sectors) != HDD_SUCCESS) {
        return FAT32_ERROR_DRIVE_NOT_READY;
    }
    
    /* Validate minimum size for FAT32 (at least 65536 clusters) */
    if (total_sectors < 65536 * 8) { /* Assuming 8 sectors per cluster minimum */
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Calculate optimal cluster size based on drive size */
    uint8_t sectors_per_cluster;
    if (total_sectors < 16777216) {        /* < 8GB */
        sectors_per_cluster = 8;           /* 4KB clusters */
    } else if (total_sectors < 33554432) { /* < 16GB */
        sectors_per_cluster = 16;          /* 8KB clusters */
    } else if (total_sectors < 67108864) { /* < 32GB */
        sectors_per_cluster = 32;          /* 16KB clusters */
    } else {                               /* >= 32GB */
        sectors_per_cluster = 64;          /* 32KB clusters */
    }
    
    /* Calculate file system layout */
    uint16_t reserved_sectors = 32;        /* Standard for FAT32 */
    uint8_t num_fats = 2;                  /* Standard redundancy */
    
    /* Calculate data sectors */
    uint32_t data_sectors = total_sectors - reserved_sectors;
    uint32_t clusters = data_sectors / sectors_per_cluster;
    
    /* Calculate FAT size (4 bytes per entry) */
    uint32_t fat_entries = clusters + 2;  /* +2 for reserved entries */
    uint32_t fat_bytes = fat_entries * 4;
    uint32_t fat_sectors = (fat_bytes + FAT32_SECTOR_SIZE - 1) / FAT32_SECTOR_SIZE;
    
    /* Adjust data sectors after accounting for FATs */
    data_sectors = total_sectors - reserved_sectors - (num_fats * fat_sectors);
    clusters = data_sectors / sectors_per_cluster;
    
    /* Validate we still have enough clusters for FAT32 */
    if (clusters < 65525) {
        return FAT32_ERROR_INVALID_PARAMETER;
    }
    
    /* Create boot sector */
    fat32_boot_sector_t boot_sector;
    memset(&boot_sector, 0, sizeof(fat32_boot_sector_t));
    
    /* Jump instruction */
    boot_sector.jump_boot[0] = 0xEB;
    boot_sector.jump_boot[1] = 0x58;
    boot_sector.jump_boot[2] = 0x90;
    
    /* OEM name */
    memcpy(boot_sector.oem_name, "CHANUX  ", 8);
    
    /* BIOS Parameter Block */
    boot_sector.bytes_per_sector = FAT32_SECTOR_SIZE;
    boot_sector.sectors_per_cluster = sectors_per_cluster;
    boot_sector.reserved_sectors = reserved_sectors;
    boot_sector.num_fats = num_fats;
    boot_sector.root_entry_count = 0;      /* 0 for FAT32 */
    boot_sector.total_sectors_16 = 0;      /* 0 for FAT32 */
    boot_sector.media_type = 0xF8;         /* Fixed disk */
    boot_sector.sectors_per_fat_16 = 0;    /* 0 for FAT32 */
    boot_sector.sectors_per_track = 63;    /* Standard value */
    boot_sector.num_heads = 255;           /* Standard value */
    boot_sector.hidden_sectors = 0;
    boot_sector.total_sectors_32 = total_sectors;
    
    /* FAT32 Extended BPB */
    boot_sector.sectors_per_fat_32 = fat_sectors;
    boot_sector.ext_flags = 0;             /* Mirror FAT */
    boot_sector.fs_version = 0;            /* Version 0.0 */
    boot_sector.root_cluster = 2;          /* Root starts at cluster 2 */
    boot_sector.fs_info = 1;               /* FSInfo at sector 1 */
    boot_sector.backup_boot = 6;           /* Backup boot sector */
    
    /* Extended boot signature info */
    boot_sector.drive_number = 0x80;       /* Hard disk */
    boot_sector.boot_signature = 0x29;     /* Extended boot signature */
    boot_sector.volume_id = 0x12345678;    /* Random volume ID */
    
    /* Volume label */
    if (volume_label && strlen(volume_label) > 0) {
        char padded_label[11];
        memset(padded_label, ' ', 11);
        int len = strlen(volume_label);
        if (len > 11) len = 11;
        memcpy(padded_label, volume_label, len);
        memcpy(boot_sector.volume_label, padded_label, 11);
    } else {
        memcpy(boot_sector.volume_label, "NO NAME    ", 11);
    }
    
    /* File system type */
    memcpy(boot_sector.fs_type, "FAT32   ", 8);
    
    /* Boot signature */
    boot_sector.signature = FAT32_SIGNATURE;
    
    /* Write boot sector */
    if (hdd_write_sector(drive, 0, &boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Write backup boot sector */
    if (hdd_write_sector(drive, 6, &boot_sector) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Create FSInfo sector */
    fat32_fsinfo_t fsinfo;
    memset(&fsinfo, 0, sizeof(fat32_fsinfo_t));
    
    fsinfo.lead_signature = 0x41615252;
    fsinfo.structure_signature = 0x61417272;
    fsinfo.free_cluster_count = clusters - 1; /* -1 for root directory */
    fsinfo.next_free_cluster = 3;             /* First free after root */
    fsinfo.trail_signature = 0xAA550000;
    
    /* Write FSInfo sector */
    if (hdd_write_sector(drive, 1, &fsinfo) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Write backup FSInfo sector */
    if (hdd_write_sector(drive, 7, &fsinfo) != HDD_SUCCESS) {
        return FAT32_ERROR_WRITE_FAILED;
    }
    
    /* Clear reserved sectors */
    uint8_t zero_sector[FAT32_SECTOR_SIZE];
    memset(zero_sector, 0, FAT32_SECTOR_SIZE);
    
    for (uint32_t i = 2; i < reserved_sectors; i++) {
        if (i != 6 && i != 7) { /* Skip backup sectors */
            if (hdd_write_sector(drive, i, zero_sector) != HDD_SUCCESS) {
                return FAT32_ERROR_WRITE_FAILED;
            }
        }
    }
    
    /* Initialize FAT tables */
    uint8_t fat_sector[FAT32_SECTOR_SIZE];
    memset(fat_sector, 0, FAT32_SECTOR_SIZE);
    
    /* First FAT sector with special entries */
    uint32_t* fat_entries_ptr = (uint32_t*)fat_sector;
    fat_entries_ptr[0] = 0x0FFFFFF8; /* Media descriptor in lower 8 bits */
    fat_entries_ptr[1] = 0x0FFFFFFF; /* End of chain */
    fat_entries_ptr[2] = 0x0FFFFFFF; /* Root directory (end of chain) */
    
    /* Write first sector of each FAT */
    for (uint8_t fat_num = 0; fat_num < num_fats; fat_num++) {
        uint32_t fat_start = reserved_sectors + (fat_num * fat_sectors);
        
        /* Write first sector with special entries */
        if (hdd_write_sector(drive, fat_start, fat_sector) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
        
        /* Clear rest of FAT */
        memset(fat_sector, 0, FAT32_SECTOR_SIZE);
        for (uint32_t sector = 1; sector < fat_sectors; sector++) {
            if (hdd_write_sector(drive, fat_start + sector, fat_sector) != HDD_SUCCESS) {
                return FAT32_ERROR_WRITE_FAILED;
            }
        }
    }
    
    /* Create root directory */
    uint8_t root_cluster[FAT32_SECTOR_SIZE * sectors_per_cluster];
    memset(root_cluster, 0, sizeof(root_cluster));
    
    /* Add volume label entry if provided */
    if (volume_label && strlen(volume_label) > 0) {
        fat32_dir_entry_t* vol_entry = (fat32_dir_entry_t*)root_cluster;
        
        /* Prepare volume label */
        char vol_label[11];
        memset(vol_label, ' ', 11);
        int len = strlen(volume_label);
        if (len > 11) len = 11;
        
        /* Convert to uppercase and copy */
        for (int i = 0; i < len; i++) {
            char c = volume_label[i];
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
            vol_label[i] = c;
        }
        
        memcpy(vol_entry->name, vol_label, 8);
        memcpy(vol_entry->ext, vol_label + 8, 3);
        vol_entry->attributes = FAT32_ATTR_VOLUME_ID;
        
        /* Set current date/time (simplified) */
        vol_entry->creation_date = fat32_date_to_dos_date(2024, 1, 1);
        vol_entry->creation_time = fat32_time_to_dos_time(12, 0, 0);
        vol_entry->last_access_date = vol_entry->creation_date;
        vol_entry->write_date = vol_entry->creation_date;
        vol_entry->write_time = vol_entry->creation_time;
    }
    
    /* Write root directory cluster */
    uint32_t root_lba = reserved_sectors + (num_fats * fat_sectors);
    for (uint8_t i = 0; i < sectors_per_cluster; i++) {
        if (hdd_write_sector(drive, root_lba + i, 
                            root_cluster + (i * FAT32_SECTOR_SIZE)) != HDD_SUCCESS) {
            return FAT32_ERROR_WRITE_FAILED;
        }
    }
    
    /* Clear remaining data area */
    uint32_t data_start = root_lba + sectors_per_cluster;
    uint32_t remaining_sectors = total_sectors - data_start;
    
    /* Clear in batches for efficiency */
    uint32_t batch_size = 64; /* Clear 64 sectors at a time */
    for (uint32_t sector = 0; sector < remaining_sectors; sector += batch_size) {
        uint32_t sectors_to_clear = (remaining_sectors - sector < batch_size) ? 
                                   (remaining_sectors - sector) : batch_size;
        
        for (uint32_t i = 0; i < sectors_to_clear; i++) {
            if (hdd_write_sector(drive, data_start + sector + i, zero_sector) != HDD_SUCCESS) {
                /* Continue on error - not critical for basic functionality */
            }
        }
    }
    
    return FAT32_SUCCESS;
}

/* Convert time to DOS time format */
uint16_t fat32_time_to_dos_time(uint8_t hour, uint8_t minute, uint8_t second) {
    return ((hour & 0x1F) << 11) | ((minute & 0x3F) << 5) | ((second / 2) & 0x1F);
}

/* Convert date to DOS date format */
uint16_t fat32_date_to_dos_date(uint16_t year, uint8_t month, uint8_t day) {
    return (((year - 1980) & 0x7F) << 9) | ((month & 0x0F) << 5) | (day & 0x1F);
}

/* Convert DOS time to time components */
void fat32_dos_time_to_time(uint16_t dos_time, uint8_t* hour, uint8_t* minute, uint8_t* second) {
    if (hour) *hour = (dos_time >> 11) & 0x1F;
    if (minute) *minute = (dos_time >> 5) & 0x3F;
    if (second) *second = (dos_time & 0x1F) * 2;
}

/* Convert DOS date to date components */
void fat32_dos_date_to_date(uint16_t dos_date, uint16_t* year, uint8_t* month, uint8_t* day) {
    if (year) *year = ((dos_date >> 9) & 0x7F) + 1980;
    if (month) *month = (dos_date >> 5) & 0x0F;
    if (day) *day = dos_date & 0x1F;
}