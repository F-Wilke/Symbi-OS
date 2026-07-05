#include "module_patch.h"
#include <elf.h>

static size_t
sp_strlen(const char *s)
{
  size_t n = 0;
  while (s && s[n]) n++;
  return n;
}

static int
sp_streq(const char *a, const char *b)
{
  size_t i = 0;
  if (!a || !b) return 0;
  while (a[i] && b[i]) {
    if (a[i] != b[i]) return 0;
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static int
sp_memcmp(const void *va, const void *vb, size_t n)
{
  const unsigned char *a = (const unsigned char *)va;
  const unsigned char *b = (const unsigned char *)vb;
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) return (int)a[i] - (int)b[i];
  }
  return 0;
}

static void
sp_memcpy(void *vd, const void *vs, size_t n)
{
  unsigned char *d = (unsigned char *)vd;
  const unsigned char *s = (const unsigned char *)vs;
  for (size_t i = 0; i < n; i++) d[i] = s[i];
}

static void
sp_memset(void *vd, int c, size_t n)
{
  unsigned char *d = (unsigned char *)vd;
  for (size_t i = 0; i < n; i++) d[i] = (unsigned char)c;
}

int
kld_patch_module_name_image(void *image, size_t image_size,
                            const char *orig_name, const char *new_name)
{
  size_t orig_len = sp_strlen(orig_name);
  size_t new_len  = sp_strlen(new_name);
  if (new_len == 0 || new_len >= KLD_MODULE_NAME_LEN || orig_len == 0) return -1;
  if (image_size < sizeof(Elf64_Ehdr)) return -1;

  Elf64_Ehdr *eh = (Elf64_Ehdr *)image;
  if (sp_memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
      eh->e_ident[EI_CLASS] != ELFCLASS64) return -1;
  if (eh->e_shoff == 0 || eh->e_shnum == 0 || eh->e_shstrndx == SHN_UNDEF) return -1;
  if ((size_t)eh->e_shoff + (size_t)eh->e_shnum * sizeof(Elf64_Shdr) > image_size) return -1;

  Elf64_Shdr *sh = (Elf64_Shdr *)((char *)image + eh->e_shoff);
  if (eh->e_shstrndx >= eh->e_shnum) return -1;
  Elf64_Shdr *shstr = &sh[eh->e_shstrndx];
  if ((size_t)shstr->sh_offset + (size_t)shstr->sh_size > image_size) return -1;
  const char *shnames = (const char *)image + shstr->sh_offset;

  for (int i = 0; i < eh->e_shnum; i++) {
    if (sh[i].sh_name >= shstr->sh_size) continue;
    const char *nm = shnames + sh[i].sh_name;
    if (!sp_streq(nm, ".gnu.linkonce.this_module")) continue;
    if ((size_t)sh[i].sh_offset + (size_t)sh[i].sh_size > image_size) return -1;

    char *sec = (char *)image + sh[i].sh_offset;
    size_t ssz = (size_t)sh[i].sh_size;
    for (size_t p = 0; p + orig_len < ssz; p++) {
      if (sp_memcmp(&sec[p], orig_name, orig_len + 1) == 0) {
        size_t field_sz = KLD_MODULE_NAME_LEN;
        if (p + field_sz > ssz) field_sz = ssz - p;
        sp_memset(&sec[p], 0, field_sz);
        sp_memcpy(&sec[p], new_name, new_len);
        return 0;
      }
    }
    return -1;
  }
  return -1;
}
