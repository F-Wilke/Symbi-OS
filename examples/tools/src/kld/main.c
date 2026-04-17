#include "incs.h"

#include "event.h"
#include "uthash.h"
#include "fs.h"
#include "sig.h"
#include "globals.h"
#include "misc.h"
#include "elf.h"

#include <libgen.h>

globals_t GBLS = {
  .fsname         = "libk",
  .pathprefix     = "libk",
  .koc            = 0,
  .komax          = 0,
  .kos            = NULL,
  .verbose        = 0,
  .startfs        = 0,
  .mkkallsymslibs = 1,
  .mkkolibs       = 1,
  .pid            = 0
};


static void addko(char *name)
{
  if (GBLS.koc == GBLS.komax) {
    GBLS.komax = (GBLS.komax) ? (GBLS.komax * 2) : 256;
    GBLS.kos   = realloc(GBLS.kos, GBLS.komax * sizeof(char *));
  }
  ko_t *ko = &GBLS.kos[GBLS.koc];
  ko->ko   = optarg;
  ko->dir  = -1;
  ko->fd   = -1;
  GBLS.koc++;
}

static int
addkodir(char *dir)
{
  struct stat sb;

  if (stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
    if (GBLS.kodirc == GBLS.kodirmax) {
      GBLS.kodirmax = (GBLS.kodirmax) ? (GBLS.kodirmax * 2) : 256;
      GBLS.kodirs   = realloc(GBLS.kodirs, GBLS.kodirmax * sizeof(char *));
    }
    GBLS.kodirs[GBLS.kodirc] = dir;
    GBLS.kodirc++;
  } else {
    fprintf(stderr, "%s does not exist\n", dir);
    return -1;
  }
  return GBLS.kodirc;
}

static void
usage(char *name, FILE *fp)
{
  fprintf(fp,
	  "%s [-h] [-v] [-N] [-K dir] [-k kofile] [elf]\n"
	  "   -h     : print this usage message\n"
	  "   -v     : verbose operation\n"
	  "   -N     : supress creating shared libraries from symbols in /proc/kallsysms\n"
	  "   -K dir : add a directory to search for ko files (can be repeated)\n"
	  "   -k ko  : create a shared libk library for the ko (can be repeated)\n"
	  "   elf    : optional elf file to process as follows:\n"
	  "              For each libk found in the elf:\n"
	  "                 find coresponding ko and load it\n"
	  "                 update the libk symbol table with runtime addresses\n"
	  "              For each ko found in the elf:\n"
	  "                 extract ko and load\n"
	  "                 update the libk symbol table with runtime addresses\n",
	  basename(name));
}

static int
openkallsyms(FILE **fp) {
  FILE      *fp_;
  fp_ = fopen("/proc/kallsyms", "r");
  if (fp_ == NULL) {
    warn(__FUNCTION__);
    return -1;
  }  
  *fp    = fp_;
  return 0;
}

int
initSE(SymbolEntry *this, uintptr_t addr, char *name, char type)
{
  int rc = 0;
  switch (type) {
  case 'A':
  case 'a':
    this->bind = BIND_GLOBAL;
    this->type = TYPE_ABS;
    break;
  case 'B':
  case 'b':
    this->bind = BIND_GLOBAL;
    this->type = TYPE_BSS;
    break;
  case 'D':
  case 'd':
    this->bind = BIND_GLOBAL;
    this->type = TYPE_DATA;
    break;
  case 'R':
  case 'r':
    this->bind = BIND_GLOBAL;
    this->type = TYPE_RODATA;
    break;
  case 'T':
  case 't':
    this->bind = BIND_GLOBAL;
    this->type = TYPE_FUNC;
    break;
  case 'V':
  case 'W':
  case 'v':
  case 'w':
    this->bind = BIND_WEAK;
    this->type = TYPE_FUNC;
    break;
  default:
    fprintf(stderr, "Unsupported type: %c\n", type);
    assert(0);
    rc = -1;
  }
  if (rc>=0) {
    this->name = strdup(name);
    this->addr = addr;
  }
  return rc;
}

static off_t
pidSize()
{
  char pidstr[24];
  int pidstrlen;
  off_t n = 0;
  pidstrlen = snprintf(pidstr, sizeof(pidstr), "%" PRIdMAX, (intmax_t)GBLS.pid);
  n = pidstrlen;
  return n;
}

