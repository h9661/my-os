/*
 * Enhanced FAT32 File System Header
 * ChanUX Operating System
 * 
 * Enhanced implementation with improved performance, caching,
 * and better error handling capabilities.
 */

#ifndef FAT32_H
#define FAT32_H

#include "../common/types.h"

/* FAT32 Constants and Definitions */
#define FAT32_SECTOR_SIZE           512
#define FAT32_MAX_FILENAME          255
#define FAT32_SFN_NAME_SIZE         8
#define FAT32_SFN_EXT_SIZE          3
#define FAT32_DIR_ENTRY_SIZE        32
#define FAT32_SIGNATURE             0xAA55
#define FAT32_FILE_SYSTEM_TYPE      "FAT32   "
#define FAT32_MAX_PATH_DEPTH        16
#define FAT32_MAX_CLUSTER_SIZE      (32 * 1024)  /* 32KB max cluster */

/* FAT32 Special Cluster Values */
#define FAT32_FREE_CLUSTER          0x00000000
#define FAT32_BAD_CLUSTER           0x0FFFFFF7
#define FAT32_EOC                   0x0FFFFFF8   /* End of Chain */
#define FAT32_CLUSTER_MASK          0x0FFFFFFF   /* 28-bit cluster number */

/* FAT32 File Attributes */
#define FAT32_ATTR_READ_ONLY        0x01
#define FAT32_ATTR_HIDDEN           0x02
#define FAT32_ATTR_SYSTEM           0x04
#define FAT32_ATTR_VOLUME_ID        0x08
#define FAT32_ATTR_DIRECTORY        0x10
#define FAT32_ATTR_ARCHIVE          0x20
#define FAT32_ATTR_LONG_NAME        0x0F
#define FAT32_ATTR_LONG_NAME_MASK   0x3F

/* Enhanced File Access Modes */
typedef enum {
    FAT32_MODE_READ             = 0x01,
    FAT32_MODE_WRITE            = 0x02,
    FAT32_MODE_APPEND           = 0x04,
    FAT32_MODE_CREATE           = 0x08,
    FAT32_MODE_TRUNCATE         = 0x10,
    FAT32_MODE_EXCLUSIVE        = 0x20
} fat32_file_mode_t;

/* Enhanced File Seek Origins */
typedef enum {
    FAT32_SEEK_SET = 0,     /* From beginning of file */
    FAT32_SEEK_CUR = 1,     /* From current position */
    FAT32_SEEK_END = 2      /* From end of file */
} fat32_seek_origin_t;

/* FAT32 Boot Sector Structure (BIOS Parameter Block) */
typedef struct __attribute__((packed)) {
    uint8_t     jump_boot[3];           /* 0x00: Jump instruction */
    char        oem_name[8];            /* 0x03: OEM Name */
    uint16_t    bytes_per_sector;       /* 0x0B: Bytes per sector */
    uint8_t     sectors_per_cluster;    /* 0x0D: Sectors per cluster */
    uint16_t    reserved_sectors;       /* 0x0E: Reserved sectors */
    uint8_t     num_fats;               /* 0x10: Number of FATs */
    uint16_t    root_entry_count;       /* 0x11: Root entries (0 for FAT32) */
    uint16_t    total_sectors_16;       /* 0x13: Total sectors (0 for FAT32) */
    uint8_t     media_type;             /* 0x15: Media descriptor */
    uint16_t    sectors_per_fat_16;     /* 0x16: Sectors per FAT (0 for FAT32) */
    uint16_t    sectors_per_track;      /* 0x18: Sectors per track */
    uint16_t    num_heads;              /* 0x1A: Number of heads */
    uint32_t    hidden_sectors;         /* 0x1C: Hidden sectors */
    uint32_t    total_sectors_32;       /* 0x20: Total sectors */
    
    /* FAT32 Extended BPB */
    uint32_t    sectors_per_fat_32;     /* 0x24: Sectors per FAT */
    uint16_t    ext_flags;              /* 0x28: Extension flags */
    uint16_t    fs_version;             /* 0x2A: File system version */
    uint32_t    root_cluster;           /* 0x2C: Root directory cluster */
    uint16_t    fs_info;                /* 0x30: FSInfo sector */
    uint16_t    backup_boot;            /* 0x32: Backup boot sector */
    uint8_t     reserved[12];           /* 0x34: Reserved */
    uint8_t     drive_number;           /* 0x40: Drive number */
    uint8_t     reserved1;              /* 0x41: Reserved */
    uint8_t     boot_signature;         /* 0x42: Extended boot signature */
    uint32_t    volume_id;              /* 0x43: Volume ID */
    char        volume_label[11];       /* 0x47: Volume label */
    char        fs_type[8];             /* 0x52: File system type */
    uint8_t     boot_code[420];         /* 0x5A: Boot code */
    uint16_t    signature;              /* 0x1FE: Boot signature (0xAA55) */
} fat32_boot_sector_t;

