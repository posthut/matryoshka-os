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
#include <matryoshka/vmm.h>
#include <matryoshka/vfs.h>
#include <matryoshka/e1000.h>
#include <matryoshka/net.h>
#include <matryoshka/syscall.h>

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
static void cmd_virt(const char *args);
static void cmd_ls(const char *args);
static void cmd_cat(const char *args);
static void cmd_mkdir(const char *args);
static void cmd_touch(const char *args);
static void cmd_write(const char *args);
static void cmd_net(const char *args);
static void cmd_syscall(const char *args);
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
    { "virt",    "Virtual memory info",      cmd_virt    },
    { "ls",      "List directory [path]",    cmd_ls      },
    { "cat",     "Show file contents",       cmd_cat     },
    { "mkdir",   "Create directory",         cmd_mkdir   },
    { "touch",   "Create empty file",        cmd_touch   },
    { "write",   "Write text: write PATH text", cmd_write },
    { "net",     "Network status",            cmd_net     },
    { "syscall", "Run user-mode syscall demo",cmd_syscall },
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

static void print_hex32(uint32_t v) {
    const char hex[] = "0123456789ABCDEF";
    char buf[9];
    for (int i = 7; i >= 0; i--) { buf[i] = hex[v & 0xF]; v >>= 4; }
    buf[8] = '\0';
    vga_puts(buf);
}

static void cmd_virt(const char *args) {
    (void)args;
    vmm_stats_t vs;
    vmm_get_stats(&vs);

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Virtual Memory (32-bit paging)\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    vga_puts("  Mapped pages:   ");
    print_uint(vs.mapped_pages);
    vga_puts(" (");
    print_size((uint64_t)vs.mapped_pages * 4096);
    vga_puts(")\n");

    vga_puts("  Page tables:    ");
    print_uint(vs.page_tables);
    vga_putchar('\n');

    vga_puts("  Identity end:   0x");
    print_hex32(vs.identity_end);
    vga_putchar('\n');

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    vga_puts("  CR3:            0x");
    print_hex32(cr3);
    vga_putchar('\n');
}

static void cmd_ls(const char *args) {
    const char *path = skip_spaces(args);
    if (*path == '\0') path = "/";

    vfs_node_t *dir = vfs_resolve(path);
    if (!dir) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ls: not found: ");
        vga_puts(path);
        vga_putchar('\n');
        return;
    }
    if (!(dir->flags & VFS_DIRECTORY)) {
        vga_puts(dir->name);
        vga_puts("  ");
        print_uint(dir->size);
        vga_puts(" B\n");
        return;
    }

    char name[VFS_NAME_MAX];
    uint32_t type;
    for (uint32_t i = 0; vfs_readdir(path, i, name, &type) == 0; i++) {
        if (type & VFS_DIRECTORY) {
            vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
            vga_puts(name);
            vga_putchar('/');
        } else {
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            vga_puts(name);
        }
        vga_puts("  ");
    }
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_putchar('\n');
}

