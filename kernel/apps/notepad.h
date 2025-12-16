#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <stdint.h>
#include <stdbool.h>

// Maximum document size
#define NOTEPAD_MAX_LINES 1000
#define NOTEPAD_MAX_LINE_LENGTH 256
#define NOTEPAD_MAX_SIZE (NOTEPAD_MAX_LINES * NOTEPAD_MAX_LINE_LENGTH)

// Initialize notepad
void notepad_init(void);

// Run notepad main loop
void notepad_run(void);

// Create new document
void notepad_new(void);

// Clear document
void notepad_clear(void);

// Insert character at cursor
void notepad_insert_char(char c);

// Delete character at cursor
void notepad_delete_char(void);

// Backspace
void notepad_backspace(void);

// Move cursor
void notepad_move_cursor(int dx, int dy);

// Set cursor position
void notepad_set_cursor(int x, int y);

// Get cursor position
int notepad_get_cursor_x(void);
int notepad_get_cursor_y(void);

// Get document content
const char* notepad_get_content(void);

// Get current line count
int notepad_get_line_count(void);

// Redraw editor
void notepad_redraw(void);

#endif // NOTEPAD_H