/* FAT32 FSInfo Sector Structure */
typedef struct __attribute__((packed)) {
    uint32_t    lead_signature;         /* 0x000: Lead signature (0x41615252) */
    uint8_t     reserved1[480];         /* 0x004: Reserved */
    uint32_t    structure_signature;    /* 0x1E4: Structure signature (0x61417272) */
    uint32_t    free_cluster_count;     /* 0x1E8: Free cluster count */
    uint32_t    next_free_cluster;      /* 0x1EC: Next free cluster */
    uint8_t     reserved2[12];          /* 0x1F0: Reserved */
    uint32_t    trail_signature;        /* 0x1FC: Trail signature (0xAA550000) */
} fat32_fsinfo_t;

/* FAT32 Directory Entry Structure */
typedef struct __attribute__((packed)) {
    char        name[8];                /* Short filename */
    char        ext[3];                 /* File extension */
    uint8_t     attributes;             /* File attributes */
    uint8_t     nt_reserved;            /* Reserved for NT */
    uint8_t     creation_time_tenths;   /* Creation time (tenths of second) */
    uint16_t    creation_time;          /* Creation time */
    uint16_t    creation_date;          /* Creation date */
    uint16_t    last_access_date;       /* Last access date */
    uint16_t    first_cluster_hi;       /* High word of first cluster */
    uint16_t    write_time;             /* Last write time */
    uint16_t    write_date;             /* Last write date */
    uint16_t    first_cluster_lo;       /* Low word of first cluster */
    uint32_t    file_size;              /* File size in bytes */
} fat32_dir_entry_t;

/* Enhanced File Handle Structure */
typedef struct {
    bool        is_open;                /* File open status */
    bool        is_directory;           /* Is directory flag */
    bool        is_modified;            /* Modified flag for caching */
    uint8_t     access_mode;            /* Access mode flags */
    uint32_t    first_cluster;          /* First cluster of file */
    uint32_t    current_cluster;        /* Current cluster */
    uint32_t    current_sector;         /* Current sector in cluster */
    uint32_t    position;               /* Current position in file */
    uint32_t    size;                   /* File size */
    uint32_t    allocated_size;         /* Allocated size in bytes */
    char        name[FAT32_MAX_FILENAME + 1];  /* Long filename */
    char        short_name[12];         /* 8.3 short name */
    
    /* Enhanced tracking for directory entry updates */
    uint32_t    dir_entry_cluster;      /* Cluster containing directory entry */
    uint32_t    dir_entry_offset;       /* Offset of entry in cluster */
    
    /* Timestamps */
    uint16_t    creation_date;
    uint16_t    creation_time;
    uint16_t    last_access_date;
    uint16_t    last_write_date;
    uint16_t    last_write_time;
} fat32_file_t;

