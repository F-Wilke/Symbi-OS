#ifndef __KCUT_H__
#define __KCUT_H__

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

#ifndef  BRACKET_PRIV //FW: Ndef -> we want to elevate on init if we have CONSTANT privilege
#define ELEVATE_ME() sym_elevate();
#define LOWER_ME() symbi_fast_lower();  
#else
#define ELEVATE_ME()
#define LOWER_ME() 
#endif

static inline void kcut_init(void)
{
#ifdef EVACUATE
  kcut_evacuate(1);
#endif
  ELEVATE_ME();
}

static inline void kcut_cleanup(void)
{
#ifdef EVACUATE
  kcut_evacuate(0);
#endif
  LOWER_ME();
}

#endif
