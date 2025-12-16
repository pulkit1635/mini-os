#ifndef JAVASCRIPT_H
#define JAVASCRIPT_H

#include <stdint.h>
#include <stdbool.h>

// JS engine limits
#define JS_MAX_VARIABLES 64
#define JS_MAX_FUNCTIONS 32
#define JS_MAX_STRING_LEN 256
#define JS_MAX_CALL_STACK 16
#define JS_MAX_CODE_LEN 4096

// Value types
typedef enum {
    JS_TYPE_UNDEFINED,
    JS_TYPE_NULL,
    JS_TYPE_BOOLEAN,
    JS_TYPE_NUMBER,
    JS_TYPE_STRING,
    JS_TYPE_FUNCTION,
    JS_TYPE_OBJECT
} js_type_t;

// Forward declarations
struct js_value;
struct js_context;

// JavaScript value
typedef struct js_value {
    js_type_t type;
    union {
        bool boolean;
        double number;
        char string[JS_MAX_STRING_LEN];
        struct {
            char name[64];
            char params[128];
            char body[512];
        } function;
    } data;
} js_value_t;

// Variable in scope
typedef struct {
    char name[64];
    js_value_t value;
} js_variable_t;

// Function definition
typedef struct {
    char name[64];
    char params[128];  // Comma-separated parameter names
    char body[512];    // Function body code
} js_function_t;

// DOM element (simplified)
typedef struct {
    char tag[32];
    char id[64];
    char class_name[64];
    char inner_text[256];
    char style[256];
    bool visible;
} js_dom_element_t;

// Callback types
typedef void (*js_alert_callback_t)(const char* message);
typedef void (*js_console_callback_t)(const char* message);
typedef js_dom_element_t* (*js_get_element_callback_t)(const char* id);
typedef void (*js_set_element_callback_t)(const char* id, const char* property, const char* value);
typedef void (*js_play_audio_callback_t)(const char* src);

// JavaScript execution context
typedef struct js_context {
    js_variable_t variables[JS_MAX_VARIABLES];
    int variable_count;
    
    js_function_t functions[JS_MAX_FUNCTIONS];
    int function_count;
    
    // Callbacks for browser integration
    js_alert_callback_t alert_callback;
    js_console_callback_t console_callback;
    js_get_element_callback_t get_element_callback;
    js_set_element_callback_t set_element_callback;
    js_play_audio_callback_t play_audio_callback;
    
    // Error handling
    bool has_error;
    char error_message[128];
    
    // Call stack for recursion
    int call_depth;
} js_context_t;

// Initialize JavaScript engine
void js_init(js_context_t* ctx);

// Set callbacks
void js_set_alert_callback(js_context_t* ctx, js_alert_callback_t callback);
void js_set_console_callback(js_context_t* ctx, js_console_callback_t callback);
void js_set_dom_callbacks(js_context_t* ctx, 
                          js_get_element_callback_t get_cb,
                          js_set_element_callback_t set_cb);
void js_set_audio_callback(js_context_t* ctx, js_play_audio_callback_t callback);

// Execute JavaScript code
js_value_t js_execute(js_context_t* ctx, const char* code);

// Evaluate an expression
js_value_t js_eval_expression(js_context_t* ctx, const char* expr);

// Get/set variables
js_value_t* js_get_variable(js_context_t* ctx, const char* name);
void js_set_variable(js_context_t* ctx, const char* name, js_value_t value);

// Create values
js_value_t js_undefined(void);
js_value_t js_null(void);
js_value_t js_boolean(bool value);
js_value_t js_number(double value);
js_value_t js_string(const char* value);

// Type conversion
double js_to_number(const js_value_t* value);
bool js_to_boolean(const js_value_t* value);
void js_to_string(const js_value_t* value, char* out, int max_len);

// Check for errors
bool js_has_error(const js_context_t* ctx);
const char* js_get_error(const js_context_t* ctx);
void js_clear_error(js_context_t* ctx);

// Reset context
void js_reset(js_context_t* ctx);

#endif // JAVASCRIPT_H
