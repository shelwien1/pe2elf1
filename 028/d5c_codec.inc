/* d5c_codec.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * Blk and Codec<MD> -- the model itself.
 * Codec is assembled from the d5c_codec_*.inc parts included in its body. */

//------------------------------------------------------------------- codec

/* Deliberately trivial -- no default member initializers.  The block grids are
 * MAX_B x MAX_B static slabs; giving Blk a non-trivial default constructor
 * would make the codec construct ~4M of them at startup and dirty every page
 * of the grids, instead of leaving them demand-zero and clearing only the
 * bw*bh actually in use.  The two places that want a zeroed Blk say so with
 * `= {}`, and level_init clears the live grid region with memset (all-zero is
 * exactly the old Blk() state). */
struct Blk { u8 a0, a1; u16 c0, c1; u64 aw; u32 cw; u8 ai[16], ci[16]; };

/* Codec inherits every generated %M%_T struct, so the Table() members declared
 * in IDX/*.inc (M0_tAM1, A0_tA0W, E0_tE00, ...) are directly in scope here.
 * The prefixes keep them unambiguous.  In the Debug build those members are
 * pointers filled by %M%_Init(); in the Const build they are fixed-size arrays
 * and %M%_Init() is empty.  Either way the allocation happens ONCE, in this
 * constructor -- init(fmt) below only rebinds and refills, which is what makes
 * the per-file reset of --ns cheap.
 *
 * NOTE: with the Const build this object is several hundred MiB.  Instantiate
 * it on the heap (ddsdet does `new d5c::Solid`), never on the stack, and use
 * default- rather than value-initialization so the slabs stay demand-zero. */
template<int MD>
struct Codec : G0_T, M0_T, A0_T, B0_T, C0_T, E0_T, X0_T, P0_T {

  /* The class body is assembled from the parts below.  Including them here
   * rather than declaring member functions out of line keeps every definition
   * in exactly one place and keeps the whole model in one translation unit --
   * the same idiom ddsdet.cpp already uses for dds_header.inc. */
  #include "d5c_codec_state.inc"
  #include "d5c_codec_geom.inc"
  #include "d5c_codec_bits.inc"
  #include "d5c_codec_pal.inc"
  #include "d5c_codec_misc.inc"
  #include "d5c_codec_level.inc"
};
