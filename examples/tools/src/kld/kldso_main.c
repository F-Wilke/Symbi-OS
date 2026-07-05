#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <errno.h>
#include "runtime_kotbl.h"
#include "module_patch.h"
#include "runtime_prepare.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

extern void *kld_initial_rsp;

/* --- minimal syscalls --- */
static inline long sys_write(int fd, const char *buf, unsigned long len) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_write), "D"(fd), "S"(buf), "d"(len)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline void sys_exit(int code) {
    asm volatile ("syscall" : : "a"(SYS_exit), "D"(code) : "rcx", "r11");
    __builtin_unreachable();
}

static inline long sys_openat(int dirfd, const char *path, int flags, int mode) {
    long ret;
    register long r10 __asm__("r10") = (long)mode;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_openat), "D"(dirfd), "S"(path), "d"(flags), "r"(r10)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_fstat(int fd, struct stat *st) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_fstat), "D"(fd), "S"(st)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline void *sys_mmap(void *addr, unsigned long len, int prot, int flags, int fd, unsigned long off) {
    long ret;
    register long r10 __asm__("r10") = (long)flags;
    register long r8 __asm__("r8") = (long)fd;
    register long r9 __asm__("r9") = (long)off;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_mmap), "D"(addr), "S"(len), "d"(prot), "r"(r10), "r"(r8), "r"(r9)
                  : "rcx", "r11", "memory");
    return (void *)ret;
}

static inline long sys_munmap(void *addr, unsigned long len) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_munmap), "D"(addr), "S"(len)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_delete_module(const char *name, unsigned int flags) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_delete_module), "D"(name), "S"(flags)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_init_module(void *image, unsigned long len, const char *param_values) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_init_module), "D"(image), "S"(len), "d"(param_values)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_read(int fd, void *buf, unsigned long len) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_read), "D"(fd), "S"(buf), "d"(len)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_close(int fd) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_close), "D"(fd)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_getpid(void) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_getpid)
                  : "rcx", "r11", "memory");
    return ret;
}

static inline long sys_rename(const char *oldpath, const char *newpath) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_rename), "D"(oldpath), "S"(newpath)
                  : "rcx", "r11", "memory");
    return ret;
}

/* --- tiny helpers --- */
static int my_strlen(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static int my_streq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }


    return a[i] == '\0' && b[i] == '\0';
}

static int starts_with(const char *s, const char *prefix) {
    int i = 0;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

static void copy_str(char *dst, int dsz, const char *src) {
    int i = 0;
    if (dsz <= 0) return;
    while (src && src[i] && i < dsz - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void my_memcpy(void *dst, const void *src, int n) {
    char *d = (char *)dst;
    const char *s = (const char *)src;
    for (int i = 0; i < n; i++) d[i] = s[i];
}

static int my_memcmp(const void *a, const void *b, int n) {
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    for (int i = 0; i < n; i++) {
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    }
    return 0;
}

static void my_memset(void *dst, int c, int n) {
    unsigned char *d = (unsigned char *)dst;
    for (int i = 0; i < n; i++) d[i] = (unsigned char)c;
}

static int is_kws(char c) {
    return c == ' ' || c == '\t';
}

static unsigned long align_down(unsigned long v, unsigned long a) { return v & ~(a - 1); }
static unsigned long align_up(unsigned long v, unsigned long a) { return (v + a - 1) & ~(a - 1); }

static char *get_env_value(char **envp, const char *key) {
    int klen = my_strlen(key);
    for (char **e = envp; *e; e++) {
        if (starts_with(*e, key) && (*e)[klen] == '=') {
            return &(*e)[klen + 1];
        }
    }
    return (char *)0;
}

static void *alloc_rw_or_die(unsigned long len);
static void debug_write(int debug, const char *m);

static char *build_ld_env_or_die(const char *ldpath) {
    const char *pfx = "LD_LIBRARY_PATH=";
    int plen = my_strlen(pfx);
    int llen = my_strlen(ldpath);
    char *s = (char *)alloc_rw_or_die((unsigned long)plen + (unsigned long)llen + 1);
    for (int i = 0; i < plen; i++) s[i] = pfx[i];
    for (int i = 0; i < llen; i++) s[plen + i] = ldpath[i];
    s[plen + llen] = '\0';
    return s;
}

static int inject_ldpath_env(char **envp, const char *ldpath, int debug) {
    if (!ldpath || !ldpath[0]) return 0;
    char *ld_env = build_ld_env_or_die(ldpath);

    for (int i = 0; envp[i]; i++) {
        if (starts_with(envp[i], "LD_LIBRARY_PATH=")) {
            envp[i] = ld_env;
            if (debug) debug_write(debug, "[KLD.SO]: env: replaced LD_LIBRARY_PATH\n");
            return 0;
        }
    }

    for (int i = 0; envp[i]; i++) {
        if (starts_with(envp[i], "KLD_LIBRARY_PATH=") ||
            starts_with(envp[i], "KLD_PREP_MODE=") ||
            starts_with(envp[i], "KLD_PREP_ONLY=") ||
            starts_with(envp[i], "KLD_NOEXEC=") ||
            starts_with(envp[i], "KLD_DEBUG=")) {
            envp[i] = ld_env;
            if (debug) {
                debug_write(debug, "[KLD.SO]: env: injected LD_LIBRARY_PATH via KLD_* slot\n");
            }
            return 0;
        }
    }

    for (int i = 0; envp[i]; i++) {
        if (starts_with(envp[i], "PWD=") ||
            starts_with(envp[i], "OLDPWD=") ||
            starts_with(envp[i], "PWD=") ||
            starts_with(envp[i], "OLDPWD=") ||
            starts_with(envp[i], "SHLVL=") ||
            starts_with(envp[i], "_=")) {
            envp[i] = ld_env;
            if (debug) {
                debug_write(debug, "[KLD.SO]: env: injected LD_LIBRARY_PATH via low-impact env slot\n");
            }
            return 0;
        }
    }

    if (envp[0]) {
        envp[0] = ld_env;
        if (debug) {
            debug_write(debug, "[KLD.SO]: env: injected LD_LIBRARY_PATH via fallback env slot\n");
        }
        return 0;
    }

    return -1;
}

static int parse_prep_mode_value(const char *s) {
    if (!s || !s[0] || my_streq(s, "all")) return KLD_PREP_ALL;
    if (my_streq(s, "load") || my_streq(s, "modules")) return KLD_PREP_LOAD_MODULES;
    if (my_streq(s, "update") || my_streq(s, "symbols") || my_streq(s, "syms")) return KLD_PREP_UPDATE_SOS;
    return -1;
}

static void die_msg(const char *m);

static void *alloc_rw_or_die(unsigned long len) {
    void *p = sys_mmap((void *)0, len, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((long)p < 0) die_msg("[KLD.SO]: mmap alloc failed");
    return p;
}

static void *grow_copy_or_die(void *oldp, unsigned long oldsz, unsigned long newsz) {
    void *np = alloc_rw_or_die(newsz);
    if (oldp && oldsz) my_memcpy(np, oldp, (int)oldsz);
    return np;
}

static char *dup_n_or_die(const char *s, int n) {
    if (n < 0) n = my_strlen(s);
    char *d = (char *)alloc_rw_or_die((unsigned long)n + 1);
    for (int i = 0; i < n; i++) d[i] = s[i];
    d[n] = '\0';
    return d;
}

typedef struct {
    char *name;
    unsigned long addr;
} SymEnt;

typedef struct {
    char *ko_path;
    char *mod_name;
    char *so_path;
    int opts_bits;
    SymEnt *syms;
    int sym_n;
    int sym_cap;
} ModEnt;

static unsigned long parse_hex_u64(const char *s, int n);

static void die_msg(const char *m) {
    sys_write(2, m, (unsigned long)my_strlen(m));
    sys_write(2, "\n", 1);
    sys_exit(1);
}

static void debug_write(int debug, const char *m) {
    if (!debug) return;
    sys_write(2, m, (unsigned long)my_strlen(m));
}

static int path_dirname(const char *path, char *out, int outsz) {
    int len = my_strlen(path);
    if (len <= 0 || outsz <= 1) return -1;
    int i = len - 1;
    while (i >= 0 && path[i] != '/') i--;
    if (i <= 0) return -1;
    if (i >= outsz) i = outsz - 1;
    for (int j = 0; j < i; j++) out[j] = path[j];
    out[i] = '\0';
    return 0;
}

static int path_list_contains(const char *list, const char *item) {
    if (!list || !item || !item[0]) return 0;
    int item_len = my_strlen(item);
    const char *p = list;
    while (*p) {
        const char *start = p;
        while (*p && *p != ':') p++;
        int n = (int)(p - start);
        if (n == item_len) {
            int same = 1;
            for (int i = 0; i < n; i++) {
                if (start[i] != item[i]) { same = 0; break; }
            }
            if (same) return 1;
        }
        if (*p == ':') p++;
    }
    return 0;
}

static void module_name_from_path(const char *path, char *out, int outsz) {
    int len = my_strlen(path), b = 0;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') { b = i + 1; break; }
    }
    int j = 0;
    for (int i = b; path[i] && j < outsz - 1; i++) out[j++] = path[i];
    out[j] = '\0';
    int n = my_strlen(out);
    if (n > 3 && out[n - 3] == '.' && out[n - 2] == 'k' && out[n - 1] == 'o')
        out[n - 3] = '\0';
}

enum {
    KLDOPT_NONE    = 0,
    KLDOPT_SHARED  = 1 << 0,
    KLDOPT_PERPROC = 1 << 1,
    KLDOPT_RELOAD  = 1 << 2
};

static int parse_kldopts_bits_raw(const char *opts) {
    return kld_parse_kldopts_bits(opts, opts ? my_strlen(opts) : 0);
}

static int parse_modinfo_kld_bits(const char *vals, int len) {
    return kld_parse_kldopts_bits(vals, len);
}

static int read_modinfo_meta(const char *ko_path, char *name_out, int name_outsz, int *opts_bits_out) {
    if (name_out && name_outsz > 0) name_out[0] = '\0';
    if (opts_bits_out) *opts_bits_out = KLDOPT_NONE;
    int fd = (int)sys_openat(AT_FDCWD, ko_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        return -1;
    }
    void *base = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((long)base < 0) {
        sys_close(fd);
        return -1;
    }
    Elf64_Ehdr *eh = (Elf64_Ehdr *)base;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }
    if (!(eh->e_shoff && eh->e_shstrndx != SHN_UNDEF && eh->e_shnum > 0)) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }
    Elf64_Shdr *sh = (Elf64_Shdr *)((char *)base + eh->e_shoff);
    const char *shnames = (const char *)base + sh[eh->e_shstrndx].sh_offset;
    Elf64_Shdr *modinfo = 0;
    for (int i = 0; i < eh->e_shnum; i++) {
        const char *nm = shnames + sh[i].sh_name;
        if (my_streq(nm, ".modinfo")) { modinfo = &sh[i]; break; }
    }
    if (!modinfo || modinfo->sh_size == 0) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }
    const char *p = (const char *)base + modinfo->sh_offset;
    int rem = (int)modinfo->sh_size;
    while (rem > 0) {
        int n = my_strlen(p);
        if (n == 0) { p++; rem--; continue; }
        if (n + 1 > rem) break;
        if (n > 5 && p[0] == 'n' && p[1] == 'a' && p[2] == 'm' && p[3] == 'e' && p[4] == '=') {
            if (name_out && name_outsz > 0) {
                int l = n - 5;
                if (l >= name_outsz) l = name_outsz - 1;
                for (int i = 0; i < l; i++) name_out[i] = p[5 + i];
                name_out[l] = '\0';
            }
        } else if (n > 4 && p[0] == 'k' && p[1] == 'l' && p[2] == 'd' && p[3] == '=') {
            if (opts_bits_out) *opts_bits_out |= parse_modinfo_kld_bits(&p[4], n - 4);
        }
        p += n + 1;
        rem -= n + 1;
    }
    sys_munmap(base, (unsigned long)st.st_size);
    sys_close(fd);
    return 0;
}

