#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

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

int stacktouch(volatile char *stackspace, int len, int stride)
{
  volatile char *data = stackspace;
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
  int ss;
  volatile char stackspace1[4*PGSIZE];
  volatile char stackspace2[4*PGSIZE];
  
  _printk_ptr = (void *)_printk;
  if (argc > 1) ssec               = atoi(argv[1]);
  if (argc > 2) bloop              = atol(argv[2]);
  if (argc > 3) yieldcnt           = atol(argv[3]);
  if (argc > 4) evac               = atoi(argv[4]);
  if (argc > 5) wait_for_stdin     = atoi(argv[5]);
  if (argc > 6) num_forks          = atoi(argv[6]);
  if (argc > 7) num_spawns         = atoi(argv[7]);
  if (argc > 8) num_increments     = atoi(argv[8]);
  if (argc > 9) stack_df           = atoi(argv[9]);
  if (argc > 10) do_test_interrupt = atoi(argv[10]);
  
  printf("%d: BASIC KCUT TESTS: BEGIN: ssec=%d bloop=%lu yieldcnt=%lu\n", mypid, ssec, bloop, yieldcnt);

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
  
  printf("\t\t%d: %lx: USER YIELD TEST: START: yielding for %lu times\n", mypid, cr3, yieldcnt);
  for (int i=0; i<yieldcnt; i++) { sched_yield(); }
  printf("\t\t%d: %lx: USER YIELD TEST: END: we are back for yields\n", mypid, cr3);

  printf("\t\t%d: %lx: KERNEL YIELD TEST: START: yielding for %lu times\n", mypid, cr3, yieldcnt);
  while (yieldcnt) {  __x64_sys_sched_yield(); yieldcnt--; }
  printf("\t\t%d: %lx: KERNEL YIELD TEST: END: we are back for yields\n", mypid, cr3);
  
  printf("\t\t%d: %lx: USER SLEEP TEST: START: going to sleep for %d\n", mypid, cr3, ssec);
  if (ssec) sleep(ssec);
  printf("\t\t%d: %lx: USER SLEEP TEST: END: wokeup\n", mypid, cr3);
  
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

  if (evac) kcut_evacuate(0);

  printf("\t%d: ELEVATED TESTS: END\n", mypid);  
  printf("%d: BASIC KCUT TESTS: END\n", mypid);
  return 0;
}
