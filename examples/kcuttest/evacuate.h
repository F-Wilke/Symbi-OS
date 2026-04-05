#ifndef __KCUT_EVACUATE_H__
#define __KCUT_EVACUATE_H__
#include <L0/sym_lib.h>
#include <L1/stack_switch.h>

struct kcut_acquire_exclusive_cpu_thunk_args {
  int cpu;
  int killifneeded;
};

static inline void * kcut_acquire_exclusive_cpu_thunk(void *args) {
  struct kcut_acquire_exclusive_cpu_thunk_args *arg = args;
  long cpu = arg->cpu;
  int  kin = arg->killifneeded;
  
  return (void *)acquire_exclusive_cpu(cpu, kin);
}

static inline void * kcut_release_exclusive_cpu_thunk(void *args) {
  long cpu = (long)args;
  release_exclusive_cpu(cpu);
  return NULL;
}

static inline void kcut_evacuate(int acquire)
{
  int rc=0;
  unsigned int cpu;

  assert(getcpu(&cpu, NULL)==0);
  
  if (acquire) {
    struct kcut_acquire_exclusive_cpu_thunk_args args =
      {.cpu = cpu, .killifneeded = EVAC_KILL_NICELY };
    sym_elevate();
    rc = (long)stack_switch_kcall(kcut_tos_offset(),
				  kcut_acquire_exclusive_cpu_thunk, &args);
    symbi_fast_lower();
    fprintf(stderr, "kcut_acquire_exclusive_cpu: %d\n", rc);
  } else {
    sym_elevate();
    rc = (long)stack_switch_kcall(kcut_tos_offset(),
				  kcut_release_exclusive_cpu_thunk,
				  (void *)((long)cpu));
    symbi_fast_lower();
    fprintf(stderr, "kcut_release_exclusive_cpu:\n");
  }
  
  assert(rc==0);
}

#endif
