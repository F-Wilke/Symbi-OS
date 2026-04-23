#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#define KOTBL_DFLT      "kotbl.bin"
#define FSNAME_DFLT     "libk"
#define PATHPREFIX_DFLT "libk"
#define KLDOPTS_DFLT    "EXCLUSIVE"
#define SOPERM_DFLT      (0644)

// Binary Object
//   kld works with binary objects that are both represented by
//   a kernel object (ko) and a user shared object (so)
//    1) Constructing both as needed
//    2) At runtime loading ko's and updating so's to reflect load addresses
typedef struct {
  UT_hash_handle hh;
  char          *kofnm;      // kernel object (ko) full path canonical file name
  char          *sofnm;      // share object (so) full path cononical file name 
  char          *modnm;      // module name
  char          *kldopts;    // kld options
  int            kofd;       // fd of kofnm once opened 
  int            sofd;       // fd of sofnm once opened
} bo_t; 
  
typedef struct {
  char      cwd[PATH_MAX];  // pwd 
  fs_t      fs;             // synthetic file system object
  sigproc_t sigproc;        // signal procesing object
  bo_t *    bos;            // hash table of binary objects
  char *    executable;     // path of executable if one was specified
  char *    botblfile;      // path of binay object table file to create
  char **   dirs;           // directory search array 
  char *    fsname;         // default name for file system mount point dirname
  char *    pathprefix;     // path prefix for default outputs eg. libkern.so
  pid_t     pid;            // pid of this process (useful for fs interface)
  int       dirc;           // count of ko entries in bo directory search array
  int       dirmax;         // maximum size of bo directory seaarch array 
  int       verbose;        // verbosity level
  int       startfs;        // boolean start fs interface 
  int       prockallsyms;   // boolean create so for kallsyms
  int       procbos;        // boolean create so's for all named bo's
} globals_t;

extern globals_t GBLS;

#endif
