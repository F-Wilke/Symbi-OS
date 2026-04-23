#include "incs.h"

/************ MISC SUPPORT CODE DIRECTLY TAKEN FROM FUSE EXAMPLE CODE **********
 * the next two functions are directly taken from the fuse example code to
 * send a data back to the fuse kernel module.... these may require work
 * if one needs to send more than a page of data back
 ******************************************************************************/
struct dirbuf {
  char *p;
  size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
		       fuse_ino_t ino)
{
  struct stat stbuf;
  size_t oldsize = b->size;
  b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
  b->p = (char *) realloc(b->p, b->size);
  memset(&stbuf, 0, sizeof(stbuf));
  stbuf.st_ino = ino;
  fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
		    b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

extern int fsFuseReplyBufLimited(fuse_req_t req, const char *buf,
				 size_t bufsize, off_t off, size_t maxsize)
{
  if (off < bufsize)
    return fuse_reply_buf(req, buf + off,
			  min(bufsize - off, maxsize));
  else
    return fuse_reply_buf(req, NULL, 0);
}
/******************************************************************************/


/**************** FILE SYSTEM SEMANTICS AND SUPPORT CODE **********************
 * These are the core routines that allow you to construct a simple synthetic
 * file system on top of fuse for your program.  The core object is a fs_file_t.
 * see fs.h for details.  To implement a file or directory simply write
 * your own implementation and register it with a path at creation time or
 * dynamically if you need to.
 *****************************************************************************/
static void
file_init(fs_file_t *this, fs_ino_t ino, fs_ino_t dir, void *data,
	     const char *name, fs_filetype_t type, bool malloced,
	     const fs_fileops_t *ops)
{
  if (malloced) bzero(this, sizeof(fs_file_t));
  this->ops      = ops;
  this->data     = data;
  this->name     = strdup(name);
  this->ino      = ino;
  this->dir      = dir;
  this->type     = type;
  this->malloced = malloced;
}
		    
static fs_ino_t
inodesc2ino(const fs_t *this, const fs_inodesc_t *id)
{
  assert(id >= &(this->ino_table[0]) &&
	 id <= &(this->ino_table[this->ino_num-1]));
  return (ino_t) (id - &this->ino_table[0]);
}

static fs_inodesc_t *
ino2inodesc(const fs_t *this, const fs_ino_t ino)
{
  assert(ino >= 0 && ino < this->ino_num);
  return &(this->ino_table[ino]);
}

static void
init_inotable(fs_t *this)
{
  fs_inodesc_t *ino_table=NULL;
  int ino_num=0;
  assert(this->ino_table == NULL && this->ino_num == 0);
  ino_table = malloc(4096);
  ino_num   = 4096/sizeof(fs_inodesc_t);
  for (int i=0; i<(ino_num-1); i++) {
    ino_table[i].file = NULL;
    ino_table[i].next = &ino_table[i+1];
  }
  ino_table[ino_num-1].file = NULL;
  ino_table[ino_num-1].next = NULL;
  this->ino_table           = ino_table;
  this->ino_num             = ino_num;
  this->ino_freelist        = this->ino_table;
  this->ino_numfree         = ino_num;
}

static void
grow_inotable(const fs_t *this)
{
  NYI;
}

static fs_ino_t
inoalloc(fs_t *this)
{
  if (this->ino_table == NULL) init_inotable(this);
  if (!this->ino_freelist) {
    assert(this->ino_numfree == 0);
    grow_inotable(this);
  }
  fs_inodesc_t *id = this->ino_freelist;
  this->ino_freelist = id->next;
  id->next = NULL;
  this->ino_numfree--;
  assert(this->ino_numfree >= 0);
  assert(id->file==NULL);
  return inodesc2ino(this, id);
}

static bool
inofree(fs_t *this, const fs_ino_t ino)
{
  fs_inodesc_t *id = ino2inodesc(this, ino);
  assert(id->next == NULL);
  id->next = this->ino_freelist;
  this->ino_freelist = id;
  this->ino_numfree++;
  return true;
}

static bool
dir_add(const fs_file_t *dir, fs_file_t *item)
{
  VLPRINT(2, "dir:%s (%ld)  item:%s (%ld)\n", dir->name, dir->ino,
	  item->name, item->ino);
  assert(dir->type == DIRECTORY && dir->data);
  fs_dir_t *dd = dir->data;
  if (item->name) {
      fs_file_t *tmp;
      HASH_FIND(hh_ino, dd->ino_files, &item->ino, sizeof(item->ino), tmp);
      if (tmp!=NULL) {
	VLPRINT(2, "%ld (%s) ino not unique in %s\n", item->ino, item->name,
		dir->name);
	return false;
      }
      HASH_FIND(hh_name, dd->nm_files, item->name, strlen(item->name), tmp);
      if (tmp!=NULL) {
	VLPRINT(2, "%s (%ld) ino not unique in %s\n", item->name, item->ino,
	        dir->name);		
	return false;
      }
      HASH_ADD(hh_ino, dd->ino_files, ino, sizeof(item->ino), item);
      HASH_ADD_KEYPTR(hh_name, dd->nm_files, item->name, strlen(item->name),
		      item);
      VLPRINT(2, "ino:%ld name:%s added to %s (%ld)\n", item->ino, item->name,
	      dir->name, dir->ino);
      return true;
  }
  VLPRINT(2, "%s (%ld) : Failed to add name null\n", dir->name, dir->ino);
  return false;
}

fs_file_t *
fsCreatedir(fs_t *this, const fs_ino_t parentino, const char *name,
	    const fs_fileops_t *ops)
{
  fs_file_t *dir, *parent = NULL;
  fs_ino_t ino            = inoalloc(this);
  fs_inodesc_t *pid, *id  = ino2inodesc(this, ino);
  
  assert(id->file == NULL && id->next == NULL);

  if (parentino != INVALID_INO) {
    pid = ino2inodesc(this, parentino);
    assert(pid && pid->next == NULL && pid->file->type == DIRECTORY);
    parent = pid->file;
  }
  dir = malloc(sizeof(fs_file_t));
  fs_dir_t *dirdata = malloc(sizeof(fs_dir_t));
  bzero(dirdata, sizeof(fs_dir_t));
  file_init(dir, ino, parentino, dirdata, name, DIRECTORY, true, ops);  
  if (parent) {
    dir_add(parent, dir);
  }
  id->file = dir;
  return dir;
}
  

fs_file_t *
fsCreatefile(fs_t *this, const fs_ino_t dirino, const char *name,
	     const char *symlink, const fs_fileops_t *ops)
{
  fs_file_t   *file, *dir;
  fs_ino_t           ino = inoalloc(this);
  fs_inodesc_t *did, *id = ino2inodesc(this, ino);
  char         *filedata = NULL;
  fs_filetype_t     type = REGULAR;
  
  assert(id->file == NULL && id->next == NULL);

  assert(dirino != INVALID_INO);
  did = ino2inodesc(this, dirino);
  
  assert(did && did->next == NULL && did->file->type == DIRECTORY);
  dir = did->file;

  if (symlink) {
    filedata = strdup(symlink);
    type     = SYMLINK;
  }
  
  file = malloc(sizeof(fs_file_t));
  file_init(file, ino, dirino, filedata, name, type, true, ops);  

  dir_add(dir, file);
  id->file = file;
  
  return file;
}

// works for all types
extern bool
fsRemoveitem(fs_t *this, fs_file_t *item, const bool purgeemptydir)
{
  fs_file_t *containingdir  = NULL;
  fs_inodesc_t *containingid;
  fs_dir_t     *dir;
  int icnt, ncnt;
  bool containingdirempty = false;
  VLPRINT(2, "this=%p item=%p item->name=%s purge=%d\n", this, item, item->name,
	  purgeemptydir);
  if (item->type == DIRECTORY) {
    dir      = item->data;
    icnt = HASH_CNT(hh_ino, dir->ino_files);
    ncnt = HASH_CNT(hh_name, dir->nm_files);
    assert(icnt == ncnt);
    if (icnt != 0) return false;   // if directory not empty then fail 
    if (item->ino != ROOT_INO) {
      // unless root directory find containing directory
      containingid = ino2inodesc(this, item->dir);
      assert(containingid->next == NULL && containingid->file != NULL);
      assert(containingid->file->type == DIRECTORY);
      containingdir = containingid->file;
    } else {
      assert(item->dir == INVALID_INO);
    }
  } else {
    assert(item->ino != ROOT_INO && item->dir != INVALID_INO);
    containingid = ino2inodesc(this, item->dir);
    assert(containingid->next == NULL && containingid->file != NULL);
    assert(containingid->file->type == DIRECTORY);
    containingdir = containingid->file;
  }
  // take care of containing directory -- if there is one
  if (containingdir) {
    dir = containingdir->data;
    icnt = HASH_CNT(hh_ino, dir->ino_files);
    ncnt = HASH_CNT(hh_name, dir->nm_files);
    assert(icnt == ncnt);
    assert(icnt > 0);
    fs_file_t *tmp;
    HASH_FIND(hh_ino, dir->ino_files, &item->ino, sizeof(item->ino), tmp);
    assert(tmp == item);
    HASH_DELETE(hh_ino, dir->ino_files, tmp);
    HASH_FIND(hh_name, dir->nm_files, item->name, strlen(item->name), tmp);
    assert(tmp == item);
    HASH_DELETE(hh_name, dir->nm_files, tmp);
    icnt = HASH_CNT(hh_ino, dir->ino_files);
    ncnt = HASH_CNT(hh_name, dir->nm_files);
    assert(icnt == ncnt);
    if (icnt == 0) containingdirempty=true;
  }
  inofree(this,item->ino);      // free item's inode (put back on free list)

  // for easier debugging we clear all pointer fields that are not malloced
  // to help catch uses after free 
  item->ops     = NULL;
  item->ino     = INVALID_INO;
  item->dir     = INVALID_INO;
  item->type    = NONE;
  if (item->data) { free(item->data); item->data = NULL; }
  if (item->name) { free((char *)item->name); item->name = NULL; }
  if (item->malloced) free(item);

  // if we have a containing directory that is now empty
  // and we are supposed to purge them then do so
  if (containingdir && containingdirempty && purgeemptydir) {
    fsRemoveitem(this, containingdir, purgeemptydir);
  }
  
  return true;
}
  
static bool
fs_dir_stat(fs_t *this, fs_file_t *dir, struct stat *stbuf)
{
  VLPRINT(2, "%s (%ld)\n", dir->name, dir->ino);
  stbuf->st_mode = S_IFDIR | 0755;
  stbuf->st_nlink = 2;      // FIXME: and link count support
  return true;
}

static bool
fs_dir_readdir(fs_t *this, fs_file_t *dir, fuse_req_t req, size_t size,
	       off_t off)
{
  struct dirbuf b;
  
  bzero(&b, sizeof(b));
  dirbuf_add(req, &b, ".", dir->ino);
  dirbuf_add(req, &b, "..", dir->dir);
  //  dirbuf_add(req, &b, fs_files[CMDS_INO].name, CMDS_INO);
  {
    fs_dir_t *dd = dir->data;
    fs_file_t *item, *tmp;
    HASH_ITER(hh_ino, dd->ino_files, item, tmp) {
      dirbuf_add(req, &b, item->name, item->ino);
      VLPRINT(2, "%s %ld added to directory list\n", item->name, item->ino);
    }
  }
  fsFuseReplyBufLimited(req, b.p, b.size, off, size);
  free(b.p);
  
  return true;
}

fs_fileops_t fs_dir_ops = {
  .stat    = fs_dir_stat,
  .open    = NULL,
  .read    = NULL,
  .write   = NULL,
  .readdir = fs_dir_readdir
};

/************ FUSE INTERFACE CALL BACK ROUTINES ******************************
 * These routines are invoked by the fuse lib and provide the bridge between
 * your specific file and directory semantics.  Using the fs objects
 * inode table they convert an inode to a fs_file_t object and when appropriate
 * pass the call on to the file objects operation function (see fs_fileops_t)
 * This is a very minimal set and may need to be expanded if you need to 
 * provide more complete file system semantics -- this code was derived
 * from the minimal fuse example code for implementing a file system
 * with a single file.
 *****************************************************************************/
static void fs_fuse_init(void *userdata, struct fuse_conn_info *conn)
{
  fs_t *this = userdata; 
  VLPRINT(2, "this=%p userdata=%p conn=%p\n", this, userdata, conn);
  (void)this;
  // copied from fuse example code
  conn->want = FUSE_CAP_ASYNC_READ;
  conn->want &= ~FUSE_CAP_ASYNC_READ;
}

void fs_fuse_lookup(fuse_req_t req, fuse_ino_t dirino, const char *name)
{
  struct fuse_entry_param e;
  fs_t *this = fuse_req_userdata(req);
  fs_inodesc_t *did;
  fs_file_t *file;
  fs_dir_t  *dir;
  VLPRINT(2, "this=%p dirino=%lu name=%s\n", this, dirino, name); 

  did = ino2inodesc(this, dirino);
  
  if (did->next != NULL || did->file->type != DIRECTORY) {
      fuse_reply_err(req, ENOENT);
  } else {
    dir = did->file->data;
    HASH_FIND(hh_name, dir->nm_files, name, strlen(name), file);
    if (file == NULL) {
      fuse_reply_err(req, ENOENT);
    } else {
      bzero(&e, sizeof(e));
      e.ino = file->ino;
      e.attr_timeout = 1.0;
      e.entry_timeout = 1.0;
      file->ops->stat(this, file, &e.attr);
      fuse_reply_entry(req, &e);
    }
  }
}

void fs_fuse_getattr(fuse_req_t req, fuse_ino_t ino,
		     struct fuse_file_info *fi)
{
  struct stat stbuf;
  fs_t *this = fuse_req_userdata(req);
  fs_file_t *file;
  fs_inodesc_t *id;
  (void) fi;
  
  VLPRINT(2, "this:%p ino:%ld fi=%p\n", this, ino, fi);

  id = ino2inodesc(this, ino);
  if (id->next != NULL) {
    fuse_reply_err(req, ENOENT);
  } else {
    file = id->file;
    assert(file);
    memset(&stbuf, 0, sizeof(stbuf));
    file->ops->stat(this, file, &stbuf);
    fuse_reply_attr(req, &stbuf, 1.0);
  }
}

void fs_fuse_readlink(fuse_req_t req, fuse_ino_t ino) { NYI; }

void fs_fuse_open(fuse_req_t req, fuse_ino_t ino,
		  struct fuse_file_info *fi)
{
  fs_t *this = fuse_req_userdata(req);
  fs_file_t *file;
  fs_inodesc_t *id;
  (void) fi;
  
  VLPRINT(2, "this:%p ino:%ld fi=%p\n", this, ino, fi);

  id = ino2inodesc(this, ino);
  assert(id->file != NULL && id->next == NULL);
  file = id->file;
  if (file->ops->open) {
    file->ops->open(this, file, req);
  } else {
    if (file->type == DIRECTORY) {
      fuse_reply_err(req, EISDIR);
    } else if ((fi->flags & O_ACCMODE) != O_RDONLY) {
      fuse_reply_err(req, EACCES);
    } else
      fuse_reply_open(req, fi);
  }
}

void fs_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
		  struct fuse_file_info *fi)
{
  fs_t *this = fuse_req_userdata(req);
  fs_file_t *file;
  fs_inodesc_t *id;
  (void) fi;

