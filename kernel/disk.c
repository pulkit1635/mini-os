#include "disk.h"
#include "io.h"
#include "string.h"
#include "memory.h"

// Global disk manager
static disk_manager_t g_disk_manager;

// Virtual disk buffer (in-memory disk for testing)
static uint8_t virtual_disk[VIRTUAL_DISK_SIZE];
static bool virtual_disk_initialized = false;

// Wait for disk to be ready
static bool ata_wait_ready(uint16_t io_base) {
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(io_base + 7);
        if (!(status & ATA_SR_BSY)) {
            return true;
        }
    }
    return false;
}

// Wait for data request
static bool ata_wait_drq(uint16_t io_base) {
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(io_base + 7);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_DRQ) {
                return true;
            }
            if (status & ATA_SR_ERR) {
                return false;
            }
        }
    }
    return false;
}

// Select drive
static void ata_select_drive(uint16_t io_base, bool slave) {
    outb(io_base + 6, slave ? 0xF0 : 0xE0);
    // Wait 400ns (read status 15 times)
    for (int i = 0; i < 15; i++) {
        inb(io_base + 7);
    }
}

// Soft reset the ATA controller
static void ata_soft_reset(uint16_t control_port) {
    outb(control_port, 0x04);  // Set SRST bit
    for (int i = 0; i < 5; i++) inb(control_port);  // Wait
    outb(control_port, 0x00);  // Clear SRST bit
    for (int i = 0; i < 5; i++) inb(control_port);  // Wait
}

// Identify drive
static bool ata_identify(uint16_t io_base, uint16_t control_port, bool slave, disk_info_t* info) {
    memset(info, 0, sizeof(disk_info_t));
    
    // Select drive
    ata_select_drive(io_base, slave);
    
    // Wait for drive ready
    if (!ata_wait_ready(io_base)) {
        return false;
    }
    
    // Clear sector count and LBA registers
    outb(io_base + 2, 0);
    outb(io_base + 3, 0);
    outb(io_base + 4, 0);
    outb(io_base + 5, 0);
    
    // Send IDENTIFY command
    outb(io_base + 7, ATA_CMD_IDENTIFY);
    
    // Wait for response
    uint8_t status = inb(io_base + 7);
    if (status == 0) {
        return false;  // Drive doesn't exist
    }
    
    // Wait for BSY to clear
    int timeout = 100000;
    while ((status & ATA_SR_BSY) && timeout--) {
        status = inb(io_base + 7);
    }
    if (timeout <= 0) {
        return false;
    }
    
    // Check for ATAPI
    uint8_t lba_mid = inb(io_base + 4);
    uint8_t lba_hi = inb(io_base + 5);
    if (lba_mid == 0x14 && lba_hi == 0xEB) {
        info->type = DISK_TYPE_ATAPI;
        // ATAPI detection successful but not fully supported
        return false;
    }
    if (lba_mid == 0x3C && lba_hi == 0xC3) {
        info->type = DISK_TYPE_SATA;
    }
    
    // Wait for DRQ or ERR
    if (!ata_wait_drq(io_base)) {
        return false;
    }
    
    // Read identify data
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(io_base);
    }
    
    // Parse identify data
    info->present = true;
    info->type = DISK_TYPE_ATA;
    info->is_master = !slave;
    info->sector_size = 512;
    
    // Extract model string (words 27-46)
    for (int i = 0; i < 20; i++) {
        uint16_t word = identify_data[27 + i];
        info->model[i * 2] = (char)(word >> 8);
        info->model[i * 2 + 1] = (char)(word & 0xFF);
    }
    info->model[40] = '\0';
    // Trim trailing spaces
    for (int i = 39; i >= 0 && info->model[i] == ' '; i--) {
        info->model[i] = '\0';
    }
    
    // Extract serial number (words 10-19)
    for (int i = 0; i < 10; i++) {
        uint16_t word = identify_data[10 + i];
        info->serial[i * 2] = (char)(word >> 8);
        info->serial[i * 2 + 1] = (char)(word & 0xFF);
    }
    info->serial[20] = '\0';
    // Trim trailing spaces
    for (int i = 19; i >= 0 && info->serial[i] == ' '; i--) {
        info->serial[i] = '\0';
    }
    
    // LBA28 sector count (words 60-61)
    info->sectors = (uint32_t)identify_data[60] | ((uint32_t)identify_data[61] << 16);
    
    // Check for LBA48 support (word 83, bit 10)
    info->supports_lba48 = (identify_data[83] & (1 << 10)) != 0;
    
    // LBA48 sector count (words 100-103)
    if (info->supports_lba48) {
        info->sectors48 = (uint64_t)identify_data[100] |
                         ((uint64_t)identify_data[101] << 16) |
                         ((uint64_t)identify_data[102] << 32) |
                         ((uint64_t)identify_data[103] << 48);
    } else {
        info->sectors48 = info->sectors;
    }
    
    // Calculate size
    info->size_bytes = (uint64_t)info->sectors48 * info->sector_size;
    info->size_mb = (uint32_t)(info->size_bytes / (1024 * 1024));
    
    // Check for DMA support (word 49, bit 8)
    info->supports_dma = (identify_data[49] & (1 << 8)) != 0;
    
    return true;
}

