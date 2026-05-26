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
#include <sys/stat.h>
#include <libelf.h>
#include <gelf.h>

#include "kldelf.h"

typedef struct { char *buf; size_t top; } StrTab;

// GNU hash (djb2 variant) — same algorithm used by GNU ld
static uint32_t
gnu_hash_fn(const char *s)
{
    uint32_t h = 5381;
    for (unsigned char c; (c = (unsigned char)*s) != '\0'; s++)
        h = h * 33 + c;
    return h;
}

// Smallest power of 2 >= n, minimum 1
static uint32_t
next_pow2_u32(uint32_t n)
{
    if (n <= 1) return 1;
    n--;
    n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n + 1;
}

typedef struct { uint32_t h; int orig; } SymBucket;

// qsort_r comparator: sort by bucket (h % nbuckets), stable by original index
static int
cmp_sym_by_bucket(const void *a, const void *b, void *ctx)
{
    uint32_t nb = *(uint32_t *)ctx;
    const SymBucket *sa = (const SymBucket *)a, *sb = (const SymBucket *)b;
    uint32_t ba = sa->h % nb, bb = sb->h % nb;
    if (ba < bb) return -1;
    if (ba > bb) return  1;
    return sa->orig - sb->orig;
}

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
		      size_t nmstrlen, const char *soname,
		      int buildtime,
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
      ".shstrtab", ".strtab", ".symtab", ".dynstr", ".dynsym", ".gnu.hash", ".dynamic"
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
    size_t n_gnuhash = add_str(&sh_st, sec_names[5]);
    size_t n_dynamic = add_str(&sh_st, sec_names[6]);

    // GNU hash parameters.
    // nbuckets: ~4 symbols/bucket (matches libkallsyms.so ratio).
    // bloom_size: power-of-2 count of 64-bit words; count/64 ≈ libkallsyms.so's 2048 for 79K syms.
    uint32_t nbuckets   = (uint32_t)count / 4 + 1;
    uint32_t bloom_size = next_pow2_u32((uint32_t)count / 64);
    if (bloom_size < 1) bloom_size = 1;
    const uint32_t bloom_shift = 6;  // standard for 64-bit bloom words

    // Sort symbols by bucket so same-bucket entries are contiguous (GNU hash requirement).
    SymBucket *sort_arr = malloc(count * sizeof(SymBucket));
    for (int i = 0; i < count; i++) {
        sort_arr[i].h    = gnu_hash_fn(entries[i].name);
        sort_arr[i].orig = i;
    }
    qsort_r(sort_arr, count, sizeof(SymBucket), cmp_sym_by_bucket, &nbuckets);

    nmstrlen++; // add for initial manditory null byte at start of a string table
    size_t soname_extra = (soname && *soname) ? strlen(soname) + 1 : 0;
    StrTab sym_st = { calloc(1, nmstrlen + soname_extra), 0 };
    add_str(&sym_st, ""); // set index 0 to '\0'
    size_t *sym_offsets = malloc(sizeof(size_t)*count);
    for(int i=0; i<count; i++) sym_offsets[i] = add_str(&sym_st, entries[sort_arr[i].orig].name);
    size_t soname_off = (soname && *soname) ? add_str(&sym_st, soname) : 0;

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
      sym_table[i+1]         = entries[sort_arr[i].orig].sym;
      sym_table[i+1].st_name = sym_offsets[i];
      if (buildtime && GELF_ST_BIND(sym_table[i+1].st_info) == STB_GLOBAL)
        sym_table[i+1].st_info = GELF_ST_INFO(STB_WEAK,
                                               GELF_ST_TYPE(sym_table[i+1].st_info));
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

    // GNU hash table layout (all little-endian on x86-64):
    //   uint32_t  header[4]     = { nbuckets, symoffset=1, bloom_size, bloom_shift }
    //   uint64_t  bloom[bloom_size]
    //   uint32_t  buckets[nbuckets]   (dynsym index of first sym in bucket, or 0)
    //   uint32_t  chains[count]       (one per hashed sym: hash&~1 | end_of_chain_bit)
    size_t gnuhash_size = 4*4 + 8*(size_t)bloom_size + 4*(size_t)nbuckets + 4*(size_t)count;
    uint8_t *gh_buf = calloc(1, gnuhash_size);
    uint32_t *gh_hdr   = (uint32_t *)gh_buf;
    uint64_t *gh_bloom = (uint64_t *)(gh_buf + 16);
    uint32_t *gh_bkts  = (uint32_t *)(gh_buf + 16 + 8*(size_t)bloom_size);
    uint32_t *gh_chain = (uint32_t *)(gh_buf + 16 + 8*(size_t)bloom_size + 4*(size_t)nbuckets);

    gh_hdr[0] = nbuckets; gh_hdr[1] = 1; gh_hdr[2] = bloom_size; gh_hdr[3] = bloom_shift;

    for (int i = 0; i < count; i++) {
        uint32_t h      = sort_arr[i].h;
        uint32_t bucket = h % nbuckets;
        // Bloom filter: two bits per symbol in a 64-bit word
        gh_bloom[(h >> 6) & (bloom_size - 1)] |= (1ULL << (h & 63)) |
                                                  (1ULL << ((h >> bloom_shift) & 63));
        // Buckets: record dynsym index (1-based) of first symbol in this bucket
        if (gh_bkts[bucket] == 0) gh_bkts[bucket] = (uint32_t)(i + 1);
        // Chain: store hash (bit 0 clear) and set bit 0 when this is the last in the bucket
        int last_in_bucket = (i + 1 == count) || ((sort_arr[i+1].h % nbuckets) != bucket);
        gh_chain[i] = (h & ~1u) | (last_in_bucket ? 1u : 0u);
    }

    Elf_Scn *s_gnuhash = elf_newscn(elf);
    Elf_Data *d_gnuhash = elf_newdata(s_gnuhash);
    d_gnuhash->d_buf = gh_buf; d_gnuhash->d_size = gnuhash_size;
    d_gnuhash->d_type = ELF_T_BYTE; d_gnuhash->d_align = 8;
    gelf_getshdr(s_gnuhash, &sh); sh.sh_type = SHT_GNU_HASH; sh.sh_name = n_gnuhash;
    sh.sh_flags = SHF_ALLOC; sh.sh_link = elf_ndxscn(s_dynsym);
    gelf_update_shdr(s_gnuhash, &sh);

    Elf_Scn *s_dyn = elf_newscn(elf);
    int ndyn = (soname_off ? 8 : 7) + 1; // +1 zero-pad sentinel after DT_NULL
    GElf_Dyn *dyn_data = calloc(ndyn, sizeof(GElf_Dyn));
    Elf_Data *d_dyn = elf_newdata(s_dyn); d_dyn->d_buf = dyn_data;
    d_dyn->d_size = ndyn*sizeof(GElf_Dyn); d_dyn->d_align = 8;
    gelf_getshdr(s_dyn, &sh); sh.sh_type = SHT_DYNAMIC; sh.sh_name = n_dynamic;
    sh.sh_flags = SHF_ALLOC; sh.sh_link = elf_ndxscn(s_dynstr);
    gelf_update_shdr(s_dyn, &sh);

    gelf_newphdr(elf, 3);
    // ELF_C_NULL computes the final layout without writing; the return value
    // is the total file size.  We need it to set PT_LOAD p_filesz so that
    // ld.so maps the entire file, not just a hardcoded 4096 bytes.
    off_t total_size = elf_update(elf, ELF_C_NULL);

    // Set sh_addr = sh_offset for every ALLOC section.  Since PT_LOAD has
    // p_vaddr=0 and p_offset=0, the virtual address of each mapped section
    // equals its file offset.  libelf leaves sh_addr=0; setting it correctly
    // fixes GDB's ".dynamic section not at expected address" warning.
    { Elf_Scn *s = NULL;
      while ((s = elf_nextscn(elf, s)) != NULL) {
        GElf_Shdr fixsh; gelf_getshdr(s, &fixsh);
        if (fixsh.sh_flags & SHF_ALLOC) {
          fixsh.sh_addr = fixsh.sh_offset;
          gelf_update_shdr(s, &fixsh);
        }
      }
    }

    GElf_Ehdr ehdr; gelf_getehdr(elf, &ehdr);
    ehdr.e_type = ET_DYN; ehdr.e_machine = EM_X86_64;
    ehdr.e_shstrndx = elf_ndxscn(s_shstr);
    gelf_update_ehdr(elf, &ehdr);

    GElf_Shdr t_sh;
    gelf_getshdr(s_dynstr, &t_sh); dyn_data[0].d_tag = DT_STRTAB;
    dyn_data[0].d_un.d_ptr = t_sh.sh_addr;
    gelf_getshdr(s_dynsym, &t_sh); dyn_data[1].d_tag = DT_SYMTAB;
    dyn_data[1].d_un.d_ptr = t_sh.sh_addr;
    dyn_data[2].d_tag = DT_STRSZ;  dyn_data[2].d_un.d_val = sym_st.top;
    dyn_data[3].d_tag = DT_SYMENT; dyn_data[3].d_un.d_val = sizeof(GElf_Sym);
    gelf_getshdr(s_gnuhash, &t_sh); dyn_data[4].d_tag = DT_GNU_HASH;
    dyn_data[4].d_un.d_ptr = t_sh.sh_addr;
    if (soname_off) {
        dyn_data[5].d_tag = DT_SONAME; dyn_data[5].d_un.d_val = soname_off;
        dyn_data[6].d_tag = DT_FLAGS;  dyn_data[6].d_un.d_val = 0;
        dyn_data[7].d_tag = DT_NULL;
    } else {
        dyn_data[5].d_tag = DT_FLAGS;  dyn_data[5].d_un.d_val = 0;
        dyn_data[6].d_tag = DT_NULL;
    }

    GElf_Phdr phdr;
    gelf_getphdr(elf, 0, &phdr);
    phdr.p_type   = PT_LOAD; phdr.p_vaddr = 0; phdr.p_offset = 0;
    phdr.p_filesz = phdr.p_memsz = (GElf_Xword)total_size;
    phdr.p_flags  = PF_R; phdr.p_align = 0x1000;
    gelf_update_phdr(elf, 0, &phdr);

    gelf_getphdr(elf, 1, &phdr);
    gelf_getshdr(s_dyn, &t_sh);
    phdr.p_type = PT_DYNAMIC; phdr.p_offset = t_sh.sh_offset; phdr.p_vaddr = t_sh.sh_addr;
    phdr.p_filesz = phdr.p_memsz = d_dyn->d_size; phdr.p_flags = PF_R;
    gelf_update_phdr(elf, 1, &phdr);

    // PT_GNU_STACK declares the stack non-executable; without it ld.so would
    // attempt to mark the stack executable, which the kernel refuses.
    gelf_getphdr(elf, 2, &phdr);
    phdr.p_type = PT_GNU_STACK; phdr.p_offset = phdr.p_vaddr = phdr.p_paddr = 0;
    phdr.p_filesz = phdr.p_memsz = 0; phdr.p_flags = PF_R|PF_W; phdr.p_align = 0x10;
    gelf_update_phdr(elf, 2, &phdr);

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
    free(sort_arr); free(gh_buf); free(dyn_data);

    return map;
}

