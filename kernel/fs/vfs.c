/**
 * MatryoshkaOS - Virtual File System core
 *
 * Path resolution, file descriptor table, and dispatch to the
 * per-node operations table provided by the concrete filesystem.
 */

#include <matryoshka/vfs.h>
#include <matryoshka/string.h>
#include <matryoshka/vga.h>

/* ── File descriptor table ────────────────────────────────────────── */

typedef struct {
    vfs_node_t *node;
    uint32_t    offset;
    uint32_t    flags;
    bool        used;
} fd_entry_t;

static fd_entry_t fd_table[VFS_MAX_FDS];
static vfs_node_t *root_node;

/* ── Helpers ──────────────────────────────────────────────────────── */

static int alloc_fd(void) {
    for (int i = 0; i < VFS_MAX_FDS; i++) {
        if (!fd_table[i].used) return i;
    }
    return -1;
}

/**
 * Split the next path component out of *cursor.
 * Writes the component into @a comp (null-terminated).
 * Advances *cursor past the component and any trailing '/'.
 * Returns false when no more components remain.
 */
static bool next_component(const char **cursor, char *comp, size_t max) {
    const char *p = *cursor;
    while (*p == '/') p++;
    if (*p == '\0') return false;

    size_t i = 0;
    while (*p && *p != '/' && i < max - 1)
        comp[i++] = *p++;
    comp[i] = '\0';

    *cursor = p;
    return true;
}

/* ── Lifecycle ────────────────────────────────────────────────────── */

void vfs_init(void) {
    memset(fd_table, 0, sizeof(fd_table));
    root_node = NULL;

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] VFS initialized\n");
}

void vfs_set_root(vfs_node_t *root) {
    root_node = root;
}

vfs_node_t *vfs_get_root(void) {
    return root_node;
}

/* ── Path resolution ──────────────────────────────────────────────── */

vfs_node_t *vfs_resolve(const char *path) {
    if (!root_node || !path) return NULL;
    if (path[0] == '/' && path[1] == '\0') return root_node;

    vfs_node_t *cur = root_node;
    const char *p = path;
    char comp[VFS_NAME_MAX];

    while (next_component(&p, comp, VFS_NAME_MAX)) {
        if (!(cur->flags & VFS_DIRECTORY)) return NULL;
        if (!cur->ops || !cur->ops->lookup) return NULL;
        cur = cur->ops->lookup(cur, comp);
        if (!cur) return NULL;
    }
    return cur;
}

/**
 * Resolve parent directory and isolate the final name component.
 * Returns the parent node and writes the leaf name into @a leaf.
 */
static vfs_node_t *resolve_parent(const char *path, char *leaf, size_t max) {
    if (!path || path[0] != '/') return NULL;

    /* Find last '/' */
    const char *last_slash = path;
    for (const char *p = path; *p; p++)
        if (*p == '/') last_slash = p;

    /* Copy leaf name */
    const char *lname = last_slash + 1;
    size_t i = 0;
    while (lname[i] && i < max - 1) { leaf[i] = lname[i]; i++; }
    leaf[i] = '\0';
    if (i == 0) return NULL;

    /* Resolve parent path */
    if (last_slash == path) return root_node;  /* parent is "/" */

    char parent_path[256];
    size_t plen = (size_t)(last_slash - path);
    if (plen >= sizeof(parent_path)) return NULL;
    memcpy(parent_path, path, plen);
    parent_path[plen] = '\0';
    return vfs_resolve(parent_path);
}

/* ── File descriptor API ──────────────────────────────────────────── */

int vfs_open(const char *path, uint32_t flags) {
    vfs_node_t *node = vfs_resolve(path);

    if (!node && (flags & VFS_O_CREAT)) {
        node = vfs_create_file(path);
    }
    if (!node) return -1;

    int fd = alloc_fd();
    if (fd < 0) return -1;

    fd_table[fd].node   = node;
    fd_table[fd].offset = (flags & VFS_O_APPEND) ? node->size : 0;
    fd_table[fd].flags  = flags;
    fd_table[fd].used   = true;

    if ((flags & VFS_O_TRUNC) && node->ops && node->ops->write) {
        node->size = 0;
    }

    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !fd_table[fd].used) return -1;
    fd_table[fd].used = false;
    fd_table[fd].node = NULL;
    return 0;
}

int vfs_read(int fd, void *buf, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !fd_table[fd].used) return -1;
    vfs_node_t *node = fd_table[fd].node;
    if (!node || !node->ops || !node->ops->read) return -1;

    int n = node->ops->read(node, fd_table[fd].offset, size, (uint8_t *)buf);
    if (n > 0) fd_table[fd].offset += (uint32_t)n;
    return n;
}

int vfs_write(int fd, const void *buf, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !fd_table[fd].used) return -1;
    vfs_node_t *node = fd_table[fd].node;
    if (!node || !node->ops || !node->ops->write) return -1;

    int n = node->ops->write(node, fd_table[fd].offset,
                             size, (const uint8_t *)buf);
    if (n > 0) fd_table[fd].offset += (uint32_t)n;
    return n;
}

/* ── Directory helpers ────────────────────────────────────────────── */

int vfs_mkdir(const char *path) {
    char leaf[VFS_NAME_MAX];
    vfs_node_t *parent = resolve_parent(path, leaf, VFS_NAME_MAX);
    if (!parent || !(parent->flags & VFS_DIRECTORY)) return -1;
    if (!parent->ops || !parent->ops->create) return -1;

    vfs_node_t *dir = parent->ops->create(parent, leaf, VFS_DIRECTORY);
    return dir ? 0 : -1;
}

vfs_node_t *vfs_create_file(const char *path) {
    char leaf[VFS_NAME_MAX];
    vfs_node_t *parent = resolve_parent(path, leaf, VFS_NAME_MAX);
    if (!parent || !(parent->flags & VFS_DIRECTORY)) return NULL;
    if (!parent->ops || !parent->ops->create) return NULL;

    return parent->ops->create(parent, leaf, VFS_FILE);
}

int vfs_readdir(const char *path, uint32_t index,
                char *name_out, uint32_t *type_out) {
    vfs_node_t *dir = vfs_resolve(path);
    if (!dir || !(dir->flags & VFS_DIRECTORY)) return -1;
    if (!dir->ops || !dir->ops->readdir) return -1;
    return dir->ops->readdir(dir, index, name_out, type_out);
}
