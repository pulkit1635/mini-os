#include "gui.h"
#include "string.h"
#include "memory.h"
#include "keyboard.h"

// Global desktop
static gui_desktop_t g_desktop;

// Default color scheme
static gui_color_scheme_t g_colors = {
    .window_bg = VGA_COLOR_LIGHT_GREY,
    .window_border = VGA_COLOR_BLACK,
    .titlebar_active_bg = VGA_COLOR_BLUE,
    .titlebar_inactive_bg = VGA_COLOR_DARK_GREY,
    .titlebar_text = VGA_COLOR_WHITE,
    .button_bg = VGA_COLOR_DARK_GREY,
    .button_fg = VGA_COLOR_WHITE,
    .button_pressed_bg = VGA_COLOR_BLACK,
    .textbox_bg = VGA_COLOR_WHITE,
    .textbox_fg = VGA_COLOR_BLACK,
    .textbox_cursor = VGA_COLOR_BLACK,
    .list_bg = VGA_COLOR_WHITE,
    .list_fg = VGA_COLOR_BLACK,
    .list_selected_bg = VGA_COLOR_BLUE,
    .list_selected_fg = VGA_COLOR_WHITE,
    .progress_bg = VGA_COLOR_DARK_GREY,
    .progress_fill = VGA_COLOR_GREEN,
    .disabled_fg = VGA_COLOR_DARK_GREY
};

// Element ID counter
static int g_element_id_counter = 0;

void gui_init(void) {
    memset(&g_desktop, 0, sizeof(gui_desktop_t));
    g_desktop.desktop_color = VGA_COLOR_CYAN;
    g_desktop.wallpaper_char = ' ';
    g_element_id_counter = 0;
}

gui_desktop_t* gui_get_desktop(void) {
    return &g_desktop;
}

gui_color_scheme_t* gui_get_colors(void) {
    return &g_colors;
}

void gui_set_colors(gui_color_scheme_t* scheme) {
    if (scheme) {
        memcpy(&g_colors, scheme, sizeof(gui_color_scheme_t));
    }
}

// Window functions
gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height) {
    gui_window_t* window = (gui_window_t*)kmalloc(sizeof(gui_window_t));
    if (!window) return NULL;
    
    memset(window, 0, sizeof(gui_window_t));
    
    window->base.type = GUI_ELEMENT_WINDOW;
    window->base.state = GUI_STATE_NORMAL;
    window->base.x = x;
    window->base.y = y;
    window->base.width = width;
    window->base.height = height;
    window->base.id = ++g_element_id_counter;
    window->base.fg_color = g_colors.titlebar_text;
    window->base.bg_color = g_colors.window_bg;
    
    strncpy(window->title, title ? title : "Window", 63);
    window->title[63] = '\0';
    
    window->has_border = true;
    window->has_titlebar = true;
    window->closable = true;
    window->movable = true;
    window->visible = true;
    window->element_count = 0;
    window->focused_element = -1;
    
    // Add to desktop
    if (g_desktop.window_count < 16) {
        g_desktop.windows[g_desktop.window_count++] = window;
        g_desktop.active_window = g_desktop.window_count - 1;
    }
    
    return window;
}

void gui_destroy_window(gui_window_t* window) {
    if (!window) return;
    
    // Remove from desktop
    for (int i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i] == window) {
            // Shift remaining windows
            for (int j = i; j < g_desktop.window_count - 1; j++) {
                g_desktop.windows[j] = g_desktop.windows[j + 1];
            }
            g_desktop.window_count--;
            break;
        }
    }
    
    // Free child elements
    for (int i = 0; i < window->element_count; i++) {
        if (window->elements[i]) {
            kfree(window->elements[i]);
        }
    }
    
    kfree(window);
}

void gui_show_window(gui_window_t* window) {
    if (window) {
        window->visible = true;
    }
}

void gui_hide_window(gui_window_t* window) {
    if (window) {
        window->visible = false;
    }
}

