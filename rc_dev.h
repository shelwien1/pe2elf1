
// -------------------------------------------------------------
//  rc_dev.h -- what coder.cpp and the encoder see of the device path, which
//  is the ispc-compiled coding kernel (rc_ispc.cpp).
//
//  Nothing here is required. With -C or in a build without RC_ISPC,
//  DEV_Encode returns 0 and model0.inc runs the host coder -- which is still
//  there, still the reference, and still what the kernel output is checked
//  against.
//
//  Include after common.inc: uint/byte/word come from there.
// -------------------------------------------------------------

#pragma once

struct DEVOpts {
  int use;      // 0 = -C, never touch the kernel
  int verbose;  // -V
};

extern DEVOpts g_devopt;

// -l: describe the compiled-in kernel
void DEV_ListDevices( FILE* f );

// One block's carryless coding pass, synchronous: nbits packed (bit<<15)|p
// pairs out of pbit, bit k in lane k%RCNUM, coded straight into RCNUM rows of
// `stride` bytes at `rows` (`cap` payload bytes each). lens[] gets the bytes
// every lane produced -- above cap means that lane did not fit -- and
// carries[] the carries that escaped it; any non-zero and the caller re-codes
// the block with the carry-propagating twin. Returns 0 (untouched output)
// with -C or without RC_ISPC.
int DEV_Encode( const word* pbit, uint nbits, uint blksize,
                byte* rows, uint stride, uint cap,
                uint* lens, uint* carries );

// -V: kernel time totals
void DEV_Report( FILE* f );
