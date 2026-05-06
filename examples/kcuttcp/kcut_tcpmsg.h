#ifndef KCUT_TCPMSG_H
#define KCUT_TCPMSG_H

#include "kcut.h"
#include <sys/uio.h>
#include "kcut_tcpmsg.kh"

// ukl inspired shortcut interfaces
struct kcut_tcpmsg_thunk_args {
  struct iovec *iov;
  int fd;
};
  
// thunks for stack switching calls
static inline
void * kcut_tcp_recvmsg_thunk(void *targs) {
  struct kcut_tcpmsg_thunk_args *args = (struct kcut_tcpmsg_thunk_args *)targs;
  int fd = args->fd;
  struct iovec *iov = args->iov; 
  return (void *)((intptr_t)kcut_tcp_recvmsg(fd, iov));
}

void * kcut_tcp_sendmsg_thunk(void *targs) {
  struct kcut_tcpmsg_thunk_args *args = (struct kcut_tcpmsg_thunk_args *)targs;
  int fd = args->fd;
  struct iovec *iov = args->iov; 
  return (void *)((intptr_t)kcut_tcp_sendmsg(fd, iov));
}


// read / write compatible interface
static inline
ssize_t kcut_tcp_read(int fd, void *buf, size_t count)
{
  struct iovec iov;
  iov.iov_base = (void *)buf;
  iov.iov_len = count;
  int ret;
  
#ifndef BRACKET_PRIV
  const int KCUT_THRESHOLD = 20;
  static int kcut_cnt = 0;
  if (kcut_cnt) {
#endif
    
#ifdef BRACKET_STACK
    struct kcut_tcpmsg_thunk_args targs = {.fd = fd, .iov = &iov };
    ELEVATE_ME();
    ret = (int)((intptr_t)
		stack_switch_kcall(kcut_tos_offset(),
				   kcut_tcp_recvmsg_thunk, &targs));
    LOWER_ME();
#else
    ELEVATE_ME();
    ret = kcut_tcp_recvmsg(fd, &iov);
    LOWER_ME();
#endif

#ifndef BRACKET_PRIV
    kcut_cnt--;
  } else {
    ret = read(fd, buf, count);
    kcut_cnt = KCUT_THRESHOLD;
  }
#endif
  
  return ret;
}

static inline
ssize_t kcut_tcp_write(int fd, void *buf, size_t count)
{
   struct iovec iov;
  iov.iov_base = (void *)buf;
  iov.iov_len = count;
  int ret;
  
#ifndef BRACKET_PRIV
  const int KCUT_THRESHOLD = 20;
  static int kcut_cnt = 0;
  if (kcut_cnt) {
#endif
    
#ifdef BRACKET_STACK
    struct kcut_tcpmsg_thunk_args targs = {.fd = fd, .iov = &iov };
    ELEVATE_ME();
    ret = (int)((intptr_t)
		stack_switch_kcall(kcut_tos_offset(),
				   kcut_tcp_sendmsg_thunk, &targs));
    LOWER_ME();
#else
    ELEVATE_ME();
    ret = kcut_tcp_sendmsg(fd, &iov);
    LOWER_ME();
#endif

#ifndef BRACKET_PRIV
    kcut_cnt--;
  } else {
    ret = read(fd, buf, count);
    kcut_cnt = KCUT_THRESHOLD;
  }
#endif
  
  return ret;  
}

#endif  // KCUT_TCPMSG_H