void gui_set_active_window(gui_window_t* window) {
    for (int i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i] == window) {
            g_desktop.active_window = i;
            break;
        }
    }
}

void gui_window_add_element(gui_window_t* window, gui_element_t* element) {
    if (!window || !element) return;
    if (window->element_count >= 32) return;
    
    element->parent = window;
    window->elements[window->element_count++] = element;
    
    if (window->focused_element < 0) {
        window->focused_element = 0;
        element->state = GUI_STATE_FOCUSED;
    }
}

// Element creation functions
gui_button_t* gui_create_button(const char* text, int x, int y, int width) {
    gui_button_t* button = (gui_button_t*)kmalloc(sizeof(gui_button_t));
    if (!button) return NULL;
    
    memset(button, 0, sizeof(gui_button_t));
    button->base.type = GUI_ELEMENT_BUTTON;
    button->base.state = GUI_STATE_NORMAL;
    button->base.x = x;
    button->base.y = y;
    button->base.width = width;
    button->base.height = 1;
    button->base.id = ++g_element_id_counter;
    button->base.fg_color = g_colors.button_fg;
    button->base.bg_color = g_colors.button_bg;
    
    strncpy(button->base.text, text ? text : "Button", 63);
    button->pressed = false;
    
    return button;
}

gui_label_t* gui_create_label(const char* text, int x, int y) {
    gui_label_t* label = (gui_label_t*)kmalloc(sizeof(gui_label_t));
    if (!label) return NULL;
    
    memset(label, 0, sizeof(gui_label_t));
    label->base.type = GUI_ELEMENT_LABEL;
    label->base.state = GUI_STATE_NORMAL;
    label->base.x = x;
    label->base.y = y;
    label->base.width = text ? strlen(text) : 0;
    label->base.height = 1;
    label->base.id = ++g_element_id_counter;
    label->base.fg_color = VGA_COLOR_BLACK;
    label->base.bg_color = g_colors.window_bg;
    
    strncpy(label->base.text, text ? text : "", 63);
    label->text_align = 0;
    
    return label;
}

gui_textbox_t* gui_create_textbox(int x, int y, int width, int max_length) {
    gui_textbox_t* textbox = (gui_textbox_t*)kmalloc(sizeof(gui_textbox_t));
    if (!textbox) return NULL;
    
    memset(textbox, 0, sizeof(gui_textbox_t));
    textbox->base.type = GUI_ELEMENT_TEXTBOX;
    textbox->base.state = GUI_STATE_NORMAL;
    textbox->base.x = x;
    textbox->base.y = y;
    textbox->base.width = width;
    textbox->base.height = 1;
    textbox->base.id = ++g_element_id_counter;
    textbox->base.fg_color = g_colors.textbox_fg;
    textbox->base.bg_color = g_colors.textbox_bg;
    
    textbox->cursor_pos = 0;
    textbox->max_length = max_length < 255 ? max_length : 255;
    textbox->password_mode = false;
    
    return textbox;
}

gui_checkbox_t* gui_create_checkbox(const char* text, int x, int y) {
    gui_checkbox_t* checkbox = (gui_checkbox_t*)kmalloc(sizeof(gui_checkbox_t));
    if (!checkbox) return NULL;
    
    memset(checkbox, 0, sizeof(gui_checkbox_t));
    checkbox->base.type = GUI_ELEMENT_CHECKBOX;
    checkbox->base.state = GUI_STATE_NORMAL;
    checkbox->base.x = x;
    checkbox->base.y = y;
    checkbox->base.width = 4 + (text ? strlen(text) : 0);
    checkbox->base.height = 1;
    checkbox->base.id = ++g_element_id_counter;
    checkbox->base.fg_color = VGA_COLOR_BLACK;
    checkbox->base.bg_color = g_colors.window_bg;
    
    strncpy(checkbox->base.text, text ? text : "", 63);
    checkbox->checked = false;
    
    return checkbox;
}

