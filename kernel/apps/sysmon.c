#include "sysmon.h"
#include "../vga.h"
#include "../keyboard.h"
#include "../string.h"
#include "../memory.h"
#include "../disk.h"
#include "../network.h"
#include "../audio.h"

// Refresh counter for simulation
static int refresh_counter = 0;

// Simulated CPU usage history (for graph)
static int cpu_history[40];
static int history_index = 0;

// Simple pseudo-random for simulation
static uint32_t sysmon_rand_seed = 54321;
static int sysmon_rand(void) {
    sysmon_rand_seed = sysmon_rand_seed * 1103515245 + 12345;
    return (sysmon_rand_seed >> 16) % 100;
}

static void draw_titlebar(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, 0);
    }
    
    const char* title = "[ System Monitor ]";
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
    
    const char* status = " R: Refresh | ESC: Exit | Auto-refresh active";
    for (int i = 0; status[i] && i < VGA_WIDTH - 1; i++) {
        vga_putchar_at(status[i], i, VGA_HEIGHT - 1);
    }
    
    vga_set_color(old_color);
}

static void draw_box(int x, int y, int w, int h, const char* title) {
    uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    // Draw border
    vga_set_color(border_color);
    vga_putchar_at('+', x, y);
    vga_putchar_at('+', x + w - 1, y);
    vga_putchar_at('+', x, y + h - 1);
    vga_putchar_at('+', x + w - 1, y + h - 1);
    
    for (int i = 1; i < w - 1; i++) {
        vga_putchar_at('-', x + i, y);
        vga_putchar_at('-', x + i, y + h - 1);
    }
    
    for (int j = 1; j < h - 1; j++) {
        vga_putchar_at('|', x, y + j);
        vga_putchar_at('|', x + w - 1, y + j);
    }
    
    // Clear inside
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int j = 1; j < h - 1; j++) {
        for (int i = 1; i < w - 1; i++) {
            vga_putchar_at(' ', x + i, y + j);
        }
    }
    
    // Draw title
    if (title && title[0]) {
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        int title_len = strlen(title);
        int title_x = x + (w - title_len - 4) / 2;
        vga_puts_at("[ ", title_x, y);
        vga_puts_at(title, title_x + 2, y);
        vga_puts_at(" ]", title_x + 2 + title_len, y);
    }
}

static void draw_progress_bar(int x, int y, int width, int value, int max, uint8_t fill_color) {
    int fill = (value * width) / max;
    if (fill > width) fill = width;
    
    for (int i = 0; i < width; i++) {
        if (i < fill) {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, fill_color));
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
        }
        vga_putchar_at(' ', x + i, y);
    }
}

static void draw_cpu_panel(void) {
    int x = 1, y = 2, w = 39, h = 10;
    draw_box(x, y, w, h, "CPU");
    
    // Simulate CPU usage
    int cpu_usage = 10 + sysmon_rand() % 30;
    cpu_history[history_index] = cpu_usage;
    history_index = (history_index + 1) % 40;
    
    // Draw CPU graph
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Usage:", x + 2, y + 2);
    
    char usage_str[8];
    itoa(cpu_usage, usage_str, 10);
    strcat(usage_str, "%");
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at(usage_str, x + 9, y + 2);
    
    // Progress bar
    draw_progress_bar(x + 2, y + 3, w - 4, cpu_usage, 100, 
                     cpu_usage < 60 ? VGA_COLOR_GREEN : 
                     (cpu_usage < 85 ? VGA_COLOR_BROWN : VGA_COLOR_RED));
    
    // Mini history graph
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    vga_puts_at("History:", x + 2, y + 5);
    
    for (int i = 0; i < 30; i++) {
        int idx = (history_index + i) % 40;
        int height = (cpu_history[idx] * 3) / 100;
        
        for (int j = 0; j < 3; j++) {
            if (3 - j <= height) {
                vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                vga_putchar_at('#', x + 2 + i, y + 6 + j);
            } else {
                vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
                vga_putchar_at('.', x + 2 + i, y + 6 + j);
            }
        }
    }
}

static void draw_memory_panel(void) {
    int x = 41, y = 2, w = 38, h = 10;
    draw_box(x, y, w, h, "Memory");
    
    memory_stats_t stats;
    memory_get_stats(&stats);
    
    int used_percent = (stats.used_size * 100) / stats.total_size;
    
    // Used memory
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Used:", x + 2, y + 2);
    
    char mem_str[32];
    utoa((uint32_t)stats.used_size / 1024, mem_str, 10);
    strcat(mem_str, " KB (");
    char pct_str[8];
    itoa(used_percent, pct_str, 10);
    strcat(mem_str, pct_str);
    strcat(mem_str, "%)");
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at(mem_str, x + 9, y + 2);
    
    // Progress bar
    draw_progress_bar(x + 2, y + 3, w - 4, used_percent, 100,
                     used_percent < 60 ? VGA_COLOR_GREEN :
                     (used_percent < 85 ? VGA_COLOR_BROWN : VGA_COLOR_RED));
    
    // Free memory
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Free:", x + 2, y + 5);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    utoa((uint32_t)stats.free_size / 1024, mem_str, 10);
    strcat(mem_str, " KB");
    vga_puts_at(mem_str, x + 9, y + 5);
    
    // Total memory
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Total:", x + 2, y + 6);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    utoa((uint32_t)stats.total_size / 1024, mem_str, 10);
    strcat(mem_str, " KB");
    vga_puts_at(mem_str, x + 9, y + 6);
    
    // Block info
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Blocks:", x + 2, y + 8);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    utoa((uint32_t)stats.block_count, mem_str, 10);
    strcat(mem_str, " (");
    char free_str[8];
    utoa((uint32_t)stats.free_block_count, free_str, 10);
    strcat(mem_str, free_str);
    strcat(mem_str, " free)");
    vga_puts_at(mem_str, x + 10, y + 8);
}

