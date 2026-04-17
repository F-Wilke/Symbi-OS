#ifndef __GLOBALS_H__
#define __GLOBALS_H__

typedef struct {
  char *ko;
  int   dir;
  int   fd;
} ko_t; 
  
typedef struct {
  fs_t      fs;          // file system object
  ko_t *    kos;
  sigproc_t sigproc;     // signal procesing object
  char *    fsname;      // default name for file system mount point dirname
  char *    pathprefix;
  char **   kodirs;
  int       kodirc;
  int       kodirmax;
  int       koc;
  int       komax;
  int       verbose;
  int       startfs;
  int       mkkallsymslibs;
  int       mkkolibs;
  pid_t     pid;
  char      cwd[PATH_MAX];
} globals_t;

extern globals_t GBLS;

#endif
