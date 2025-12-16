#include "css.h"
#include "../string.h"
#include "../memory.h"

// Forward declaration
static const char* css_strstr(const char* haystack, const char* needle);

// CSS color name to VGA color mapping
static const css_color_map_t color_map[] = {
    {"black", VGA_COLOR_BLACK},
    {"blue", VGA_COLOR_BLUE},
    {"green", VGA_COLOR_GREEN},
    {"cyan", VGA_COLOR_CYAN},
    {"red", VGA_COLOR_RED},
    {"magenta", VGA_COLOR_MAGENTA},
    {"brown", VGA_COLOR_BROWN},
    {"lightgray", VGA_COLOR_LIGHT_GREY},
    {"lightgrey", VGA_COLOR_LIGHT_GREY},
    {"gray", VGA_COLOR_DARK_GREY},
    {"grey", VGA_COLOR_DARK_GREY},
    {"darkgray", VGA_COLOR_DARK_GREY},
    {"darkgrey", VGA_COLOR_DARK_GREY},
    {"lightblue", VGA_COLOR_LIGHT_BLUE},
    {"lightgreen", VGA_COLOR_LIGHT_GREEN},
    {"lightcyan", VGA_COLOR_LIGHT_CYAN},
    {"lightred", VGA_COLOR_LIGHT_RED},
    {"pink", VGA_COLOR_LIGHT_RED},
    {"lightmagenta", VGA_COLOR_LIGHT_MAGENTA},
    {"yellow", VGA_COLOR_LIGHT_BROWN},
    {"white", VGA_COLOR_WHITE},
    {"navy", VGA_COLOR_BLUE},
    {"teal", VGA_COLOR_CYAN},
    {"maroon", VGA_COLOR_RED},
    {"purple", VGA_COLOR_MAGENTA},
    {"olive", VGA_COLOR_BROWN},
    {"silver", VGA_COLOR_LIGHT_GREY},
    {"lime", VGA_COLOR_LIGHT_GREEN},
    {"aqua", VGA_COLOR_LIGHT_CYAN},
    {"fuchsia", VGA_COLOR_LIGHT_MAGENTA},
    {"orange", VGA_COLOR_LIGHT_BROWN},
    {NULL, 0}
};

void css_init(void) {
    // Nothing to initialize
}

// Skip whitespace
static const char* skip_whitespace(const char* s) {
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) s++;
    return s;
}

