#ifndef __FS_H__
#define __FS_H__

struct fs;
struct fs_filedesc; 
typedef fuse_ino_t fs_ino_t;
#define INVALID_INO 0
#define ROOT_INO 1

typedef  void (*fs_createfsfunc_t)(struct fs *fs, fs_ino_t rootino);
typedef  bool (*fs_statfunc_t)(struct fs *, struct fs_filedesc *, struct stat *);
typedef  bool (*fs_openfunc_t)(struct fs *, struct fs_filedesc *, fuse_req_t);
typedef  bool (*fs_readfunc_t)(struct fs *, struct fs_filedesc *, fuse_req_t,
			       size_t, off_t);
typedef  bool (*fs_writefunc_t)(struct fs *, struct fs_filedesc *,
				struct stat *);
typedef  bool (*fs_readdirfunc_t)(struct fs *, struct fs_filedesc *, fuse_req_t,
				  size_t, off_t);
typedef struct {
  fs_statfunc_t    stat;
  fs_openfunc_t    open;
  fs_readfunc_t    read;
  fs_writefunc_t   write;
  fs_readdirfunc_t readdir;
} fs_fileops_t;

typedef enum { NONE=0, REGULAR=1, SYMLINK=2, DIRECTORY=3 } fs_filetype_t;

// fixme add hard link count tracking (every subdirectory should add one to link
// count of parent). On creation a directory will have a link count of 2 +1 for
// '.' entry and +1 for the parent having an entry for the new).  New files
// will have a link count of 1. 
struct fs_filedesc {
  UT_hash_handle      hh_ino;   // ino hashtable handle
  UT_hash_handle      hh_name;  // name hashtable handle
  const fs_fileops_t *ops;
  void               *data;
  const char         *name;
  fs_ino_t            ino;
  fs_ino_t            dir;      // inode of dir this item is listed in           
  fs_filetype_t       type;
  bool                malloced; // malloced requires freeing
};
typedef struct fs_filedesc fs_file_t;

typedef struct {
  fs_file_t  *ino_files; // hash table of files indexed by ino
  fs_file_t  *nm_files;  // hash table of files indexed by name
} fs_dir_t;

typedef struct iodesc {
  fs_file_t     *file;
  struct iodesc *next;
} fs_inodesc_t;

// File System Object
//   Provides a file system oriented interface to the yar process
typedef struct fs {
  struct fuse_args     fuse_args;
  struct fuse_buf      fuse_buf;
  evntdesc_t           ed;        // event descriptor for theLoop   
  struct fuse_session *fuse_se;
  char                *mntpt;     // mount point
  fs_inodesc_t        *ino_table;
  fs_inodesc_t        *ino_freelist;
  int                  ino_num;
  int                  ino_numfree;
  int                  fuse_fd;
  bool                 mkdir;     // true after mkdir of mount point happens  
} fs_t;

extern fs_file_t *fsCreatedir(fs_t *this, const fs_ino_t parent,
			      const char *name, const fs_fileops_t *ops);
extern fs_file_t *fsCreatefile(fs_t *this, const fs_ino_t dirino,
				const char *name, const char *symlink,
				const fs_fileops_t *ops);
extern bool fsRemoveitem(fs_t *this, fs_file_t *item, const bool purgeemptydir);
extern bool fsRegisterEvents(fs_t *this, int epollfd);
extern bool fsCreate(fs_t *this, char *name, fs_createfsfunc_t cf);
extern bool fsInit(fs_t *this, bool initmtpt, char *mntptdir, bool iszeroed);
extern bool fsCleanup(fs_t *this);
extern int  fsFuseReplyBufLimited(fuse_req_t req, const char *buf,
				  size_t bufsize, off_t off, size_t maxsize);
#endif
