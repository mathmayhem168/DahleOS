/* ================================================================
   fs/fs.c  –  In-memory virtual filesystem
   ================================================================
   All state lives in a flat pool of FS_MAX_NODES nodes.
   No dynamic allocation.  Node 0 is always the root "/".

   HOW IT WORKS
   ─────────────
   Every file and directory is a fs_node_t stored in pool[].
   Directories hold a list of child indices (children[]).
   The current working directory is just an int (cwd) that
   indexes into pool[].  Navigating is just updating cwd.

   EXTENDING THIS FILESYSTEM
   ──────────────────────────
   • More nodes / deeper names: raise FS_MAX_NODES / FS_MAX_NAME
     in fs.h.  Memory cost is ~400 bytes per extra node.
   • Write to files: add fs_write(name, buf, len) that finds the
     node, copies buf into node->data, sets node->data_len.
   • rmdir:  check child_count == 0 first, then remove from parent.
   • Persistence (ramdisk): serialise pool[] to a fixed RAM region
     at boot, and deserialise on startup.
   • Persistence (disk): implement an ATA PIO driver (port I/O on
     0x1F0-0x1F7), then write pool[] to a sector on the disk.
   ================================================================ */

#include "fs.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"

/* ---- internal pool ---- */
static fs_node_t pool[FS_MAX_NODES];
static int       pool_used = 0;
static int       cwd       = 0;   /* index of current working directory */

/* Allocate a fresh node from the pool. Returns its index, or -1 if full. */
static int node_alloc(void) {
    if (pool_used >= FS_MAX_NODES) return -1;
    int idx = pool_used++;
    memset(&pool[idx], 0, sizeof(fs_node_t));
    pool[idx].parent = -1;
    return idx;
}

/* Scan parent's children[] for a node named 'name'. Returns index or -1. */
static int node_find_child(int parent, const char *name) {
    fs_node_t *p = &pool[parent];
    for (int i = 0; i < p->child_count; i++) {
        if (strcmp(pool[p->children[i]].name, name) == 0)
            return p->children[i];
    }
    return -1;
}

/* Add child_idx into parent_idx's children[]. Returns 0, or -1 if full. */
static int node_add_child(int parent_idx, int child_idx) {
    fs_node_t *p = &pool[parent_idx];
    if (p->child_count >= FS_MAX_CHILDREN) return -1;
    p->children[p->child_count++] = child_idx;
    pool[child_idx].parent = parent_idx;
    return 0;
}

/* ================================================================
   Public API
   ================================================================ */

void fs_init(void) {
    memset(pool, 0, sizeof(pool));
    pool_used = 0;
    cwd       = 0;

    /* Node 0: root "/" */
    node_alloc();
    pool[0].name[0] = '/';
    pool[0].name[1] = '\0';
    pool[0].type    = FS_DIR;
    pool[0].parent  = -1;

    /* Starter directories so 'ls' isn't empty on boot */
    int home = node_alloc();
    strncpy(pool[home].name, "home", FS_MAX_NAME - 1);
    pool[home].type = FS_DIR;
    node_add_child(0, home);

    int tmp = node_alloc();
    strncpy(pool[tmp].name, "tmp", FS_MAX_NAME - 1);
    pool[tmp].type = FS_DIR;
    node_add_child(0, tmp);
}

/* Build an absolute path string for the current directory.
   Uses a static buffer – safe for single-threaded use. */
const char *fs_pwd_str(void) {
    static char path[512];

    if (cwd == 0) {
        path[0] = '/';
        path[1] = '\0';
        return path;
    }

    /* Walk from cwd up to root, recording node indices */
    int stack[32];
    int depth = 0;
    int cur   = cwd;
    while (cur != 0 && depth < 32) {
        stack[depth++] = cur;
        cur = pool[cur].parent;
    }

    /* Reverse the stack to get root-to-cwd order, build string */
    int pos = 0;
    for (int i = depth - 1; i >= 0; i--) {
        if (pos < 510) path[pos++] = '/';
        const char *n = pool[stack[i]].name;
        while (*n && pos < 510) path[pos++] = *n++;
    }
    path[pos] = '\0';
    return path;
}

/* ---- command implementations ---- */

void fs_cmd_pwd(void) {
    kprint(fs_pwd_str());
    kprint("\n");
}

