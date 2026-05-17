#include "incs.h"
#include "modinfo.h"

// From Claude Sonnet 4.6  

// Iterate the flat array of null-terminated "key=value" strings that make
// up the .modinfo section and collect the fields kld cares about.
int
kld_read_modinfo(const char *path, int fd, kld_modinfo *mi)
{
  kld_secdata sd;
  int rc = kld_open_elf_secdata(&sd, path, fd, ".modinfo");
  if (rc < 0) return rc;

  mi->name      = NULL;
  mi->kld_vals  = NULL;
  mi->kld_count = 0;
  int kld_cap   = 0;

  const char *p   = (const char *)sd.data;
  const char *end = p + sd.size;

  while (p < end) {
    size_t rem = (size_t)(end - p);
    size_t len = strnlen(p, rem);

    if (len == 0) { p++; continue; } // skip empty / padding bytes

    const char *eq = memchr(p, '=', len);
    if (eq) {
      size_t     keylen = (size_t)(eq - p);
      const char *val   = eq + 1;

      if (keylen == 4 && strncmp(p, "name", 4) == 0) {
        free(mi->name);               // last "name=" wins (should be only one)
        mi->name = strdup(val);
      } else if (keylen == 3 && strncmp(p, "kld", 3) == 0) {
        if (mi->kld_count == kld_cap) {
          kld_cap      = kld_cap ? kld_cap * 2 : 4;
          mi->kld_vals = realloc(mi->kld_vals, (size_t)kld_cap * sizeof(char *));
        }
        mi->kld_vals[mi->kld_count++] = strdup(val);
      }
    }

    p += len + 1; // advance past the null terminator
  }

  kld_close_elf_secdata(&sd, fd);
  return 0;
}

void
kld_free_modinfo(kld_modinfo *mi)
{
  if (!mi) return;
  if (mi->name) free(mi->name);
  mi->name = NULL;
  for (int i = 0; i < mi->kld_count; i++) free(mi->kld_vals[i]);
  if (mi->kld_vals) free(mi->kld_vals);
  mi->kld_vals  = NULL;
  mi->kld_count = 0;
}

#ifdef MAIN

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <module.ko>\n", argv[0]);
    return 1;
  }

  kld_modinfo mi = {0};
  int rc = kld_read_modinfo(argv[1], -1, &mi);
  if (rc < 0) {
    fprintf(stderr, "ERROR: could not read .modinfo from '%s'\n", argv[1]);
    return 1;
  }

  printf("name: %s\n", mi.name ? mi.name : "(none)");
  if (mi.kld_count == 0) {
    printf("kld:  (none)\n");
  } else {
    for (int i = 0; i < mi.kld_count; i++)
      printf("kld:  %s\n", mi.kld_vals[i]);
  }

  kld_free_modinfo(&mi);
  return 0;
}

#endif