  VLPRINT(2, "this:%p ino:%ld size=%lu off=%lu fi=%p\n", this, ino, size,
	  off, fi);

  id = ino2inodesc(this, ino);
  assert(id->file != NULL && id->next == NULL);
  file = id->file;

  assert(file->ops->read);
  if (!file->ops->read(this, file, req, size, off)) {
    fuse_reply_err(req, EACCES);
  }
}

void fs_fuse_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
		   size_t size, off_t off, struct fuse_file_info *fi)
{
  fs_file_t *file = (fs_file_t *)fi->fh;
  (void)file;
  NYI;
}


void fs_fuse_readdir(fuse_req_t req, fuse_ino_t dirino, size_t size, off_t off,
		     struct fuse_file_info *fi)
{
  fs_t          *this = fuse_req_userdata(req);
  fs_inodesc_t  *did;
  (void) fi;
  
  VLPRINT(2, "this:%p dirino:%ld size=%lu off=%lu fi=%p\n", this, dirino, size,
	  off, fi);

  did = ino2inodesc(this, dirino);
  if (did->next != NULL || did->file->type != DIRECTORY) {
      fuse_reply_err(req, ENOENT);
  } else {
    fs_file_t *dir = did->file;
    assert(dir->ops->readdir);
    if (!dir->ops->readdir(this, dir, req, size, off)) {
      fuse_reply_err(req, ENOENT);
    }
  }
}