static void cmd_cat(const char *args) {
    const char *path = skip_spaces(args);
    if (*path == '\0') {
        vga_puts("Usage: cat <path>\n");
        return;
    }

    int fd = vfs_open(path, VFS_O_RDONLY);
    if (fd < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("cat: not found: ");
        vga_puts(path);
        vga_putchar('\n');
        return;
    }

    char buf[128];
    int n;
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    while ((n = vfs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        vga_puts(buf);
    }
    vfs_close(fd);
}

static void cmd_mkdir(const char *args) {
    const char *path = skip_spaces(args);
    if (*path == '\0') {
        vga_puts("Usage: mkdir <path>\n");
        return;
    }
    if (vfs_mkdir(path) < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("mkdir: failed: ");
        vga_puts(path);
        vga_putchar('\n');
    }
}

static void cmd_touch(const char *args) {
    const char *path = skip_spaces(args);
    if (*path == '\0') {
        vga_puts("Usage: touch <path>\n");
        return;
    }
    if (!vfs_create_file(path)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("touch: failed: ");
        vga_puts(path);
        vga_putchar('\n');
    }
}

static void cmd_write(const char *args) {
    const char *p = skip_spaces(args);
    if (*p == '\0') {
        vga_puts("Usage: write <path> <text>\n");
        return;
    }

    /* Extract path (first token) */
    char path[128];
    size_t i = 0;
    while (*p && *p != ' ' && i < sizeof(path) - 1) path[i++] = *p++;
    path[i] = '\0';

    const char *text = skip_spaces(p);
    if (*text == '\0') {
        vga_puts("Usage: write <path> <text>\n");
        return;
    }

    int fd = vfs_open(path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC);
    if (fd < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("write: cannot open: ");
        vga_puts(path);
        vga_putchar('\n');
        return;
    }
    vfs_write(fd, text, strlen(text));
    vfs_write(fd, "\n", 1);
    vfs_close(fd);
}

static void cmd_net(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Network Status\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    uint8_t mac[6];
    e1000_get_mac(mac);
    vga_puts("  MAC:    ");
    for (int i = 0; i < 6; i++) {
        const char hex[] = "0123456789ABCDEF";
        vga_putchar(hex[mac[i] >> 4]);
        vga_putchar(hex[mac[i] & 0xF]);
        if (i < 5) vga_putchar(':');
    }
    vga_putchar('\n');

    uint8_t ip[4];
    net_get_ip(ip);
    vga_puts("  IP:     ");
    for (int i = 0; i < 4; i++) {
        print_uint(ip[i]);
        if (i < 3) vga_putchar('.');
    }
    vga_putchar('\n');

    vga_puts("  Link:   ");
    if (e1000_link_up()) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("UP");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("DOWN");
    }
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_putchar('\n');

    vga_puts("  RX:     ");
    print_uint(e1000_packets_rx());
    vga_puts(" packets\n");

    vga_puts("  TX:     ");
    print_uint(e1000_packets_tx());
    vga_puts(" packets\n");
}

/**
 * User-mode demo: this function runs in ring 3 and communicates
 * with the kernel exclusively through INT 0x80 system calls.
 */
static void user_demo_entry(void) {
    /* sys_write(1, msg, len) — write to VGA */
    char msg1[] = {'[', 'U', 'S', 'E', 'R', ']', ' ',
                   'H', 'e', 'l', 'l', 'o', ' ', 'f', 'r', 'o', 'm',
                   ' ', 'R', 'i', 'n', 'g', ' ', '3', '!', '\n', '\0'};
    _syscall3(SYS_WRITE, 1, (uint32_t)msg1, 26);

    /* sys_getpid() */
    int32_t pid = _syscall0(SYS_GETPID);
    char pidmsg[] = {'[', 'U', 'S', 'E', 'R', ']', ' ',
                     'P', 'I', 'D', '=', '0' + (pid % 10), '\n', '\0'};
    _syscall3(SYS_WRITE, 1, (uint32_t)pidmsg, 13);

    /* sys_uptime() */
    int32_t ticks = _syscall0(SYS_UPTIME);
    char tmsg[] = {'[', 'U', 'S', 'E', 'R', ']', ' ',
                   'T', 'i', 'c', 'k', 's', '=', '0', '0', '0', '0', '0', '\n', '\0'};
    for (int i = 17; i >= 13; i--) {
        tmsg[i] = '0' + (ticks % 10);
        ticks /= 10;
    }
    _syscall3(SYS_WRITE, 1, (uint32_t)tmsg, 19);

    char done[] = {'[', 'U', 'S', 'E', 'R', ']', ' ',
                   'E', 'x', 'i', 't', 'i', 'n', 'g', '\n', '\0'};
    _syscall3(SYS_WRITE, 1, (uint32_t)done, 15);

    /* sys_exit(0) */
    _syscall1(SYS_EXIT, 0);
}

static void cmd_syscall(const char *args) {
    (void)args;
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Spawning user-mode (ring 3) task...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Mark the page containing user_demo_entry as user-accessible */
    uint32_t id = task_create_user(user_demo_entry, "user_demo");
    if (id == (uint32_t)-1) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("  ERROR: failed to create user task\n");
        return;
    }

    vga_puts("  Task created (id=");
    print_uint(id);
    vga_puts("), will run on next schedule tick\n");
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
