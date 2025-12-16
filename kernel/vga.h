#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

// VGA text mode dimensions
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

// VGA colors
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

// Create VGA color byte
static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

// Create VGA character entry
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

// Initialize VGA
void vga_init(void);

// Clear screen
void vga_clear(void);

// Set cursor position
void vga_set_cursor(int x, int y);

// Get cursor X position
int vga_get_cursor_x(void);

// Get cursor Y position
int vga_get_cursor_y(void);

// Set text color
void vga_set_color(uint8_t color);

// Get current color
uint8_t vga_get_color(void);

// Put character at current position
void vga_putchar(char c);

// Put character at specific position
void vga_putchar_at(char c, int x, int y);

// Put string at current position
void vga_puts(const char* str);

// Put string at specific position
void vga_puts_at(const char* str, int x, int y);

// Print formatted string (basic printf)
void vga_printf(const char* format, ...);

// Scroll screen up by one line
void vga_scroll(void);

// Enable/disable cursor
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void vga_disable_cursor(void);

// Update hardware cursor position
void vga_update_cursor(int x, int y);

// Draw a box
void vga_draw_box(int x, int y, int width, int height, uint8_t color);

// Fill area with character
void vga_fill_area(int x, int y, int width, int height, char c, uint8_t color);

// Get character at position
uint16_t vga_get_entry_at(int x, int y);

// Direct screen buffer access
uint16_t* vga_get_buffer(void);

#endif // VGA_H
