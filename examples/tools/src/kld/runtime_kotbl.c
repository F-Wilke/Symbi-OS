#include "runtime_kotbl.h"

enum {
  RKLDOPT_NONE    = 0,
  RKLDOPT_SHARED  = 1 << 0,
  RKLDOPT_PERPROC = 1 << 1,
  RKLDOPT_RELOAD  = 1 << 2
};

static int
find_nul(const char *tbl, size_t size, size_t start, size_t *end_out)
{
  size_t i = start;
  while (i < size && tbl[i] != '\0') i++;
  if (i >= size) return -1;
  *end_out = i;
  return 0;
}

int
kld_kotbl_next(const char *tbl, size_t size, size_t *off, kld_kotbl_ent *out)
{
  size_t p = *off;
  size_t e = 0;
  if (!tbl || !off || !out) return -1;
  if (p >= size) return 0;

  if (find_nul(tbl, size, p, &e) < 0) return -1;
  if (e == p) return 0; /* table terminator */
  out->fnm = &tbl[p];
  out->fnm_len = (int)(e - p);
  p = e + 1;
  if (p >= size) return -1;

  if (find_nul(tbl, size, p, &e) < 0) return -1;
  out->modnm = &tbl[p];
  out->modnm_len = (int)(e - p);
  p = e + 1;
  if (p >= size) return -1;

  if (find_nul(tbl, size, p, &e) < 0) return -1;
  out->opts = &tbl[p];
  out->opts_len = (int)(e - p);
  p = e + 1;

  *off = p;
  return 1;
}

static int
opt_token_to_bit(const char *s, int n)
{
  if (n == 6 &&
      s[0] == 'S' && s[1] == 'H' && s[2] == 'A' &&
      s[3] == 'R' && s[4] == 'E' && s[5] == 'D') return RKLDOPT_SHARED;
  if (n == 7 &&
      s[0] == 'P' && s[1] == 'E' && s[2] == 'R' &&
      s[3] == 'P' && s[4] == 'R' && s[5] == 'O' && s[6] == 'C') return RKLDOPT_PERPROC;
  if (n == 6 &&
      s[0] == 'R' && s[1] == 'E' && s[2] == 'L' &&
      s[3] == 'O' && s[4] == 'A' && s[5] == 'D') return RKLDOPT_RELOAD;
  return RKLDOPT_NONE;
}

int
kld_parse_kldopts_bits(const char *opts, int opts_len)
{
  int bits = RKLDOPT_NONE;
  int start = 0;
  if (!opts || opts_len <= 0) return bits;
  for (int i = 0; i <= opts_len; i++) {
    if (i == opts_len || opts[i] == '|') {
      int tok_len = i - start;
      if (tok_len > 0) bits |= opt_token_to_bit(&opts[start], tok_len);
      start = i + 1;
    }
  }
  return bits;
}
