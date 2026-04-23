#include "incs.h"

globals_t GBLS = {
  .bos            = NULL,
  .executable     = NULL,
  .botblfile      = KOTBL_DFLT,
  .dirs           = NULL,
  .fsname         = FSNAME_DFLT,
  .pathprefix     = PATHPREFIX_DFLT,
  .pid            = 0,
  .dirc           = 0,
  .dirmax         = 0,
  .verbose        = 0,
  .startfs        = 0,
  .prockallsyms   = 1,
  .procbos        = 1
};

static void
usage(char *name, FILE *fp)
{
  fprintf(fp,
	  "%s [-h] [-v] [-N] [-K dir] [-k kofile[,modnm[,kldopts]]] "
	  "[-o kotblfile] [elf]\n"
	  "   -h     : print this usage message\n"
	  "   -v     : verbose operation\n"
	  "   -N     : supress creating shared libraries from symbols in"
	  " /proc/kallsysms\n"
	  "   -K dir : add a directory to search for ko files"
	  " (can be repeated)\n"
	  "   -k ko  : create a shared libk library for the ko "
	  "(can be repeated)\n"
	  "   -o kotblfile: file to output binary kotbl (default: %s)\n"
	  "   elf    : optional elf file to process as follows:\n"
	  "              For each libk found in the elf:\n"
	  "                 find coresponding ko and load it\n"
	  "                 update the libk symbol table with runtime"
	  " addresses\n"
	  "              For each ko found in the elf:\n"
	  "                 extract ko and load\n"
	  "                 update the libk symbol table with runtime"
	  " addresses\n",
	  basename(name), KOTBL_DFLT);
}

static int
openko(char *kofnm, char **kofullpath)
{
  int fd, i;
  char fullpath[PATH_MAX];
  
  *kofullpath = NULL;
  for (i=0; i<GBLS.dirc; i++) {
    snprintf(fullpath, sizeof(fullpath), "%s/%s", GBLS.dirs[i], kofnm);
    fd = open(fullpath, O_RDONLY|O_CLOEXEC);
    if (fd != -1) break;
  }
  if (fd != -1) {
    *kofullpath = realpath(fullpath,NULL);
    VPRINT("Found %s in %s (%d:%s) fd:%d.\n", kofnm, GBLS.dirs[i], i,
	   *kofullpath, fd);
  } else {
    fprintf(stderr, "ERROR:%s:%s:%d:%s\n", __func__, kofnm,
	    errno, strerror(errno));
  }
  return fd;
}

static int
openso(char *kofnm, char *modnm, char **sofullpath)
{
  int fd;
  char *tmp=NULL;
  char fullpath[PATH_MAX];

  
  if (kofnm && modnm) {
    tmp = strdup(kofnm);
    char *dir=dirname(tmp);
    snprintf(fullpath, sizeof(fullpath), "%s/lib%s.so", dir, modnm);
    fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC, SOPERM_DFLT);
    if (fd != -1) {
      *sofullpath = strdup(fullpath);
      VPRINT("opened so:%s for ko:%s fd:%d\n", *sofullpath, modnm, fd);
    } else {
      fprintf(stderr, "ERROR:%s:%s:%d:%s\n", __func__, kofnm,
	      errno, strerror(errno));
    }
  } else {
    fd = -1;
  }

  if (tmp) free(tmp);
  return fd;
}

static char
*modnmfromfnm(char *fnm)
{
  char *tmp   = strdup(fnm);
  char *base  = basename(tmp);
  char *modnm = NULL;
  int      i = 0;

  for (i=0; base[i]!='\0'; i++);
  // remove '.ko' extension 
  if (i>3 && base[i-3]=='.' && base[i-2]=='k' && base[i-1]=='o') {
    base[i-3]='\0';
  }
  modnm = strdup(base);  // make a copy in new memory to pass back
  free(tmp);
  return modnm;
}