/* Enhanced Volume Information Structure */
typedef struct {
    uint8_t         drive;                      /* Drive number */
    bool            initialized;                /* Initialization status */
    bool            read_only;                  /* Read-only flag */
    uint32_t        total_sectors;              /* Total sectors */
    uint32_t        fat_begin_lba;              /* First FAT LBA */
    uint32_t        cluster_begin_lba;          /* First cluster LBA */
    uint32_t        root_dir_first_cluster;     /* Root directory cluster */
    uint32_t        data_sectors;               /* Data sectors count */
    uint32_t        total_clusters;             /* Total clusters */
    uint32_t        free_clusters;              /* Free clusters count */
    uint32_t        next_free_cluster;          /* Next free cluster hint */
    uint32_t        bad_clusters;               /* Bad clusters count */
    uint8_t         sectors_per_cluster;        /* Sectors per cluster */
    uint32_t        fat_size;                   /* FAT size in sectors */
    uint8_t         num_fats;                   /* Number of FATs */
    char            volume_label[12];           /* Volume label */
    uint16_t        bytes_per_sector;           /* Bytes per sector */
    uint32_t        volume_id;                  /* Volume ID */
    fat32_boot_sector_t boot_sector;            /* Boot sector copy */
    
    /* Performance counters */
    uint32_t        read_operations;
    uint32_t        write_operations;
    uint32_t        cache_hits;
    uint32_t        cache_misses;
} fat32_volume_t;

/* Enhanced File System Statistics */
typedef struct {
    uint32_t        total_files;
    uint32_t        total_directories;
    uint32_t        fragmented_files;
    uint32_t        average_file_size;
    uint32_t        largest_free_space;
    float           fragmentation_ratio;
    uint32_t        bad_sectors;
} fat32_stats_t;

/* FAT32 Operation Results with Enhanced Error Codes */
typedef enum {
    FAT32_SUCCESS = 0,
    
    /* Initialization Errors */
    FAT32_ERROR_NOT_INITIALIZED,
    FAT32_ERROR_ALREADY_INITIALIZED,
    FAT32_ERROR_INVALID_PARAMETER,
    FAT32_ERROR_DRIVE_NOT_READY,
    
    /* I/O Errors */
    FAT32_ERROR_READ_FAILED,
    FAT32_ERROR_WRITE_FAILED,
    FAT32_ERROR_SEEK_FAILED,
    
    /* File System Errors */
    FAT32_ERROR_NOT_FOUND,
    FAT32_ERROR_INVALID_CLUSTER,
    FAT32_ERROR_INVALID_PATH,
    FAT32_ERROR_INVALID_FILENAME,
    FAT32_ERROR_CORRUPTED_FS,
    
    /* File Operation Errors */
    FAT32_ERROR_EOF,
    FAT32_ERROR_ACCESS_DENIED,
    FAT32_ERROR_FILE_EXISTS,
    FAT32_ERROR_NOT_DIRECTORY,
    FAT32_ERROR_IS_DIRECTORY,
    FAT32_ERROR_ALREADY_OPEN,
    FAT32_ERROR_NOT_OPEN,
    
    /* Space Management Errors */
    FAT32_ERROR_DISK_FULL,
    FAT32_ERROR_NO_FREE_CLUSTER,
    FAT32_ERROR_CLUSTER_CHAIN_BROKEN,
    
    /* Cache and Memory Errors */
    FAT32_ERROR_CACHE_FULL,
    FAT32_ERROR_OUT_OF_MEMORY,
    
    /* Security and Permission Errors */
    FAT32_ERROR_READ_ONLY,
    FAT32_ERROR_LOCKED,
    
    /* Advanced Feature Errors */
    FAT32_ERROR_NOT_SUPPORTED,
    FAT32_ERROR_VERSION_MISMATCH
} fat32_result_t;

/* Cache Configuration */
typedef struct {
    bool        enable_fat_cache;       /* Enable FAT sector caching */
    bool        enable_cluster_cache;   /* Enable cluster caching */
    bool        enable_dir_cache;       /* Enable directory caching */
    uint32_t    fat_cache_size;         /* FAT cache size in sectors */
    uint32_t    cluster_cache_size;     /* Cluster cache size */
    uint32_t    dir_cache_size;         /* Directory cache size */
    bool        write_through;          /* Write-through vs write-back */
} fat32_cache_config_t;

/* Function Prototypes */

/* === Core Initialization and Volume Management === */
fat32_result_t fat32_initialize(uint8_t drive);
fat32_result_t fat32_initialize_enhanced(uint8_t drive);
fat32_result_t fat32_initialize_with_config(uint8_t drive, const fat32_cache_config_t* config);
void fat32_shutdown(void);
void fat32_shutdown_enhanced(void);
fat32_result_t fat32_get_volume_info(fat32_volume_t* info);
fat32_result_t fat32_format_drive(uint8_t drive, const char* volume_label);
fat32_result_t fat32_check_filesystem(uint8_t drive);

