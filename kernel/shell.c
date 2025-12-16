#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "memory.h"
#include "io.h"
#include "disk.h"
#include "network.h"
#include "gui.h"
#include "audio.h"
#include "apps/notepad.h"
#include "apps/browser.h"
#include "apps/diskmgr.h"
#include "apps/settings.h"
#include "apps/sysmon.h"

// Current application
static app_type_t current_app = APP_SHELL;

// Command buffer
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];

void shell_init(void) {
    current_app = APP_SHELL;
    shell_refresh();
}

void shell_draw_titlebar(const char* title) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    // Fill title bar
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, 0);
    }
    
    // Center title
    int title_len = strlen(title);
    int start_x = (VGA_WIDTH - title_len) / 2;
    for (int i = 0; title[i]; i++) {
        vga_putchar_at(title[i], start_x + i, 0);
    }
    
    // Time placeholder on right
    const char* time_str = "MiniOS v1.0";
    int time_len = strlen(time_str);
    for (int i = 0; time_str[i]; i++) {
        vga_putchar_at(time_str[i], VGA_WIDTH - time_len - 1 + i, 0);
    }
    
    vga_set_color(old_color);
}

void shell_draw_statusbar(const char* status) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    
    // Fill status bar
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, VGA_HEIGHT - 1);
    }
    
    // Status text
    for (int i = 0; status[i] && i < VGA_WIDTH - 1; i++) {
        vga_putchar_at(status[i], i + 1, VGA_HEIGHT - 1);
    }
    
    vga_set_color(old_color);
}