void fs_fuse_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
		      const char *value, size_t size, int flags)
{
  fs_t *this = fuse_req_userdata(req);
  fs_inodesc_t *id;
  
  VLPRINT(2, "this:%p ino:%ld name=%s value=%s size=%ld flags=%d\n",
	  this, ino, name, value, size, flags);

  id = ino2inodesc(this, ino);
  assert(id->file != NULL && id->next == NULL);
  const char* exp_val = "fs_ll_setxattr_value";
  if (strcmp(name, "fs_ll_setxattr_name") == 0 &&
      strlen(exp_val) == size &&
      strncmp(value, exp_val, size) == 0)
    {
      fuse_reply_err(req, 0);
    }
  else
    {
      fuse_reply_err(req, ENOTSUP);
    }
}

void
fs_fuse_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
		 size_t size)
{
  fs_t *this = fuse_req_userdata(req);
  fs_inodesc_t *id;
  
  VLPRINT(2, "this:%p ino:%ld name=%s size=%ld\n", this, ino, name, size);

  id = ino2inodesc(this, ino);
  assert(id->file != NULL && id->next == NULL);
  if (strcmp(name, "fs_ll_getxattr_name") == 0) {
    const char *buf = "fs_ll_getxattr_value";
    fuse_reply_buf(req, buf, strlen(buf));
  } else {
    fuse_reply_err(req, ENOTSUP);
  }
}

