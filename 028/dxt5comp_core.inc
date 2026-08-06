/* dxt5comp_core.inc -- the whole dxt5comp codec as a reusable header.
 *
 * Everything except the command-line driver: dds parsing, the logistic tables,
 * the range coder, counters/mixer/APM, struct Codec, and struct Solid (the
 * multi-texture SOLID archive). Used by both dxt5comp.cpp and ddsdet.cpp.
 *
 * NO parameter folding: the generated MOD/*.inc are #included normally, so
 * this header works in BOTH build modes --
 *   mk.sh          -> Debug/mapping build, parameters live in the binary as
 *                     pdesc/pmask pattern strings for IDX/opt.pl to flip
 *   mk.sh release  -> Const build, patterns evaluated at preprocess time
 * A host outside this directory just needs the dxt5comp tree on its include
 * path (-I<dxt5comp-dir>) so "sh_mapping.inc" and "MOD/..." resolve.
 *
 * Everything lives in namespace d5c so the header can be dropped into a host
 * that has its own byte/uint/qword typedefs and its own crc32b -- ddsdet has
 * both, via Lib3/common.inc. dxt5comp.cpp just says `using namespace d5c;`.
 */
#ifndef DXT5COMP_CORE_INC
#define DXT5COMP_CORE_INC

/* PREREQUISITES -- the frontend includes these itself, at global scope, before
 * including this header:
 *
 *     #include <cstdint>   #include <cstdlib>   #include <cmath>
 *     #include <cstdio>    #include <cstring>
 *
 * They are deliberately NOT pulled in here: a host like ddsdet has already
 * included them through its own framework header, and re-including them from
 * inside this file made include order -- and the std::byte workaround that
 * used to live here -- depend on who included whom first.
 *
 * On std::byte: this header typedefs `byte` inside namespace d5c.  That
 * declaration wins over the one <cstddef> puts in namespace std, because a
 * using-directive only makes std's names visible as if declared in the nearest
 * common enclosing namespace -- the global one -- while d5c::byte is declared
 * in d5c itself and hides it.  So a frontend needs no `#define byte byte1`
 * guard around its system includes.
 */
#ifndef INT8_MAX
#  error "dxt5comp_core.inc: include <cstdint> <cstdio> <cstdlib> <cstring> <cmath> before this header"
#endif


namespace d5c {

/* ============================================================================
 * dxt5comp_core.inc is a table of contents.  Every part below is a .inc
 * included into this one translation unit: no separate objects, and every
 * entity is defined exactly once, in one file.
 *
 * THE INCLUDE ORDER IS LOAD-BEARING in one place, and it is not obvious:
 *
 *   d5c_counter.inc      declares Ctr / CtrS      -- must come BEFORE MOD/*_h,
 *                                                    whose Table() members are
 *                                                    typed `Ctr`
 *   MOD/dxt5-*_h.inc     generated tables         -- define CW's feeders and
 *   MOD/dxt5-*_p.inc     generated parameters        G0_SLOW_MUL
 *   d5c_counter_upd.inc  defines Ctr::upd bodies  -- must come AFTER, because
 *                                                    they use those
 *
 * Splitting the counter into two files is the price of that cycle; do not try
 * to merge them.  The Lib3 coroutine chain (common.inc / coro3b.inc) must be
 * included by the .cpp before this file, since RC derives from Coroutine.
 * ========================================================================= */

#include "d5c_prelude.inc"
#include "sh_mapping.inc"
#include "d5c_counter.inc"

#include "MOD/dxt5-G0_h.inc"
#include "MOD/dxt5-M0_h.inc"
#include "MOD/dxt5-A0_h.inc"
#include "MOD/dxt5-B0_h.inc"
#include "MOD/dxt5-C0_h.inc"
#include "MOD/dxt5-E0_h.inc"
#include "MOD/dxt5-X0_h.inc"
#include "MOD/dxt5-P0_h.inc"
#include "MOD/dxt5-M0_p.inc"
#include "MOD/dxt5-A0_p.inc"
#include "MOD/dxt5-B0_p.inc"
#include "MOD/dxt5-C0_p.inc"
#include "MOD/dxt5-E0_p.inc"
#include "MOD/dxt5-X0_p.inc"
#include "MOD/dxt5-P0_p.inc"

#include "crc32b.inc"
#include "d5c_sha256.inc"
#include "d5c_dds.inc"
#include "d5c_logistic.inc"
#include "d5c_bytebuf.inc"
#include "d5c_dedup.inc"      // needs ByteBuf and Hash128
#include "d5c_rc.inc"
#include "d5c_mix.inc"
#include "d5c_codec.inc"
#include "d5c_solid.inc"

} // namespace d5c

#endif // DXT5COMP_CORE_INC
