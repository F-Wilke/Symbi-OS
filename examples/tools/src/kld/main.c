#include "incs.h"

globals_t GBLS = {
  .bosbypath      = NULL,
  .bosbymod       = NULL,
  .executable     = NULL,
  .buildexe       = NULL,
  .kotblfile      = KOTBL_DFLT,
  .dirs           = NULL,
  .fsname         = FSNAME_DFLT,
  .libkernpath    = NULL,
  .pid            = 0,
  .dirc           = 0,
  .dirmax         = 0,
  .verbose        = 0,
  .startfs        = 0,
  .prockallsyms   = 0,
  .procbos        = 1
};

static void
usage(char *name, FILE *fp)
{
  fprintf(fp,
	  "%s [-h] [-v] [-O lib] [-t kotbl path] [-K dir] "
	  "[-k kofile[,modnm[,kldopts]]] "
	  "[-o exe] [Elf]\n"
	  "   -h     : print this usage message\n"
	  "   -v     : verbose operation\n"
	  "   -O lib : create kernel so as lib for %s "
	  "(default:%s) implys '-k %s' (see below).\n"
	  "   -K dir : add a directory to search for ko files"
	  " (can be repeated)\n"
	  "   -k ko  : create a shared libk library for the ko "
	  "(can be repeated)\n"
	  "   -k /proc/kallsyms: create a shared library from\n"
	  "                      exported kernel runtime symbols\n"
	  "   -o exe : (build time) patch exe dynsym so kernel symbols\n"
	  "                      are UNDEF, enabling runtime resolution\n"
	  "            and set PT_INTERP to %s while storing original\n"
	  "            interpreter in %s\n"
	  "   elf    : optional elf file to process as follows:\n"
	  "              For each libk found in the elf:\n"
	  "                 find coresponding ko and load it\n"
	  "                 update the libk symbol table with runtime"
	  " addresses\n"
	  "              For each ko found in the elf:\n"
	  "                 extract ko and load\n"
	  "                 update the libk symbol table with runtime"
	  " addresses\n",
	  basename(name), KALLSYMSPATH, LIBKERNPATH_DFLT, KALLSYMSPATH,
	  TOSTRING(KLDSO_PATH_DFLT), KOTBL_INTERP_SEC);
}

struct KLDOPT_DESC {
  char      *optstr;
  kldopts_t  optval;
} KLDOPT_TBL[] = {
  { .optstr = "SHARED",  .optval = KLDOPT_SHARED  },
  { .optstr = "PERPROC", .optval = KLDOPT_PERPROC },
  { .optstr = "RELOAD",  .optval = KLDOPT_RELOAD  },
  { .optstr =  NULL,     .optval = KLDOPT_NONE    }
};
  
static kldopts_t
parseOpts(char *kldoptstr)
{
  struct KLDOPT_DESC *optdesc;
  kldopts_t           val = KLDOPT_NONE;
  
  for (optdesc = &(KLDOPT_TBL[0]);
       optdesc->optstr != NULL;
       optdesc++) {
    if (strcmp(optdesc->optstr, kldoptstr) == 0) {
      val = optdesc->optval;
      break;
    }
  }
  VLPRINT(2, "kld opt: %s -> %lu\n", kldoptstr, val); 
  return val;
}

static int
openko(char *kofnm, char **kofullpath)
{
  int fd=-1, i;
  char *fullpath;
  
  if (kofnm[0] == '/') {
    // kofnm is absolute
    fullpath = kofnm;
    fd = open(fullpath, O_RDONLY|O_CLOEXEC);
  } else  {
    fullpath=malloc(PATH_MAX); // avoid large stack alloc
    assert(fullpath);
    
    assert(GBLS.dirc > 0); // we expect at least one directory to check
    *kofullpath = NULL;
    for (i=0; i<GBLS.dirc; i++) {
      snprintf(fullpath, PATH_MAX, "%s/%s", GBLS.dirs[i], kofnm);
      fd = open(fullpath, O_RDONLY|O_CLOEXEC);
      if (fd != -1) {
	VPRINT("Found %s in %s (%d:%s) fd:%d.\n", kofnm, GBLS.dirs[i], i,
	       fullpath, fd);
	break;
      }
    }
  }
  if (fd != -1) {
    *kofullpath = realpath(fullpath, NULL);
    VPRINT("%s: fullpath:%s->realpath:%s fd:%d.\n", kofnm,
	   fullpath, *kofullpath, fd);

  } else {
    fprintf(stderr, "ERROR:%s:%s:%d:%s\n", __func__, kofnm,
	    errno, strerror(errno));
  }
  if (fullpath != kofnm) free(fullpath);
  return fd;
}

static int
openso(char *kofnm, char *modnm, char **sofullpath, int readonly)
{
  int fd;
  char *tmp=NULL;
  char *fullpath=malloc(PATH_MAX); // avoid large stack alloc
  assert(fullpath);
  
  if (kofnm && modnm) {
    tmp = strdup(kofnm);
    char *dir=dirname(tmp);
    snprintf(fullpath, PATH_MAX, "%s/lib%s.so", dir, modnm);
    if (readonly) {
      fd = open(fullpath, O_RDONLY);
    } else {
      fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC, SOPERM_DFLT);
    }
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
  free(fullpath);
  return fd;
}


#if 0
// not used
// if you want to support using the file name as a default
// then resurrect 
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
#endif

static int
parsekospec(const char *kospec, char **fnm, char **kldopts,
	    char **modnm)
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
    *modnm   = NULL;
    *kldopts = NULL;
    break;
  case 1:
    *modnm   = (*n == '\0') ? NULL : strdup(n);
    *kldopts = NULL;
    break;
  case 2:
    *modnm   = (*n == '\0') ? NULL : strdup(n);
    *kldopts = (*o == '\0') ? NULL : strdup(o);
  }
  
  VLPRINT(2, "%s: fnm:%s modnm:%s kldopts:%s\n", kospec, *fnm,
	  *modnm, *kldopts);
done:
  if (rc<0) free(tmp);
  return rc;
}

static bo_t *
newbo(char *kofullpath, char *modnm, char *komodnm, int forcemodnm,
      kldopts_t kldopts, char *kldoptstr, int kofd,
      char *sofullpath, int sofd)
{
  bo_t *bo = malloc(sizeof(bo_t));
  assert(bo);
  bo->kofnm      = kofullpath;
  bo->modnm      = modnm;
  bo->komodnm    = komodnm;
  bo->forcemodnm = forcemodnm;
  bo->kldopts    = kldopts;
  bo->kldoptstr  = kldoptstr; 
  bo->kofd       = kofd;
  bo->sofnm      = sofullpath;
  bo->sofd       = sofd;
  bo->loaded     = 0;
  return bo;
}

