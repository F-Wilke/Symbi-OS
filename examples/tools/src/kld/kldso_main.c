#include <stddef.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/stat.h>


#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif
  
#define WIFEXITED_LOCAL(st)   (((st) & 0x7f) == 0)
#define WEXITSTATUS_LOCAL(st) (((st) >> 8) & 0xff)

static int   debug=0;
// some code from copilot (with various model -- mostly GPT 5.3 Codex)

static unsigned long
align_down(unsigned long v, unsigned long a) {
  return v & ~(a - 1);
}

static unsigned long
align_up(unsigned long v, unsigned long a) {
  return (v + a - 1) & ~(a - 1);
}


/* --- 1. Minimal Syscall Wrappers --- */
static inline long sys_write(int fd, const char *buf, unsigned long len) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(SYS_write), "D"(fd), "S"(buf), "d"(len) : "rcx", "r11", "memory");
    return ret;
}

static inline void sys_exit(int code) {
    asm volatile ("syscall" : : "a"(SYS_exit), "D"(code) : "rcx", "r11");
    __builtin_unreachable();
}

static inline long sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(SYS_execve), "D"(filename), "S"(argv), "d"(envp) : "rcx", "r11", "memory");
    return ret;
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

#if 0
static inline long sys_getpid(void) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_getpid)
                  : "rcx", "r11", "memory");
    return ret;
}

#endif

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

static inline long sys_mprotect(void *addr, unsigned long len, int prot) {
    long ret;
    asm volatile ("syscall" : "=a"(ret)
                  : "a"(SYS_mprotect), "D"(addr), "S"(len), "d"(prot)
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

static inline long sys_fork(void) {
  long ret;
  asm volatile ("syscall" : "=a"(ret)
		: "a"(SYS_fork)
		: "rcx", "r11", "memory");
  return ret;
}

static inline long sys_wait4(int pid, int *status, int options,
			     void *rusage) {
  long ret;
  register long r10 __asm__("r10") = (long)rusage;
  asm volatile ("syscall" : "=a"(ret)
		: "a"(SYS_wait4), "D"(pid), "S"(status),
		  "d"(options), "r"(r10)
		: "rcx", "r11", "memory");
  return ret;
}

static inline long sys_pipe2(int fds[2], int flags) {
  long ret;
  asm volatile ("syscall" : "=a"(ret)
		: "a"(SYS_pipe2), "D"(fds), "S"(flags)
		: "rcx", "r11", "memory");
  return ret;
}

static inline long sys_dup2(int oldfd, int newfd) {
  long ret;
  asm volatile ("syscall" : "=a"(ret)
		: "a"(SYS_dup2), "D"(oldfd), "S"(newfd)
		: "rcx", "r11", "memory");
  return ret;
}

/* --- 2. Minimal String Helpers (Since we have no string.h) --- */
int my_strlen(const char *str) {
    const char *s = str;
    while (*s) s++;
    return s - str;
}

// Checks if 'str' starts with 'prefix'. Returns 1 (true) or 0 (false).
int starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) return 0;
    }
    return 1;
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

