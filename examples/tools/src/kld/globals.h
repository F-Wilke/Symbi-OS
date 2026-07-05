#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


#define KOTBL_SEC        ".kotbl"
#define KOTBL_DFLT       "kotbl.bin"
#define KOTBL_INTERP_SEC ".interp_orig"
#define PATHPREFIX_DFLT  (NULL)
#define LIBKERNPATH_DFLT "libkern.so"
#ifndef KLDSO_PATH_DFLT
#define KLDSO_PATH_DFLT  /lib64/kld.so
#endif
#define SOPERM_DFLT      (0644)
#define KALLSYMSPATH     "/proc/kallsyms"
enum {
  KLDOPT_NONE    = 0,
  KLDOPT_SHARED  = 1<<0,
  KLDOPT_PERPROC = 1<<1,
  KLDOPT_RELOAD  = 1<<2
};
#define KLDOPTS_DFLT     (KLDOPT_RELOAD)
typedef uint64_t kldopts_t;

// Binary Object
//   kld works with binary objects that are both represented by
//   a kernel object (ko) and a user shared object (so)
//    1) Constructing both as needed
//    2) At runtime loading ko's and updating so's to reflect load addresses
typedef struct {
  UT_hash_handle hhpath;     // hash handle for ko path lookup        
  UT_hash_handle hhmod;      // hash handle for mod name lookup
  char          *kofnm;      // kernel object (ko) full canonical path
  char          *sofnm;      // share object (so) full cononical path
  char          *modnm;      // module name
  char          *komodnm;    // compiled-in name from .moodinfo (NULL
                             // if not forced or not yet read)
  char          *kldoptstr;  // kld options string  (if overridden
                             // from those specified in modinfo)
  kldopts_t      kldopts;    // parsed kld options
  int            kofd;       // fd of kofnm once opened 
  int            sofd;       // fd of sofnm once opened
  int            forcemodnm; // modnm is forced recored in kotbl
  int            loaded;     // 1 = loaded
} bo_t; 
  
typedef struct {
  char      cwd[PATH_MAX];  // pwd 
  bo_t *    bosbypath;      // hash table of binary objects (by path)
  bo_t *    bosbymod;       // hash table of binary objects (by modnm)
  char *    executable;     // path of executable if one was specified (runtime)
  char *    buildexe;       // path of executable to patch at build time (-o flag)
  char *    kotblfile;      // path of kernel object table file to create
  char **   dirs;           // directory search array 
  char *    libkernpath;    // name for lib kernel so
  char *    kallsymspath;   // path used to overide where to read kernel symbols from
  int       dirc;           // count of ko entries in bo directory search array
  int       dirmax;         // maximum size of bo directory seaarch array 
  int       verbose;        // verbosity level
  int       prockallsyms;   // boolean create so for kallsyms
  int       procbos;        // boolean create so's for all named bo's
  int       resetinterp;    // indicates we are to rest the interpret
  int       noexec;         // -N: print exec info instead of execve'ing
} globals_t;
extern globals_t GBLS;

#endif
