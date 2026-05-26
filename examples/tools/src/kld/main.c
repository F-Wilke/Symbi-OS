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
	  "%s [-h] [-v] [-O lib] [-K dir] "
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
	  "   elf    : optional elf file to process as follows:\n"
	  "              For each libk found in the elf:\n"
	  "                 find coresponding ko and load it\n"
	  "                 update the libk symbol table with runtime"
	  " addresses\n"
	  "              For each ko found in the elf:\n"
	  "                 extract ko and load\n"
	  "                 update the libk symbol table with runtime"
	  " addresses\n",
	  basename(name), KALLSYMSPATH, LIBKERNPATH_DFLT, KALLSYMSPATH);
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
openso(char *kofnm, char *modnm, char **sofullpath)
{
  int fd;
  char *tmp=NULL;
  char *fullpath=malloc(PATH_MAX); // avoid large stack alloc
  assert(fullpath);
  
  if (kofnm && modnm) {
    tmp = strdup(kofnm);
    char *dir=dirname(tmp);
    snprintf(fullpath, PATH_MAX, "%s/lib%s.so", dir, modnm);
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
addbo(char *kofnm,char *kldopts,char *modnm, bo_t **thebo)
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
  
  sofd = openso(kofullpath, modnm, &sofullpath);
  if (sofd < 0) { rc = -1; goto done; }
  
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
  
  bo = newbo(kofullpath, modnm, komodnm, (origmodnm!=NULL), opts,
	     kldopts, kofd, sofullpath, sofd);
  // at this point bo contains all state of memory and fd's
  // cleanup when bo deleted 
  HASH_ADD_KEYPTR(hhpath, GBLS.bosbypath, bo->kofnm,
		  strlen(bo->kofnm), bo);
  HASH_ADD_KEYPTR(hhmod, GBLS.bosbymod, bo->modnm,
		  strlen(bo->modnm), bo);
  
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
  
  while ((opt = getopt(argc, argv, "K:k:O:o:hv")) != -1) {
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
    case 'v':
      GBLS.verbose++;
      break;
    default:
      usage(argv[0],stderr);
      rc = -1;
      goto done;
    }
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
    if (addbo(kofnm, kldopts, modnm, NULL)<0) {
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
      anum < 1) {
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

static void
writeso(const char *path, int fd, const kld_sym *entries, size_t n,
	 size_t nmstrlen)
{
  int mfd;
  int dfd=fd;
  size_t elf_size;
  int buildtime = (GBLS.executable == NULL);
  const char *soname = strrchr(path, '/');
  soname = soname ? soname + 1 : path;
  void *elf_ptr = kld_generate_elf_mmap(entries, n, nmstrlen, soname, buildtime, &elf_size, &mfd);
  
  if (elf_ptr != MAP_FAILED) {
    VPRINT("%s: ELF mapped at %p (Size: %zu)\n", path, elf_ptr, elf_size);
    
    // Example: Write the mapped buffer to a file
    if (fd==-1) {
      dfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (dfd == -1) {
	perror(path);
	goto done;
      }
    } 
    ssize_t nw = write(dfd, elf_ptr, elf_size);
    assert(nw>0 && (size_t)nw==elf_size);
    if (fd == -1) close(dfd);

 done:
    // Cleanup mapping
    munmap(elf_ptr, elf_size);
    close(mfd);
  }
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
    writeso(GBLS.libkernpath, -1, kentries, ksyms_i, knmstrlen);
    if (GBLS.buildexe)
      kld_undef_elf_dynsym(GBLS.buildexe, kentries, ksyms_i);
    *kernpath = realpath(GBLS.libkernpath, NULL);
    cleanupEntries(kentries, ksyms_i);
  }
  
  {
    struct Module *mod, *tmp;
    HASH_ITER(hh, mhash, mod, tmp) {
      writeso(mod->bo->sofnm, mod->bo->sofd, mod->entries,
	      mod->syms_i, mod->nmstrlen);
      if (GBLS.buildexe)
        kld_undef_elf_dynsym(GBLS.buildexe, mod->entries, mod->syms_i);
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
  // not dynamic memory
  // if (GBLS.libkernpath) free(GBLS.libkernpath);
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
  writeso(bo->sofnm, bo->sofd, entries, n, nmstrlen);
  if (GBLS.buildexe)
    kld_undef_elf_dynsym(GBLS.buildexe, entries, n);
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

static int
prcExec(const char *exec)
{
  kld_secdata sd;
  int rc = 0;
  bo_t *bo=NULL, **bos=NULL;
  int bosi=0, bosn=0;
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
      if (modnm != NULL && strcmp(modnm, KALLSYMSPATH)==0) {
	GBLS.libkernpath = fnm;
      } else {
	if (addbo(fnm, opts, modnm, &bo) <0) {
	  assert(0);
	  rc = -1;
	  goto done;
	} else {
	  if (bosi==bosn) {
	    bosn = (bosn==0) ? 16 : bosn<<1;
	    bos  = realloc(bos, sizeof(bos[0])*bosn);
	  }
	  bos[bosi] = bo;
	  bosi++;
	}
      }
      i++;
    }
    kld_close_elf_secdata(&sd,-1);
    if (i) GBLS.prockallsyms = 1;
  }

  
  for (int i=0; i<bosi; i++) {
    if (loadBO(bos[i])<0) {
      rc = -1;
    }
  }
  if (bos) free(bos);
 done:
  return rc;
}

int
main(int argc, char **argv)
{
  FILE *ksfp   = NULL;
  FILE *kotblf = NULL;
  
  if (GBLSInit(argc, argv)<0) {
    EEXIT();
  }

  atexit(cleanup);  // from this point on exits will trigger cleanups
  
  if (GBLS.executable) {
    prcExec(GBLS.executable);
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
  // optionally expose objects via synthetic filesystem
  // This support has not been completed.
  if (GBLS.startfs) {
    if (!fsCreate(&(GBLS.fs), argv[0], kldfsCreate)) EEXIT();
    if (!kldfsLoop(&GBLS.fs, &GBLS.sigproc)) EEXIT();
  }
  
  return EXIT_SUCCESS;
}
