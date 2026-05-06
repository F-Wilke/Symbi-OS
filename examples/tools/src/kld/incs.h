#ifndef __INCS_H__
#define __INCS_H__

#include <stddef.h>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)
#include <fuse_lowlevel.h>
#include <stdlib.h>
#include <string.h>
#include <uthash.h>
#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>  // for PATH_MAX
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <err.h>
#include <stdbool.h>
#include <assert.h>
#include <libgen.h>
#include <sys/mman.h>
#include <elf.h>
#include <gelf.h>

#include "event.h"
#include "uthash.h"
#include "fs.h"
#include "sigproc.h"
#include "globals.h"
#include "misc.h"
#include "kldelf.h"
#include "kldfs.h"


#endif
