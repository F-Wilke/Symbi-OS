#ifndef __SIG_H__
#define __SIG_H__

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
  int         sfd;
  sigset_t    mask;
} sigproc_t;

#endif 