// qsort/bsearch comparator: sort kld_sym by name
static int
cmp_sym_by_name(const void *a, const void *b)
{
    return strcmp(((const kld_sym *)a)->name, ((const kld_sym *)b)->name);
}

// Update symbol values and bindings in-place in an existing kld-generated .so.
// For each symbol in .dynsym and .symtab, if the name is found in 'entries',
// st_value is overwritten and STB_WEAK is promoted to STB_GLOBAL.
// Symbols absent from 'entries' are left unchanged (stay WEAK with value 0).
extern int
kld_update_elf_dynsym(const char *path, const kld_sym *entries, size_t n)
{
    int rc = 0;
    int dfd = open(path, O_RDWR);
    if (dfd == -1) { perror(path); return -1; }

    // Sorted copy of entries for O(log n) lookup by name
    kld_sym *sorted = malloc(n * sizeof(kld_sym));
    if (!sorted) { close(dfd); return -1; }
    memcpy(sorted, entries, n * sizeof(kld_sym));
    qsort(sorted, n, sizeof(kld_sym), cmp_sym_by_name);

    elf_version(EV_CURRENT);
    Elf *elf = elf_begin(dfd, ELF_C_RDWR, NULL);
    if (!elf) {
        fprintf(stderr, "kld_update_elf_dynsym: elf_begin: %s\n", elf_errmsg(-1));
        rc = -1; goto done;
    }

    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type != SHT_DYNSYM && shdr.sh_type != SHT_SYMTAB)
            continue;

        Elf_Data *data = elf_getdata(scn, NULL);
        if (!data) continue;

        size_t sym_count = shdr.sh_size / shdr.sh_entsize;
        int updated = 0;
        for (size_t i = 1; i < sym_count; i++) {
            GElf_Sym sym;
            gelf_getsym(data, (int)i, &sym);
            const char *sname = elf_strptr(elf, shdr.sh_link, sym.st_name);
            if (!sname || !*sname) continue;
            kld_sym key = { .name = (char *)sname };
            const kld_sym *hit = bsearch(&key, sorted, n,
                                         sizeof(kld_sym), cmp_sym_by_name);
            if (!hit) continue;
            sym.st_value = hit->sym.st_value;
            if (GELF_ST_BIND(sym.st_info) == STB_WEAK)
                sym.st_info = GELF_ST_INFO(STB_GLOBAL,
                                           GELF_ST_TYPE(sym.st_info));
            gelf_update_sym(data, (int)i, &sym);
            updated = 1;
        }
        if (updated)
            elf_flagdata(data, ELF_C_SET, ELF_F_DIRTY);
    }

    if (elf_update(elf, ELF_C_WRITE) < 0)
        fprintf(stderr, "kld_update_elf_dynsym: elf_update: %s\n",
                elf_errmsg(-1));
    elf_end(elf);
