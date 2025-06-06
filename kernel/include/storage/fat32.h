#ifndef FAT32_H
#define FAT32_H

#include "../common/types.h"
#include "../storage/hdd.h"

/* FAT32 Specific Constants */
#define FAT32_SIGNATURE               0x29    /* Extended boot signature */
#define FAT32_FILE_SYSTEM_TYPE        "FAT32   " /* File system type string */
#define FAT32_SECTOR_SIZE             512     /* Standard sector size */
#define FAT32_DIR_ENTRY_SIZE          32      /* Size of directory entry */
#define FAT32_ENTRIES_PER_SECTOR      (FAT32_SECTOR_SIZE / FAT32_DIR_ENTRY_SIZE)
#define FAT32_MAX_FILENAME            255     /* Max filename length with LFN */
#define FAT32_SFN_NAME_SIZE           8       /* 8.3 filename main part */
#define FAT32_SFN_EXT_SIZE            3       /* 8.3 filename extension part */

/* FAT32 Cluster Constants */
#define FAT32_EOC                     0x0FFFFFF8  /* End of cluster chain marker */
#define FAT32_BAD_CLUSTER             0x0FFFFFF7  /* Bad cluster marker */
#define FAT32_FREE_CLUSTER            0x00000000  /* Free cluster marker */
#define FAT32_RESERVED_CLUSTER        0x00000001  /* Reserved cluster */
#define FAT32_CLUSTER_MASK            0x0FFFFFFF  /* Mask for cluster value (28 bits) */

/* FAT32 Attribute Bits */
#define FAT32_ATTR_READ_ONLY          0x01    /* Read-only */
#define FAT32_ATTR_HIDDEN             0x02    /* Hidden */
#define FAT32_ATTR_SYSTEM             0x04    /* System */
#define FAT32_ATTR_VOLUME_ID          0x08    /* Volume ID */
#define FAT32_ATTR_DIRECTORY          0x10    /* Directory */
#define FAT32_ATTR_ARCHIVE            0x20    /* Archive */
#define FAT32_ATTR_LONG_NAME          0x0F    /* Long name entry */
#define FAT32_ATTR_LONG_NAME_MASK     0x3F    /* Mask for long name attributes */

/* FAT32 Time/Date Macros */
#define FAT32_TIME(h,m,s)             ((h<<11)|(m<<5)|(s/2))
#define FAT32_DATE(y,m,d)             (((y-1980)<<9)|(m<<5)|d)

/* FAT32 Boot Sector Structure */
typedef struct {
    /* BIOS Parameter Block (BPB) */
    uint8_t     jump_boot[3];         /* Jump instruction to boot code */
    char        oem_name[8];          /* OEM Name identifier */
    uint16_t    bytes_per_sector;     /* Bytes per sector (usually 512) */
    uint8_t     sectors_per_cluster;  /* Sectors per cluster */
    uint16_t    reserved_sectors;     /* Reserved sectors count */
    uint8_t     num_fats;             /* Number of FAT copies (usually 2) */
    uint16_t    root_entry_count;     /* Number of root directory entries (0 for FAT32) */
    uint16_t    total_sectors_16;     /* Total sectors (0 for FAT32) */
    uint8_t     media_type;           /* Media descriptor type */
    uint16_t    sectors_per_fat_16;   /* Sectors per FAT (0 for FAT32) */
    uint16_t    sectors_per_track;    /* Sectors per track (geometry) */
    uint16_t    num_heads;            /* Number of heads (geometry) */
    uint32_t    hidden_sectors;       /* Hidden sectors count */
    uint32_t    total_sectors_32;     /* Total sectors (used for FAT32) */
    
    /* FAT32 Extended Boot Record */
    uint32_t    sectors_per_fat_32;   /* Sectors per FAT */
    uint16_t    ext_flags;            /* Flags for FAT32 */
    uint16_t    fs_version;           /* File system version */
    uint32_t    root_cluster;         /* First cluster of root directory */
    uint16_t    fs_info;              /* FS info sector number */
    uint16_t    backup_boot;          /* Backup boot sector */
    uint8_t     reserved[12];         /* Reserved for future expansion */
    uint8_t     drive_number;         /* BIOS drive number */
    uint8_t     reserved1;            /* Reserved */
    uint8_t     boot_signature;       /* Extended boot signature (0x29) */
    uint32_t    volume_id;            /* Volume serial number */
    char        volume_label[11];     /* Volume label */
    char        fs_type[8];           /* File system type (FAT32) */
    
    /* The rest of the sector is boot code and signature */
} __attribute__((packed)) fat32_boot_sector_t;