/* --- 3. Custom getenv Implementation --- */
char *get_env_value(char **envp, const char *key) {
    for (char **e = envp; *e; e++) {
        char *current = *e;
        
        // Check if the current string starts with the key
        if (starts_with(current, key)) {
            // Check if the next character is '=', ensuring exact match
            // e.g., ensure "HOME" doesn't match "HOMEPATH"
            int key_len = my_strlen(key);
            if (current[key_len] == '=') {
                // Return the address of the value (character after '=')
                return &current[key_len + 1];
            }
        }
    }
    return (char *)0; // NULL
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* --- kld so helpers ---- */

static void die_msg(const char *m) {
    sys_write(2, m, (unsigned long)my_strlen(m));
    sys_write(2, "\n", 1);
    sys_exit(1);
}



static int parse_kld_triplet(char *buf, long n,
			     char **out_interp, char **out_exec,
			     char **out_ld)
{
  // expected: interp\0execpath\0ldpath\0
  if (!buf || n <= 0 || !out_interp || !out_exec || !out_ld) return -1;
  
  char *p = buf, *end = buf + n;
  
  // field 1: interp
  char *interp = p;
  while (p < end && *p) p++;
  if (p >= end) return -1;   // missing NUL terminator
  p++;                       // skip NUL
  
  // field 2: execpath
  char *execp = p;
  while (p < end && *p) p++;
  if (p >= end) return -1;   // missing NUL terminator
  p++;                       // skip NUL
  
  // field 3: ldpath
  char *ldp = p;
  while (p < end && *p) p++;
  if (p >= end) return -1;   // missing final NUL terminator
  
  *out_interp = interp;
  *out_exec   = execp;
  *out_ld     = ldp;
  return 0;
}

static int run_kldso_sh(const char *script_path,
			int argc, char **argv, char **envp,
			char *outbuf, unsigned long outcap,
			long *outn) {
  int pfds[2];
  if (sys_pipe2(pfds, O_CLOEXEC) < 0) return -1;
  
  char *child_argv[argc + 2];
  child_argv[0] = (char *)script_path;
  for (int i = 0; i < argc + 1; i++) child_argv[i + 1] = argv[i];
  long pid = sys_fork();
  if (pid < 0) {
    sys_close(pfds[0]); sys_close(pfds[1]);
    return -1;
  }

  if (pid == 0) {
    // child: stdout -> pipe write end (keep stderr unchanged)
    sys_close(pfds[0]);
    
    if (sys_dup2(pfds[1], 1) < 0) sys_exit(126); // stdout only
    
    sys_close(pfds[1]);
    
    sys_execve(script_path, (char *const *)child_argv, (char *const *)envp);
    sys_exit(127); // exec failed
  }
    
  // parent
  sys_close(pfds[1]);
  unsigned long used = 0;
  int overflow = 0;
  char drain[512];
  
  for (;;) {
     if (!overflow && used + 1 < outcap) {
       long r = sys_read(pfds[0], outbuf + used, outcap - used - 1);
      if (r < 0) { sys_close(pfds[0]); return -1; }
      if (r == 0) break;
      used += (unsigned long)r;
      
      // If buffer is now full, switch to drain mode.
      if (used + 1 >= outcap) overflow = 1;
    } else {
      // Drain remaining helper stdout so child won't block/SIGPIPE due to
      // full pipe.
	long r = sys_read(pfds[0], drain, sizeof(drain));
      if (r < 0) { sys_close(pfds[0]); return -1; }
      if (r == 0) break;
      overflow = 1;
    }
  }
  
  if (outcap > 0) {
    if (used >= outcap) used = outcap - 1;
    outbuf[used] = '\0';
  }
  sys_close(pfds[0]);
  
  if (overflow) {
    // helper output too large for parser buffer
    return -1;
  }
  
  int st = 0;
  for (;;) {
    long wr = sys_wait4((int)pid, &st, 0, 0);
    if (wr == pid) break;
    if (wr < 0 && (-wr == EINTR)) continue;
    return -1;
  }
  
  if (!WIFEXITED_LOCAL(st) || WEXITSTATUS_LOCAL(st) != 0) return -1;
      
  if (debug) {
    sys_write(2, "[KLD.SO]: helper stdout captured\n", 33);
  }
  *outn = (long)used;
  return 0;
}

static int patch_auxv_at_base(void *initial_rsp,
			      unsigned long at_base) {
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
            return 0;
        }
    }
    return -1;
}

typedef struct kld_link_map {
    unsigned long l_addr;
    char *l_name;
    void *l_ld;
    struct kld_link_map *l_next;
    struct kld_link_map *l_prev;
} kld_link_map;

typedef struct kld_r_debug {
    int r_version;
    kld_link_map *r_map;
    unsigned long r_brk;
    int r_state;
    unsigned long r_ldbase;
} kld_r_debug;

#define RT_CONSISTENT 0
#define RT_ADD        1
#define RT_DELETE     2

__attribute__((visibility("default")))
kld_r_debug _r_debug;

