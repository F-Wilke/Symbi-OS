#include "load_mod.h"

FILE *ko_log_file = NULL;


void print_load_info(const struct load_info *info) {
    if (!info) {
        PRINTF("load_info: NULL\n");
        return;
    }
    PRINTF("load_info {\n");
    PRINTF("  name: %s\n", info->name ? info->name : "(null)");
    PRINTF("  mod: %p\n", info->mod);
    PRINTF("  hdr: %p\n", info->hdr);
    PRINTF("  len: %lu\n", info->len);
    PRINTF("  sechdrs: %p\n", info->sechdrs);
    PRINTF("  secstrings: %s\n", info->secstrings ? info->secstrings : "(null)");
    PRINTF("  strtab: %s\n", info->strtab ? info->strtab : "(null)");
    PRINTF("  symoffs: %lu\n", info->symoffs);
    PRINTF("  stroffs: %lu\n", info->stroffs);
    PRINTF("  init_typeoffs: %lu\n", info->init_typeoffs);
    PRINTF("  core_typeoffs: %lu\n", info->core_typeoffs);
    PRINTF("  debug: %p\n", info->debug);
    PRINTF("  num_debug: %u\n", info->num_debug);
    PRINTF("  sig_ok: %d\n", info->sig_ok);
    PRINTF("  mod_kallsyms_init_off: %lu\n", info->mod_kallsyms_init_off);
    PRINTF("  index: { sym: %u, str: %u, mod: %u, vers: %u, info: %u, pcpu: %u }\n",
           info->index.sym, info->index.str, info->index.mod,
           info->index.vers, info->index.info, info->index.pcpu);
    PRINTF("}\n");
}


static int copy_chunked_from_user(void *dst, void  *usrc, unsigned long len)
{
    PRINTF("[copy_chunked_from_user] dst=%p usrc=%p len=%lu\n", dst, usrc, len);
    unsigned long orig_len = len;
    void *orig_dst = dst;
    void *orig_usrc = usrc;
    int ret = 0;
    do {
        unsigned long n = min(len, COPY_CHUNK_SIZE);
        PRINTF("[copy_chunked_from_user] Copying chunk: n=%lu, dst=%p, usrc=%p, remaining len=%lu\n", n, dst, usrc, len);
        if (_copy_from_user(dst, usrc, n) != 0) {
            PRINTF("[copy_chunked_from_user] _copy_from_user failed at dst=%p usrc=%p n=%lu\n", dst, usrc, n);
            ret = -EFAULT;
            break;
        }
        __cond_resched();
        dst += n;
        usrc += n;
        len -= n;
    } while (len);
    if (ret == 0)
        PRINTF("[copy_chunked_from_user] Successfully copied %lu bytes from %p to %p\n", orig_len, orig_usrc, orig_dst);
    return ret;
}

/* Sets info->hdr and info->len. */
static int copy_module_from_user(void *umod, unsigned long len,
                                  struct load_info *info)
{
    PRINTF("[copy_module_from_user] umod=%p len=%lu info=%p\n", umod, len, info);
    int err = 0;
    info->len = len;
    /* Suck in entire file: we'll want most of it. */
    info->hdr = __vmalloc(info->len, GFP_KERNEL | ___GFP_NOWARN);
    PRINTF("[copy_module_from_user] Allocated info->hdr=%p (len=%lu)\n", info->hdr, info->len);
    if (!info->hdr) {
        PRINTF("[copy_module_from_user] __vmalloc failed!\n");
        return -ENOMEM;
    }
    if (copy_chunked_from_user(info->hdr, umod, info->len) != 0) {
        PRINTF("[copy_module_from_user] copy_chunked_from_user failed!\n");
        err = -EFAULT;
        goto out;
    }
out:
    if (err) {
        PRINTF("[copy_module_from_user] Freeing info->hdr=%p due to error\n", info->hdr);
        vfree(info->hdr);
    }
    PRINTF("[copy_module_from_user] Returning err=%d\n", err);
    return err;
}


//assume sym_elevate has been called before this function
int load_included_module() {
    PRINTF("starting load_included_module\n");
    
    int ret = 0;
    size_t size = (size_t)&_binary_greeter_ko_size;
    PRINTF("greeter.ko blob loaded at %p, size: %zu bytes\n",
        _binary_greeter_ko_start, size);
        
        // Optional: dump first few bytes
        PRINTF("First 8 bytes: ");
        for (size_t i = 0; i < 8 && i < size; i++) {
            PRINTF("%02x ", _binary_greeter_ko_start[i]);
        }
        PRINTF("\n");
        
        
    // sym_elevate();
    PRINTF("called elevate\n");
    // struct load_info __info = {0};
    // struct load_info* info = &__info;

    struct load_info* info = __vmalloc(sizeof(struct load_info), GFP_KERNEL);
    if (!info) {
        PRINTF("Failed to allocate memory for load_info\n");
        return 1;
    }
    memset(info, 0, sizeof(struct load_info));
    int err = copy_module_from_user((void*)_binary_greeter_ko_start, size, info);
	if (err) {
        PRINTF("Failed to copy module from user space: %d\n", err);
        vfree(info->hdr);
	    return 1;
    }
	
    PRINTF("starting load_module\n");
    char * uargs = "name=Hansi";
    // char *uargs = __vmalloc(strlen(uargs_local) + 1, GFP_KERNEL);
    // if (!uargs) {
    //     PRINTF("Failed to allocate memory for uargs\n");
    //     vfree(info->hdr);
    //     return 1;
    // }
    // if (_copy_from_user(uargs, uargs_local, strlen(uargs_local) + 1) != 0) {
    //     PRINTF("Failed to copy uargs from user space\n");
    //     // vfree(uargs);
    //     vfree(info->hdr);
    //     return 1;
    // }
    
    //print argument locations
    PRINTF("uargs: %p, info: %p\n", uargs, info);
    print_load_info(info);

	ret = load_module(info, uargs, 0);
    PRINTF("exited load_module\n");
	
    // vfree(uargs);
    // vfree(info->hdr); //done by load_module
    // sym_lower();
    PRINTF("called sym_lower\n");
	return ret;
}