static void fs_fuse_removexattr(fuse_req_t req, fuse_ino_t ino,
			      const char *name)
{
  fs_t *this = fuse_req_userdata(req);
  fs_inodesc_t *id;
  
  VLPRINT(2, "this:%p ino:%ld name=%s\n",  this, ino, name);
  
  id = ino2inodesc(this, ino);
  assert(id->file != NULL && id->next == NULL);

  if (strcmp(name, "fs_ll_removexattr_name") == 0)
    {
      fuse_reply_err(req, 0);
    }
  else
    {
      fuse_reply_err(req, ENOTSUP);
    }
}

static const struct fuse_lowlevel_ops fs_fuse_ops = {
  .init            = fs_fuse_init,
  .destroy         = NULL,
  .lookup          = fs_fuse_lookup,
  .forget          = NULL,
  .getattr         = fs_fuse_getattr,
  .setattr         = NULL,
  .readlink        = fs_fuse_readlink,
  .mknod           = NULL,
  .unlink          = NULL,
  .rmdir           = NULL,
  .symlink         = NULL,
  .rename          = NULL,
  .link            = NULL,
  .open            = fs_fuse_open,
  .read            = fs_fuse_read,
  .write           = fs_fuse_write,
  .flush           = NULL,
  .release         = NULL,
  .fsync           = NULL,
  .opendir         = NULL,
  .readdir         = fs_fuse_readdir,
  .releasedir      = NULL,
  .fsyncdir        = NULL,
  .setxattr        = fs_fuse_setxattr,
  .getxattr        = fs_fuse_getxattr,
  .listxattr       = NULL,
  .removexattr     = fs_fuse_removexattr,
  .access          = NULL,
  .create          = NULL,
  .getlk           = NULL,
  .setlk           = NULL,
  .bmap            = NULL,
  .ioctl           = NULL,
  .poll            = NULL,
  .write_buf       = NULL,
  .retrieve_reply  = NULL,
  .forget_multi    = NULL,
  .flock           = NULL,
  .fallocate       = NULL,
  .readdirplus     = NULL,
  .copy_file_range = NULL,
  .lseek           = NULL
};
/*****************************************************************************/