gui_listbox_t* gui_create_listbox(int x, int y, int width, int height) {
    gui_listbox_t* listbox = (gui_listbox_t*)kmalloc(sizeof(gui_listbox_t));
    if (!listbox) return NULL;
    
    memset(listbox, 0, sizeof(gui_listbox_t));
    listbox->base.type = GUI_ELEMENT_LISTBOX;
    listbox->base.state = GUI_STATE_NORMAL;
    listbox->base.x = x;
    listbox->base.y = y;
    listbox->base.width = width;
    listbox->base.height = height;
    listbox->base.id = ++g_element_id_counter;
    listbox->base.fg_color = g_colors.list_fg;
    listbox->base.bg_color = g_colors.list_bg;
    
    listbox->item_count = 0;
    listbox->selected_index = -1;
    listbox->scroll_offset = 0;
    listbox->visible_items = height;
    
    return listbox;
}

gui_progress_bar_t* gui_create_progress_bar(int x, int y, int width) {
    gui_progress_bar_t* bar = (gui_progress_bar_t*)kmalloc(sizeof(gui_progress_bar_t));
    if (!bar) return NULL;
    
    memset(bar, 0, sizeof(gui_progress_bar_t));
    bar->base.type = GUI_ELEMENT_PROGRESS_BAR;
    bar->base.state = GUI_STATE_NORMAL;
    bar->base.x = x;
    bar->base.y = y;
    bar->base.width = width;
    bar->base.height = 1;
    bar->base.id = ++g_element_id_counter;
    bar->base.fg_color = g_colors.progress_fill;
    bar->base.bg_color = g_colors.progress_bg;
    
    bar->value = 0;
    bar->max_value = 100;
    bar->show_percentage = true;
    
    return bar;
}

// Listbox functions
void gui_listbox_add_item(gui_listbox_t* list, const char* text, int id, void* data) {
    if (!list || list->item_count >= 32) return;
    
    gui_list_item_t* item = &list->items[list->item_count++];
    strncpy(item->text, text ? text : "", 63);
    item->text[63] = '\0';
    item->id = id;
    item->data = data;
    
    if (list->selected_index < 0) {
        list->selected_index = 0;
    }
}

void gui_listbox_clear(gui_listbox_t* list) {
    if (!list) return;
    list->item_count = 0;
    list->selected_index = -1;
    list->scroll_offset = 0;
}

gui_list_item_t* gui_listbox_get_selected(gui_listbox_t* list) {
    if (!list || list->selected_index < 0 || list->selected_index >= list->item_count) {
        return NULL;
    }
    return &list->items[list->selected_index];
}

void gui_progress_set_value(gui_progress_bar_t* bar, int value) {
    if (!bar) return;
    if (value < 0) value = 0;
    if (value > bar->max_value) value = bar->max_value;
    bar->value = value;
}

// Drawing functions
void gui_draw_desktop(void) {
    // Fill desktop background
    uint8_t color = vga_entry_color(VGA_COLOR_WHITE, g_desktop.desktop_color);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_set_color(color);
            vga_putchar_at(g_desktop.wallpaper_char, x, y);
        }
    }
    
    // Draw all visible windows
    for (int i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i] && g_desktop.windows[i]->visible) {
            gui_draw_window(g_desktop.windows[i]);
        }
    }
}