/* FAT32 FSInfo Sector Structure */
typedef struct {
    uint32_t    lead_signature;       /* Lead signature (0x41615252) */
    uint8_t     reserved1[480];       /* Reserved */
    uint32_t    structure_signature;  /* Structure signature (0x61417272) */
    uint32_t    free_cluster_count;   /* Free cluster count */
    uint32_t    next_free_cluster;    /* Next free cluster */
    uint8_t     reserved2[12];        /* Reserved */
    uint32_t    trail_signature;      /* Trail signature (0xAA550000) */
} __attribute__((packed)) fat32_fsinfo_t;

/* FAT32 Directory Entry Structure */
typedef struct {
    char        name[FAT32_SFN_NAME_SIZE];    /* Base filename */
    char        ext[FAT32_SFN_EXT_SIZE];      /* Extension */
    uint8_t     attributes;           /* File attributes */
    uint8_t     reserved;             /* Reserved for Windows NT */
    uint8_t     creation_time_tenth;  /* Creation time (tenth of second) */
    uint16_t    creation_time;        /* Creation time */
    uint16_t    creation_date;        /* Creation date */
    uint16_t    last_access_date;     /* Last access date */
    uint16_t    first_cluster_high;   /* High word of first cluster */
    uint16_t    last_modify_time;     /* Last modification time */
    uint16_t    last_modify_date;     /* Last modification date */
    uint16_t    first_cluster_low;    /* Low word of first cluster */
    uint32_t    file_size;            /* File size in bytes */
} __attribute__((packed)) fat32_dir_entry_t;

/* FAT32 Long Filename Entry Structure */
typedef struct {
    uint8_t     order;                /* Sequence number */
    uint16_t    name1[5];             /* Characters 1-5 */
    uint8_t     attributes;           /* Attributes (always 0x0F) */
    uint8_t     type;                 /* Type (reserved, always 0) */
    uint8_t     checksum;             /* Checksum of short name */
    uint16_t    name2[6];             /* Characters 6-11 */
    uint16_t    first_cluster_low;    /* First cluster (always 0) */
    uint16_t    name3[2];             /* Characters 12-13 */
} __attribute__((packed)) fat32_lfn_entry_t;

/* File Handle Structure */
typedef struct {
    bool        is_open;              /* Is file open */
    bool        is_directory;         /* Is this a directory */
    uint32_t    first_cluster;        /* First cluster */
    uint32_t    current_cluster;      /* Current cluster */
    uint32_t    current_sector;       /* Current sector in cluster */
    uint32_t    position;             /* Current position in file */
    uint32_t    size;                 /* File size */
    char        name[FAT32_MAX_FILENAME+1]; /* File name */
} fat32_file_t;

/* FAT32 Volume Information Structure */
typedef struct {
    uint8_t         drive;                   /* Drive number */
    bool            initialized;             /* Is initialized */
    uint32_t        total_sectors;           /* Total sectors */
    uint32_t        fat_begin_lba;           /* First FAT LBA */
    uint32_t        cluster_begin_lba;       /* First cluster LBA */
    uint32_t        root_dir_first_cluster;  /* Root directory cluster */
    uint32_t        data_sectors;            /* Data sectors count */
    uint32_t        total_clusters;          /* Total clusters */
    uint32_t        free_clusters;           /* Free clusters count */
    uint32_t        next_free_cluster;       /* Next free cluster */
    uint8_t         sectors_per_cluster;     /* Sectors per cluster */
    uint32_t        fat_size;                /* FAT size in sectors */
    uint8_t         num_fats;                /* Number of FATs */
    char            volume_label[12];        /* Volume label */
    uint16_t        bytes_per_sector;        /* Bytes per sector */
    fat32_boot_sector_t boot_sector;         /* Boot sector */
} fat32_volume_t;