// Parse hex color (#RGB or #RRGGBB) to VGA color
static uint8_t parse_hex_color(const char* hex) {
    if (*hex != '#') return VGA_COLOR_LIGHT_GREY;
    hex++;
    
    int r = 0, g = 0, b = 0;
    int len = strlen(hex);
    
    if (len == 3) {
        // #RGB format
        r = (hex[0] >= 'a' ? hex[0] - 'a' + 10 : (hex[0] >= 'A' ? hex[0] - 'A' + 10 : hex[0] - '0')) * 17;
        g = (hex[1] >= 'a' ? hex[1] - 'a' + 10 : (hex[1] >= 'A' ? hex[1] - 'A' + 10 : hex[1] - '0')) * 17;
        b = (hex[2] >= 'a' ? hex[2] - 'a' + 10 : (hex[2] >= 'A' ? hex[2] - 'A' + 10 : hex[2] - '0')) * 17;
    } else if (len >= 6) {
        // #RRGGBB format
        for (int i = 0; i < 2; i++) {
            int c = hex[i];
            r = r * 16 + (c >= 'a' ? c - 'a' + 10 : (c >= 'A' ? c - 'A' + 10 : c - '0'));
        }
        for (int i = 2; i < 4; i++) {
            int c = hex[i];
            g = g * 16 + (c >= 'a' ? c - 'a' + 10 : (c >= 'A' ? c - 'A' + 10 : c - '0'));
        }
        for (int i = 4; i < 6; i++) {
            int c = hex[i];
            b = b * 16 + (c >= 'a' ? c - 'a' + 10 : (c >= 'A' ? c - 'A' + 10 : c - '0'));
        }
    }
    
    // Map RGB to nearest VGA color
    // Simple approach: determine dominant component
    int max_val = r > g ? (r > b ? r : b) : (g > b ? g : b);
    
    if (max_val < 64) return VGA_COLOR_BLACK;
    
    bool bright = max_val > 170;
    bool has_r = r > 85;
    bool has_g = g > 85;
    bool has_b = b > 85;
    
    if (!has_r && !has_g && !has_b) {
        return bright ? VGA_COLOR_DARK_GREY : VGA_COLOR_BLACK;
    }
    
    if (has_r && has_g && has_b) {
        return bright ? VGA_COLOR_WHITE : VGA_COLOR_LIGHT_GREY;
    }
    
    if (has_r && has_g) {
        return bright ? VGA_COLOR_LIGHT_BROWN : VGA_COLOR_BROWN;
    }
    if (has_r && has_b) {
        return bright ? VGA_COLOR_LIGHT_MAGENTA : VGA_COLOR_MAGENTA;
    }
    if (has_g && has_b) {
        return bright ? VGA_COLOR_LIGHT_CYAN : VGA_COLOR_CYAN;
    }
    
    if (has_r) return bright ? VGA_COLOR_LIGHT_RED : VGA_COLOR_RED;
    if (has_g) return bright ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_GREEN;
    if (has_b) return bright ? VGA_COLOR_LIGHT_BLUE : VGA_COLOR_BLUE;
    
    return VGA_COLOR_LIGHT_GREY;
}

uint8_t css_color_to_vga(const char* color) {
    if (!color || !*color) return VGA_COLOR_LIGHT_GREY;
    
    // Skip whitespace
    while (*color == ' ') color++;
    
    // Check for hex color
    if (*color == '#') {
        return parse_hex_color(color);
    }
    
    // Check for rgb() format
    if (strncmp(color, "rgb(", 4) == 0) {
        // Parse rgb(r, g, b)
        const char* p = color + 4;
        int r = atoi(p);
        while (*p && *p != ',') p++;
        if (*p) p++;
        int g = atoi(p);
        while (*p && *p != ',') p++;
        if (*p) p++;
        int b = atoi(p);
        
        // Convert to hex format and parse
        char hex[8];
        hex[0] = '#';
        hex[1] = "0123456789abcdef"[(r >> 4) & 0xF];
        hex[2] = "0123456789abcdef"[r & 0xF];
        hex[3] = "0123456789abcdef"[(g >> 4) & 0xF];
        hex[4] = "0123456789abcdef"[g & 0xF];
        hex[5] = "0123456789abcdef"[(b >> 4) & 0xF];
        hex[6] = "0123456789abcdef"[b & 0xF];
        hex[7] = '\0';
        return parse_hex_color(hex);
    }
    
    // Look up color name
    for (int i = 0; color_map[i].name != NULL; i++) {
        if (strcmp(color, color_map[i].name) == 0) {
            return color_map[i].vga_color;
        }
    }
    
    // Try case-insensitive match
    char lower[32];
    int len = strlen(color);
    if (len > 31) len = 31;
    for (int i = 0; i < len; i++) {
        lower[i] = tolower(color[i]);
    }
    lower[len] = '\0';
    
    for (int i = 0; color_map[i].name != NULL; i++) {
        if (strcmp(lower, color_map[i].name) == 0) {
            return color_map[i].vga_color;
        }
    }
    
    return VGA_COLOR_LIGHT_GREY;
}

