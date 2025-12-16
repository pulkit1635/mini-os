#include "javascript.h"
#include "../string.h"
#include "../memory.h"

// Forward declarations
static js_value_t eval_statement(js_context_t* ctx, const char** code);
static js_value_t eval_expression(js_context_t* ctx, const char** code);
static js_value_t eval_term(js_context_t* ctx, const char** code);
static js_value_t eval_factor(js_context_t* ctx, const char** code);

// Skip whitespace and comments
static void skip_whitespace(const char** code) {
    while (**code) {
        if (**code == ' ' || **code == '\t' || **code == '\n' || **code == '\r') {
            (*code)++;
        }
        else if (**code == '/' && *(*code + 1) == '/') {
            // Single-line comment
            while (**code && **code != '\n') (*code)++;
        }
        else if (**code == '/' && *(*code + 1) == '*') {
            // Multi-line comment
            *code += 2;
            while (**code && !(**code == '*' && *(*code + 1) == '/')) (*code)++;
            if (**code) *code += 2;
        }
        else {
            break;
        }
    }
}

// Parse identifier
static int parse_identifier(const char** code, char* out, int max_len) {
    skip_whitespace(code);
    int i = 0;
    
    if (isalpha(**code) || **code == '_' || **code == '$') {
        while ((isalnum(**code) || **code == '_' || **code == '$') && i < max_len - 1) {
            out[i++] = *(*code)++;
        }
    }
    out[i] = '\0';
    return i;
}

// Parse string literal
static int parse_string(const char** code, char* out, int max_len) {
    skip_whitespace(code);
    char quote = **code;
    if (quote != '"' && quote != '\'' && quote != '`') return 0;
    (*code)++;
    
    int i = 0;
    while (**code && **code != quote && i < max_len - 1) {
        if (**code == '\\' && *(*code + 1)) {
            (*code)++;
            switch (**code) {
                case 'n': out[i++] = '\n'; break;
                case 't': out[i++] = '\t'; break;
                case 'r': out[i++] = '\r'; break;
                case '\\': out[i++] = '\\'; break;
                default: out[i++] = **code; break;
            }
        } else {
            out[i++] = **code;
        }
        (*code)++;
    }
    out[i] = '\0';
    if (**code == quote) (*code)++;
    return i;
}

// Parse number
static double parse_number(const char** code) {
    skip_whitespace(code);
    double result = 0;
    int sign = 1;
    
    if (**code == '-') {
        sign = -1;
        (*code)++;
    } else if (**code == '+') {
        (*code)++;
    }
    
    // Integer part
    while (isdigit(**code)) {
        result = result * 10 + (**code - '0');
        (*code)++;
    }
    
    // Decimal part
    if (**code == '.') {
        (*code)++;
        double decimal = 0.1;
        while (isdigit(**code)) {
            result += (**code - '0') * decimal;
            decimal *= 0.1;
            (*code)++;
        }
    }
    
    return sign * result;
}

// Create value helpers
js_value_t js_undefined(void) {
    js_value_t v;
    v.type = JS_TYPE_UNDEFINED;
    return v;
}

js_value_t js_null(void) {
    js_value_t v;
    v.type = JS_TYPE_NULL;
    return v;
}

js_value_t js_boolean(bool value) {
    js_value_t v;
    v.type = JS_TYPE_BOOLEAN;
    v.data.boolean = value;
    return v;
}

js_value_t js_number(double value) {
    js_value_t v;
    v.type = JS_TYPE_NUMBER;
    v.data.number = value;
    return v;
}

js_value_t js_string(const char* value) {
    js_value_t v;
    v.type = JS_TYPE_STRING;
    strncpy(v.data.string, value, JS_MAX_STRING_LEN - 1);
    v.data.string[JS_MAX_STRING_LEN - 1] = '\0';
    return v;
}

// Type conversion
double js_to_number(const js_value_t* value) {
    switch (value->type) {
        case JS_TYPE_NUMBER: return value->data.number;
        case JS_TYPE_BOOLEAN: return value->data.boolean ? 1 : 0;
        case JS_TYPE_STRING: return atoi(value->data.string);
        default: return 0;
    }
}

bool js_to_boolean(const js_value_t* value) {
    switch (value->type) {
        case JS_TYPE_BOOLEAN: return value->data.boolean;
        case JS_TYPE_NUMBER: return value->data.number != 0;
        case JS_TYPE_STRING: return value->data.string[0] != '\0';
        case JS_TYPE_NULL:
        case JS_TYPE_UNDEFINED: return false;
        default: return true;
    }
}