__attribute__((unused)) static void
deletebo(bo_t *bo)
{
  if (bo) {
    if (bo->kofnm)     { free(bo->kofnm); bo->kofnm = NULL; }
    if (bo->sofnm)     { free(bo->sofnm); bo->sofnm = NULL; }
    if (bo->modnm)     { free(bo->modnm); bo->modnm = NULL; }
    if (bo->komodnm)   { free(bo->komodnm); bo->komodnm = NULL; }
    if (bo->kldoptstr) { free(bo->kldoptstr); bo->kldoptstr = NULL; }
    if (bo->kofd >= 0) { close(bo->kofd); bo->kofd = -1; }
    if (bo->sofd >= 0) { close(bo->sofd); bo->sofd = -1; }
  }
}

static void
dumpbo(bo_t *bo)
{
  EPRINT(stderr, "bo:%p mod:%s komodnm=%s kldopts:%lx ko:%s (%d) "
	         "so:%s (%d) forcemodnm:%d loaded:%dn",
	 bo, bo->modnm,  bo->komodnm, bo->kldopts, bo->kofnm,
	 bo->kofd, bo->sofnm, bo->sofd, bo->forcemodnm, bo->loaded);
}

static int
addbo(char *kofnm,char *kldopts,char *modnm, bo_t **thebo, int readonly,
      int add_to_hash)
{
  bo_t        *bo         = NULL;
  kldopts_t    opts       = KLDOPT_NONE;
  char        *komodnm    = NULL;
  char        *kofullpath = NULL, *sofullpath=NULL;
  char        *origmodnm  = modnm;
  int          kofd =-1, sofd=-1, rc=0;
  
  kofd = openko(kofnm, &kofullpath);
  if (kofd < 0) { rc = -1; goto done; } 

  if (modnm == NULL || kldopts == NULL) {
    // cli / kotbl is not over riding all values in
    // the modinfo so we must get them
    kld_modinfo  mi         = {0};
    rc = kld_read_modinfo(kofullpath, kofd, &mi);
    if (rc < 0) {
      VPRINT("WARNING: could not modinfo from %s\n", kofnm);
    } else {
      if (mi.name && mi.name[0]!='\0') {
	if (modnm == NULL) { // use module name from modinfo
	  modnm = strdup(mi.name);
	  VLPRINT(2, "%s: mi.name=%s\n", kofnm, mi.name);
	} else {
	  // modnm forced; save compiled-in name for loadBO name
	  // patching
	  komodnm = strdup(mi.name);
	}
      }
      if (kldopts == NULL) {// if not null will be taken care of below
	// use kld opts from modinfo 
	for (int i=0; i<mi.kld_count; i++) {
	  opts |= parseOpts(mi.kld_vals[i]); 
	}
      }
    }
    kld_free_modinfo(&mi);
  }
  assert(modnm!=NULL);   // one way or another modnm must be set

  if (kldopts) { // if kldopts str passed in use these
    for (char *ob = kldopts, *oe = kldopts; ; oe++) {
      if (*oe == '\0' && *ob != '\0') {
	opts |= parseOpts(ob);
	break;
      }
      if (*oe == '|') {
	assert(*ob != '\0');
	*oe = '\0';  // temporarily convert '|' to null
	opts |= parseOpts(ob);
	*oe = '|';   // restore '|'
      }
      oe++;
    }
  }
    
  if (opts == KLDOPT_NONE) {  // no options set by cli/kotbl or module
    // then use defaults
    opts = KLDOPTS_DFLT;
  }
  
  sofd = openso(kofullpath, modnm, &sofullpath, readonly);
  if (sofd < 0) { rc = -1; goto done; }
  
  if (add_to_hash) {
    HASH_FIND(hhpath, GBLS.bosbypath, kofullpath, strlen(kofullpath),
	      bo);
    
    if (bo) {
      if (strcmp(bo->sofnm, sofullpath)!=0) unlink(sofullpath);
      fprintf(stderr, "WARNING: %s already specified ignoring\n",
	      kofullpath);
      rc = -1;
      goto done;
    }

    HASH_FIND(hhmod, GBLS.bosbymod, modnm, strlen(modnm), bo);
    if (bo) {
      unlink(sofullpath); // rm the ignored ko's so
      fprintf(stderr, "WARNING: %s already specified ignoring\n",
	     modnm);
      rc = -1;
      goto done;
    }
  }
  
  bo = newbo(kofullpath, modnm, komodnm, (origmodnm!=NULL), opts,
	     kldopts, kofd, sofullpath, sofd);
  // at this point bo contains all state of memory and fd's
  // cleanup when bo deleted 
  if (add_to_hash) {
    HASH_ADD_KEYPTR(hhpath, GBLS.bosbypath, bo->kofnm,
		    strlen(bo->kofnm), bo);
    HASH_ADD_KEYPTR(hhmod, GBLS.bosbymod, bo->modnm,
		    strlen(bo->modnm), bo);
  }
  
  if (GBLS.verbose>1) {
    VPRINT("Added ob: %d (%d)total obs:\n",
	   HASH_CNT(hhpath, GBLS.bosbypath),
	   HASH_CNT(hhmod, GBLS.bosbymod));
    dumpbo(bo);
  }
done:
  if (rc<0) {
    // on errors cleanup state 
    if (origmodnm == NULL && modnm != NULL) free(modnm);
    if (komodnm) free(komodnm);
    if (kofullpath) free(kofullpath);
    if (sofullpath) free(sofullpath);
    if (kofd != -1) close(kofd); 
    if (sofd != -1) close(sofd);
    if (thebo) *thebo = NULL;
  } else {
    if (thebo) *thebo = bo;
  }
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
      assert(GBLS.dirs);
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
  
  while ((opt = getopt(argc, argv, "K:k:O:o:t:hv")) != -1) {
    switch (opt) {
    case 'K':
      addDir(optarg);
      break;
    case 'O':
      GBLS.prockallsyms = 1;
      GBLS.libkernpath = optarg;  // static memory
      break;
    case 'o':
      GBLS.buildexe = optarg;  // static memory
      break;
    case 'h':
      usage(argv[0],stderr);
      return -1;
    case 'k':
      if (strcmp(KALLSYMSPATH, optarg) == 0) {
	// handle /proc/kallsysms as special case
	GBLS.prockallsyms = 1;
      }  else {
	if (kospecc == kospecmax) {
	  kospecmax = (kospecmax) ? kospecmax << 1 : 2;
	  kospec    = realloc(kospec, sizeof(char *)*kospecmax);
	  assert(kospec);
	  memset(&kospec[kospecc], 0,
		 (kospecmax - kospecc)*sizeof(char *));
	}
	kospec[kospecc] = optarg;
	kospecc++;
      }
      break;
    case 't':
      GBLS.kotblfile = optarg;
      break;
    case 'v':
      GBLS.verbose++;
      break;
    default:
      usage(argv[0],stderr);
      rc = -1;
      goto done;
    }
  }

  if (GBLS.verbose == 0) {
    char *klddebug = getenv("KLD_DEBUG");
    if (klddebug) GBLS.verbose = 1;
  }
  
  if (GBLS.libkernpath==NULL) {
    GBLS.libkernpath = LIBKERNPATH_DFLT;
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
    char *kofnm, *kldopts, *modnm;
    kofnm = kldopts = modnm = NULL;
    if (parsekospec(kospec[i], &kofnm, &kldopts, &modnm) < 0) {
      rc = -2;
      goto done;
    }
    if (addbo(kofnm, kldopts, modnm, NULL, 0, 1)<0) {
      if (kofnm)   free(kofnm);
      if (kldopts) free(kldopts);
      if (modnm)   free(modnm);
      rc = -1; goto done;
    }
  }

  int anum=argc-optind;
  char **args=&(argv[optind]);

  if (GBLS.prockallsyms==0 &&
      HASH_CNT(hhpath, GBLS.bosbypath) == 0 &&
      anum < 1 && !GBLS.buildexe) {
    fprintf(stderr, "kld: no input files\n");
    rc = -1;
    goto done;
  }
  
  if (anum >= 1) {
    if (kospecc) {
      fprintf(stderr, "ERROR: executable and ko specified.\n");
      rc = -1;
      goto done;
    }
    if (GBLS.buildexe) {
      fprintf(stderr, "ERROR: -o and positional executable cannot both be specified.\n");
      rc = -1;
      goto done;
    }
    GBLS.executable = args[0];
    VLPRINT(2, "GBLS.executable=%s\n", GBLS.executable);
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
cleanupEntries(kld_sym *entries, size_t n)
{
  for (size_t i=0; i<n; i++) free(entries[i].name);
  free(entries);
}

static int
writeso(const char *path, int fd, const kld_sym *entries, size_t n,
	 size_t nmstrlen)
{
  int mfd;
  int dfd=fd;
  size_t elf_size;
  int rc = 0;
  int buildtime = (GBLS.executable == NULL);
  const char *soname = strrchr(path, '/');
  soname = soname ? soname + 1 : path;
  void *elf_ptr = kld_generate_elf_mmap(entries, n, nmstrlen, soname, buildtime, &elf_size, &mfd);
  
  if (elf_ptr == MAP_FAILED) return -1;

  VPRINT("%s: ELF mapped at %p (Size: %zu)\n", path, elf_ptr, elf_size);
  
  if (fd==-1) {
    dfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dfd == -1) {
      perror(path);
      rc = -1;
      goto done;
    }
  } 
  {
    ssize_t nw = write(dfd, elf_ptr, elf_size);
    if (nw < 0 || (size_t)nw != elf_size) {
      perror(path);
      rc = -1;
    }
  }
  if (fd == -1) close(dfd);

 done:
  munmap(elf_ptr, elf_size);
  close(mfd);
  return rc;
}

#if 0
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
#endif

static int
openKallsyms(FILE **fp)
{
  FILE      *fp_;
  fp_ = fopen(KALLSYMSPATH, "r");
  if (fp_ == NULL) {
    warn(__FUNCTION__);
    return -1;
  }  
  *fp    = fp_;
  return 0;
}

#define KLD_RUN_FALLBACK_ROOT "/run/kld"
#define KLD_RUN_FALLBACK_TMP  "/tmp"

static uint64_t
fnv1a64_update(uint64_t h, const void *buf, size_t n)
{
  const unsigned char *p = (const unsigned char *)buf;
  for (size_t i = 0; i < n; i++) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ULL;
  }
  return h;
}

