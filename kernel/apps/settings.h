#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

// Settings categories
typedef enum {
    SETTINGS_CAT_DISPLAY = 0,
    SETTINGS_CAT_AUDIO,
    SETTINGS_CAT_NETWORK,
    SETTINGS_CAT_SYSTEM,
    SETTINGS_CAT_ABOUT,
    SETTINGS_CAT_COUNT
} settings_category_t;

// Initialize settings app
void settings_init(void);

// Run settings app
void settings_run(void);

// Redraw settings
void settings_redraw(void);

#endif // SETTINGS_H
