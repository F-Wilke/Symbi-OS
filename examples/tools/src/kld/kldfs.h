#ifndef __KLDFS_H__
#define __KLDFS_H__

extern void kldfsCreate(fs_t *fs, fs_ino_t rootino);
extern bool kldfsLoop(fs_t *fs, sigproc_t *sigproc);


#endif // __KLDFS_H__