__attribute__((visibility("default"), noinline))
void _dl_debug_state(void) { }

static int
patch_dt_debug_ptr(void *initial_rsp, kld_r_debug *rd)
{
    unsigned long *p = (unsigned long *)initial_rsp;
    unsigned long argc = *p++;
    p += argc;      /* argv[] */
    p += 1;         /* NULL */
    while (*p) p++; /* envp[] */
    p += 1;         /* NULL */

    Elf64_auxv_t *aux = (Elf64_auxv_t *)p;
    Elf64_Phdr *phdr = NULL;
    unsigned long phnum = 0, phent = 0;

    for (; aux->a_type != AT_NULL; aux++) {
        if (aux->a_type == AT_PHDR)  phdr = (Elf64_Phdr *)aux->a_un.a_val;
        if (aux->a_type == AT_PHNUM) phnum = aux->a_un.a_val;
        if (aux->a_type == AT_PHENT) phent = aux->a_un.a_val;
    }
    if (!phdr || !phnum || phent != sizeof(Elf64_Phdr)) return -1;

    int found_pt_phdr = 0;
    unsigned long load_bias = 0;
    for (unsigned long i = 0; i < phnum; i++) {
        if (phdr[i].p_type == PT_PHDR) {
            load_bias = (unsigned long)phdr - (unsigned long)phdr[i].p_vaddr;
	    found_pt_phdr = 1;
            break;
        }
    }
    if (!found_pt_phdr) {
      load_bias = 0;
    }
    
    Elf64_Dyn *dyn = NULL;
    for (unsigned long i = 0; i < phnum; i++) {
        if (phdr[i].p_type == PT_DYNAMIC) {
            dyn = (Elf64_Dyn *)(load_bias + (unsigned long)phdr[i].p_vaddr);
            break;
        }
    }
    if (!dyn) return -3;

    for (; dyn->d_tag != DT_NULL; dyn++) {
        if (dyn->d_tag == DT_DEBUG) {
            dyn->d_un.d_ptr = (unsigned long)rd;
            return 0;
        }
    }
    return -4;
}

static int
init_gdb_rendezvous(void *initial_rsp, char **argv,
		    const char *interp_path, unsigned long interp_base)
{
    static kld_link_map main_map;
    static kld_link_map interp_map;

    main_map.l_addr = 0;
    main_map.l_name = (argv && argv[0]) ? argv[0] : (char *)"";
    main_map.l_ld   = 0;
    main_map.l_prev = 0;
    main_map.l_next = &interp_map;

    interp_map.l_addr = interp_base;
    interp_map.l_name = (char *)interp_path;
    interp_map.l_ld   = 0;
    interp_map.l_prev = &main_map;
    interp_map.l_next = 0;

    _r_debug.r_version = 1;
    _r_debug.r_map     = &main_map;
    _r_debug.r_brk     = (unsigned long)&_dl_debug_state;
    _r_debug.r_ldbase  = interp_base;
    _r_debug.r_state   = RT_ADD;

    if (patch_dt_debug_ptr(initial_rsp, &_r_debug) < 0) return -1;

    _dl_debug_state();
    _r_debug.r_state = RT_CONSISTENT;
    _dl_debug_state();
    return 0;
}

