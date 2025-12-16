#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>

// Application types
typedef enum {
    APP_NONE = 0,
    APP_SHELL,
    APP_NOTEPAD,
    APP_BROWSER,
    APP_DISKMGR,
    APP_SETTINGS,
    APP_SYSMON
} app_type_t;

// Initialize shell
void shell_init(void);

// Run shell main loop
void shell_run(void);

// Get current running application
app_type_t shell_get_current_app(void);

// Switch to application
void shell_switch_app(app_type_t app);

// Draw title bar
void shell_draw_titlebar(const char* title);

// Draw status bar
void shell_draw_statusbar(const char* status);

// Process shell command
void shell_process_command(const char* command);

// Show help
void shell_show_help(void);

// Clear screen and redraw shell
void shell_refresh(void);

#endif // SHELL_H
