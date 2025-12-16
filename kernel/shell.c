#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "memory.h"
#include "io.h"
#include "apps/notepad.h"
#include "apps/browser.h"

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
    shell_draw_statusbar("F1: Notepad | F2: Browser | help: Commands");
    
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
    vga_puts("  meminfo  - Show memory information\n");
    vga_puts("  about    - About MiniOS\n");
    vga_puts("  reboot   - Reboot the system\n");
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_puts("\n=== Shortcuts ===\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_puts("  F1       - Open Notepad\n");
    vga_puts("  F2       - Open Browser\n");
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
    else if (strcmp(command, "meminfo") == 0) {
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_puts("\n=== Memory Information ===\n");
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        vga_printf("  Total Heap: %u bytes\n", memory_get_total());
        vga_printf("  Free:       %u bytes\n", memory_get_free());
        vga_printf("  Used:       %u bytes\n", memory_get_total() - memory_get_free());
        vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        vga_putchar('\n');
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
        vga_puts("  Features: Shell, Notepad, Web Browser\n\n");
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