done:
    free(sorted);
    close(dfd);
    return rc;
}

// Make kernel symbols UNDEF in the consumer executable's .dynsym so that
// ld.so performs a full scope search at startup, resolving them from the
// runtime DSOs (e.g. libkern.so with real kernel addresses) rather than
// using the zero placeholder values from the buildtime .so.
extern int
kld_undef_elf_dynsym(const char *path, const kld_sym *entries, size_t n)
{
    int rc = 0;

    kld_sym *sorted = malloc(n * sizeof(kld_sym));
    if (!sorted) return -1;
    memcpy(sorted, entries, n * sizeof(kld_sym));
    qsort(sorted, n, sizeof(kld_sym), cmp_sym_by_name);

    // mmap the file directly — avoids libelf write-layout issues entirely
    int dfd = open(path, O_RDWR);
    if (dfd == -1) { perror(path); free(sorted); return -1; }

    struct stat st;
    if (fstat(dfd, &st) < 0) {
        perror(path); rc = -1; goto done;
    }
    void *base = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, dfd, 0);
    if (base == MAP_FAILED) {
        perror("mmap"); rc = -1; goto done;
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)base;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "kld_undef_elf_dynsym: %s: not a 64-bit ELF\n", path);
        rc = -1; goto unmap;
    }

    Elf64_Shdr *shdrs = (Elf64_Shdr *)((char *)base + ehdr->e_shoff);
    Elf64_Shdr *dynsym_sh = NULL;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_DYNSYM) { dynsym_sh = &shdrs[i]; break; }
    }
    if (!dynsym_sh || !dynsym_sh->sh_entsize) {
        fprintf(stderr, "kld_undef_elf_dynsym: %s: no dynsym\n", path);
        rc = -1; goto unmap;
    }

    Elf64_Shdr *strtab_sh = &shdrs[dynsym_sh->sh_link];
    Elf64_Sym  *syms   = (Elf64_Sym *)((char *)base + dynsym_sh->sh_offset);
    char       *strtab = (char *)base + strtab_sh->sh_offset;
    size_t      sym_count = dynsym_sh->sh_size / dynsym_sh->sh_entsize;

    fprintf(stderr, "kld_undef_elf_dynsym: %s: sym_count=%zu n=%zu\n",
            path, sym_count, n);
    for (size_t i = 1; i < sym_count; i++) {
        // only patch WEAK ABS zero-value placeholders absorbed from buildtime DSOs
        if (syms[i].st_shndx != SHN_ABS || syms[i].st_value != 0) continue;
        const char *sname = strtab + syms[i].st_name;
        kld_sym key = { .name = (char *)sname };
        if (!bsearch(&key, sorted, n, sizeof(kld_sym), cmp_sym_by_name)) continue;
        fprintf(stderr, "  patching [%zu] %s shndx=SHN_ABS->UNDEF\n", i, sname);
        syms[i].st_shndx = SHN_UNDEF;
        syms[i].st_value = 0;
    }

unmap:
    munmap(base, st.st_size);
done:
    free(sorted);
    close(dfd);
    return rc;
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
kld_close_elf_secdata(kld_secdata *sd, int fd)
{
  if (!sd) return;
  if (sd->elf) {
    elf_end((Elf *)sd->elf);
    sd->elf = NULL;
  }
  if (sd->fd != fd && sd->fd >= 0) {
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
  
  // 6. Weak Data Symbol (Appears as 'V' in nm)
  e = &entries[i]; i++;
  assert(kld_sym_init_from_kallsyms(e, 0x7FFF00006000, "weak_data", 'V')>=0); 
    
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
  elf_ptr = kld_generate_elf_mmap(entries, n, nmstrlen, "libstubs.so", 0, &elf_size, &mfd);
  
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