void gui_draw_window(gui_window_t* window) {
    if (!window || !window->visible) return;
    
    int x = window->base.x;
    int y = window->base.y;
    int w = window->base.width;
    int h = window->base.height;
    
    bool is_active = (g_desktop.windows[g_desktop.active_window] == window);
    
    // Draw window background
    uint8_t bg_color = vga_entry_color(VGA_COLOR_BLACK, g_colors.window_bg);
    gui_draw_filled_box(x, y, w, h, ' ', bg_color);
    
    // Draw border
    if (window->has_border) {
        gui_draw_box(x, y, w, h, vga_entry_color(g_colors.window_border, g_colors.window_bg));
    }
    
    // Draw title bar
    if (window->has_titlebar) {
        uint8_t title_bg = is_active ? g_colors.titlebar_active_bg : g_colors.titlebar_inactive_bg;
        uint8_t title_color = vga_entry_color(g_colors.titlebar_text, title_bg);
        
        gui_draw_filled_box(x + 1, y, w - 2, 1, ' ', title_color);
        
        // Title text
        int title_len = strlen(window->title);
        int title_x = x + (w - title_len) / 2;
        gui_draw_text(window->title, title_x, y, title_color);
        
        // Close button
        if (window->closable) {
            vga_set_color(vga_entry_color(VGA_COLOR_RED, title_bg));
            vga_putchar_at('X', x + w - 2, y);
        }
    }
    
    // Draw child elements
    int content_y = window->has_titlebar ? y + 1 : y;
    for (int i = 0; i < window->element_count; i++) {
        gui_element_t* elem = window->elements[i];
        if (elem && elem->state != GUI_STATE_HIDDEN) {
            // Adjust element position relative to window
            int elem_x = x + 1 + elem->x;
            int elem_y = content_y + 1 + elem->y;
            
            // Temporarily modify element position for drawing
            int orig_x = elem->x;
            int orig_y = elem->y;
            elem->x = elem_x;
            elem->y = elem_y;
            
            gui_draw_element(elem);
            
            // Restore original position
            elem->x = orig_x;
            elem->y = orig_y;
        }
    }
}

void gui_draw_element(gui_element_t* element) {
    if (!element || element->state == GUI_STATE_HIDDEN) return;
    
    switch (element->type) {
        case GUI_ELEMENT_BUTTON:
            gui_draw_button((gui_button_t*)element);
            break;
        case GUI_ELEMENT_LABEL:
            gui_draw_label((gui_label_t*)element);
            break;
        case GUI_ELEMENT_TEXTBOX:
            gui_draw_textbox((gui_textbox_t*)element);
            break;
        case GUI_ELEMENT_CHECKBOX:
            gui_draw_checkbox((gui_checkbox_t*)element);
            break;
        case GUI_ELEMENT_LISTBOX:
            gui_draw_listbox((gui_listbox_t*)element);
            break;
        case GUI_ELEMENT_PROGRESS_BAR:
            gui_draw_progress_bar((gui_progress_bar_t*)element);
            break;
        default:
            break;
    }
}

void gui_draw_button(gui_button_t* button) {
    if (!button) return;
    
    gui_element_t* base = &button->base;
    uint8_t bg = button->pressed ? g_colors.button_pressed_bg : g_colors.button_bg;
    
    if (base->state == GUI_STATE_FOCUSED) {
        bg = VGA_COLOR_LIGHT_BLUE;
    }
    
    uint8_t color = vga_entry_color(g_colors.button_fg, bg);
    
    // Draw button background
    gui_draw_filled_box(base->x, base->y, base->width, 1, ' ', color);
    
    // Draw brackets and text
    vga_set_color(color);
    vga_putchar_at('[', base->x, base->y);
    vga_putchar_at(']', base->x + base->width - 1, base->y);
    
    // Center text
    int text_len = strlen(base->text);
    int text_x = base->x + (base->width - text_len) / 2;
    gui_draw_text(base->text, text_x, base->y, color);
}

void gui_draw_label(gui_label_t* label) {
    if (!label) return;
    
    gui_element_t* base = &label->base;
    uint8_t color = vga_entry_color(base->fg_color, base->bg_color);
    
    gui_draw_text(base->text, base->x, base->y, color);
}

