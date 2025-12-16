#include "browser.h"
#include "css.h"
#include "javascript.h"
#include "../vga.h"
#include "../keyboard.h"
#include "../string.h"
#include "../memory.h"
#include "../io.h"
#include "../audio.h"

#define CONTENT_START_Y 2
#define STATUSBAR_Y (VGA_HEIGHT - 1)
#define CONTENT_HEIGHT (VGA_HEIGHT - 3)

static browser_state_t g;

// --- Small utilities ---
static bool starts_with(const char* s, const char* prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static const char* find_substr(const char* hay, const char* needle) {
    if (!hay || !needle || !*needle) return hay;
    size_t nlen = strlen(needle);
    for (const char* p = hay; *p; p++) {
        // Early exit if remaining shorter than needle
        const char* q = p;
        size_t i = 0;
        while (q[i] && i < nlen && q[i] == needle[i]) i++;
        if (i == nlen) return p;
    }
    return 0;
}

static void safe_strcpy(char* dst, const char* src, size_t max) {
    if (!dst || !src || max == 0) return;
    strncpy(dst, src, max - 1);
    dst[max - 1] = '\0';
}

static void safe_strcat(char* dst, const char* src, size_t max) {
    if (!dst || !src || max == 0) return;
    size_t dlen = strlen(dst);
    if (dlen >= max - 1) return;
    size_t to_copy = max - 1 - dlen;
    strncpy(dst + dlen, src, to_copy);
    dst[max - 1] = '\0';
}

// --- JS callbacks ---
static void js_cb_alert(const char* message) {
    browser_alert(message);
}

static void js_cb_console(const char* message) {
    browser_console_log(message);
}

static js_dom_element_t* js_cb_get_el(const char* id) {
    dom_element_t* el = browser_get_element_by_id(id);
    if (!el) return 0;
    // Static to keep lifetime; minimal and shared for simplicity
    static js_dom_element_t js_el;
    memset(&js_el, 0, sizeof(js_el));
    safe_strcpy(js_el.tag, el->tag, sizeof(js_el.tag));
    safe_strcpy(js_el.id, el->id, sizeof(js_el.id));
    safe_strcpy(js_el.class_name, el->class_name, sizeof(js_el.class_name));
    safe_strcpy(js_el.inner_text, el->inner_text, sizeof(js_el.inner_text));
    safe_strcpy(js_el.style, el->style, sizeof(js_el.style));
    js_el.visible = el->visible;
    return &js_el;
}

static void js_cb_set_el(const char* id, const char* prop, const char* value) {
    browser_set_element_property(id, prop, value);
}

static void js_cb_play_audio(const char* src) {
    browser_play_audio(src);
}

// --- UI drawing helpers ---
static void draw_titlebar(const char* title) {
    uint8_t old = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    for (int x = 0; x < VGA_WIDTH; x++) vga_putchar_at(' ', x, 0);
    int len = strlen(title);
    int start_x = (VGA_WIDTH - len) / 2;
    for (int i = 0; i < len; i++) vga_putchar_at(title[i], start_x + i, 0);
    vga_set_color(old);
}

static void draw_statusbar(void) {
    uint8_t old = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    for (int x = 0; x < VGA_WIDTH; x++) vga_putchar_at(' ', x, STATUSBAR_Y);
    char status[256];
    memset(status, 0, sizeof(status));
    safe_strcpy(status, "URL: ", sizeof(status));
    safe_strcat(status, g.current_url, sizeof(status));
    safe_strcat(status, "  |  Arrows: scroll  Tab: links  Enter: open  ESC: exit", sizeof(status));
    for (int i = 0; status[i] && i < VGA_WIDTH - 1; i++) vga_putchar_at(status[i], i + 1, STATUSBAR_Y);
    vga_set_color(old);
}

static void draw_console(void) {
    if (!g.show_console) return;
    uint8_t old = vga_get_color();
    int h = 5;
    int top = VGA_HEIGHT - 1 - h;
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_DARK_GREY));
    for (int y = top; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) vga_putchar_at(' ', x, y);
    }
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_DARK_GREY));
    vga_puts_at("[Console]", 1, top);
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
    // Show last lines of console_output
    int line = 0; int col = 0; int shown = 0;
    int y = top + 1;
    for (int i = 0; g.console_output[i] && y < VGA_HEIGHT - 1; i++) {
        char c = g.console_output[i];
        if (c == '\n' || col >= VGA_WIDTH - 2) {
            y++; col = 0; line++;
            if (y >= VGA_HEIGHT - 1) break;
            continue;
        }
        vga_putchar_at(c, 1 + col, y);
        col++;
        shown++;
    }
    vga_set_color(old);
}

