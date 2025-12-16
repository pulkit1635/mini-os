#include "keyboard.h"
#include "idt.h"
#include "io.h"
#include "vga.h"

// Keyboard buffer
static key_event_t key_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;
static int buffer_count = 0;

// Key state
static bool shift_held = false;
static bool ctrl_held = false;
static bool alt_held = false;
static bool capslock_on = false;

// US keyboard layout - lowercase
static const char scancode_ascii_lower[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // F1-F10
    0, 0,  // Num lock, scroll lock
    0, 0, 0, '-',  // Home, up, pgup, -
    0, 0, 0, '+',  // Left, 5, right, +
    0, 0, 0, 0, 0,  // End, down, pgdn, ins, del
    0, 0, 0,
    0, 0,  // F11, F12
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// US keyboard layout - uppercase/shifted
static const char scancode_ascii_upper[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // F1-F10
    0, 0,  // Num lock, scroll lock
    0, 0, 0, '-',  // Home, up, pgup, -
    0, 0, 0, '+',  // Left, 5, right, +
    0, 0, 0, 0, 0,  // End, down, pgdn, ins, del
    0, 0, 0,
    0, 0,  // F11, F12
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char scancode_to_ascii(uint8_t scancode, bool shift) {
    if (scancode >= 128) return 0;
    
    bool use_upper = shift ^ capslock_on;
    
    // For letters, apply capslock
    char lower = scancode_ascii_lower[scancode];
    if (lower >= 'a' && lower <= 'z') {
        return use_upper ? scancode_ascii_upper[scancode] : scancode_ascii_lower[scancode];
    }
    
    // For other characters, only use shift
    return shift ? scancode_ascii_upper[scancode] : scancode_ascii_lower[scancode];
}

static void keyboard_handler(registers_t* regs) {
    (void)regs;
    
    uint8_t scancode = inb(0x60);
    bool released = (scancode & KEY_RELEASED) != 0;
    uint8_t key = scancode & 0x7F;
    
    // Handle modifier keys
    switch (key) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_held = !released;
            return;
        case KEY_LCTRL:
            ctrl_held = !released;
            return;
        case KEY_LALT:
            alt_held = !released;
            return;
        case KEY_CAPSLOCK:
            if (!released) {
                capslock_on = !capslock_on;
            }
            return;
    }
    
    // Add to buffer if not full
    if (buffer_count < KEYBOARD_BUFFER_SIZE) {
        key_event_t event;
        event.scancode = key;
        event.ascii = scancode_to_ascii(key, shift_held);
        event.shift = shift_held;
        event.ctrl = ctrl_held;
        event.alt = alt_held;
        event.released = released;
        
        key_buffer[buffer_end] = event;
        buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
        buffer_count++;
    }
}

void keyboard_init(void) {
    // Register keyboard handler for IRQ1
    irq_register_handler(1, keyboard_handler);
    
    // Enable keyboard
    while (inb(0x64) & 0x01) {
        inb(0x60);  // Clear any pending data
    }
    
    // Enable keyboard interrupts
    uint8_t mask = inb(0x21);
    mask &= ~0x02;  // Clear bit 1 to enable IRQ1
    outb(0x21, mask);
}

bool keyboard_has_key(void) {
    return buffer_count > 0;
}

key_event_t keyboard_get_key(void) {
    // Wait for key
    while (buffer_count == 0) {
        hlt();  // Wait for interrupt
    }
    
    key_event_t event = key_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    buffer_count--;
    
    return event;
}

bool keyboard_try_get_key(key_event_t* event) {
    if (buffer_count == 0) {
        return false;
    }
    
    *event = key_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    buffer_count--;
    
    return true;
}

char keyboard_getchar(void) {
    while (1) {
        key_event_t event = keyboard_get_key();
        if (!event.released && event.ascii != 0) {
            return event.ascii;
        }
    }
}

bool keyboard_shift_held(void) {
    return shift_held;
}

bool keyboard_ctrl_held(void) {
    return ctrl_held;
}

bool keyboard_alt_held(void) {
    return alt_held;
}

void keyboard_readline(char* buffer, int max_length) {
    int pos = 0;
    
    while (pos < max_length - 1) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            vga_putchar('\n');
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                vga_putchar('\b');
            }
        } else if (c >= 32 && c < 127) {
            buffer[pos++] = c;
            vga_putchar(c);
        }
    }
    
    buffer[pos] = '\0';
}