void gui_draw_textbox(gui_textbox_t* textbox) {
    if (!textbox) return;
    
    gui_element_t* base = &textbox->base;
    uint8_t bg = g_colors.textbox_bg;
    
    if (base->state == GUI_STATE_FOCUSED) {
        bg = VGA_COLOR_LIGHT_CYAN;
    }
    
    uint8_t color = vga_entry_color(g_colors.textbox_fg, bg);
    
    // Draw textbox background
    gui_draw_filled_box(base->x, base->y, base->width, 1, ' ', color);
    
    // Draw text or asterisks
    vga_set_color(color);
    int display_len = base->width - 2;
    int start = 0;
    if ((int)strlen(textbox->value) > display_len) {
        start = strlen(textbox->value) - display_len;
    }
    
    for (int i = 0; i < display_len && textbox->value[start + i]; i++) {
        char c = textbox->password_mode ? '*' : textbox->value[start + i];
        vga_putchar_at(c, base->x + 1 + i, base->y);
    }
    
    // Draw cursor if focused
    if (base->state == GUI_STATE_FOCUSED) {
        int cursor_screen_x = base->x + 1 + (textbox->cursor_pos - start);
        if (cursor_screen_x < base->x + base->width - 1) {
            vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            vga_putchar_at('_', cursor_screen_x, base->y);
        }
    }
}

void gui_draw_checkbox(gui_checkbox_t* checkbox) {
    if (!checkbox) return;
    
    gui_element_t* base = &checkbox->base;
    uint8_t color = vga_entry_color(base->fg_color, base->bg_color);
    
    if (base->state == GUI_STATE_FOCUSED) {
        color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    }
    
    vga_set_color(color);
    
    // Draw checkbox
    vga_putchar_at('[', base->x, base->y);
    vga_putchar_at(checkbox->checked ? 'X' : ' ', base->x + 1, base->y);
    vga_putchar_at(']', base->x + 2, base->y);
    
    // Draw label
    gui_draw_text(base->text, base->x + 4, base->y, color);
}

void gui_draw_listbox(gui_listbox_t* listbox) {
    if (!listbox) return;
    
    gui_element_t* base = &listbox->base;
    uint8_t bg_color = vga_entry_color(g_colors.list_fg, g_colors.list_bg);
    uint8_t sel_color = vga_entry_color(g_colors.list_selected_fg, g_colors.list_selected_bg);
    
    // Draw list background
    gui_draw_filled_box(base->x, base->y, base->width, base->height, ' ', bg_color);
    
    // Draw items
    for (int i = 0; i < listbox->visible_items && (listbox->scroll_offset + i) < listbox->item_count; i++) {
        int item_idx = listbox->scroll_offset + i;
        bool selected = (item_idx == listbox->selected_index);
        
        uint8_t color = selected ? sel_color : bg_color;
        
        // Fill line
        for (int x = 0; x < base->width; x++) {
            vga_set_color(color);
            vga_putchar_at(' ', base->x + x, base->y + i);
        }
        
        // Draw text
        gui_draw_text(listbox->items[item_idx].text, base->x + 1, base->y + i, color);
    }
    
    // Draw scrollbar if needed
    if (listbox->item_count > listbox->visible_items) {
        int scroll_x = base->x + base->width - 1;
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
        for (int y = 0; y < base->height; y++) {
            vga_putchar_at(' ', scroll_x, base->y + y);
        }
        
        // Scroll position indicator
        int indicator_pos = (listbox->scroll_offset * base->height) / listbox->item_count;
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
        vga_putchar_at(' ', scroll_x, base->y + indicator_pos);
    }
}

