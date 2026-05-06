// kotbl.c - ELF .kotbl section reader implementation
//
// Provides functions to read and access the .kotbl section from ELF files
// using libelf

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libelf.h>
#include <gelf.h>
#include <errno.h>

#include "kotbl.h"

int kotbl_read_section(const char *elf_path, KotblSection *kotbl)
{
    int fd = -1;
    Elf *elf = NULL;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    Elf_Data *data = NULL;
    int result = 0;

    if (!elf_path || !kotbl) {
        return -1;
    }

    // Initialize to zero
    kotbl->data = NULL;
    kotbl->size = 0;
    kotbl->elf = NULL;
    kotbl->fd = -1;

    // Initialize libelf
    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ERROR: libelf initialization failed\n");
        return -1;
    }

    // Open the ELF file
    fd = open(elf_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open ELF file '%s': %s\n", 
                elf_path, strerror(errno));
        return -1;
    }

    // Begin ELF reading
    elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf == NULL) {
        fprintf(stderr, "ERROR: elf_begin failed for '%s': %s\n",
                elf_path, elf_errmsg(-1));
        close(fd);
        return -1;
    }

    // Find the .kotbl section
    scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) == NULL) {
            fprintf(stderr, "ERROR: gelf_getshdr failed: %s\n", elf_errmsg(-1));
            result = -3;
            goto cleanup;
        }

        // Get section name
        const char *sec_name = elf_strptr(elf, shdr.sh_link, shdr.sh_name);
        if (sec_name && strcmp(sec_name, ".kotbl") == 0) {
            // Found the .kotbl section
            data = elf_getdata(scn, NULL);
            if (data == NULL) {
                fprintf(stderr, "ERROR: elf_getdata failed: %s\n", elf_errmsg(-1));
                result = -3;
                goto cleanup;
            }

            // Set data pointer directly to elf d_buf (no memcpy)
            kotbl->data = (const uint8_t *)data->d_buf;
            kotbl->size = data->d_size;
            kotbl->elf = elf;
            kotbl->fd = fd;
            
            // Return early, keeping elf and fd open for kotbl_free_section
            return 0;
        }
    }

    // .kotbl section not found
    fprintf(stderr, "ERROR: .kotbl section not found in '%s'\n", elf_path);
    result = -2;

cleanup:
    if (elf) {
        elf_end(elf);
    }
    if (fd >= 0) {
        close(fd);
    }

    return result;
}

void kotbl_free_section(KotblSection *kotbl)
{
    if (!kotbl) {
        return;
    }

    if (kotbl->elf) {
        elf_end((Elf *)kotbl->elf);
        kotbl->elf = NULL;
    }

    if (kotbl->fd >= 0) {
        close(kotbl->fd);
        kotbl->fd = -1;
    }

    kotbl->data = NULL;
    kotbl->size = 0;
}