static void draw_alert(void) {
    if (!g.show_alert) return;
    int w = 40, h = 5;
    int x0 = (VGA_WIDTH - w) / 2;
    int y0 = (VGA_HEIGHT - h) / 2;
    uint8_t old = vga_get_color();
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    vga_draw_box(x0, y0, w, h, vga_get_color());
    // Fill background
    for (int y = y0 + 1; y < y0 + h - 1; y++) {
        for (int x = x0 + 1; x < x0 + w - 1; x++) vga_putchar_at(' ', x, y);
    }
    vga_puts_at("Alert", x0 + 2, y0);
    vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    int len = strlen(g.alert_message);
    int sx = x0 + 2;
    int sy = y0 + 2;
    for (int i = 0; i < len && i < w - 4; i++) vga_putchar_at(g.alert_message[i], sx + i, sy);
    vga_set_color(old);
}

// --- Content rendering ---
typedef struct {
    int line_no; // absolute content line number
    int x;
} draw_cursor_t;

static void output_char(draw_cursor_t* c, char ch) {
    if (ch == '\n') {
        c->line_no++;
        c->x = 0;
        return;
    }
    if (c->x >= VGA_WIDTH - 2) { c->line_no++; c->x = 0; }
    int screen_y = CONTENT_START_Y + (c->line_no - g.view_offset);
    if (screen_y >= CONTENT_START_Y && screen_y < STATUSBAR_Y) {
        vga_putchar_at(ch, 1 + c->x, screen_y);
    }
    c->x++;
    if (c->x >= VGA_WIDTH - 2) { c->line_no++; c->x = 0; }
}

static void output_text(draw_cursor_t* c, const char* text) {
    for (int i = 0; text[i]; i++) output_char(c, text[i]);
}

static void add_link(int x, int line_no, const char* url, const char* onclick, int length) {
    if (g.link_count >= BROWSER_MAX_LINKS) return;
    browser_link_t* L = &g.links[g.link_count++];
    L->x = x; L->y = line_no; L->length = length;
    safe_strcpy(L->url, url ? url : "", sizeof(L->url));
    safe_strcpy(L->onclick, onclick ? onclick : "", sizeof(L->onclick));
}

static void clear_links(void) {
    g.link_count = 0; g.current_link = 0;
}

static void reset_dom(void) {
    g.element_count = 0;
}

static void add_dom_element(const char* tag, const char* id, const char* cls, const char* text, const char* style, int line_no, int x) {
    if (g.element_count >= BROWSER_MAX_ELEMENTS) return;
    dom_element_t* e = &g.elements[g.element_count++];
    memset(e, 0, sizeof(*e));
    safe_strcpy(e->tag, tag ? tag : "", sizeof(e->tag));
    safe_strcpy(e->id, id ? id : "", sizeof(e->id));
    safe_strcpy(e->class_name, cls ? cls : "", sizeof(e->class_name));
    safe_strcpy(e->inner_text, text ? text : "", sizeof(e->inner_text));
    safe_strcpy(e->style, style ? style : "", sizeof(e->style));
    e->line = line_no; e->x = x; e->visible = true;
}

static void apply_style_for(const char* tag, const char* cls, const char* id, const char* inline_style) {
    css_computed_style_t s;
    css_compute_style(tag ? tag : "", cls ? cls : "", id ? id : "", inline_style ? inline_style : "", &g.stylesheet, &s);
    css_apply_style(&s);
}

// Very small attribute parser: extracts value for given attribute name inside tag like: name="value"
static void parse_attr(const char* tag_src, const char* name, char* out, size_t outsz) {
    out[0] = '\0';
    const char* p = find_substr(tag_src, name);
    if (!p) return;
    p += strlen(name);
    while (*p == ' ' || *p == '=') { if (*p == '=') { p++; break; } p++; }
    if (*p == '\'' || *p == '"') {
        char q = *p++; size_t i = 0;
        while (*p && *p != q && i + 1 < outsz) out[i++] = *p++;
        out[i] = '\0';
    }
}

