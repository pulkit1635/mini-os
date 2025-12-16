#include "notepad.h"
#include "../vga.h"
#include "../keyboard.h"
#include "../string.h"
#include "../memory.h"
#include "../io.h"

// Document buffer
static char document[NOTEPAD_MAX_SIZE];
static int doc_length = 0;

// Line information
static int line_starts[NOTEPAD_MAX_LINES];
static int line_count = 0;

// Cursor position (in document)
static int cursor_pos = 0;
static int cursor_x = 0;
static int cursor_y = 0;

// View offset (for scrolling)
static int view_offset_y = 0;

// Editor area
#define EDIT_START_Y 2
#define EDIT_END_Y (VGA_HEIGHT - 2)
#define EDIT_HEIGHT (EDIT_END_Y - EDIT_START_Y)

// Recalculate line starts
static void recalc_lines(void) {
    line_count = 1;
    line_starts[0] = 0;
    
    for (int i = 0; i < doc_length && line_count < NOTEPAD_MAX_LINES; i++) {
        if (document[i] == '\n') {
            line_starts[line_count++] = i + 1;
        }
    }
}

// Get line length
static int get_line_length(int line) {
    if (line < 0 || line >= line_count) return 0;
    
    int start = line_starts[line];
    int end = (line + 1 < line_count) ? line_starts[line + 1] - 1 : doc_length;
    
    return end - start;
}

// Update cursor position from document position
static void update_cursor_from_pos(void) {
    cursor_y = 0;
    cursor_x = cursor_pos;
    
    for (int i = 0; i < line_count; i++) {
        if (line_starts[i] <= cursor_pos) {
            cursor_y = i;
            cursor_x = cursor_pos - line_starts[i];
        } else {
            break;
        }
    }
    
    // Ensure cursor_x doesn't exceed line length
    int line_len = get_line_length(cursor_y);
    if (cursor_x > line_len) {
        cursor_x = line_len;
    }
}

// Update document position from cursor
static void update_pos_from_cursor(void) {
    if (cursor_y >= line_count) {
        cursor_y = line_count - 1;
    }
    if (cursor_y < 0) {
        cursor_y = 0;
    }
    
    int line_len = get_line_length(cursor_y);
    if (cursor_x > line_len) {
        cursor_x = line_len;
    }
    if (cursor_x < 0) {
        cursor_x = 0;
    }
    
    cursor_pos = line_starts[cursor_y] + cursor_x;
}

// Draw title bar
static void draw_titlebar(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, 0);
    }
    
    const char* title = "[ Notepad - Untitled ]";
    int title_len = strlen(title);
    int start_x = (VGA_WIDTH - title_len) / 2;
    for (int i = 0; title[i]; i++) {
        vga_putchar_at(title[i], start_x + i, 0);
    }
    
    vga_set_color(old_color);
}

// Draw status bar
static void draw_statusbar(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar_at(' ', x, VGA_HEIGHT - 1);
    }
    
    // Status info
    char status[80];
    itoa(cursor_y + 1, status, 10);
    strcat(status, ":");
    char col_str[16];
    itoa(cursor_x + 1, col_str, 10);
    strcat(status, col_str);
    strcat(status, " | ");
    char chars_str[16];
    itoa(doc_length, chars_str, 10);
    strcat(status, chars_str);
    strcat(status, " chars | ESC: Exit | Ctrl+N: New");
    
    for (int i = 0; status[i] && i < VGA_WIDTH - 1; i++) {
        vga_putchar_at(status[i], i + 1, VGA_HEIGHT - 1);
    }
    
    vga_set_color(old_color);
}

// Draw line numbers
static void draw_line_numbers(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    
    for (int screen_y = 0; screen_y < EDIT_HEIGHT; screen_y++) {
        int line = view_offset_y + screen_y;
        int y = EDIT_START_Y + screen_y;
        
        // Clear line number area
        for (int x = 0; x < 5; x++) {
            vga_putchar_at(' ', x, y);
        }
        
        if (line < line_count) {
            char num_str[8];
            itoa(line + 1, num_str, 10);
            int len = strlen(num_str);
            for (int i = 0; i < len; i++) {
                vga_putchar_at(num_str[i], 4 - len + i, y);
            }
        }
        
        // Separator
        vga_putchar_at('|', 5, y);
    }
    
    vga_set_color(old_color);
}

// Draw document content
static void draw_content(void) {
    uint8_t old_color = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    int content_start_x = 6;
    int content_width = VGA_WIDTH - content_start_x;
    
    for (int screen_y = 0; screen_y < EDIT_HEIGHT; screen_y++) {
        int line = view_offset_y + screen_y;
        int y = EDIT_START_Y + screen_y;
        
        // Clear content area
        for (int x = content_start_x; x < VGA_WIDTH; x++) {
            vga_putchar_at(' ', x, y);
        }
        
        if (line < line_count) {
            int line_start = line_starts[line];
            int line_len = get_line_length(line);
            
            for (int i = 0; i < line_len && i < content_width; i++) {
                char c = document[line_start + i];
                if (c >= 32 && c < 127) {
                    vga_putchar_at(c, content_start_x + i, y);
                }
            }
        }
    }
    
    vga_set_color(old_color);
}