void shell_refresh(void) {
    vga_clear();
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    shell_draw_titlebar("MiniOS Shell");
    shell_draw_statusbar("F1:Notepad F2:Browser F3:Disk F4:Settings F5:SysMon | help: Commands");
    
    vga_set_cursor(0, 2);
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    vga_puts("Welcome to MiniOS!\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    vga_puts("Type 'help' for available commands.\n\n");
}

void shell_show_help(void) {
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts("\n=== Available Commands ===\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_puts("  help     - Show this help message\n");
    vga_puts("  clear    - Clear the screen\n");
    vga_puts("  notepad  - Open text editor\n");
    vga_puts("  browser  - Open web browser\n");
    vga_puts("  diskmgr  - Open disk manager\n");
    vga_puts("  settings - Open settings\n");
    vga_puts("  sysmon   - Open system monitor\n");
    vga_puts("  meminfo  - Show memory information\n");
    vga_puts("  diskinfo - Show disk information\n");
    vga_puts("  netinfo  - Show network information\n");
    vga_puts("  wifi     - WiFi control (on/off/scan/list)\n");
    vga_puts("  about    - About MiniOS\n");
    vga_puts("  reboot   - Reboot the system\n");
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts("\n=== Shortcuts ===\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_puts("  F1       - Open Notepad\n");
    vga_puts("  F2       - Open Browser\n");
    vga_puts("  F3       - Open Disk Manager\n");
    vga_puts("  F4       - Open Settings\n");
    vga_puts("  F5       - Open System Monitor\n");
    vga_puts("  ESC      - Return to Shell\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    vga_putchar('\n');
}

void shell_process_command(const char* command) {
    // Skip leading whitespace
    while (*command == ' ') command++;
    
    if (strlen(command) == 0) {
        return;
    }
    
    if (strcmp(command, "help") == 0) {
        shell_show_help();
    }
    else if (strcmp(command, "clear") == 0) {
        shell_refresh();
    }
    else if (strcmp(command, "notepad") == 0) {
        shell_switch_app(APP_NOTEPAD);
    }
    else if (strcmp(command, "browser") == 0) {
        shell_switch_app(APP_BROWSER);
    }
    else if (strcmp(command, "diskmgr") == 0) {
        shell_switch_app(APP_DISKMGR);
    }
    else if (strcmp(command, "settings") == 0) {
        shell_switch_app(APP_SETTINGS);
    }
    else if (strcmp(command, "sysmon") == 0) {
        shell_switch_app(APP_SYSMON);
    }
    else if (strcmp(command, "meminfo") == 0) {
        memory_stats_t stats;
        memory_get_stats(&stats);
        
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts("\n=== Memory Information ===\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        vga_printf("  Total Heap:     %u bytes\n", (uint32_t)stats.total_size);
        vga_printf("  Free:           %u bytes\n", (uint32_t)stats.free_size);
        vga_printf("  Used:           %u bytes\n", (uint32_t)stats.used_size);
        vga_printf("  Largest Free:   %u bytes\n", (uint32_t)stats.largest_free);
        vga_printf("  Block Count:    %u\n", (uint32_t)stats.block_count);
        vga_printf("  Free Blocks:    %u\n", (uint32_t)stats.free_block_count);
        vga_printf("  Allocations:    %u\n", (uint32_t)stats.alloc_count);
        vga_printf("  Frees:          %u\n", (uint32_t)stats.free_count);
        vga_printf("  Failed Allocs:  %u\n", (uint32_t)stats.failed_allocs);
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        vga_putchar('\n');
    }
    else if (strcmp(command, "diskinfo") == 0) {
        disk_manager_t* mgr = disk_get_manager();
        
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts("\n=== Disk Information ===\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        vga_printf("  Detected Disks: %d\n\n", mgr->disk_count);
        
        for (int i = 0; i < mgr->disk_count; i++) {
            disk_info_t* disk = &mgr->disks[i];
            if (!disk->present) continue;
            
            char size_str[16];
            disk_format_size(disk->size_bytes, size_str, sizeof(size_str));
            
            vga_printf("  Disk %d: %s\n", i, disk->model);
            vga_printf("    Type: %s, Size: %s\n", 
                      disk->type == DISK_TYPE_VIRTUAL ? "Virtual" : "ATA",
                      size_str);
        }
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        vga_putchar('\n');
    }
    else if (strcmp(command, "netinfo") == 0) {
        network_manager_t* net = network_get_manager();
        
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts("\n=== Network Information ===\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        vga_printf("  WiFi: %s\n", net->config.enabled ? "Enabled" : "Disabled");
        vga_printf("  Status: %s\n", network_status_string(net->status));
        
        if (net->status == NET_STATUS_CONNECTED) {
            char ip[16];
            network_get_ip_string(ip, sizeof(ip));
            vga_printf("  Connected to: %s\n", net->config.connected_ssid);
            vga_printf("  IP Address: %s\n", ip);
            
            if (net->selected_network >= 0) {
                vga_printf("  Signal: %s\n", 
                          network_signal_bars(net->networks[net->selected_network].signal_strength));
            }
        }
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        vga_putchar('\n');
    }
    else if (strcmp(command, "wifi on") == 0) {
        network_set_enabled(true);
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        vga_puts("WiFi enabled.\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    else if (strcmp(command, "wifi off") == 0) {
        network_set_enabled(false);
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        vga_puts("WiFi disabled.\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    else if (strcmp(command, "wifi scan") == 0) {
        if (!network_is_enabled()) {
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
            vga_puts("WiFi is disabled. Use 'wifi on' first.\n");
        } else {
            vga_puts("Scanning for networks...\n");
            network_start_scan();
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            vga_printf("Found %d networks. Use 'wifi list' to see them.\n", 
                      network_get_manager()->network_count);
        }
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    else if (strcmp(command, "wifi list") == 0) {
        network_manager_t* net = network_get_manager();
        if (!net->config.enabled) {
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
            vga_puts("WiFi is disabled.\n");
        } else if (net->network_count == 0) {
            vga_puts("No networks found. Use 'wifi scan' first.\n");
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            vga_puts("\n=== Available Networks ===\n");
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
            
            for (int i = 0; i < net->network_count; i++) {
                wifi_network_t* wifi = &net->networks[i];
                vga_printf("  %d. %s %s %s%s\n", 
                          i + 1,
                          network_signal_bars(wifi->signal_strength),
                          wifi->ssid,
                          network_security_string(wifi->security),
                          wifi->saved ? " *" : "");
            }
        }
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        vga_putchar('\n');
    }
    else if (strncmp(command, "wifi connect ", 13) == 0) {
        const char* ssid = command + 13;
        if (network_connect(ssid, "password123")) {
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            vga_printf("Connected to %s\n", ssid);
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
            vga_printf("Failed to connect to %s\n", ssid);
        }
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    else if (strcmp(command, "wifi disconnect") == 0) {
        if (network_disconnect()) {
            vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            vga_puts("Disconnected from network.\n");
        } else {
            vga_puts("Not connected to any network.\n");
        }
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    else if (strcmp(command, "about") == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
        vga_puts("\n");
        vga_puts("  __  __ _       _  ___  ____  \n");
        vga_puts(" |  \\/  (_)_ __ (_)/ _ \\/ ___| \n");
        vga_puts(" | |\\/| | | '_ \\| | | | \\___ \\ \n");
        vga_puts(" | |  | | | | | | | |_| |___) |\n");
        vga_puts(" |_|  |_|_|_| |_|_|\\___/|____/ \n");
        vga_puts("\n");
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts("  MiniOS v1.0\n");
        vga_puts("  A simple operating system from scratch\n");
        vga_puts("  Features: Shell, Notepad, Browser, Disk Manager, Settings, SysMon\n\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    else if (strcmp(command, "reboot") == 0) {
        vga_puts("Rebooting...\n");
        // Trigger triple fault to reboot
        uint8_t good = 0x02;
        while (good & 0x02) {
            good = inb(0x64);
        }
        outb(0x64, 0xFE);
    }
    else if (strncmp(command, "echo ", 5) == 0) {
        vga_puts(command + 5);
        vga_putchar('\n');
    }
    else {
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        vga_printf("Unknown command: %s\n", command);
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        vga_puts("Type 'help' for available commands.\n");
    }
}

app_type_t shell_get_current_app(void) {
    return current_app;
}

void shell_switch_app(app_type_t app) {
    current_app = app;
    
    switch (app) {
        case APP_NOTEPAD:
            notepad_init();
            notepad_run();
            break;
        case APP_BROWSER:
            browser_init();
            browser_run();
            break;
        case APP_DISKMGR:
            diskmgr_init();
            diskmgr_run();
            break;
        case APP_SETTINGS:
            settings_init();
            settings_run();
            break;
        case APP_SYSMON:
            sysmon_init();
            sysmon_run();
            break;
        case APP_SHELL:
        default:
            shell_refresh();
            break;
    }
}

void shell_run(void) {
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    vga_puts("minios> ");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    while (1) {
        key_event_t event;
        
        if (keyboard_try_get_key(&event)) {
            if (event.released) continue;
            
            // Check for function keys
            switch (event.scancode) {
                case KEY_F1:
                    shell_switch_app(APP_NOTEPAD);
                    current_app = APP_SHELL;
                    shell_refresh();
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                    vga_puts("minios> ");
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    continue;
                    
                case KEY_F2:
                    shell_switch_app(APP_BROWSER);
                    current_app = APP_SHELL;
                    shell_refresh();
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                    vga_puts("minios> ");
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    continue;
                    
                case KEY_F3:
                    shell_switch_app(APP_DISKMGR);
                    current_app = APP_SHELL;
                    shell_refresh();
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                    vga_puts("minios> ");
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    continue;
                    
                case KEY_F4:
                    shell_switch_app(APP_SETTINGS);
                    current_app = APP_SHELL;
                    shell_refresh();
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                    vga_puts("minios> ");
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    continue;
                    
                case KEY_F5:
                    shell_switch_app(APP_SYSMON);
                    current_app = APP_SHELL;
                    shell_refresh();
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                    vga_puts("minios> ");
                    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                    continue;
            }
        }
        
        // Read command
        keyboard_readline(cmd_buffer, CMD_BUFFER_SIZE);
        
        // Process command
        shell_process_command(cmd_buffer);
        
        // Show prompt again
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        vga_puts("minios> ");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
}
