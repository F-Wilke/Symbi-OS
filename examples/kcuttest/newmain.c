#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <math.h>

#include "L0/sym_lib.h"
#include "LINF/sym_all.h"
#include "L1/stack_switch.h"
#include <dlfcn.h>
#include <assert.h>
#include <inttypes.h>
#include "greeter.kh"
#include "pfadaptor.kh"
#include "evacuate.kh"
#include "efadaptor.kh"

#include <pthread.h>

extern int mmap_stack_test(unsigned operation);

extern void* vmalloc_noprof(unsigned long size);
extern void vfree(void* ptr);

// discuss this not sure this is right
__thread uintptr_t myktos = 0;

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

static inline uintptr_t
ktos()
{
  if (myktos==0) {
    if (!resolve_sym("cpu_current_top_of_stack", (void **)&myktos)) {
      printf("failed to resolve cpu_current_top_of_stack\n");
      assert(0);
    }
  }
  return myktos;
}

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

int stacktouch(void)
{
  const int PGSIZE=4096;
  const int n=PGSIZE * 8;
  volatile char data[n];
  int sum = 0;

  for (int i=0; i<n; i+=4096) { data[i]=0xff; sum += data[i]; }  
  
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


void evacuate(int acquire)
{
  int rc;
  unsigned int cpu;

  assert(getcpu(&cpu, NULL)==0);
  
  if (acquire) {
    SYM_ON_KERN_STACK_DYNSYM_DO(ktos(), 
				rc=acquire_exclusive_cpu(cpu,EVAC_KILL_NICELY));
    printf("acquire_exclusive_cpu: %d\n", rc);
  } else {
    SYM_ON_KERN_STACK_DYNSYM_DO(ktos(), 
				release_exclusive_cpu(cpu));
    printf("release_exclusive_cpu:\n");
  }
  
  assert(rc==0);
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
  thread_arg_t* argptr = arg;
  int mypid = current_pid();
  if (argptr->arg % 100 == 0) {
    printf("\t\t\tthreadfn: in thread with arg=%u pid=%d\n", argptr->arg, mypid);
  }
  stacktouch();


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

#define CLOCK_SOURCE CLOCK_MONOTONIC
#define NSEC_IN_SECOND (1000000000)
#define USEC_IN_SECOND (1000000)
#define NULL_WORK_COUNT (10000) 

struct CLArgs {
  int usflg;
  double usdelay;
  int ssec;
  signed long bloop;
  signed long yieldcnt;
  int evac;
  unsigned num_forks;
  unsigned num_spawns;
  unsigned num_increments;
  unsigned stack_df;
  unsigned do_test_pfasm;
  unsigned do_test_interrupt;
} CLArgsDefaults = {
  .usflg             = 0,
  .usdelay           = 0,
  .ssec              = 1,  
  .bloop             = 1000000000,
  .yieldcnt          = 10,
  .evac              = 1,
  .num_forks         = 1000,
  .num_spawns        = 1000,
  .num_increments    = 1000,
  .stack_df          = 1,
  .do_test_pfasm     = 0,
  .do_test_interrupt = 0
};

sruct CLArgs CLArgs = { 0 };


void processArgs(int argc, char **argv)
{
  int opt;
    
  while ((opt = getopt(argc, argv, "U:sbyefSitpI")) != -1) {
    switch (opt) {
    case 'I': CLArgs.num_increments = CLArgsDefaults.num_increments; break;  
    case 'U':
      char *endptr;
      CLArgs.usdelay = strtod(optarg, &endptr);
      
      if (endptr == optarg || *endptr != '\0') {
	fprintf(stderr, "Error: Invalid double value for -U: %s\n", optarg);
	exit(-1);
      }
      
      CLArgs.usflg = 1;
      break;
    case 'S': CLArgs.num_increments = CLArgsDefaults.num_spawns; break;  
    case 'b': CLArgs.bloop = CLArgsDefaults.bloop; break;
    case 'e':
    default:
      fprintf(stderr, "Usage: %s [-U value]\n", argv[0]);
      exit(-1);
    }
  }
  int carg = argc - optind;

  // cleanup later 
  if (carg > 0)  CLArgs.ssec     = atoi(argv[optind]);
  if (carg > 1)  CLArgs.bloop    = atol(argv[optind+1]);
  if (carg > 2)  CLArgs.yieldcnt = atol(argv[optind+2]);
  if (carg > 3)  CLArgs.evac     = atoi(argv[optind+3]);
  if (carg > 4)  CLArgs.num_forks = atoi(argv[optind+4]);
  if (carg > 5)  CLArgs.num_spawns = atoi(argv[optind+5]);
  if (carg > 6)  CLArgs.num_increments = atoi(argv[optind+6]);
  if (carg > 7)  CLArgs.stack_df = atoi(argv[optind+7]);
  if (carg > 8)  CLArgs.do_test_pfasm = atoi(argv[optind+8]);
  if (carg > 9)  CLArgs.do_test_interrupt = atoi(argv[optind+9]);

}

void userstab_test()
{
  typedef struct timespec ts_t;
  volatile int val  = 0;
  double   delay    = CLArgs.usdelay;
  ts_t     thedelay = { .tv_sec = 0, .tv_nsec = 0 };
  ts_t     ndelay   = { .tv_sec = 0, .tv_nsec = 0 };
  ts_t     nrem     = { .tv_sec = 0, .tv_nsec = 0 };
  if (delay >= 1.0) {
    thedelay.tv_sec = (time_t)delay;
      delay = delay - thedelay.tv_sec;
  }
  thedelay.tv_nsec = delay * (double)NSEC_IN_SECOND;
  while (!val) {
    double ss=0;
    while (!val) {
      ss += sin(3.1444);
    }
    ndelay = thedelay;
    while (nanosleep(&ndelay,&nrem)<0) {
      ndelay = nrem;
    }
  }
}

// external kernel symbol forward declarations
// "native" kernel symbols -- NOT kernel "extension" symbols will be resolved at load time
extern int overflowuid;
extern int __x64_sys_sched_yield(void);
extern int _printk(const char *fmt, ...);

int main(int argc, char **argv) {
  pid_t mypid = getpid();
  volatile void * _printk_ptr;


  processArgs(argc, argv);
  
  _printk_ptr = (void *)_printk;
  
  printf("%d: BASIC KCUT TESTS: BEGIN: ssec=%d bloop=%lu yieldcnt=%lu\n", mypid,
	 CLArgs.ssec, CLArgs.bloop, CLArgs.yieldcnt);

  printf("\t%d: BEFORE ELEVATE SYMBOL RESOLUTION TEST: BEGIN\n", mypid); 
  printf("\t_printk_ptr=%p\n", _printk_ptr);
  printf("\toverflowuid=%p\n", &overflowuid);
  
  intptr_t  pfaddr   = (intptr_t)dlsym(RTLD_DEFAULT, "asm_exc_page_fault");
  intptr_t  dfaddr   = (intptr_t)dlsym(RTLD_DEFAULT, "asm_exc_double_fault");
  printf("\tpfaddr=0x%lx dfaddr=0x%lx\n", pfaddr, dfaddr);
  printf("\t%d: BEFORE ELEVATE SYMBOL RESOLUTION TEST: END\n", mypid);
  
  unsigned long cr3=0xdeadbeefdeadbeef;
  unsigned long df_cnt=0, pf_cnt=0;
  
  if (CLArgs.usflg) { userstab_test();  }
  
  if (CLArgs.evac) evacuate(1);
  
  sym_elevate();
  
  printf("\t\t%d: %lx: PRIVILEGED INSTRUCTION TEST: START\n", mypid, cr3);
  __asm__ __volatile__("movq %%cr3,%0" : "=r"( cr3 ));

  printf("\t\t%d: %lx: PRIVILEGED INSTRUCTION TEST: END: CR3: %lx\n", mypid, cr3, cr3);

  printf("\t\t%d: %lx: STACK TOUCH TEST: START\n", mypid, cr3);
  int ss = stacktouch();
  printf("\t\t%d: %lx: STACK TOUCH TEST: END: ss=%d\n", mypid, cr3, ss);

  printf("\t\t%d: %lx: READ NATIVE KERNEL SYMBOL TEST: START\n", mypid, cr3);
  printf("\t\toverflowuid: %d\n", overflowuid);
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
  
  printf("\t\t%d: %lx: CALL KERNEL EXTENSION SYMBOL TEST 3: START\n", mypid, cr3);
  pf_cnt = pf_adaptor_pf_cnt_get();
  df_cnt = pf_adaptor_df_cnt_get();
  printf("\t\t%d: %lx: CALL KERNEL EXTENSION SYMBOL TEST 3: END: pf_cnt=%ld df_cnt=%ld\n", mypid, cr3, pf_cnt, df_cnt);
  
  printf("\t\t%d: %lx: USER YIELD TEST: START: yielding for %lu times\n", mypid, cr3,
	 CLArgs.yieldcnt);
  for (int i=0; i<CLArgs.yieldcnt; i++) { sched_yield(); }
  printf("\t\t%d: %lx: USER YIELD TEST: END: we are back for yields\n", mypid, cr3);

  printf("\t\t%d: %lx: KERNEL YIELD TEST: START: yielding for %lu times\n", mypid, cr3,
	 CLArgs.yieldcnt);
  while (CLArgs.yieldcnt) {  __x64_sys_sched_yield(); CLArgs.yieldcnt--; }
  printf("\t\t%d: %lx: KERNEL YIELD TEST: END: we are back for yields\n", mypid, cr3);
  
  printf("\t\t%d: %lx: USER SLEEP TEST: START: going to sleep for %d\n", mypid, cr3,
	 CLArgs.ssec);
  if (CLArgs.ssec) sleep(CLArgs.ssec);
  printf("\t\t%d: %lx: USER SLEEP TEST: END: wokeup\n", mypid, cr3);
  
  printf("\t\t%d: %lx: USER BUSY LOOP TEST: START: going into busy loop for %lu\n", mypid, cr3,
	 CLArgs.bloop);
  while (CLArgs.bloop) { CLArgs.bloop--; }
  printf("\t\t%d: %lx: USER BUSY LOOP TEST: END: %lu\n", mypid, cr3, CLArgs.bloop);
  
  printf("\t\t%d: %lx: FORK TEST: forking %u times with stack touches\n", mypid, cr3, CLArgs.num_forks);
  for (unsigned i=0; i<CLArgs.num_forks; i++) {
    pid_t cpid = fork();
    if (cpid==0) {
      if (i % 100 == 0) printf("\t\t%d: %lx: FORK TEST: in child at iteration %u\n", mypid, cr3, i);
      int mypid = current_pid(); //indirect elevate test: current_pid() is a kernel extension symbol that will only work if we are properly elevated in the child after fork
      (void)mypid;
      stacktouch();
      exit(0);
    } else if (cpid<0) {
      printf("\t\t%d: %lx: FORK TEST: fork failed at iteration %u\n", mypid, cr3, i);
      break;
    }
  }
  printf("\t\t%d: %lx: FORK TEST: END\n", mypid , cr3);

  printf("\t\t%d: %lx: PTHREAD TEST: spawning %u times with stack touches\n", mypid, cr3, CLArgs.num_spawns);
  if (CLArgs.num_spawns) {
    pthread_barrier_init(&barrier, NULL, CLArgs.num_spawns);
    
    pthread_t* threads = malloc(sizeof(pthread_t)*CLArgs.num_spawns);

    for (unsigned i=0; i<CLArgs.num_spawns; i++) {
      thread_arg_t* argptr = malloc(sizeof(thread_arg_t));
      argptr->arg = i;
      argptr->num_increments = CLArgs.num_increments;
      int rc = pthread_create(&threads[i], NULL, threadfn, (void *)argptr);
      if (rc) {
        printf("\t\t%d: %lx: PTHREAD TEST: pthread_create failed at iteration %u with rc=%d\n", mypid, cr3, i, rc);
        break;
      }
    }
    //join all threads before exiting
    for (unsigned i=0; i<CLArgs.num_spawns; i++) {
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
  printf("\t\t%d: %lx: PTHREAD TEST: counter value is %u, expected %lu\n", mypid, cr3, counter, (unsigned long)CLArgs.num_spawns*CLArgs.num_increments);

  printf("\t\t%d: %lx: PTHREAD TEST: END\n", mypid, cr3);
  
  printf("\t\t%d: %lx: DOUBLE FAULT TEST: START\n", mypid, cr3);
  
  int ret;
  if (CLArgs.stack_df) {
    ret = mmap_stack_test(CLArgs.stack_df - 1); // pass 1 to cause gen prot fault, pass 0 to cause page fault
  } else {
    printf("\t\t%d: %lx: DOUBLE FAULT TEST: SKIPPED\n", mypid, cr3);
    ret = 0;
  }
  
  printf("\t\t%d: %lx: DOUBLE FAULT TEST: %d END\n", mypid, cr3, ret);
  
  printf("\t\t%d: %lx: IST STACK BEHAVIOUR TEST: START\n", mypid, cr3);
  
  if (CLArgs.do_test_pfasm) {
    test_pfasm(1,1);
    printf("\t\t%d: %lx: IST STACK BEHAVIOUR TEST: Completed kernel + err code scenario\n", mypid, cr3);
    
    test_pfasm(1,0);
    printf("\t\t%d: %lx: IST STACK BEHAVIOUR TEST: Completed kernel + no err code scenario\n", mypid, cr3);
    
    test_pfasm(0,1);
    printf("\t\t%d: %lx: IST STACK BEHAVIOUR TEST: Completed user + err code scenario\n", mypid, cr3);
    
    test_pfasm(0,0);
    printf("\t\t%d: %lx: IST STACK BEHAVIOUR TEST: Completed user + no err code scenario\n", mypid, cr3);
  }
  
  printf("\t\t%d: %lx: IST STACK BEHAVIOUR TEST: END\n", mypid, cr3);
  
  printf("\t\t%d: %lx: E0 INTERRUPT TEST: START\n", mypid, cr3);
  if (CLArgs.do_test_interrupt) {
    __asm__ volatile ("int $0xE0\n");
    printf("\t\t%d: %lx: E0 INTERRUPT TEST: Completed first E0 test\n", mypid, cr3);
  }
  
  printf("\t\t%d: %lx: E0 INTERRUPT TEST: END\n", mypid, cr3);

  sym_lower();

  if (CLArgs.evac) evacuate(0);

  printf("\t%d: ELEVATED TESTS: END\n", mypid);  
  printf("%d: BASIC KCUT TESTS: END\n", mypid);
  return 0;
}
