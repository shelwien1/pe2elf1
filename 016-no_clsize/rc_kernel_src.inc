//  mk_kernel.sh's input: whichever coder rc_config.inc's RC_RANS selects.
//
//  It is one line of #if rather than a shell case in mk_kernel.sh because the
//  choice is part of the -D set, and the -D set is what the preprocessor
//  already has in hand -- so the generator stays "run the C preprocessor over
//  one file", with nothing about the configuration duplicated in shell.
//
//  Both files are written against the bare name `Rangecoder` and the generator
//  strips the struct wrapper, so nothing has to be #defined around them here.

#if RC_RANS
#include "rans.inc"
#else
#include "rc.inc"
#endif
