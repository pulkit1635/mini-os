#include "settings.h"
#include "../vga.h"
#include "../keyboard.h"
#include "../string.h"
#include "../memory.h"
#include "../audio.h"
#include "../network.h"
#include "../gui.h"

// Current state
static settings_category_t current_category = SETTINGS_CAT_DISPLAY;
static int selected_item = 0;

// Content area
#define SIDEBAR_WIDTH 20
#define CONTENT_X (SIDEBAR_WIDTH + 1)
#define CONTENT_Y 2
#define CONTENT_WIDTH (VGA_WIDTH - SIDEBAR_WIDTH - 2)
#define CONTENT_HEIGHT (VGA_HEIGHT - 4)

static const char* category_names[] = {
    "Display",
    "Audio",
    "Network",
    "System",
    "About"
};

static void draw_titlebar(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, 0);
    }
    
    const char* title = "[ Settings ]";
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
    
    const char* status = " Tab: Categories | Up/Down: Select | Enter: Change | ESC: Exit";
    for (int i = 0; status[i] && i < VGA_WIDTH - 1; i++) {
        vga_putchar_at(status[i], i, VGA_HEIGHT - 1);
    }
    
    vga_set_color(old_color);
}

static void draw_sidebar(void) {
    // Sidebar background
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_DARK_GREY));
    for (int y = 1; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < SIDEBAR_WIDTH; x++) {
            vga_putchar_at(' ', x, y);
        }
    }
    
    // Category items
    for (int i = 0; i < SETTINGS_CAT_COUNT; i++) {
        if (i == (int)current_category) {
            vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
        }
        
        // Clear line
        for (int x = 1; x < SIDEBAR_WIDTH - 1; x++) {
            vga_putchar_at(' ', x, CONTENT_Y + i * 2);
        }
        
        // Draw category name
        vga_puts_at(category_names[i], 2, CONTENT_Y + i * 2);
    }
}

static void clear_content(void) {
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int y = 1; y < VGA_HEIGHT - 1; y++) {
        for (int x = CONTENT_X; x < VGA_WIDTH; x++) {
            vga_putchar_at(' ', x, y);
        }
    }
}

static void draw_display_settings(void) {
    int y = CONTENT_Y;
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("Display Settings", CONTENT_X + 2, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    for (int x = CONTENT_X + 2; x < VGA_WIDTH - 2; x++) {
        vga_putchar_at('-', x, y);
    }
    y += 2;
    
    // Display info
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Resolution:", CONTENT_X + 2, y);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("80x25 (Text Mode)", CONTENT_X + 20, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Color Depth:", CONTENT_X + 2, y);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("16 colors", CONTENT_X + 20, y);
    y += 2;
    
    // Color preview
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Available Colors:", CONTENT_X + 2, y);
    y += 1;
    
    for (int i = 0; i < 16; i++) {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, (vga_color_t)i));
        vga_putchar_at(' ', CONTENT_X + 2 + i * 3, y);
        vga_putchar_at(' ', CONTENT_X + 3 + i * 3, y);
    }
}