void js_to_string(const js_value_t* value, char* out, int max_len) {
    switch (value->type) {
        case JS_TYPE_STRING:
            strncpy(out, value->data.string, max_len - 1);
            break;
        case JS_TYPE_NUMBER:
            itoa((int)value->data.number, out, 10);
            break;
        case JS_TYPE_BOOLEAN:
            strcpy(out, value->data.boolean ? "true" : "false");
            break;
        case JS_TYPE_NULL:
            strcpy(out, "null");
            break;
        case JS_TYPE_UNDEFINED:
            strcpy(out, "undefined");
            break;
        default:
            strcpy(out, "[object]");
            break;
    }
    out[max_len - 1] = '\0';
}

void js_init(js_context_t* ctx) {
    memset(ctx, 0, sizeof(js_context_t));
}

void js_reset(js_context_t* ctx) {
    ctx->variable_count = 0;
    ctx->function_count = 0;
    ctx->has_error = false;
    ctx->error_message[0] = '\0';
    ctx->call_depth = 0;
}

void js_set_alert_callback(js_context_t* ctx, js_alert_callback_t callback) {
    ctx->alert_callback = callback;
}

void js_set_console_callback(js_context_t* ctx, js_console_callback_t callback) {
    ctx->console_callback = callback;
}

void js_set_dom_callbacks(js_context_t* ctx, 
                          js_get_element_callback_t get_cb,
                          js_set_element_callback_t set_cb) {
    ctx->get_element_callback = get_cb;
    ctx->set_element_callback = set_cb;
}

void js_set_audio_callback(js_context_t* ctx, js_play_audio_callback_t callback) {
    ctx->play_audio_callback = callback;
}

js_value_t* js_get_variable(js_context_t* ctx, const char* name) {
    for (int i = 0; i < ctx->variable_count; i++) {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            return &ctx->variables[i].value;
        }
    }
    return NULL;
}

void js_set_variable(js_context_t* ctx, const char* name, js_value_t value) {
    // Check if variable exists
    for (int i = 0; i < ctx->variable_count; i++) {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            ctx->variables[i].value = value;
            return;
        }
    }
    
    // Add new variable
    if (ctx->variable_count < JS_MAX_VARIABLES) {
        strncpy(ctx->variables[ctx->variable_count].name, name, 63);
        ctx->variables[ctx->variable_count].value = value;
        ctx->variable_count++;
    }
}

bool js_has_error(const js_context_t* ctx) {
    return ctx->has_error;
}

const char* js_get_error(const js_context_t* ctx) {
    return ctx->error_message;
}

void js_clear_error(js_context_t* ctx) {
    ctx->has_error = false;
    ctx->error_message[0] = '\0';
}

static void set_error(js_context_t* ctx, const char* msg) {
    ctx->has_error = true;
    strncpy(ctx->error_message, msg, 127);
    ctx->error_message[127] = '\0';
}

// Call a function
static js_value_t call_function(js_context_t* ctx, const char* name, const char** args, int arg_count) {
    // Built-in functions
    if (strcmp(name, "alert") == 0) {
        if (arg_count > 0 && ctx->alert_callback) {
            ctx->alert_callback(args[0]);
        }
        return js_undefined();
    }
    
    if (strcmp(name, "console.log") == 0 || strcmp(name, "log") == 0) {
        if (arg_count > 0 && ctx->console_callback) {
            ctx->console_callback(args[0]);
        }
        return js_undefined();
    }
    
    if (strcmp(name, "parseInt") == 0) {
        if (arg_count > 0) {
            return js_number(atoi(args[0]));
        }
        return js_number(0);
    }
    
    if (strcmp(name, "String") == 0) {
        if (arg_count > 0) {
            return js_string(args[0]);
        }
        return js_string("");
    }
    
    if (strcmp(name, "Math.floor") == 0) {
        if (arg_count > 0) {
            return js_number((int)atoi(args[0]));
        }
        return js_number(0);
    }
    
    if (strcmp(name, "Math.random") == 0) {
        // Simple pseudo-random (not great, but works)
        static uint32_t seed = 12345;
        seed = seed * 1103515245 + 12345;
        return js_number((double)(seed % 1000) / 1000.0);
    }
    
    if (strcmp(name, "playAudio") == 0 || strcmp(name, "play") == 0) {
        if (arg_count > 0 && ctx->play_audio_callback) {
            ctx->play_audio_callback(args[0]);
        }
        return js_undefined();
    }
    
    // Look for user-defined function
    for (int i = 0; i < ctx->function_count; i++) {
        if (strcmp(ctx->functions[i].name, name) == 0) {
            if (ctx->call_depth >= JS_MAX_CALL_STACK) {
                set_error(ctx, "Maximum call stack exceeded");
                return js_undefined();
            }
            
            ctx->call_depth++;
            
            // User-defined functions currently ignore parameter lists and just run the stored body
            const char* body = ctx->functions[i].body;
            js_value_t result = js_execute(ctx, body);
            
            ctx->call_depth--;
            return result;
        }
    }
    
    return js_undefined();
}

