/**
 * MatryoshkaOS - Virtual File System (VFS)
 *
 * Provides a unified API over pluggable filesystem backends.
 * Each vfs_node carries a pointer to its fs-specific operations table.
 */

#ifndef MATRYOSHKA_VFS_H
#define MATRYOSHKA_VFS_H

#include <matryoshka/types.h>

#define VFS_NAME_MAX  64
#define VFS_MAX_FDS   32

/* Node types (stored in flags) */
#define VFS_FILE      0x01
#define VFS_DIRECTORY 0x02

/* Open flags */
#define VFS_O_RDONLY  0x00
#define VFS_O_WRONLY  0x01
#define VFS_O_RDWR    0x02
#define VFS_O_CREAT   0x40
#define VFS_O_TRUNC   0x200
#define VFS_O_APPEND  0x400

/* Forward declarations */
typedef struct vfs_node vfs_node_t;
typedef struct vfs_ops  vfs_ops_t;

/**
 * Per-filesystem operations.
 * A filesystem backend (e.g. ramfs) fills this table.
 */
struct vfs_ops {
    /** Read up to @a size bytes from @a offset. Returns bytes read or <0. */
    int         (*read)(vfs_node_t *node, uint32_t offset,
                        uint32_t size, uint8_t *buf);
    /** Write up to @a size bytes at @a offset. Returns bytes written or <0. */
    int         (*write)(vfs_node_t *node, uint32_t offset,
                         uint32_t size, const uint8_t *buf);
    /** Look up a child by name inside a directory. */
    vfs_node_t *(*lookup)(vfs_node_t *dir, const char *name);
    /**
     * Read the @a index-th directory entry.
     * Writes name into @a name_out, type into @a type_out.
     * Returns 0 on success, -1 if index out of range.
     */
    int         (*readdir)(vfs_node_t *dir, uint32_t index,
                           char *name_out, uint32_t *type_out);
    /** Create a child node (file or dir) inside @a parent. */
    vfs_node_t *(*create)(vfs_node_t *parent, const char *name,
                           uint32_t flags);
};

/**
 * VFS node (inode-like).
 * Each open file, directory, or mount point is represented by one node.
 */
struct vfs_node {
    char         name[VFS_NAME_MAX];
    uint32_t     flags;          /* VFS_FILE / VFS_DIRECTORY */
    uint32_t     size;           /* file size in bytes */
    uint32_t     inode;          /* unique id within this fs */
    vfs_ops_t   *ops;
    void        *fs_data;        /* backend-specific payload */
    vfs_node_t  *parent;
    vfs_node_t  *children;       /* first child (directories) */
    vfs_node_t  *next;           /* sibling linked list */
};

/* ── Lifecycle ────────────────────────────────────────────────────── */

void vfs_init(void);
void vfs_set_root(vfs_node_t *root);

/* ── File descriptor API ──────────────────────────────────────────── */

int  vfs_open(const char *path, uint32_t flags);
int  vfs_close(int fd);
int  vfs_read(int fd, void *buf, uint32_t size);
int  vfs_write(int fd, const void *buf, uint32_t size);

/* ── Directory / path helpers ─────────────────────────────────────── */

vfs_node_t *vfs_resolve(const char *path);
int         vfs_mkdir(const char *path);
vfs_node_t *vfs_create_file(const char *path);
int         vfs_readdir(const char *path, uint32_t index,
                        char *name_out, uint32_t *type_out);
vfs_node_t *vfs_get_root(void);

#endif /* MATRYOSHKA_VFS_H */
