// FROM GEMINI 3
#include <stdio.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

// Helper to determine the single-letter type code
char get_symbol_type(Elf *elf, GElf_Sym *sym, GElf_Shdr *sym_shdr) {
    char type = '?';
    GElf_Shdr shdr;
    Elf_Scn *scn;

    // 1. Handle special section indices first
    if (sym->st_shndx == SHN_UNDEF) return 'U';
    if (sym->st_shndx == SHN_ABS) return 'A';
    if (sym->st_shndx == SHN_COMMON) return 'C';

    // 2. Get the section header where the symbol is defined
    scn = elf_getscn(elf, sym->st_shndx);
    if (!scn || gelf_getshdr(scn, &shdr) == NULL) return '?';

    // 3. Determine type based on section flags and type
    if (shdr.sh_type == SHT_NOBITS) {
        type = 'B'; // BSS (Uninitialized Data)
    } else if (shdr.sh_flags & SHF_EXECINSTR) {
        type = 'T'; // Text (Code)
    } else if (shdr.sh_flags & SHF_WRITE) {
        type = 'D'; // Data (Initialized)
    } else if (shdr.sh_flags & SHF_ALLOC) {
        type = 'R'; // Read-only Data
    } else {
        type = 'N'; // Debugging or other non-allocatable
    }

    // 4. Handle Weak symbols (overrides previous types)
    if (GELF_ST_BIND(sym->st_info) == STB_WEAK) {
        type = (sym->st_shndx == SHN_UNDEF) ? 'w' : 'W';
        return type; // Weak symbols are returned directly
    }

    // 5. Convert to lowercase if the symbol is Local
    if (GELF_ST_BIND(sym->st_info) == STB_LOCAL) {
        type = tolower(type);
    }

    return type;
}

int main(int argc, char **argv) {
    Elf *elf;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    Elf_Data *data;
    int fd;

    if (argc != 2) return 1;

    elf_version(EV_CURRENT);
    fd = open(argv[1], O_RDONLY);
    if (fd==-1) {
      fprintf(stderr, "ERROR: open %s: %s\n", argv[1], strerror(errno));
      return -1;
    }
    elf = elf_begin(fd, ELF_C_READ, NULL);
    if (elf==NULL) {
      fprintf(stderr, "ERROR: elf_begin: failed\n");
      return -1;
    }
    if (elf_kind(elf) != ELF_K_ELF) {
      fprintf(stderr, "ERROR: %s: Not a valid ELF file.\n", argv[1]);
      return -1;
    }
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
            data = elf_getdata(scn, NULL);
            int count = shdr.sh_size / shdr.sh_entsize;

            for (int i = 0; i < count; i++) {
                GElf_Sym sym;
                gelf_getsym(data, i, &sym);

                // Skip empty symbols
                if (sym.st_name == 0 && sym.st_value == 0) continue;

                char type = get_symbol_type(elf, &sym, &shdr);
                char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
 
                // NM Format: Address Type Name
                // (If undefined, nm often prints spaces for address, but 0 is common in simple implementations)
                if (sym.st_shndx == SHN_UNDEF)
                    printf("                 %c %s\n", type, name);
                else
                    printf("%016lx %c %s\n", sym.st_value, type, name);
            }
        }
    }

    elf_end(elf);
    close(fd);
    return 0;
}