css_property_type_t css_parse_property_name(const char* name) {
    if (strcmp(name, "color") == 0) return CSS_PROP_COLOR;
    if (strcmp(name, "background-color") == 0) return CSS_PROP_BACKGROUND_COLOR;
    if (strcmp(name, "background") == 0) return CSS_PROP_BACKGROUND_COLOR;
    if (strcmp(name, "font-weight") == 0) return CSS_PROP_FONT_WEIGHT;
    if (strcmp(name, "font-style") == 0) return CSS_PROP_FONT_STYLE;
    if (strcmp(name, "text-decoration") == 0) return CSS_PROP_TEXT_DECORATION;
    if (strcmp(name, "text-align") == 0) return CSS_PROP_TEXT_ALIGN;
    if (strcmp(name, "display") == 0) return CSS_PROP_DISPLAY;
    if (strcmp(name, "margin") == 0) return CSS_PROP_MARGIN;
    if (strcmp(name, "padding") == 0) return CSS_PROP_PADDING;
    if (strcmp(name, "border") == 0) return CSS_PROP_BORDER;
    if (strcmp(name, "width") == 0) return CSS_PROP_WIDTH;
    if (strcmp(name, "height") == 0) return CSS_PROP_HEIGHT;
    return CSS_PROP_UNKNOWN;
}

void css_get_default_style(const char* tag_name, css_computed_style_t* style) {
    // Initialize with defaults
    style->color = VGA_COLOR_LIGHT_GREY;
    style->background_color = VGA_COLOR_BLACK;
    style->bold = false;
    style->italic = false;
    style->underline = false;
    style->text_align = 0;  // left
    style->margin_top = 0;
    style->margin_bottom = 0;
    style->margin_left = 0;
    style->margin_right = 0;
    style->is_block = false;
    style->is_hidden = false;
    
    if (!tag_name) return;
    
    // Set defaults based on tag
    if (strcmp(tag_name, "h1") == 0) {
        style->color = VGA_COLOR_LIGHT_CYAN;
        style->bold = true;
        style->is_block = true;
        style->margin_top = 1;
        style->margin_bottom = 1;
    }
    else if (strcmp(tag_name, "h2") == 0) {
        style->color = VGA_COLOR_LIGHT_GREEN;
        style->bold = true;
        style->is_block = true;
        style->margin_top = 1;
        style->margin_bottom = 1;
    }
    else if (strcmp(tag_name, "h3") == 0) {
        style->color = VGA_COLOR_LIGHT_BROWN;
        style->bold = true;
        style->is_block = true;
    }
    else if (strcmp(tag_name, "h4") == 0 || strcmp(tag_name, "h5") == 0 || strcmp(tag_name, "h6") == 0) {
        style->color = VGA_COLOR_WHITE;
        style->bold = true;
        style->is_block = true;
    }
    else if (strcmp(tag_name, "p") == 0) {
        style->is_block = true;
        style->margin_bottom = 1;
    }
    else if (strcmp(tag_name, "div") == 0) {
        style->is_block = true;
    }
    else if (strcmp(tag_name, "span") == 0) {
        style->is_block = false;
    }
    else if (strcmp(tag_name, "a") == 0) {
        style->color = VGA_COLOR_LIGHT_MAGENTA;
        style->underline = true;
    }
    else if (strcmp(tag_name, "strong") == 0 || strcmp(tag_name, "b") == 0) {
        style->bold = true;
        style->color = VGA_COLOR_WHITE;
    }
    else if (strcmp(tag_name, "em") == 0 || strcmp(tag_name, "i") == 0) {
        style->italic = true;
    }
    else if (strcmp(tag_name, "u") == 0) {
        style->underline = true;
    }
    else if (strcmp(tag_name, "code") == 0 || strcmp(tag_name, "pre") == 0) {
        style->color = VGA_COLOR_LIGHT_GREEN;
        style->background_color = VGA_COLOR_DARK_GREY;
    }
    else if (strcmp(tag_name, "blockquote") == 0) {
        style->color = VGA_COLOR_CYAN;
        style->is_block = true;
        style->margin_left = 4;
    }
    else if (strcmp(tag_name, "ul") == 0 || strcmp(tag_name, "ol") == 0) {
        style->is_block = true;
        style->margin_left = 2;
    }
    else if (strcmp(tag_name, "li") == 0) {
        style->is_block = true;
        style->color = VGA_COLOR_CYAN;
    }
    else if (strcmp(tag_name, "hr") == 0) {
        style->is_block = true;
        style->color = VGA_COLOR_DARK_GREY;
    }
    else if (strcmp(tag_name, "button") == 0) {
        style->color = VGA_COLOR_BLACK;
        style->background_color = VGA_COLOR_LIGHT_GREY;
    }
    else if (strcmp(tag_name, "input") == 0) {
        style->color = VGA_COLOR_WHITE;
        style->background_color = VGA_COLOR_BLUE;
    }
}