static uint64_t
fnv1a64_str(const char *s)
{
  return fnv1a64_update(1469598103934665603ULL, s, strlen(s));
}

static int
read_boot_id(char *buf, size_t n)
{
  FILE *f = fopen("/proc/sys/kernel/random/boot_id", "r");
  if (!f) return -1;
  if (!fgets(buf, (int)n, f)) {
    fclose(f);
    return -1;
  }
  fclose(f);
  size_t len = strlen(buf);
  while (len && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
  return (len > 0) ? 0 : -1;
}

static int
parse_uid_env(const char *s, uid_t *uid_out)
{
  char *end = NULL;
  unsigned long v;
  if (!s || !*s) return -1;
  errno = 0;
  v = strtoul(s, &end, 10);
  if (errno || !end || *end != '\0') return -1;
  *uid_out = (uid_t)v;
  return 0;
}

static int
resolve_run_dir(char *out, size_t outn)
{
  uid_t euid = geteuid();
  uid_t owner = euid;
  uid_t sudo_uid = 0;
  int has_sudo_uid = (euid == 0 &&
                      parse_uid_env(getenv("SUDO_UID"), &sudo_uid) == 0);
  char base[PATH_MAX];
  const char *xdg = getenv("XDG_RUNTIME_DIR");
  int n;

  if (has_sudo_uid) owner = sudo_uid;

  if (has_sudo_uid) {
    n = snprintf(base, sizeof(base), "/run/user/%lu",
                 (unsigned long)owner);
    if (n > 0 && (size_t)n < sizeof(base) &&
        access(base, W_OK | X_OK) == 0) {
      n = snprintf(out, outn, "%s/kld", base);
      return (n > 0 && (size_t)n < outn) ? 0 : -1;
    }
  }

  if (xdg && xdg[0] == '/' && access(xdg, W_OK | X_OK) == 0) {
    n = snprintf(out, outn, "%s/kld", xdg);
    return (n > 0 && (size_t)n < outn) ? 0 : -1;
  }

  n = snprintf(base, sizeof(base), "/run/user/%lu", (unsigned long)euid);
  if (n > 0 && (size_t)n < sizeof(base) &&
      access(base, W_OK | X_OK) == 0) {
    n = snprintf(out, outn, "%s/kld", base);
    return (n > 0 && (size_t)n < outn) ? 0 : -1;
  }

  if (euid == 0) {
    n = snprintf(out, outn, "%s/u%lu", KLD_RUN_FALLBACK_ROOT,
                 (unsigned long)owner);
  } else {
    n = snprintf(out, outn, "%s/kld.u%lu", KLD_RUN_FALLBACK_TMP,
                 (unsigned long)euid);
  }
  return (n > 0 && (size_t)n < outn) ? 0 : -1;
}

static int
ensure_dir_tree(const char *path, mode_t mode)
{
  char tmp[PATH_MAX];
  size_t len;
  if (!path || !*path) return -1;
  len = strnlen(path, sizeof(tmp));
  if (len == 0 || len >= sizeof(tmp)) return -1;
  memcpy(tmp, path, len + 1);
  for (size_t i = 1; i < len; i++) {
    if (tmp[i] == '/') {
      tmp[i] = '\0';
      if (mkdir(tmp, mode) < 0 && errno != EEXIST) return -1;
      tmp[i] = '/';
    }
  }
  if (mkdir(tmp, mode) < 0 && errno != EEXIST) return -1;
  return 0;
}

static int
ensure_stamp_parent_dir(const char *stamp_path)
{
  char tmp[PATH_MAX];
  char *dir;
  size_t n = strnlen(stamp_path, sizeof(tmp));
  if (n == 0 || n >= sizeof(tmp)) return -1;
  memcpy(tmp, stamp_path, n + 1);
  dir = dirname(tmp);
  return ensure_dir_tree(dir, 0700);
}

static void
make_stamp_path(char *out, size_t outn, const char *kind, const char *key)
{
  char run_dir[PATH_MAX];
  uint64_t h = fnv1a64_str(key);
  if (resolve_run_dir(run_dir, sizeof(run_dir)) < 0) {
    out[0] = '\0';
    return;
  }
  snprintf(out, outn, "%s/%s.%016" PRIx64 ".stamp", run_dir, kind, h);
}

static uintptr_t
syms_text_anchor(const kld_sym *entries, size_t n)
{
  uintptr_t a = 0;
  for (size_t i = 0; i < n; i++) {
    uintptr_t v = (uintptr_t)entries[i].sym.st_value;
    if (v == 0) continue;
    if (a == 0 || v < a) a = v;
  }
  return a;
}

static int
cmp_sym_name_ptr(const void *a, const void *b)
{
  const kld_sym *sa = *(const kld_sym *const *)a;
  const kld_sym *sb = *(const kld_sym *const *)b;
  return strcmp(sa->name, sb->name);
}

static uint64_t
syms_hash(const kld_sym *entries, size_t n)
{
  if (!entries || n == 0) return 0;
  const kld_sym **ptrs = malloc(sizeof(ptrs[0]) * n);
  if (!ptrs) return 0;
  for (size_t i = 0; i < n; i++) ptrs[i] = &entries[i];
  qsort(ptrs, n, sizeof(ptrs[0]), cmp_sym_name_ptr);

  uint64_t h = 1469598103934665603ULL;
  for (size_t i = 0; i < n; i++) {
    const kld_sym *s = ptrs[i];
    h = fnv1a64_update(h, s->name, strlen(s->name) + 1);
    h = fnv1a64_update(h, &(s->sym.st_value), sizeof(s->sym.st_value));
  }
  free(ptrs);
  return h;
}

static int
read_stamp(const char *stamp_path, char *boot_id, size_t boot_n,
           uintptr_t *anchor, uint64_t *hash)
{
  FILE *f = fopen(stamp_path, "r");
  if (!f) return -1;
  char bid[128] = {0};
  uintptr_t a = 0;
  uint64_t h = 0;
  int n = fscanf(f, "boot=%127s\nanchor=%" SCNxPTR "\nhash=%" SCNx64 "\n",
                 bid, &a, &h);
  fclose(f);
  if (n != 3) return -1;
  strncpy(boot_id, bid, boot_n - 1);
  boot_id[boot_n - 1] = '\0';
  if (anchor) *anchor = a;
  if (hash) *hash = h;
  return 0;
}

static int
write_stamp(const char *stamp_path, const char *boot_id,
            uintptr_t anchor, uint64_t hash)
{
  if (ensure_stamp_parent_dir(stamp_path) < 0) return -1;
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s.tmp.%ld", stamp_path, (long)getpid());
  FILE *f = fopen(tmp, "w");
  if (!f) return -1;
  fprintf(f, "boot=%s\nanchor=%" PRIxPTR "\nhash=%" PRIx64 "\n",
          boot_id, anchor, hash);
  fclose(f);
  if (rename(tmp, stamp_path) < 0) {
    unlink(tmp);
    return -1;
  }
  return 0;
}

/*
 * stamp_covers_so - return 1 if the stamp file is strictly newer than the .so.
 *
 * The runtime always writes the stamp AFTER updating the .so (write_stamp uses
 * atomic rename), so a valid stamp is never older than the .so it covers.
 * If the .so is newer the stamp is stale — build mode (writeso) rewrote the
 * .so with build-time placeholder addresses without resetting the stamp.
 * Use nanosecond precision to avoid false matches when make rebuilds the .so
 * within the same second the stamp was written.
 */
static int
stamp_covers_so(const char *stamp_path, const char *so_path)
{
  struct stat stamp_st, so_st;
  if (stat(stamp_path, &stamp_st) < 0) return 0;
  if (stat(so_path, &so_st) < 0) return 0;
  if (stamp_st.st_mtime != so_st.st_mtime)
    return stamp_st.st_mtime > so_st.st_mtime;
  return stamp_st.st_mtim.tv_nsec >= so_st.st_mtim.tv_nsec;
}

static int
update_so_runtime(const char *path, const kld_sym *entries, size_t n,
                  size_t nmstrlen)
{
  if (kld_update_elf_dynsym(path, entries, n) == 0) {
    VLPRINT(2, "runtime so update in-place: %s\n", path);
    return 0;
  }
  VLPRINT(1, "runtime so update fallback rebuild: %s\n", path);
  if (writeso(path, -1, entries, n, nmstrlen) < 0) {
    VLPRINT(1, "runtime so update failed: %s\n", path);
    return -1;
  }
  return 0;
}

static int
fast_precheck_runtime_skip(int *all_skip)
{
  FILE *fp = NULL;
  char *line = NULL, *module = NULL;
  size_t len = 0;
  uintptr_t addr = 0;
  int n = 0, mod_start = 0, mod_end = 0;
  char type = 0;

  struct ModAnchor {
    UT_hash_handle hh;
    char *name;
    bo_t *bo;
    uintptr_t anchor;
    int seen;
    int skip;
  } *m = NULL;

  char boot_id[128] = {0};
  int libkern_skip = 0;
  *all_skip = 0;

  if (read_boot_id(boot_id, sizeof(boot_id)) < 0) return -1;

  bo_t *bo_it, *bo_tmp;
  HASH_ITER(hhmod, GBLS.bosbymod, bo_it, bo_tmp) {
    struct ModAnchor *x = calloc(1, sizeof(*x));
    x->name = strdup(bo_it->modnm);
    x->bo = bo_it;
    x->skip = 0;
    HASH_ADD_KEYPTR(hh, m, x->name, strlen(x->name), x);
  }

  if (openKallsyms(&fp) < 0) goto done;
  while (getline(&line, &len, fp) != -1) {
    mod_start = mod_end = 0;
    n = sscanf(line, "%" SCNxPTR " %c %*s %n%*s%n", &addr, &type,
               &mod_start, &mod_end);
    if (n != 2 || mod_start == 0 || mod_end <= mod_start) continue;
    module = &line[mod_start];
    module[0] = '['; module++;
    if (line[mod_end-1] == '\n' || line[mod_end-1] == '\r' ||
        line[mod_end-1] == ']') line[mod_end-1] = '\0';
    line[mod_end] = '\0';
    struct ModAnchor *x = NULL;
    HASH_FIND_STR(m, module, x);
    if (!x) continue;
    if (!x->seen || addr < x->anchor) x->anchor = addr;
    x->seen = 1;
  }

  {
    char sp[PATH_MAX], sbid[128];
    uintptr_t sanchor = 0;
    uint64_t shash = 0;
    make_stamp_path(sp, sizeof(sp), "libkern", GBLS.libkernpath);
    if (sp[0] == '\0') {
      VLPRINT(1, "%s", "runtime stamp path unavailable; skipping precheck stamp read\n");
    } else {
      libkern_skip = (access(GBLS.libkernpath, R_OK) == 0 &&
                      read_stamp(sp, sbid, sizeof(sbid), &sanchor, &shash) == 0 &&
                      strcmp(sbid, boot_id) == 0);
    }
  }

  *all_skip = libkern_skip;
  struct ModAnchor *x, *xt;
  HASH_ITER(hh, m, x, xt) {
    char sp[PATH_MAX], sbid[128];
    uintptr_t sanchor = 0;
    uint64_t shash = 0;
    x->skip = 0;
    if (x->seen && access(x->bo->sofnm, R_OK) == 0) {
      make_stamp_path(sp, sizeof(sp), "mod", x->bo->sofnm);
      if (sp[0] == '\0') {
        VLPRINT(1, "runtime stamp path unavailable; skipping precheck stamp read: %s\n",
                x->bo->sofnm);
      } else if (read_stamp(sp, sbid, sizeof(sbid), &sanchor, &shash) == 0 &&
                 strcmp(sbid, boot_id) == 0 && sanchor == x->anchor) {
          x->skip = 1;
      }
    }
    if (!x->skip) *all_skip = 0;
    HASH_DEL(m, x);
    free(x->name);
    free(x);
  }

done:
  if (fp) fclose(fp);
  if (line) free(line);
  return 0;
}

static void
prcKallsyms(FILE *fp, char **kernpath)
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
  kld_sym     *kentries = NULL;;
  
  struct Module {
    UT_hash_handle hh;
    kld_sym       *entries;
    char          *name;
    bo_t          *bo;
    size_t         syms_n;
    size_t         syms_i;
    size_t         nmstrlen;
  } * mhash = NULL;

  int all_skip = 0;
  if (fast_precheck_runtime_skip(&all_skip) == 0 && all_skip) {
    VLPRINT(2, "%s", "runtime fast-precheck matched; verifying via full parse\n");
  }

  
  while (getline(&line, &len, fp) != -1 ) {
    sym_start = sym_end = mod_start = mod_end = 0;
    name = module = NULL;
    n = sscanf(line, "%" SCNxPTR " %c %n%*s%n %n%*s%n", &addr, &type,
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
	// printf("%d: addr:0x%" PRIx64 " type:%c symbol:%s"
	//        " module:%s\n",n, addr, type, name, module);
	struct Module *mod = NULL;
	HASH_FIND_STR(mhash, module, mod);
	if (mod==NULL) {
	  // only process modules that we are intrested in
	  bo_t *bo      = NULL;
	  HASH_FIND(hhmod, GBLS.bosbymod, module, strlen(module), bo);
	  if (bo == NULL) continue;
	  
	  mod           = malloc(sizeof(struct Module));
	  assert(mod);
	  mod->name     = strdup(module);
	  mod->bo       = bo;
	  mod->syms_n   = 0;
	  mod->syms_i   = 0;
	  mod->nmstrlen = 0;
	  mod->entries  = NULL;
	  
          // memset(mod->entries, 0,
	  //        sizeof(mod->syms_n * sizeof(SymbolEntry)));
	  HASH_ADD_KEYPTR(hh, mhash, mod->name, strlen(mod->name),
			  mod);
	}
	if (mod->syms_i == mod->syms_n) {
	  mod->syms_n = (mod->syms_n) ? mod->syms_n << 1 : 1024;
	  mod->entries = realloc((void *)mod->entries,
				 mod->syms_n*sizeof(kld_sym));
	  assert(mod->entries);
	  // for good measure zero out new memory
	  memset(&(mod->entries[mod->syms_i]), 0,
		 (mod->syms_n - mod->syms_i) * sizeof(kld_sym));
	}
	if (kld_sym_init_from_kallsyms(&(mod->entries[mod->syms_i]),
				       addr, name, type)>=0) {
	  mod->nmstrlen += nmlen;
	  mod->syms_i++;
	}
      } else {
	// add to libkern.so symbol entries
	// printf("%d: addr:0x%" PRIx64 " type:%c symbol:%s\n",
	//        n,addr, type, name);
	if (ksyms_i == ksyms_n) {
	  ksyms_n = (ksyms_n) ? ksyms_n << 1 : 1024;
	  kentries = realloc((void *)kentries,			     
			     ksyms_n*sizeof(kld_sym));
	  assert(kentries);
	  // for good measure zero out new memory
	  memset(&(kentries[ksyms_i]), 0,
		 (ksyms_n - ksyms_i) * sizeof(kld_sym));
	}
	if (kld_sym_init_from_kallsyms(&kentries[ksyms_i],addr, name,
				       type)>=0) {
	  knmstrlen += nmlen;
	  ksyms_i++;
	}
      }
    }
  }
  
  if (ksyms_i) {
    assert(GBLS.libkernpath);
    uint64_t khash = syms_hash(kentries, ksyms_i);
    uintptr_t kanchor = syms_text_anchor(kentries, ksyms_i);
    char boot_id[128] = {0}, sbid[128] = {0}, sp[PATH_MAX];
    uintptr_t sanchor = 0;
    uint64_t shash = 0;
    int do_update = 1;
    if (read_boot_id(boot_id, sizeof(boot_id)) == 0) {
      make_stamp_path(sp, sizeof(sp), "libkern", GBLS.libkernpath);
      if (sp[0] == '\0') {
        VLPRINT(1, "%s", "runtime stamp path unavailable; skipping stamp read/write for libkern\n");
      } else if (access(GBLS.libkernpath, R_OK) == 0 &&
                 read_stamp(sp, sbid, sizeof(sbid), &sanchor, &shash) == 0 &&
                 strcmp(sbid, boot_id) == 0 && shash == khash &&
                 stamp_covers_so(sp, GBLS.libkernpath)) {
        do_update = 0;
      }
      if (do_update) {
        if (update_so_runtime(GBLS.libkernpath, kentries, ksyms_i, knmstrlen) == 0) {
          if (sp[0] != '\0' && write_stamp(sp, boot_id, kanchor, khash) < 0) {
            VLPRINT(1, "runtime stamp write failed: %s\n", sp);
          }
        } else {
          VLPRINT(1, "runtime so update failed, stamp not written: %s\n",
                  GBLS.libkernpath);
        }
      } else {
        VLPRINT(2, "runtime so skip (hash match): %s\n", GBLS.libkernpath);
      }
    } else {
      update_so_runtime(GBLS.libkernpath, kentries, ksyms_i, knmstrlen);
    }
    if (GBLS.buildexe) {
      const char *ksname = strrchr(GBLS.libkernpath, '/');
      ksname = ksname ? ksname + 1 : GBLS.libkernpath;
      kld_undef_elf_dynsym(GBLS.buildexe, ksname, kentries, ksyms_i);
    }
    *kernpath = realpath(GBLS.libkernpath, NULL);
    cleanupEntries(kentries, ksyms_i);
  }
  
  {
    struct Module *mod, *tmp;
    HASH_ITER(hh, mhash, mod, tmp) {
      uint64_t mhashv = syms_hash(mod->entries, mod->syms_i);
      uintptr_t manchor = syms_text_anchor(mod->entries, mod->syms_i);
      char boot_id[128] = {0}, sbid[128] = {0}, sp[PATH_MAX];
      uintptr_t sanchor = 0;
      uint64_t shash = 0;
      int do_update = 1;
      if (read_boot_id(boot_id, sizeof(boot_id)) == 0) {
        make_stamp_path(sp, sizeof(sp), "mod", mod->bo->sofnm);
        if (sp[0] == '\0') {
          VLPRINT(1, "runtime stamp path unavailable; skipping stamp read/write: %s\n",
                  mod->bo->sofnm);
        } else if (access(mod->bo->sofnm, R_OK) == 0 &&
                   read_stamp(sp, sbid, sizeof(sbid), &sanchor, &shash) == 0 &&
                   strcmp(sbid, boot_id) == 0 &&
                   sanchor == manchor && shash == mhashv &&
                   !(mod->bo->kldopts & KLDOPT_RELOAD) &&
                   stamp_covers_so(sp, mod->bo->sofnm)) {
          do_update = 0;
        }
        if (do_update) {
          if (update_so_runtime(mod->bo->sofnm, mod->entries,
                                mod->syms_i, mod->nmstrlen) == 0) {
            if (sp[0] != '\0' && write_stamp(sp, boot_id, manchor, mhashv) < 0) {
              VLPRINT(1, "runtime stamp write failed: %s\n", sp);
            }
          } else {
            VLPRINT(1, "runtime so update failed, stamp not written: %s\n",
                    mod->bo->sofnm);
          }
        } else {
          VLPRINT(2, "runtime so skip (anchor/hash match): %s\n", mod->bo->sofnm);
        }
      } else {
        update_so_runtime(mod->bo->sofnm, mod->entries,
                          mod->syms_i, mod->nmstrlen);
      }
      if (GBLS.buildexe) {
        const char *msname = strrchr(mod->bo->sofnm, '/');
        msname = msname ? msname + 1 : mod->bo->sofnm;
        kld_undef_elf_dynsym(GBLS.buildexe, msname, mod->entries, mod->syms_i);
      }
      cleanupEntries(mod->entries, mod->syms_i);
      free(mod->name);
      HASH_DEL(mhash, mod);
      free((struct Module *)mod);
    }
  }

  free(line);
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
  bo_t *bo, *tmp;
  HASH_ITER(hhpath, GBLS.bosbypath, bo, tmp) {
    HASH_DELETE(hhpath, GBLS.bosbypath, bo);
    HASH_DELETE(hhmod, GBLS.bosbymod, bo);
    deletebo(bo);
    free(bo);
  }
}

