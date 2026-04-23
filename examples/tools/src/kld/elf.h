#ifndef __ELF_H__
#define __ELF_H__

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

extern void *elf_generate_elf_mmap(const SymbolEntry *entries, int count,
				   size_t strlen,
				   size_t *out_size, int *out_fd);
extern int   elf_read_syms(const char *path, int fd, SymbolEntry **entries,
			   size_t *n, size_t *nmstrlen);

#endif
