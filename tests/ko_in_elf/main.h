#include <stdint.h>
#include <stdio.h>
#include <elf.h>
#include "LINF/sym_all.h"

#define	ENOMEM		12	/* Out of memory */
#define	EFAULT		14	/* Bad address */

#define ___GFP_DMA		0x01u
#define ___GFP_HIGHMEM		0x02u
#define ___GFP_DMA32		0x04u
#define ___GFP_MOVABLE		0x08u
#define ___GFP_RECLAIMABLE	0x10u
#define ___GFP_HIGH		0x20u
#define ___GFP_IO		0x40u
#define ___GFP_FS		0x80u
#define ___GFP_ZERO		0x100u
#define ___GFP_ATOMIC		0x200u
#define ___GFP_DIRECT_RECLAIM	0x400u
#define ___GFP_KSWAPD_RECLAIM	0x800u
#define ___GFP_WRITE		0x1000u
#define ___GFP_NOWARN		0x2000u
#define ___GFP_RETRY_MAYFAIL	0x4000u
#define ___GFP_NOFAIL		0x8000u
#define ___GFP_NORETRY		0x10000u
#define ___GFP_MEMALLOC		0x20000u
#define ___GFP_COMP		0x40000u
#define ___GFP_NOMEMALLOC	0x80000u
#define ___GFP_HARDWALL		0x100000u
#define ___GFP_THISNODE		0x200000u
#define ___GFP_ACCOUNT		0x400000u
#define ___GFP_ZEROTAGS		0x800000u
#define ___GFP_SKIP_KASAN_POISON	0x1000000u

//prevent che
#define MODULE_INIT_IGNORE_MODVERSIONS	1
#define MODULE_INIT_IGNORE_VERMAGIC	2


#define GFP_KERNEL	(___GFP_DIRECT_RECLAIM|___GFP_KSWAPD_RECLAIM | ___GFP_IO | ___GFP_FS)

#define min(a, b) ((a) < (b) ? (a) : (b))

FILE *ko_log_file = NULL;

#define PRINTF(fmt, ...) do { \
    printf(fmt, ##__VA_ARGS__); \
    if (ko_log_file) { \
        fprintf(ko_log_file, fmt, ##__VA_ARGS__); \
        fflush(ko_log_file); \
        fsync(fileno(ko_log_file)); \
    } \
} while(0)

typedef unsigned int gfp_t;
struct load_info {
    const char *name;
    /* pointer to module in temporary copy, freed at end of load_module() */
    void *mod;
    void *hdr;
    unsigned long len;
    void *sechdrs;
    char *secstrings, *strtab;
    unsigned long symoffs, stroffs, init_typeoffs, core_typeoffs;
    struct _ddebug *debug;
    unsigned int num_debug;
    _Bool sig_ok;
    unsigned long mod_kallsyms_init_off;
    struct {
        unsigned int sym, str, mod, vers, info, pcpu;
    } index;
};

// These symbols are provided by the linker (from greeter.o)
extern const uint8_t _binary_greeter_ko_start[];
extern const uint8_t _binary_greeter_ko_end[];
extern const uint8_t _binary_greeter_ko_size;

#ifdef DYNLINK

extern void *vmalloc_noprof(unsigned long size);
extern void vfree(const void *addr);

extern int __cond_resched(void);

// extern unsigned long _copy_from_user(void *to, void *from, unsigned long n);

extern int load_module(struct load_info *info, char *uargs,
		       int flags);

extern int __x64_sys_init_module(void* umod, unsigned long len,
                                 const char *uargs);

// extern int printk(const char *fmt, ...);
#else // DYNLINK
// typedef (*void) (*vmalloc_t)(unsigned long size, gfp_t gfp_mask);
typedef void *(*vmalloc_t)(unsigned long size);
vmalloc_t vmalloc_noprof;

typedef void (*vfree_t)(const void *addr);
vfree_t vfree;


typedef int (*__cond_resched_t)(void);
__cond_resched_t __cond_resched;

typedef unsigned long (*_copy_from_user_t)(void *to, void *from, unsigned long n);
_copy_from_user_t _copy_from_user;

typedef int (*init_module_t)(void* umod, unsigned long len,
                                 const char *uargs);
init_module_t init_module;

#endif // DYNLINK

#define PAGE_SHIFT 12 //just for this host!
#define PAGE_SIZE  (1UL << PAGE_SHIFT)
#define COPY_CHUNK_SIZE (16*PAGE_SIZE)