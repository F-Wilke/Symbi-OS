#include "incs.h"

static off_t
pidSize()
{
  char pidstr[24];
  int pidstrlen;
  off_t n = 0;
  pidstrlen = snprintf(pidstr, sizeof(pidstr), "%" PRIdMAX, (intmax_t)GBLS.pid);
  n = pidstrlen;
  return n;
}

static bool
fs_libkernel_stat(fs_t *this, fs_file_t *file, struct stat *stbuf)
{
  VLPRINT(2, "%s %ld: ", file->name, file->ino);
  stbuf->st_ino = file->ino;
  stbuf->st_mode = S_IFREG | 0444;
  stbuf->st_nlink = 1;
  stbuf->st_size = pidSize();
  VLPRINT(2, "%ld\n", stbuf->st_size);
  return true; 
}

static bool
fs_libkernel_read(fs_t *this, fs_file_t *file, fuse_req_t req, size_t size,
			    off_t off)
{
  off_t  n = pidSize();
  char *buf = malloc(n+1);
  snprintf(buf, n+1, "%" PRIdMAX, (intmax_t)GBLS.pid);

  int rc=fsFuseReplyBufLimited(req, buf, n, off, size);
  if (rc!=0) fprintf(stderr, "fuse_reply_buf failed: %d", rc);
    
  free(buf);
  return true;
}

fs_fileops_t fs_libkernel_ops = {
  .stat    = fs_libkernel_stat,
  .open    = NULL,
  .read    = fs_libkernel_read,
  .write   = NULL,
  .readdir = NULL 
};


extern void
kldfsCreate(fs_t *fs, fs_ino_t rootino)
{
  fs_file_t *item;
  VLPRINT(2, "fs=%p rootino=%ld\n", fs, rootino);
  item = fsCreatefile(fs, rootino, "libkernel.so", NULL, &fs_libkernel_ops);
  assert(item);
}


static bool checkfd(int fd)
{
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    fprintf(stderr, "fstat on %d failed errno=%d\n", fd, errno);
    if (errno == EBADFD) {
      fprintf(stderr, "EBADFD: %d\n", fd);
    }
    return false;
  }
  return true;
}

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

static void
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
sigprocInit(sigproc_t *this, bool iszeroed)
{
  if (!iszeroed) bzero(this, sizeof(*this));
  this->sfd = -1;
  this->ed  = (evntdesc_t){ .obj = this, .hdlr=sigEvent }; 

  sigemptyset(&(this->mask));
  // block all the signals so that we avoid standard signal handling
  // behavior --> we will use a signal fd to convert them into events
  sigAddTermSignals(&(this->mask));
  
  assert(sigprocmask(SIG_BLOCK, &(this->mask), NULL)!=-1);

  this->sfd = signalfd(-1, &(this->mask), SFD_CLOEXEC|SFD_NONBLOCK);
  assert(this->sfd != -1); 
}

#define MAX_EVENTS 1024
// epoll code is based on example from the man page
extern bool
kldfsLoop(fs_t *fs, sigproc_t *sigproc)
{
  bool rc;
  int epollfd;
  // create the kernel event poll object
  {
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1) {
      perror("epoll_create1");
      return false;
    }
  }

  // switch over to using epoll events for signal handling from now on
  sigprocRegisterEvents(sigproc, epollfd);

  fsRegisterEvents(fs, epollfd);
  
  // loop: detect events and dispatch handlers
  for (;;) {
    struct epoll_event events[MAX_EVENTS];
    errno = 0;
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      if (verbose(1)) perror("epoll_wait");
      if (errno == EINTR) {
	// maybe we got a signal we are not handling or something
	// else made us wakeup ...  log it but just keep on going 
	VLPRINT(2, "%s: EINTR: Continuing\n", __func__);
	continue;
      }
      if (errno == EINVAL) {
	// I don't know why this is happening
	// once we added logging
	//  trigger -> run yar, ctl-z, bg, enter
	EPRINT(stderr, "FAIL:errno=%d epollfd=%d ME=%d checkfd=%d\n",
	       errno, epollfd, MAX_EVENTS, checkfd(epollfd));
	continue;
      }
      rc = false;
      EPRINT(stderr, "FAIL:errno=%d epollfd=%d ME=%d checkfd=%d\n",
	     errno, epollfd, MAX_EVENTS, checkfd(epollfd));
      goto done;
    }
    
    for (int n = 0; n < nfds; ++n) {
      evnthdlrrc_t erc;
      evntdesc_t *ed = events[n].data.ptr;
      uint32_t evnts = events[n].events;
      assert(ed);
      VLPRINT(3, "%d/%d: ed:%p (.hdlr=0x%p .obj=Ox%p) evnts:0x%08x\n",
	      n, nfds, ed, ed->hdlr, ed->obj, evnts);
      assert(ed->hdlr);
      // call handler registered for this event source 
      erc = ed->hdlr(ed->obj, evnts, epollfd);
      if (erc == EVNT_HDLR_EXIT_LOOP) {
	VLPRINT(1, "eventhandler returned exiting loop rc"
		" hdlr:%p obj:0x%p evnts:%08x\n", ed->hdlr, ed->obj, evnts);
	rc = true;
	goto done;
      } else if (erc == EVNT_HDLR_FAILED) {
	EPRINT(stderr, "event handler failed hdlr:%p obj:0x%p evnts:%08x\n",
	       ed->hdlr, ed->obj, evnts);
	rc = false;
	goto done;
      }
    }
  }
  
  // Exit logic
 done:
  return rc;
}
