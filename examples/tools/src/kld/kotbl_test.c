// kotbl_test.c - Example program demonstrating how to use kotbl reading functions
//
// Compile with: gcc -o kotbl_test kotbl_test.c kotbl.c -lelf
// Usage: ./kotbl_test <elf_file>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kotbl.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf_file>\n", argv[0]);
        return 1;
    }

    const char *elf_file = argv[1];
    KotblSection kotbl = {0};

    printf("Reading .kotbl section from '%s'...\n", elf_file);

    // Read the .kotbl section
    int rc = kotbl_read_section(elf_file, &kotbl);
    if (rc != 0) {
        fprintf(stderr, "ERROR: Failed to read .kotbl section (error code: %d)\n", rc);
        return 1;
    }

    printf("Successfully read .kotbl section\n");
    printf("  Size: %zu bytes\n", kotbl_get_size(&kotbl));
    printf("\n");

    // Example 1: Display section as hex dump
    if (kotbl.size > 0) {
        printf("Hex dump of .kotbl section:\n");
        const uint8_t *data = kotbl_get_data(&kotbl);
        size_t size = kotbl_get_size(&kotbl);
        
        for (size_t i = 0; i < size; i++) {
            if (i % 16 == 0) {
                printf("%08zx: ", i);
            }
            printf("%02x ", data[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        if (size % 16 != 0) {
            printf("\n");
        }
        printf("\n");
    }

    // Example 2: Display section as ASCII (if text)
    printf("Section data as text (printable chars):\n");
    const uint8_t *data = kotbl_get_data(&kotbl);
    size_t size = kotbl_get_size(&kotbl);
    
    for (size_t i = 0; i < size; i++) {
        char c = data[i];
        if (c >= 32 && c < 127) {
            printf("%c", c);
        } else {
            printf(".");
        }
    }
    printf("\n\n");

    // Example 3: Show how to iterate through the data
    printf("Reading data in chunks:\n");
    size_t chunk_size = 16;
    for (size_t offset = 0; offset < size; offset += chunk_size) {
        size_t len = (offset + chunk_size > size) ? (size - offset) : chunk_size;
        printf("  Chunk at offset 0x%zx: %zu bytes\n", offset, len);
    }

    // Clean up (closes ELF and file descriptor)
    kotbl_free_section(&kotbl);
    printf("\nDone!\n");

    return 0;
}