// Evaluate a primary value (number, string, identifier, function call)
static js_value_t eval_factor(js_context_t* ctx, const char** code) {
    skip_whitespace(code);
    
    // Check for parenthesized expression
    if (**code == '(') {
        (*code)++;
        js_value_t result = eval_expression(ctx, code);
        skip_whitespace(code);
        if (**code == ')') (*code)++;
        return result;
    }
    
    // Check for string literal
    if (**code == '"' || **code == '\'' || **code == '`') {
        char str[JS_MAX_STRING_LEN];
        parse_string(code, str, JS_MAX_STRING_LEN);
        return js_string(str);
    }
    
    // Check for number
    if (isdigit(**code) || (**code == '-' && isdigit(*(*code + 1)))) {
        return js_number(parse_number(code));
    }
    
    // Check for boolean/null/undefined
    if (strncmp(*code, "true", 4) == 0 && !isalnum(*(*code + 4))) {
        *code += 4;
        return js_boolean(true);
    }
    if (strncmp(*code, "false", 5) == 0 && !isalnum(*(*code + 5))) {
        *code += 5;
        return js_boolean(false);
    }
    if (strncmp(*code, "null", 4) == 0 && !isalnum(*(*code + 4))) {
        *code += 4;
        return js_null();
    }
    if (strncmp(*code, "undefined", 9) == 0 && !isalnum(*(*code + 9))) {
        *code += 9;
        return js_undefined();
    }
    
    // Check for identifier or function call
    char ident[64];
    if (parse_identifier(code, ident, 64) > 0) {
        skip_whitespace(code);
        
        // Check for property access (simple)
        while (**code == '.') {
            (*code)++;
            char prop[64];
            parse_identifier(code, prop, 64);
            strcat(ident, ".");
            strcat(ident, prop);
            skip_whitespace(code);
        }
        
        // Check for function call
        if (**code == '(') {
            (*code)++;
            skip_whitespace(code);
            
            // Parse arguments
            const char* args[8];
            char arg_buffers[8][128];
            int arg_count = 0;
            
            while (**code && **code != ')' && arg_count < 8) {
                skip_whitespace(code);
                
                // Parse argument expression
                js_value_t arg_val = eval_expression(ctx, code);
                js_to_string(&arg_val, arg_buffers[arg_count], 128);
                args[arg_count] = arg_buffers[arg_count];
                arg_count++;
                
                skip_whitespace(code);
                if (**code == ',') (*code)++;
            }
            
            if (**code == ')') (*code)++;
            
            return call_function(ctx, ident, args, arg_count);
        }
        
        // Variable lookup
        js_value_t* var = js_get_variable(ctx, ident);
        if (var) {
            return *var;
        }
        
        return js_undefined();
    }
    
    return js_undefined();
}

// Evaluate multiplication/division
static js_value_t eval_term(js_context_t* ctx, const char** code) {
    js_value_t left = eval_factor(ctx, code);
    
    while (true) {
        skip_whitespace(code);
        char op = **code;
        
        if (op == '*' || op == '/' || op == '%') {
            (*code)++;
            js_value_t right = eval_factor(ctx, code);
            
            double l = js_to_number(&left);
            double r = js_to_number(&right);
            
            if (op == '*') left = js_number(l * r);
            else if (op == '/') left = js_number(r != 0 ? l / r : 0);
            else if (op == '%') left = js_number(r != 0 ? (int)l % (int)r : 0);
        } else {
            break;
        }
    }
    
    return left;
}