static evnthdlrrc_t
fsEvent(void *obj, uint32_t evnts, int epollfd)
{
  fs_t *this = obj;
  int     fd = this->fuse_fd;

  VLPRINT(3,"START: fs: fd:%d evnts:0x%08x\n", fd, evnts);
  if (evnts & EPOLLIN) {
    int n = fuse_session_receive_buf(this->fuse_se, &(this->fuse_buf));
    if (n>0) fuse_session_process_buf(this->fuse_se, &(this->fuse_buf));
    evnts = evnts & ~EPOLLIN;
    if (evnts==0) goto done;
  }
  if (evnts & EPOLLHUP) {
    VLPRINT(2,"EPOLLHUP(%x)\n", EPOLLHUP);
    evnts = evnts & ~EPOLLHUP;
    if (evnts==0) goto done;
  }
  if (evnts & EPOLLRDHUP) {
    VLPRINT(2,"EPOLLRDHUP(%x)\n", EPOLLRDHUP);
    evnts = evnts & ~EPOLLRDHUP;
    if (evnts==0) goto done;
  }
  if (evnts & EPOLLERR) {
    VLPRINT(2,"EPOLLERR(%x)\n", EPOLLERR);
    evnts = evnts & ~EPOLLRDHUP;
    if (evnts==0) goto done;
  }
  if (evnts != 0) {
    VLPRINT(2,"unknown events evnts:%x", evnts);
  }
 done:
  VLPRINT(3, "END: fs: fd:%d evnts:0x%08x\n", fd, evnts);
  return EVNT_HDLR_SUCCESS;
}

