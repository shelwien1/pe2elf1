/* Replacement for glibc's <features.h> (MinGW-w64): only __GNUC_PREREQ is used by ROSE. */
#ifndef ROSE_MINGW_FEATURES_H
#define ROSE_MINGW_FEATURES_H

#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(major, minor) \
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((major) << 16) + (minor))
#endif

#endif