void gui_draw_progress_bar(gui_progress_bar_t* bar) {
    if (!bar) return;
    
    gui_element_t* base = &bar->base;
    
    // Calculate fill width
    int fill_width = (bar->value * (base->width - 2)) / bar->max_value;
    
    // Draw background
    uint8_t bg_color = vga_entry_color(VGA_COLOR_WHITE, g_colors.progress_bg);
    gui_draw_filled_box(base->x, base->y, base->width, 1, ' ', bg_color);
    
    // Draw fill
    uint8_t fill_color = vga_entry_color(VGA_COLOR_WHITE, g_colors.progress_fill);
    for (int i = 0; i < fill_width; i++) {
        vga_set_color(fill_color);
        vga_putchar_at(' ', base->x + 1 + i, base->y);
    }
    
    // Draw percentage
    if (bar->show_percentage) {
        char percent_str[8];
        int percent = (bar->value * 100) / bar->max_value;
        itoa(percent, percent_str, 10);
        strcat(percent_str, "%");
        
        int text_x = base->x + (base->width - strlen(percent_str)) / 2;
        
        // Choose color based on position
        for (int i = 0; percent_str[i]; i++) {
            int x = text_x + i;
            bool on_fill = (x - base->x - 1) < fill_width;
            uint8_t color = on_fill ? 
                vga_entry_color(VGA_COLOR_WHITE, g_colors.progress_fill) :
                vga_entry_color(VGA_COLOR_WHITE, g_colors.progress_bg);
            vga_set_color(color);
            vga_putchar_at(percent_str[i], x, base->y);
        }
    }
    
    // Draw brackets
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, g_colors.window_bg));
    vga_putchar_at('[', base->x, base->y);
    vga_putchar_at(']', base->x + base->width - 1, base->y);
}

// Draw helpers
void gui_draw_box(int x, int y, int w, int h, uint8_t color) {
    vga_set_color(color);
    
    // Corners
    vga_putchar_at('+', x, y);
    vga_putchar_at('+', x + w - 1, y);
    vga_putchar_at('+', x, y + h - 1);
    vga_putchar_at('+', x + w - 1, y + h - 1);
    
    // Top and bottom
    for (int i = 1; i < w - 1; i++) {
        vga_putchar_at('-', x + i, y);
        vga_putchar_at('-', x + i, y + h - 1);
    }
    
    // Sides
    for (int j = 1; j < h - 1; j++) {
        vga_putchar_at('|', x, y + j);
        vga_putchar_at('|', x + w - 1, y + j);
    }
}

void gui_draw_filled_box(int x, int y, int w, int h, char c, uint8_t color) {
    vga_set_color(color);
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (x + i >= 0 && x + i < VGA_WIDTH && y + j >= 0 && y + j < VGA_HEIGHT) {
                vga_putchar_at(c, x + i, y + j);
            }
        }
    }
}

void gui_draw_text(const char* text, int x, int y, uint8_t color) {
    if (!text) return;
    vga_set_color(color);
    for (int i = 0; text[i] && x + i < VGA_WIDTH; i++) {
        if (x + i >= 0 && y >= 0 && y < VGA_HEIGHT) {
            vga_putchar_at(text[i], x + i, y);
        }
    }
}

void gui_draw_centered_text(const char* text, int x, int y, int width, uint8_t color) {
    if (!text) return;
    int len = strlen(text);
    int start_x = x + (width - len) / 2;
    gui_draw_text(text, start_x, y, color);
}

void gui_draw_horizontal_line(int x, int y, int length, char c, uint8_t color) {
    vga_set_color(color);
    for (int i = 0; i < length; i++) {
        vga_putchar_at(c, x + i, y);
    }
}

void gui_draw_vertical_line(int x, int y, int length, char c, uint8_t color) {
    vga_set_color(color);
    for (int i = 0; i < length; i++) {
        vga_putchar_at(c, x, y + i);
    }
}