// Evaluate addition/subtraction and string concatenation
static js_value_t eval_additive(js_context_t* ctx, const char** code) {
    js_value_t left = eval_term(ctx, code);
    
    while (true) {
        skip_whitespace(code);
        char op = **code;
        
        if (op == '+' || op == '-') {
            (*code)++;
            js_value_t right = eval_term(ctx, code);
            
            // String concatenation
            if (op == '+' && (left.type == JS_TYPE_STRING || right.type == JS_TYPE_STRING)) {
                char l_str[JS_MAX_STRING_LEN];
                char r_str[JS_MAX_STRING_LEN];
                js_to_string(&left, l_str, JS_MAX_STRING_LEN);
                js_to_string(&right, r_str, JS_MAX_STRING_LEN);
                
                char result[JS_MAX_STRING_LEN];
                strncpy(result, l_str, JS_MAX_STRING_LEN - 1);
                int len = strlen(result);
                strncpy(result + len, r_str, JS_MAX_STRING_LEN - len - 1);
                result[JS_MAX_STRING_LEN - 1] = '\0';
                
                left = js_string(result);
            } else {
                double l = js_to_number(&left);
                double r = js_to_number(&right);
                left = js_number(op == '+' ? l + r : l - r);
            }
        } else {
            break;
        }
    }
    
    return left;
}

// Evaluate comparison
static js_value_t eval_comparison(js_context_t* ctx, const char** code) {
    js_value_t left = eval_additive(ctx, code);
    
    skip_whitespace(code);
    
    // Check for comparison operators
    if (strncmp(*code, "===", 3) == 0) {
        *code += 3;
        js_value_t right = eval_additive(ctx, code);
        if (left.type != right.type) return js_boolean(false);
        if (left.type == JS_TYPE_NUMBER) 
            return js_boolean(left.data.number == right.data.number);
        if (left.type == JS_TYPE_STRING)
            return js_boolean(strcmp(left.data.string, right.data.string) == 0);
        if (left.type == JS_TYPE_BOOLEAN)
            return js_boolean(left.data.boolean == right.data.boolean);
        return js_boolean(true);
    }
    if (strncmp(*code, "!==", 3) == 0) {
        *code += 3;
        js_value_t right = eval_additive(ctx, code);
        if (left.type != right.type) return js_boolean(true);
        if (left.type == JS_TYPE_NUMBER)
            return js_boolean(left.data.number != right.data.number);
        if (left.type == JS_TYPE_STRING)
            return js_boolean(strcmp(left.data.string, right.data.string) != 0);
        return js_boolean(false);
    }
    if (strncmp(*code, "==", 2) == 0) {
        *code += 2;
        js_value_t right = eval_additive(ctx, code);
        double l = js_to_number(&left);
        double r = js_to_number(&right);
        return js_boolean(l == r);
    }
    if (strncmp(*code, "!=", 2) == 0) {
        *code += 2;
        js_value_t right = eval_additive(ctx, code);
        double l = js_to_number(&left);
        double r = js_to_number(&right);
        return js_boolean(l != r);
    }
    if (strncmp(*code, "<=", 2) == 0) {
        *code += 2;
        js_value_t right = eval_additive(ctx, code);
        return js_boolean(js_to_number(&left) <= js_to_number(&right));
    }
    if (strncmp(*code, ">=", 2) == 0) {
        *code += 2;
        js_value_t right = eval_additive(ctx, code);
        return js_boolean(js_to_number(&left) >= js_to_number(&right));
    }
    if (**code == '<') {
        (*code)++;
        js_value_t right = eval_additive(ctx, code);
        return js_boolean(js_to_number(&left) < js_to_number(&right));
    }
    if (**code == '>') {
        (*code)++;
        js_value_t right = eval_additive(ctx, code);
        return js_boolean(js_to_number(&left) > js_to_number(&right));
    }
    
    return left;
}

// Evaluate logical operators
static js_value_t eval_expression(js_context_t* ctx, const char** code) {
    js_value_t left = eval_comparison(ctx, code);
    
    while (true) {
        skip_whitespace(code);
        
        if (strncmp(*code, "&&", 2) == 0) {
            *code += 2;
            js_value_t right = eval_comparison(ctx, code);
            left = js_boolean(js_to_boolean(&left) && js_to_boolean(&right));
        }
        else if (strncmp(*code, "||", 2) == 0) {
            *code += 2;
            js_value_t right = eval_comparison(ctx, code);
            left = js_boolean(js_to_boolean(&left) || js_to_boolean(&right));
        }
        else {
            break;
        }
    }
    
    return left;
}