// Parse <style>...</style> and <script>...</script>, simple tags and text.
static void render_html(void) {
    uint8_t default_color = vga_get_color();
    clear_links();
    reset_dom();
    // Clear content area
    for (int y = CONTENT_START_Y; y < STATUSBAR_Y; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) vga_putchar_at(' ', x, y);
    }

    draw_cursor_t cur; cur.line_no = 0; cur.x = 0;

    const char* p = g.html_content;
    while (*p) {
        if (*p == '<') {
            // read tag
            const char* tag_start = p + 1;
            const char* tag_end = strchr(tag_start, '>');
            if (!tag_end) break;
            char tagbuf[256];
            size_t tlen = (size_t)(tag_end - tag_start);
            if (tlen >= sizeof(tagbuf)) tlen = sizeof(tagbuf) - 1;
            memcpy(tagbuf, tag_start, tlen); tagbuf[tlen] = '\0';

            bool closing = (tagbuf[0] == '/');
            const char* tagname = tagbuf + (closing ? 1 : 0);
            // Trim spaces
            while (*tagname == ' ') tagname++;

            if (!closing) {
                if (starts_with(tagname, "style")) {
                    // Collect until </style>
                    const char* body_start = tag_end + 1;
                    const char* close = find_substr(body_start, "</style>");
                    if (!close) close = body_start;
                    char cssbuf[512];
                    size_t blen = (size_t)(close - body_start);
                    if (blen >= sizeof(cssbuf)) blen = sizeof(cssbuf) - 1;
                    memcpy(cssbuf, body_start, blen); cssbuf[blen] = '\0';
                    css_parse(cssbuf, &g.stylesheet);
                    p = close + strlen("</style>");
                    continue;
                }
                if (starts_with(tagname, "script")) {
                    const char* body_start = tag_end + 1;
                    const char* close = find_substr(body_start, "</script>");
                    if (!close) close = body_start;
                    char jsbuf[512];
                    size_t blen = (size_t)(close - body_start);
                    if (blen >= sizeof(jsbuf)) blen = sizeof(jsbuf) - 1;
                    memcpy(jsbuf, body_start, blen); jsbuf[blen] = '\0';
                    browser_execute_js(jsbuf);
                    p = close + strlen("</script>");
                    continue;
                }
                if (starts_with(tagname, "audio")) {
                    char src[128]; char autoplay[16];
                    parse_attr(tagbuf, "src", src, sizeof(src));
                    parse_attr(tagbuf, "autoplay", autoplay, sizeof(autoplay));
                    if (src[0]) safe_strcpy(g.audio.src, src, sizeof(g.audio.src));
                    if (autoplay[0]) browser_play_audio(src);
                    p = tag_end + 1;
                    continue;
                }
                // Blocks
                if (starts_with(tagname, "h1") || starts_with(tagname, "h2") || starts_with(tagname, "h3") || starts_with(tagname, "p") || starts_with(tagname, "div")) {
                    // newline before block
                    if (cur.x != 0) { output_char(&cur, '\n'); }
                    // Apply style
                    apply_style_for(tagname, "", "", "");
                    p = tag_end + 1;
                    continue;
                }
                if (starts_with(tagname, "br")) {
                    output_char(&cur, '\n');
                    p = tag_end + 1; continue;
                }
                if (starts_with(tagname, "span")) {
                    char style_attr[256]; style_attr[0] = '\0';
                    parse_attr(tagbuf, "style", style_attr, sizeof(style_attr));
                    apply_style_for("span", "", "", style_attr);
                    p = tag_end + 1; continue;
                }
                if (starts_with(tagname, "a")) {
                    // Record start position; we'll add link when text closes or ends
                    char href[256]; char onclick[128];
                    href[0] = '\0'; onclick[0] = '\0';
                    parse_attr(tagbuf, "href", href, sizeof(href));
                    parse_attr(tagbuf, "onclick", onclick, sizeof(onclick));
                    // Store as a temporary marker in DOM: We'll add after we read text content until </a>
                    const char* text_start = tag_end + 1;
                    const char* close = find_substr(text_start, "</a>");
                    if (!close) close = text_start;
                    int link_len = 0;
                    // Count visible text length
                    for (const char* q = text_start; q < close; q++) if (*q != '\n' && *q != '\r' && *q != '\t') link_len++;
                    // Add DOM element and link
                    add_dom_element("a", "", "", "", "", cur.line_no, cur.x);
                    add_link(cur.x, cur.line_no, href, onclick, link_len);
                    // Render with underline-style coloring
                    css_computed_style_t s; css_get_default_style("a", &s); s.underline = true; css_apply_style(&s);
                    // Emit text
                    for (const char* q = text_start; q < close; q++) {
                        char ch = *q;
                        if (ch == '\n' || ch == '\r' || ch == '\t') ch = ' ';
                        output_char(&cur, ch);
                    }
                    // Reset to default after link text
                    vga_set_color(default_color);
                    p = close + strlen("</a>");
                    continue;
                }
                // Unknown tag: just skip
                p = tag_end + 1; continue;
            } else {
                // closing tags cause at most a newline for certain blocks
                if (starts_with(tagname, "p") || starts_with(tagname, "div") || starts_with(tagname, "h1") || starts_with(tagname, "h2") || starts_with(tagname, "h3")) {
                    output_char(&cur, '\n');
                }
                // Reset color/style after any closing tag
                vga_set_color(default_color);
                p = tag_end + 1; continue;
            }
        } else {
            // Text node
            char ch = *p++;
            if (ch == '\r') continue;
            output_char(&cur, ch);
        }
    }

    g.total_lines = cur.line_no + 1;

    // Highlight current link on screen
    if (g.link_count > 0 && g.current_link >= 0 && g.current_link < g.link_count) {
        browser_link_t* L = &g.links[g.current_link];
        int screen_y = CONTENT_START_Y + (L->y - g.view_offset);
        if (screen_y >= CONTENT_START_Y && screen_y < STATUSBAR_Y) {
            uint8_t old = vga_get_color();
            vga_set_color(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN));
            for (int i = 0; i < L->length && 1 + L->x + i < VGA_WIDTH - 1; i++) {
                uint16_t entry = vga_get_entry_at(1 + L->x + i, screen_y);
                char ch = (char)(entry & 0xFF);
                vga_putchar_at(ch, 1 + L->x + i, screen_y);
            }
            vga_set_color(old);
        }
    }
}

