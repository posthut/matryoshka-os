/**
 * MatryoshkaOS - RAM Filesystem (ramfs)
 *
 * Simple in-memory filesystem backed by kmalloc.
 * Each file stores its contents in a dynamically grown buffer.
 * Directories are linked lists of vfs_node children.
 */

#include <matryoshka/vfs.h>
#include <matryoshka/heap.h>
#include <matryoshka/string.h>
#include <matryoshka/vga.h>

/* Per-file payload: growable data buffer */
typedef struct {
    uint8_t *data;
    uint32_t capacity;
} ramfs_file_t;

static uint32_t next_inode = 1;

/* ── Forward declarations ─────────────────────────────────────────── */

static int         ramfs_read(vfs_node_t *node, uint32_t off,
                              uint32_t size, uint8_t *buf);
static int         ramfs_write(vfs_node_t *node, uint32_t off,
                               uint32_t size, const uint8_t *buf);
static vfs_node_t *ramfs_lookup(vfs_node_t *dir, const char *name);
static int         ramfs_readdir(vfs_node_t *dir, uint32_t index,
                                 char *name_out, uint32_t *type_out);
static vfs_node_t *ramfs_create(vfs_node_t *parent, const char *name,
                                uint32_t flags);

static vfs_ops_t ramfs_ops = {
    .read    = ramfs_read,
    .write   = ramfs_write,
    .lookup  = ramfs_lookup,
    .readdir = ramfs_readdir,
    .create  = ramfs_create,
};

/* ── Helpers ──────────────────────────────────────────────────────── */

static vfs_node_t *alloc_node(const char *name, uint32_t flags) {
    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!node) return NULL;
    memset(node, 0, sizeof(*node));

    size_t len = strlen(name);
    if (len >= VFS_NAME_MAX) len = VFS_NAME_MAX - 1;
    memcpy(node->name, name, len);
    node->name[len] = '\0';

    node->flags = flags;
    node->inode = next_inode++;
    node->ops   = &ramfs_ops;

    if (flags & VFS_FILE) {
        ramfs_file_t *fd = (ramfs_file_t *)kmalloc(sizeof(ramfs_file_t));
        if (fd) {
            fd->data     = NULL;
            fd->capacity = 0;
        }
        node->fs_data = fd;
    }

    return node;
}

static bool ensure_capacity(ramfs_file_t *f, uint32_t needed) {
    if (needed <= f->capacity) return true;
    uint32_t new_cap = f->capacity ? f->capacity : 64;
    while (new_cap < needed) new_cap *= 2;

    uint8_t *new_data = (uint8_t *)kmalloc(new_cap);
    if (!new_data) return false;

    if (f->data) {
        memcpy(new_data, f->data, f->capacity);
        kfree(f->data);
    }
    memset(new_data + f->capacity, 0, new_cap - f->capacity);
    f->data     = new_data;
    f->capacity = new_cap;
    return true;
}

/* ── Operations ───────────────────────────────────────────────────── */

static int ramfs_read(vfs_node_t *node, uint32_t off,
                      uint32_t size, uint8_t *buf) {
    if (!(node->flags & VFS_FILE)) return -1;
    ramfs_file_t *f = (ramfs_file_t *)node->fs_data;
    if (!f) return -1;

    if (off >= node->size) return 0;
    uint32_t avail = node->size - off;
    if (size > avail) size = avail;
    if (f->data)
        memcpy(buf, f->data + off, size);
    return (int)size;
}

static int ramfs_write(vfs_node_t *node, uint32_t off,
                       uint32_t size, const uint8_t *buf) {
    if (!(node->flags & VFS_FILE)) return -1;
    ramfs_file_t *f = (ramfs_file_t *)node->fs_data;
    if (!f) return -1;

    uint32_t end = off + size;
    if (!ensure_capacity(f, end)) return -1;

    memcpy(f->data + off, buf, size);
    if (end > node->size) node->size = end;
    return (int)size;
}

static vfs_node_t *ramfs_lookup(vfs_node_t *dir, const char *name) {
    if (!(dir->flags & VFS_DIRECTORY)) return NULL;
    for (vfs_node_t *c = dir->children; c; c = c->next) {
        if (strcmp(c->name, name) == 0) return c;
    }
    return NULL;
}

static int ramfs_readdir(vfs_node_t *dir, uint32_t index,
                         char *name_out, uint32_t *type_out) {
    if (!(dir->flags & VFS_DIRECTORY)) return -1;
    vfs_node_t *c = dir->children;
    for (uint32_t i = 0; c; c = c->next, i++) {
        if (i == index) {
            strcpy(name_out, c->name);
            *type_out = c->flags;
            return 0;
        }
    }
    return -1;
}

static vfs_node_t *ramfs_create(vfs_node_t *parent, const char *name,
                                uint32_t flags) {
    if (!(parent->flags & VFS_DIRECTORY)) return NULL;
    if (ramfs_lookup(parent, name)) return NULL;  /* already exists */

    vfs_node_t *node = alloc_node(name, flags);
    if (!node) return NULL;

    node->parent  = parent;
    node->next    = parent->children;
    parent->children = node;

    return node;
}

/* ── Init ─────────────────────────────────────────────────────────── */

void ramfs_init(void) {
    vfs_node_t *root = alloc_node("/", VFS_DIRECTORY);
    if (!root) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("  ERROR: ramfs root allocation failed\n");
        return;
    }
    vfs_set_root(root);

    /* Create a few default directories */
    ramfs_create(root, "tmp",  VFS_DIRECTORY);
    ramfs_create(root, "dev",  VFS_DIRECTORY);
    ramfs_create(root, "etc",  VFS_DIRECTORY);

    /* Create a welcome file */
    vfs_node_t *etc = ramfs_lookup(root, "etc");
    if (etc) {
        vfs_node_t *motd = ramfs_create(etc, "motd", VFS_FILE);
        if (motd) {
            const char *msg = "Welcome to MatryoshkaOS!\n";
            ramfs_write(motd, 0, strlen(msg), (const uint8_t *)msg);
        }
    }

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] ramfs mounted at /\n");
    vga_puts("  [OK] Default dirs: /tmp /dev /etc\n");
    vga_puts("  [OK] /etc/motd created\n\n");
}