static void
cleanup(void)
{
  if (GBLS.startfs)  {
    fsCleanup(&(GBLS.fs));
    sigprocCleanup(&(GBLS.sigproc));
  }
  
  if (GBLS.bosbypath) {
    bosCleanup();
    GBLS.bosbypath = NULL;
    GBLS.bosbymod  = NULL; 
  }
  if (GBLS.dirs) {
    free(GBLS.dirs);
    GBLS.dirs = NULL;
  }
}

static int
prcBO(bo_t *bo)
{
  int  rc = 0;
  kld_sym *entries = NULL;
  size_t   n=0, nmstrlen=0;
  
  if (GBLS.verbose) {
    VPRINT("Processing bo (%p):\n", bo);
    dumpbo(bo);
  }
  if (kld_read_elf_syms(bo->kofnm, bo->kofd, &entries, &n, &nmstrlen)<0) {
    rc = -1;
    goto done;
  }
  if (writeso(bo->sofnm, bo->sofd, entries, n, nmstrlen) < 0) {
    rc = -1;
    goto done;
  }
  if (GBLS.buildexe) {
    const char *bsname = strrchr(bo->sofnm, '/');
    bsname = bsname ? bsname + 1 : bo->sofnm;
    kld_undef_elf_dynsym(GBLS.buildexe, bsname, entries, n);
  }
  VPRINT("%s: read %zu symbols\n", bo->kofnm, n);

 done:
  if (entries) cleanupEntries(entries,n);
  return rc;
}