// --- Public API ---
void browser_init(void) {
    memset(&g, 0, sizeof(g));
    css_reset_stylesheet(&g.stylesheet);
    js_init(&g.js_context);
    js_set_alert_callback(&g.js_context, js_cb_alert);
    js_set_console_callback(&g.js_context, js_cb_console);
    js_set_dom_callbacks(&g.js_context, js_cb_get_el, js_cb_set_el);
    js_set_audio_callback(&g.js_context, js_cb_play_audio);
    g.view_offset = 0;
    g.show_console = false;
    g.show_alert = false;
    g.audio.playing = false; g.audio.volume = audio_get_volume();
    browser_home();
}

void browser_run(void) {
    vga_clear();
    draw_titlebar("MiniOS Browser");
    browser_render();

    bool running = true;
    while (running) {
        key_event_t ev = keyboard_get_key();
        if (ev.released) continue;

        if (g.show_alert) {
            if (ev.scancode == KEY_ENTER || ev.scancode == KEY_ESCAPE) {
                browser_dismiss_alert();
                browser_render();
            }
            continue;
        }

        switch (ev.scancode) {
            case KEY_ESCAPE:
                running = false;
                break;
            case KEY_UP:
                browser_scroll(-1);
                browser_render();
                break;
            case KEY_DOWN:
                browser_scroll(1);
                browser_render();
                break;
            case KEY_PAGEUP:
                browser_scroll(-CONTENT_HEIGHT);
                browser_render();
                break;
            case KEY_PAGEDOWN:
                browser_scroll(CONTENT_HEIGHT);
                browser_render();
                break;
            case KEY_TAB:
                if (ev.shift) browser_prev_link(); else browser_next_link();
                browser_render();
                break;
            case KEY_ENTER:
                browser_activate_link();
                browser_render();
                break;
            default:
                if (ev.ascii == 'c') { browser_toggle_console(); browser_render(); }
                else if (ev.ascii == 'h') { browser_home(); browser_render(); }
                else if (ev.ascii == 'r') { browser_load_html(g.html_content); browser_render(); }
                else if (ev.ascii == 's') { browser_stop_audio(); browser_render(); }
                break;
        }
    }
}