/* === Enhanced File Operations === */
fat32_result_t fat32_open_file(const char* path, fat32_file_t* file);
fat32_result_t fat32_open_file_ex(const char* path, fat32_file_t* file, fat32_file_mode_t mode);
fat32_result_t fat32_close_file(fat32_file_t* file);
fat32_result_t fat32_read_file(fat32_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);
fat32_result_t fat32_write_file(fat32_file_t* file, const void* buffer, uint32_t size, uint32_t* bytes_written);
fat32_result_t fat32_seek_file(fat32_file_t* file, uint32_t position);
fat32_result_t fat32_seek_file_ex(fat32_file_t* file, int32_t offset, fat32_seek_origin_t origin);
fat32_result_t fat32_seek_file_enhanced(fat32_file_t* file, uint32_t position);
fat32_result_t fat32_tell_file(fat32_file_t* file, uint32_t* position);
fat32_result_t fat32_flush_file(fat32_file_t* file);
fat32_result_t fat32_truncate_file(fat32_file_t* file, uint32_t size);

/* === File Management === */
fat32_result_t fat32_create_file(const char* path, fat32_file_t* file);
fat32_result_t fat32_delete_file(const char* path);
fat32_result_t fat32_rename_file(const char* old_path, const char* new_path);
fat32_result_t fat32_copy_file(const char* src_path, const char* dst_path);
fat32_result_t fat32_move_file(const char* old_path, const char* new_path);
fat32_result_t fat32_get_file_size(const char* path, uint32_t* size);
fat32_result_t fat32_file_exists(const char* path, bool* exists);

/* === Directory Operations === */
fat32_result_t fat32_open_directory(const char* path, fat32_file_t* dir);
fat32_result_t fat32_create_directory(const char* path);
fat32_result_t fat32_delete_directory(const char* path);
fat32_result_t fat32_read_directory(fat32_file_t* dir, fat32_dir_entry_t* entry, char* long_name);
fat32_result_t fat32_get_current_directory(char* path, uint32_t size);
fat32_result_t fat32_change_directory(const char* path);
fat32_result_t fat32_list_directory(const char* path, fat32_dir_entry_t* entries, uint32_t max_entries, uint32_t* count);

/* === File Attributes and Properties === */
fat32_result_t fat32_get_file_attributes(const char* path, uint8_t* attributes);
fat32_result_t fat32_set_file_attributes(const char* path, uint8_t attributes);
fat32_result_t fat32_get_file_info(const char* path, fat32_dir_entry_t* info);
fat32_result_t fat32_get_file_timestamps(const char* path, uint16_t* create_date, uint16_t* create_time,
                                        uint16_t* access_date, uint16_t* write_date, uint16_t* write_time);
fat32_result_t fat32_set_file_timestamps(const char* path, uint16_t create_date, uint16_t create_time,
                                        uint16_t access_date, uint16_t write_date, uint16_t write_time);

/* === Space Management === */
fat32_result_t fat32_get_free_space(uint32_t* free_bytes);
fat32_result_t fat32_get_total_space(uint32_t* total_bytes);
fat32_result_t fat32_get_cluster_info(uint32_t* total_clusters, uint32_t* free_clusters, uint32_t* bad_clusters);
fat32_result_t fat32_get_statistics(fat32_stats_t* stats);

/* === Enhanced Internal Functions === */
uint32_t fat32_cluster_to_lba(uint32_t cluster);
uint32_t fat32_get_next_cluster(uint32_t cluster);
fat32_result_t fat32_set_next_cluster(uint32_t cluster, uint32_t next_cluster);
fat32_result_t fat32_allocate_cluster(uint32_t* cluster);
fat32_result_t fat32_allocate_cluster_enhanced(uint32_t* cluster);
fat32_result_t fat32_allocate_cluster_chain(uint32_t count, uint32_t* first_cluster);
fat32_result_t fat32_free_cluster_chain(uint32_t start_cluster);
fat32_result_t fat32_extend_cluster_chain(uint32_t last_cluster, uint32_t count);

