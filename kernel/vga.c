#include "vga.h"
#include "io.h"
#include "string.h"
#include <stdarg.h>

// VGA buffer address
static uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;

// VGA state
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color;

void vga_init(void) {
    current_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    vga_enable_cursor(14, 15);
}

void vga_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', current_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor(cursor_x, cursor_y);
}

void vga_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    vga_update_cursor(cursor_x, cursor_y);
}

int vga_get_cursor_x(void) {
    return cursor_x;
}

int vga_get_cursor_y(void) {
    return cursor_y;
}

void vga_set_color(uint8_t color) {
    current_color = color;
}

uint8_t vga_get_color(void) {
    return current_color;
}

void vga_scroll(void) {
    // Move all lines up by one
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = VGA_BUFFER[(y + 1) * VGA_WIDTH + x];
        }
    }
    // Clear the last line
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', current_color);
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', current_color);
        }
    } else {
        VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, current_color);
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }
    
    vga_update_cursor(cursor_x, cursor_y);
}

void vga_putchar_at(char c, int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        VGA_BUFFER[y * VGA_WIDTH + x] = vga_entry(c, current_color);
    }
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_puts_at(const char* str, int x, int y) {
    int orig_x = cursor_x;
    int orig_y = cursor_y;
    cursor_x = x;
    cursor_y = y;
    vga_puts(str);
    cursor_x = orig_x;
    cursor_y = orig_y;
}

void vga_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[256];
    int i = 0;
    
    while (*format && i < 255) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int val = va_arg(args, int);
                    char num_buf[32];
                    itoa(val, num_buf, 10);
                    for (char* p = num_buf; *p; p++) {
                        vga_putchar(*p);
                    }
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    char num_buf[32];
                    utoa(val, num_buf, 10);
                    for (char* p = num_buf; *p; p++) {
                        vga_putchar(*p);
                    }
                    break;
                }
                case 'x':
                case 'X': {
                    unsigned int val = va_arg(args, unsigned int);
                    char num_buf[32];
                    utoa(val, num_buf, 16);
                    for (char* p = num_buf; *p; p++) {
                        vga_putchar(*format == 'X' ? toupper(*p) : *p);
                    }
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str) {
                        vga_puts(str);
                    } else {
                        vga_puts("(null)");
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putchar(c);
                    break;
                }
                case '%': {
                    vga_putchar('%');
                    break;
                }
                default:
                    vga_putchar('%');
                    vga_putchar(*format);
                    break;
            }
        } else {
            vga_putchar(*format);
        }
        format++;
    }
    
    va_end(args);
}

void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_draw_box(int x, int y, int width, int height, uint8_t color) {
    uint8_t old_color = current_color;
    current_color = color;
    
    // Top border
    vga_putchar_at('+', x, y);
    for (int i = 1; i < width - 1; i++) {
        vga_putchar_at('-', x + i, y);
    }
    vga_putchar_at('+', x + width - 1, y);
    
    // Side borders
    for (int j = 1; j < height - 1; j++) {
        vga_putchar_at('|', x, y + j);
        vga_putchar_at('|', x + width - 1, y + j);
    }
    
    // Bottom border
    vga_putchar_at('+', x, y + height - 1);
    for (int i = 1; i < width - 1; i++) {
        vga_putchar_at('-', x + i, y + height - 1);
    }
    vga_putchar_at('+', x + width - 1, y + height - 1);
    
    current_color = old_color;
}

void vga_fill_area(int x, int y, int width, int height, char c, uint8_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            if (x + i < VGA_WIDTH && y + j < VGA_HEIGHT) {
                VGA_BUFFER[(y + j) * VGA_WIDTH + (x + i)] = vga_entry(c, color);
            }
        }
    }
}

uint16_t vga_get_entry_at(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return VGA_BUFFER[y * VGA_WIDTH + x];
    }
    return 0;
}

uint16_t* vga_get_buffer(void) {
    return VGA_BUFFER;
}
