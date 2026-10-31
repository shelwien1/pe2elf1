// vp - lossless MP4/MOV, Matroska/WebM and AVI container splitter
//
//   vp c [-v] input out_prefix   -> out_prefix.meta + out_prefix.<i>.<ext> per track
//   vp d [-v] inp_prefix output  -> restores the original file byte-exact
//   vp file                      -> info: container, per-track analysis, the streams and
//                                   meta "vp c" would write (nothing is written); for a
//                                   .meta: its track list
//
// Track streams hold the frame data of each track, converted to standalone decodable
// formats where possible (Annex B H.264/HEVC, ADTS AAC, Ogg Vorbis, WAV, ...). The .meta
// holds everything else: per-track descriptors, a layout op program and the residual
// container bytes, with everything that can be derived from the streams or other
// container data regenerated on restore (see meta.inc and the container modules).
// Unrecognized input: the .meta is a copy of it. The encoder verifies its output by
// running the decoder in memory and falls back to simpler layouts, then to the copy.
//
// Single translation unit: all system includes are here, the .inc modules have none.
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include "base.inc"
#include "fileio.inc"
#include "writer.inc"
#include "reader.inc"
#include "sink.inc"
#include "coding.inc"
#include "column.inc"
#include "wide.inc"
#include "aac_tables.inc"
#include "aac_frame.inc"
#include "units.inc"
#include "adts.inc"
#include "annexb.inc"
#include "xiph.inc"
#include "vorbis.inc"
#include "ogg.inc"
#include "xform.inc"
#include "bitsmsb.inc"
#include "h264state.inc"
#include "hevcstate.inc"
#include "videofeat.inc"
#include "frameclass.inc"
#include "codecs.inc"
#include "meta.inc"

#include "mp4_tables.inc"
#include "mp4_time.inc"
#include "mp4_meta.inc"
#include "mp4_dec.inc"
#include "mp4_track.inc"
#include "mp4_frag.inc"
#include "mp4_enc.inc"

#include "mkv_ebml.inc"
#include "mkv_meta.inc"
#include "mkv_dec.inc"
#include "mkv_parse.inc"
#include "mkv_time.inc"
#include "mkv_enc.inc"

#include "avi_riff.inc"
#include "avi_hdr.inc"
#include "avi_meta.inc"
#include "avi_dec.inc"
#include "avi_parse.inc"
#include "avi_enc.inc"

#include "driver.inc"

int main( int argc, char** argv ) {
  try { return Main( argc, argv ); }
  catch( ... ) { fprintf( stderr, "out of memory or bad input\n" ); return 5; }
}
