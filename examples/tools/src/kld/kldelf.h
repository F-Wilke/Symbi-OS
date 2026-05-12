#ifndef __KLD_ELF_H__
#define __KLD_ELF_H__

// Our own wrapper for describing symbols
//  Includes name which is a pointer to string rather than
//  an index.
//  For the rest of the information describing a symbol we
//  use the GElf_Sym struct of libelf.
// The following is from libelf/elf.h and libelf/gelf.h
// typedef Elf64_Sym GElf_Sym;
//
// typedef struct
// {
//  Elf64_Word	st_name;		/* Symbol name (string tbl index) */
//  unsigned char	st_info;	/* Symbol type and binding */
//  unsigned char st_other;		/* Symbol visibility */
//  Elf64_Section	st_shndx;	/* Section index */
//  Elf64_Addr	st_value;		/* Symbol value */
//  Elf64_Xword	st_size;		/* Symbol size */
//} Elf64_Sym;
// This allows us to seed the values directly values read from an existing
// elf file or translate nm output to libelf values with out indirection
typedef struct {
  char    *name;
  GElf_Sym sym;    
} kld_sym;

static __attribute__((unused)) int
kld_sym_init_from_kallsyms(kld_sym *this, uintptr_t addr, char *name, 
			   char type)
{
  int   rc  = 0;
  GElf_Sym *sym = &(this->sym);
  unsigned int bind, stype;

  switch (type) {
  case 'A': bind = STB_GLOBAL; stype = STT_NOTYPE; break;
  case 'B': bind = STB_GLOBAL; stype = STT_OBJECT; break;
  case 'C': bind = STB_GLOBAL; stype = STT_NOTYPE; break;
  case 'D': bind = STB_GLOBAL; stype = STT_OBJECT; break;
  case 'I': assert(0); rc = -1; break;
  case 'N': bind = STB_GLOBAL; stype = STT_NOTYPE; break;
  case 'R': bind = STB_GLOBAL; stype = STT_OBJECT; break;
  case 'T': bind = STB_GLOBAL; stype = STT_FUNC;   break;
  case 'U': assert(0); rc = -1; break;
  case 'V': bind = STB_WEAK;   stype = STT_NOTYPE; break;
  case 'W': bind = STB_WEAK;   stype = STT_NOTYPE; break;
#if 0
  case 'a': bind = STB_LOCAL;  stype = STT_NOTYPE; break;
  case 'b': bind = STB_LOCAL;  stype = STT_OBJECT; break;
  case 'c': bind = STB_LOCAL;  stype = STT_NOTYPE; break;
  case 'd': bind = STB_LOCAL;  stype = STT_OBJECT; break;
  case 'i': assert(0); rc = -1; break;
  case 'n': bind = STB_LOCAL;  stype = STT_NOTYPE; break;
  case 'r': bind = STB_LOCAL;  stype = STT_OBJECT; break;
  case 't': bind = STB_LOCAL;  stype = STT_FUNC;   break;
  case 'u': assert(0); rc = -1; break;
  case 'v': bind = STB_WEAK;   stype = STT_NOTYPE; break;
  case 'w': bind = STB_WEAK;   stype = STT_NOTYPE; break;
#else
    // for the moment strip local symbols as they
    // do not belong in .dynsyms
  case 'a': 
  case 'b': 
  case 'c': 
  case 'd': 
  case 'i': 
  case 'n': 
  case 'r': 
  case 't': 
  case 'u': 
  case 'v': 
  case 'w':
    rc = -1; break;
#endif    
  default:
    fprintf(stderr, "Unsupported type: %c\n", type);
    assert(0);
    rc = -1;
  }
  
  if (rc>=0) {
    this->name    = strdup(name); // name string to be placed in string table
    sym->st_name  = -1;           // no string table index yet
    sym->st_info  = GELF_ST_INFO(bind, stype); // encode binding any type
    sym->st_shndx = SHN_ABS;      // we encode all symbols in the abs section
    sym->st_value = addr;         // value of symbol
    sym->st_size  = 0;     
  } else {
    this->name = NULL;
  }

  return rc;
}

static __attribute__((unused)) int
kld_sym_init_from_sym(kld_sym *this, const GElf_Sym *isym, Elf *elf,
		      size_t *nmstrlen, const GElf_Shdr *shdr)
{
  int       rc  = 0;
  GElf_Sym *sym = &(this->sym);

  // filter out undefined symbols ... add more conditions as needed
  if (isym->st_shndx == SHN_UNDEF) {
    rc = -1;
    goto done;
  }
  if (GELF_ST_BIND(isym->st_info) == STB_LOCAL) {
    rc = -1;
    goto done;
  }
  // copy fields from input symbol
  *sym           = *isym;
   sym->st_shndx = SHN_ABS;  // put all symbols in absolute section
  
  // copy name name  
  char *name = elf_strptr(elf, shdr->sh_link, sym->st_name);
  this->name    = strdup(name);
  *nmstrlen += strlen(this->name)+1; // +1 for null
 done:
  return rc;
}

typedef struct {
  const uint8_t *data;
  size_t size;
  void *elf;              // Elf* pointer (kept open until free)
  int fd;                 // File descriptor (kept open until free)
} kld_secdata;

extern void *kld_generate_elf_mmap(const kld_sym *entries, int count,
				   size_t strlen,
				   size_t *out_size, int *out_fd);
extern int   kld_read_elf_syms(const char *path, int fd, kld_sym **entries,
			   size_t *n, size_t *nmstrlen);
extern int   kld_open_elf_secdata(kld_secdata *sd, const char *path,
			      int fd, const char *scnm);
extern void  kld_close_elf_secdata(kld_secdata *sd);

#endif