static int
parsekospec(const char *kospec, char **fnm, char **kldopts, char **modnm)
{
  int   commas = 0;
  char *tmp    = strdup(kospec);
  char *f, *n, *o;
  int   rc=0;
  
  f = n = o = NULL;
  
  f = tmp;
  for (int i=0; tmp[i]!=0; i++) {
    if (tmp[i] == ',') {
      // found a comma
      tmp[i] = '\0';
      commas++;
      if (commas==1) {
	// first comma
	n = &(tmp[i+1]); // record start
      } else if (commas==2) {
	o = &tmp[i+1];  // record start 
      } else {
	fprintf(stderr, "ERROR: bad ko specification: %s\n", kospec);
	rc = -1;
	goto done;
      }
    }
  }
  *fnm = f;
  switch (commas) {
  case 0:
    *modnm   = modnmfromfnm(f);
    *kldopts = strdup(KLDOPTS_DFLT);
    break;
  case 1:
    *modnm   = (*n == '\0') ? modnmfromfnm(f) : strdup(n);
    *kldopts = strdup(KLDOPTS_DFLT);
    break;
  case 2:
    *modnm   = (*n == '\0') ? modnmfromfnm(f) : strdup(n);
    *kldopts = (*o == '\0') ? strdup(KLDOPTS_DFLT) : strdup(o);
  }
  
  VLPRINT(2, "%s: fnm:%s modnm:%s kldopts:%s\n", kospec, *fnm, *modnm, *kldopts);
done:
  return rc;
}

static bo_t *
newbo(char *kofullpath, char *modnm, char *kldopts, int kofd,
      char *sofullpath, int sofd)
{
  bo_t *bo = malloc(sizeof(bo_t));  
  bo->kofnm   = kofullpath;
  bo->modnm   = modnm;
  bo->kldopts = kldopts;
  bo->kofd    = kofd;
  bo->sofnm   = sofullpath;
  bo->sofd    = sofd;
  return bo;
}

__attribute__((unused)) static void
deletebo(bo_t *bo)
{
  if (bo) {
    if (bo->kofnm)     free(bo->kofnm);
    if (bo->sofnm)     free(bo->sofnm);
    if (bo->modnm)     free(bo->modnm);
    if (bo->kldopts)   free(bo->kldopts);
    if (bo->kofd >= 0) close(bo->kofd);
    if (bo->sofd >= 0) close(bo->sofd);
  }
}

static void
dumpbo(bo_t *bo)
{
  EPRINT(stderr, "bo:%p mod:%s kldopts:%s ko:%s (%d) so:%s (%d)\n",
	 bo, bo->modnm, bo->kldopts, bo->kofnm, bo->kofd, bo->sofnm, bo->sofd);
}

static int
addbo(const char *kospec)
{
  bo_t *bo=NULL;
  char *kofnm, *kldopts, *modnm;
  char *kofullpath, *sofullpath; 
  int   kofd=-1, sofd=-1, rc=0;

  kofnm = kldopts = modnm = kofullpath = sofullpath = NULL;
  
  if (parsekospec(kospec, &kofnm, &kldopts, &modnm) < 0) { rc = -1; goto done; }
  
  kofd = openko(kofnm, &kofullpath);
  if (kofd < 0) { rc = -1; goto done; } 

  sofd = openso(kofullpath, modnm, &sofullpath);
  if (sofd < 0) { rc = -1; goto done; }
  
  HASH_FIND_STR(GBLS.bos, kofullpath, bo);
  
  if (bo) {
    fprintf(stderr, "WARNING: %s already specified ignoring %s\n",
	    kofullpath, kospec);
    rc = -1;
    goto done;
  }

  bo = newbo(kofullpath, modnm, kldopts, kofd, sofullpath, sofd);
  HASH_ADD_KEYPTR(hh, GBLS.bos, bo->kofnm, strlen(bo->kofnm), bo);

  if (GBLS.verbose>1) {
    VPRINT("Added ob: %d total obs:\n", HASH_COUNT(GBLS.bos));
    dumpbo(bo);
  }
done:
  if (kofnm)              free(kofnm);
  if (rc<0 && kofd != -1) close(kofd);
  if (rc<0 && sofd != -1) close(sofd);
  if (rc<0 && kldopts)    free(kldopts);
  if (rc<0 && modnm)      free(modnm);
  if (rc<0 && kofullpath) free(kofullpath);
  if (rc<0 && sofullpath) free(sofullpath);
  return rc;
}

static int
addDir(char *dir)
{
  struct stat sb;

  if (stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
    if (GBLS.dirc == GBLS.dirmax) {
      GBLS.dirmax = (GBLS.dirmax) ? (GBLS.dirmax << 1) : 256;
      GBLS.dirs   = realloc(GBLS.dirs, GBLS.dirmax * sizeof(char *));
    }
    GBLS.dirs[GBLS.dirc] = dir;
    GBLS.dirc++;
  } else {
    fprintf(stderr, "%s does not exist\n", dir);
    return -1;
  }
  return GBLS.dirc;
}

