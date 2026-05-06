#ifndef KCUT_TCPMSG_H
#define KCUT_TCPMSG_H

#include "kcut.h"
#include <sys/uio.h>
#include "kcut_tcpmsg.kh"

extern int KCUT_THRESHOLD;

#ifndef SHORTCUT
#define SHORTCUT 1
#endif


#if !SHORTCUT
#include <unistd.h>
extern ssize_t ksys_read(int fd, void *buf, size_t count);
extern ssize_t ksys_write(int fd, const void *buf, size_t count);
#endif


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
#if SHORTCUT
  struct iovec *iov = args->iov; 
  return (void *)((intptr_t)kcut_tcp_recvmsg(fd, iov));
#else
  return (void*)((intptr_t)ksys_read(fd, args->iov->iov_base, args->iov->iov_len));
#endif

}

void * kcut_tcp_sendmsg_thunk(void *targs) {
  struct kcut_tcpmsg_thunk_args *args = (struct kcut_tcpmsg_thunk_args *)targs;
  int fd = args->fd;

#if SHORTCUT
  struct iovec *iov = args->iov; 
  return (void *)((intptr_t)kcut_tcp_sendmsg(fd, iov));
#else
  return (void*)((intptr_t)ksys_write(fd, args->iov->iov_base, args->iov->iov_len));
#endif
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
#if SHORTCUT
    ret = kcut_tcp_recvmsg(fd, &iov);
#else
    ret = (int)((intptr_t)ksys_read(fd, iov.iov_base, iov.iov_len));
#endif
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
#if SHORTCUT
    ret = kcut_tcp_sendmsg(fd, &iov);
#else
    ret = (int)((intptr_t)ksys_write(fd, iov.iov_base, iov.iov_len));
#endif
    LOWER_ME();
#endif

#ifndef BRACKET_PRIV
    kcut_cnt--;
  } else {
    ret = write(fd, buf, count);
    kcut_cnt = KCUT_THRESHOLD;
  }
#endif
  
  return ret;  
}

#endif  // KCUT_TCPMSG_H
