#include "incs.h"

static evnthdlrrc_t
sigEvent(void *obj, uint32_t evnts, int epollfd)
{
  sigproc_t *this = obj;
  assert(this == &GBLS.sigproc);
  int          fd = this->sfd;
  evnthdlrrc_t rc = EVNT_HDLR_SUCCESS;
  
  VLPRINT(3,"START: sigproc: fd:%d evnts:0x%08x\n", fd, evnts);
  if (evnts & EPOLLIN) {
    struct signalfd_siginfo fdsi;
    ssize_t                 s;
    s = read(fd, &fdsi, sizeof(fdsi));
    assert(s==sizeof(fdsi));
    switch (fdsi.ssi_signo) {
    case SIGALRM:
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
    case SIGKILL:
    case SIGUSR1:
    case SIGVTALRM:
    case SIGUSR2:
    case SIGPIPE:
    case SIGIO:
      // exit yar if any of this signals occur
      VPRINT("exiting on signal event %d (%s)\n",
	     fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
      rc = EVNT_HDLR_EXIT_LOOP;
      break;
    default:
      EPRINT(stderr, "unknown signal event: %d\n", fdsi.ssi_signo);
    }
    evnts = evnts & ~EPOLLIN;
    if (evnts==0) goto done;
  }
  if (evnts & EPOLLHUP) {
    VLPRINT(2,"EPOLLHUP(%x)\n", EPOLLHUP);
    evnts = evnts & ~EPOLLHUP;
    if (evnts==0) goto done;
  }
  if (evnts & EPOLLRDHUP) {
    VLPRINT(2,"EPOLLRDHUP(%x)\n", EPOLLRDHUP);
    evnts = evnts & ~EPOLLRDHUP;
    if (evnts==0) goto done;
  }
  if (evnts & EPOLLERR) {
    VLPRINT(2,"EPOLLERR(%x)\n", EPOLLERR);
    evnts = evnts & ~EPOLLRDHUP;
    if (evnts==0) goto done;
  }
  if (evnts != 0) {
    VLPRINT(2,"unknown events evnts:%x", evnts);
  }
 done:
  VLPRINT(3, "END: sigproc: fd:%d evnts:0x%08x\n", fd, evnts);
  return rc;
}

extern void
sigprocRegisterEvents(sigproc_t *this, int epollfd)
{
  struct epoll_event ev;
  ASSERT(this && epollfd != -1);
  ASSERT(this->sfd != -1 && this->ed.obj == this && this->ed.hdlr == sigEvent);
  ev.data.ptr = &(this->ed);
  ev.events  = EPOLLIN | EPOLLET; // Edge
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->sfd, &ev) == -1 ) {
      perror("epoll_ctl: this->sffd");
      assert(0);
  }    
}

extern void
sigprocReset(sigproc_t *this, bool iszeroed)
{
  if (!iszeroed) bzero(this, sizeof(*this));
  this->sfd = -1;
  this->ed  = (evntdesc_t){ .obj = this, .hdlr=sigEvent }; 
}

extern void
sigprocInit(sigproc_t *this, bool iszeroed)
{
  sigprocReset(this, iszeroed);
  sigemptyset(&(this->mask));
  // block all the signals so that we avoid standard signal handling
  // behavior --> we will use a signal fd to convert them into events
  sigAddTermSignals(&(this->mask));
  
  assert(sigprocmask(SIG_BLOCK, &(this->mask), NULL)!=-1);

  this->sfd = signalfd(-1, &(this->mask), SFD_CLOEXEC|SFD_NONBLOCK);
  assert(this->sfd != -1); 
}
