// Following was generated with github copilot (Claude Haiku 4.5)
// kotbl.h - ELF .kotbl section reader interface
//
// This header provides functions to read and access the .kotbl section
// from an ELF file.

#ifndef __KOTBL_H__
#define __KOTBL_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *data;
    size_t size;
    void *elf;              // Elf* pointer (kept open until free)
    int fd;                 // File descriptor (kept open until free)
} KotblSection;

// Read the .kotbl section from an ELF file
// Arguments:
//   elf_path: Path to the ELF file
//   kotbl: Pointer to KotblSection structure (will be filled with section data)
// Returns:
//   0 on success
//   -1 if file cannot be opened
//   -2 if .kotbl section is not found
//   -3 if memory allocation fails
int kotbl_read_section(const char *elf_path, KotblSection *kotbl);

// Free resources allocated by kotbl_read_section
// This will close the ELF file and file descriptor
void kotbl_free_section(KotblSection *kotbl);

// Get a pointer to raw data and size
static inline const uint8_t* kotbl_get_data(const KotblSection *kotbl) {
    return kotbl->data;
}

static inline size_t kotbl_get_size(const KotblSection *kotbl) {
    return kotbl->size;
}

#endif // __KOTBL_H__