extern bool
fsRegisterEvents(fs_t *this, int epollfd)
{
  int fd;
  struct epoll_event ev;

  ASSERT(this && this->fuse_se && this->fuse_fd != -1);
  fd = this->fuse_fd;
  ev.events   = EPOLLIN |  EPOLLHUP | EPOLLRDHUP | EPOLLERR; // Level 
  ev.data.ptr = &this->ed;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1 ) {
    perror("epoll_ctl: fs->fuse_fd");
    return false;
  }
  return true;
}

extern bool
fsCreate(fs_t *this, char *name, fs_createfsfunc_t createfsfunc)
{
  ASSERT(this);
  // mkdir
  if (mkdir(this->mntpt, 0700)<0) {
    perror("fsCreate: mkdir");
    return false;
  }
  this->mkdir = true;

  // allocate the 0th ino to reserve it as invalid 
  fs_ino_t ino=inoalloc(this);
  assert(ino == 0 && ino == INVALID_INO);

  // allocate the 1st ino for the root directory (".") of the file system
  fs_file_t *root = fsCreatedir(this, INVALID_INO, ".", &fs_dir_ops);
  assert(root->ino == ROOT_INO);

  if (createfsfunc)  createfsfunc(this, root->ino);
  
  // must pass at least one argument as the name
  fuse_opt_add_arg(&this->fuse_args, name);

  // let fuse / kernel deal with mount permissions
  fuse_opt_add_arg(&this->fuse_args, "-o");  
  fuse_opt_add_arg(&this->fuse_args, "default_permissions");

  // turn on fuse debug message if we are verbose level 2
  if (verbose(2)) {
    fuse_opt_add_arg(&this->fuse_args, "--debug");
  } 
                                                             
  this->fuse_se = fuse_session_new(&this->fuse_args, &fs_fuse_ops,
				   sizeof(fs_fuse_ops),
				   this                           // userdata
				   );
  assert(this->fuse_se);

  if (fuse_session_mount(this->fuse_se, this->mntpt) != 0) {
    fprintf(stderr, "Error: fuse_session_mount() failed\n");
    return false;
  }
  
  this->fuse_fd = fuse_session_fd(this->fuse_se);
  assert(this->fuse_fd != -1);
  fcntl(this->fuse_fd, F_SETFD, FD_CLOEXEC);

  assert(this->ed.obj == this && this->ed.hdlr == fsEvent);
  return true;
}

