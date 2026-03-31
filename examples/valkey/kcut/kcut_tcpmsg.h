#ifndef KCUT_TCPMSG_H
#define KCUT_TCPMSG_H

#include <sys/uio.h>
#include "LINF/sym_all.h"
#include "kcut_tcpmsg.kh"

#ifdef BRACKET_STACK
#include <dlfcn.h>
__thread unsigned long kcut_ktos = 0;

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


static inline void set_kcut_ktos(void)
{
  if (!resolve_sym("cpu_current_top_of_stack", (void **)&kcut_ktos)) {
    printf("failed to resolve cpu_current_top_of_stack\n");
    assert(0);
  }
}
#endif


static inline int kcut_tcp_recvmsg(int fd, void *buf, size_t len) {
    struct iovec iov;
    iov.iov_base = (void *)buf;
    iov.iov_len = len;

#ifdef BRACKET_STACK
    if (!kcut_ktos) set_kcut_ktos();
#endif

    const int KCUT_THRESHOLD = 20;
    static int kcut_cnt = 0;
    int ret;    
    if (kcut_cnt) {
      kcut_tcp_args_t args = { .fd = fd, .iov = &iov };
#ifdef BRACKET_PRIV
#ifdef BRACKET_STACK
      // privilege switching and stack switching
        sym_elevate();
        ret = (int)stack_switch_kcall(kcut_ktos, (void * (*)(void *))kcut_tcp_recvmsg_ker, &args);
        symbi_fast_lower();
#else
      // privilege switch only, stay on user stack
        sym_elevate();
        ret = (int)kcut_tcp_recvmsg_ker(&args);
        symbi_fast_lower();
#endif 
#else // no privilege switch, assume we have already elevated
#ifdef BRACKET_STACK
      // no privilege switch, switch to kernel stack
        ret = (int)stack_switch_kcall(kcut_ktos, (void * (*)(void *))kcut_tcp_recvmsg_ker, &args);
#else
      // no privilege switch, remain on user stack
        ret = (int)kcut_tcp_recvmsg_ker(&args);
#endif
#endif // BRACKET_PRIV

      kcut_cnt--;
    } else {
      ret = read(fd, buf, len);
      kcut_cnt = KCUT_THRESHOLD;
    }

    return ret;
}




static inline int kcut_tcp_sendmsg(int fd, void *buf, size_t len) {
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = len;

#ifdef BRACKET_STACK
    if (!kcut_ktos) set_kcut_ktos();
#endif

    const int KCUT_THRESHOLD = 20;
    static int kcut_cnt = 0;
    int ret;    
    if (kcut_cnt) {
        kcut_tcp_args_t args = { .fd = fd, .iov = &iov };

#ifdef BRACKET_PRIV
#ifdef BRACKET_STACK
      // privilege switching and stack switching
        sym_elevate();
        ret = (int)stack_switch_kcall(kcut_ktos, (void * (*)(void *))kcut_tcp_sendmsg_ker, &args);
        symbi_fast_lower();
#else
      // privilege switch only, stay on user stack
      sym_elevate();
      ret = (int)kcut_tcp_sendmsg_ker(&args);
      symbi_fast_lower();
#endif 
#else // no privilege switch, assume we have already elevated
#ifdef BRACKET_STACK
      // no privilege switch, switch to kernel stack
        ret = (int)stack_switch_kcall(kcut_ktos, (void * (*)(void *))kcut_tcp_sendmsg_ker, &args);
#else
        // no privilege switch, remain on user stack
        ret = (int)kcut_tcp_sendmsg_ker(&args);
#endif
#endif // BRACKET_PRIV

        kcut_cnt--;
    } else {
        ret = write(fd, buf, len);
        kcut_cnt = KCUT_THRESHOLD;
    }
    
    
    return ret;
}

#endif  // KCUT_TCPMSG_H