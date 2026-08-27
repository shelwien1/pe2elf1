
// -------------------------------------------------------------
//  rc_dev.h -- what coder.cpp and the encoder see of the device path, which
//  is the ISPC-compiled coding kernel (rc_ispc.cpp).
//
//  Nothing here is required. With -C or in a build without RC_ISPC, DEV_Init
//  reports failure and model0.inc runs the host coder -- which is still
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

// The row geometry of the encoder's substream buffers, which rc_ispc.cpp
// cannot see the template for. Call before DEV_Init.
void DEV_Geometry( uint stride, uint cap );

// Idempotent, and a no-op unless DEV_Enable(1) was called first. Returns 1
// when the device path is live.
int  DEV_Init( void );

// The encoder calls this; the decoder never does (its split path calls the
// batch kernel directly and needs none of this machinery).
void DEV_Enable( int on );

int  DEV_Active( void );

// Queue one block's cached {p;bit} pairs for the carryless coder, and come
// back for it later. Split in two so the host can model the next block while
// the worker codes this one -- see RC_DEV_NBLK.
//
//   slot    which of the RC_DEV_NBLK slots; the caller's ring index
//   pbit    nbits packed (bit<<15)|p pairs, bit k belonging to lane k%RCNUM
//   blksize the block's length in bytes, for the block-length header bits
//   lens    filled by the time DEV_Collect returns: bytes each lane produced
//           (> the row capacity means it did not fit, the same condition the
//           host path reports)
//   carries likewise: carries that escaped each lane. Any non-zero and the
//           block has to be re-coded with the carry-propagating twin.
//
// DEV_Submit does not block, and pbit, lens and carries must stay put until
// the matching DEV_Collect. Both return 1 on success; 0 routes the caller to
// the host coder.
int  DEV_Submit( uint slot, const word* pbit, uint nbits, uint blksize,
                 uint* lens, uint* carries );

// Wait for slot's block and read its substreams into RCNUM rows of `stride`
// bytes at `rows`. Only the bytes each lane produced are read back.
int  DEV_Collect( uint slot, byte* rows, uint stride );

// -V: kernel time totals
void DEV_Report( FILE* f );

void DEV_Quit( void );