static void draw_audio_settings(void) {
    int y = CONTENT_Y;
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("Audio Settings", CONTENT_X + 2, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    for (int x = CONTENT_X + 2; x < VGA_WIDTH - 2; x++) {
        vga_putchar_at('-', x, y);
    }
    y += 2;
    
    // Audio status
    bool enabled = audio_is_enabled();
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Audio:", CONTENT_X + 2, y);
    
    if (selected_item == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    vga_puts_at(enabled ? "[X] Enabled " : "[ ] Enabled ", CONTENT_X + 20, y);
    y += 2;
    
    // Volume
    int volume = audio_get_volume();
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Volume:", CONTENT_X + 2, y);
    
    if (selected_item == 1) {
        vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    
    // Draw volume bar
    vga_putchar_at('[', CONTENT_X + 20, y);
    for (int i = 0; i < 20; i++) {
        if (i < volume / 5) {
            vga_putchar_at('#', CONTENT_X + 21 + i, y);
        } else {
            vga_putchar_at('-', CONTENT_X + 21 + i, y);
        }
    }
    vga_putchar_at(']', CONTENT_X + 41, y);
    
    char vol_str[8];
    itoa(volume, vol_str, 10);
    strcat(vol_str, "%");
    vga_puts_at(vol_str, CONTENT_X + 43, y);
    y += 2;
    
    // Test sound
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Test:", CONTENT_X + 2, y);
    
    if (selected_item == 2) {
        vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    vga_puts_at("[ Play Test Sound ]", CONTENT_X + 20, y);
}

static void draw_network_settings(void) {
    int y = CONTENT_Y;
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("Network Settings", CONTENT_X + 2, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    for (int x = CONTENT_X + 2; x < VGA_WIDTH - 2; x++) {
        vga_putchar_at('-', x, y);
    }
    y += 2;
    
    network_manager_t* net = network_get_manager();
    
    // WiFi toggle
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("WiFi:", CONTENT_X + 2, y);
    
    if (selected_item == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    vga_puts_at(net->config.enabled ? "[X] Enabled " : "[ ] Enabled ", CONTENT_X + 20, y);
    y += 2;
    
    // Connection status
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Status:", CONTENT_X + 2, y);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at(network_status_string(net->status), CONTENT_X + 20, y);
    y += 2;
    
    if (net->status == NET_STATUS_CONNECTED) {
        // Connected network
        vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        vga_puts_at("Network:", CONTENT_X + 2, y);
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        vga_puts_at(net->config.connected_ssid, CONTENT_X + 20, y);
        y += 2;
        
        // IP address
        vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        vga_puts_at("IP Address:", CONTENT_X + 2, y);
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        char ip[16];
        network_get_ip_string(ip, sizeof(ip));
        vga_puts_at(ip, CONTENT_X + 20, y);
        y += 2;
        
        // Signal
        if (net->selected_network >= 0 && net->selected_network < net->network_count) {
            vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
            vga_puts_at("Signal:", CONTENT_X + 2, y);
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            vga_puts_at(network_signal_bars(net->networks[net->selected_network].signal_strength), 
                       CONTENT_X + 20, y);
        }
        y += 2;
        
        // Disconnect button
        if (selected_item == 1) {
            vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
        vga_puts_at("[ Disconnect ]", CONTENT_X + 20, y);
    } else if (net->config.enabled) {
        // Available networks
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts_at("Available Networks:", CONTENT_X + 2, y);
        y += 1;
        
        for (int i = 0; i < net->network_count && i < 5; i++) {
            wifi_network_t* wifi = &net->networks[i];
            
            if (selected_item == i + 1) {
                vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
            } else {
                vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            }
            
            // Clear line
            for (int x = CONTENT_X + 2; x < VGA_WIDTH - 2; x++) {
                vga_putchar_at(' ', x, y);
            }
            
            vga_puts_at(network_signal_bars(wifi->signal_strength), CONTENT_X + 4, y);
            vga_puts_at(wifi->ssid, CONTENT_X + 12, y);
            
            const char* sec = network_security_string(wifi->security);
            vga_puts_at(sec, CONTENT_X + 40, y);
            
            if (wifi->saved) {
                vga_puts_at("*", CONTENT_X + 50, y);
            }
            
            y++;
        }
    }
}

static void draw_system_settings(void) {
    int y = CONTENT_Y;
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("System Settings", CONTENT_X + 2, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    for (int x = CONTENT_X + 2; x < VGA_WIDTH - 2; x++) {
        vga_putchar_at('-', x, y);
    }
    y += 2;
    
    // Memory info
    memory_stats_t stats;
    memory_get_stats(&stats);
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Memory Usage:", CONTENT_X + 2, y);
    y += 1;
    
    // Progress bar for memory
    int used_percent = (stats.used_size * 100) / stats.total_size;
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_putchar_at('[', CONTENT_X + 4, y);
    for (int i = 0; i < 30; i++) {
        if (i < used_percent * 30 / 100) {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_GREEN));
        } else {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
        }
        vga_putchar_at(' ', CONTENT_X + 5 + i, y);
    }
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_putchar_at(']', CONTENT_X + 35, y);
    
    char mem_str[32];
    utoa((uint32_t)stats.used_size / 1024, mem_str, 10);
    strcat(mem_str, " KB / ");
    char total_str[16];
    utoa((uint32_t)stats.total_size / 1024, total_str, 10);
    strcat(mem_str, total_str);
    strcat(mem_str, " KB");
    vga_puts_at(mem_str, CONTENT_X + 37, y);
    y += 3;
    
    // Allocations
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Allocations:", CONTENT_X + 2, y);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    utoa((uint32_t)stats.alloc_count, mem_str, 10);
    vga_puts_at(mem_str, CONTENT_X + 20, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Free Operations:", CONTENT_X + 2, y);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    utoa((uint32_t)stats.free_count, mem_str, 10);
    vga_puts_at(mem_str, CONTENT_X + 20, y);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Failed Allocs:", CONTENT_X + 2, y);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    utoa((uint32_t)stats.failed_allocs, mem_str, 10);
    vga_puts_at(mem_str, CONTENT_X + 20, y);
    y += 3;
    
    // Memory integrity check
    if (selected_item == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    vga_puts_at("[ Check Memory Integrity ]", CONTENT_X + 4, y);
    y += 2;
    
    // Defragment
    if (selected_item == 1) {
        vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
    } else {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    vga_puts_at("[ Defragment Memory ]", CONTENT_X + 4, y);
}

static void draw_about(void) {
    int y = CONTENT_Y;
    
    // Logo
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("  __  __ _       _  ___  ____  ", CONTENT_X + 8, y++);
    vga_puts_at(" |  \\/  (_)_ __ (_)/ _ \\/ ___| ", CONTENT_X + 8, y++);
    vga_puts_at(" | |\\/| | | '_ \\| | | | \\___ \\ ", CONTENT_X + 8, y++);
    vga_puts_at(" | |  | | | | | | | |_| |___) |", CONTENT_X + 8, y++);
    vga_puts_at(" |_|  |_|_|_| |_|_|\\___/|____/ ", CONTENT_X + 8, y++);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("MiniOS Version 1.0", CONTENT_X + 15, y++);
    y++;
    
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    vga_puts_at("A simple operating system from scratch", CONTENT_X + 5, y++);
    y += 2;
    
    vga_set_color(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    vga_puts_at("Features:", CONTENT_X + 2, y++);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts_at("* VGA Text Mode Display", CONTENT_X + 4, y++);
    vga_puts_at("* PS/2 Keyboard Input", CONTENT_X + 4, y++);
    vga_puts_at("* Memory Management", CONTENT_X + 4, y++);
    vga_puts_at("* Disk Manager", CONTENT_X + 4, y++);
    vga_puts_at("* Network Simulation", CONTENT_X + 4, y++);
    vga_puts_at("* Audio (PC Speaker)", CONTENT_X + 4, y++);
    vga_puts_at("* Text Editor (Notepad)", CONTENT_X + 4, y++);
    vga_puts_at("* Web Browser", CONTENT_X + 4, y++);
}

static void draw_content(void) {
    clear_content();
    
    switch (current_category) {
        case SETTINGS_CAT_DISPLAY:
            draw_display_settings();
            break;
        case SETTINGS_CAT_AUDIO:
            draw_audio_settings();
            break;
        case SETTINGS_CAT_NETWORK:
            draw_network_settings();
            break;
        case SETTINGS_CAT_SYSTEM:
            draw_system_settings();
            break;
        case SETTINGS_CAT_ABOUT:
            draw_about();
            break;
        default:
            break;
    }
}

void settings_init(void) {
    current_category = SETTINGS_CAT_DISPLAY;
    selected_item = 0;
}

void settings_redraw(void) {
    vga_clear();
    draw_titlebar();
    draw_sidebar();
    draw_content();
    draw_statusbar();
}

static int get_max_items(void) {
    switch (current_category) {
        case SETTINGS_CAT_AUDIO:
            return 3;
        case SETTINGS_CAT_NETWORK:
            return network_is_connected() ? 2 : network_get_manager()->network_count + 1;
        case SETTINGS_CAT_SYSTEM:
            return 2;
        default:
            return 0;
    }
}

void settings_run(void) {
    settings_init();
    settings_redraw();
    
    bool running = true;
    
    while (running) {
        key_event_t event = keyboard_get_key();
        if (event.released) continue;
        
        switch (event.scancode) {
            case KEY_ESCAPE:
                running = false;
                break;
                
            case KEY_TAB:
                // Switch category
                if (event.shift) {
                    current_category = (current_category + SETTINGS_CAT_COUNT - 1) % SETTINGS_CAT_COUNT;
                } else {
                    current_category = (current_category + 1) % SETTINGS_CAT_COUNT;
                }
                selected_item = 0;
                settings_redraw();
                break;
                
            case KEY_UP:
                if (selected_item > 0) {
                    selected_item--;
                    settings_redraw();
                }
                break;
                
            case KEY_DOWN:
                if (selected_item < get_max_items() - 1) {
                    selected_item++;
                    settings_redraw();
                }
                break;
                
            case KEY_LEFT:
                // Decrease value
                if (current_category == SETTINGS_CAT_AUDIO && selected_item == 1) {
                    int vol = audio_get_volume();
                    if (vol > 0) {
                        audio_set_volume(vol - 5);
                        settings_redraw();
                    }
                }
                break;
                
            case KEY_RIGHT:
                // Increase value
                if (current_category == SETTINGS_CAT_AUDIO && selected_item == 1) {
                    int vol = audio_get_volume();
                    if (vol < 100) {
                        audio_set_volume(vol + 5);
                        settings_redraw();
                    }
                }
                break;
                
            case KEY_ENTER:
                // Toggle/activate
                if (current_category == SETTINGS_CAT_AUDIO) {
                    if (selected_item == 0) {
                        audio_enable(!audio_is_enabled());
                        settings_redraw();
                    } else if (selected_item == 2) {
                        audio_play_effect(SFX_SUCCESS);
                    }
                } else if (current_category == SETTINGS_CAT_NETWORK) {
                    if (selected_item == 0) {
                        network_set_enabled(!network_is_enabled());
                        settings_redraw();
                    } else if (network_is_connected() && selected_item == 1) {
                        network_disconnect();
                        settings_redraw();
                    } else if (!network_is_connected() && selected_item > 0) {
                        // Try to connect
                        network_manager_t* net = network_get_manager();
                        int net_idx = selected_item - 1;
                        if (net_idx >= 0 && net_idx < net->network_count) {
                            wifi_network_t* wifi = &net->networks[net_idx];
                            if (wifi->security == WIFI_SEC_NONE) {
                                network_connect(wifi->ssid, NULL);
                            } else {
                                gui_message_box("Connect", "Password required (demo: use 'password')");
                                network_connect(wifi->ssid, "password123");
                            }
                            settings_redraw();
                        }
                    }
                } else if (current_category == SETTINGS_CAT_SYSTEM) {
                    if (selected_item == 0) {
                        bool valid = memory_validate();
                        gui_message_box("Memory Check", 
                            valid ? "Memory integrity OK!" : "Memory corruption detected!");
                        settings_redraw();
                    } else if (selected_item == 1) {
                        memory_defragment();
                        gui_message_box("Defragment", "Memory defragmented!");
                        settings_redraw();
                    }
                }
                break;
                
            default:
                break;
        }
    }
}