bool css_parse_inline(const char* style_attr, css_computed_style_t* style) {
    if (!style_attr || !style) return false;
    
    const char* p = style_attr;
    char prop_name[64];
    char prop_value[64];
    
    while (*p) {
        p = skip_whitespace(p);
        if (!*p) break;
        
        // Parse property name
        int i = 0;
        while (*p && *p != ':' && *p != ';' && i < 63) {
            prop_name[i++] = tolower(*p);
            p++;
        }
        prop_name[i] = '\0';
        
        if (*p == ':') {
            p++;
            p = skip_whitespace(p);
            
            // Parse property value
            i = 0;
            while (*p && *p != ';' && i < 63) {
                prop_value[i++] = *p;
                p++;
            }
            // Trim trailing whitespace
            while (i > 0 && (prop_value[i-1] == ' ' || prop_value[i-1] == '\t')) i--;
            prop_value[i] = '\0';
            
            // Apply property
            css_property_type_t type = css_parse_property_name(prop_name);
            
            switch (type) {
                case CSS_PROP_COLOR:
                    style->color = css_color_to_vga(prop_value);
                    break;
                case CSS_PROP_BACKGROUND_COLOR:
                    style->background_color = css_color_to_vga(prop_value);
                    break;
                case CSS_PROP_FONT_WEIGHT:
                    style->bold = (strcmp(prop_value, "bold") == 0 || 
                                  strcmp(prop_value, "700") == 0 ||
                                  strcmp(prop_value, "800") == 0 ||
                                  strcmp(prop_value, "900") == 0);
                    break;
                case CSS_PROP_FONT_STYLE:
                    style->italic = (strcmp(prop_value, "italic") == 0 ||
                                   strcmp(prop_value, "oblique") == 0);
                    break;
                case CSS_PROP_TEXT_DECORATION:
                    style->underline = (css_strstr(prop_value, "underline") != NULL);
                    break;
                case CSS_PROP_TEXT_ALIGN:
                    if (strcmp(prop_value, "center") == 0) style->text_align = 1;
                    else if (strcmp(prop_value, "right") == 0) style->text_align = 2;
                    else style->text_align = 0;
                    break;
                case CSS_PROP_DISPLAY:
                    if (strcmp(prop_value, "none") == 0) style->is_hidden = true;
                    else if (strcmp(prop_value, "block") == 0) style->is_block = true;
                    else if (strcmp(prop_value, "inline") == 0) style->is_block = false;
                    break;
                default:
                    break;
            }
        }
        
        if (*p == ';') p++;
    }
    
    return true;
}

// Simple strstr implementation
static const char* css_strstr(const char* haystack, const char* needle) {
    if (!*needle) return haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return haystack;
    }
    return NULL;
}

