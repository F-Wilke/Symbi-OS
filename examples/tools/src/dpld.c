#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "LINF/sym_all.h"
#include "L1/stack_switch.h"

#define DECLARE_ALL_FUNCS
#include "app_got.h"
#include "modules.h"

static int module_loaded = 0;
static int verbose       = 0;

extern void symbi_fast_lower(void);
extern unsigned long kallsyms_lookup_name(const char *name);
extern void* vmalloc_noprof(unsigned long size);

void *kallsyms_lookup_name_thunk(void *str) {
  return (void *)kallsyms_lookup_name((const char *)str);
}


void (*set_app_got)(app_got_t* got);

#define VPRINTF(fmt, ...) do {					\
    if (verbose) { fprintf(stderr, fmt, ##__VA_ARGS__); }	\
  } while(0)

#define FATAL(fmt, ...) do {					\
    fprintf(stderr, fmt, ##__VA_ARGS__); \
  } while(0)

static inline int init_module(void* umod, unsigned long len, char* uargs)
{
  int ret;
  ret=syscall(__NR_init_module, umod, len, uargs);
  if (ret == -1) {
    VPRINTF("Error loading module errno=%d\n", errno);
    if (errno == EEXIST) {
      VPRINTF("Already loaded...\n");
    } else {
      assert(0);
    }
  }
  return ret;
}

static inline int
resolve_sym(char *name, void **value)
{
  char *error;
  dlerror();  // clear as per manpage
  *value = dlsym(RTLD_DEFAULT, name);
  error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "%s\n", error);
    return 0;
  }
  return 1;
}

static inline int
force_symres_now()
{
  void *value;
  // force symbol resolution before we elevate  
  // lookup symbols to avoid problems once elevated -- touch symbol tables
  if (!resolve_sym("cpu_current_top_of_stack", &value)) return 0;
  if (!resolve_sym("kallsyms_lookup_name", &value)) return 0;
  if (!resolve_sym("vmalloc_noprof", &value)) return 0;
  if (!resolve_sym("exit", &value)) return 0; //does this go through the GOT?
  return 1;
}

//assume sym_elevate has been called before this function
static int load_ext_module() {
  VPRINTF("starting load_ext_module\n");
  int ret = 0;
  uintptr_t ktos_offset;
   
  if (force_symres_now()==0) {
    VPRINTF("ERROR: failed to resolve symbols needed\n");
    assert(0);
    return 0;
  }
  
  if (!resolve_sym("cpu_current_top_of_stack", (void **)&ktos_offset)) {
    VPRINTF("failed to resolve cpu_current_top_of_stack\n");
    assert(0);
    return 0;
  }
  
  unsigned long pfaddr;
  if (!resolve_sym("asm_exc_page_fault", (void **)&pfaddr)) {
    VPRINTF("failed to resolve asm_exc_page_fault\n");
    assert(0);
    return 0;
  }
  unsigned long dfaddr;
  if (!resolve_sym("asm_exc_double_fault", (void **)&dfaddr)) {
    VPRINTF("failed to resolve asm_exc_double_fault\n");
    assert(0);
    return 0;
  }
  unsigned long gpaddr;
  if (!resolve_sym("asm_exc_general_protection", (void **)&gpaddr)) {
    VPRINTF("failed to resolve asm_exc_general_protection\n");
    assert(0);
    return 0;
  }
  
  
  VPRINTF("starting load_module\n");
  
  LOAD_ALL_MODULES();

  VPRINTF("init_load_module: ret=%d\n", ret);

  asm volatile(".global __load_ext_module_end\n"
	       "__load_ext_module_end:");
  return ret;
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
static inline void touch_bytes(char *start, char *end)
{
  volatile char *ptr;
  volatile char val;
  for (ptr=start; ptr<end; ptr++) (void)(val = *ptr);
}
#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize ("O0")
static inline void touchstack(int numpages)
{
  const int PGSIZE=4096;
  const int n=PGSIZE * numpages;
  volatile char data[n];
  for (int i=0; i<n; i+=4096) { (void)(data[i] = 0xff); }
}
#pragma GCC pop_options


extern char __load_ext_module_end[];
static inline void touchfuncs()
{
  touch_bytes((char *)load_ext_module, __load_ext_module_end);
}


// JA: FIXME: Discuss if app_got has to be in kernel memory
//     and if so free it
static void *
elevated_app_got_init(void *unused)
{
  long rc = 0;
  (void)unused;
  
  set_app_got = (void (*)(app_got_t *))kallsyms_lookup_name("set_app_got");;
  if (!set_app_got) {
    rc = -1;
    goto done;
  }

  app_got_t* app_got = (app_got_t*)vmalloc_noprof(sizeof(app_got_t));
  if (app_got == NULL) {
    rc = -2;
    goto done;
  }
  VPRINTF("allocated app_got at %p\n", app_got); 

  SET_ALL_GOT_ENTRIES();
  VPRINTF("Set GOT entries\n");
  
  set_app_got(app_got);
  VPRINTF("Set app got pointer in extension\n");
 done:
  return (void *)rc;
}


static long
app_got_init(unsigned long ktos_offset)
{
  long rc;
  
  sym_elevate();
  rc = (long)stack_switch_kcall(ktos_offset, elevated_app_got_init, NULL);
  symbi_fast_lower();

  if (rc == -1) {
    FATAL("Failed to resolve set_app_got symbol!\n");
    exit(1);
  }
  if (rc == -2 ){
    FATAL("Failed to allocate app_got");
    exit(1);
  }
  return rc;
}

//resolves a symbol by name, loading the module if necessary
//this is for symbols from the kernel module included in our fat binary
void* dpld_resolver(char* symbol_name) {
  static unsigned long ktos_offset = 0;
  unsigned long addr;

  if (!verbose && getenv("DPLD_DEBUG")) verbose=1;
  
  VPRINTF("%s: Resolving symbol %s\n", __func__, symbol_name);

  if (ktos_offset == 0 && !resolve_sym("cpu_current_top_of_stack", (void **)&ktos_offset)) {
    VPRINTF("failed to resolve cpu_current_top_of_stack\n");
    assert(0);
    return 0;
  }

  if (!module_loaded) {
    long rc;
    // we are responsible for getting pagefault adaptor installed
    //   -- need to be careful that text pages and stack do
    //      not generate faults while we do this
    touchfuncs();
    touchstack(8);       
    rc = load_ext_module();
    if (rc != 1) {
      VPRINTF("Failed to load ext module: %ld\n", rc);
      //      exit(1);
    }
    VPRINTF("Loaded kallsyms module\n");
    
#ifndef NO_APP_GOT_ENTRIES
    rc = app_got_init(ktos_offset);
    assert(rc==0);
#else
    VPRINTF("No GOT entries to set, skipping GOT setup\n");
#endif
    module_loaded = 1;
  }

  sym_elevate();

  addr = (unsigned long)stack_switch_kcall(ktos_offset, kallsyms_lookup_name_thunk, (void *)symbol_name);

  symbi_fast_lower();

  VPRINTF("Resolved symbol %s to address %p\n", symbol_name, (void*)addr);
  
  if (addr == 0) {
    FATAL("Symbol %s not found!\n", symbol_name);
    //exit program! We have a linkage problem
    exit(1);
  }

  return (void*)addr;
}


