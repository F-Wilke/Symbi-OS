#ifndef __KLD_ELF_H__
#define __KLD_ELF_H__

typedef enum { TYPE_FUNC, TYPE_DATA,
	       TYPE_RODATA,
	       TYPE_BSS, TYPE_ABS } SymType;

typedef enum { BIND_GLOBAL,
	       BIND_LOCAL,
	       BIND_WEAK } SymBind;

typedef struct {
    char    *name;
    uint64_t addr;
    uint64_t size;
    SymBind  bind;
    SymType  type;
} SymbolEntry;

typedef struct {
  const uint8_t *data;
  size_t size;
  void *elf;              // Elf* pointer (kept open until free)
  int fd;                 // File descriptor (kept open until free)
} SectionData;

extern void *elf_generate_elf_mmap(const SymbolEntry *entries, int count,
				   size_t strlen,
				   size_t *out_size, int *out_fd);
extern int   elf_read_syms(const char *path, int fd, SymbolEntry **entries,
			   size_t *n, size_t *nmstrlen);
extern int   elf_open_secdata(SectionData *sd, const char *path,
			      int fd, const char *scnm);
extern void  elf_close_secdata(SectionData *sd);

#endif
