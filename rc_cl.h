
// -------------------------------------------------------------
//  rc_cl.h -- what coder.cpp and the encoder see of the OpenCL path.
//
//  Nothing here is required. With -C, with no usable device, or in a build
//  with RC_OPENCL=0, CL_Init reports failure and model0.inc runs the host
//  coder -- which is still there, still the reference, and still what the
//  device output is checked against.
//
//  Include after common.inc: uint/byte/word come from there.
// -------------------------------------------------------------

#pragma once

struct CLOpts {
  int use;      // 0 = -C, never touch a device
  int plat;     // -p, -1 = any
  int dev;      // -d, -1 = pick one
  int type;     // -T: 0 any (prefer GPU), 1 cpu, 2 gpu, 3 accelerator
  int verbose;  // -V
};

extern CLOpts g_clopt;

// -l: every platform and device, in the order -d counts them
void CL_ListDevices( FILE* f );

// The row geometry of the encoder's substream buffers, which rc_cl.cpp cannot
// see the template for. Call before CL_Init.
void CL_Geometry( uint stride, uint cap );

// Open the device and build the kernel. Idempotent, and a no-op unless
// CL_Enable(1) was called first. Returns 1 when the device path is live.
// Anything that goes wrong is reported on stderr and leaves it at 0.
int  CL_Init( void );

// The encoder calls this; the decoder never does.
void CL_Enable( int on );

int  CL_Active( void );

// Code one block's cached {p;bit} pairs on the device, carryless.
//
//   pbit    nbits packed (bit<<15)|p pairs, bit k belonging to lane k%RCNUM
//   blksize the block's length in bytes, for the block-length header bits
//   tmpbuf  RCNUM rows of `stride` bytes -- the substream buffers
//   lens    out: bytes each lane produced (> the row capacity means it did
//           not fit, same condition the host path reports)
//   carries out: carries that escaped each lane. Any non-zero and the block
//           has to be re-coded on the host with the carry-propagating twin.
//
// Returns 1 on success. On failure the device path is switched off for the
// rest of the run and the caller falls back to the host coder.
int  CL_EncodeBlock( const word* pbit, uint nbits, uint blksize,
                     byte* tmpbuf, uint stride, uint* lens, uint* carries );

// -V: device time, and how much of the wall clock the launches cost
void CL_Report( FILE* f );

void CL_Quit( void );
