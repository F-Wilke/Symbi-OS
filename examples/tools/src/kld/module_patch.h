#ifndef MODULE_PATCH_H
#define MODULE_PATCH_H

#include <stddef.h>

#ifndef KLD_MODULE_NAME_LEN
#define KLD_MODULE_NAME_LEN 56
#endif

/* Patches .gnu.linkonce.this_module module name in a writable ELF image.
 * Returns 0 on success, -1 on failure.
 */
int kld_patch_module_name_image(void *image, size_t image_size,
                                const char *orig_name, const char *new_name);

#endif
