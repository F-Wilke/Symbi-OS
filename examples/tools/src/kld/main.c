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
	  "%s [-h] [-v] [-N] [-K dir] [-k kofile] [-o kotblfile] [elf]\n"
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



#if 0
static int
parsekospec(const char kospec, char **path, char **kldopts, char **name)
{
  char *tmp = strdup(kospec)
  *kopath    = tmp;
  for (int i=0; tmp[i]!=0; i++) {
    if (tmp[i] = ',') {
      // found a comma
      tmp[i] = '\0';
      commas++;
      if (commas==1) {
	// first comma
	kokldopts = tmp[i+1]; // record start
      } else if (commas==2) {
	koname = &kospec[i+1];  // record start 
      } else {
	fprintf(stderr, "ERROR: bad ko specification: %s\n", orig);
	rc = -1;
	goto done;
      }
    }
  }

  switch (commas) {
  case 0:
    koname    = konamefrompath(kopath);
    kokldopts = strdup(DEFAULT_KLDOPTS);
    break;
  case 1:
    koname    = strdup(koname);
    kokldopts = strdup(DEFAULT_KLDOPTS);
    break;
  case 2:
    koname    = strdup(koname);
    kokldopts = strdup(kokdopts);
  }
  

}

static int
addbo(const char *kospec)
{
  struct bo_t *bo;
  int          commas    = 0;
  char        *path    = NULL;
  char        *kldopts = NULL;
  char        *name    = NULL;

  if (parsekospec(kospec, &path, &kldopts, &name) < 0) {
    return -1;
  }
  
  HASH_FIND_STR(GLBS.bos, name, bo);

  if (bo) {
    fprintf("WARNING: %s already specified ignoring %s\n", name, kospec)
      return -1;
  }
  
  bo = malloc(sizeof(ko_t));

  strlen(name);
  bo->kofnm  = path;
  ko->modnm  = name;
  ko->kldopts = kldopts;
  
  ko->dir     = -1;
  ko->fd      = -1;
  
  return 0;
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
#endif 


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


int GBLSInit(int argc, char **argv)
{
  int opt;

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
    return -1;
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
      //addko(optarg);
      break;
    case 'v':
      GBLS.verbose++;
      break;
    default:
      usage(argv[0],stderr);
      return -1;
    }
  }

#if 0  
  HASH_ITER()
    openko(ko);
    if (ko->fd<0) {
      fprintf(stderr, "ERROR: could not open %s\n", ko->ko);
      return -1;
    }
  }
#endif

  if (GBLS.startfs) {
    GBLS.pid = getpid();
    assert(fsInit(&GBLS.fs,
		  true,   // init mount point
		  NULL,   // mount point prefix
		  false)); // all ready done
    sigprocInit(&(GBLS.sigproc), true);
  }
  return 0;
}

void
cleanupEntries(SymbolEntry *entries, ssize_t n)
{
  for (int i=0; i<n; i++) free(entries[i].name);
  free(entries);
}

static void
writeso(const char *path, const SymbolEntry *entries, ssize_t n,
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
openKallsyms(FILE **fp) {
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
  ssize_t      read;
  uintptr_t    addr;
  int          n;
  char         type;
  int          sym_start, sym_end, mod_start, mod_end;
  char *       name     = NULL;
  char *       module   = NULL;
  ssize_t      ksyms_n  = 0;
  ssize_t      ksyms_i  = 0;
  SymbolEntry *kentries = NULL;;
  
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
    writeso(path, kentries, ksyms_i, knmstrlen);
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
      writeso(path, mod->entries, mod->syms_i, mod->nmstrlen);
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

void
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
prcExec(char *exec)
{
  return 0;
}

int
main(int argc, char **argv)
{
  FILE *ksfp;
  
  if (GBLSInit(argc, argv)<0) {
    usage(argv[0], stderr);
    EEXIT();
  }

  atexit(cleanup);    // from this point on exits will trigger cleanups
  
  if (GBLS.executable) {
    prcExec(GBLS.executable);
  }
  
  if (GBLS.procbos) {
    //    HASH_ITER() {
    //      mkboso(bo);
    //    }
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