void fs_cmd_ls(void) {
    fs_node_t *dir = &pool[cwd];
    if (dir->child_count == 0) {
        kprint("(empty)\n");
        return;
    }
    for (int i = 0; i < dir->child_count; i++) {
        int c = dir->children[i];
        if (pool[c].type == FS_DIR) {
            kprint_color(pool[c].name, LCYAN, BLACK);
            kprint_color("/", LCYAN, BLACK);
        } else {
            kprint(pool[c].name);
        }
        kprint("\n");
    }
}

void fs_cmd_mkdir(const char *name) {
    if (!name || !*name) { kprint("Usage: mkdir <name>\n"); return; }
    if (node_find_child(cwd, name) >= 0) {
        kprint_color("mkdir: ", LRED, BLACK);
        kprint(name);
        kprint(": already exists\n");
        return;
    }
    int idx = node_alloc();
    if (idx < 0) { kprint_color("mkdir: filesystem full\n", LRED, BLACK); return; }
    strncpy(pool[idx].name, name, FS_MAX_NAME - 1);
    pool[idx].type = FS_DIR;
    if (node_add_child(cwd, idx) < 0)
        kprint_color("mkdir: parent directory full\n", LRED, BLACK);
}

void fs_cmd_touch(const char *name) {
    if (!name || !*name) { kprint("Usage: touch <name>\n"); return; }
    if (node_find_child(cwd, name) >= 0) return;  /* already exists – silent */
    int idx = node_alloc();
    if (idx < 0) { kprint_color("touch: filesystem full\n", LRED, BLACK); return; }
    strncpy(pool[idx].name, name, FS_MAX_NAME - 1);
    pool[idx].type = FS_FILE;
    if (node_add_child(cwd, idx) < 0)
        kprint_color("touch: parent directory full\n", LRED, BLACK);
}

void fs_cmd_rm(const char *name) {
    if (!name || !*name) { kprint("Usage: rm <name>\n"); return; }
    int child = node_find_child(cwd, name);
    if (child < 0) {
        kprint_color("rm: ", LRED, BLACK); kprint(name); kprint(": no such file\n");
        return;
    }
    if (pool[child].type == FS_DIR) {
        kprint_color("rm: ", LRED, BLACK); kprint(name); kprint(": is a directory\n");
        return;
    }
    /* Compact the parent's children[] by shifting left */
    fs_node_t *dir = &pool[cwd];
    for (int i = 0; i < dir->child_count; i++) {
        if (dir->children[i] == child) {
            for (int j = i; j < dir->child_count - 1; j++)
                dir->children[j] = dir->children[j + 1];
            dir->child_count--;
            break;
        }
    }
    memset(pool[child].name, 0, FS_MAX_NAME);   /* mark node slot as dead */
}

void fs_cmd_cd(const char *path) {
    if (!path || !*path) { cwd = 0; return; }   /* bare 'cd' → root */

    int cur = cwd;
    if (path[0] == '/') {                        /* absolute path */
        cur = 0;
        path++;
        if (!*path) { cwd = 0; return; }
    }

    /* Walk each '/'-separated component */
    char component[FS_MAX_NAME];
    while (*path) {
        int i = 0;
        while (*path && *path != '/' && i < FS_MAX_NAME - 1)
            component[i++] = *path++;
        component[i] = '\0';
        if (*path == '/') path++;
        if (!i) continue;

        if (strcmp(component, ".") == 0)  continue;
        if (strcmp(component, "..") == 0) {
            if (pool[cur].parent >= 0) cur = pool[cur].parent;
            continue;
        }
        int child = node_find_child(cur, component);
        if (child < 0) {
            kprint_color("cd: ", LRED, BLACK); kprint(component);
            kprint(": no such directory\n");
            return;
        }
        if (pool[child].type != FS_DIR) {
            kprint_color("cd: ", LRED, BLACK); kprint(component);
            kprint(": not a directory\n");
            return;
        }
        cur = child;
    }
    cwd = cur;
}

void fs_cmd_cat(const char *name) {
    if (!name || !*name) { kprint("Usage: cat <name>\n"); return; }
    int child = node_find_child(cwd, name);
    if (child < 0) {
        kprint_color("cat: ", LRED, BLACK); kprint(name); kprint(": no such file\n");
        return;
    }
    if (pool[child].type == FS_DIR) {
        kprint_color("cat: ", LRED, BLACK); kprint(name); kprint(": is a directory\n");
        return;
    }
    if (pool[child].data_len == 0) { kprint("(empty file)\n"); return; }
    kprint(pool[child].data);
    kprint("\n");
}
