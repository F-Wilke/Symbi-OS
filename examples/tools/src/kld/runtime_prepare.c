#include "runtime_prepare.h"

/* Implemented in kldso_main.c and intentionally reused through one prep boundary. */
extern int load_modules_from_kotbl(const char *target_path, int debug);
extern int collect_and_apply_runtime_symbols(const char *target_path, int debug);

int kld_runtime_prepare_for_exec(const char *target_path, int debug, int prep_mode) {
    if (prep_mode & KLD_PREP_LOAD_MODULES) {
        if (load_modules_from_kotbl(target_path, debug) < 0) return -1;
    }
    if (prep_mode & KLD_PREP_UPDATE_SOS) {
        if (collect_and_apply_runtime_symbols(target_path, debug) < 0) return -1;
    }
    return 0;
}