static void path_list_append_unique(char *list, int lsz, const char *item) {
    if (!item || !item[0]) return;
    if (path_list_contains(list, item)) return;
    int cur = my_strlen(list);
    int add = my_strlen(item);
    if (cur + (cur ? 1 : 0) + add >= lsz) return;
    if (cur) list[cur++] = ':';
    for (int i = 0; i < add; i++) list[cur++] = item[i];
    list[cur] = '\0';
}

static void path_list_append_unique_n(char *list, int lsz, const char *item, int item_len) {
    if (!item || item_len <= 0) return;
    int cur = my_strlen(list);
    const char *p = list;
    while (*p) {
        const char *start = p;
        while (*p && *p != ':') p++;
        if ((int)(p - start) == item_len && my_memcmp(start, item, item_len) == 0) return;
        if (*p == ':') p++;
    }
    if (cur + (cur ? 1 : 0) + item_len >= lsz) return;
    if (cur) list[cur++] = ':';
    for (int i = 0; i < item_len; i++) list[cur++] = item[i];
    list[cur] = '\0';
}

static void path_list_append_dir_unique(char *list, int lsz, const char *path) {
    int len = my_strlen(path);
    int slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') { slash = i; break; }
    }
    if (slash <= 0) return;
    path_list_append_unique_n(list, lsz, path, slash);
}

static int read_original_interp(const char *target_path, char *out, int outsz) {
    int fd = (int)sys_openat(AT_FDCWD, target_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;

    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        return -1;
    }

    void *base = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((long)base < 0) {
        sys_close(fd);
        return -1;
    }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)base;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    if (eh->e_shoff && eh->e_shstrndx != SHN_UNDEF && eh->e_shnum > 0) {
        Elf64_Shdr *sh = (Elf64_Shdr *)((char *)base + eh->e_shoff);
        Elf64_Shdr *shstr = &sh[eh->e_shstrndx];
        const char *shnames = (const char *)base + shstr->sh_offset;
        for (int i = 0; i < eh->e_shnum; i++) {
            const char *nm = shnames + sh[i].sh_name;
            if (my_streq(nm, ".interp_orig")) {
                const char *p = (const char *)base + sh[i].sh_offset;
                int n = (int)sh[i].sh_size;
                if (n > 0) {
                    if (n >= outsz) n = outsz - 1;
                    my_memcpy(out, p, n);
                    out[n] = '\0';
                    if (out[0]) {
                        sys_munmap(base, (unsigned long)st.st_size);
                        sys_close(fd);
                        return 0;
                    }
                }
            }
        }
    }

    if (eh->e_phoff && eh->e_phnum > 0) {
        Elf64_Phdr *ph = (Elf64_Phdr *)((char *)base + eh->e_phoff);
        for (int i = 0; i < eh->e_phnum; i++) {
            if (ph[i].p_type == PT_INTERP && ph[i].p_filesz > 1) {
                const char *p = (const char *)base + ph[i].p_offset;
                int n = (int)ph[i].p_filesz;
                if (n >= outsz) n = outsz - 1;
                my_memcpy(out, p, n);
                out[n] = '\0';
                sys_munmap(base, (unsigned long)st.st_size);
                sys_close(fd);
                return out[0] ? 0 : -1;
            }
        }
    }

    sys_munmap(base, (unsigned long)st.st_size);
    sys_close(fd);
    return -1;
}

