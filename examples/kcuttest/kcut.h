#ifndef __KCUT_H__
#define __KCUT_H__

#include <stdlib.h>
#include <stdio.h>
#include <L0/sym_lib.h>
#include <L1/stack_switch.h>

// misc support and init
#ifdef BRACKET_STACK
#include "ktos.h"
#endif

#ifdef EVACUATE
#include "ktos.h"
#include "evacuate.kh"
#include "evacuate.h"
#endif

#include "greeter.kh"

#ifdef  BRACKET_PRIV
#define ELEVATE_ME() sym_elevate();
#define LOWER_ME() symbi_fast_lower();  
#else
#define ELEVATE_ME()
#define LOWER_ME() 
#endif

extern int KCUT_THRESHOLD;

static inline void kcut_init(void)
{
#ifdef EVACUATE
  kcut_evacuate(1);
#endif
  //get threshold from env variable
  char *env = getenv("KCUT_THRESHOLD");
  if (env) {
    KCUT_THRESHOLD = atoi(env);
  } else {
    KCUT_THRESHOLD = 20; // default value
  }

  printf("KCUT_THRESHOLD set to %d\n", KCUT_THRESHOLD);
  sym_elevate();
  printf("kcut_init: current pid %d\n", current_pid()); //this is also makes sure that the kernel module is loaded because current_pid is resolved by ifunc
  symbi_fast_lower();

#ifndef BRACKET_PRIV
  sym_elevate();
#endif
}

static inline void kcut_cleanup(void)
{
#ifdef EVACUATE
  kcut_evacuate(0);
#endif
#ifndef BRACKET_PRIV
  symbi_fast_lower();
#endif
}

#endif
