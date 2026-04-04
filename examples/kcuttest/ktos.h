#ifndef __KCUT_TOS_H__
#define __KCUT_TOS_H__
#include <dlfcn.h>
#include <assert.h>

static inline int
kcut_resolve_sym(char *name, void **value)
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

// FIXME: JA make multithreaded safe and consider implications of
//        static cache of ktos_offset
static inline uintptr_t
kcut_tos_offset(void)
{
  // discuss this not sure this is right
  static uintptr_t kcut_tos_offset_val = 0;
  
  if (kcut_tos_offset_val==0) {
    if (!kcut_resolve_sym("cpu_current_top_of_stack",
		     (void **)&kcut_tos_offset_val)) {
      fprintf(stderr,  "failed to resolve cpu_current_top_of_stack\n");
      assert(0);
    }
  }
  return kcut_tos_offset_val;
}

#endif
