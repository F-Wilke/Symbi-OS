#ifndef RUNTIME_PREPARE_H
#define RUNTIME_PREPARE_H

enum {
    KLD_PREP_LOAD_MODULES = 1 << 0,
    KLD_PREP_UPDATE_SOS   = 1 << 1,
    KLD_PREP_ALL          = KLD_PREP_LOAD_MODULES | KLD_PREP_UPDATE_SOS
};

int kld_runtime_prepare_for_exec(const char *target_path, int debug, int prep_mode);

#endif
