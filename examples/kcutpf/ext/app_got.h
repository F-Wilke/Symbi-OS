#ifndef APP_GOT_H_EXT_
#define APP_GOT_H_EXT_

#include "app_orig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SET_APP_GOT_FUNC(func_name) \
    app_got->func_name = (void *)&func_name;

typedef struct {
// GOT Declarations
void* dummy; // No GOT entries found, dummy entry to avoid empty struct
} app_got_t;

#define SET_ALL_GOT_ENTRIES() // No functions found, no GOT entries to set
#ifdef __cplusplus
}
#endif

#endif // APP_GOT_H_EXT_