void browser_load_html(const char* html) {
    memset(g.html_content, 0, sizeof(g.html_content));
    g.html_length = 0;
    if (html) {
        safe_strcpy(g.html_content, html, sizeof(g.html_content));
        g.html_length = strlen(g.html_content);
    }
    // Reset state for new page
    css_reset_stylesheet(&g.stylesheet);
    g.view_offset = 0;
    g.total_lines = 0;
    clear_links();
    reset_dom();

    // Auto-extract <title>
    const char* t1 = find_substr(g.html_content, "<title>");
    const char* t2 = t1 ? find_substr(t1 + 7, "</title>") : 0;
    if (t1 && t2) {
        char tb[128]; size_t n = (size_t)(t2 - (t1 + 7)); if (n >= sizeof(tb)) n = sizeof(tb) - 1;
        memcpy(tb, t1 + 7, n); tb[n] = '\0';
        safe_strcpy(g.page_title, tb, sizeof(g.page_title));
    } else {
        safe_strcpy(g.page_title, "MiniOS Browser", sizeof(g.page_title));
    }
}

void browser_navigate(const char* url) {
    if (!url || !url[0]) return;
    safe_strcpy(g.current_url, url, sizeof(g.current_url));

    // Built-in pages
    if (strcmp(url, "about:home") == 0 || strcmp(url, "home") == 0) {
        browser_home();
        return;
    }
    if (strcmp(url, "test:css") == 0) {
        const char* html =
            "<title>CSS Test</title>"
            "<style> p { color: lightgreen; } a { color: cyan; }</style>"
            "<h1>CSS Test</h1><p>Paragraph in light green.</p>"
            "<p>Visit <a href=\"about:home\">Home</a></p>";
        browser_load_html(html);
        browser_render();
        return;
    }
    if (strcmp(url, "test:js") == 0) {
        const char* html =
            "<title>JS Test</title>"
            "<h1>JavaScript Test</h1>"
            "<script>console.log('Hello from JS');</script>"
            "<p><a href=\"#\" onclick=\"alert('Clicked!')\">Click for alert</a></p>"
            "<p><a href=\"#\" onclick=\"playAudio('beep')\">Play beep</a></p>";
        browser_load_html(html);
        browser_render();
        return;
    }
    if (strcmp(url, "test:audio") == 0) {
        const char* html =
            "<title>Audio Test</title>"
            "<h1>Audio Test</h1>"
            "<p>Auto beeps using &lt;audio&gt; tag.</p>"
            "<audio src=\"beep\" autoplay=\"1\"></audio>"
            "<p><a href=\"about:home\">Back</a></p>";
        browser_load_html(html);
        browser_render();
        return;
    }

    // Anything else: show simple message
    char htmlbuf[512];
    memset(htmlbuf, 0, sizeof(htmlbuf));
    safe_strcpy(htmlbuf, "<title>NAV</title><h1>Navigate</h1><p>Opening: ", sizeof(htmlbuf));
    safe_strcat(htmlbuf, url, sizeof(htmlbuf));
    safe_strcat(htmlbuf, "</p><p>Internet not available. Use built-ins: about:home, test:css, test:js, test:audio</p>", sizeof(htmlbuf));
    browser_load_html(htmlbuf);
    browser_render();
}

void browser_home(void) {
    safe_strcpy(g.current_url, "about:home", sizeof(g.current_url));
    const char* html =
        "<title>Home</title>"
        "<style> h1{color: lightcyan;} p{color: lightgrey;} a{color: lightmagenta;} </style>"
        "<h1>Welcome to MiniOS Browser</h1>"
        "<p>This is a minimal text-mode browser.</p>"
        "<p>Try: <a href=\"test:css\">CSS</a> | <a href=\"test:js\">JavaScript</a> | <a href=\"test:audio\">Audio</a></p>"
        "<p><span style=\"color: yellow; background-color: blue\">Inline styled text</span></p>"
        "<script>console.log('Home loaded');</script>";
    browser_load_html(html);
    browser_render();
}

void browser_render(void) {
    draw_titlebar(g.page_title[0] ? g.page_title : "MiniOS Browser");
    render_html();
    draw_statusbar();
    draw_console();
    draw_alert();
}

