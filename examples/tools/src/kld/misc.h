#ifndef __MISC_H__
#define __MISC_H__

// conditional asserts
#ifdef ASSERTS_OFF
#define ASSERT(...)
#else
#define ASSERT(...) assert(__VA_ARGS__)
#endif


#define NYI { fprintf(stderr, "%s: %d: NYI\n", __func__, __LINE__); assert(0); }

#ifdef VERBOSE_CHECKS_OFF
static inline bool verbose(int l) { return 0; }
#define VLPRINT(VL, fmt, ...)
#define VPRINT(fmt, ...)
#else
static inline bool verbose(int l) { return GBLS.verbose >= l; }
#define VLPRINT(VL, fmt, ...)  {					\
    if (verbose(VL)) {							\
	  fprintf(stderr, "%s: " fmt, __func__, __VA_ARGS__);		\
    } }

#define VPRINT(fmt, ...) VLPRINT(1, fmt, __VA_ARGS__)
#endif

// Error print
#define EPRINT(f, fmt, ...)						\
    fprintf(f, "%s: " fmt, __func__, __VA_ARGS__)			


// Error Exit
static inline void EEXIT() {
  exit(EXIT_FAILURE);
}

#endif // __MISC_H__
