// THE BASE VERSION OF THIS CODE WAS PRODUCED WITH Google GEMINI 3 FLASH
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <libelf.h>
#include <gelf.h>

#include "kldelf.h"

typedef struct { char *buf; size_t top; } StrTab;

static size_t
add_str(StrTab *st, const char *s)
{
    size_t off = st->top;
    strcpy(st->buf + off, s);
    st->top += strlen(s) + 1;
    return off;
}

// Returns a memory-mapped pointer to the ELF data
extern void*
kld_generate_elf_mmap(const kld_sym *entries, int count,
		      size_t nmstrlen,   
		      size_t *out_size, int *out_fd)
{
  void *map = NULL; 
  int mfd = memfd_create("elf_mmap", MFD_CLOEXEC);
  if (mfd < 0) { perror("memfd_create"); return NULL; }
  
  elf_version(EV_CURRENT);
  Elf *elf = elf_begin(mfd, ELF_C_WRITE, NULL);
  if (elf == NULL) {
    close(mfd);
    fprintf(stderr, "ERROR: elf_begin failed\n");
    return map;
  } 
  gelf_newehdr(elf, ELFCLASS64);

    // [ ... String table and Section logic remains the same ... ]
    const char *sec_names[] = {
      ".shstrtab", ".strtab", ".symtab", ".dynstr", ".dynsym", ".hash", ".dynamic"
    };
    int numsec = sizeof(sec_names)/sizeof(sec_names[0]);
    size_t schdrlen = 0;
    for (int i=0; i<numsec; i++) {
      schdrlen += strlen(sec_names[i]) + 1; // +1 for null 
    }
    schdrlen++; // add for initial manditory null byte at start of a string table
    StrTab sh_st = { calloc(1, schdrlen), 0 };
    
    add_str(&sh_st, "");  // set index 0 to '\0'
    // need section specific lengths so we add by hand and not in a loop
    // be careful this code is brittle and needs indexs and names to match
    size_t n_shstr   = add_str(&sh_st, sec_names[0]); 
    size_t n_strtab  = add_str(&sh_st, sec_names[1]); 
    size_t n_symtab  = add_str(&sh_st, sec_names[2]);
    size_t n_dynstr  = add_str(&sh_st, sec_names[3]);
    size_t n_dynsym  = add_str(&sh_st, sec_names[4]);
    size_t n_hash    = add_str(&sh_st, sec_names[5]);
    size_t n_dynamic = add_str(&sh_st, sec_names[6]);

    nmstrlen++; // add for initial manditory null byte at start of a string table
    StrTab sym_st = { calloc(1, nmstrlen), 0 };
    add_str(&sym_st, ""); // set index 0 to '\0'
    size_t *sym_offsets = malloc(sizeof(size_t)*count);
    for(int i=0; i<count; i++) sym_offsets[i] = add_str(&sym_st, entries[i].name);

    GElf_Shdr sh;
    Elf_Scn *s_shstr = elf_newscn(elf);
    Elf_Data *d_shstr = elf_newdata(s_shstr);
    d_shstr->d_buf = sh_st.buf; d_shstr->d_size = sh_st.top; d_shstr->d_align = 1;
    gelf_getshdr(s_shstr, &sh); sh.sh_type = SHT_STRTAB; sh.sh_name = n_shstr;
    gelf_update_shdr(s_shstr, &sh);

    Elf_Scn *s_strtab = elf_newscn(elf);
    Elf_Data *d_strtab = elf_newdata(s_strtab);
    d_strtab->d_buf = sym_st.buf; d_strtab->d_size = sym_st.top; d_strtab->d_align = 1;
    gelf_getshdr(s_strtab, &sh); sh.sh_type = SHT_STRTAB; sh.sh_name = n_strtab;
    gelf_update_shdr(s_strtab, &sh);

    Elf_Scn *s_dynstr = elf_newscn(elf);
    Elf_Data *d_dynstr = elf_newdata(s_dynstr);
    d_dynstr->d_buf = sym_st.buf; d_dynstr->d_size = sym_st.top; d_dynstr->d_align = 1;
    gelf_getshdr(s_dynstr, &sh); sh.sh_type = SHT_STRTAB; sh.sh_name = n_dynstr;
    sh.sh_flags = SHF_ALLOC;
    gelf_update_shdr(s_dynstr, &sh);

    GElf_Sym *sym_table = calloc(count + 1, sizeof(GElf_Sym));
    for(int i=0; i<count; i++) {
      sym_table[i+1]         = entries[i].sym;
      sym_table[i+1].st_name = sym_offsets[i];
    }

    Elf_Scn *s_symtab = elf_newscn(elf);
    Elf_Data *d_symtab = elf_newdata(s_symtab);
    d_symtab->d_buf = sym_table; d_symtab->d_size = (count+1)*sizeof(GElf_Sym);
    d_symtab->d_type = ELF_T_SYM; d_symtab->d_align = 8;
    gelf_getshdr(s_symtab, &sh); sh.sh_type = SHT_SYMTAB; sh.sh_name = n_symtab;
    sh.sh_link = elf_ndxscn(s_strtab); sh.sh_info = 1; sh.sh_entsize = sizeof(GElf_Sym);
    gelf_update_shdr(s_symtab, &sh);

    Elf_Scn *s_dynsym = elf_newscn(elf);
    Elf_Data *d_dynsym = elf_newdata(s_dynsym);
    d_dynsym->d_buf = sym_table; d_dynsym->d_size = (count+1)*sizeof(GElf_Sym);
    d_dynsym->d_type = ELF_T_SYM; d_dynsym->d_align = 8;
    gelf_getshdr(s_dynsym, &sh); sh.sh_type = SHT_DYNSYM; sh.sh_name = n_dynsym;
    sh.sh_flags = SHF_ALLOC; sh.sh_link = elf_ndxscn(s_dynstr); sh.sh_info = 1;
    sh.sh_entsize = sizeof(GElf_Sym);
    gelf_update_shdr(s_dynsym, &sh);

    Elf_Scn *s_hash = elf_newscn(elf);
    uint32_t *h_buf = calloc(count+3, 4);
    h_buf[0]=1; h_buf[1]=count+1; for(int i=0; i<count; i++) h_buf[3+i]=i+1;
    Elf_Data *d_hash = elf_newdata(s_hash); d_hash->d_buf = h_buf;
    d_hash->d_size = (count+3)*4; d_hash->d_align = 8;
    gelf_getshdr(s_hash, &sh); sh.sh_type = SHT_HASH; sh.sh_name = n_hash;
    sh.sh_flags = SHF_ALLOC; sh.sh_link = elf_ndxscn(s_dynsym);
    gelf_update_shdr(s_hash, &sh);

    Elf_Scn *s_dyn = elf_newscn(elf);
    GElf_Dyn *dyn_data = calloc(7, sizeof(GElf_Dyn));
    Elf_Data *d_dyn = elf_newdata(s_dyn); d_dyn->d_buf = dyn_data;
    d_dyn->d_size = 7*sizeof(GElf_Dyn); d_dyn->d_align = 8;
    gelf_getshdr(s_dyn, &sh); sh.sh_type = SHT_DYNAMIC; sh.sh_name = n_dynamic;
    sh.sh_flags = SHF_ALLOC;
    gelf_update_shdr(s_dyn, &sh);

    gelf_newphdr(elf, 2);
    elf_update(elf, ELF_C_NULL);

    GElf_Ehdr ehdr; gelf_getehdr(elf, &ehdr);
    ehdr.e_type = ET_DYN; ehdr.e_machine = EM_X86_64;
    ehdr.e_shstrndx = elf_ndxscn(s_shstr);
    gelf_update_ehdr(elf, &ehdr);

    GElf_Shdr t_sh;
    gelf_getshdr(s_dynstr, &t_sh); dyn_data[0].d_tag = DT_STRTAB;
    dyn_data[0].d_un.d_ptr = t_sh.sh_offset;
    gelf_getshdr(s_dynsym, &t_sh); dyn_data[1].d_tag = DT_SYMTAB;
    dyn_data[1].d_un.d_ptr = t_sh.sh_offset;
    dyn_data[2].d_tag = DT_STRSZ;  dyn_data[2].d_un.d_val = sym_st.top;
    dyn_data[3].d_tag = DT_SYMENT; dyn_data[3].d_un.d_val = sizeof(GElf_Sym);
    gelf_getshdr(s_hash, &t_sh);   dyn_data[4].d_tag = DT_HASH;
    dyn_data[4].d_un.d_ptr = t_sh.sh_offset;
    dyn_data[5].d_tag = DT_NULL;

    GElf_Phdr phdr;
    gelf_getphdr(elf, 0, &phdr);
    phdr.p_type = PT_LOAD; phdr.p_vaddr = 0;
    phdr.p_filesz = phdr.p_memsz = 0x1000;
    phdr.p_flags = PF_R; phdr.p_align = 0x1000;
    gelf_update_phdr(elf, 0, &phdr);

    gelf_getphdr(elf, 1, &phdr);
    gelf_getshdr(s_dyn, &t_sh);
    phdr.p_type = PT_DYNAMIC; phdr.p_offset = phdr.p_vaddr = t_sh.sh_offset;
    phdr.p_filesz = phdr.p_memsz = d_dyn->d_size; phdr.p_flags = PF_R;
    gelf_update_phdr(elf, 1, &phdr);

    gelf_getshdr(s_dynsym, &t_sh); t_sh.sh_info = 1;
    gelf_update_shdr(s_dynsym, &t_sh);
    gelf_getshdr(s_symtab, &t_sh); t_sh.sh_info = 1;
    gelf_update_shdr(s_symtab, &t_sh);

    // Write to memory FD
    elf_update(elf, ELF_C_WRITE);
    elf_end(elf);

    // 2. Mmap the file descriptor instead of reading
    off_t size = lseek(mfd, 0, SEEK_END);
    *out_size = (size_t)size;
    map = mmap(NULL, *out_size, PROT_READ, MAP_PRIVATE, mfd, 0);
    
    // We return the FD so the caller can close it after munmap, 
    // or we can close it here (the mapping remains valid).
    *out_fd = mfd;

    // Local Cleanup
    free(sym_offsets);
    free(sh_st.buf); free(sym_st.buf); free(sym_table);
    free(h_buf); free(dyn_data);

    return map;
}