static bool
fs_libkernel_stat(fs_t *this, fs_file_t *file, struct stat *stbuf)
{
  VLPRINT(2, "%s %ld: ", file->name, file->ino);
  stbuf->st_ino = file->ino;
  stbuf->st_mode = S_IFREG | 0444;
  stbuf->st_nlink = 1;
  stbuf->st_size = pidSize();
  VLPRINT(2, "%ld\n", stbuf->st_size);
  return true; 
}

static bool
fs_libkernel_read(fs_t *this, fs_file_t *file, fuse_req_t req, size_t size,
			    off_t off)
{
  off_t  n = pidSize();
  char *buf = malloc(n+1);
  snprintf(buf, n+1, "%" PRIdMAX, (intmax_t)GBLS.pid);

  int rc=fsFuseReplyBufLimited(req, buf, n, off, size);
  if (rc!=0) fprintf(stderr, "fuse_reply_buf failed: %d", rc);
    
  free(buf);
  return true;
}

fs_fileops_t fs_libkernel_ops = {
  .stat    = fs_libkernel_stat,
  .open    = NULL,
  .read    = fs_libkernel_read,
  .write   = NULL,
  .readdir = NULL 
};


static void
createfs(fs_t *fs, fs_ino_t rootino)
{
  fs_file_t *item;
  VLPRINT(2, "fs=%p rootino=%ld\n", fs, rootino);
  item = fsCreatefile(fs, rootino, "libkernel.so", NULL, &fs_libkernel_ops);
  assert(item);
}

static evnthdlrrc_t
sigEvent(void *obj, uint32_t evnts, int epollfd)
{
  sigproc_t *this = obj;
  assert(this == &GBLS.sigproc);
  int          fd = this->sfd;
  evnthdlrrc_t rc = EVNT_HDLR_SUCCESS;
  
  VLPRINT(3,"START: sigproc: fd:%d evnts:0x%08x\n", fd, evnts);
  if (evnts & EPOLLIN) {
    struct signalfd_siginfo fdsi;
    ssize_t                 s;
    s = read(fd, &fdsi, sizeof(fdsi));
    assert(s==sizeof(fdsi));
    switch (fdsi.ssi_signo) {
    case SIGALRM:
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
    case SIGKILL:
    case SIGUSR1:
    case SIGVTALRM:
    case SIGUSR2:
    case SIGPIPE:
    case SIGIO:
      // exit yar if any of this signals occur
      VPRINT("exiting on signal event %d (%s)\n",
	     fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
      rc = EVNT_HDLR_EXIT_LOOP;
      break;
    default:
      EPRINT(stderr, "unknown signal event: %d\n", fdsi.ssi_signo);
    }
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
  VLPRINT(3, "END: sigproc: fd:%d evnts:0x%08x\n", fd, evnts);
  return rc;
}

static void
sigprocRegisterEvents(sigproc_t *this, int epollfd)
{
  struct epoll_event ev;
  ASSERT(this && epollfd != -1);
  ASSERT(this->sfd != -1 && this->ed.obj == this && this->ed.hdlr == sigEvent);
  ev.data.ptr = &(this->ed);
  ev.events  = EPOLLIN | EPOLLET; // Edge
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->sfd, &ev) == -1 ) {
      perror("epoll_ctl: this->sffd");
      assert(0);
  }    
}

static void
sigprocInit(sigproc_t *this, bool iszeroed)
{
  if (!iszeroed) bzero(this, sizeof(*this));
  this->sfd = -1;
  this->ed  = (evntdesc_t){ .obj = this, .hdlr=sigEvent }; 

  sigemptyset(&(this->mask));
  // block all the signals so that we avoid standard signal handling
  // behavior --> we will use a signal fd to convert them into events
  sigAddTermSignals(&(this->mask));
  
  assert(sigprocmask(SIG_BLOCK, &(this->mask), NULL)!=-1);

  this->sfd = signalfd(-1, &(this->mask), SFD_CLOEXEC|SFD_NONBLOCK);
  assert(this->sfd != -1); 
}

static void
sigprocCleanup(sigproc_t *this)
{
  VPRINT("%p\n", this);
  if (this->sfd != -1) {
    close(this->sfd);
    this->sfd = -1;
    sigemptyset(&this->mask);
  }
}