void disk_init(void) {
    memset(&g_disk_manager, 0, sizeof(g_disk_manager));
    
    // Initialize virtual disk
    memset(virtual_disk, 0, VIRTUAL_DISK_SIZE);
    virtual_disk_initialized = true;
    
    // Create a virtual disk entry for testing
    disk_info_t* vdisk = &g_disk_manager.disks[0];
    vdisk->present = true;
    vdisk->type = DISK_TYPE_VIRTUAL;
    vdisk->is_master = true;
    vdisk->is_primary = true;
    strcpy(vdisk->model, "MiniOS Virtual Disk");
    strcpy(vdisk->serial, "VDISK001");
    vdisk->sector_size = VIRTUAL_SECTOR_SIZE;
    vdisk->sectors = VIRTUAL_DISK_SIZE / VIRTUAL_SECTOR_SIZE;
    vdisk->sectors48 = vdisk->sectors;
    vdisk->size_bytes = VIRTUAL_DISK_SIZE;
    vdisk->size_mb = VIRTUAL_DISK_SIZE / (1024 * 1024);
    vdisk->supports_lba48 = false;
    vdisk->supports_dma = false;
    
    g_disk_manager.disk_count = 1;
    g_disk_manager.selected_disk = 0;
    
    // Try to detect real disks (but don't fail if none found)
    disk_detect_all();
}

disk_manager_t* disk_get_manager(void) {
    return &g_disk_manager;
}

int disk_detect_all(void) {
    int found = 0;
    
    // Keep the virtual disk at index 0
    if (g_disk_manager.disks[0].type == DISK_TYPE_VIRTUAL) {
        found = 1;
    }
    
    // Try to detect real ATA drives
    // Primary master
    disk_info_t temp_info;
    if (ata_identify(ATA_PRIMARY_DATA, ATA_PRIMARY_CONTROL, false, &temp_info)) {
        temp_info.is_primary = true;
        if (found < 4) {
            memcpy(&g_disk_manager.disks[found], &temp_info, sizeof(disk_info_t));
            found++;
        }
    }
    
    // Primary slave
    if (ata_identify(ATA_PRIMARY_DATA, ATA_PRIMARY_CONTROL, true, &temp_info)) {
        temp_info.is_primary = true;
        if (found < 4) {
            memcpy(&g_disk_manager.disks[found], &temp_info, sizeof(disk_info_t));
            found++;
        }
    }
    
    // Secondary master
    if (ata_identify(ATA_SECONDARY_DATA, ATA_SECONDARY_CONTROL, false, &temp_info)) {
        temp_info.is_primary = false;
        if (found < 4) {
            memcpy(&g_disk_manager.disks[found], &temp_info, sizeof(disk_info_t));
            found++;
        }
    }
    
    // Secondary slave
    if (ata_identify(ATA_SECONDARY_DATA, ATA_SECONDARY_CONTROL, true, &temp_info)) {
        temp_info.is_primary = false;
        if (found < 4) {
            memcpy(&g_disk_manager.disks[found], &temp_info, sizeof(disk_info_t));
            found++;
        }
    }
    
    g_disk_manager.disk_count = found;
    return found;
}

disk_info_t* disk_get_info(int disk_index) {
    if (disk_index < 0 || disk_index >= 4) {
        return NULL;
    }
    return &g_disk_manager.disks[disk_index];
}

bool disk_read_sectors(int disk_index, uint32_t lba, uint8_t count, void* buffer) {
    if (disk_index < 0 || disk_index >= g_disk_manager.disk_count) {
        return false;
    }
    
    disk_info_t* disk = &g_disk_manager.disks[disk_index];
    if (!disk->present) {
        return false;
    }
    
    // Handle virtual disk
    if (disk->type == DISK_TYPE_VIRTUAL) {
        for (uint8_t i = 0; i < count; i++) {
            if (!virtual_disk_read(lba + i, (uint8_t*)buffer + i * VIRTUAL_SECTOR_SIZE)) {
                return false;
            }
        }
        return true;
    }
    
    // Real ATA disk read
    uint16_t io_base = disk->is_primary ? ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    
    // Wait for drive ready
    if (!ata_wait_ready(io_base)) {
        return false;
    }
    
    // Select drive and set LBA mode
    uint8_t drive_select = disk->is_master ? 0xE0 : 0xF0;
    drive_select |= (lba >> 24) & 0x0F;
    outb(io_base + 6, drive_select);
    
    // Set sector count and LBA
    outb(io_base + 2, count);
    outb(io_base + 3, (uint8_t)(lba & 0xFF));
    outb(io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    
    // Send read command
    outb(io_base + 7, ATA_CMD_READ_PIO);
    
    // Read sectors
    uint16_t* buf = (uint16_t*)buffer;
    for (uint8_t i = 0; i < count; i++) {
        // Wait for data
        if (!ata_wait_drq(io_base)) {
            return false;
        }
        
        // Read 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            buf[i * 256 + j] = inw(io_base);
        }
    }
    
    return true;
}

