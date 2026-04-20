#ifndef __SIGPROC_H__
#define __SIGPROC_H__

__attribute__((unused)) static void
sigAddTermSignals(sigset_t *mask)
{
  sigaddset(mask, SIGALRM);
  sigaddset(mask, SIGTERM);
  sigaddset(mask, SIGINT);
  sigaddset(mask, SIGHUP);
  sigaddset(mask, SIGQUIT);
  sigaddset(mask, SIGUSR1);
  sigaddset(mask, SIGVTALRM);
  sigaddset(mask, SIGUSR2);
  sigaddset(mask, SIGPIPE);
  sigaddset(mask, SIGIO);
}


typedef struct signalprocessor {
  evntdesc_t  ed;
  sigset_t    mask;
  int         sfd;
} sigproc_t;

extern void sigprocInit(sigproc_t *this, bool iszeroed);
extern void sigprocReset(sigproc_t *this, bool iszeroed);
extern void sigprocRegisterEvents(sigproc_t *this, int epollfd);

#endif 