void cleanup(void)
{
  fsCleanup(&(GBLS.fs));
  sigprocCleanup(&(GBLS.sigproc));
}

int
openko(ko_t *ko)
{
  int fd, i;
  char fullpath[PATH_MAX];
  
  for (i=0; i<GBLS.kodirc; i++) {
    snprintf(fullpath, PATH_MAX, "%s/%s", GBLS.kodirs[i], ko->ko);
    fd = open(fullpath, O_RDONLY);
    if (fd != -1) break;
  }
  if (fd != -1) {
    ko->fd  = fd;
    ko->dir = i;
    VPRINT("Found %s in %s (%d).\n", ko->ko, GBLS.kodirs[ko->dir], ko->dir);
  }
  return fd;
}

int GBLSInit(int argc, char **argv)
{
  int opt;

  if (getcwd(GBLS.cwd, sizeof(GBLS.cwd)) == NULL) {
    fprintf(stderr, "ERROR: failed to get cwd\n");
    return -1;
  }
  addkodir(GBLS.cwd);
  
  while ((opt = getopt(argc, argv, "K:k:Nhv")) != -1) {
    switch (opt) {
    case 'K':
      addkodir(optarg);
      break;
    case 'N':
      GBLS.mkkallsymslibs = 0;
      break;
    case 'h':
      usage(argv[0],stderr);
      return -1;
    case 'k':
      addko(optarg);
      break;
    case 'v':
      GBLS.verbose++;
      break;
    default:
      usage(argv[0],stderr);
      return -1;
    }
  }

  for (int i=0; i<GBLS.koc; i++) {
    ko_t *ko = &(GBLS.kos[i]);
    openko(ko);
    if (ko->fd<0) {
      fprintf(stderr, "ERROR: could not open %s\n", ko->ko);
      return -1;
    }
  }
  
  if (GBLS.startfs) {
    GBLS.pid = getpid();
    assert(fsInit(&GBLS.fs,
		  true,   // init mount point
		  NULL,   // mount point prefix
		  true)); // zero out all other fields
    sigprocInit(&(GBLS.sigproc), true);
  }
  return 0;
}


static bool checkfd(int fd)
{
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    fprintf(stderr, "fstat on %d failed errno=%d\n", fd, errno);
    if (errno == EBADFD) {
      fprintf(stderr, "EBADFD: %d\n", fd);
    }
    return false;
  }
  return true;
}

#define MAX_EVENTS 1024
// epoll code is based on example from the man page
static bool
theLoop()
{
  bool rc;
  int epollfd;
  // create the kernel event poll object
  {
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1) {
      perror("epoll_create1");
      return false;
    }
  }

  // switch over to using epoll events for signal handling from now on
  sigprocRegisterEvents(&GBLS.sigproc, epollfd);

  fsRegisterEvents(&GBLS.fs, epollfd);
  
  // loop: detect events and dispatch handlers
  for (;;) {
    struct epoll_event events[MAX_EVENTS];
    errno = 0;
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      if (verbose(1)) perror("epoll_wait");
      if (errno == EINTR) {
	// maybe we got a signal we are not handling or something
	// else made us wakeup ...  log it but just keep on going 
	VLPRINT(2, "%s: EINTR: Continuing\n", __func__);
	continue;
      }
      if (errno == EINVAL) {
	// I don't know why this is happening
	// once we added logging
	//  trigger -> run yar, ctl-z, bg, enter
	EPRINT(stderr, "FAIL:errno=%d epollfd=%d ME=%d checkfd=%d\n",
	       errno, epollfd, MAX_EVENTS, checkfd(epollfd));
	continue;
      }
      rc = false;
      EPRINT(stderr, "FAIL:errno=%d epollfd=%d ME=%d checkfd=%d\n",
	     errno, epollfd, MAX_EVENTS, checkfd(epollfd));
      goto done;
    }
    
    for (int n = 0; n < nfds; ++n) {
      evnthdlrrc_t erc;
      evntdesc_t *ed = events[n].data.ptr;
      uint32_t evnts = events[n].events;
      assert(ed);
      VLPRINT(3, "%d/%d: ed:%p (.hdlr=0x%p .obj=Ox%p) evnts:0x%08x\n",
	      n, nfds, ed, ed->hdlr, ed->obj, evnts);
      assert(ed->hdlr);
      // call handler registered for this event source 
      erc = ed->hdlr(ed->obj, evnts, epollfd);
      if (erc == EVNT_HDLR_EXIT_LOOP) {
	VLPRINT(1, "eventhandler returned exiting loop rc"
		" hdlr:%p obj:0x%p evnts:%08x\n", ed->hdlr, ed->obj, evnts);
	rc = true;
	goto done;
      } else if (erc == EVNT_HDLR_FAILED) {
	EPRINT(stderr, "event handler failed hdlr:%p obj:0x%p evnts:%08x\n",
	       ed->hdlr, ed->obj, evnts);
	rc = false;
	goto done;
      }
    }
  }
  
  // Exit logic
 done:
  return rc;
}