extern int
kld_read_elf_syms(const char *path, int fd, kld_sym **entries, size_t *n,
	      size_t *nmstrlen)
{
  Elf         *elf  = NULL;
  Elf_Scn     *scn  = NULL;
  GElf_Shdr   shdr;
  Elf_Data    *data;
  kld_sym     *ents = NULL;
  int          rc   = 0;
  int          emax = 0;
  int          ei   = 0;
  
  elf_version(EV_CURRENT);
  elf = elf_begin(fd, ELF_C_READ, NULL);
  if (elf==NULL) {
    fprintf(stderr, "ERROR: elf_begin: failed\n");
    rc = -1;
    goto done;
  }
  if (elf_kind(elf) != ELF_K_ELF) {
    fprintf(stderr, "ERROR: %s: Not a valid ELF file.\n", path);
    rc = -1;
    goto done;
  }

  assert(*entries == NULL);
  *nmstrlen = 0;
  while ((scn = elf_nextscn(elf, scn)) != NULL) {
    gelf_getshdr(scn, &shdr);
    if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
      data = elf_getdata(scn, NULL);
      int count = shdr.sh_size / shdr.sh_entsize;
      
      for (int i = 0; i < count; i++) {
	GElf_Sym sym;
	gelf_getsym(data, i, &sym);
	
	// Skip empty symbols
	if (sym.st_name == 0 && sym.st_value == 0) continue;

	if (ei == emax) {
	  emax = (emax > 0) ? emax << 1 : 1024;
	  ents = realloc(ents, emax * sizeof(kld_sym));
	}
	
	kld_sym *e = &ents[ei];
	if (kld_sym_init_from_sym(e, &sym, elf, nmstrlen, &shdr)>=0) {
	    ei++;
	}
      }
    }
  }
 done:
  if (elf) elf_end(elf);
  *entries = ents;
  *n = ei;
  return rc;
}