extern int loadBO(bo_t *bo);

static size_t
parsekotbl(char *kotblnxt,  char **kofnm, char **modnm, char **koopts)
{
  assert(kotblnxt != NULL);
  assert(*kofnm   == NULL);
  assert(*koopts  == NULL);
  assert(*modnm   == NULL);
  size_t n,nn;
  char *ptr = kotblnxt;

 
  // Loop until -1 is returned (EOF or error)
  n = strlen(ptr);
  if (n==0) { nn=0; goto done; }
  *kofnm = strdup(ptr);
  ptr += (n+1); // +1 to skip null
  nn   = (n+1);
  
  n = strlen(ptr);
  if (n>0) { 
    *modnm = strdup(ptr);
  }
  ptr += (n+1); // +1 to skip null
  nn  += (n+1);
  
  n = strlen(ptr);
  if (n>0) {
    *koopts = strdup(ptr);
  }
  ptr += (n+1); // +1 to skip null
  nn  += (n+1);
 done:
  if (nn==0) {
    if (*kofnm)  free(*kofnm);
    if (*modnm)  free(*modnm);
    if (*koopts) free(*koopts);
  }
  return nn;
}

static void
append_ld_dir(const char *sofnm, char ***dirs, int *dirc, int *dirmax)
{
  char *tmp = strdup(sofnm);
  char *dir = dirname(tmp);
  for (int i = 0; i < *dirc; i++) {
    if (strcmp((*dirs)[i], dir) == 0) {
      free(tmp);
      return;
    }
  }
  if (*dirc == *dirmax) {
    *dirmax = (*dirmax) ? (*dirmax << 1) : 8;
    *dirs = realloc(*dirs, *dirmax * sizeof((*dirs)[0]));
  }
  (*dirs)[*dirc] = strdup(dir);
  (*dirc)++;
  free(tmp);
}