static int map_interp_elf(const char *interp_path,
			  unsigned long *base_out,
			  unsigned long *entry_out) {
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
    if (my_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
        eh->e_ident[EI_CLASS] != ELFCLASS64 ||
        eh->e_machine != EM_X86_64 ||
        eh->e_type != ET_DYN) {
        sys_munmap(file, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    if (eh->e_phentsize != sizeof(Elf64_Phdr) ||
        eh->e_phoff > (unsigned long)st.st_size ||
        eh->e_phnum > ((unsigned long)st.st_size - eh->e_phoff) / sizeof(Elf64_Phdr)) {
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
    if (sys_munmap(reserve, span) < 0) {
        sys_munmap(file, (unsigned long)st.st_size);
        sys_close(fd);
        return -1;
    }

    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        unsigned long voff = (unsigned long)ph[i].p_vaddr & (PAGE - 1);
        unsigned long map_addr = load_base + align_down((unsigned long)ph[i].p_vaddr, PAGE);
        unsigned long map_off = align_down((unsigned long)ph[i].p_offset, PAGE);
        unsigned long file_len = align_up(voff + (unsigned long)ph[i].p_filesz, PAGE);
        unsigned long seg_start = load_base + align_down((unsigned long)ph[i].p_vaddr, PAGE);
        unsigned long seg_end = load_base + align_up((unsigned long)ph[i].p_vaddr + (unsigned long)ph[i].p_memsz, PAGE);
        int prot = 0;
        if (ph[i].p_flags & PF_R) prot |= PROT_READ;
        if (ph[i].p_flags & PF_W) prot |= PROT_WRITE;
        if (ph[i].p_flags & PF_X) prot |= PROT_EXEC;

        if (seg_end <= seg_start) {
            continue;
        }

        if (ph[i].p_filesz == 0) {
            void *am = sys_mmap((void *)seg_start,
                                seg_end - seg_start,
                                prot,
                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                                -1, 0);
            if ((long)am < 0) {
                sys_munmap(file, (unsigned long)st.st_size);
                sys_close(fd);
                return -1;
            }
            continue;
        }

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
                unsigned long partial_page = align_down(bss_start, PAGE);
                if ((prot & PROT_WRITE) == 0 &&
                    sys_mprotect((void *)partial_page, PAGE, prot | PROT_WRITE) < 0) {
                    sys_munmap(file, (unsigned long)st.st_size);
                    sys_close(fd);
                    return -1;
                }
                my_memset((void *)bss_start, 0, (int)(first_full - bss_start));
                if ((prot & PROT_WRITE) == 0 &&
                    sys_mprotect((void *)partial_page, PAGE, prot) < 0) {
                    sys_munmap(file, (unsigned long)st.st_size);
                    sys_close(fd);
                    return -1;
                }
            }
        }
    }

    *base_out = load_base;
    *entry_out = load_base + (unsigned long)eh->e_entry;
    sys_munmap(file, (unsigned long)st.st_size);
    sys_close(fd);
    return 0;
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

/* ---  Entry Point --- */
extern void *kld_initial_rsp;  // kldso_entry.S defines and sets this

#define BUFSIZE (4096*4)
char stdoutbuf[BUFSIZE]; 

#ifndef KLDD_SH_PATH
    #error KLDD_SH_PATH UNDEFINED
#endif

char **find_NULL_element(char **start)
{
  char **end = start;
  while (*end) end++;
  return end;
}

typedef struct {
  unsigned long a_type;
  unsigned long a_val;
} auxv_word_t;

void
c_entry(int argc, char **argv, char *extraspace,
	unsigned long maxextrabytes)
{
  // we extend the stack so that we can modify envp as needed
  char **envp;
  char *orig_interp=NULL, *exec=NULL, *ldlibpath=NULL;
  char *script_path       = TOSTRING(KLDD_SH_PATH);
  long  n=0, ldlibpathlen = 0;

  if (argc < 1 || argc > 4096) {
    die_msg("Bad argc???");
  }

  if (&extraspace[maxextrabytes] != kld_initial_rsp) {
    die_msg("kld_initial_rsp and extraspace layout error");
  }

  envp = &argv[argc + 1];
  if (get_env_value(envp, "KLD_DEBUG")) debug=1;
  if (debug) {
    sys_write(2, "[KLD.SO]:", 9);
    for (int i=0; i<argc; i++) {
      sys_write(2, " ", 1);
      sys_write(2, argv[i], my_strlen(argv[i]));
    }
    sys_write(2, "\n", 1);
  }

  if (run_kldso_sh(script_path, argc, argv, envp,
		   stdoutbuf, sizeof(stdoutbuf), &n)<0) {
    die_msg("[KLD.SO]: run_kldso_sh failed");
  }

  if (parse_kld_triplet(stdoutbuf, n, &orig_interp,&exec,
			&ldlibpath) < 0) {
    die_msg("[KLD.SO]: parse of kld output failed");
  }

  ldlibpathlen = (ldlibpath) ? my_strlen(ldlibpath) : 0;

  // see if we need to insert LD_LIBRARY_PATH into the envp
  if (ldlibpathlen) {
    long extrabytes =
      sizeof(char *) +                 // new envp pointer
      sizeof("LD_LIBRARY_PATH=")-1 +   // env string prefix
      ldlibpathlen + 1;                // value + null
    extrabytes = roundup(extrabytes, 16); // ensure alignment
    
    if (extrabytes > maxextrabytes) {
      die_msg("[KLD.SO]: insufficient extra stack space");
    }
    
    // locate the frame achor points
    char **initenvp           = envp;
    char * initframestart     = (char *)kld_initial_rsp;
    char * newframestart      = &extraspace[maxextrabytes-extrabytes];
    char * initframeenvpstart = (char *)initenvp;
    char * initframeauxvstart = (char *)(find_NULL_element(initenvp)+1);
    auxv_word_t *aux          = (auxv_word_t *) initframeauxvstart;
    while (aux->a_type != AT_NULL) aux++;
    char * initframestrings   = (char *)(aux+1);
    
    // pt1 includes: argc, argv, NULL (every thing before envp array)
    long framept1bytes = (initframeenvpstart - initframestart);
    // pt2 includes: envp, NULL, auxv, AT_NULL 
    long framept2bytes = (initframestrings  - initframeenvpstart);

    // copy the parts down leaving gap in envp and open space
    // infront of existing strings (that way we don't have to
    // do any pointer updates
    // copy p1 
    my_memcpy(newframestart, initframestart, framept1bytes);
    // we now know where newevp will be
    char **newenvp = (char **)(newframestart + framept1bytes);
    
    // copy pt2 with a gap
    // skip newenvp[0] slot for new LD_LIBRARY_PATH 
    my_memcpy((char *)&(newenvp[1]), initframeenvpstart,
	      framept2bytes);
    // new strings are now located at the end of pt2 in the new frame
    char *newstrings = (char *)(&(newenvp[1])) + framept2bytes;
    newenvp[0] = newstrings;
    
    // write "LD_LIBRARY_PATH=<ldlibpath>\0"
    my_memcpy(newstrings, "LD_LIBRARY_PATH=",
	      sizeof("LD_LIBRARY_PATH=")-1);
    my_memcpy(newstrings + (sizeof("LD_LIBRARY_PATH=")-1), ldlibpath, 
	      ldlibpathlen);
    newstrings[(sizeof("LD_LIBRARY_PATH=")-1) + ldlibpathlen] = '\0';
     
    // if there was an old LD_LIBRARY_PATH, poison key in place
    char *oldldlibvar = get_env_value(&newenvp[1],
				      "LD_LIBRARY_PATH");
    if (oldldlibvar) {
      oldldlibvar -= sizeof("LD_LIBRARY_PATH=")-1;
      my_memcpy(oldldlibvar,"KLD_OLD_LD_PATH=",
		sizeof("LD_LIBRARY_PATH=")-1);
    }
    
    // update where kld_initial_rsp with the new frame start
    kld_initial_rsp = newframestart;
  }
  
  unsigned long interp_base = 0, interp_entry = 0;
  
  if (map_interp_elf(orig_interp, &interp_base, &interp_entry) < 0) {
    die_msg("[KLD.SO]: map_interp_elf failed");
  }

  if (patch_auxv_at_base(kld_initial_rsp, interp_base) < 0) {
    die_msg("[KLD.SO]: AT_BASE not found in auxv");
  }

  if (init_gdb_rendezvous(kld_initial_rsp, argv,
			  orig_interp, interp_base) < 0) {
    if (debug) {
      sys_write(2, "[KLD.SO]: WARNING: GDB rendezvous init failed; continuing\n", 
		58);
    }
  }
  
  handoff_to_interp(kld_initial_rsp, interp_entry);
  
  sys_exit(1);
}