/* === Cluster I/O Operations === */
fat32_result_t fat32_read_cluster(uint32_t cluster, void* buffer);
fat32_result_t fat32_write_cluster(uint32_t cluster, const void* buffer);
fat32_result_t fat32_read_cluster_cached(uint32_t cluster, void* buffer);
fat32_result_t fat32_write_cluster_cached(uint32_t cluster, const void* buffer);
fat32_result_t fat32_clear_cluster(uint32_t cluster);

/* === Enhanced Utility Functions === */
uint8_t fat32_calculate_checksum(const char* short_name);
void fat32_filename_to_83(const char* filename, char* shortname);
void fat32_83_to_filename(const fat32_dir_entry_t* entry, char* filename);
fat32_result_t fat32_parse_path(const char* path, char components[][12], int* component_count);
fat32_result_t fat32_parse_path_enhanced(const char* path, char components[][12], int* component_count);
fat32_result_t fat32_build_path(const char components[][12], int component_count, char* path, uint32_t max_len);
const char* fat32_result_to_string(fat32_result_t result);

/* === File System Validation and Repair === */
fat32_result_t fat32_validate_cluster_chain(uint32_t start_cluster, uint32_t* chain_length);
fat32_result_t fat32_check_filesystem_integrity(void);
fat32_result_t fat32_repair_filesystem(bool fix_errors);
fat32_result_t fat32_scan_for_bad_clusters(uint32_t* bad_count);

/* === Cache Management === */
fat32_result_t fat32_flush_all_caches(void);
fat32_result_t fat32_invalidate_cache(void);
fat32_result_t fat32_configure_cache(const fat32_cache_config_t* config);
fat32_result_t fat32_get_cache_stats(uint32_t* hits, uint32_t* misses, uint32_t* dirty_entries);

/* === Advanced Features === */
fat32_result_t fat32_defragment_basic(void);
fat32_result_t fat32_compact_free_space(void);
fat32_result_t fat32_optimize_fat_allocation(void);
fat32_result_t fat32_update_file_size(fat32_file_t* file, uint32_t new_size);

/* === DOS Time/Date Conversion Utilities === */
uint16_t fat32_time_to_dos_time(uint8_t hour, uint8_t minute, uint8_t second);
uint16_t fat32_date_to_dos_date(uint16_t year, uint8_t month, uint8_t day);
void fat32_dos_time_to_time(uint16_t dos_time, uint8_t* hour, uint8_t* minute, uint8_t* second);
void fat32_dos_date_to_date(uint16_t dos_date, uint16_t* year, uint8_t* month, uint8_t* day);

/* === Helper Functions for First Cluster Handling === */
static inline uint32_t fat32_get_first_cluster(const fat32_dir_entry_t* entry) {
    return ((uint32_t)entry->first_cluster_hi << 16) | entry->first_cluster_lo;
}

static inline void fat32_set_first_cluster(fat32_dir_entry_t* entry, uint32_t cluster) {
    entry->first_cluster_hi = (uint16_t)(cluster >> 16);
    entry->first_cluster_lo = (uint16_t)(cluster & 0xFFFF);
}

/* === Validation Macros === */
#define FAT32_VALIDATE_CLUSTER(cluster) \
    ((cluster) >= 2 && (cluster) < (fat32_volume.total_clusters + 2) && (cluster) < FAT32_EOC)

#define FAT32_IS_EOC(cluster) \
    ((cluster) >= FAT32_EOC)

#define FAT32_IS_FREE_CLUSTER(cluster) \
    ((cluster) == FAT32_FREE_CLUSTER)

#define FAT32_IS_BAD_CLUSTER(cluster) \
    ((cluster) == FAT32_BAD_CLUSTER)

/* === Debug and Logging Macros === */
#ifdef FAT32_DEBUG
    #define fat32_debug(msg, ...) printf("[FAT32] " msg "\n", ##__VA_ARGS__)
#else
    #define fat32_debug(msg, ...)
#endif

/* === External Volume Reference === */
extern fat32_volume_t fat32_volume;
extern uint32_t fat32_current_directory;

#endif /* FAT32_H */