static char *
build_ldpath(char **dirs, int dirc)
{
  size_t tot = 1;
  for (int i = 0; i < dirc; i++) tot += strlen(dirs[i]) + 1;
  char *ldp = calloc(1, tot);
  if (!ldp) return NULL;
  for (int i = 0; i < dirc; i++) {
    strcat(ldp, dirs[i]);
    if (i + 1 < dirc) strcat(ldp, ":");
  }
  return ldp;
}

static void
maybe_install_kldso_interp(const char *exe)
{
  char interp[PATH_MAX] = {0};
  if (kld_get_interp(exe, interp, sizeof(interp)) < 0) return;
  if (strcmp(interp, TOSTRING(KLDSO_PATH_DFLT)) == 0) return;

  kld_add_elf_section(exe, KOTBL_INTERP_SEC, interp, strlen(interp) + 1);
  if (kld_set_interp(exe, TOSTRING(KLDSO_PATH_DFLT)) < 0) {
    fprintf(stderr, "WARNING: failed to set interpreter for %s\n", exe);
  }
}

static int
get_exec_orig_interp(const char *exe, char *buf, size_t bufsz)
{
  kld_secdata sd;
  if (kld_open_elf_secdata(&sd, exe, -1, KOTBL_INTERP_SEC) >= 0) {
    size_t n = sd.size;
    if (n >= bufsz) n = bufsz - 1;
    memcpy(buf, sd.data, n);
    buf[n] = '\0';
    kld_close_elf_secdata(&sd, -1);
    if (buf[0]) return 0;
  }
  return kld_get_interp(exe, buf, bufsz);
}