/* FAT32 Operation Result */
typedef enum {
    FAT32_SUCCESS = 0,
    FAT32_ERROR_NOT_INITIALIZED,
    FAT32_ERROR_READ_FAILED,
    FAT32_ERROR_WRITE_FAILED,
    FAT32_ERROR_NOT_FOUND,
    FAT32_ERROR_INVALID_PARAMETER,
    FAT32_ERROR_EOF,
    FAT32_ERROR_INVALID_CLUSTER,
    FAT32_ERROR_NO_FREE_CLUSTER,
    FAT32_ERROR_ACCESS_DENIED,
    FAT32_ERROR_FILE_EXISTS,
    FAT32_ERROR_NOT_DIRECTORY,
    FAT32_ERROR_IS_DIRECTORY,
    FAT32_ERROR_DISK_FULL,
    FAT32_ERROR_INVALID_PATH,
    FAT32_ERROR_ALREADY_OPEN,
    FAT32_ERROR_DRIVE_NOT_READY
} fat32_result_t;

/* Function prototypes */

/* Initialization and volume operations */
fat32_result_t fat32_initialize(uint8_t drive);
void fat32_shutdown(void);
fat32_result_t fat32_get_volume_info(fat32_volume_t* info);
fat32_result_t fat32_format_drive(uint8_t drive, const char* volume_label);
fat32_result_t fat32_check_filesystem(uint8_t drive);

/* File operations */
fat32_result_t fat32_open_file(const char* path, fat32_file_t* file);
fat32_result_t fat32_close_file(fat32_file_t* file);
fat32_result_t fat32_read_file(fat32_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);
fat32_result_t fat32_write_file(fat32_file_t* file, const void* buffer, uint32_t size, uint32_t* bytes_written);
fat32_result_t fat32_seek_file(fat32_file_t* file, uint32_t position);
fat32_result_t fat32_create_file(const char* path, fat32_file_t* file);
fat32_result_t fat32_delete_file(const char* path);
fat32_result_t fat32_rename_file(const char* old_path, const char* new_path);
fat32_result_t fat32_get_file_size(const char* path, uint32_t* size);

/* Directory operations */
fat32_result_t fat32_open_directory(const char* path, fat32_file_t* dir);
fat32_result_t fat32_create_directory(const char* path);
fat32_result_t fat32_delete_directory(const char* path);
fat32_result_t fat32_read_directory(fat32_file_t* dir, fat32_dir_entry_t* entry, char* long_name);
fat32_result_t fat32_get_current_directory(char* path, uint32_t size);
fat32_result_t fat32_change_directory(const char* path);

/* Utility functions */
fat32_result_t fat32_get_free_space(uint32_t* free_bytes);
fat32_result_t fat32_get_total_space(uint32_t* total_bytes);
fat32_result_t fat32_get_file_attributes(const char* path, uint8_t* attributes);
fat32_result_t fat32_set_file_attributes(const char* path, uint8_t attributes);
const char* fat32_result_to_string(fat32_result_t result);

/* Internal functions (not to be used directly) */
uint32_t fat32_cluster_to_lba(uint32_t cluster);
uint32_t fat32_get_next_cluster(uint32_t cluster);
fat32_result_t fat32_set_next_cluster(uint32_t cluster, uint32_t next_cluster);
fat32_result_t fat32_allocate_cluster(uint32_t* cluster);
fat32_result_t fat32_free_cluster_chain(uint32_t start_cluster);
uint8_t fat32_calculate_checksum(const char* short_name);
void fat32_filename_to_83(const char* filename, char* shortname);
void fat32_83_to_filename(const fat32_dir_entry_t* entry, char* filename);

#endif /* FAT32_H */