// Event handling
bool gui_handle_key(uint8_t scancode, char ascii, bool shift, bool ctrl) {
    if (g_desktop.window_count == 0 || g_desktop.active_window < 0) {
        return false;
    }
    
    gui_window_t* window = g_desktop.windows[g_desktop.active_window];
    if (!window || !window->visible) return false;
    
    // Tab to switch focus
    if (scancode == KEY_TAB) {
        if (shift) {
            gui_focus_prev_element(window);
        } else {
            gui_focus_next_element(window);
        }
        return true;
    }
    
    // Get focused element
    if (window->focused_element < 0 || window->focused_element >= window->element_count) {
        return false;
    }
    
    gui_element_t* focused = window->elements[window->focused_element];
    if (!focused) return false;
    
    // Handle input based on element type
    switch (focused->type) {
        case GUI_ELEMENT_BUTTON:
            if (scancode == KEY_ENTER || ascii == ' ') {
                if (focused->on_click) {
                    focused->on_click(focused, 0);
                }
                return true;
            }
            break;
            
        case GUI_ELEMENT_TEXTBOX: {
            gui_textbox_t* textbox = (gui_textbox_t*)focused;
            if (scancode == KEY_BACKSPACE) {
                if (textbox->cursor_pos > 0) {
                    textbox->cursor_pos--;
                    // Shift characters left
                    for (int i = textbox->cursor_pos; textbox->value[i]; i++) {
                        textbox->value[i] = textbox->value[i + 1];
                    }
                }
                return true;
            } else if (scancode == KEY_LEFT) {
                if (textbox->cursor_pos > 0) textbox->cursor_pos--;
                return true;
            } else if (scancode == KEY_RIGHT) {
                if (textbox->cursor_pos < (int)strlen(textbox->value)) {
                    textbox->cursor_pos++;
                }
                return true;
            } else if (ascii >= 32 && ascii < 127) {
                int len = strlen(textbox->value);
                if (len < textbox->max_length) {
                    // Insert character
                    for (int i = len + 1; i > textbox->cursor_pos; i--) {
                        textbox->value[i] = textbox->value[i - 1];
                    }
                    textbox->value[textbox->cursor_pos++] = ascii;
                }
                return true;
            }
            break;
        }
            
        case GUI_ELEMENT_CHECKBOX: {
            gui_checkbox_t* checkbox = (gui_checkbox_t*)focused;
            if (scancode == KEY_ENTER || ascii == ' ') {
                checkbox->checked = !checkbox->checked;
                if (focused->on_click) {
                    focused->on_click(focused, checkbox->checked);
                }
                return true;
            }
            break;
        }
            
        case GUI_ELEMENT_LISTBOX: {
            gui_listbox_t* listbox = (gui_listbox_t*)focused;
            if (scancode == KEY_UP) {
                if (listbox->selected_index > 0) {
                    listbox->selected_index--;
                    if (listbox->selected_index < listbox->scroll_offset) {
                        listbox->scroll_offset = listbox->selected_index;
                    }
                }
                return true;
            } else if (scancode == KEY_DOWN) {
                if (listbox->selected_index < listbox->item_count - 1) {
                    listbox->selected_index++;
                    if (listbox->selected_index >= listbox->scroll_offset + listbox->visible_items) {
                        listbox->scroll_offset++;
                    }
                }
                return true;
            } else if (scancode == KEY_ENTER) {
                if (focused->on_click) {
                    focused->on_click(focused, listbox->selected_index);
                }
                return true;
            }
            break;
        }
            
        default:
            break;
    }
    
    return false;
}

void gui_focus_next_element(gui_window_t* window) {
    if (!window || window->element_count == 0) return;
    
    // Clear current focus
    if (window->focused_element >= 0 && window->focused_element < window->element_count) {
        window->elements[window->focused_element]->state = GUI_STATE_NORMAL;
    }
    
    // Find next focusable element
    int start = window->focused_element;
    do {
        window->focused_element = (window->focused_element + 1) % window->element_count;
        gui_element_t* elem = window->elements[window->focused_element];
        if (elem && elem->state != GUI_STATE_DISABLED && elem->state != GUI_STATE_HIDDEN &&
            elem->type != GUI_ELEMENT_LABEL) {
            elem->state = GUI_STATE_FOCUSED;
            return;
        }
    } while (window->focused_element != start);
}