/* read_file_buf - read an entire file into a malloc'd buffer.
   Caller must free(*buf) on success. */
static int
read_file_buf(const char *path, char **buf, size_t *outsz)
{
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "ERROR: %s: %s\n", path, strerror(errno));
    return -1;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz <= 0) { fclose(f); *buf = NULL; *outsz = 0; return 0; }
  char *b = malloc((size_t)sz + 1);
  if (!b) { fclose(f); return -1; }
  if ((long)fread(b, 1, (size_t)sz, f) != sz) {
    perror(path); fclose(f); free(b); return -1;
  }
  fclose(f);
  b[sz] = '\0';
  *buf  = b;
  *outsz = (size_t)sz;
  return 0;
}

/*
 * prcExec - process the .kotbl section embedded in an ELF executable.
 *
 * buildtime=0 (runtime): parses kotbl, calls addbo+loadBO to rebuild .so
 *   files with live kernel addresses.
 *
 * buildtime=1 (build-time patch): parses kotbl, derives the path of each
 *   kld-generated .so, reads its exported symbols, and calls
 *   kld_undef_elf_dynsym to turn absorbed WEAK-ABS placeholders in exec's
 *   dynsym into UNDEF entries so ld.so scope-searches at runtime.
 */
static int
prcExec(const char *exec, int buildtime, char **ldpath_out)
{
  kld_secdata sd;
  int rc = 0;
  bo_t *bo=NULL, **bos=NULL;
  int bosi=0, bosn=0;
  char **lddirs = NULL;
  int lddirc = 0, lddirmax = 0;
  rc = kld_open_elf_secdata(&sd, exec, -1, KOTBL_SEC);
  if (rc>=0) {
    VLPRINT(2, "%s:%s mapped\n", exec, KOTBL_SEC);
    char *kotbl = (char *)sd.data;
    size_t size = sd.size, n, nn=0;
    char *fnm, *modnm, *opts;
    int i=0;
  
    while (nn<size) {
      fnm = modnm = opts = NULL;
      n = parsekotbl(&(kotbl[nn]), &fnm, &modnm, &opts);
      if (n == 0) break;
      nn+=n;
      VLPRINT(2, "kotbl[%d]: fnm:%s modnm:%s opts:%s\n", i, fnm,
	      modnm, opts);

      if (buildtime) {
	/* build-time: read existing .so, patch exec dynsym */
	if (modnm && strcmp(modnm, KALLSYMSPATH) == 0) {
	  const char *soname = strrchr(fnm, '/');
	  soname = soname ? soname + 1 : fnm;
	  int sofd = open(fnm, O_RDONLY);
	  if (sofd < 0) {
	    VPRINT("WARNING: %s: %s\n", fnm, strerror(errno));
	  } else {
	    kld_sym *entries = NULL;
	    size_t nent = 0, nmstrlen = 0;
	    if (kld_read_elf_syms(fnm, sofd, &entries, &nent, &nmstrlen) >= 0
		&& nent > 0)
	      kld_undef_elf_dynsym(exec, soname, entries, nent);
	    cleanupEntries(entries, nent);
	    close(sofd);
	  }
	  if (ldpath_out) append_ld_dir(fnm, &lddirs, &lddirc, &lddirmax);
	  free(fnm); free(modnm); free(opts);
	} else {
	  bo_t *bbo = NULL;
	  if (addbo(fnm, opts, modnm, &bbo, 1, 0) < 0 || !bbo) {
	    free(fnm); free(modnm); free(opts);
	    rc = -1;
	    goto done;
	  }
	  if (ldpath_out) append_ld_dir(bbo->sofnm, &lddirs, &lddirc, &lddirmax);
	  const char *soname = strrchr(bbo->sofnm, '/');
	  soname = soname ? soname + 1 : bbo->sofnm;
	  kld_sym *entries = NULL;
	  size_t nent = 0, nmstrlen = 0;
	  if (kld_read_elf_syms(bbo->sofnm, bbo->sofd, &entries, &nent, &nmstrlen) >= 0
	      && nent > 0)
	    kld_undef_elf_dynsym(exec, soname, entries, nent);
	  cleanupEntries(entries, nent);
	  deletebo(bbo);
	  free(bbo);
	}
      } else {
	/* runtime: collect bo list for loadBO */
	if (modnm != NULL && strcmp(modnm, KALLSYMSPATH)==0) {
	  GBLS.libkernpath = fnm;
	  if (ldpath_out) append_ld_dir(fnm, &lddirs, &lddirc, &lddirmax);
	} else {
	  if (addbo(fnm, opts, modnm, &bo, 1, 1) <0) {
	    assert(0);
	    rc = -1;
	    goto done;
	  } else {
	    if (ldpath_out) append_ld_dir(bo->sofnm, &lddirs, &lddirc, &lddirmax);
	    if (bosi==bosn) {
	      bosn = (bosn==0) ? 16 : bosn<<1;
	      bos  = realloc(bos, sizeof(bos[0])*bosn);
	    }
	    bos[bosi] = bo;
	    bosi++;
	  }
	}
      }
      i++;
    }
    kld_close_elf_secdata(&sd,-1);
    if (i && !buildtime) GBLS.prockallsyms = 1;
  }

done:
  for (int i=0; i<bosi; i++) {
    if (loadBO(bos[i])<0) {
      rc = -1;
    }
  }
  if (ldpath_out) {
    *ldpath_out = build_ldpath(lddirs, lddirc);
  }
  if (bos) free(bos);
  for (int i = 0; i < lddirc; i++) free(lddirs[i]);
  if (lddirs) free(lddirs);
  return rc;
}