static void
writelib(const char *path, const SymbolEntry *entries, ssize_t n,
	 size_t nmstrlen)
{
  int mfd;
  size_t elf_size;
  void *elf_ptr = elf_generate_elf_mmap(entries, n, nmstrlen, &elf_size, &mfd);
  
  if (elf_ptr != MAP_FAILED) {
    VPRINT("%s: ELF mapped at %p (Size: %zu)\n", path, elf_ptr, elf_size);
    
    // Example: Write the mapped buffer to a file
    int dfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(dfd, elf_ptr, elf_size);
    close(dfd);
    
    // Cleanup mapping
    munmap(elf_ptr, elf_size);
    close(mfd);
  }
}

void
entcleanup(SymbolEntry *entries, ssize_t n)
{
  while (n) {
    free(entries[n].name);
    n--;
  }
  free(entries);
}

/** FROM: Gemini
 * Creates directories recursively for a given path.
 * Returns 0 on success, -1 on failure.
 */
static
int mkpath(const char *path, mode_t mode) {
    char *temp_path = strdup(path);
    if (!temp_path) return -1;

    for (char *p = temp_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0'; // Temporarily truncate at current directory level
            if (mkdir(temp_path, mode) == -1 && errno != EEXIST) {
                free(temp_path);
                return -1;
            }
            *p = '/'; // Restore delimiter
        }
    }
    mode_t old_mask = umask(0);
    // Create the final level
    if (mkdir(temp_path, mode) == -1 && errno != EEXIST) {
        free(temp_path);
	umask(old_mask);
        return -1;
    }
    umask(old_mask);
    free(temp_path);
    return 0;
}
/****/