extern int
kld_open_elf_secdata(kld_secdata *sd, const char *path, int fd, const char *scnm)
{
  GElf_Shdr shdr;
  size_t shstrndx;
  Elf *elf       = NULL;
  Elf_Scn *scn   = NULL;
  Elf_Data *data = NULL;
  int rc         = 0;
  int sfd        = fd;

  
  if (!path || !scnm || !sd) {
    return -1;
  }

  // Initialize to zero
  sd->data = NULL;
  sd->size = 0;
  sd->elf = NULL;
  sd->fd = -1;
  
  // Initialize libelf
  if (elf_version(EV_CURRENT) == EV_NONE) {
    fprintf(stderr, "ERROR: libelf initialization failed\n");
    return -1;
  }

  if (fd == -1) {
    // Open the ELF file
    sfd = open(path, O_RDONLY);
    if (sfd < 0) {
      fprintf(stderr, "ERROR: Cannot open ELF file '%s': %s\n", 
	      path, strerror(errno));
      return -1;
    }
  }

    // Begin ELF reading
    elf = elf_begin(sfd, ELF_C_READ, NULL);
    if (elf == NULL) {
      fprintf(stderr, "ERROR: elf_begin failed for '%s': %s\n",
	      path, elf_errmsg(-1));
      if (fd==-1) close(sfd);
      return -1;
    }

    // Get the index of the section name string table (.shstrtab)
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
      fprintf(stderr, "elf_getshdrstrndx() failed: %s", elf_errmsg(-1));
      rc = -3;
      goto cleanup;
    }
    
    // Find the section
    scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
      if (gelf_getshdr(scn, &shdr) == NULL) {
	fprintf(stderr, "ERROR: gelf_getshdr failed: %s\n", elf_errmsg(-1));
	rc = -3;
	goto cleanup;
      }
      // Get section name
      const char *sec_name = elf_strptr(elf, shstrndx, shdr.sh_name);
      if (sec_name && strcmp(sec_name, scnm) == 0) {
	// Found the section
	data = elf_getdata(scn, NULL);
	if (data == NULL) {
	  fprintf(stderr, "ERROR: elf_getdata failed: %s\n", elf_errmsg(-1));
	  rc = -3;
	  goto cleanup;
	}
	sd->data = (const uint8_t *)data->d_buf;
	sd->size = data->d_size;
	sd->elf  = elf;
	sd->fd   = sfd;
	return 0;
      }
    }
    //  section not found
    fprintf(stderr, "ERROR:  %s section not found in '%s'\n", scnm, path);
    rc = -2;
    