void browser_scroll(int delta) {
    int max_offset = g.total_lines - CONTENT_HEIGHT;
    if (max_offset < 0) max_offset = 0;
    g.view_offset += delta;
    if (g.view_offset < 0) g.view_offset = 0;
    if (g.view_offset > max_offset) g.view_offset = max_offset;
}

void browser_next_link(void) {
    if (g.link_count == 0) return;
    g.current_link = (g.current_link + 1) % g.link_count;
    // Auto-scroll if needed
    browser_link_t* L = &g.links[g.current_link];
    if (L->y < g.view_offset) g.view_offset = L->y;
    int bottom = g.view_offset + CONTENT_HEIGHT - 1;
    if (L->y > bottom) g.view_offset = L->y - CONTENT_HEIGHT + 1;
}

void browser_prev_link(void) {
    if (g.link_count == 0) return;
    g.current_link = (g.current_link - 1);
    if (g.current_link < 0) g.current_link = g.link_count - 1;
    browser_link_t* L = &g.links[g.current_link];
    if (L->y < g.view_offset) g.view_offset = L->y;
    int bottom = g.view_offset + CONTENT_HEIGHT - 1;
    if (L->y > bottom) g.view_offset = L->y - CONTENT_HEIGHT + 1;
}

void browser_activate_link(void) {
    if (g.link_count == 0) return;
    browser_link_t* L = &g.links[g.current_link];
    if (L->onclick[0]) {
        browser_execute_js(L->onclick);
    } else if (L->url[0]) {
        browser_navigate(L->url);
    }
}

void browser_execute_js(const char* code) {
    if (!code || !code[0]) return;
    js_clear_error(&g.js_context);
    js_value_t r = js_execute(&g.js_context, code);
    (void)r; // ignored
    if (js_has_error(&g.js_context)) {
        browser_console_log(js_get_error(&g.js_context));
        js_clear_error(&g.js_context);
    }
}

dom_element_t* browser_get_element_by_id(const char* id) {
    if (!id || !id[0]) return 0;
    for (int i = 0; i < g.element_count; i++) {
        if (strcmp(g.elements[i].id, id) == 0) return &g.elements[i];
    }
    return 0;
}

void browser_set_element_property(const char* id, const char* prop, const char* value) {
    dom_element_t* el = browser_get_element_by_id(id);
    if (!el) return;
    if (strcmp(prop, "innerText") == 0 || strcmp(prop, "innerHTML") == 0) {
        safe_strcpy(el->inner_text, value, sizeof(el->inner_text));
    } else if (strcmp(prop, "style") == 0) {
        safe_strcpy(el->style, value, sizeof(el->style));
    } else if (strcmp(prop, "visible") == 0) {
        el->visible = (value && value[0] != '0');
    }
    browser_render();
}

void browser_play_audio(const char* src) {
    if (!src || !src[0]) return;
    safe_strcpy(g.audio.src, src, sizeof(g.audio.src));
    // Map some names to effects/tones
    if (strcmp(src, "beep") == 0) {
        audio_play_effect(SFX_BEEP);
    } else if (strcmp(src, "success") == 0) {
        audio_play_effect(SFX_SUCCESS);
    } else if (strcmp(src, "error") == 0) {
        audio_play_effect(SFX_ERROR);
    } else if (starts_with(src, "tone:")) {
        int freq = atoi(src + 5);
        if (freq <= 0) freq = NOTE_A4;
        audio_play_tone((uint16_t)freq, 250);
    } else {
        // default beep
        audio_beep();
    }
    g.audio.playing = true;
}

void browser_stop_audio(void) {
    audio_stop();
    g.audio.playing = false;
}

void browser_toggle_console(void) {
    g.show_console = !g.show_console;
}

void browser_console_log(const char* message) {
    if (!message) return;
    if (strlen(g.console_output) + strlen(message) + 2 >= sizeof(g.console_output)) {
        // reset if overflow (keep simple)
        memset(g.console_output, 0, sizeof(g.console_output));
    }
    safe_strcat(g.console_output, message, sizeof(g.console_output));
    safe_strcat(g.console_output, "\n", sizeof(g.console_output));
}

void browser_alert(const char* message) {
    safe_strcpy(g.alert_message, message ? message : "", sizeof(g.alert_message));
    g.show_alert = true;
}

void browser_dismiss_alert(void) {
    g.show_alert = false;
}