static void
mkkallsymslibs(const char *pathprefix, FILE *fp)
{
  char *       line = NULL;
  size_t       len, nmlen, knmstrlen=0;
  ssize_t      read;
  uintptr_t    addr;
  int          n;
  char         type;
  int          sym_start, sym_end, mod_start, mod_end;
  char *       name = NULL;
  char *       module = NULL;
  ssize_t      ksyms_n = 1024;
  ssize_t      ksyms_i = 0;
  SymbolEntry *kentries;
  
  struct Module {
    UT_hash_handle hh;
    SymbolEntry   *entries;
    char          *name;
    ssize_t        syms_n;
    ssize_t        syms_i;
    size_t         nmstrlen;
  } * mhash = NULL;

  if (pathprefix && (mkpath(pathprefix, 0777)<0)) {
    fprintf(stderr, "ERROR: %s invalid path.\n", pathprefix);
    return;
  }
  
  kentries = malloc(ksyms_n * sizeof(SymbolEntry));
  
  while ((read = getline(&line, &len, fp)) != -1 ) {
    sym_start = sym_end = mod_start = mod_end = 0;
    name = module = NULL;
    n = sscanf(line, "%" SCNx64 " %c %n%*s%n %n%*s%n", &addr, &type,
	       &sym_start, &sym_end, &mod_start, &mod_end);
    if (n>=2) {
      name = &line[sym_start];
      line[sym_end]='\0';
      nmlen = sym_end - sym_start + 1; // +1 for null
      if (mod_start != 0 && mod_end > mod_start) {
	module = &line[mod_start];
	module[0] = '['; module++;
	if (line[mod_end-1] == '\n' || line[mod_end-1] == '\r' ||
	    line[mod_end-1] == ']') line[mod_end-1] = '\0';
	line[mod_end] = '\0'; // Null-terminate str2 in-place
	// add symbol to appropriate module symbol entries
	// printf("%d: addr:0x%" PRIx64 " type:%c symbol:%s module:%s\n",
	// n, addr, type, name, module);
	struct Module *mod;
	HASH_FIND_STR(mhash, module, mod);
	if (mod==NULL) {
	  mod           = malloc(sizeof(struct Module));
	  mod->name     = strdup(module); mod->syms_n = 1024; mod->syms_i = 0;
	  mod->nmstrlen = 0;
	  mod->entries  = malloc(mod->syms_n * sizeof(SymbolEntry));
	  HASH_ADD_KEYPTR(hh, mhash, mod->name, strlen(mod->name), mod);
	}
	if (mod->syms_i == mod->syms_n) {
	  mod->syms_n <<= 1;
	  mod->entries = realloc((void *)mod->entries,
				 mod->syms_n*sizeof(SymbolEntry));
	}
	if (initSE(&(mod->entries[mod->syms_i]),addr, name, type)>=0) {
	  mod->nmstrlen += nmlen;
	  mod->syms_i++;
	}
      } else {
	// add to libkern.so symbol entries
	// printf("%d: addr:0x%" PRIx64 " type:%c symbol:%s\n", n, addr,
	// type, name);
	if (ksyms_i == ksyms_n) {
	  ksyms_n <<= 1;
	  kentries = realloc((void *)kentries, ksyms_n*sizeof(SymbolEntry));
	}
	if (initSE(&kentries[ksyms_i],addr, name, type)>=0) {
	  knmstrlen += nmlen;
	  ksyms_i++;
	}
      }
    }
  }
  
  if (ksyms_i) {
    char path[PATH_MAX];
    if (pathprefix) {
      snprintf(path, sizeof(path), "%s/libkern.so", pathprefix);
    } else {
      snprintf(path, sizeof(path), "libkern.so");
    }
    writelib(path, kentries, ksyms_i, knmstrlen);
    entcleanup(kentries, ksyms_i);
  }
  
  {
    const struct Module *mod, *tmp;
    HASH_ITER(hh, mhash, mod, tmp) {
      char path[PATH_MAX];
      if (pathprefix) {
	snprintf(path, sizeof(path), "%s/lib%s.so", pathprefix, mod->name);
      } else {
	snprintf(path, sizeof(path), "lib%s.so", mod->name);
      }
      writelib(path, mod->entries, mod->syms_i, mod->nmstrlen);
      entcleanup(mod->entries, mod->syms_i);
      free(mod->name);
      HASH_DEL(mhash, mod);
      free((struct Module *)mod);
    }
  }

  free(line);
  fclose(fp);
  EEXIT();
}

static void
mkkolib(ko_t *ko)
{
  char  sopath[PATH_MAX];
  char  kopath[PATH_MAX];
  char *kobn = basename(ko->ko);
  char *tmpstr = strdup(kobn);
  int   i;
  
  for (i=0; tmpstr[i]!='\0'; i++);
  if (i>3 || tmpstr[i-3]!='.' || tmpstr[i-2]!='k' || tmpstr[i-1]!='o') {
    tmpstr[i-3]='\0';
  }
  snprintf(sopath,PATH_MAX,"%s/lib%s.so", GBLS.kodirs[ko->dir], tmpstr);
  snprintf(kopath,PATH_MAX,"%s/%s", GBLS.kodirs[ko->dir], kobn);
  free(tmpstr);
  
  VPRINT("%s -> %s\n",  kopath, sopath);
}

int
main(int argc, char **argv)
{
  FILE *ksfp;
  
  if (GBLSInit(argc, argv)<0) {
    //    usage(argv[0], stderr);
    EEXIT();
  }

  if (GBLS.mkkolibs) {
    for (int i=0; i<GBLS.koc; i++) {
      mkkolib(&GBLS.kos[i]);
    }
  }
  
  if (GBLS.mkkallsymslibs) {
    if (openkallsyms(&ksfp)<0) {
      fprintf(stderr, "ERROR: failed to open kallsyms\n");
      EEXIT();
    }
    mkkallsymslibs(GBLS.pathprefix, ksfp);
  }
  
  if (GBLS.startfs) {
    if (!fsCreate(&(GBLS.fs), argv[0], createfs)) EEXIT();
    atexit(cleanup);    // from this point on exits will trigger cleanups   
    if (!theLoop()) EEXIT();
  }
  
  return EXIT_SUCCESS;
}
