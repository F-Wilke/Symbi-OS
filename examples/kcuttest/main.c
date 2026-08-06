#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <assert.h>
#include <inttypes.h>
#include "greeter.kh"
#include "evacuate.kh"
#include "efadaptor.kh"
#include "kcut_tcpmsg.h"

#include "ktos.h"
#include "evacuate.h"

#include <pthread.h>

extern int mmap_stack_test(unsigned operation);

extern void* vmalloc_noprof(unsigned long size);
extern void vfree(void* ptr);

static inline unsigned long get_exc_page_fault_addr()
{
  unsigned long val;

  val = (unsigned long)dlsym(RTLD_DEFAULT, "asm_exc_page_fault");

#if 0
  __asm__ volatile (
		    "mov exc_page_fault, %0"
		    : "=r" (val)
		    :
		    : "memory" );
#endif
  return val;
}
const int PGSIZE=4096;

int stacktouch(volatile char *data, int len, int stride)
{
  int sum = 0;

  for (int i=0; i<len; i+=stride) { data[i]=0xff; sum += data[i]; }  
  
  return sum;
}

int k_heaptouch(void)
{
  const int PGSIZE=4096;
  const int n=PGSIZE * 8;
  volatile char *data = vmalloc_noprof(n);
  int sum = 0;

  for (int i=0; i<n; i+=4096) { data[i]=0xff; sum += data[i]; }  
  
  vfree((void*)data);
  
  return sum;
}

/* user_add — non-static so -rdynamic exports it for dlsym(RTLD_DEFAULT, "user_add"). */
int user_add(int a, int b)
{
  return a + b;
}

/* user_counter — non-static data symbol referenced from kernel via __user_data__. */
int user_counter = 0;

int heaptouch(void)
{
  const int PGSIZE=4096;
  const int n=PGSIZE * 8;
  volatile char *data = malloc(n);
  int sum = 0;

  for (int i=0; i<n; i+=4096) { data[i]=0xff; sum += data[i]; }  
  
  free((void*)data);
  
  return sum;
}

unsigned int counter = 0;
pthread_barrier_t barrier;
//lock for counter
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  unsigned int arg;
  unsigned int num_increments;
} thread_arg_t;


void *threadfn(void *arg)
{
  volatile char stkspace[2*PGSIZE];
  thread_arg_t* argptr = arg;
  int mypid = current_pid();
  if (argptr->arg % 100 == 0) {
    printf("\t\t\tthreadfn: in thread with arg=%u pid=%d\n", argptr->arg, mypid);
  }
  stacktouch(stkspace, sizeof(stkspace), PGSIZE);


  pthread_barrier_wait(&barrier);

  for (unsigned int i=0; i<argptr->num_increments; i++) {
    pthread_mutex_lock(&counter_lock);
    counter++;
    pthread_mutex_unlock(&counter_lock);

    heaptouch();
    k_heaptouch();
  }
  
  
  pthread_barrier_wait(&barrier);
  
  free(argptr);
  return NULL;
}

// external kernel symbol forward declarations
// "native" kernel symbols -- NOT kernel "extension" symbols will be resolved at load time
extern int overflowuid;
extern int __x64_sys_sched_yield(void);
extern int _printk(const char *fmt, ...);
extern int __cond_resched(void);

/*
 * Preemption mode helpers.
 *
 * With CONFIG_PREEMPT_DYNAMIC the kernel exposes the current preemption model
 * via debugfs.  Reading /sys/kernel/debug/sched/preempt returns a string like:
 *   (none) voluntary full
 * where the active mode is wrapped in parentheses.  Writing "none",
 * "voluntary", or "full" to the same file switches the mode immediately.
 *
 * These helpers let the test detect and temporarily override the mode so the
 * involuntary preemption test can enable full preemption, run, then restore
 * the original setting.  Both read and write are ordinary syscalls and work
 * correctly from elevated (CPL0) mode.
 *
 * If debugfs is not mounted or CONFIG_PREEMPT_DYNAMIC is not enabled the
 * helpers return -1 and the test skips or reports "n/a" gracefully.
 */
#define PREEMPT_SYSFS "/sys/kernel/debug/sched/preempt"