static int
kldd(int argc, char **argv)
{
  kld_secdata sd;
  char *exec;
  int rc;

  if (strcmp("kldd",basename(argv[0])) != 0) return 0;
  
  if (argc != 2) {
    fprintf(stderr, "ERROR: no input file\nkldd <file>\n");
    return -1;
  }
  exec = argv[1];
  
  rc = kld_open_elf_secdata(&sd, exec, -1, KOTBL_SEC);
  
  if (rc>=0) {
    char *kotbl = (char *)sd.data;
    size_t size = sd.size, n, nn=0;
    char *fnm, *modnm, *opts;
    VLPRINT(2, "%s:%s mapped\n", exec, KOTBL_SEC);
  
    while (nn<size) {
      fnm = modnm = opts = NULL;
      n = parsekotbl(&(kotbl[nn]), &fnm, &modnm, &opts);
      if (n == 0) break;
      nn+=n;
      printf("%s\n", fnm);
      if (fnm) free(fnm);
      if (modnm) free(modnm);
      if (opts) free(opts);
    }
    kld_close_elf_secdata(&sd,-1);
    rc = 1;
  } else {
    rc = -1;
  }

  return rc;
}

int
main(int argc, char **argv)
{
  FILE *ksfp   = NULL;
  FILE *kotblf = NULL;

  {
    int rc = kldd(argc, argv);
    if (rc != 0) {
      if (rc>0)   return EXIT_SUCCESS;
      else EEXIT();
    }
  }
  
  if (GBLSInit(argc, argv)<0) {
    EEXIT();
  }

  atexit(cleanup);  // from this point on exits will trigger cleanups

  /* true when -o is given with no -k/-K flags: read existing kotbl.bin */
  int no_kflags = (GBLS.buildexe &&
                   HASH_CNT(hhpath, GBLS.bosbypath) == 0 &&
                   !GBLS.prockallsyms);

  if (GBLS.executable) {
    char *ldpath = NULL;
    char interp[PATH_MAX] = {0};
    prcExec(GBLS.executable, 0, &ldpath);
    if (get_exec_orig_interp(GBLS.executable, interp, sizeof(interp)) == 0) {
      char *execpath = realpath(GBLS.executable, NULL);
      const char *ep = execpath ? execpath : GBLS.executable;
      const char *lp = (ldpath && ldpath[0]) ? ldpath : "";
      /* Structured output for kldso.sh: interp\0execpath\0ldpath\0 */
      fwrite(interp, 1, strlen(interp), stdout);
      fwrite("\0",1, 1, stdout);
      fwrite(ep, 1, strlen(ep), stdout);
      fwrite("\0",1, 1, stdout);
      fwrite(lp, 1, strlen(lp), stdout);
      fwrite("\0",1, 1, stdout);
      if (execpath) free(execpath);
    } else {
      fprintf(stderr, "ERROR: failed to determine interpreter for %s\n",
              GBLS.executable);
    }
    if (ldpath) free(ldpath);
  } else if (no_kflags) {
    /* No-KFLAGS build-time mode:
       1. embed existing kotbl.bin as .kotbl ELF section
       2. parse .kotbl, read existing .so symbols, patch dynsym */
    char *kd  = NULL;
    size_t ksz = 0;
    if (read_file_buf(GBLS.kotblfile, &kd, &ksz) < 0 || ksz == 0) {
      fprintf(stderr, "ERROR: %s: not found or empty; run kcc first\n",
	      GBLS.kotblfile);
      EEXIT();
    }
    kld_add_elf_section(GBLS.buildexe, KOTBL_SEC, kd, ksz);
    free(kd);
    prcExec(GBLS.buildexe, 1, NULL);
    maybe_install_kldso_interp(GBLS.buildexe);
  } else {
    kotblf = fopen(GBLS.kotblfile, "w");
    if (kotblf == NULL) {
      perror("Error opening file");
      return EXIT_FAILURE;
    }
    // process each of the bo's found either from command line or
    // from the executable
    if (GBLS.procbos && HASH_CNT(hhpath, GBLS.bosbypath)) {
      bo_t *bo, *tmp;
      HASH_ITER(hhpath, GBLS.bosbypath, bo, tmp) {
	// use null as seperators to allow paths to contain all valid 
	// ascii chars including whitespaces chars and newlines 
	if (prcBO(bo)==0) {
	  size_t n = strlen(bo->kofnm);
	  assert(1==fwrite(bo->kofnm,n,1,kotblf));
	  assert(1==fwrite("\0",1,1,kotblf));
	  if (bo->forcemodnm) {
	    n = strlen(bo->modnm);
	    assert(1==fwrite(bo->modnm,n,1,kotblf));
	  }
	  assert(1==fwrite("\0",1,1,kotblf));
	  if (bo->kldoptstr) {
	    n = strlen(bo->kldoptstr);
	    assert(1==fwrite(bo->kldoptstr,n,1,kotblf));
	  }
	  assert(1==fwrite("\0",1,1,kotblf));
	} else {
	  fprintf(stderr, "ERROR: failed to process bo:%s %s %s\n",
		  bo->kofnm, bo->modnm, bo->kldoptstr);
	}
      }
    }
  }
  
  // we do this last so that ko loads will be reflected in so updates
    if (GBLS.prockallsyms) {
      char *kernpath;
      if (openKallsyms(&ksfp)<0) {
      fprintf(stderr, "ERROR: failed to open kallsyms\n");
      EEXIT();
    }
    prcKallsyms(ksfp, &kernpath);

    if (kotblf) {
	size_t n = strlen(kernpath);
	assert(1==fwrite(kernpath,n,1,kotblf));
	assert(1==fwrite("\0",1,1,kotblf));
	n = strlen(KALLSYMSPATH);
	assert(1==fwrite(KALLSYMSPATH,n,1,kotblf));
	assert(1==fwrite("\0",1,1,kotblf));
	assert(1==fwrite("\0",1,1,kotblf));
    }
    fclose(ksfp);
  }

  if (kotblf) fclose(kotblf);

  /* KFLAGS + buildexe path: embed the newly-written kotbl.bin into the
     executable as .kotbl section (replaces the separate kldinst step). */
  if (GBLS.buildexe && !GBLS.executable && !no_kflags) {
    char *kd  = NULL;
    size_t ksz = 0;
    if (read_file_buf(GBLS.kotblfile, &kd, &ksz) >= 0 && ksz > 0) {
      kld_add_elf_section(GBLS.buildexe, KOTBL_SEC, kd, ksz);
      free(kd);
      maybe_install_kldso_interp(GBLS.buildexe);
    }
  }

  // optionally expose objects via synthetic filesystem
  // This support has not been completed.
  if (GBLS.startfs) {
    if (!fsCreate(&(GBLS.fs), argv[0], kldfsCreate)) EEXIT();
    if (!kldfsLoop(&GBLS.fs, &GBLS.sigproc)) EEXIT();
  }
  
  return EXIT_SUCCESS;
}
