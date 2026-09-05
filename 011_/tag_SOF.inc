
// uint i;

void pa_SOF( int is_prog, int is_arith ) { CORO_START;

  progressive_mode = is_prog;
  arith_code = is_arith;

  data_precision = coro2_get();
  image_height = coro2_get2();
  image_width = coro2_get2();
  num_components = coro2_get();

  printf( "Start Of Frame 0x%02x: width=%u, height=%u, components=%d\n", tag_id, image_width, image_height, num_components );

  for( i=0; i<num_components; i++ ) {
    //jpeg_component_info& ci = comp_info[i];
    #define ci comp_info[i]

    ci.component_index = i;
    ci.component_id = coro2_get();
    ci.h_samp_factor = coro2_getH();
    ci.v_samp_factor = coro2_getL();
    ci.quant_tbl_no = coro2_get();

    printf( "    Component %d: %dhx%dv q=%d\n", ci.component_id, ci.h_samp_factor, ci.v_samp_factor, ci.quant_tbl_no );
    #undef ci
  }

  saw_SOF = 1;

  CORO_END;
}