bool css_parse(const char* css_text, css_stylesheet_t* stylesheet) {
    if (!css_text || !stylesheet) return false;
    
    const char* p = css_text;
    
    while (*p && stylesheet->rule_count < CSS_MAX_RULES) {
        p = skip_whitespace(p);
        if (!*p) break;
        
        // Skip comments
        if (*p == '/' && *(p+1) == '*') {
            p += 2;
            while (*p && !(*p == '*' && *(p+1) == '/')) p++;
            if (*p) p += 2;
            continue;
        }
        
        // Parse selector
        css_rule_t* rule = &stylesheet->rules[stylesheet->rule_count];
        int i = 0;
        while (*p && *p != '{' && i < CSS_MAX_SELECTOR_LEN - 1) {
            if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
                if (i > 0 && rule->selector[i-1] != ' ') {
                    rule->selector[i++] = ' ';
                }
            } else {
                rule->selector[i++] = *p;
            }
            p++;
        }
        // Trim trailing space
        while (i > 0 && rule->selector[i-1] == ' ') i--;
        rule->selector[i] = '\0';
        
        if (*p == '{') {
            p++;
            rule->property_count = 0;
            
            // Parse properties
            while (*p && *p != '}' && rule->property_count < CSS_MAX_PROPERTIES) {
                p = skip_whitespace(p);
                if (*p == '}') break;
                
                // Parse property name
                char prop_name[64];
                i = 0;
                while (*p && *p != ':' && *p != '}' && i < 63) {
                    if (*p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') {
                        prop_name[i++] = tolower(*p);
                    }
                    p++;
                }
                prop_name[i] = '\0';
                
                if (*p == ':') {
                    p++;
                    p = skip_whitespace(p);
                    
                    // Parse property value
                    css_property_t* prop = &rule->properties[rule->property_count];
                    prop->type = css_parse_property_name(prop_name);
                    
                    i = 0;
                    while (*p && *p != ';' && *p != '}' && i < CSS_MAX_VALUE_LEN - 1) {
                        prop->value[i++] = *p;
                        p++;
                    }
                    // Trim trailing whitespace
                    while (i > 0 && (prop->value[i-1] == ' ' || prop->value[i-1] == '\n')) i--;
                    prop->value[i] = '\0';
                    
                    rule->property_count++;
                }
                
                if (*p == ';') p++;
            }
            
            if (*p == '}') p++;
            stylesheet->rule_count++;
        }
    }
    
    return true;
}

void css_compute_style(const char* tag_name, const char* class_name,
                       const char* id, const char* inline_style,
                       const css_stylesheet_t* stylesheet,
                       css_computed_style_t* out_style) {
    // Get default style
    css_get_default_style(tag_name, out_style);
    
    // Apply stylesheet rules
    if (stylesheet) {
        for (int r = 0; r < stylesheet->rule_count; r++) {
            const css_rule_t* rule = &stylesheet->rules[r];
            bool matches = false;
            
            // Check if selector matches
            if (rule->selector[0] == '#' && id) {
                matches = (strcmp(rule->selector + 1, id) == 0);
            }
            else if (rule->selector[0] == '.') {
                matches = (class_name && strcmp(rule->selector + 1, class_name) == 0);
            }
            else {
                matches = (tag_name && strcmp(rule->selector, tag_name) == 0);
            }
            
            if (matches) {
                // Apply properties
                for (int p = 0; p < rule->property_count; p++) {
                    const css_property_t* prop = &rule->properties[p];
                    
                    switch (prop->type) {
                        case CSS_PROP_COLOR:
                            out_style->color = css_color_to_vga(prop->value);
                            break;
                        case CSS_PROP_BACKGROUND_COLOR:
                            out_style->background_color = css_color_to_vga(prop->value);
                            break;
                        case CSS_PROP_FONT_WEIGHT:
                            out_style->bold = (strcmp(prop->value, "bold") == 0);
                            break;
                        case CSS_PROP_TEXT_ALIGN:
                            if (strcmp(prop->value, "center") == 0) out_style->text_align = 1;
                            else if (strcmp(prop->value, "right") == 0) out_style->text_align = 2;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    
    // Apply inline styles (highest priority)
    if (inline_style) {
        css_parse_inline(inline_style, out_style);
    }
}

void css_apply_style(const css_computed_style_t* style) {
    if (!style) return;
    
    uint8_t color = vga_entry_color(style->color, style->background_color);
    vga_set_color(color);
}

void css_reset_stylesheet(css_stylesheet_t* stylesheet) {
    if (stylesheet) {
        stylesheet->rule_count = 0;
    }
}