static int
GBLSInit(int argc, char **argv)
{
  int opt;
  char **kospec    = NULL;
  int    kospecc   = 0;
  int    kospecmax = 0;
  int    rc        = 0;;
  
  // init objects to ensure cleanup is safe at any point
  assert(fsInit(&GBLS.fs,
		false,   // init mount point
		NULL,    // mount point prefix
		true));  // zero out and reset all fields

  // This is a kludge but I don't want to rewrite sigprocInit
  sigprocReset(&(GBLS.sigproc),
	       true);    // zero out and reset all fields
  
  if (getcwd(GBLS.cwd, sizeof(GBLS.cwd)) == NULL) {
    fprintf(stderr, "ERROR: failed to get cwd\n");
    rc=-1;
    goto done;
  }
  addDir(GBLS.cwd);
  
  while ((opt = getopt(argc, argv, "K:k:Nhv")) != -1) {
    switch (opt) {
    case 'K':
      addDir(optarg);
      break;
    case 'N':
      GBLS.prockallsyms = 0;
      break;
    case 'h':
      usage(argv[0],stderr);
      return -1;
    case 'k':
      if (kospecc == kospecmax) {
	kospecmax = (kospecmax) ? kospecmax << 1 : 2;
	kospec    = realloc(kospec, sizeof(char *)*kospecmax);
	memset(&kospec[kospecc], 0, kospecmax - kospecc);
      }
      kospec[kospecc] = optarg;
      kospecc++;
      break;
    case 'v':
      GBLS.verbose++;
      break;
    default:
      usage(argv[0],stderr);
      return -1;
    }
  }

  if (GBLS.verbose) {
    VPRINT("%d Directories to searched:\n", GBLS.dirc);
    for (int i=0; i<GBLS.dirc; i++) {
      VPRINT("GBLS.dirs[%d]=%s\n", i, GBLS.dirs[i]);
    }
    VPRINT("%d Kernel Object specifications:\n", kospecc);
    for (int i=0; i<kospecc; i++) {
      VPRINT("kospec[%d]=%s\n", i, kospec[i]);
    }
  }

  for (int i=0; i<kospecc; i++) {
    if (addbo(kospec[i])<0) {
      rc=-1;
      goto done;
    }
  }

  if (GBLS.startfs) {
    GBLS.pid = getpid();
    assert(fsInit(&GBLS.fs,
		  true,   // init mount point
		  NULL,   // mount point prefix
		  false)); // all ready done
    sigprocInit(&(GBLS.sigproc), true);
  }
  
done:
  if (kospec) free(kospec);
  return rc;
}

static void
cleanupEntries(SymbolEntry *entries, ssize_t n)
{
  for (int i=0; i<n; i++) free(entries[i].name);
  free(entries);
}

static void
writeso(const char *path, int fd, const SymbolEntry *entries, ssize_t n,
	 size_t nmstrlen)
{
  int mfd;
  int dfd=fd;
  size_t elf_size;
  void *elf_ptr = elf_generate_elf_mmap(entries, n, nmstrlen, &elf_size, &mfd);
  
  if (elf_ptr != MAP_FAILED) {
    VPRINT("%s: ELF mapped at %p (Size: %zu)\n", path, elf_ptr, elf_size);
    
    // Example: Write the mapped buffer to a file
    if (fd==-1) {
      dfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } 
    write(dfd, elf_ptr, elf_size);
    if (fd == -1) close(dfd);
    
    // Cleanup mapping
    munmap(elf_ptr, elf_size);
    close(mfd);
  }
}

static int
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
  } else {
    this->name = NULL;
    this->addr = 0;
  }
  this->size   = 0; 
  return rc;
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

static int
openKallsyms(FILE **fp)
{
  FILE      *fp_;
  fp_ = fopen("/proc/kallsyms", "r");
  if (fp_ == NULL) {
    warn(__FUNCTION__);
    return -1;
  }  
  *fp    = fp_;
  return 0;
}

