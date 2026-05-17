#ifndef __MODINFO_H__
#define __MODINFO_H__

// From Claude Sonnet 4.6

// Parsed contents of a kernel module's .modinfo section relevant to kld.
typedef struct {
  char  *name;       // module name from "name=..." entry, NULL if absent
  char **kld_vals;   // array of values from all "kld=..." entries
  int    kld_count;  // number of kld= entries
} kld_modinfo;

// Read the .modinfo section from the ELF file at 'path'.
// Pass fd=-1 to have the function open the file itself, or a valid open
// file descriptor to reuse an already-open fd (the fd is NOT closed by
// this function; kld_open_elf_secdata semantics apply).
// Returns 0 on success, <0 on error.
extern int  kld_read_modinfo(const char *path, int fd, kld_modinfo *mi);

// Free all memory owned by a kld_modinfo populated by kld_read_modinfo.
// Safe to call on a zero-initialised struct.
extern void kld_free_modinfo(kld_modinfo *mi);

#endif