static int preempt_mode_get(char *buf, int len)
{
  char raw[64] = {0};
  int fd = open(PREEMPT_SYSFS, O_RDONLY);
  if (fd < 0) return -1;
  int n = read(fd, raw, sizeof(raw) - 1);
  close(fd);
  if (n <= 0) return -1;
  /* The active mode is wrapped in parens: e.g. "(none) voluntary full" */
  char *s = strchr(raw, '(');
  char *e = s ? strchr(s, ')') : NULL;
  if (!s || !e) return -1;
  int toklen = (int)(e - s - 1);
  if (toklen >= len) toklen = len - 1;
  strncpy(buf, s + 1, toklen);
  buf[toklen] = '\0';
  return 0;
}

static int preempt_mode_set(const char *mode)
{
  int fd = open(PREEMPT_SYSFS, O_WRONLY);
  if (fd < 0) return -1;
  int n = write(fd, mode, strlen(mode));
  close(fd);
  return (n > 0) ? 0 : -1;
}

int main(int argc, char **argv) {
  pid_t mypid = getpid();
  int ssec = 0;
  volatile signed long bloop=0;
  signed long yieldcnt=0;
  int evac=0;
  int wait_for_stdin = 0;
  volatile void * _printk_ptr;
  unsigned num_forks = 0, num_spawns = 0, num_increments = 0;
  unsigned stack_df = 0;
  unsigned do_test_interrupt = 0; 
  unsigned preemptcnt = 0;        /* argv[11]: busy-loop count for involuntary preemption test */
  int force_preempt_full = 0;     /* argv[12]: 1 = auto-enable full preemption and restore after */
  int ss;
  volatile char stackspace1[4*PGSIZE];
  volatile char stackspace2[4*PGSIZE];
  
  _printk_ptr = (void *)_printk;
  if (argc > 1)  ssec               = atoi(argv[1]);
  if (argc > 2)  bloop              = atol(argv[2]);
  if (argc > 3)  yieldcnt           = atol(argv[3]);
  if (argc > 4)  evac               = atoi(argv[4]);
  if (argc > 5)  wait_for_stdin     = atoi(argv[5]);
  if (argc > 6)  num_forks          = atoi(argv[6]);
  if (argc > 7)  num_spawns         = atoi(argv[7]);
  if (argc > 8)  num_increments     = atoi(argv[8]);
  if (argc > 9)  stack_df           = atoi(argv[9]);
  if (argc > 10) do_test_interrupt  = atoi(argv[10]);
  if (argc > 11) preemptcnt         = atoi(argv[11]);
  if (argc > 12) force_preempt_full = atoi(argv[12]);

  /*
   * Preflight checks — fail fast with clear instructions before doing
   * anything that requires elevated privileges or specific kernel config.
   */

  /* 1. Must run as root: kcuttest calls sym_elevate() which requires CAP_SYS_ADMIN */
  if (getuid() != 0) {
    fprintf(stderr, "%d: ERROR: kcuttest must be run as root (uid=0, current uid=%d)\n",
            mypid, getuid());
    fprintf(stderr, "%d:        Re-run with: sudo %s ...\n", mypid, argv[0]);
    return 1;
  }

  /* 2. If force_preempt_full is requested, debugfs must be mounted and the
   *    preempt control file must be accessible.  Without this the involuntary
   *    preemption test cannot enable full preemption and will silently not test
   *    what it claims to test. */
  if (force_preempt_full) {
    struct stat st;
    /* Check that debugfs is mounted by verifying the sched directory exists */
    if (stat("/sys/kernel/debug/sched", &st) != 0 || !S_ISDIR(st.st_mode)) {
      fprintf(stderr, "%d: ERROR: debugfs does not appear to be mounted at /sys/kernel/debug\n",
              mypid);
      fprintf(stderr, "%d:        Mount it with:\n", mypid);
      fprintf(stderr, "%d:          mount -t debugfs none /sys/kernel/debug\n", mypid);
      fprintf(stderr, "%d:        Or add to /etc/fstab:\n", mypid);
      fprintf(stderr, "%d:          debugfs  /sys/kernel/debug  debugfs  defaults  0 0\n", mypid);
      return 1;
    }
    /* Check that the preempt control file itself is readable and writable */
    if (access(PREEMPT_SYSFS, R_OK | W_OK) != 0) {
      fprintf(stderr, "%d: ERROR: cannot access %s\n", mypid, PREEMPT_SYSFS);
      fprintf(stderr, "%d:        This file requires CONFIG_PREEMPT_DYNAMIC=y in the kernel.\n",
              mypid);
      fprintf(stderr, "%d:        Check your kernel config: grep PREEMPT_DYNAMIC /boot/config-$(uname -r)\n",
              mypid);
      fprintf(stderr, "%d:        If CONFIG_PREEMPT_DYNAMIC is enabled but the file is missing,\n",
              mypid);
      fprintf(stderr, "%d:        ensure debugfs is mounted (see above) and re-run as root.\n",
              mypid);
      return 1;
    }
  }

  printf("%d: BASIC KCUT TESTS: BEGIN:\n"
	 "  ssec=%d bloop=%ld yieldcnt=%ld evac=%d wait_for_stdin=%d\n"
	 "  num_forks=%u num_spawns=%u num_increments=%u stack_df=%u\n"
	 "  do_test_interrupt=%u preemptcnt=%u force_preempt_full=%d\n",
	 mypid,
	 ssec, bloop, yieldcnt, evac, wait_for_stdin,
	 num_forks, num_spawns, num_increments, stack_df,
	 do_test_interrupt, preemptcnt, force_preempt_full);

  printf("\t%d: BEFORE ELEVATE SYMBOL RESOLUTION TEST: BEGIN\n", mypid); 
  printf("\t_printk_ptr=%p\n", _printk_ptr);
  void *sym = dlsym(RTLD_DEFAULT, "overflowuid");
  printf("\tdlsym:overflowuid=%p GOT:overflowuid=%p\n", sym, &overflowuid);
  
  intptr_t  pfaddr   = (intptr_t)dlsym(RTLD_DEFAULT, "asm_exc_page_fault");
  intptr_t  dfaddr   = (intptr_t)dlsym(RTLD_DEFAULT, "asm_exc_double_fault");
  printf("\tpfaddr=0x%lx dfaddr=0x%lx\n", pfaddr, dfaddr);
  printf("\t%d: BEFORE ELEVATE SYMBOL RESOLUTION TEST: END\n", mypid);

  unsigned long cr3=0xdeadbeefdeadbeef;
  
  if (evac) {
    printf("\t%d: BEFORE EVACUATE\n", mypid);
    kcut_evacuate(1);
    printf("\t%d: AFTER EVACUATE\n", mypid);
  }
  
  if (wait_for_stdin) {
    printf("\n%d: Press enter to continue\n", mypid);
    (void)getchar();
  }

  /*
   * Detect preemption mode and optionally switch to full before elevating.
   * The mode is saved and restored after the elevated section so the test
   * is self-contained.  Requires root and debugfs mounted at /sys/kernel/debug.
   */
  char saved_preempt_mode[16] = {0};
  int  preempt_dynamic_available = (preempt_mode_get(saved_preempt_mode,
                                     sizeof(saved_preempt_mode)) == 0);
  int  preempt_was_full = preempt_dynamic_available &&
                          (strcmp(saved_preempt_mode, "full") == 0);

  printf("\t%d: PREEMPT MODE: current=%s (CONFIG_PREEMPT_DYNAMIC: %s)\n",
         mypid,
         preempt_dynamic_available ? saved_preempt_mode : "n/a",
         preempt_dynamic_available ? "yes" : "no");

  if (preemptcnt && force_preempt_full &&
      preempt_dynamic_available && !preempt_was_full) {
    if (preempt_mode_set("full") == 0)
      printf("\t%d: PREEMPT MODE: switched to full for involuntary preemption test\n", mypid);
    else
      printf("\t%d: PREEMPT MODE: WARNING: failed to set full (need root + debugfs mounted)\n", mypid);
  }

  sym_elevate();
  
  printf("\t\t%d: %lx: PRIVILEGED INSTRUCTION TEST: START\n", mypid, cr3);
  __asm__ __volatile__("movq %%cr3,%0" : "=r"( cr3 ));

  printf("\t\t%d: %lx: PRIVILEGED INSTRUCTION TEST: END: CR3: %lx\n", mypid, cr3, cr3);

  printf("\t\t%d: %lx: STACK TOUCH TEST: START\n", mypid, cr3);
  ss = stacktouch(stackspace1, sizeof(stackspace1), PGSIZE);
  printf("\t\t%d: %lx: STACK TOUCH TEST: END: ss=%d\n", mypid, cr3, ss);


  printf("\t\t%d: %lx: IN-KERNEL STACK TOUCH TEST: START\n", mypid, cr3);
  ss = greeter_k_stacktouch(stackspace2, sizeof(stackspace2), PGSIZE);
  printf("\t\t%d: %lx: IN-KERNEL STACK TOUCH TEST: END: ss=%d\n", mypid, cr3, ss);

  printf("\t\t%d: %lx: READ NATIVE KERNEL SYMBOL TEST: START\n", mypid, cr3);
  printf("\t\t%d: %lx: overflowuid: %d\n", mypid, cr3, overflowuid);
  printf("\t\t%d: %lx: READ NATIVE KERNEL SYMBOL TEST: END\n", mypid, cr3);

  printf("\t\t%d: %lx: CALL NATIVE KERNEL SYMBOL TEST: START\n", mypid, cr3);
  _printk("\t\t** kprint test from pid=%d cr3=%lx\n", mypid, cr3);
  printf("\t\t%d: %lx: CALL NATIVE KERNEL SYMBOL TEST: END\n", mypid, cr3);
  
  printf("\t\t%d: %lx: CALL KERNEL EXTENSION SYMBOL TEST 1: START\n", mypid, cr3);
  int sum = kernel_add(3, 4);
  printf("\t\t%d: %lx: CALL KERNEL EXTENSION SYMBOL TEST 1: END: kernel_add(3, 4) = %d\n", mypid, cr3, sum);
    
  printf("\t\t%d: %lx: CALL KERNEL EXTENSION SYMBOL TEST 2: START\n", mypid, cr3);
  int pid = current_pid();
  printf("\t\t%d: %lx: CALL KERNEL EXTENSION SYMBOL TEST 2: END:  current_pid() = %d\n", mypid, cr3, pid);

  printf("\t\t%d: %lx: BACK-LINKAGE TEST: START\n", mypid, cr3);
  {
    int bla_sum = kernel_user_add(10, 32);
    printf("\t\t%d: %lx: BACK-LINKAGE TEST: kernel_user_add(10, 32) = %d (expected 42)\n",
           mypid, cr3, bla_sum);
    if (bla_sum != 42)
      printf("\t\t%d: %lx: BACK-LINKAGE TEST: FAIL — got %d, expected 42\n",
             mypid, cr3, bla_sum);
    else
      printf("\t\t%d: %lx: BACK-LINKAGE TEST: PASS\n", mypid, cr3);
  }
  printf("\t\t%d: %lx: BACK-LINKAGE TEST: END\n", mypid, cr3);

  printf("\t\t%d: %lx: BACK-LINKAGE DATA TEST: START\n", mypid, cr3);
  {
    user_counter = 5;
    int before = kernel_user_counter_inc(3);
    printf("\t\t%d: %lx: BACK-LINKAGE DATA TEST: before=%d user_counter=%d (expected 5, 8)\n",
           mypid, cr3, before, user_counter);
    if (before != 5 || user_counter != 8)
      printf("\t\t%d: %lx: BACK-LINKAGE DATA TEST: FAIL\n", mypid, cr3);
    else
      printf("\t\t%d: %lx: BACK-LINKAGE DATA TEST: PASS\n", mypid, cr3);
  }
  printf("\t\t%d: %lx: BACK-LINKAGE DATA TEST: END\n", mypid, cr3);

  /* ------------------------------------------------------------------ */
  /* Voluntary context switch tests                                       */
  /* ------------------------------------------------------------------ */

  printf("\t\t%d: %lx: USER YIELD TEST: START: yielding for %lu times\n", mypid, cr3, yieldcnt);
  for (int i=0; i<yieldcnt; i++) { sched_yield(); }
  printf("\t\t%d: %lx: USER YIELD TEST: END: we are back for yields\n", mypid, cr3);

  printf("\t\t%d: %lx: KERNEL YIELD TEST: START: yielding for %lu times\n", mypid, cr3, yieldcnt);
  {
    signed long cnt = yieldcnt;
    while (cnt) { __x64_sys_sched_yield(); cnt--; }
  }
  printf("\t\t%d: %lx: KERNEL YIELD TEST: END: we are back for yields\n", mypid, cr3);

  /*
   * COND_RESCHED TEST
   *
   * Exercises the __cond_resched() → __schedule(SM_PREEMPT) path.  This is
   * the path taken by in-kernel latency reduction points (cond_resched()),
   * distinct from the sched_yield() syscall path tested above.
   *
   * With CONFIG_PREEMPT_DYNAMIC and mode "none", __cond_resched() is a no-op
   * (cond_resched static call points to __static_call_return0).  With mode
   * "voluntary" or higher it actually schedules.  The nvcsw delta reflects
   * how many times the scheduler actually ran another task; it may be less
   * than yieldcnt if no other runnable task exists, which is not a failure.
   */
  printf("\t\t%d: %lx: COND_RESCHED TEST: START: __cond_resched for %lu times\n", mypid, cr3, yieldcnt);
  {
    unsigned long nvcsw_before = kernel_get_nvcsw();
    signed long cnt = yieldcnt;
    while (cnt) { __cond_resched(); cnt--; }
    unsigned long nvcsw_after  = kernel_get_nvcsw();
    unsigned long delta = nvcsw_after - nvcsw_before;
    printf("\t\t%d: %lx: COND_RESCHED TEST: END: nvcsw_delta=%lu\n", mypid, cr3, delta);
    printf("\t\t%d: %lx: COND_RESCHED TEST: PASS (survived %lu cond_resched calls)\n",
           mypid, cr3, yieldcnt);
  }

  printf("\t\t%d: %lx: USER SLEEP TEST: START: going to sleep for %d\n", mypid, cr3, ssec);
  if (ssec) sleep(ssec);
  printf("\t\t%d: %lx: USER SLEEP TEST: END: wokeup\n", mypid, cr3);

  /* ------------------------------------------------------------------ */
  /* Involuntary preemption test                                          */
  /*                                                                      */
  /* Runs a tight busy loop and checks whether the elevated task was      */
  /* preempted by the scheduler (nivcsw delta > 0).  For preemption to   */
  /* actually occur two conditions must hold:                             */
  /*   1. Full preemption must be enabled (preempt=full or               */
  /*      force_preempt_full=1 arg, which switches the mode via debugfs  */
  /*      before elevation and restores it after).                        */
  /*   2. There must be scheduling pressure (other runnable tasks).       */
  /*      Run with BGWRK=N ./kcuttest.sh to create background workers.   */
  /*                                                                      */
  /* PASS = the elevated task survived preemption and resumed correctly.  */
  /* INFO = reports the observed nivcsw delta as evidence.                */
  /* ------------------------------------------------------------------ */
  printf("\t\t%d: %lx: INVOLUNTARY PREEMPTION TEST: START: iters=%u preempt=%s\n",
         mypid, cr3, preemptcnt,
         !preempt_dynamic_available  ? "unknown (no CONFIG_PREEMPT_DYNAMIC?)" :
         preempt_was_full            ? "full (was already full)"               :
         force_preempt_full          ? "full (switched for this test)"         :
                                       saved_preempt_mode);
  if (preemptcnt) {
    unsigned long nivcsw_before = kernel_get_nivcsw();
    volatile unsigned long cnt = preemptcnt;
    while (cnt) { cnt--; }
    unsigned long nivcsw_after  = kernel_get_nivcsw();
    unsigned long delta = nivcsw_after - nivcsw_before;
    if (delta > 0)
      printf("\t\t%d: %lx: INVOLUNTARY PREEMPTION TEST: PASS: "
             "preempted %lu times and resumed correctly\n",
             mypid, cr3, delta);
    else
      printf("\t\t%d: %lx: INVOLUNTARY PREEMPTION TEST: INFO: "
             "no involuntary preemption observed "
             "(need preempt=full + scheduling pressure)\n",
             mypid, cr3);
  } else {
    printf("\t\t%d: %lx: INVOLUNTARY PREEMPTION TEST: SKIPPED "
           "(pass preempt_iters as argv[11] to enable)\n",
           mypid, cr3);
  }

  printf("\t\t%d: %lx: USER BUSY LOOP TEST: START: going into busy loop for %lu\n", mypid, cr3, bloop);
  while (bloop) { bloop--; }
  printf("\t\t%d: %lx: USER BUSY LOOP TEST: END: %lu\n", mypid, cr3, bloop);
  
  printf("\t\t%d: %lx: FORK TEST: forking %u times with stack touches\n", mypid, cr3, num_forks);
  for (unsigned i=0; i<num_forks; i++) {
    pid_t cpid = fork();
    if (cpid==0) {
      if (i % 100 == 0) printf("\t\t%d: %lx: FORK TEST: in child at iteration %u\n", mypid, cr3, i);
      int mypid = current_pid(); //indirect elevate test: current_pid() is a kernel extension symbol that will only work if we are properly elevated in the child after fork
      (void)mypid;
      stacktouch(stackspace1, sizeof(stackspace1), PGSIZE);
      exit(0);
    } else if (cpid<0) {
      printf("\t\t%d: %lx: FORK TEST: fork failed at iteration %u\n", mypid, cr3, i);
      break;
    }
  }
  printf("\t\t%d: %lx: FORK TEST: END\n", mypid , cr3);

  printf("\t\t%d: %lx: PTHREAD TEST: spawning %u times with stack touches\n", mypid, cr3, num_spawns);
  if (num_spawns) {
    pthread_barrier_init(&barrier, NULL, num_spawns);
    
    pthread_t* threads = malloc(sizeof(pthread_t)*num_spawns);

    for (unsigned i=0; i<num_spawns; i++) {
      thread_arg_t* argptr = malloc(sizeof(thread_arg_t));
      argptr->arg = i;
      argptr->num_increments = num_increments;
      int rc = pthread_create(&threads[i], NULL, threadfn, (void *)argptr);
      if (rc) {
        printf("\t\t%d: %lx: PTHREAD TEST: pthread_create failed at iteration %u with rc=%d\n", mypid, cr3, i, rc);
        break;
      }
    }
    //join all threads before exiting
    for (unsigned i=0; i<num_spawns; i++) {
      int rc = pthread_join(threads[i], NULL);
      if (rc) {
        printf("\t\t%d: %lx: PTHREAD TEST: pthread_join failed at iteration %u with rc=%d\n", mypid, cr3, i, rc);
        break;
      }
    }
    pthread_barrier_destroy(&barrier);

    free(threads);  
  }

  //check counter value to make sure threads ran properly
  printf("\t\t%d: %lx: PTHREAD TEST: counter value is %u, expected %lu\n", mypid, cr3, counter, (unsigned long)num_spawns*num_increments);

  printf("\t\t%d: %lx: PTHREAD TEST: END\n", mypid, cr3);
  
  printf("\t\t%d: %lx: DOUBLE FAULT TEST: START\n", mypid, cr3);
  
  int ret;
  if (stack_df) {
    ret = mmap_stack_test(stack_df - 1); // pass 1 to cause gen prot fault, pass 0 to cause page fault
  } else {
    printf("\t\t%d: %lx: DOUBLE FAULT TEST: SKIPPED\n", mypid, cr3);
    ret = 0;
  }
  
  printf("\t\t%d: %lx: DOUBLE FAULT TEST: %d END\n", mypid, cr3, ret);
  
  
  printf("\t\t%d: %lx: E0 INTERRUPT TEST: START\n", mypid, cr3);
  if (do_test_interrupt) {
    __asm__ volatile ("int $0xE0\n");
    printf("\t\t%d: %lx: E0 INTERRUPT TEST: Completed first E0 test\n", mypid, cr3);
  }
  
  printf("\t\t%d: %lx: E0 INTERRUPT TEST: END\n", mypid, cr3);

  printf("\t\t%d: %lx: kcuttcp reference: BEGIN\n", mypid, cr3);
  printf("kcut_tcp_recvmsg: %p\n", (void *)&kcut_tcp_recvmsg);
  printf("\t\t%d: %lx: kcuttcp reference: end\n", mypid, cr3);

  symbi_fast_lower();

  /* Restore preemption mode if we changed it */
  if (preemptcnt && force_preempt_full &&
      preempt_dynamic_available && !preempt_was_full) {
    if (preempt_mode_set(saved_preempt_mode) == 0)
      printf("\t%d: PREEMPT MODE: restored to %s\n", mypid, saved_preempt_mode);
    else
      printf("\t%d: PREEMPT MODE: WARNING: failed to restore to %s\n", mypid, saved_preempt_mode);
  }

  if (evac) kcut_evacuate(0);

  printf("\t%d: ELEVATED TESTS: END\n", mypid);  
  printf("%d: BASIC KCUT TESTS: END\n", mypid);
  return 0;
}
