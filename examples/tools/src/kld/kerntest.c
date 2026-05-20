#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

extern int overflowuid;
extern int __x64_sys_sched_yield(void);
extern int _printk(const char *fmt, ...);

int
main()
{
  printf("--- Function Addresses ---\n");
  printf("_printk:\t%p\n", (void *)_printk);
  printf("__x64_sys_sched_yield:\t%p\n", (void *)__x64_sys_sched_yield);
  
  printf("--- Data Addresses ---\n");
  printf("overflowuid:\t%p\n", (void*)&overflowuid);

  printf("--- Dynamic Symbol Resolution ---\n");
  intptr_t  pfaddr   = (intptr_t)dlsym(RTLD_DEFAULT, "asm_exc_page_fault");
  intptr_t  dfaddr   = (intptr_t)dlsym(RTLD_DEFAULT, "asm_exc_double_fault");
  printf("pfaddr:\t0x%lx\ndfaddr:\t0x%lx\n", pfaddr, dfaddr);

  return 0;
}
