
// Frame header: SOF0-3, SOF5-7, SOF9-11, SOF13-15 and DHP, which all share the
// same syntax.  The process flags (progressive / arithmetic / lossless /
// differential / hierarchical) are set from the marker code by do_process().

void pa_SOF( void ) { CORO_START;

  // P(1) + Y(2) + X(2) + Nf(1) = 6, then 3 bytes per component.
  if( l_tag<6 ) { pjpg_err( E_BADLEN, "SOF segment shorter than 6 bytes" ); CORO_EXIT; }

  progressive_mode = sof_prog;
  arith_code       = sof_arith;

  data_precision = coro2_get();
  image_height = coro2_get2();
  image_width = coro2_get2();
  num_components = coro2_get();

  pf( "Start Of Frame 0x%02x: width=%u, height=%u, components=%d\n", tag_id, image_width, image_height, num_components );
  pf( "  %s, %d-bit%s\n", sof_name(), data_precision, sof_arith ? ", arithmetic" : ", Huffman" );

  // Nf==0 is illegal and would leave every later scan resolving component ids
  // against an empty frame.
  if( num_components<1 ) { pjpg_err( E_BADSOF, "frame declares 0 components" ); CORO_EXIT; }

  // Nf is one byte so it cannot exceed comp_info[256], but the segment must
  // actually carry the 3 bytes per component it claims.
  if( l_tag < 3*uint(num_components) ) { pjpg_err( E_BADSOF, "frame header shorter than its component count" ); CORO_EXIT; }

  if( image_width==0 ) { pjpg_err( E_BADSOF, "frame width is 0" ); CORO_EXIT; }
  // Y==0 is legal: the true height then arrives in a DNL segment.
  if( image_height==0 ) { pjpg_warn( "frame height is 0, expecting DNL" ); }

  if( (data_precision<2) || (data_precision>16) ) { pjpg_err( E_BADSOF, "frame precision out of range" ); CORO_EXIT; }

  // T.81 B.2.2 table B.2: a DCT-based frame is 8-bit (baseline) or 8- or
  // 12-bit (extended and progressive).  Only the lossless processes take the
  // full 2..16.  Anything else is not a precision a decoder can be asked for,
  // and libjpeg stops on it ("Unsupported JPEG data precision N").
  if( !sof_lossless && (data_precision!=8) && (data_precision!=12) ) {
    pjpg_warn( "DCT frame precision is neither 8 nor 12" );
    n_nonconf++;
  }

  max_h_samp_factor = 1; max_v_samp_factor = 1;

  for( i=0; i<uint(num_components); i++ ) {
    #define ci comp_info[i]

    ci.component_index = i;
    ci.component_id = coro2_get();
    ci.h_samp_factor = coro2_getH();
    ci.v_samp_factor = coro2_getL();
    ci.quant_tbl_no = coro2_get();

    pf( "    Component %d: %dhx%dv q=%d\n", ci.component_id, ci.h_samp_factor, ci.v_samp_factor, ci.quant_tbl_no );

    // T.81 B.2.2 puts Hi and Vi in 1..4.  Outside that the frame geometry is
    // not merely unusual, it is undefined, and libjpeg stops on it ("Bogus
    // sampling factors") -- so it counts against conformance rather than
    // being just a remark.
    if( (ci.h_samp_factor<1) || (ci.h_samp_factor>4) || (ci.v_samp_factor<1) || (ci.v_samp_factor>4) ) {
      pjpg_warn( "component sampling factor outside 1..4" );
      n_nonconf++;
    }
    if( ci.quant_tbl_no>=4 ) {
      pjpg_err( E_BADSOF, "component references quantisation table >= 4" ); CORO_EXIT;
    }
    if( ci.h_samp_factor>max_h_samp_factor ) max_h_samp_factor = ci.h_samp_factor;
    if( ci.v_samp_factor>max_v_samp_factor ) max_v_samp_factor = ci.v_samp_factor;
    #undef ci
  }

  saw_SOF = 1;

  coro_done: CORO_END;
}
