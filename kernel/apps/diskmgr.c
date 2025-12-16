#include "diskmgr.h"
#include "../vga.h"
#include "../keyboard.h"
#include "../string.h"
#include "../memory.h"
#include "../disk.h"
#include "../gui.h"

// Disk manager state
static int selected_disk = 0;
static int view_offset = 0;

// Content area
#define CONTENT_Y 2
#define CONTENT_HEIGHT (VGA_HEIGHT - 4)

static void draw_titlebar(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, 0);
    }
    
    const char* title = "[ Disk Manager ]";
    int title_len = strlen(title);
    int start_x = (VGA_WIDTH - title_len) / 2;
    for (int i = 0; title[i]; i++) {
        vga_putchar_at(title[i], start_x + i, 0);
    }
    
    vga_set_color(old_color);
}

static void draw_statusbar(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, VGA_HEIGHT - 1);
    }
    
    const char* status = " Up/Down: Select | Enter: Details | R: Refresh | F: Format | ESC: Exit";
    for (int i = 0; status[i] && i < VGA_WIDTH - 1; i++) {
        vga_putchar_at(status[i], i, VGA_HEIGHT - 1);
    }
    
    vga_set_color(old_color);
}

static void draw_disk_list(void) {
    disk_manager_t* mgr = disk_get_manager();
    
    // Clear content area
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int y = CONTENT_Y; y < VGA_HEIGHT - 2; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar_at(' ', x, y);
        }
    }
    
    // Header
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("  # | Type     | Model                    | Size      | Status", 1, CONTENT_Y);
    
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    for (int x = 1; x < VGA_WIDTH - 1; x++) {
        vga_putchar_at('-', x, CONTENT_Y + 1);
    }
    
    // Disk entries
    int y = CONTENT_Y + 2;
    for (int i = 0; i < mgr->disk_count && y < VGA_HEIGHT - 3; i++) {
        disk_info_t* disk = &mgr->disks[i];
        if (!disk->present) continue;
        
        // Highlight selected
        if (i == selected_disk) {
            vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
            for (int x = 1; x < VGA_WIDTH - 1; x++) {
                vga_putchar_at(' ', x, y);
            }
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        }
        
        // Disk number
        char num_str[4];
        itoa(i, num_str, 10);
        vga_puts_at(num_str, 3, y);
        
        // Type
        const char* type_str = "???";
        switch (disk->type) {
            case DISK_TYPE_ATA:     type_str = "ATA"; break;
            case DISK_TYPE_ATAPI:   type_str = "ATAPI"; break;
            case DISK_TYPE_SATA:    type_str = "SATA"; break;
            case DISK_TYPE_VIRTUAL: type_str = "Virtual"; break;
            default: break;
        }
        vga_puts_at(type_str, 7, y);
        
        // Model (truncated)
        char model_buf[26];
        strncpy(model_buf, disk->model, 25);
        model_buf[25] = '\0';
        vga_puts_at(model_buf, 17, y);
        
        // Size
        char size_str[16];
        disk_format_size(disk->size_bytes, size_str, sizeof(size_str));
        vga_puts_at(size_str, 44, y);
        
        // Status
        vga_puts_at("Ready", 56, y);
        
        y++;
    }
    
    // If no disks
    if (mgr->disk_count == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        vga_puts_at("No disks detected", VGA_WIDTH / 2 - 8, VGA_HEIGHT / 2);
    }
}

static void draw_disk_details(void) {
    disk_manager_t* mgr = disk_get_manager();
    if (selected_disk < 0 || selected_disk >= mgr->disk_count) return;
    
    disk_info_t* disk = &mgr->disks[selected_disk];
    if (!disk->present) return;
    
    // Draw details panel at bottom
    int panel_y = VGA_HEIGHT - 8;
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, panel_y);
    }
    vga_puts_at(" Disk Details:", 1, panel_y);
    
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int y = panel_y + 1; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar_at(' ', x, y);
        }
    }
    
    // Show details
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Model:", 2, panel_y + 2);
    vga_puts_at("Serial:", 2, panel_y + 3);
    vga_puts_at("Sectors:", 2, panel_y + 4);
    vga_puts_at("Sector Size:", 2, panel_y + 5);
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at(disk->model, 15, panel_y + 2);
    vga_puts_at(disk->serial, 15, panel_y + 3);
    
    char buf[32];
    utoa(disk->sectors, buf, 10);
    vga_puts_at(buf, 15, panel_y + 4);
    
    utoa(disk->sector_size, buf, 10);
    strcat(buf, " bytes");
    vga_puts_at(buf, 15, panel_y + 5);
    
    // Right column
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Size:", 42, panel_y + 2);
    vga_puts_at("LBA48:", 42, panel_y + 3);
    vga_puts_at("DMA:", 42, panel_y + 4);
    vga_puts_at("Position:", 42, panel_y + 5);
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    char size_str[16];
    disk_format_size(disk->size_bytes, size_str, sizeof(size_str));
    vga_puts_at(size_str, 52, panel_y + 2);
    
    vga_puts_at(disk->supports_lba48 ? "Yes" : "No", 52, panel_y + 3);
    vga_puts_at(disk->supports_dma ? "Yes" : "No", 52, panel_y + 4);
    
    const char* pos_str = disk->is_primary ? 
        (disk->is_master ? "Primary Master" : "Primary Slave") :
        (disk->is_master ? "Secondary Master" : "Secondary Slave");
    if (disk->type == DISK_TYPE_VIRTUAL) {
        pos_str = "Virtual";
    }
    vga_puts_at(pos_str, 52, panel_y + 5);
}

void diskmgr_init(void) {
    selected_disk = 0;
    view_offset = 0;
    
    // Refresh disk list
    disk_detect_all();
}

void diskmgr_refresh(void) {
    disk_detect_all();
    diskmgr_redraw();
}

void diskmgr_redraw(void) {
    vga_clear();
    draw_titlebar();
    draw_disk_list();
    draw_disk_details();
    draw_statusbar();
}

void diskmgr_run(void) {
    diskmgr_init();
    diskmgr_redraw();
    
    disk_manager_t* mgr = disk_get_manager();
    bool running = true;
    
    while (running) {
        key_event_t event = keyboard_get_key();
        if (event.released) continue;
        
        switch (event.scancode) {
            case KEY_ESCAPE:
                running = false;
                break;
                
            case KEY_UP:
                if (selected_disk > 0) {
                    selected_disk--;
                    diskmgr_redraw();
                }
                break;
                
            case KEY_DOWN:
                if (selected_disk < mgr->disk_count - 1) {
                    selected_disk++;
                    diskmgr_redraw();
                }
                break;
                
            case KEY_ENTER:
                // Show more details (already shown in panel)
                diskmgr_redraw();
                break;
                
            default:
                if (event.ascii == 'r' || event.ascii == 'R') {
                    // Refresh
                    diskmgr_refresh();
                } else if (event.ascii == 'f' || event.ascii == 'F') {
                    // Format virtual disk
                    if (mgr->disks[selected_disk].type == DISK_TYPE_VIRTUAL) {
                        gui_message_box("Format", "Virtual disk formatted!");
                        virtual_disk_format();
                        diskmgr_redraw();
                    } else {
                        gui_message_box("Error", "Cannot format real disks!");
                        diskmgr_redraw();
                    }
                }
                break;
        }
    }
}
