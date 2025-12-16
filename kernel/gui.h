#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "vga.h"

// GUI element types
typedef enum {
    GUI_ELEMENT_NONE = 0,
    GUI_ELEMENT_WINDOW,
    GUI_ELEMENT_BUTTON,
    GUI_ELEMENT_LABEL,
    GUI_ELEMENT_TEXTBOX,
    GUI_ELEMENT_CHECKBOX,
    GUI_ELEMENT_LISTBOX,
    GUI_ELEMENT_PROGRESS_BAR,
    GUI_ELEMENT_MENU,
    GUI_ELEMENT_STATUSBAR
} gui_element_type_t;

// GUI element state
typedef enum {
    GUI_STATE_NORMAL = 0,
    GUI_STATE_FOCUSED,
    GUI_STATE_PRESSED,
    GUI_STATE_DISABLED,
    GUI_STATE_HIDDEN
} gui_element_state_t;

// Forward declarations
struct gui_element;
struct gui_window;

// Event callback type
typedef void (*gui_callback_t)(struct gui_element* element, int event_type);

// Base GUI element structure
typedef struct gui_element {
    gui_element_type_t type;
    gui_element_state_t state;
    int x, y;                      // Position
    int width, height;             // Size
    char text[64];                 // Display text
    int id;                        // Element ID
    uint8_t fg_color;             // Foreground color
    uint8_t bg_color;             // Background color
    struct gui_window* parent;     // Parent window
    gui_callback_t on_click;       // Click callback
    gui_callback_t on_focus;       // Focus callback
    void* user_data;               // User data pointer
} gui_element_t;

// Window structure
typedef struct gui_window {
    gui_element_t base;
    char title[64];
    bool has_border;
    bool has_titlebar;
    bool closable;
    bool movable;
    bool visible;
    gui_element_t* elements[32];   // Child elements
    int element_count;
    int focused_element;
    gui_callback_t on_close;
} gui_window_t;

// Button element
typedef struct {
    gui_element_t base;
    bool pressed;
} gui_button_t;

// Label element
typedef struct {
    gui_element_t base;
    int text_align;  // 0=left, 1=center, 2=right
} gui_label_t;

// Textbox element
typedef struct {
    gui_element_t base;
    char value[256];
    int cursor_pos;
    int max_length;
    bool password_mode;
} gui_textbox_t;

// Checkbox element
typedef struct {
    gui_element_t base;
    bool checked;
} gui_checkbox_t;

// List item
typedef struct {
    char text[64];
    int id;
    void* data;
} gui_list_item_t;

// Listbox element
typedef struct {
    gui_element_t base;
    gui_list_item_t items[32];
    int item_count;
    int selected_index;
    int scroll_offset;
    int visible_items;
} gui_listbox_t;

// Progress bar element
typedef struct {
    gui_element_t base;
    int value;           // Current value (0-100)
    int max_value;       // Maximum value
    bool show_percentage;
} gui_progress_bar_t;

// Menu item
typedef struct {
    char text[32];
    char shortcut[16];
    gui_callback_t on_select;
    bool separator;
    bool enabled;
} gui_menu_item_t;

// Menu element
typedef struct {
    gui_element_t base;
    gui_menu_item_t items[16];
    int item_count;
    bool open;
    int selected_item;
} gui_menu_t;

// Desktop/screen manager
typedef struct {
    gui_window_t* windows[16];
    int window_count;
    int active_window;
    uint8_t desktop_color;
    char wallpaper_char;
} gui_desktop_t;

// GUI color scheme
typedef struct {
    uint8_t window_bg;
    uint8_t window_border;
    uint8_t titlebar_active_bg;
    uint8_t titlebar_inactive_bg;
    uint8_t titlebar_text;
    uint8_t button_bg;
    uint8_t button_fg;
    uint8_t button_pressed_bg;
    uint8_t textbox_bg;
    uint8_t textbox_fg;
    uint8_t textbox_cursor;
    uint8_t list_bg;
    uint8_t list_fg;
    uint8_t list_selected_bg;
    uint8_t list_selected_fg;
    uint8_t progress_bg;
    uint8_t progress_fill;
    uint8_t disabled_fg;
} gui_color_scheme_t;

// Initialize GUI subsystem
void gui_init(void);

// Get desktop
gui_desktop_t* gui_get_desktop(void);

// Get color scheme
gui_color_scheme_t* gui_get_colors(void);

// Set color scheme
void gui_set_colors(gui_color_scheme_t* scheme);

// Window functions
gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height);
void gui_destroy_window(gui_window_t* window);
void gui_show_window(gui_window_t* window);
void gui_hide_window(gui_window_t* window);
void gui_set_active_window(gui_window_t* window);
void gui_window_add_element(gui_window_t* window, gui_element_t* element);

// Element creation functions
gui_button_t* gui_create_button(const char* text, int x, int y, int width);
gui_label_t* gui_create_label(const char* text, int x, int y);
gui_textbox_t* gui_create_textbox(int x, int y, int width, int max_length);
gui_checkbox_t* gui_create_checkbox(const char* text, int x, int y);
gui_listbox_t* gui_create_listbox(int x, int y, int width, int height);
gui_progress_bar_t* gui_create_progress_bar(int x, int y, int width);

// Listbox functions
void gui_listbox_add_item(gui_listbox_t* list, const char* text, int id, void* data);
void gui_listbox_clear(gui_listbox_t* list);
gui_list_item_t* gui_listbox_get_selected(gui_listbox_t* list);

// Progress bar functions
void gui_progress_set_value(gui_progress_bar_t* bar, int value);

// Drawing functions
void gui_draw_desktop(void);
void gui_draw_window(gui_window_t* window);
void gui_draw_element(gui_element_t* element);
void gui_draw_button(gui_button_t* button);
void gui_draw_label(gui_label_t* label);
void gui_draw_textbox(gui_textbox_t* textbox);
void gui_draw_checkbox(gui_checkbox_t* checkbox);
void gui_draw_listbox(gui_listbox_t* listbox);
void gui_draw_progress_bar(gui_progress_bar_t* bar);

// Draw helpers
void gui_draw_box(int x, int y, int w, int h, uint8_t color);
void gui_draw_filled_box(int x, int y, int w, int h, char c, uint8_t color);
void gui_draw_text(const char* text, int x, int y, uint8_t color);
void gui_draw_centered_text(const char* text, int x, int y, int width, uint8_t color);
void gui_draw_horizontal_line(int x, int y, int length, char c, uint8_t color);
void gui_draw_vertical_line(int x, int y, int length, char c, uint8_t color);

// Event handling
bool gui_handle_key(uint8_t scancode, char ascii, bool shift, bool ctrl);
void gui_focus_next_element(gui_window_t* window);
void gui_focus_prev_element(gui_window_t* window);

// Message box
void gui_message_box(const char* title, const char* message);

// Input dialog
bool gui_input_dialog(const char* title, const char* prompt, char* buffer, int buffer_size);

#endif // GUI_H
