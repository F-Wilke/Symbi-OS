#ifndef RUNTIME_KOTBL_H
#define RUNTIME_KOTBL_H

#include <stddef.h>

typedef struct {
  const char *fnm;
  int         fnm_len;
  const char *modnm;
  int         modnm_len;
  const char *opts;
  int         opts_len;
} kld_kotbl_ent;

/* Returns: 1 on entry parsed, 0 on end-of-table, -1 on malformed table. */
int kld_kotbl_next(const char *tbl, size_t size, size_t *off, kld_kotbl_ent *out);

/* Parse '|'-separated options to bitmask (unknown tokens ignored). */
int kld_parse_kldopts_bits(const char *opts, int opts_len);

#endif
