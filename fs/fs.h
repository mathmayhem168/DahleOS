#ifndef FS_H
#define FS_H

/* ================================================================
   fs/fs.h  –  In-memory virtual filesystem
   ================================================================
   Tune these limits to trade memory for capacity:
     FS_MAX_NODES    – total files + directories that can ever exist
     FS_MAX_CHILDREN – how many entries one directory can hold
     FS_MAX_NAME     – longest filename (including null terminator)
     FS_MAX_DATA     – max bytes a single file can store
   ================================================================ */

#include "../include/types.h"

#define FS_MAX_NAME      64
#define FS_MAX_CHILDREN  16
#define FS_MAX_NODES     64
#define FS_MAX_DATA     256

typedef enum { FS_DIR = 0, FS_FILE = 1 } fs_type_t;

typedef struct {
    char      name[FS_MAX_NAME];
    fs_type_t type;
    int       parent;                      /* pool index; -1 = no parent (root) */
    int       children[FS_MAX_CHILDREN];   /* pool indices of direct children   */
    int       child_count;
    char      data[FS_MAX_DATA];           /* file content (unused for dirs)    */
    int       data_len;
} fs_node_t;

/* Call once before the shell starts. Creates root "/" and starter dirs. */
void fs_init(void);

/* Zero the pool without creating any starter dirs — call before loading from disk. */
void fs_init_empty(void);

/* Restore pool from a serialized snapshot (called by persist_load_fs). */
void fs_load_state(int saved_pool_used, int saved_cwd,
                   const void *pool_data, uint32_t pool_data_len);

/* Persistence accessors — used only by storage/persist.c */
int         fs_get_pool_used(void);
int         fs_get_cwd(void);
const void *fs_get_pool_ptr(void);
uint32_t    fs_get_pool_size(void);

/* Returns a static string of the absolute path to the current directory.
   Example: "/home/projects"
   The buffer is overwritten on every call – copy it if you need to keep it. */
const char *fs_pwd_str(void);

/* ---- shell command implementations ---- */
void fs_cmd_pwd(void);
void fs_cmd_ls(void);
void fs_cmd_mkdir(const char *name);
void fs_cmd_touch(const char *name);
void fs_cmd_rm(const char *name);
void fs_cmd_cd(const char *path);
void fs_cmd_cat(const char *name);

#endif
