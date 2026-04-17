#include <err.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Helper to check if a file exists in a specific directory
char* resolve_path(const char *lib_name, const char *search_dirs) {
    if (!search_dirs) return NULL;
    char *dirs = strdup(search_dirs);
    char *dir = strtok(dirs, ":");
    static char full_path[1024];

    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, lib_name);
        struct stat buffer;
        if (stat(full_path, &buffer) == 0) {
            free(dirs);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }
    free(dirs);
    return NULL;
}

void libs(const char *filename) {
    int fd;
    Elf *e;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    char *runpath = NULL;

    if (elf_version(EV_CURRENT) == EV_NONE) errx(1, "ELF init failed");
    if ((fd = open(filename, O_RDONLY, 0)) < 0) err(1, "open failed");
    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) errx(1, "elf_begin failed");

    // First pass: Find RUNPATH/RPATH if they exist
    while ((scn = elf_nextscn(e, scn)) != NULL) {
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type == SHT_DYNAMIC) {
            Elf_Data *data = elf_getdata(scn, NULL);
            int entries = shdr.sh_size / shdr.sh_entsize;
            for (int i = 0; i < entries; i++) {
                GElf_Dyn dyn;
                gelf_getdyn(data, i, &dyn);
                if (dyn.d_tag == DT_RUNPATH || dyn.d_tag == DT_RPATH) {
                    runpath = (char*)elf_strptr(e, shdr.sh_link, dyn.d_un.d_val);
                }
            }
        }
    }

    // Second pass: Resolve NEEDED libraries
    scn = NULL;
    while ((scn = elf_nextscn(e, scn)) != NULL) {
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type == SHT_DYNAMIC) {
            Elf_Data *data = elf_getdata(scn, NULL);
            int entries = shdr.sh_size / shdr.sh_entsize;
            for (int i = 0; i < entries; i++) {
                GElf_Dyn dyn;
                gelf_getdyn(data, i, &dyn);
                if (dyn.d_tag == DT_NEEDED) {
                    const char *name = elf_strptr(e, shdr.sh_link, dyn.d_un.d_val);
                    char *found = NULL;

                    // Search Order: 1. RUNPATH, 2. LD_LIBRARY_PATH, 3. Standard paths
                    if (!(found = resolve_path(name, runpath)))
                        if (!(found = resolve_path(name, getenv("LD_LIBRARY_PATH"))))
                            found = resolve_path(name, "/lib:/usr/lib:/lib64:/usr/lib64");

                    printf("%-20s => %s\n", name, found ? found : "not found");
                }
            }
        }
    }
    elf_end(e);
    close(fd);
}

#ifdef MAIN
int main(int argc, char **argv){
  if (argc>1) {
    for (int i=1;i<argc; i++) {
      libs(argv[i]);
    }
  } else {
    fprintf(stderr, "USAGE:%s <elf> [elf]...\n", argv[0]);
    return -1;
  }
}
#endif