static void
prcKallsyms(const char *pathprefix, FILE *fp)
{
  char *       line = NULL;
  size_t       len, nmlen, knmstrlen=0;
  uintptr_t    addr;
  int          n;
  char         type;
  int          sym_start, sym_end, mod_start, mod_end;
  char *       name     = NULL;
  char *       module   = NULL;
  size_t       ksyms_n  = 0;
  size_t       ksyms_i  = 0;
  SymbolEntry *kentries = NULL;;
  
  struct Module {
    UT_hash_handle hh;
    SymbolEntry   *entries;
    char          *name;
    size_t         syms_n;
    size_t         syms_i;
    size_t         nmstrlen;
  } * mhash = NULL;

  if (pathprefix && (mkpath(pathprefix, 0777)<0)) {
    fprintf(stderr, "ERROR: %s invalid path.\n", pathprefix);
    return;
  }
  
  while (getline(&line, &len, fp) != -1 ) {
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
	struct Module *mod = NULL;
	HASH_FIND_STR(mhash, module, mod);
	if (mod==NULL) {
	  mod           = malloc(sizeof(struct Module));
	  mod->name     = strdup(module);
	  mod->syms_n   = 0;
	  mod->syms_i   = 0;
	  mod->nmstrlen = 0;
	  mod->entries  = NULL;
	  // memset(mod->entries, 0, sizeof(mod->syms_n * sizeof(SymbolEntry)));
	  HASH_ADD_KEYPTR(hh, mhash, mod->name, strlen(mod->name), mod);
	}
	if (mod->syms_i == mod->syms_n) {
	  mod->syms_n = (mod->syms_n) ? mod->syms_n << 1 : 1024;
	  mod->entries = realloc((void *)mod->entries,
				 mod->syms_n*sizeof(SymbolEntry));
	  // for good measure zero out new memory
	  memset(&(mod->entries[mod->syms_i]), 0,
		 (mod->syms_n - mod->syms_i) * sizeof(SymbolEntry));
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
	  ksyms_n = (ksyms_n) ? ksyms_n << 1 : 1024;
	  kentries = realloc((void *)kentries, ksyms_n*sizeof(SymbolEntry));
	  // for good measure zero out new memory
	  memset(&(kentries[ksyms_i]), 0,
		 (ksyms_n - ksyms_i) * sizeof(SymbolEntry));
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
    writeso(path, -1, kentries, ksyms_i, knmstrlen);
    cleanupEntries(kentries, ksyms_i);
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
      writeso(path, -1, mod->entries, mod->syms_i, mod->nmstrlen);
      cleanupEntries(mod->entries, mod->syms_i);
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
sigprocCleanup(sigproc_t *this)
{
  VPRINT("%p\n", this);
  if (this->sfd != -1) {
    close(this->sfd);
    this->sfd = -1;
    sigemptyset(&this->mask);
  }
}

static void
bosCleanup()
{
  
}

static void
cleanup(void)
{
  if (GBLS.startfs)  {
    fsCleanup(&(GBLS.fs));
    sigprocCleanup(&(GBLS.sigproc));
  }
  
  if (GBLS.bos) {
    bosCleanup();
    GBLS.bos = NULL;
  }
  if (GBLS.dirs) {
    free(GBLS.dirs);
    GBLS.dirs = NULL;
  }
  
}

static int
prcBO(bo_t *bo)
{
  int i=0, rc=0;
  SymbolEntry *entries = NULL;
  size_t       n, nmstrlen;
  
  if (GBLS.verbose) {
    VPRINT("Processing ko (%d):\n",i);
    dumpbo(bo);
    i++;
  }
  if (elf_read_syms(bo->kofnm, bo->kofd, &entries, &n, &nmstrlen)<0) {
    rc = -1;
    goto done;
  }
  writeso(bo->sofnm, bo->sofd, entries, n, nmstrlen);
  VPRINT("%s: read %lu symbols\n", bo->kofnm, n);

 done:
  if (entries) cleanupEntries(entries,n);
  return rc;
}

static int
prcExec(char *exec)
{
  return 0;
}

int
main(int argc, char **argv)
{
  FILE *ksfp;
  
  if (GBLSInit(argc, argv)<0) {
    EEXIT();
  }

  atexit(cleanup);    // from this point on exits will trigger cleanups
  
  if (GBLS.executable) {
    prcExec(GBLS.executable);
  }

  // process each of the bo's found either from command line or
  // from the executable
  if (GBLS.procbos) {
    bo_t *bo, *tmp;
    HASH_ITER(hh, GBLS.bos, bo, tmp) {
      prcBO(bo);
    }
  }

  // we do this last so that ko loads will be reflected in so updates
  if (GBLS.prockallsyms) {
    if (openKallsyms(&ksfp)<0) {
      fprintf(stderr, "ERROR: failed to open kallsyms\n");
      EEXIT();
    }
    prcKallsyms(GBLS.pathprefix, ksfp);
  }

  // optionally expose objects via synthetic filesystem
  // This support has not been completed.
  if (GBLS.startfs) {
    if (!fsCreate(&(GBLS.fs), argv[0], kldfsCreate)) EEXIT();
    if (!kldfsLoop(&GBLS.fs, &GBLS.sigproc)) EEXIT();
  }
  
  return EXIT_SUCCESS;
}
