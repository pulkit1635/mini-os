#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ATA I/O ports (Primary IDE controller)
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERROR        0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7
#define ATA_PRIMARY_CONTROL      0x3F6

// ATA I/O ports (Secondary IDE controller)
#define ATA_SECONDARY_DATA       0x170
#define ATA_SECONDARY_ERROR      0x171
#define ATA_SECONDARY_SECCOUNT   0x172
#define ATA_SECONDARY_LBA_LO     0x173
#define ATA_SECONDARY_LBA_MID    0x174
#define ATA_SECONDARY_LBA_HI     0x175
#define ATA_SECONDARY_DRIVE_HEAD 0x176
#define ATA_SECONDARY_STATUS     0x177
#define ATA_SECONDARY_COMMAND    0x177
#define ATA_SECONDARY_CONTROL    0x376

// ATA commands
#define ATA_CMD_READ_PIO         0x20
#define ATA_CMD_WRITE_PIO        0x30
#define ATA_CMD_CACHE_FLUSH      0xE7
#define ATA_CMD_IDENTIFY         0xEC

// ATA status bits
#define ATA_SR_BSY               0x80    // Busy
#define ATA_SR_DRDY              0x40    // Drive ready
#define ATA_SR_DF                0x20    // Drive fault
#define ATA_SR_DSC               0x10    // Drive seek complete
#define ATA_SR_DRQ               0x08    // Data request ready
#define ATA_SR_CORR              0x04    // Corrected data
#define ATA_SR_IDX               0x02    // Index
#define ATA_SR_ERR               0x01    // Error

// Drive types
typedef enum {
    DISK_TYPE_NONE = 0,
    DISK_TYPE_ATA,
    DISK_TYPE_ATAPI,
    DISK_TYPE_SATA,
    DISK_TYPE_VIRTUAL
} disk_type_t;

// Disk information structure
typedef struct {
    bool present;                   // Drive is present
    disk_type_t type;              // Type of disk
    bool is_master;                // True = master, False = slave
    bool is_primary;               // True = primary controller
    char model[41];                // Model string
    char serial[21];               // Serial number
    uint32_t sectors;              // Total sectors (LBA28)
    uint64_t sectors48;            // Total sectors (LBA48)
    uint32_t sector_size;          // Bytes per sector (usually 512)
    uint64_t size_bytes;           // Total size in bytes
    uint32_t size_mb;              // Total size in MB
    bool supports_lba48;           // LBA48 support
    bool supports_dma;             // DMA support
} disk_info_t;

// Disk manager state
typedef struct {
    disk_info_t disks[4];          // Up to 4 IDE drives
    int disk_count;                // Number of detected disks
    int selected_disk;             // Currently selected disk
} disk_manager_t;

// Virtual disk for testing (in-memory)
#define VIRTUAL_DISK_SIZE (64 * 1024)  // 64 KB virtual disk
#define VIRTUAL_SECTOR_SIZE 512

// Initialize disk subsystem
void disk_init(void);

// Get disk manager state
disk_manager_t* disk_get_manager(void);

// Detect all disks
int disk_detect_all(void);

// Get disk info
disk_info_t* disk_get_info(int disk_index);

// Read sectors from disk
bool disk_read_sectors(int disk_index, uint32_t lba, uint8_t count, void* buffer);

// Write sectors to disk
bool disk_write_sectors(int disk_index, uint32_t lba, uint8_t count, const void* buffer);

// Select disk for operations
void disk_select(int disk_index);

// Get selected disk
int disk_get_selected(void);

// Format disk size as string
void disk_format_size(uint64_t bytes, char* buffer, size_t buffer_size);

// Check if disk is present
bool disk_is_present(int disk_index);

// Get number of detected disks
int disk_get_count(void);

// Virtual disk operations (for testing without real hardware)
bool virtual_disk_read(uint32_t sector, void* buffer);
bool virtual_disk_write(uint32_t sector, const void* buffer);
void virtual_disk_format(void);

#endif // DISK_H