cleanup:
    if (elf) {
        elf_end(elf);
    }
    if (fd < 0 && sfd >=0 ) {
      close(sfd);
    }
    return rc;
}

extern void
kld_close_elf_secdata(kld_secdata *sd)
{
  if (!sd) return;
  if (sd->elf) {
    elf_end((Elf *)sd->elf);
    sd->elf = NULL;
  }
  if (sd->fd >= 0) {
    close(sd->fd);
    sd->fd = -1;
  }
  sd->data = NULL;
  sd->size = 0;
}

#ifdef MAIN

int main() {
  kld_sym entries[7];
  kld_sym *e;
  int i=0;
  // 1. Global Function (Standard 'T' type, but 'A' because it's Absolute)
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00001000, "global_func", 'T')>=0);
  
  // 2. Weak Function (Appears as 'W' in nm)
  // If the app defines its own 'weak_func', the app's version wins.
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00002000, "weak_func", 'W')>=0); 
  
  // 3. Global Initialized Data (Standard 'D' type, but 'A' because it's Absolute)
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00003000, "global_data", 'D')>=0); 
  
  // 4. Global Read-Only Data (Standard 'R' type)
  // Note: For Absolute symbols, the 'RO' is enforced by your external memory protection
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00004000, "global_rodata", 'R')>=0); 
  
  // 5. Global BSS / Uninitialized Data (Standard 'B' type)
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00005000, "global_bss", 'B')>=0); 
  
  // 6. Weak Data Symbol (Appears as 'V' or 'W' in nm)
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00006000, "weak_data", 'W')>=0); 
    
  // 7. A Pure Absolute Constant (No type, just a value)
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0xDEADBEEF, "absolute_val", 'A')>=0); 

  assert(i==(sizeof(entries)/sizeof(entries[0])));
  
  size_t elf_size;
  int mfd;
  size_t nmstrlen = 0;
  void *elf_ptr;
  int n = sizeof(entries)/sizeof(entries[0]);
  for (int i = 0; i<n; i++) {
    nmstrlen += strlen(entries[i].name) + 1; // +1 for null byte
  }
  elf_ptr = kld_generate_elf_mmap(entries, n, nmstrlen, &elf_size, &mfd);
  
  if (elf_ptr != MAP_FAILED) {
    printf("ELF mapped at %p (Size: %zu)\n", elf_ptr, elf_size);
    
    // Example: Write the mapped buffer to a file
    int dfd = open("libstubs.so", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(dfd, elf_ptr, elf_size);
    close(dfd);
    
    // Cleanup mapping
    munmap(elf_ptr, elf_size);
    close(mfd);
  }
  
  return 0;
}

#endif