static int build_ldpath_from_kotbl(const char *target_path, char *out, int outsz) {
    out[0] = '\0';
    int fd = (int)sys_openat(AT_FDCWD, target_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;

    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        return -1;
    }
    void *base = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((long)base < 0) {
        sys_close(fd);
        return -1;
    }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)base;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    Elf64_Shdr *sh = 0;
    const char *shnames = 0;
    if (eh->e_shoff && eh->e_shstrndx != SHN_UNDEF && eh->e_shnum > 0) {
        sh = (Elf64_Shdr *)((char *)base + eh->e_shoff);
        shnames = (const char *)base + sh[eh->e_shstrndx].sh_offset;
    }
    if (!sh || !shnames) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    Elf64_Shdr *kotbl = 0;
    for (int i = 0; i < eh->e_shnum; i++) {
        const char *nm = shnames + sh[i].sh_name;
        if (my_streq(nm, ".kotbl")) { kotbl = &sh[i]; break; }
    }
    if (!kotbl || kotbl->sh_size == 0) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    const char *tbl = (const char *)base + kotbl->sh_offset;
    size_t nn = 0, size = (size_t)kotbl->sh_size;
    kld_kotbl_ent ent;
    while (1) {
        int krc = kld_kotbl_next(tbl, size, &nn, &ent);
        if (krc == 0) break;
        if (krc < 0) break;
        path_list_append_dir_unique(out, outsz, ent.fnm);
    }

    sys_munmap(base, (unsigned long)st.st_size);
    sys_close(fd);
    return out[0] ? 0 : -1;
}

enum {
    MODLOAD_STATUS_LOADED = 0,
    MODLOAD_STATUS_SHARED,
    MODLOAD_STATUS_RELOADED
};

