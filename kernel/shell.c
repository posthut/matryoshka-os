/**
 * MatryoshkaOS - Kernel Debug Shell
 * Built-in commands: help, clear, meminfo, uptime, echo, reboot
 */

#include <matryoshka/shell.h>
#include <matryoshka/keyboard.h>
#include <matryoshka/vga.h>
#include <matryoshka/timer.h>
#include <matryoshka/pmm.h>
#include <matryoshka/heap.h>
#include <matryoshka/io.h>
#include <matryoshka/string.h>
#include <matryoshka/task.h>

#define CMD_MAX 256

/* ── Helpers ──────────────────────────────────────────────────────── */

static void print_uint(uint64_t n) {
    if (n == 0) { vga_putchar('0'); return; }
    char buf[21];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (--i >= 0) vga_putchar(buf[i]);
}

static void print_size(uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        print_uint(bytes / (1024ULL * 1024 * 1024)); vga_puts(" GB");
    } else if (bytes >= 1024 * 1024) {
        print_uint(bytes / (1024 * 1024)); vga_puts(" MB");
    } else if (bytes >= 1024) {
        print_uint(bytes / 1024); vga_puts(" KB");
    } else {
        print_uint(bytes); vga_puts(" B");
    }
}

static const char *skip_spaces(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ── Command handlers ─────────────────────────────────────────────── */

static void cmd_help(const char *args);
static void cmd_clear(const char *args);
static void cmd_meminfo(const char *args);
static void cmd_uptime(const char *args);
static void cmd_ps(const char *args);
static void cmd_echo(const char *args);
static void cmd_reboot(const char *args);

typedef struct {
    const char *name;
    const char *desc;
    void (*handler)(const char *args);
} shell_cmd_t;

static const shell_cmd_t commands[] = {
    { "help",    "Show available commands",   cmd_help    },
    { "clear",   "Clear the screen",          cmd_clear   },
    { "meminfo", "Display memory statistics", cmd_meminfo },
    { "uptime",  "Show system uptime",        cmd_uptime  },
    { "ps",      "List kernel tasks",         cmd_ps      },
    { "echo",    "Print text to screen",      cmd_echo    },
    { "reboot",  "Restart the system",        cmd_reboot  },
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Available commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        vga_puts("  ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_puts(commands[i].name);
        size_t pad = 10 - strlen(commands[i].name);
        while (pad--) vga_putchar(' ');
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_puts(commands[i].desc);
        vga_putchar('\n');
    }
}

static void cmd_clear(const char *args) {
    (void)args;
    vga_clear(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void cmd_meminfo(const char *args) {
    (void)args;

    pmm_stats_t ps;
    pmm_get_stats(&ps);

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Physical Memory:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Total: "); print_size(ps.total_memory); vga_putchar('\n');
    vga_puts("  Free:  "); print_size(ps.free_memory);  vga_putchar('\n');
    vga_puts("  Used:  "); print_size(ps.used_memory);  vga_putchar('\n');

    size_t h_total, h_used, h_free;
    heap_get_stats(&h_total, &h_used, &h_free);

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Kernel Heap:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Total: "); print_size(h_total); vga_putchar('\n');
    vga_puts("  Used:  "); print_size(h_used);  vga_putchar('\n');
    vga_puts("  Free:  "); print_size(h_free);  vga_putchar('\n');
}

static void cmd_uptime(const char *args) {
    (void)args;

    uint64_t ticks = timer_get_ticks();
    uint32_t sec   = timer_get_uptime_sec();
    uint32_t d = sec / 86400;  sec %= 86400;
    uint32_t h = sec / 3600;   sec %= 3600;
    uint32_t m = sec / 60;     sec %= 60;

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Uptime: ");
    print_uint(d); vga_puts("d ");
    print_uint(h); vga_puts("h ");
    print_uint(m); vga_puts("m ");
    print_uint(sec); vga_puts("s (");
    print_uint(ticks);
    vga_puts(" ticks)\n");
}

static void cmd_ps(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("  PID  STATE       NAME\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    uint32_t n = task_count();
    for (uint32_t i = 0; i < n; i++) {
        uint32_t id;
        const char *name;
        task_state_t st;
        if (!task_get_info(i, &id, &name, &st)) continue;

        vga_puts("  ");
        print_uint(id);
        size_t pad = 5 - (id >= 10 ? 2 : 1);
        while (pad--) vga_putchar(' ');

        switch (st) {
        case TASK_RUNNING:    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
                              vga_puts("RUNNING    "); break;
        case TASK_READY:      vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
                              vga_puts("READY      "); break;
        case TASK_TERMINATED: vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
                              vga_puts("TERMINATED "); break;
        }
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_puts(name);
        vga_putchar('\n');
    }
}

static void cmd_echo(const char *args) {
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts(args);
    vga_putchar('\n');
}

static void cmd_reboot(const char *args) {
    (void)args;
    vga_puts("Rebooting...\n");
    uint8_t status;
    do { status = inb(0x64); } while (status & 0x02);
    outb(0x64, 0xFE);
    __asm__ volatile("cli; hlt");
}

/* ── Line reading ─────────────────────────────────────────────────── */

static char line_buf[CMD_MAX];
static uint32_t line_len;

static void shell_readline(void) {
    line_len = 0;
    while (1) {
        char c = keyboard_getchar();
        if (c == '\n') {
            line_buf[line_len] = '\0';
            vga_putchar('\n');
            return;
        }
        if (c == '\b') {
            if (line_len > 0) {
                line_len--;
                vga_putchar('\b');
                vga_putchar(' ');
                vga_putchar('\b');
            }
            continue;
        }
        if (c >= ' ' && c <= '~' && line_len < CMD_MAX - 1) {
            line_buf[line_len++] = c;
            vga_putchar(c);
        }
    }
}

/* ── Command dispatch ─────────────────────────────────────────────── */

static void shell_execute(void) {
    const char *p = skip_spaces(line_buf);
    if (*p == '\0') return;

    const char *cmd_start = p;
    while (*p && *p != ' ') p++;
    size_t cmd_name_len = (size_t)(p - cmd_start);

    const char *args = skip_spaces(p);

    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        if (strlen(commands[i].name) == cmd_name_len &&
            strncmp(commands[i].name, cmd_start, cmd_name_len) == 0) {
            commands[i].handler(args);
            return;
        }
    }

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("Unknown command: ");
    for (size_t i = 0; i < cmd_name_len; i++) vga_putchar(cmd_start[i]);
    vga_putchar('\n');
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Type 'help' for available commands.\n");
}

/* ── Public API ───────────────────────────────────────────────────── */

static void shell_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("mshka> ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void shell_run(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("MatryoshkaOS Shell v0.1\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Type 'help' for available commands.\n\n");

    while (1) {
        shell_prompt();
        shell_readline();
        shell_execute();
    }
}