static void draw_disk_panel(void) {
    int x = 1, y = 12, w = 39, h = 6;
    draw_box(x, y, w, h, "Storage");
    
    disk_manager_t* mgr = disk_get_manager();
    
    int row = 0;
    for (int i = 0; i < mgr->disk_count && row < 3; i++) {
        disk_info_t* disk = &mgr->disks[i];
        if (!disk->present) continue;
        
        vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        
        // Disk icon and name
        char label[32];
        strcpy(label, "Disk ");
        char num[4];
        itoa(i, num, 10);
        strcat(label, num);
        strcat(label, ":");
        vga_puts_at(label, x + 2, y + 2 + row);
        
        // Size
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        char size_str[16];
        disk_format_size(disk->size_bytes, size_str, sizeof(size_str));
        vga_puts_at(size_str, x + 10, y + 2 + row);
        
        // Type
        const char* type = disk->type == DISK_TYPE_VIRTUAL ? "Virtual" : "ATA";
        vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        vga_puts_at(type, x + 22, y + 2 + row);
        
        row++;
    }
    
    if (mgr->disk_count == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        vga_puts_at("No disks detected", x + 8, y + 3);
    }
}

static void draw_network_panel(void) {
    int x = 41, y = 12, w = 38, h = 6;
    draw_box(x, y, w, h, "Network");
    
    network_manager_t* net = network_get_manager();
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Status:", x + 2, y + 2);
    
    switch (net->status) {
        case NET_STATUS_DISABLED:
            vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
            vga_puts_at("Disabled", x + 10, y + 2);
            break;
        case NET_STATUS_DISCONNECTED:
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
            vga_puts_at("Disconnected", x + 10, y + 2);
            break;
        case NET_STATUS_CONNECTING:
            vga_set_color(vga_entry_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK));
            vga_puts_at("Connecting...", x + 10, y + 2);
            break;
        case NET_STATUS_CONNECTED:
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            vga_puts_at("Connected", x + 10, y + 2);
            break;
        default:
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
            vga_puts_at("Error", x + 10, y + 2);
            break;
    }
    
    if (net->status == NET_STATUS_CONNECTED) {
        vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        vga_puts_at("SSID:", x + 2, y + 3);
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts_at(net->config.connected_ssid, x + 10, y + 3);
        
        // Signal
        if (net->selected_network >= 0) {
            vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
            vga_puts_at("Signal:", x + 2, y + 4);
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            vga_puts_at(network_signal_bars(net->networks[net->selected_network].signal_strength),
                       x + 10, y + 4);
        }
    }
}

static void draw_audio_panel(void) {
    int x = 1, y = 18, w = 78, h = 5;
    draw_box(x, y, w, h, "Audio");
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Status:", x + 2, y + 2);
    
    if (audio_is_enabled()) {
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        vga_puts_at("Enabled", x + 10, y + 2);
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        vga_puts_at("Disabled", x + 10, y + 2);
    }
    
    // Volume
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Volume:", x + 25, y + 2);
    
    int volume = audio_get_volume();
    draw_progress_bar(x + 33, y + 2, 20, volume, 100, VGA_COLOR_GREEN);
    
    char vol_str[8];
    itoa(volume, vol_str, 10);
    strcat(vol_str, "%");
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at(vol_str, x + 54, y + 2);
    
    // Output
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Output:", x + 2, y + 3);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("PC Speaker (Internal)", x + 10, y + 3);
}

void sysmon_init(void) {
    refresh_counter = 0;
    history_index = 0;
    
    // Initialize history
    for (int i = 0; i < 40; i++) {
        cpu_history[i] = 0;
    }
}

void sysmon_redraw(void) {
    vga_clear();
    draw_titlebar();
    draw_cpu_panel();
    draw_memory_panel();
    draw_disk_panel();
    draw_network_panel();
    draw_audio_panel();
    draw_statusbar();
}

void sysmon_run(void) {
    sysmon_init();
    sysmon_redraw();
    
    bool running = true;
    int auto_refresh = 0;
    
    while (running) {
        // Check for key without blocking
        key_event_t event;
        if (keyboard_try_get_key(&event)) {
            if (event.released) continue;
            
            switch (event.scancode) {
                case KEY_ESCAPE:
                    running = false;
                    break;
                    
                default:
                    if (event.ascii == 'r' || event.ascii == 'R') {
                        sysmon_redraw();
                    }
                    break;
            }
        }
        
        // Auto-refresh simulation (crude timing)
        auto_refresh++;
        if (auto_refresh > 50000) {
            auto_refresh = 0;
            
            // Simulate network activity
            network_simulate_activity();
            
            // Redraw
            sysmon_redraw();
        }
    }
}