static int init_module_with_opts(void *image, unsigned long image_size,
                                 const char *mod_name, int opts_bits,
                                 int *status_out) {
    long r = sys_init_module(image, image_size, "");
    if (r >= 0) {
        if (status_out) *status_out = MODLOAD_STATUS_LOADED;
        return 0;
    }
    int err = (int)(-r);
    if (err == EEXIST && (opts_bits & KLDOPT_SHARED)) {
        if (status_out) *status_out = MODLOAD_STATUS_SHARED;
        return 0;
    }
    if (err == EEXIST && (opts_bits & KLDOPT_RELOAD)) {
        if (sys_delete_module(mod_name, O_NONBLOCK) == 0) {
            long r2 = sys_init_module(image, image_size, "");
            if (r2 >= 0) {
                if (status_out) *status_out = MODLOAD_STATUS_RELOADED;
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

int load_modules_from_kotbl(const char *target_path, int debug) {
    int fd = (int)sys_openat(AT_FDCWD, target_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        return -1;
    }

    void *base = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((long)base < 0) {
        sys_close(fd);
        return -1;
    }
    Elf64_Ehdr *eh = (Elf64_Ehdr *)base;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }
    Elf64_Shdr *sh = 0;
    const char *shnames = 0;
    if (eh->e_shoff && eh->e_shstrndx != SHN_UNDEF && eh->e_shnum > 0) {
        sh = (Elf64_Shdr *)((char *)base + eh->e_shoff);
        shnames = (const char *)base + sh[eh->e_shstrndx].sh_offset;
    }
    if (!sh || !shnames) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    Elf64_Shdr *kotbl = 0;
    for (int i = 0; i < eh->e_shnum; i++) {
        const char *nm = shnames + sh[i].sh_name;
        if (my_streq(nm, ".kotbl")) { kotbl = &sh[i]; break; }
    }
    if (!kotbl || kotbl->sh_size == 0) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return 0;
    }

    int rc = 0;
    const char *tbl = (const char *)base + kotbl->sh_offset;
    size_t nn = 0, size = (size_t)kotbl->sh_size;
    kld_kotbl_ent ent;
    while (1) {
        int krc = kld_kotbl_next(tbl, size, &nn, &ent);
        if (krc == 0) break;
        if (krc < 0) { rc = -1; break; }

        const char *fnm = ent.fnm;
        const char *modnm = ent.modnm;
        const char *opts = ent.opts;
        int modnm_len = ent.modnm_len;
        if (modnm_len > 0 && my_streq(modnm, "/proc/kallsyms")) continue;

        char modinfo_name[256];
        int modinfo_opts = KLDOPT_NONE;
        (void)read_modinfo_meta(fnm, modinfo_name, sizeof(modinfo_name), &modinfo_opts);
        int opts_bits = modinfo_opts | parse_kldopts_bits_raw(opts);
        if (opts_bits == KLDOPT_NONE) opts_bits = KLDOPT_RELOAD;

        int mfd = (int)sys_openat(AT_FDCWD, fnm, O_RDONLY | O_CLOEXEC, 0);
        if (mfd < 0) { rc = -1; continue; }
        struct stat mst;
        if (sys_fstat(mfd, &mst) < 0 || mst.st_size < (off_t)sizeof(Elf64_Ehdr)) {
            rc = -1;
            sys_close(mfd);
            continue;
        }
        void *mimg = sys_mmap((void *)0, (unsigned long)mst.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, mfd, 0);
        if ((long)mimg < 0) {
            rc = -1;
            sys_close(mfd);
            continue;
        }

        char target_mod_name[256];
        if (modnm_len > 0) {
            int n = modnm_len;
            if (n >= (int)sizeof(target_mod_name)) n = (int)sizeof(target_mod_name) - 1;
            for (int i = 0; i < n; i++) target_mod_name[i] = modnm[i];
            target_mod_name[n] = '\0';
        } else if (modinfo_name[0]) {
            copy_str(target_mod_name, sizeof(target_mod_name), modinfo_name);
        } else {
            module_name_from_path(fnm, target_mod_name, sizeof(target_mod_name));
        }

        int mod_rc = 0;
        if (modnm_len > 0 && modinfo_name[0] && !my_streq(modinfo_name, target_mod_name)) {
            if (kld_patch_module_name_image(mimg, (size_t)mst.st_size, modinfo_name, target_mod_name) < 0) {
                mod_rc = -1;
            }
        }

        int mod_status = MODLOAD_STATUS_LOADED;
        if (mod_rc == 0 &&
            init_module_with_opts(mimg, (unsigned long)mst.st_size, target_mod_name, opts_bits, &mod_status) < 0) {
            mod_rc = -1;
        }
        if (mod_rc < 0) rc = -1;
        sys_munmap(mimg, (unsigned long)mst.st_size);
        if (debug) {
            debug_write(debug, "[KLD.SO]: module=");
            debug_write(debug, fnm);
            debug_write(debug, " status=");
            if (mod_rc < 0) debug_write(debug, "failed");
            else if (mod_status == MODLOAD_STATUS_SHARED) debug_write(debug, "shared");
            else if (mod_status == MODLOAD_STATUS_RELOADED) debug_write(debug, "reloaded");
            else debug_write(debug, "loaded");
            debug_write(debug, "\n");
        }
        sys_close(mfd);
    }

    sys_munmap(base, (unsigned long)st.st_size);
    sys_close(fd);
    return rc;
}

static unsigned long fnv1a64_str(const char *s) {
    unsigned long h = 1469598103934665603ULL;
    for (int i = 0; s && s[i]; i++) {
        h ^= (unsigned long)(unsigned char)s[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static int read_boot_id(char *out, int outsz) {
    int fd = (int)sys_openat(AT_FDCWD, "/proc/sys/kernel/random/boot_id", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    long n = sys_read(fd, out, (unsigned long)outsz - 1);
    sys_close(fd);
    if (n <= 0) return -1;
    int nn = (int)n;
    while (nn > 0 && (out[nn - 1] == '\n' || out[nn - 1] == '\r')) nn--;
    out[nn] = '\0';
    return (nn > 0) ? 0 : -1;
}

static void make_stamp_path(const char *kind, const char *key, char *out, int outsz) {
    unsigned long h = fnv1a64_str(key);
    int p = 0;
    const char *base = "/tmp/kld.";
    for (int i = 0; base[i] && p + 1 < outsz; i++) out[p++] = base[i];
    for (int i = 0; kind[i] && p + 1 < outsz; i++) out[p++] = kind[i];
    if (p + 1 < outsz) out[p++] = '.';
    for (int shift = 60; shift >= 0 && p + 1 < outsz; shift -= 4) {
        int d = (int)((h >> shift) & 0xf);
        out[p++] = (char)(d < 10 ? ('0' + d) : ('a' + (d - 10)));
    }
    out[p] = '\0';
}

static unsigned long syms_anchor(const SymEnt *syms, int n) {
    unsigned long a = 0;
    for (int i = 0; i < n; i++) {
        unsigned long v = syms[i].addr;
        if (v == 0) continue;
        if (a == 0 || v < a) a = v;
    }
    return a;
}

static unsigned long fnv1a64_update_u64(unsigned long h, unsigned long v) {
    for (int i = 0; i < 8; i++) {
        h ^= (unsigned long)((v >> (i * 8)) & 0xff);
        h *= 1099511628211ULL;
    }
    return h;
}

static unsigned long syms_hash(const SymEnt *syms, int n) {
    unsigned long h = 1469598103934665603ULL;
    for (int i = 0; i < n; i++) {
        h = fnv1a64_update_u64(h, fnv1a64_str(syms[i].name));
        h = fnv1a64_update_u64(h, syms[i].addr);
    }
    return h;
}

static int read_stamp_for_path(const char *kind, const char *path,
                               char *boot_id_out, int boot_id_outsz,
                               unsigned long *anchor_out,
                               unsigned long *hash_out,
                               struct stat *stamp_st_out) {
    char sp[96];
    make_stamp_path(kind, path, sp, sizeof(sp));
    int fd = (int)sys_openat(AT_FDCWD, sp, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    char got[192];
    long n = sys_read(fd, got, sizeof(got) - 1);
    struct stat st_stamp = {0};
    long sr = sys_fstat(fd, &st_stamp);
    sys_close(fd);
    if (n <= 0 || sr < 0) return -1;
    got[(int)n] = '\0';
    if (stamp_st_out) *stamp_st_out = st_stamp;

    if (boot_id_out && boot_id_outsz > 0) boot_id_out[0] = '\0';
    if (anchor_out) *anchor_out = 0;
    if (hash_out) *hash_out = 0;

    char *boot = (char *)0, *anchor = (char *)0, *hash = (char *)0;
    char *p = got;
    while (*p) {
        if (p[0] == 'b' && p[1] == 'o' && p[2] == 'o' && p[3] == 't' && p[4] == '=') boot = &p[5];
        else if (p[0] == 'a' && p[1] == 'n' && p[2] == 'c' && p[3] == 'h' && p[4] == 'o' && p[5] == 'r' && p[6] == '=') anchor = &p[7];
        else if (p[0] == 'h' && p[1] == 'a' && p[2] == 's' && p[3] == 'h' && p[4] == '=') hash = &p[5];
        while (*p && *p != '\n' && *p != '\r') p++;
        if (*p) *p++ = '\0';
        while (*p == '\n' || *p == '\r') p++;
    }
    if (!boot || !hash) return -1;
    if (boot_id_out && boot_id_outsz > 0) {
        int bl = my_strlen(boot);
        if (bl >= boot_id_outsz) bl = boot_id_outsz - 1;
        for (int i = 0; i < bl; i++) boot_id_out[i] = boot[i];
        boot_id_out[bl] = '\0';
    }
    if (anchor_out && anchor) *anchor_out = parse_hex_u64(anchor, my_strlen(anchor));
    if (hash_out) *hash_out = parse_hex_u64(hash, my_strlen(hash));
    return 0;
}

static int stamp_covers_so(const struct stat *stamp_st, const char *so_path) {
    int pfd = (int)sys_openat(AT_FDCWD, so_path, O_RDONLY | O_CLOEXEC, 0);
    if (pfd < 0) return 0;
    struct stat so_st = {0};
    long pr = sys_fstat(pfd, &so_st);
    sys_close(pfd);
    if (pr < 0) return 0;
    return (stamp_st->st_mtime > so_st.st_mtime) ||
           (stamp_st->st_mtime == so_st.st_mtime &&
            stamp_st->st_mtim.tv_nsec >= so_st.st_mtim.tv_nsec);
}

static void write_stamp_for_path(const char *kind, const char *path, const char *boot_id,
                                 unsigned long anchor, unsigned long hash) {
    char sp[96];
    char tmp[120];
    make_stamp_path(kind, path, sp, sizeof(sp));
    int pid = (int)sys_getpid();
    int tlen = 0;
    for (; sp[tlen] && tlen < (int)sizeof(tmp) - 1; tlen++) tmp[tlen] = sp[tlen];
    if (tlen + 16 >= (int)sizeof(tmp)) return;
    tmp[tlen++] = '.';
    tmp[tlen++] = 't'; tmp[tlen++] = 'm'; tmp[tlen++] = 'p'; tmp[tlen++] = '.';
    char digs[16];
    int di = 0;
    if (pid == 0) digs[di++] = '0';
    while (pid > 0 && di < (int)sizeof(digs)) { digs[di++] = (char)('0' + (pid % 10)); pid /= 10; }
    for (int i = di - 1; i >= 0; i--) tmp[tlen++] = digs[i];
    tmp[tlen] = '\0';
    int fd = (int)sys_openat(AT_FDCWD, tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0) return;
    sys_write(fd, "boot=", 5);
    sys_write(fd, boot_id, (unsigned long)my_strlen(boot_id));
    sys_write(fd, "\nanchor=", 8);
    for (int shift = 60; shift >= 0; shift -= 4) {
        int d = (int)((anchor >> shift) & 0xf);
        char c = (char)(d < 10 ? ('0' + d) : ('a' + (d - 10)));
        sys_write(fd, &c, 1);
    }
    sys_write(fd, "\nhash=", 6);
    for (int shift = 60; shift >= 0; shift -= 4) {
        int d = (int)((hash >> shift) & 0xf);
        char c = (char)(d < 10 ? ('0' + d) : ('a' + (d - 10)));
        sys_write(fd, &c, 1);
    }
    sys_write(fd, "\n", 1);
    sys_close(fd);
    (void)sys_rename(tmp, sp);
}

static unsigned long parse_hex_u64(const char *s, int n) {
    unsigned long v = 0;
    for (int i = 0; i < n; i++) {
        char c = s[i];
        unsigned long d;
        if (c >= '0' && c <= '9') d = (unsigned long)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (unsigned long)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = (unsigned long)(c - 'A' + 10);
        else break;
        v = (v << 4) | d;
    }
    return v;
}

static int append_sym(ModEnt *m, const char *name, int name_len, unsigned long addr) {
    if (!m || !name || name_len <= 0) return -1;
    if (m->sym_n == m->sym_cap) {
        int new_cap = (m->sym_cap == 0) ? 1024 : (m->sym_cap << 1);
        unsigned long oldsz = (unsigned long)m->sym_cap * sizeof(SymEnt);
        unsigned long newsz = (unsigned long)new_cap * sizeof(SymEnt);
        SymEnt *ns = (SymEnt *)grow_copy_or_die(m->syms, oldsz, newsz);
        if (m->syms) sys_munmap(m->syms, oldsz);
        m->syms = ns;
        m->sym_cap = new_cap;
    }
    m->syms[m->sym_n].name = dup_n_or_die(name, name_len);
    m->syms[m->sym_n].addr = addr;
    m->sym_n++;
    return 0;
}

typedef struct {
    const SymEnt *syms;
    int n;
    int cap;
    int *slots; /* -1 empty, otherwise symbol index */
} SymIndex;

static int sym_index_build(SymIndex *idx, const SymEnt *syms, int n) {
    int cap = 1;
    while (cap < (n << 1)) cap <<= 1;
    if (cap < 16) cap = 16;
    idx->slots = (int *)alloc_rw_or_die((unsigned long)cap * sizeof(int));
    for (int i = 0; i < cap; i++) idx->slots[i] = -1;
    idx->syms = syms;
    idx->n = n;
    idx->cap = cap;
    for (int i = 0; i < n; i++) {
        unsigned long h = fnv1a64_str(syms[i].name);
        int s = (int)(h & (unsigned long)(cap - 1));
        while (idx->slots[s] != -1) {
            if (my_streq(syms[idx->slots[s]].name, syms[i].name)) break;
            s = (s + 1) & (cap - 1);
        }
        if (idx->slots[s] == -1) idx->slots[s] = i;
    }
    return 0;
}

static void sym_index_free(SymIndex *idx) {
    if (!idx || !idx->slots) return;
    sys_munmap(idx->slots, (unsigned long)idx->cap * sizeof(int));
    idx->slots = (int *)0;
    idx->cap = 0;
    idx->n = 0;
    idx->syms = (const SymEnt *)0;
}

static unsigned long sym_index_lookup(const SymIndex *idx, const char *name) {
    int cap = idx->cap;
    int s = (int)(fnv1a64_str(name) & (unsigned long)(cap - 1));
    while (idx->slots[s] != -1) {
        int i = idx->slots[s];
        if (my_streq(idx->syms[i].name, name)) return idx->syms[i].addr;
        s = (s + 1) & (cap - 1);
    }
    return 0;
}

static int update_sym_section_simple(void *base, Elf64_Ehdr *eh, Elf64_Shdr *sh,
                                     Elf64_Shdr *symsec, const SymIndex *idx) {
    if (!symsec || symsec->sh_entsize == 0 || symsec->sh_link >= (unsigned)eh->e_shnum) return 0;
    Elf64_Shdr *strtab = &sh[symsec->sh_link];
    char *strs = (char *)base + strtab->sh_offset;
    unsigned long strsz = (unsigned long)strtab->sh_size;
    int nsyms = (int)(symsec->sh_size / symsec->sh_entsize);
    Elf64_Sym *es = (Elf64_Sym *)((char *)base + symsec->sh_offset);
    int updates = 0;
    for (int i = 0; i < nsyms; i++) {
        if (es[i].st_name >= strsz) continue;
        char *nm = strs + es[i].st_name;
        if (!nm || !nm[0]) continue;
        unsigned long a = sym_index_lookup(idx, nm);
        if (a == 0) continue;
        es[i].st_value = a;
        es[i].st_shndx = SHN_ABS;
        if (ELF64_ST_BIND(es[i].st_info) == STB_WEAK)
            es[i].st_info = ELF64_ST_INFO(STB_GLOBAL, ELF64_ST_TYPE(es[i].st_info));
        updates++;
    }
    return updates;
}

static int update_so_dynsym_simple(const char *so_path, const SymEnt *syms, int sym_n, int debug) {
    if (!so_path || !syms || sym_n <= 0) return -1;
    SymIndex idx = {0};
    sym_index_build(&idx, syms, sym_n);
    int fd = (int)sys_openat(AT_FDCWD, so_path, O_RDWR | O_CLOEXEC, 0);
    if (fd < 0) {
        sym_index_free(&idx);
        return -1;
    }
    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        sym_index_free(&idx);
        return -1;
    }
    void *base = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((long)base < 0) {
        sys_close(fd);
        sym_index_free(&idx);
        return -1;
    }
    Elf64_Ehdr *eh = (Elf64_Ehdr *)base;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64 ||
        eh->e_shoff == 0 || eh->e_shnum == 0 || eh->e_shstrndx == SHN_UNDEF) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        sym_index_free(&idx);
        return -1;
    }
    Elf64_Shdr *sh = (Elf64_Shdr *)((char *)base + eh->e_shoff);
    Elf64_Shdr *dynsym = 0;
    Elf64_Shdr *symtab = 0;
    for (int i = 0; i < eh->e_shnum; i++) {
        if (sh[i].sh_type == SHT_DYNSYM && !dynsym) dynsym = &sh[i];
        if (sh[i].sh_type == SHT_SYMTAB && !symtab) symtab = &sh[i];
    }
    if (!dynsym) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        sym_index_free(&idx);
        return -1;
    }
    int dyn_updates = update_sym_section_simple(base, eh, sh, dynsym, &idx);
    int sym_updates = update_sym_section_simple(base, eh, sh, symtab, &idx);
    sys_munmap(base, (unsigned long)st.st_size);
    sys_close(fd);
    sym_index_free(&idx);
    if (debug && (dyn_updates > 0 || sym_updates > 0)) {
        debug_write(debug, "[KLD.SO]: updated ");
        debug_write(debug, so_path);
        if (sym_updates > 0) debug_write(debug, " (dynsym+symtab)");
        debug_write(debug, "\n");
    }
    return (dyn_updates > 0 || sym_updates > 0) ? 0 : -1;
}

static int parse_kotbl_modules(const char *target_path, ModEnt **mods_out, int *mod_n_out, char **libkern_so_out) {
    *mods_out = (ModEnt *)0;
    *mod_n_out = 0;
    *libkern_so_out = (char *)0;
    int fd = (int)sys_openat(AT_FDCWD, target_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;
    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        return -1;
    }
    void *base = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((long)base < 0) {
        sys_close(fd);
        return -1;
    }
    Elf64_Ehdr *eh = (Elf64_Ehdr *)base;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64 ||
        eh->e_shoff == 0 || eh->e_shnum == 0 || eh->e_shstrndx == SHN_UNDEF) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }
    Elf64_Shdr *sh = (Elf64_Shdr *)((char *)base + eh->e_shoff);
    const char *shnames = (const char *)base + sh[eh->e_shstrndx].sh_offset;
    Elf64_Shdr *kotbl = 0;
    for (int i = 0; i < eh->e_shnum; i++) {
        const char *nm = shnames + sh[i].sh_name;
        if (my_streq(nm, ".kotbl")) { kotbl = &sh[i]; break; }
    }
    if (!kotbl || kotbl->sh_size == 0) {
        sys_munmap(base, (unsigned long)st.st_size);
        sys_close(fd);
        return 0;
    }

    int mod_cap = 0, mod_n = 0;
    ModEnt *mods = (ModEnt *)0;
    const char *tbl = (const char *)base + kotbl->sh_offset;
    size_t nn = 0, size = (size_t)kotbl->sh_size;
    kld_kotbl_ent ent;
    while (1) {
        int krc = kld_kotbl_next(tbl, size, &nn, &ent);
        if (krc == 0) break;
        if (krc < 0) break;
        const char *fnm = ent.fnm;
        int fnm_len = ent.fnm_len;
        const char *modnm = ent.modnm;
        int modnm_len = ent.modnm_len;
        const char *opts = ent.opts;
        int opts_len = ent.opts_len;

        if (modnm_len > 0 && my_streq(modnm, "/proc/kallsyms")) {
            *libkern_so_out = dup_n_or_die(fnm, fnm_len);
            continue;
        }
        if (mod_n == mod_cap) {
            int nc = (mod_cap == 0) ? 32 : (mod_cap << 1);
            unsigned long oldsz = (unsigned long)mod_cap * sizeof(ModEnt);
            unsigned long newsz = (unsigned long)nc * sizeof(ModEnt);
            ModEnt *nm = (ModEnt *)grow_copy_or_die(mods, oldsz, newsz);
            if (mods) sys_munmap(mods, oldsz);
            mods = nm;
            my_memset(&mods[mod_cap], 0, (nc - mod_cap) * (int)sizeof(ModEnt));
            mod_cap = nc;
        }

        char mname_tmp[256];
        int modinfo_opts = KLDOPT_NONE;
        mname_tmp[0] = '\0';
        (void)read_modinfo_meta(fnm, mname_tmp, sizeof(mname_tmp), &modinfo_opts);
        if (modnm_len > 0) mods[mod_n].mod_name = dup_n_or_die(modnm, modnm_len);
        else if (mname_tmp[0]) mods[mod_n].mod_name = dup_n_or_die(mname_tmp, -1);
        else {
            char fallback[256];
            module_name_from_path(fnm, fallback, sizeof(fallback));
            mods[mod_n].mod_name = dup_n_or_die(fallback, -1);
        }
        mods[mod_n].opts_bits = modinfo_opts | parse_kldopts_bits_raw(opts);
        if (mods[mod_n].opts_bits == KLDOPT_NONE) mods[mod_n].opts_bits = KLDOPT_RELOAD;
        mods[mod_n].ko_path = dup_n_or_die(fnm, fnm_len);

        int slash = -1;
        for (int i = fnm_len - 1; i >= 0; i--) if (fnm[i] == '/') { slash = i; break; }
        int mlen = my_strlen(mods[mod_n].mod_name);
        int so_len = (slash > 0 ? slash : 0) + 4 + mlen + 3 + 1; /* /lib + name + .so + nul */
        char *so = (char *)alloc_rw_or_die((unsigned long)so_len);
        int p = 0;
        for (int i = 0; i < slash; i++) so[p++] = fnm[i];
        so[p++] = '/'; so[p++] = 'l'; so[p++] = 'i'; so[p++] = 'b';
        for (int i = 0; i < mlen; i++) so[p++] = mods[mod_n].mod_name[i];
        so[p++] = '.'; so[p++] = 's'; so[p++] = 'o'; so[p] = '\0';
        mods[mod_n].so_path = so;
        mod_n++;
    }

    sys_munmap(base, (unsigned long)st.st_size);
    sys_close(fd);
    *mods_out = mods;
    *mod_n_out = mod_n;
    return 0;
}

int collect_and_apply_runtime_symbols(const char *target_path, int debug) {
    ModEnt *mods = (ModEnt *)0;
    int mod_n = 0;
    char *libkern_so = (char *)0;
    if (parse_kotbl_modules(target_path, &mods, &mod_n, &libkern_so) < 0) return -1;

    int kcap = 0, kn = 0;
    SymEnt *ksyms = (SymEnt *)0;
    if (debug) debug_write(debug, "[KLD.SO]: update: reading /proc/kallsyms\n");
    int kfd = (int)sys_openat(AT_FDCWD, "/proc/kallsyms", O_RDONLY | O_CLOEXEC, 0);
    if (kfd < 0) return -1;
    unsigned long cap = 1UL << 20;
    unsigned long used = 0;
    char *buf = (char *)alloc_rw_or_die(cap);
    while (1) {
        if (used + 4096 >= cap) {
            unsigned long nc = cap << 1;
            char *nb = (char *)grow_copy_or_die(buf, used, nc);
            sys_munmap(buf, cap);
            buf = nb;
            cap = nc;
        }
        long r = sys_read(kfd, buf + used, cap - used - 1);
        if (r < 0) { sys_close(kfd); return -1; }
        if (r == 0) break;
        used += (unsigned long)r;
    }
    sys_close(kfd);
    buf[used] = '\0';
    if (debug) debug_write(debug, "[KLD.SO]: update: parsing kallsyms\n");

    char *line = buf;
    for (unsigned long i = 0; i <= used; i++) {
        if (buf[i] != '\n' && buf[i] != '\0') continue;
        buf[i] = '\0';
        if (line[0]) {
            int p = 0;
            while (is_kws(line[p])) p++;
            int ah = p; while (line[p] && !is_kws(line[p])) p++;
            int ah_len = p - ah;
            while (is_kws(line[p])) p++;
            if (line[p]) p++; /* type */
            while (is_kws(line[p])) p++;
            int ns = p; while (line[p] && !is_kws(line[p])) p++;
            int nl = p - ns;
            while (is_kws(line[p])) p++;
            int ms = -1, ml = 0;
            if (line[p] == '[') {
                ms = p + 1;
                p++;
                while (line[p] && line[p] != ']') p++;
                ml = p - ms;
            }
            if (ah_len > 0 && nl > 0) {
                unsigned long addr = parse_hex_u64(&line[ah], ah_len);
                if (ms >= 0 && ml > 0) {
                    for (int m = 0; m < mod_n; m++) {
                        if (my_strlen(mods[m].mod_name) == ml &&
                            my_memcmp(mods[m].mod_name, &line[ms], ml) == 0) {
                            append_sym(&mods[m], &line[ns], nl, addr);
                            break;
                        }
                    }
                } else {
                    if (kn == kcap) {
                        int nc = (kcap == 0) ? 4096 : (kcap << 1);
                        unsigned long oldsz = (unsigned long)kcap * sizeof(SymEnt);
                        unsigned long newsz = (unsigned long)nc * sizeof(SymEnt);
                        SymEnt *nk = (SymEnt *)grow_copy_or_die(ksyms, oldsz, newsz);
                        if (ksyms) sys_munmap(ksyms, oldsz);
                        ksyms = nk;
                        kcap = nc;
                    }
                    ksyms[kn].name = dup_n_or_die(&line[ns], nl);
                    ksyms[kn].addr = addr;
                    kn++;
                }
            }
        }
        line = &buf[i + 1];
    }
    if (debug) {
        debug_write(debug, "[KLD.SO]: update: parsed kallsyms complete\n");
    }

    int rc = 0;
    char boot_id[96];
    int have_boot_id = (read_boot_id(boot_id, sizeof(boot_id)) == 0);
    if (libkern_so && kn > 0) {
        if (debug) {
            debug_write(debug, "[KLD.SO]: update: processing ");
            debug_write(debug, libkern_so);
            debug_write(debug, "\n");
        }
        unsigned long khash = syms_hash(ksyms, kn);
        unsigned long kanchor = syms_anchor(ksyms, kn);
        int do_update = 1;
        if (have_boot_id) {
            char sbid[96];
            unsigned long sanchor = 0, shash = 0;
            struct stat sst = {0};
            if (read_stamp_for_path("libkern", libkern_so, sbid, sizeof(sbid), &sanchor, &shash, &sst) == 0 &&
                my_streq(sbid, boot_id) && shash == khash && stamp_covers_so(&sst, libkern_so)) {
                do_update = 0;
                if (debug) debug_write(debug, "[KLD.SO]: update: libkern stamp match; skipping\n");
            }
        }
        if (do_update) {
            if (update_so_dynsym_simple(libkern_so, ksyms, kn, debug) < 0) {
                if (debug) {
                    debug_write(debug, "[KLD.SO]: update: in-place update failed; rebuild fallback unavailable in standalone mode\n");
                }
                rc = -1;
            }
            else if (have_boot_id) write_stamp_for_path("libkern", libkern_so, boot_id, kanchor, khash);
        }
    }
    for (int i = 0; i < mod_n; i++) {
        if (mods[i].sym_n <= 0) {
            if (debug) {
                debug_write(debug, "[KLD.SO]: update: skipping ");
                debug_write(debug, mods[i].so_path);
                debug_write(debug, " (module symbols not present in /proc/kallsyms)\n");
            }
            continue;
        }
        if (debug) {
            debug_write(debug, "[KLD.SO]: update: processing ");
            debug_write(debug, mods[i].so_path);
            debug_write(debug, "\n");
        }
        unsigned long mhash = syms_hash(mods[i].syms, mods[i].sym_n);
        unsigned long manchor = syms_anchor(mods[i].syms, mods[i].sym_n);
        int do_update = 1;
        if (!(mods[i].opts_bits & KLDOPT_RELOAD) &&
            have_boot_id) {
            char sbid[96];
            unsigned long sanchor = 0, shash = 0;
            struct stat sst = {0};
            if (read_stamp_for_path("mod", mods[i].so_path, sbid, sizeof(sbid), &sanchor, &shash, &sst) == 0 &&
                my_streq(sbid, boot_id) && sanchor == manchor && shash == mhash &&
                stamp_covers_so(&sst, mods[i].so_path)) {
                do_update = 0;
                if (debug) {
                    debug_write(debug, "[KLD.SO]: update: stamp match; skipping ");
                    debug_write(debug, mods[i].so_path);
                    debug_write(debug, "\n");
                }
            }
        }
        if (do_update) {
            if (update_so_dynsym_simple(mods[i].so_path, mods[i].syms, mods[i].sym_n, debug) < 0) {
                if (debug) {
                    debug_write(debug, "[KLD.SO]: update: in-place update failed; rebuild fallback unavailable in standalone mode\n");
                }
                rc = -1;
            }
            else if (have_boot_id) write_stamp_for_path("mod", mods[i].so_path, boot_id, manchor, mhash);
        }
    }
    return rc;
}

static int map_interp_elf(const char *interp_path, unsigned long *base_out, unsigned long *entry_out) {
    const unsigned long PAGE = 4096;
    int fd = (int)sys_openat(AT_FDCWD, interp_path, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return -1;

    struct stat st;
    if (sys_fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        sys_close(fd);
        return -1;
    }
    void *file = sys_mmap((void *)0, (unsigned long)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if ((long)file < 0) {
        sys_close(fd);
        return -1;
    }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)file;
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS64) {
        sys_munmap(file, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    Elf64_Phdr *ph = (Elf64_Phdr *)((char *)file + eh->e_phoff);
    unsigned long min_v = ~0UL, max_v = 0;
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        unsigned long sv = align_down((unsigned long)ph[i].p_vaddr, PAGE);
        unsigned long ev = align_up((unsigned long)ph[i].p_vaddr + (unsigned long)ph[i].p_memsz, PAGE);
        if (sv < min_v) min_v = sv;
        if (ev > max_v) max_v = ev;
    }
    if (min_v == ~0UL || max_v <= min_v) {
        sys_munmap(file, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    unsigned long span = max_v - min_v;
    void *reserve = sys_mmap((void *)0, span, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((long)reserve < 0) {
        sys_munmap(file, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }
    unsigned long load_base = (unsigned long)reserve - min_v;

    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        unsigned long voff = (unsigned long)ph[i].p_vaddr & (PAGE - 1);
        unsigned long map_addr = load_base + align_down((unsigned long)ph[i].p_vaddr, PAGE);
        unsigned long map_off = align_down((unsigned long)ph[i].p_offset, PAGE);
        unsigned long file_len = align_up(voff + (unsigned long)ph[i].p_filesz, PAGE);
        int prot = 0;
        if (ph[i].p_flags & PF_R) prot |= PROT_READ;
        if (ph[i].p_flags & PF_W) prot |= PROT_WRITE;
        if (ph[i].p_flags & PF_X) prot |= PROT_EXEC;

        void *m = sys_mmap((void *)map_addr, file_len, prot, MAP_PRIVATE | MAP_FIXED, fd, map_off);
        if ((long)m < 0) {
            sys_munmap(file, (unsigned long)st.st_size);
            sys_close(fd);
            return -1;
        }

        if (ph[i].p_memsz > ph[i].p_filesz) {
            unsigned long bss_start = load_base + (unsigned long)ph[i].p_vaddr + (unsigned long)ph[i].p_filesz;
            unsigned long bss_end = load_base + (unsigned long)ph[i].p_vaddr + (unsigned long)ph[i].p_memsz;
            unsigned long first_full = align_up(bss_start, PAGE);
            if (first_full < bss_end) {
                void *bm = sys_mmap((void *)first_full, align_up(bss_end, PAGE) - first_full, prot,
                                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
                if ((long)bm < 0) {
                    sys_munmap(file, (unsigned long)st.st_size);
                    sys_close(fd);
                    return -1;
                }
            }
            if (bss_start < first_full) {
                my_memset((void *)bss_start, 0, (int)(first_full - bss_start));
            }
        }
    }

    *base_out = load_base;
    *entry_out = load_base + (unsigned long)eh->e_entry;
    sys_munmap(file, (unsigned long)st.st_size);
    sys_close(fd);
    return 0;
}

static void patch_auxv_at_base(void *initial_rsp, unsigned long at_base) {
    unsigned long *p = (unsigned long *)initial_rsp;
    unsigned long argc = *p++;
    p += argc;     /* argv[] */
    p += 1;        /* NULL */
    while (*p) p++; /* envp[] */
    p += 1;         /* NULL */

    Elf64_auxv_t *aux = (Elf64_auxv_t *)p;
    for (; aux->a_type != AT_NULL; aux++) {
        if (aux->a_type == AT_BASE) {
            aux->a_un.a_val = at_base;
            return;
        }
    }
}

__attribute__((noreturn))
static void handoff_to_interp(void *initial_rsp, unsigned long entry) {
    asm volatile(
        "mov %0, %%rsp\n\t"
        "xor %%rbp, %%rbp\n\t"
        "jmp *%1\n\t"
        :
        : "r"(initial_rsp), "r"(entry)
        : "memory");
    __builtin_unreachable();
}

void c_entry(int argc, char **argv) {
    char **envp = &argv[argc + 1];
    int debug = (get_env_value(envp, "KLD_DEBUG") != (char *)0);
    int noexec = (get_env_value(envp, "KLD_NOEXEC") != (char *)0);
    int prep_only = (get_env_value(envp, "KLD_PREP_ONLY") != (char *)0);
    char *prep_mode_raw = get_env_value(envp, "KLD_PREP_MODE");
    int prep_mode = parse_prep_mode_value(prep_mode_raw);
    char *kld_library_path = get_env_value(envp, "KLD_LIBRARY_PATH");
    char *old_ld_library_path = get_env_value(envp, "LD_LIBRARY_PATH");
    int ldpath_cap = 8 * PATH_MAX;
    char *orig_interp = (char *)alloc_rw_or_die(PATH_MAX);
    char *kotbl_ldpath = (char *)alloc_rw_or_die((unsigned long)ldpath_cap);
    char *merged_ldpath = (char *)alloc_rw_or_die((unsigned long)ldpath_cap);

    if (argc < 1 || !argv[0]) die_msg("[KLD.SO]: missing target argv[0]");
    if (prep_mode < 0) die_msg("[KLD.SO]: invalid KLD_PREP_MODE (use: all|load|update)");

    if (read_original_interp(argv[0], orig_interp, PATH_MAX) < 0) {
        die_msg("[KLD.SO]: failed to read original interpreter from target");
    }
    kotbl_ldpath[0] = '\0';
    (void)build_ldpath_from_kotbl(argv[0], kotbl_ldpath, ldpath_cap);
    merged_ldpath[0] = '\0';
    if (kotbl_ldpath[0]) path_list_append_unique(merged_ldpath, ldpath_cap, kotbl_ldpath);
    if (kld_library_path && kld_library_path[0]) {
        const char *p = kld_library_path;
        while (*p) {
            const char *start = p;
            while (*p && *p != ':') p++;
            int n = (int)(p - start);
            path_list_append_unique_n(merged_ldpath, ldpath_cap, start, n);
            if (*p == ':') p++;
        }
    }
    if (old_ld_library_path && old_ld_library_path[0]) {
        const char *p = old_ld_library_path;
        while (*p) {
            const char *start = p;
            while (*p && *p != ':') p++;
            int n = (int)(p - start);
            path_list_append_unique_n(merged_ldpath, ldpath_cap, start, n);
            if (*p == ':') p++;
        }
    }

    debug_write(debug, "[KLD.SO]: target=");
    debug_write(debug, argv[0]);
    debug_write(debug, "\n");
    if (debug && kld_library_path && kld_library_path[0]) {
        debug_write(debug, "[KLD.SO]: KLD_LIBRARY_PATH=");
        debug_write(debug, kld_library_path);
        debug_write(debug, "\n");
    }
    if (debug && prep_mode_raw && prep_mode_raw[0]) {
        debug_write(debug, "[KLD.SO]: KLD_PREP_MODE=");
        debug_write(debug, prep_mode_raw);
        debug_write(debug, "\n");
    }
    if (debug && prep_only) {
        debug_write(debug, "[KLD.SO]: KLD_PREP_ONLY=1\n");
    }
    if (debug) {
        debug_write(debug, "[KLD.SO]: original_interp=");
        debug_write(debug, orig_interp);
        debug_write(debug, "\n");
        if (kotbl_ldpath[0]) {
            debug_write(debug, "[KLD.SO]: kotbl_ldpath=");
            debug_write(debug, kotbl_ldpath);
            debug_write(debug, "\n");
        }
        if (merged_ldpath[0]) {
            debug_write(debug, "[KLD.SO]: merged_ldpath=");
            debug_write(debug, merged_ldpath);
            debug_write(debug, "\n");
        }
        debug_write(debug, "[KLD.SO]: initial_rsp=");
        {
            /* tiny hex print without libc */
            char buf[2 + sizeof(void *) * 2 + 2];
            const char *hex = "0123456789abcdef";
            unsigned long v = (unsigned long)kld_initial_rsp;
            int idx = 0;
            buf[idx++] = '0';
            buf[idx++] = 'x';
            for (int shift = (int)(sizeof(void *) * 8) - 4; shift >= 0; shift -= 4) {
                buf[idx++] = hex[(v >> shift) & 0xf];
            }
            buf[idx++] = '\n';
            buf[idx] = '\0';
            debug_write(debug, buf);
        }
    }

    if (noexec && !prep_only) {
        sys_write(2, "interp:   ", 10);
        sys_write(2, orig_interp, (unsigned long)my_strlen(orig_interp));
        sys_write(2, "\n", 1);
        sys_write(2, "execpath: ", 10);
        sys_write(2, argv[0], (unsigned long)my_strlen(argv[0]));
        sys_write(2, "\n", 1);
        sys_write(2, "ldpath:   ", 10);
        sys_write(2, merged_ldpath[0] ? merged_ldpath : "", (unsigned long)my_strlen(merged_ldpath[0] ? merged_ldpath : ""));
        sys_write(2, "\n", 1);
        sys_exit(0);
    }
    if (kld_runtime_prepare_for_exec(argv[0], debug, prep_mode) < 0) {
        die_msg("[KLD.SO]: runtime preparation failed");
    }
    if (prep_only) {
        sys_write(2, "[KLD.SO]: preparation-only mode complete\n", 41);
        sys_exit(0);
    }
    if (inject_ldpath_env(envp, merged_ldpath, debug) < 0) {
        die_msg("[KLD.SO]: failed to inject LD_LIBRARY_PATH into runtime env");
    }
    unsigned long interp_base = 0, interp_entry = 0;
    if (map_interp_elf(orig_interp, &interp_base, &interp_entry) < 0) {
        die_msg("[KLD.SO]: map_interp_elf failed");
    }
    patch_auxv_at_base(kld_initial_rsp, interp_base);
    handoff_to_interp(kld_initial_rsp, interp_entry);
}
