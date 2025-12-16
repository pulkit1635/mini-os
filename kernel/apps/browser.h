#ifndef BROWSER_H
#define BROWSER_H

#include <stdint.h>
#include <stdbool.h>
#include "css.h"
#include "javascript.h"

// Maximum sizes
#define BROWSER_MAX_HTML_SIZE 16384
#define BROWSER_MAX_LINKS 64
#define BROWSER_MAX_URL_LENGTH 256
#define BROWSER_MAX_ELEMENTS 128
#define BROWSER_MAX_SCRIPTS 8

// Link structure
typedef struct {
    int x, y;
    int length;
    char url[256];
    char onclick[128];  // JavaScript onclick handler
} browser_link_t;

// DOM element for JavaScript interaction
typedef struct {
    char tag[32];
    char id[64];
    char class_name[64];
    char inner_text[256];
    char style[256];
    int line;
    int x;
    bool visible;
} dom_element_t;

// Audio track
typedef struct {
    char src[128];
    bool playing;
    bool loop;
    int volume;
} audio_track_t;

// Browser state
typedef struct {
    // HTML content
    char html_content[BROWSER_MAX_HTML_SIZE];
    int html_length;
    
    // CSS stylesheet
    css_stylesheet_t stylesheet;
    
    // JavaScript context
    js_context_t js_context;
    
    // DOM elements
    dom_element_t elements[BROWSER_MAX_ELEMENTS];
    int element_count;
    
    // Links
    browser_link_t links[BROWSER_MAX_LINKS];
    int link_count;
    int current_link;
    
    // Scroll position
    int view_offset;
    int total_lines;
    
    // Current URL
    char current_url[BROWSER_MAX_URL_LENGTH];
    char page_title[128];
    
    // Audio
    audio_track_t audio;
    
    // Console output
    char console_output[512];
    bool show_console;
    
    // Alert message
    char alert_message[256];
    bool show_alert;
} browser_state_t;

// Initialize browser
void browser_init(void);

// Run browser main loop
void browser_run(void);

// Load HTML with CSS and JS
void browser_load_html(const char* html);

// Navigate to URL
void browser_navigate(const char* url);

// Go to home page
void browser_home(void);

// Render current page
void browser_render(void);

// Scroll page
void browser_scroll(int delta);

// Link navigation
void browser_next_link(void);
void browser_prev_link(void);
void browser_activate_link(void);

// JavaScript integration
void browser_execute_js(const char* code);
dom_element_t* browser_get_element_by_id(const char* id);
void browser_set_element_property(const char* id, const char* prop, const char* value);

// Audio control
void browser_play_audio(const char* src);
void browser_stop_audio(void);

// Console
void browser_toggle_console(void);
void browser_console_log(const char* message);

// Alert
void browser_alert(const char* message);
void browser_dismiss_alert(void);

#endif // BROWSER_H