// Update hardware cursor
static void update_hw_cursor(void) {
    int screen_y = cursor_y - view_offset_y;
    
    // Scroll if needed
    if (screen_y < 0) {
        view_offset_y = cursor_y;
        notepad_redraw();
        screen_y = 0;
    } else if (screen_y >= EDIT_HEIGHT) {
        view_offset_y = cursor_y - EDIT_HEIGHT + 1;
        notepad_redraw();
        screen_y = EDIT_HEIGHT - 1;
    }
    
    int hw_x = 6 + cursor_x;
    int hw_y = EDIT_START_Y + screen_y;
    
    if (hw_x >= VGA_WIDTH) hw_x = VGA_WIDTH - 1;
    
    vga_update_cursor(hw_x, hw_y);
}

void notepad_init(void) {
    notepad_clear();
}

void notepad_clear(void) {
    memset(document, 0, NOTEPAD_MAX_SIZE);
    doc_length = 0;
    cursor_pos = 0;
    cursor_x = 0;
    cursor_y = 0;
    view_offset_y = 0;
    line_count = 1;
    line_starts[0] = 0;
}

void notepad_new(void) {
    notepad_clear();
    notepad_redraw();
}

void notepad_redraw(void) {
    draw_titlebar();
    draw_statusbar();
    draw_line_numbers();
    draw_content();
    update_hw_cursor();
}

void notepad_insert_char(char c) {
    if (doc_length >= NOTEPAD_MAX_SIZE - 1) return;
    
    // Shift text after cursor
    for (int i = doc_length; i > cursor_pos; i--) {
        document[i] = document[i - 1];
    }
    
    document[cursor_pos] = c;
    doc_length++;
    cursor_pos++;
    
    recalc_lines();
    update_cursor_from_pos();
}

void notepad_delete_char(void) {
    if (cursor_pos >= doc_length) return;
    
    // Shift text after cursor
    for (int i = cursor_pos; i < doc_length - 1; i++) {
        document[i] = document[i + 1];
    }
    
    doc_length--;
    document[doc_length] = '\0';
    
    recalc_lines();
    update_cursor_from_pos();
}

void notepad_backspace(void) {
    if (cursor_pos <= 0) return;
    
    cursor_pos--;
    notepad_delete_char();
}

void notepad_move_cursor(int dx, int dy) {
    cursor_x += dx;
    cursor_y += dy;
    
    // Clamp Y
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= line_count) cursor_y = line_count - 1;
    
    // Clamp X
    int line_len = get_line_length(cursor_y);
    if (cursor_x < 0) {
        // Move to end of previous line
        if (cursor_y > 0) {
            cursor_y--;
            cursor_x = get_line_length(cursor_y);
        } else {
            cursor_x = 0;
        }
    } else if (cursor_x > line_len) {
        // Move to start of next line
        if (cursor_y < line_count - 1) {
            cursor_y++;
            cursor_x = 0;
        } else {
            cursor_x = line_len;
        }
    }
    
    update_pos_from_cursor();
}

void notepad_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    update_pos_from_cursor();
}

int notepad_get_cursor_x(void) {
    return cursor_x;
}

int notepad_get_cursor_y(void) {
    return cursor_y;
}

const char* notepad_get_content(void) {
    return document;
}

int notepad_get_line_count(void) {
    return line_count;
}

void notepad_run(void) {
    vga_clear();
    notepad_redraw();
    
    bool running = true;
    
    while (running) {
        key_event_t event = keyboard_get_key();
        
        if (event.released) continue;
        
        // Handle special keys
        switch (event.scancode) {
            case KEY_ESCAPE:
                running = false;
                break;
                
            case KEY_UP:
                notepad_move_cursor(0, -1);
                break;
                
            case KEY_DOWN:
                notepad_move_cursor(0, 1);
                break;
                
            case KEY_LEFT:
                notepad_move_cursor(-1, 0);
                break;
                
            case KEY_RIGHT:
                notepad_move_cursor(1, 0);
                break;
                
            case KEY_HOME:
                cursor_x = 0;
                update_pos_from_cursor();
                break;
                
            case KEY_END:
                cursor_x = get_line_length(cursor_y);
                update_pos_from_cursor();
                break;
                
            case KEY_PAGEUP:
                notepad_move_cursor(0, -EDIT_HEIGHT);
                break;
                
            case KEY_PAGEDOWN:
                notepad_move_cursor(0, EDIT_HEIGHT);
                break;
                
            case KEY_DELETE:
                notepad_delete_char();
                break;
                
            case KEY_BACKSPACE:
                notepad_backspace();
                break;
                
            case KEY_ENTER:
                notepad_insert_char('\n');
                break;
                
            case KEY_TAB:
                // Insert 4 spaces
                for (int i = 0; i < 4; i++) {
                    notepad_insert_char(' ');
                }
                break;
                
            default:
                // Handle Ctrl+N (new document)
                if (event.ctrl && event.ascii == 'n') {
                    notepad_new();
                }
                // Regular character
                else if (event.ascii >= 32 && event.ascii < 127) {
                    notepad_insert_char(event.ascii);
                }
                break;
        }
        
        notepad_redraw();
    }
}