void gui_focus_prev_element(gui_window_t* window) {
    if (!window || window->element_count == 0) return;
    
    // Clear current focus
    if (window->focused_element >= 0 && window->focused_element < window->element_count) {
        window->elements[window->focused_element]->state = GUI_STATE_NORMAL;
    }
    
    // Find previous focusable element
    int start = window->focused_element;
    do {
        window->focused_element--;
        if (window->focused_element < 0) {
            window->focused_element = window->element_count - 1;
        }
        gui_element_t* elem = window->elements[window->focused_element];
        if (elem && elem->state != GUI_STATE_DISABLED && elem->state != GUI_STATE_HIDDEN &&
            elem->type != GUI_ELEMENT_LABEL) {
            elem->state = GUI_STATE_FOCUSED;
            return;
        }
    } while (window->focused_element != start);
}

void gui_message_box(const char* title, const char* message) {
    // Calculate box size
    int msg_len = message ? strlen(message) : 0;
    int title_len = title ? strlen(title) : 0;
    int width = msg_len > title_len ? msg_len + 4 : title_len + 4;
    if (width < 20) width = 20;
    if (width > 60) width = 60;
    
    int height = 6;
    int x = (VGA_WIDTH - width) / 2;
    int y = (VGA_HEIGHT - height) / 2;
    
    // Save screen content (simplified - just clear)
    uint8_t box_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    uint8_t text_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    
    // Draw box
    gui_draw_filled_box(x, y, width, height, ' ', box_color);
    gui_draw_box(x, y, width, height, text_color);
    
    // Draw title
    if (title) {
        gui_draw_centered_text(title, x, y, width, text_color);
    }
    
    // Draw message
    if (message) {
        gui_draw_centered_text(message, x, y + 2, width, text_color);
    }
    
    // Draw OK button
    gui_draw_centered_text("[ OK ]", x, y + height - 2, width, text_color);
    
    // Wait for key
    keyboard_getchar();
}

bool gui_input_dialog(const char* title, const char* prompt, char* buffer, int buffer_size) {
    if (!buffer || buffer_size <= 0) return false;
    
    int width = 50;
    int height = 8;
    int x = (VGA_WIDTH - width) / 2;
    int y = (VGA_HEIGHT - height) / 2;
    
    uint8_t box_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    uint8_t text_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    uint8_t input_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
    
    // Draw box
    gui_draw_filled_box(x, y, width, height, ' ', box_color);
    gui_draw_box(x, y, width, height, text_color);
    
    // Draw title
    if (title) {
        uint8_t title_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        gui_draw_filled_box(x + 1, y, width - 2, 1, ' ', title_color);
        gui_draw_centered_text(title, x, y, width, title_color);
    }
    
    // Draw prompt
    if (prompt) {
        gui_draw_text(prompt, x + 2, y + 2, text_color);
    }
    
    // Draw input area
    int input_y = y + 4;
    int input_width = width - 4;
    gui_draw_filled_box(x + 2, input_y, input_width, 1, ' ', input_color);
    
    // Get input
    buffer[0] = '\0';
    int pos = 0;
    
    while (1) {
        // Redraw input
        gui_draw_filled_box(x + 2, input_y, input_width, 1, ' ', input_color);
        gui_draw_text(buffer, x + 3, input_y, input_color);
        
        // Show cursor
        vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        vga_putchar_at('_', x + 3 + pos, input_y);
        
        // Draw buttons
        gui_draw_text("[ OK ]", x + width - 20, y + height - 2, text_color);
        gui_draw_text("[Cancel]", x + width - 12, y + height - 2, text_color);
        
        key_event_t event = keyboard_get_key();
        if (event.released) continue;
        
        if (event.scancode == KEY_ENTER) {
            return true;
        } else if (event.scancode == KEY_ESCAPE) {
            return false;
        } else if (event.scancode == KEY_BACKSPACE) {
            if (pos > 0) {
                buffer[--pos] = '\0';
            }
        } else if (event.ascii >= 32 && event.ascii < 127 && pos < buffer_size - 1) {
            buffer[pos++] = event.ascii;
            buffer[pos] = '\0';
        }
    }
}
