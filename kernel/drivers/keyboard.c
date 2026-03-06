/**
 * MatryoshkaOS - PS/2 Keyboard Driver
 * Scancode Set 1, US QWERTY layout
 */

#include <matryoshka/keyboard.h>
#include <matryoshka/idt.h>
#include <matryoshka/pic.h>
#include <matryoshka/io.h>
#include <matryoshka/vga.h>

/* ── Scancode Set 1 translation tables (US QWERTY) ───────────────── */

static const char sc_normal[128] = {
/*0x00*/  0,   27,  '1', '2', '3', '4', '5', '6',
/*0x08*/ '7', '8', '9', '0', '-', '=', '\b', '\t',
/*0x10*/ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
/*0x18*/ 'o', 'p', '[', ']', '\n',  0,  'a', 's',
/*0x20*/ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
/*0x28*/'\'', '`',  0, '\\', 'z', 'x', 'c', 'v',
/*0x30*/ 'b', 'n', 'm', ',', '.', '/',  0,  '*',
/*0x38*/  0,  ' ',  0,   0,   0,   0,   0,   0,
/*0x40*/  0,   0,   0,   0,   0,   0,   0,   0,
/*0x48*/  0,   0,  '-',  0,   0,   0,  '+',  0,
/*0x50*/  0,   0,   0,  '.',  0,   0,   0,   0,
};

static const char sc_shifted[128] = {
/*0x00*/  0,   27,  '!', '@', '#', '$', '%', '^',
/*0x08*/ '&', '*', '(', ')', '_', '+', '\b', '\t',
/*0x10*/ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
/*0x18*/ 'O', 'P', '{', '}', '\n',  0,  'A', 'S',
/*0x20*/ 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
/*0x28*/ '"', '~',  0,  '|', 'Z', 'X', 'C', 'V',
/*0x30*/ 'B', 'N', 'M', '<', '>', '?',  0,  '*',
/*0x38*/  0,  ' ',  0,   0,   0,   0,   0,   0,
/*0x40*/  0,   0,   0,   0,   0,   0,   0,   0,
/*0x48*/  0,   0,  '-',  0,   0,   0,  '+',  0,
/*0x50*/  0,   0,   0,  '.',  0,   0,   0,   0,
};

/* ── State ────────────────────────────────────────────────────────── */

static volatile uint8_t modifiers;
static volatile bool    extended_pending;

static char             kb_buffer[KB_BUFFER_SIZE];
static volatile uint8_t kb_head;
static volatile uint8_t kb_tail;

/* ── Helpers ──────────────────────────────────────────────────────── */

static void buf_put(char c) {
    uint8_t next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next == kb_tail)
        return;             /* buffer full — drop key */
    kb_buffer[kb_head] = c;
    kb_head = next;
}

static inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline char toggle_case(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

/* ── IRQ1 handler ─────────────────────────────────────────────────── */

static void keyboard_irq_handler(interrupt_frame_t *frame) {
    (void)frame;

    uint8_t scancode = inb(KB_DATA_PORT);

    /* Extended scancode prefix — remember and wait for next byte */
    if (scancode == 0xE0) {
        extended_pending = true;
        pic_send_eoi(1);
        return;
    }

    /* Skip extended-key make/break codes for now */
    if (extended_pending) {
        extended_pending = false;
        pic_send_eoi(1);
        return;
    }

    bool released = (scancode & 0x80) != 0;
    uint8_t key   = scancode & 0x7F;

    /* ── Modifier tracking ────────────────────────────────────────── */
    if (key == 0x2A) { /* Left Shift */
        if (released) modifiers &= ~KB_MOD_LSHIFT;
        else          modifiers |=  KB_MOD_LSHIFT;
        pic_send_eoi(1);
        return;
    }
    if (key == 0x36) { /* Right Shift */
        if (released) modifiers &= ~KB_MOD_RSHIFT;
        else          modifiers |=  KB_MOD_RSHIFT;
        pic_send_eoi(1);
        return;
    }
    if (key == 0x1D) { /* Ctrl */
        if (released) modifiers &= ~KB_MOD_CTRL;
        else          modifiers |=  KB_MOD_CTRL;
        pic_send_eoi(1);
        return;
    }
    if (key == 0x38) { /* Alt */
        if (released) modifiers &= ~KB_MOD_ALT;
        else          modifiers |=  KB_MOD_ALT;
        pic_send_eoi(1);
        return;
    }
    if (key == 0x3A && !released) { /* Caps Lock — toggle on press */
        modifiers ^= KB_MOD_CAPSLOCK;
        pic_send_eoi(1);
        return;
    }

    /* Ignore key-release events for normal keys */
    if (released) {
        pic_send_eoi(1);
        return;
    }

    /* ── Translate scancode → ASCII ───────────────────────────────── */
    bool shift = (modifiers & (KB_MOD_LSHIFT | KB_MOD_RSHIFT)) != 0;
    char c = shift ? sc_shifted[key] : sc_normal[key];

    if (c && is_alpha(c) && (modifiers & KB_MOD_CAPSLOCK))
        c = toggle_case(c);

    if (c)
        buf_put(c);

    pic_send_eoi(1);
}

/* ── Public API ───────────────────────────────────────────────────── */

void keyboard_init(void) {
    kb_head = 0;
    kb_tail = 0;
    modifiers = 0;
    extended_pending = false;

    /* Flush any pending data from the PS/2 controller */
    while (inb(KB_STATUS_PORT) & 0x01)
        inb(KB_DATA_PORT);

    idt_register_handler(33, keyboard_irq_handler);
    pic_enable_irq(1);

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Keyboard driver initialized (IRQ1)\n\n");
}

char keyboard_getchar(void) {
    while (kb_head == kb_tail)
        __asm__ volatile("hlt");

    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

bool keyboard_has_input(void) {
    return kb_head != kb_tail;
}

uint8_t keyboard_get_modifiers(void) {
    return modifiers;
}
