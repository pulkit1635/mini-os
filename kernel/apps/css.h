#ifndef CSS_H
#define CSS_H

#include <stdint.h>
#include <stdbool.h>
#include "../vga.h"

// Maximum CSS rules and properties
#define CSS_MAX_RULES 64
#define CSS_MAX_PROPERTIES 16
#define CSS_MAX_SELECTOR_LEN 64
#define CSS_MAX_VALUE_LEN 64

// CSS color names to VGA colors
typedef struct {
    const char* name;
    uint8_t vga_color;
} css_color_map_t;

// CSS property types
typedef enum {
    CSS_PROP_COLOR,
    CSS_PROP_BACKGROUND_COLOR,
    CSS_PROP_FONT_WEIGHT,
    CSS_PROP_FONT_STYLE,
    CSS_PROP_TEXT_DECORATION,
    CSS_PROP_TEXT_ALIGN,
    CSS_PROP_DISPLAY,
    CSS_PROP_MARGIN,
    CSS_PROP_PADDING,
    CSS_PROP_BORDER,
    CSS_PROP_WIDTH,
    CSS_PROP_HEIGHT,
    CSS_PROP_UNKNOWN
} css_property_type_t;

// CSS property value
typedef struct {
    css_property_type_t type;
    char value[CSS_MAX_VALUE_LEN];
    int numeric_value;
} css_property_t;

// CSS rule (selector + properties)
typedef struct {
    char selector[CSS_MAX_SELECTOR_LEN];
    css_property_t properties[CSS_MAX_PROPERTIES];
    int property_count;
} css_rule_t;

// CSS stylesheet
typedef struct {
    css_rule_t rules[CSS_MAX_RULES];
    int rule_count;
} css_stylesheet_t;

// Computed style for an element
typedef struct {
    uint8_t color;
    uint8_t background_color;
    bool bold;
    bool italic;
    bool underline;
    int text_align;  // 0=left, 1=center, 2=right
    int margin_top;
    int margin_bottom;
    int margin_left;
    int margin_right;
    bool is_block;
    bool is_hidden;
} css_computed_style_t;

// Initialize CSS engine
void css_init(void);

// Parse CSS text and add to stylesheet
bool css_parse(const char* css_text, css_stylesheet_t* stylesheet);

// Parse inline style attribute
bool css_parse_inline(const char* style_attr, css_computed_style_t* style);

// Get computed style for an element
void css_compute_style(const char* tag_name, const char* class_name, 
                       const char* id, const char* inline_style,
                       const css_stylesheet_t* stylesheet,
                       css_computed_style_t* out_style);

// Convert CSS color to VGA color
uint8_t css_color_to_vga(const char* color);

// Get default style for a tag
void css_get_default_style(const char* tag_name, css_computed_style_t* style);

// Apply style to VGA
void css_apply_style(const css_computed_style_t* style);

// Parse a single CSS property
css_property_type_t css_parse_property_name(const char* name);

// Reset stylesheet
void css_reset_stylesheet(css_stylesheet_t* stylesheet);

#endif // CSS_H