static bool
fsMountPoint(char *mntpt, int n, char *dir)
{
  int rc;
  if (dir) {
    rc = snprintf(mntpt, n, "%s/%s", dir, GBLS.fsname);
  } else {
    rc = snprintf(mntpt, n, "%s", GBLS.fsname);    
  }
  if (rc < 0 || rc >= n) {
    perror("snprintf"); 
    return false;
  }
  return true;
}

static bool
fsSetMntPt(fs_t *this, char *mntpt)
{
  assert(this);
  assert(this->mntpt==NULL);
  if (mntpt==NULL) { this->mntpt = NULL; return true; }
  // WE ASSUME mntpt is a properly null terminated string
  this->mntpt = strdup(mntpt);
  assert(this->mntpt);
  return true;
}

extern bool
fsInit(fs_t *this, bool initmntpt, char *mntptdir, bool iszeroed)
{
  char *tmp=malloc(PATH_MAX);
  if (!iszeroed) {
    bzero(this, sizeof(fs_t));
  }
  *this = (fs_t){ .fuse_args = FUSE_ARGS_INIT(0, NULL),
		  .fuse_fd = -1 };
  if (initmntpt) {
    if (!fsMountPoint(tmp, sizeof(tmp), mntptdir)) { free(tmp);  EEXIT(); }
    if (!fsSetMntPt(this, tmp)) { free(tmp); EEXIT(); }
  } 
  this->ed = (evntdesc_t){ .obj = this, .hdlr = fsEvent };
  free(tmp);
  return true;
}

extern bool
fsCleanup(fs_t *this)
{
  VPRINT("%p\n", this);
  if (this->mntpt && this->mkdir) {  
    if (this->fuse_se) {
      fuse_session_unmount(this->fuse_se);
      fuse_session_destroy(this->fuse_se);
      this->fuse_se = NULL;
      fuse_opt_free_args(&this->fuse_args);
      this->fuse_args = (struct fuse_args)FUSE_ARGS_INIT(0, NULL);
    }
    if (this->fuse_buf.mem) free(this->fuse_buf.mem);
    rmdir(this->mntpt);
    if (this->mntpt) free(this->mntpt);
    this->mntpt = NULL;
    if (this->ino_table) {
      int removed=0;
      int allocated = this->ino_num - this->ino_numfree;
      assert(this->ino_num);
      for (int i=0; i<this->ino_num; i++) {
	// scan and clean
	fs_inodesc_t *id= &(this->ino_table[i]);
	if (id->file) {
	  assert(id->next == NULL);
	  fs_file_t *file = id->file;
	  fsRemoveitem(this, file, true);
	  removed++;
	}
      }
      inofree(this, INVALID_INO);
      removed++;
      assert(removed==allocated);
      assert(this->ino_num == this->ino_numfree);
      free(this->ino_table);
      this->ino_table    = NULL;
      this->ino_num      = 0;
      this->ino_freelist = NULL;
      this->ino_numfree  = 0;
    }
  }
  return true;
}