// Evaluate a statement
static js_value_t eval_statement(js_context_t* ctx, const char** code) {
    skip_whitespace(code);
    
    // Empty statement
    if (**code == ';') {
        (*code)++;
        return js_undefined();
    }
    
    // Block
    if (**code == '{') {
        (*code)++;
        js_value_t result = js_undefined();
        while (**code && **code != '}') {
            result = eval_statement(ctx, code);
            skip_whitespace(code);
        }
        if (**code == '}') (*code)++;
        return result;
    }
    
    // Variable declaration
    if (strncmp(*code, "var ", 4) == 0 || strncmp(*code, "let ", 4) == 0 || strncmp(*code, "const ", 6) == 0) {
        if (strncmp(*code, "const ", 6) == 0) *code += 6;
        else *code += 4;
        
        skip_whitespace(code);
        char name[64];
        parse_identifier(code, name, 64);
        
        skip_whitespace(code);
        js_value_t value = js_undefined();
        
        if (**code == '=') {
            (*code)++;
            value = eval_expression(ctx, code);
        }
        
        js_set_variable(ctx, name, value);
        
        skip_whitespace(code);
        if (**code == ';') (*code)++;
        
        return value;
    }
    
    // Function declaration
    if (strncmp(*code, "function ", 9) == 0) {
        *code += 9;
        skip_whitespace(code);
        
        char name[64];
        parse_identifier(code, name, 64);
        
        skip_whitespace(code);
        
        // Parse parameters
        char params[128] = "";
        if (**code == '(') {
            (*code)++;
            int pi = 0;
            while (**code && **code != ')' && pi < 127) {
                params[pi++] = *(*code)++;
            }
            params[pi] = '\0';
            if (**code == ')') (*code)++;
        }
        
        skip_whitespace(code);
        
        // Parse body
        char body[512] = "";
        if (**code == '{') {
            (*code)++;
            int bi = 0;
            int brace_count = 1;
            while (**code && brace_count > 0 && bi < 511) {
                if (**code == '{') brace_count++;
                if (**code == '}') brace_count--;
                if (brace_count > 0) body[bi++] = *(*code);
                (*code)++;
            }
            body[bi] = '\0';
        }
        
        // Store function
        if (ctx->function_count < JS_MAX_FUNCTIONS) {
            strncpy(ctx->functions[ctx->function_count].name, name, 63);
            strncpy(ctx->functions[ctx->function_count].params, params, 127);
            strncpy(ctx->functions[ctx->function_count].body, body, 511);
            ctx->function_count++;
        }
        
        return js_undefined();
    }
    
    // If statement
    if (strncmp(*code, "if", 2) == 0 && !isalnum(*(*code + 2))) {
        *code += 2;
        skip_whitespace(code);
        
        if (**code == '(') {
            (*code)++;
            js_value_t cond = eval_expression(ctx, code);
            skip_whitespace(code);
            if (**code == ')') (*code)++;
            
            if (js_to_boolean(&cond)) {
                return eval_statement(ctx, code);
            } else {
                // Skip the if body
                skip_whitespace(code);
                if (**code == '{') {
                    int brace_count = 1;
                    (*code)++;
                    while (**code && brace_count > 0) {
                        if (**code == '{') brace_count++;
                        if (**code == '}') brace_count--;
                        (*code)++;
                    }
                } else {
                    // Single statement - skip to semicolon
                    while (**code && **code != ';') (*code)++;
                    if (**code == ';') (*code)++;
                }
                
                // Check for else
                skip_whitespace(code);
                if (strncmp(*code, "else", 4) == 0 && !isalnum(*(*code + 4))) {
                    *code += 4;
                    return eval_statement(ctx, code);
                }
            }
        }
        return js_undefined();
    }
    
    // While loop
    if (strncmp(*code, "while", 5) == 0 && !isalnum(*(*code + 5))) {
        *code += 5;
        skip_whitespace(code);
        
        const char* loop_start = *code;
        int iterations = 0;
        const int max_iterations = 1000;  // Prevent infinite loops
        
        if (**code == '(') {
            while (iterations < max_iterations) {
                *code = loop_start;
                (*code)++;  // Skip (
                js_value_t cond = eval_expression(ctx, code);
                skip_whitespace(code);
                if (**code == ')') (*code)++;
                
                if (!js_to_boolean(&cond)) break;
                
                eval_statement(ctx, code);
                iterations++;
            }
            
            // Skip past loop body
            if (iterations >= max_iterations) {
                set_error(ctx, "Maximum iterations exceeded");
            }
        }
        return js_undefined();
    }
    
    // For loop (simple)
    if (strncmp(*code, "for", 3) == 0 && !isalnum(*(*code + 3))) {
        *code += 3;
        skip_whitespace(code);
        
        if (**code == '(') {
            (*code)++;
            
            // Init
            eval_statement(ctx, code);
            
            const char* cond_start = *code;
            const char* body_start = NULL;
            
            int iterations = 0;
            const int max_iterations = 1000;
            
            while (iterations < max_iterations) {
                // Condition
                *code = cond_start;
                js_value_t cond = eval_expression(ctx, code);
                skip_whitespace(code);
                if (**code == ';') (*code)++;
                
                if (!js_to_boolean(&cond)) break;
                
                // Skip increment to find body
                const char* inc_start = *code;
                while (**code && **code != ')') (*code)++;
                if (**code == ')') (*code)++;
                
                if (body_start == NULL) body_start = *code;
                
                // Execute body
                *code = body_start;
                eval_statement(ctx, code);
                
                // Execute increment
                *code = inc_start;
                eval_expression(ctx, code);
                
                iterations++;
            }
            
            // Skip past loop
            skip_whitespace(code);
            if (**code == '{') {
                int brace = 1;
                (*code)++;
                while (**code && brace > 0) {
                    if (**code == '{') brace++;
                    if (**code == '}') brace--;
                    (*code)++;
                }
            }
        }
        return js_undefined();
    }
    
    // Return statement
    if (strncmp(*code, "return", 6) == 0 && !isalnum(*(*code + 6))) {
        *code += 6;
        skip_whitespace(code);
        
        js_value_t result = js_undefined();
        if (**code != ';' && **code != '}') {
            result = eval_expression(ctx, code);
        }
        
        skip_whitespace(code);
        if (**code == ';') (*code)++;
        
        return result;
    }
    
    // Expression statement (including assignments)
    char ident[64];
    const char* start = *code;
    
    if (parse_identifier(code, ident, 64) > 0) {
        skip_whitespace(code);
        
        // Check for assignment
        if (**code == '=' && *(*code + 1) != '=') {
            (*code)++;
            js_value_t value = eval_expression(ctx, code);
            js_set_variable(ctx, ident, value);
            
            skip_whitespace(code);
            if (**code == ';') (*code)++;
            
            return value;
        }
        
        // Check for compound assignment
        if (strncmp(*code, "+=", 2) == 0 || strncmp(*code, "-=", 2) == 0 ||
            strncmp(*code, "*=", 2) == 0 || strncmp(*code, "/=", 2) == 0) {
            char op = **code;
            *code += 2;
            
            js_value_t* var = js_get_variable(ctx, ident);
            js_value_t right = eval_expression(ctx, code);
            
            double l = var ? js_to_number(var) : 0;
            double r = js_to_number(&right);
            
            js_value_t result;
            switch (op) {
                case '+': result = js_number(l + r); break;
                case '-': result = js_number(l - r); break;
                case '*': result = js_number(l * r); break;
                case '/': result = js_number(r != 0 ? l / r : 0); break;
                default: result = js_number(l); break;
            }
            
            js_set_variable(ctx, ident, result);
            
            skip_whitespace(code);
            if (**code == ';') (*code)++;
            
            return result;
        }
        
        // Check for increment/decrement
        if (strncmp(*code, "++", 2) == 0 || strncmp(*code, "--", 2) == 0) {
            char op = **code;
            *code += 2;
            
            js_value_t* var = js_get_variable(ctx, ident);
            double val = var ? js_to_number(var) : 0;
            
            js_value_t result = js_number(op == '+' ? val + 1 : val - 1);
            js_set_variable(ctx, ident, result);
            
            skip_whitespace(code);
            if (**code == ';') (*code)++;
            
            return result;
        }
    }
    
    // Reset and evaluate as expression
    *code = start;
    js_value_t result = eval_expression(ctx, code);
    
    skip_whitespace(code);
    if (**code == ';') (*code)++;
    
    return result;
}

js_value_t js_execute(js_context_t* ctx, const char* code) {
    js_value_t last_result = js_undefined();
    
    while (*code) {
        skip_whitespace(&code);
        if (!*code) break;
        
        last_result = eval_statement(ctx, &code);
        
        if (ctx->has_error) break;
    }
    
    return last_result;
}

js_value_t js_eval_expression(js_context_t* ctx, const char* expr) {
    return eval_expression(ctx, &expr);
}