bool disk_write_sectors(int disk_index, uint32_t lba, uint8_t count, const void* buffer) {
    if (disk_index < 0 || disk_index >= g_disk_manager.disk_count) {
        return false;
    }
    
    disk_info_t* disk = &g_disk_manager.disks[disk_index];
    if (!disk->present) {
        return false;
    }
    
    // Handle virtual disk
    if (disk->type == DISK_TYPE_VIRTUAL) {
        for (uint8_t i = 0; i < count; i++) {
            if (!virtual_disk_write(lba + i, (const uint8_t*)buffer + i * VIRTUAL_SECTOR_SIZE)) {
                return false;
            }
        }
        return true;
    }
    
    // Real ATA disk write
    uint16_t io_base = disk->is_primary ? ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    
    // Wait for drive ready
    if (!ata_wait_ready(io_base)) {
        return false;
    }
    
    // Select drive and set LBA mode
    uint8_t drive_select = disk->is_master ? 0xE0 : 0xF0;
    drive_select |= (lba >> 24) & 0x0F;
    outb(io_base + 6, drive_select);
    
    // Set sector count and LBA
    outb(io_base + 2, count);
    outb(io_base + 3, (uint8_t)(lba & 0xFF));
    outb(io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    
    // Send write command
    outb(io_base + 7, ATA_CMD_WRITE_PIO);
    
    // Write sectors
    const uint16_t* buf = (const uint16_t*)buffer;
    for (uint8_t i = 0; i < count; i++) {
        // Wait for ready
        if (!ata_wait_drq(io_base)) {
            return false;
        }
        
        // Write 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            outw(io_base, buf[i * 256 + j]);
        }
        
        // Flush cache
        outb(io_base + 7, ATA_CMD_CACHE_FLUSH);
        ata_wait_ready(io_base);
    }
    
    return true;
}

void disk_select(int disk_index) {
    if (disk_index >= 0 && disk_index < g_disk_manager.disk_count) {
        g_disk_manager.selected_disk = disk_index;
    }
}

int disk_get_selected(void) {
    return g_disk_manager.selected_disk;
}

void disk_format_size(uint64_t bytes, char* buffer, size_t buffer_size) {
    if (buffer_size < 16) return;
    
    if (bytes >= 1024ULL * 1024 * 1024 * 1024) {
        uint32_t tb = (uint32_t)(bytes / (1024ULL * 1024 * 1024 * 1024));
        utoa(tb, buffer, 10);
        strcat(buffer, " TB");
    } else if (bytes >= 1024ULL * 1024 * 1024) {
        uint32_t gb = (uint32_t)(bytes / (1024ULL * 1024 * 1024));
        utoa(gb, buffer, 10);
        strcat(buffer, " GB");
    } else if (bytes >= 1024 * 1024) {
        uint32_t mb = (uint32_t)(bytes / (1024 * 1024));
        utoa(mb, buffer, 10);
        strcat(buffer, " MB");
    } else if (bytes >= 1024) {
        uint32_t kb = (uint32_t)(bytes / 1024);
        utoa(kb, buffer, 10);
        strcat(buffer, " KB");
    } else {
        utoa((uint32_t)bytes, buffer, 10);
        strcat(buffer, " B");
    }
}

bool disk_is_present(int disk_index) {
    if (disk_index < 0 || disk_index >= 4) {
        return false;
    }
    return g_disk_manager.disks[disk_index].present;
}

int disk_get_count(void) {
    return g_disk_manager.disk_count;
}

// Virtual disk operations
bool virtual_disk_read(uint32_t sector, void* buffer) {
    if (!virtual_disk_initialized) {
        return false;
    }
    
    uint32_t offset = sector * VIRTUAL_SECTOR_SIZE;
    if (offset + VIRTUAL_SECTOR_SIZE > VIRTUAL_DISK_SIZE) {
        return false;
    }
    
    memcpy(buffer, virtual_disk + offset, VIRTUAL_SECTOR_SIZE);
    return true;
}

bool virtual_disk_write(uint32_t sector, const void* buffer) {
    if (!virtual_disk_initialized) {
        return false;
    }
    
    uint32_t offset = sector * VIRTUAL_SECTOR_SIZE;
    if (offset + VIRTUAL_SECTOR_SIZE > VIRTUAL_DISK_SIZE) {
        return false;
    }
    
    memcpy(virtual_disk + offset, buffer, VIRTUAL_SECTOR_SIZE);
    return true;
}

void virtual_disk_format(void) {
    memset(virtual_disk, 0, VIRTUAL_DISK_SIZE);
}
