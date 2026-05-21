Rangecoder rc;

PROBE_DECL(sub_140002AD0)
__int64 sub_140002AD0() {
  PROBE_HIT(sub_140002AD0);
  uint v0;
  char v1;
  int v2;
  uint v3;
  int v4;
  int v5;
  uint v6;
  uint v7;
  uint v8;
  uint i;
  __m128i si128;
  uint v11;
  __m128i v12;
  uint v13;
  uint v14;
  uint v15;
  uint v16;
  uint v17;
  uint v18;
  uint v19;
  uint v20;
  uint v21;
  uint v22;
  uint v23;
  uint v24;
  uint v25;
  uint v26;
  uint v27;
  __m128i v28;
  uint v29;
  __m128i v30;
  __m128i v31;
  __m128i v32;
  uint v33;
  __m128i v34;
  uint v35;
  uint j;
  int v37;
  uint v38;
  uint v39;
  uint v40;
  uint k;
  __m128i v42;
  uint v43;
  uint v44;
  uint v45;
  uint v46;
  uint v47;
  uint v48;
  uint v49;
  uint v50;
  uint v51;
  uint v52;
  uint v53;
  uint v54;
  uint v55;
  uint v56;
  uint v57;
  __m128i v58;
  uint v59;
  __m128i v60;
  __m128i v61;
  __m128i v62;
  __m128i v63;
  __m128i v64;
  uint v65;
  uint v66;
  __m128i v67;
  int v68;
  uint m;
  uint v70;
  uint n;
  __m128i v72;
  __int64 v73;
  __int64 v74;
  char v75;
  int v76;
  __int64 result;
  int v78;
  uint ii;
  __m128i v80;
  __int64 v81;
  __m128i v82;
  __m128i v83;
  __m128i v84;
  __m128i v85;
  __m128i v86;
  __m128i v87;
  uint v88;
  __m128i v89;
  __m128i v90;
  __m128i v91;
  __m128i v92;
  __m128i v93;
  __m128i v94;
  __m128i v95;
  uint v96;
  int v97;
  int v98;
  uint v99;
  int v100;
  int v101;
  uint v102;
  int v103;
  int v104;
  uint v105;
  int v106;
  int v107;
  uint v108;
  int v109;
  uint v110;
  uint v111;
  int v112;
  uint v113;
  uint v114;
  uint v115;
  uint v116;
  uint v117;
  uint v118;
  uint v119;
  uint v120;
  int v121;
  uint v122;

  (int &)__algn_141136BC4[1084+0] = 0x01000000;
  (int &)__algn_141136BC4[1084+4] = 0x03030201;
  (int &)__algn_141136BC4[1084+8] = 0x00000004;

  LODWORD(x1) = 0x4030201;
  BYTE4(x1) = 5;
  v108 = 0;
  v0 = 5;
  do {
    v1 = 2*v108+7;
    v2 = 2*v108+++9;
    *((byte*)&x1+v0++) = v1;
    v3 = v0;
  } while( v108<3 );
  v109 = v2;
  v4 = v2+1;
  v5 = v2+1;
  if( v0<0xB ) {
    v6 = 11-v0;
    v7 = ((byte)v0+(byte)(uintptr_t)&x1)&0xF;
    if( (((byte)v0+(byte)(uintptr_t)&x1)&0xF)!=0 )
      v7 = 16-(((byte)v0+(byte)(uintptr_t)&x1)&0xF);
    if( v6<v7+16 ) {
      v29 = 0;
    } else {
      v8 = 0;
      for( i = v0; v8<v7; ++v8 ) {
        *((byte*)&x1+i) = v4;
        v4 += 3;
        ++i;
      }
      si128 = _mm_load_si128((const __m128i*)&x2);
      v111 = v7;
      v119 = v0;
      v113 = 11-v0;
      v115 = v6-(((byte)v6-(byte)v7)&0xF);
      v11 = 3*v7+v109+1;
      v117 = v0;
      v12 = _mm_cvtsi32_si128(v11);
      v121 = v5;
      v13 = v11+3;
      v14 = v11+6;
      v15 = v11+9;
      v16 = v11+12;
      v17 = v11+15;
      v18 = v11+18;
      v19 = v11+21;
      v20 = v11+24;
      v21 = v11+27;
      v22 = v11+30;
      v23 = v11+33;
      v24 = v11+36;
      v25 = v11+39;
      v122 = v11+42;
      v26 = v11+45;
      v27 = v111;
      v28 = _mm_cvtsi32_si128(v13);
      v29 = v115;
      v30 = _mm_unpacklo_epi8(_mm_cvtsi32_si128(v14), _mm_cvtsi32_si128(v15));
      v5 = v121;
      v6 = v113;
      v31 = _mm_cvtsi32_si128(v16);
      v3 = v117;
      v32 = _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(v12, v28), v30), _mm_unpacklo_epi16(_mm_unpacklo_epi8(v31, _mm_cvtsi32_si128(v17)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v18), _mm_cvtsi32_si128(v19))));
      v33 = v119+v111;
      v34 = _mm_unpacklo_epi64(v32, _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v20), _mm_cvtsi32_si128(v21)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v22), _mm_cvtsi32_si128(v23))), _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v24), _mm_cvtsi32_si128(v25)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v122), _mm_cvtsi32_si128(v26)))));
      do {
        *(__int128*)((char*)&x1+v33) = (__int128)v34;
        v34 = _mm_add_epi8(v34, si128);
        v33 += 16;
        v27 += 16;
      } while( v27<v115 );
      v0 = v27+v119;
      v4 = 3*v27+v109+1;
    }
    v35 = v29+v3;
    for( j = 3*v29+v5; v29<v6; ++v35 ) {
      *((byte*)&x1+v35) = j;
      v0 = v35+1;
      v4 = j+3;
      ++v29;
      j += 3;
    }
  }
  v37 = v4+1;
  if( v0<0x26 ) {
    v38 = 38-v0;
    v39 = ((byte)v0+(byte)(uintptr_t)&x1)&0xF;
    if( (((byte)v0+(byte)(uintptr_t)&x1)&0xF)!=0 )
      v39 = 16-(((byte)v0+(byte)(uintptr_t)&x1)&0xF);
    if( v38<v39+16 ) {
      v59 = 0;
    } else {
      v40 = 0;
      for( k = v0; v40<v39; ++v40 ) {
        *((byte*)&x1+k++) = v37;
        v37 += 4;
      }
      v42 = _mm_load_si128((const __m128i*)&x3);
      v110 = v39;
      v120 = v0;
      v112 = v4;
      v114 = 38-v0;
      v116 = v38-(((byte)v38-(byte)v39)&0xF);
      v43 = 4*v39+v4+1;
      v44 = 4*v39+v4+5;
      v45 = 4*v39+v4+9;
      v46 = 4*v39+v4+13;
      v47 = 4*v39+v4+17;
      v48 = 4*v39+v4+21;
      v49 = 4*v39+v4+25;
      v50 = v43+28;
      v51 = v43+32;
      v52 = v43+36;
      v53 = v43+40;
      v54 = v43+44;
      v55 = v43+48;
      v56 = v43+52;
      v118 = v43+56;
      v57 = v43+60;
      v58 = _mm_cvtsi32_si128(v43);
      v59 = v116;
      v60 = _mm_cvtsi32_si128(v44);
      v38 = v114;
      v61 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(v58, v60), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v45), _mm_cvtsi32_si128(v46)));
      v62 = _mm_cvtsi32_si128(v48);
      v0 = v120;
      v63 = _mm_cvtsi32_si128(v49);
      v4 = v112;
      v64 = _mm_cvtsi32_si128(v50);
      v65 = v110;
      v66 = v120+v110;
      v67 = _mm_unpacklo_epi64(_mm_unpacklo_epi32(v61, _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v47), v62), _mm_unpacklo_epi8(v63, v64))), _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v51), _mm_cvtsi32_si128(v52)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v53), _mm_cvtsi32_si128(v54))), _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v55), _mm_cvtsi32_si128(v56)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v118), _mm_cvtsi32_si128(v57)))));
      do {
        *(__int128*)((char*)&x1+v66) = (__int128)v67;
        v67 = _mm_add_epi8(v67, v42);
        v66 += 16;
        v65 += 16;
      } while( v65<v116 );
    }
    v68 = 4*v59+v4+1;
    for( m = v59+v0; v59<v38; ++v59 ) {
      *((byte*)&x1+m++) = v68;
      v68 += 4;
    }
  }
  v70 = 0;
  for( n = 0; n<0x80; ++n ) {
    v70 += *((byte*)&x1+v70)<n+1;
    *((byte*)&__arch_hdr[12]+n) = v70;
  }
  x4[0] = 0;
  x5[0] = 0;
  x6[0] = 0;
  x7[0] = 0;
  v72 = _mm_load_si128((const __m128i*)&x8);
  v73 = 12;
  do {
    x7[v73] = (__int128)v72;
    x6[v73] = (__int128)v72;
    x5[v73] = (__int128)v72;
    x4[v73] = (__int128)v72;
    v73 -= 4;
  } while( v73*16 );
  b1[0] = 1;
  v74 = 2;
  v75 = 2;
  //*(char*)(blob1+0x140029001-0x1400227B0) = 2;
  blob1[0x140029001-0x1400227B0] = 2;
  v76 = 1;
  result = 1;
  v78 = 1;
  for( ii = 2; ii<0x100; ++ii ) {
    b1[v74] = v75+1;
    if( !--v78 ) {
      ++v75;
      v78 = ++v76;
    }
    v74 = ii+1;
  }
  //*(char*)(blob1+0x1400290FF-0x1400227B0) = 24;
  blob1[0x1400290FF-0x1400227B0] = 24;
  BYTE2(x9[0]) = 2;
  LOWORD(x9[0]) = 512;
  *(qword*)((char*)x9+3) = 0x404040404040404LL;
  *(qword*)((char*)x9+11) = 0x404040404040404LL;
  //*(__int64*)(blob1+0x140029113-0x1400227B0) = 0x404040404040404LL;
  (__int64&)blob1[0x140029113-0x1400227B0] = 0x404040404040404LL;

  //*(int*)(blob1+0x14002911B-0x1400227B0) = 67372036;
  (int&)blob1[0x14002911B-0x1400227B0] = 67372036;

  //*(char*)(blob1+0x14002911F-0x1400227B0) = 4;
  blob1[0x14002911F-0x1400227B0] = 4;

  LODWORD(x10[0]) = 67372036;
  WORD2(x10[0]) = 1028;
  v80 = _mm_load_si128((const __m128i*)&x11);
  v81 = 12;
  do {
    x12[v81] = (__int128)v80;
    x10[v81] = (__int128)v80;
    *(__m128i*)((char*)&x13+v81*16) = v80;
    x9[v81] = (__int128)v80;
    v81 -= 4;
  } while( v81*16 );
  *(qword*)((char*)x10+6) = _mm_cvtsi128_si64(v80);
  *(qword*)((char*)x10+14) = _mm_cvtsi128_si64(v80);
  *(qword*)((char*)x12+6) = _mm_cvtsi128_si64(v80);
  HIWORD(x12[0]) = 1542;
  v82 = _mm_load_si128((const __m128i*)&x14);
  v83 = _mm_load_si128((const __m128i*)&x15);
  v84 = _mm_load_si128((const __m128i*)&x16);
  v85 = _mm_load_si128((const __m128i*)&x17);
  v86 = _mm_load_si128((const __m128i*)&x18);
  v87 = _mm_load_si128((const __m128i*)&x19);
  v88 = 0;
  v89 = _mm_setzero_si128();
  do {
    v90 = _mm_cmpeq_epi32(_mm_and_si128(v83, v84), v84);
    v91 = _mm_cmpeq_epi32(_mm_and_si128(v83, v82), _mm_setzero_si128());
    v92 = _mm_cmpeq_epi32(_mm_and_si128(v83, v85), _mm_setzero_si128());
    v93 = _mm_and_si128(v90, v91);
    v83 = _mm_add_epi32(v83, v82);
    v94 = _mm_or_si128(_mm_xor_si128(_mm_set1_epi32(-1), v90), v91);
    v95 = _mm_andnot_si128(v91, v90);
    v89 = _mm_and_si128(_mm_or_si128(_mm_and_si128(v86, v93), _mm_andnot_si128(v93, _mm_or_si128(_mm_and_si128(_mm_or_si128(_mm_and_si128(v89, _mm_or_si128(v94, v92)), _mm_and_si128(_mm_andnot_si128(v92, v95), v84)), _mm_or_si128(v94, _mm_xor_si128(_mm_set1_epi32(-1), v92))), _mm_and_si128(_mm_and_si128(v95, v92), v87)))), v90);
    x20[v88++] = (__int128)_mm_slli_epi32(v89, 9u);
  } while( v88<4 );
  v96 = 0;
  v97 = 0;
  do {
    b2[v96] = v97+1;
    v98 = v96++==(byte)b3[v97];
    v97 += v98;
  } while( v96<0x80 );
  v99 = 0;
  v100 = 0;
  do {
    b4[v99] = v100+1;
    v101 = v99++==(byte)b5[v100];
    v100 += v101;
  } while( v99<0x100 );
  v102 = 0;
  v103 = 0;
  do {
    b6[v102] = v103;
    v104 = v102++==(byte)b7[v103];
    v103 += v104;
  } while( v102<0x80 );
  v105 = 0;
  v106 = 0;
  do {
    b8[v105] = v106;
    v107 = v105++==(byte)b9[v106];
    v106 += v107;
  } while( v105<0x100 );
  return result;
}

PROBE_DECL(sub_140014894)
void sub_140014894(void* a1) {
  PROBE_HIT(sub_140014894);
  q1 = a1;
}

PROBE_DECL(sub_14001A5C0)
void sub_14001A5C0() {
  PROBE_HIT(sub_14001A5C0);
  d1 = 0;
}

PROBE_DECL(sub_140003370)
__int64 sub_140003370() {
  PROBE_HIT(sub_140003370);
  __int64 v0;
  void* v1;
  qword v2;
  __int64 v3;
  v0 = __PPMblock_size;
  if( !__PPMblock_size )
    return 0;
  if( d2==1 ) {
    v1 = __PPMblock;
    if( !(d3+d4) ) {
      if( (q2+(qword)(108*(uint)(__PPMblock_size/0x78uLL))-q3)/9<=q4-(__int64)__PPMblock )
        v2 = q4-(qword)__PPMblock;
      else
        v2 = (q2+(qword)(108*(uint)(__PPMblock_size/0x78uLL))-q3)/9;
      v3 = 5*v2;
      if( __PPMblock_size>(qword)(2*v3+12) )
        v0 = 2*v3+12;
    }
  } else {
    v1 = __PPMblock;
  }
  __PPMblock_size = 0;
  free(v1);
  // VFree(v1);
  return (uint)((qword)(v0+0xFFFFF)>>20);
}

PROBE_DECL(sub_1400032F0)
__int64 sub_1400032F0() {
  PROBE_HIT(sub_1400032F0);
  __int64 result;
  __int64 v1;
  uint i;
  int v3;
  if( !__PPMblock_size )
    return 0;
  v1 = 0;
  LODWORD(result) = q4-q5+q2+__PPMblock_size-q3;
  for( i = 0; i<0x26; ++i ) {
    v3 = *(uint*)(q6+12*v1)*12**((byte*)&x1+v1);
    v1 = i+1;
    result = (uint)(result-v3);
  }
  return result;
}

PROBE_DECL(sub_140012804)
char (**sub_140012804())[4096] {
  PROBE_HIT(sub_140012804);
  return &__off_140027060;
}
PROBE_DECL(sub_140017A58)
void sub_140017A58() {
  PROBE_HIT(sub_140017A58);
  void(**i)(void);
  for( i = (void(**)(void))&q7; i<(void(**)(void))&q7; ++i ) {
    if( *i )
      (*i)();
  }
}

PROBE_DECL(sub_14000C0E0)
__int64 sub_14000C0E0(byte* a1) {
  PROBE_HIT(sub_14000C0E0);
  __int64 v2;
  int v3;
  int v4;
  byte* v5;
  byte* v6;
  __int64 v7;
  __int64 v8;
  byte* i;
  int v10;
  byte* j;
  uint v12;
  int v13;
  uint v14;
  uint v15;
  int v16;
  int v17;
  int v18;
  int v19;
  uint v20;
  byte v21;
  uint v22;
  __int64 v23;
  short v24;
  int v25;
  int v26;
  byte* v27;
  v2 = q8;
  v3 = *a1;
  v4 = v3+1;
  v5 = (byte*)(q8+*((uint*)a1+1)+6LL*(a1[1]&0xF));
  v6 = (byte*)(q8+*((uint*)a1+2));
  v7 = *((uint*)v6+1);
  v8 = *v6;
  for( i = (byte*)(q8+v7+6*v8); !i[1]; i = (byte*)(q8+v7+6*v8) ) {
    if( *i!=*v5 )
      break;
    v6 = (byte*)(q8+*((uint*)v6+2));
    v7 = *((uint*)v6+1);
    v8 = *v6;
  }
  if( !*(byte*)(v7+q8+6LL*(v6[1]&0xF)+1) ) {
    sub_14000C0E0(v6);
    v7 = *((uint*)v6+1);
  }
  v10 = *v5;
  for( j = (byte*)(v2+v7); *j!=v10; j += 6 )
    ;
  v12 = j[1];
  v13 = *((word*)a1+1);
  v14 = v13-v12+2*v8+2+*((word*)v6+1);
  if( v12>=6 )
    v15 = v12-1;
  else
    v15 = ((3-v12)>>30)|1;
  v16 = 4*v3+4;
  v17 = v8+1-v16;
  if( v17<-4 ) {
    v19 = 0;
  } else {
    v18 = v17+4;
    v19 = 16*v4;
    if( v18<16*v4 )
      v19 = v18;
  }
  v20 = 3*(v19+v13+v16)*v15/v14;
  if( v20>=0x24 ) {
    v21 = 11;
  } else {
    v21 = b10[v20];
    if( v20<=0x21 ) {
LABEL_18:
      v5[1] = v21;
      goto LABEL_26;
    }
  }
  if( v13>=32*v4 )
    goto LABEL_18;
  v22 = 23;
  if( v13>=2*(3*v3+3) )
    v22 = 7;
  if( v20/6-4<v22 )
    v22 = v20/6-4;
  v21 += v22;
  v5[1] = v21;
LABEL_26:
  v23 = *((uint*)a1+1);
  for( *((word*)a1+1) = v21+(word)v13; v5!=(byte*)(v2+v23); v5 -= 6 ) {
    if( v5[1]<=*(v5-5)+1 )
      break;
    v24 = *(word*)v5;
    v25 = *(uint*)(v5+2);
    v26 = *((uint*)v5-1);
    *(word*)v5 = *((word*)v5-3);
    *(uint*)(v5+2) = v26;
    *((word*)v5-3) = v24;
    *((uint*)v5-1) = v25;
    v23 = *((uint*)a1+1);
  }
  v27 = &v5[-v23-v2];
  a1[1] = ((__int64)v27/6)|a1[1]&0xF0;
  return 0x2AAAAAAAAAAAAAABLL*(qword)v27;
}

PROBE_DECL(sub_14000B750)
__int64 sub_14000B750(byte* a1) {
  PROBE_HIT(sub_14000B750);
  int v2;
  __int64 v3;
  __int64 v4;
  uint v5;
  int v6;
  char v7;
  int v8;
  int v9;
  int v10;
  __int64 v11;
  short v12;
  int v13;
  int v14;
  byte* v15;
  byte* v16;
  int v17;
  byte* i;
  int v19;
  __int64 result;
  byte v21;
  __int64 v22;
  __int64 v23;
  byte v24;
  uint v25;
  __int64 v26;
  __int64 v27;
  uint v28;
  __int64 v29;
  uint* v30;
  __int64 v31;
  byte* v32;
  __int64 v33;
  uint v34;
  uint v35;
  int v36;
  __int64 v37;
  uint v38;
  __int64 v39;
  __int64 v40;
  int v41;
  uint* v42;
  __int64 v43;
  uint* v44;
  int v45;
  __int64 v46;
  __int64 v47;
  uint v48;
  uint v49;
  uint v50;
  __int64 v51;
  uint* v52;
  __int64 v53;
  uint* v54;
  int v55;
  __int64 v56;
  uint v57;
  uint v58;
  uint v59;
  uint v60;
  __int64 v61;
  __int64 v62;
  __int64 v63;
  uint v64;
  uint v65;
  int v66;
  __int64 v67;
  uint v68;
  __int64 v69;
  __int64 v70;
  int v71;
  uint* v72;
  __int64 v73;
  int v74;
  __int64 v75;
  __int64 v76;
  __int64 v77;
  uint v78;
  __int64 v79;
  __int64 v80;
  byte* j;
  __int64 v82;
  uint v83;
  uint v84;
  int v85;
  __int64 v86;
  uint v87;
  __int64 v88;
  __int64 v89;
  int v90;
  uint* v91;
  __int64 v92;
  int v93;
  uint v94;
  word v95;
  int v96;
  v2 = *a1;
  v3 = q8;
  v4 = *((uint*)a1+1);
  v5 = v2+1;
  v6 = (byte)(a1[1]&0x40)>>6;
  v7 = v6+1;
  v8 = 2*v6+2;
  v9 = (1<<(v6+1))-1;
  v10 = -v6;
  if( d5!=d6 )
    v10 = v8;
  v11 = q9;
  if( q9!=q8+v4 ) {
    do {
      v12 = *(word*)v11;
      v13 = *(uint*)(v11+2);
      v14 = *(uint*)(v11-4);
      *(word*)v11 = *(word*)(v11-6);
      *(uint*)(v11+2) = v14;
      *(word*)(v11-6) = v12;
      *(uint*)(v11-4) = v13;
      v11 -= 6;
    } while( v11!=v3+*((uint*)a1+1) );
  }
  v15 = (byte*)(v11+6LL*v5);
  v16 = v15-6;
  a1[1] = 0;
  *((word*)a1+1) = 0;
  do {
    v15 -= 6;
    v17 = (v10+v9*v15[1])>>v7;
    v15[1] = v17;
    *((word*)a1+1) += (byte)v17;
    if( v15[1] ) {
      a1[1] |= *((byte*)x4+*v15);
    } else {
      for( i = v15; i<v16; i += 6 ) {
        v19 = *((uint*)i+2);
        *(word*)i = *((word*)i+3);
        *(uint*)(i+2) = v19;
      }
      i[1] = 0;
    }
    --v5;
  } while( v5 );
  result = v16[1];
  if( v16[1] ) {
    v94 = *((uint*)a1+1);
LABEL_61:
    a1[1] |= 0x40u;
    q9 = v94+v3;
    return result;
  }
  do {
    LOBYTE(v5) = v5+1;
    v16 -= 6;
  } while( !v16[1] );
  v21 = *a1-v5;
  *a1 = v21;
  if( v21 ) {
    v46 = *((uint*)a1+1);
    v47 = v3+v46;
    v48 = v21+2;
    v49 = *((byte*)&__arch_hdr[11]+((uint)(v2+2)>>1)+3);
    result = (v48>>1)-1;
    v50 = *((byte*)&__arch_hdr[12]+result);
    if( v49!=v50 ) {
      v51 = q6;
      v52 = (uint*)(q6+12LL*v50);
      if( *v52 ) {
        v53 = (uint)v52[1];
        v54 = (uint*)(v3+v53);
        v52[1] = *(uint*)(v3+v53+4);
        *(uint*)(*(uint*)(v3+v53+4)+v3+8) = (uint)(uintptr_t)v52-v3;
        v55 = v3+v53;
        --*v52;
        v56 = v3+v46;
        if( ((v48>>1)&1)!=0 ) {
          *v54 = *(uint*)v47;
          v54[1] = *(uint*)(v47+4);
          v54[2] = *(uint*)(v47+8);
          v54 = (uint*)(v3+v53+12);
          v56 = v3+v46+12;
        }
        v57 = v48>>2;
        if( v57 ) {
          v58 = 0;
          v59 = 0;
          do {
            v54[v59] = *(uint*)(v56+v59*4);
            ++v58;
            v54[v59+1] = *(uint*)(v56+v59*4+4);
            v54[v59+2] = *(uint*)(v56+v59*4+8);
            v54[v59+3] = *(uint*)(v56+v59*4+12);
            v54[v59+4] = *(uint*)(v56+v59*4+16);
            v54[v59+5] = *(uint*)(v56+v59*4+20);
            v59 += 6;
          } while( v58<v57 );
        }
        v60 = *((byte*)&x1+v49);
        v61 = *((byte*)&x1+v49);
        if( *(byte*)(12*v61+v47+1)==255 ) {
          v62 = 12*v61;
          do {
            *(uint*)(*(uint*)(v62+v47+4)+v3+8) = *(uint*)(v62+v47+8);
            *(uint*)(*(uint*)(v62+v47+8)+v3+4) = *(uint*)(v62+v47+4);
            v63 = 12LL**((byte*)&__arch_hdr[11]+*(byte*)(v62+v47)+3);
            --*(uint*)(v63+v51);
            v60 += *(byte*)(v47+v62);
            v62 = 12LL*v60;
          } while( *(byte*)(v62+v47+1)==255 );
          v64 = v60;
          if( v60>0x80 ) {
            v65 = 0;
            v66 = 0;
            v67 = -((__int64)(((qword)((1LL-v60)>>6)>>57)-v60+1)>>7);
            do {
              v60 = v66+v64-128;
              v66 -= 128;
              ++v65;
            } while( v65<(uint)v67 );
            v68 = 0;
            do {
              *(byte*)(v47+1) = -1;
              *(uint*)(v47+8) = v51-v3+444;
              ++v68;
              *(byte*)v47 = 0x80;
              *(uint*)(v47+4) = *(uint*)(v51+448);
              *(uint*)(*(uint*)(v51+448)+v3+8) = v46;
              ++*(uint*)(v51+444);
              *(uint*)(v51+448) = v46;
              v46 += 1536;
              v47 += 1536;
            } while( v68<(uint)v67 );
          }
          v69 = *((byte*)&__arch_hdr[11]+v60+3);
          LODWORD(v70) = *((byte*)&x1+v69);
          if( v60!=(uint)v70 ) {
            v69 = (uint)(v69-1);
            v70 = *((byte*)&x1+v69);
            v71 = v60-v70;
            v72 = (uint*)(v51+12LL*(uint)(v71-1));
            v73 = v47+12*v70;
            *(byte*)(v73+1) = -1;
            *(byte*)v73 = v71;
            *(uint*)(v73+8) = (uint)(uintptr_t)v72-v3;
            *(uint*)(v73+4) = v72[1];
            LODWORD(v73) = v73-v3;
            *(uint*)((uint)v72[1]+v3+8) = v73;
            v72[1] = v73;
            ++*v72;
          }
          *(byte*)(v47+1) = -1;
          result = 12*v69+v51;
          *(byte*)v47 = v70;
          *(uint*)(v47+8) = result-v3;
          *(uint*)(v47+4) = *(uint*)(result+4);
          v74 = v47-v3;
          *(uint*)(*(uint*)(result+4)+v3+8) = v74;
          *(uint*)(result+4) = v74;
          ++*(uint*)result;
        } else {
          *(byte*)(v47+1) = -1;
          result = 12LL*v49+v51;
          *(byte*)v47 = v60;
          *(uint*)(v47+8) = result-v3;
          *(uint*)(v47+4) = *(uint*)(result+4);
          *(uint*)(*(uint*)(result+4)+v3+8) = v46;
          *(uint*)(result+4) = v46;
          ++*(uint*)result;
        }
        LODWORD(v47) = v55;
      } else {
        v75 = *((byte*)&x1+v50);
        v76 = v47+12*v75;
        v77 = v76-v3;
        v78 = *((byte*)&x1+v49)-(uint)v75;
        v79 = v78;
        v80 = 12LL*v78;
        for( j = (byte*)(v76+v80); *(byte*)(v76+v80+1)==255; j = (byte*)(v76+v80) ) {
          *(uint*)(*((uint*)j+1)+v3+8) = *((uint*)j+2);
          *(uint*)(*((uint*)j+2)+v3+4) = *((uint*)j+1);
          v82 = 12LL**((byte*)&__arch_hdr[11]+*j+3);
          --*(uint*)(v82+v51);
          v78 += *(byte*)(v76+v80);
          v79 = v78;
          v80 = 12LL*v78;
        }
        v83 = v78;
        if( v78>0x80 ) {
          v84 = 0;
          v85 = 0;
          v86 = -((__int64)(((qword)((1-v79)>>6)>>57)-v79+1)>>7);
          do {
            v78 = v85+v83-128;
            v85 -= 128;
            ++v84;
          } while( v84<(uint)v86 );
          v87 = 0;
          do {
            *(byte*)(v76+1) = -1;
            *(uint*)(v76+8) = v51-v3+444;
            ++v87;
            *(byte*)v76 = 0x80;
            *(uint*)(v76+4) = *(uint*)(v51+448);
            *(uint*)(*(uint*)(v51+448)+v3+8) = v77;
            ++*(uint*)(v51+444);
            *(uint*)(v51+448) = v77;
            v77 += 1536;
            v76 += 1536;
          } while( v87<(uint)v86 );
        }
        v88 = *((byte*)&__arch_hdr[11]+v78+3);
        LODWORD(v89) = *((byte*)&x1+v88);
        if( v78!=(uint)v89 ) {
          v88 = (uint)(v88-1);
          v89 = *((byte*)&x1+v88);
          v90 = v78-v89;
          v91 = (uint*)(v51+12LL*(uint)(v90-1));
          v92 = v76+12*v89;
          *(byte*)(v92+1) = -1;
          *(byte*)v92 = v90;
          *(uint*)(v92+8) = (uint)(uintptr_t)v91-v3;
          *(uint*)(v92+4) = v91[1];
          LODWORD(v92) = v92-v3;
          *(uint*)((uint)v91[1]+v3+8) = v92;
          v91[1] = v92;
          ++*v91;
        }
        *(byte*)(v76+1) = -1;
        result = 12*v88+v51;
        *(byte*)v76 = v89;
        *(uint*)(v76+8) = result-v3;
        *(uint*)(v76+4) = *(uint*)(result+4);
        v93 = v76-v3;
        *(uint*)(*(uint*)(result+4)+v3+8) = v93;
        *(uint*)(result+4) = v93;
        ++*(uint*)result;
      }
    }
    v94 = v47-v3;
    *((uint*)a1+1) = v94;
    goto LABEL_61;
  }
  v22 = *((uint*)a1+1);
  v23 = v3+v22;
  v95 = *(word*)(v3+v22);
  v96 = *(uint*)(v3+v22+2);
  v24 = (v2+((v95>>7)&0xFFFFFFFE))/(__int64)(v2+1);
  v25 = v2+2;
  if( v24>=0x2Cu )
    v24 = 44;
  HIBYTE(v95) = v24;
  v26 = q6;
  v27 = *((byte*)&__arch_hdr[11]+(v25>>1)+3);
  v28 = *((byte*)&x1+v27);
  v29 = *((byte*)&x1+v27);
  if( *(byte*)(12*v29+v23+1)==255 ) {
    v31 = 12*v29;
    v32 = (byte*)(v23+v31);
    do {
      *(uint*)(*((uint*)v32+1)+v3+8) = *((uint*)v32+2);
      *(uint*)(*((uint*)v32+2)+v3+4) = *((uint*)v32+1);
      v33 = 12LL**((byte*)&__arch_hdr[11]+*v32+3);
      --*(uint*)(v33+v26);
      v28 += *(byte*)(v23+v31);
      v31 = 12LL*v28;
      v32 = (byte*)(v23+v31);
    } while( *(byte*)(v23+v31+1)==255 );
    v34 = v28;
    if( v28>0x80 ) {
      v35 = 0;
      v36 = 0;
      v37 = -((__int64)(((qword)((1LL-v28)>>6)>>57)-v28+1)>>7);
      do {
        v28 = v36+v34-128;
        v36 -= 128;
        ++v35;
      } while( v35<(uint)v37 );
      v38 = 0;
      do {
        *(byte*)(v23+1) = -1;
        *(uint*)(v23+8) = v26-v3+444;
        ++v38;
        *(byte*)v23 = 0x80;
        *(uint*)(v23+4) = *(uint*)(v26+448);
        *(uint*)(*(uint*)(v26+448)+v3+8) = v22;
        ++*(uint*)(v26+444);
        *(uint*)(v26+448) = v22;
        v22 += 1536;
        v23 += 1536;
      } while( v38<(uint)v37 );
    }
    v39 = *((byte*)&__arch_hdr[11]+v28+3);
    LODWORD(v40) = *((byte*)&x1+v39);
    if( v28!=(uint)v40 ) {
      v39 = (uint)(v39-1);
      v40 = *((byte*)&x1+v39);
      v41 = v28-v40;
      v42 = (uint*)(v26+12LL*(uint)(v41-1));
      v43 = v23+12*v40;
      *(byte*)(v43+1) = -1;
      *(byte*)v43 = v41;
      *(uint*)(v43+8) = (uint)(uintptr_t)v42-v3;
      *(uint*)(v43+4) = v42[1];
      LODWORD(v43) = v43-v3;
      *(uint*)((uint)v42[1]+v3+8) = v43;
      v42[1] = v43;
      ++*v42;
    }
    *(byte*)(v23+1) = -1;
    v44 = (uint*)(12*v39+v26);
    *(byte*)v23 = v40;
    *(uint*)(v23+8) = (uint)(uintptr_t)v44-v3;
    *(uint*)(v23+4) = v44[1];
    v45 = v23-v3;
    *(uint*)((uint)v44[1]+v3+8) = v45;
    v44[1] = v45;
    ++*v44;
  } else {
    *(byte*)(v23+1) = -1;
    v30 = (uint*)(v26+12*v27);
    *(byte*)v23 = v28;
    *(uint*)(v23+8) = (uint)(uintptr_t)v30-v3;
    *(uint*)(v23+4) = v30[1];
    *(uint*)((uint)v30[1]+v3+8) = v22;
    v30[1] = v22;
    ++*v30;
  }
  q9 = (__int64)(a1+2);
  *((word*)a1+1) = v95;
  *((uint*)a1+1) = v96;
  a1[1] = *((byte*)x4+(byte)v95);
  return v95;
}

PROBE_DECL(sub_140010730)
__int64 sub_140010730(__int64 a1) {
  PROBE_HIT(sub_140010730);
  uint v1;
  uint v2;
  uint v3;
  uint v4;
  uint v5;
  uint v6;
  uint v7;
  __int64 result;
  uint v9;
  short v10;
  v1 = *(word*)(a1+4);
  v2 = *(word*)(a1+6)*(*(uint*)a1/(11*v1)+1);
  if( v2>=0x800 )
    LOWORD(v2) = 2048;
  v3 = *(byte*)q10+1;
  v4 = *(word*)(a1+4)/v3;
  v5 = (256-v3)**(word*)(q10+2)/v3;
  v6 = 16*v3;
  v7 = v4*v5;
  if( *(uint*)a1<v7 )
    v7 = *(uint*)a1;
  if( v1>v6 )
    v6 = *(word*)(a1+4);
  result = v7-(v7>>1);
  v9 = v6-(v6>>1);
  if( (word)v2<=0x400u ) {
    *(word*)(a1+4) = v9;
    *(uint*)a1 = result;
    if( (word)v2<=0x200u ) {
      if( (word)v2<=0x40u ) {
        result = (32-(uint)(word)v2)>>31;
        v10 = v2-result;
      } else {
        v10 = v2-32;
      }
      *(word*)(a1+6) = v10;
    } else {
      *(word*)(a1+6) = v2-128;
    }
  } else {
    *(uint*)a1 = result-((uint)result>>1);
    *(word*)(a1+4) = v9-((word)v9>>1);
    *(word*)(a1+6) = (word)v2>>1;
    return (word)v9>>1;
  }
  return result;
}

PROBE_DECL(sub_140010850)
__int64 sub_140010850(__int64 a1) {
  PROBE_HIT(sub_140010850);
  int v2;
  int v3;
  int v4;
  uint v5;
  char v6;
  uint v7;
  int v8;
  uint v9;
  int v10;
  uint v11;
  __int64 result;
  int v13;
  uint v14;
  int v15;
  uint v16;
  int v17;
  v2 = *(word*)(a1+6);
  v3 = HIWORD(*(uint*)a1);
  v4 = (word)*(uint*)a1;
  v5 = *(word*)(a1+4);
  v6 = 1;
  v7 = ((v5>>1)+(v4<<15))/v5;
  if( v7>=0x7FFF )
    v7 = 0x7FFF;
  if( v7<=1 )
    v7 = 1;
  v8 = (v3-v7)*(v3-v7);
  v9 = (0x10000-(v7+v3))*(v7+v3);
  v10 = 4096;
  if( v2<4096 )
    v10 = v2;
  v11 = (v2*(v9>>11))>>4;
  if( v10<=24||v10==4096 )
    v6 = 14;
  if( 2*v8<=3*v11 ) {
    result = v10*(((v11>>v6)+5*v8)/v11+1);
    if( (uint)result>=0x2000 )
      result = 0x2000;
  } else {
    result = 0x2000;
  }
  if( (word)result<=0x40u&&(word)result<=(uint)v2 ) {
    if( (word)result<=0x20u )
      LOWORD(v13) = result-((1-(uint)(word)result)>>31)-((24-(uint)(word)result)>>31);
    else
      LOWORD(v13) = result-8;
    *(word*)(a1+6) = v13;
  } else {
    v13 = (word)result>>1;
    *(word*)(a1+6) = v13;
  }
  if( (word)v13<=2u ) {
    v15 = 17*(8*v4<v5);
    v16 = 8*(v5-v4)<v5;
    v17 = 2*v5;
    result = (uint)(1431655766*v17);
    v14 = (-6*v16+v15+2*v4)/3u;
    *(word*)(a1+4) = (unsigned)v17/3u;
  } else {
    *(word*)(a1+4) = v5>>1;
    v14 = (uint)(v4+ -12*(6*(v5-v4)<v5)+16*(uint)(4*v4<v5))>>1;
  }
  *(uint*)a1 = (v7<<16)|v14;
  return result;
}

PROBE_DECL(sub_140011240)
__int64 sub_140011240(uint a1) {
  PROBE_HIT(sub_140011240);
  __int64 v1;
  __int64 v2;
  uint v3;
  uint* v4;
  __int64 v5;
  __int64 result;
  int v7;
  __int64 v8;
  __int64 v9;
  uint v10;
  __int64 v11;
  __int64 v12;
  byte* i;
  __int64 v14;
  uint v15;
  uint v16;
  int v17;
  __int64 v18;
  int v19;
  uint v20;
  __int64 v21;
  __int64 v22;
  int v23;
  uint* v24;
  int v25;
  __int64 v26;
  uint* v27;
  int v28;
  int v29;
  __int64 v30;
  char* v31;
  __int64 v32;
  int v33;
  __int64 v34;
  uint v35;
  int v36;
  uint* v37;
  int v38;
  int v39;
  __int64 v40;
  char* v41;
  __int64 v42;
  uint v43;
  __int64 v44;
  uint* v45;
  int v46;
  int v47;
  byte* v48;
  __int64 v49;
  __int64 v50;
  uint v51;
  uint v52;
  int v53;
  __int64 v54;
  int v55;
  uint v56;
  __int64 v57;
  __int64 v58;
  int v59;
  uint* v60;
  byte* v61;
  uint* v62;
  int v63;
  int v64;
  int v65;
  __int64 v66;
  uint* v67;
  __int64 v68;
  bool v69;
  int v70;
  __int64 v71;
  __int64 v72;
  v1 = q6;
  v2 = a1;
  v3 = *((byte*)&x1+a1);
  while( ++a1!=38 ) {
    v4 = (uint*)(q6+12LL*a1);
    if( *v4 ) {
      v5 = q8;
      result = q8+(uint)v4[1];
      v4[1] = *(uint*)(result+4);
      *(uint*)(v5+*(uint*)(result+4)+8) = (uint)(uintptr_t)v4-v5;
      v7 = *((byte*)&x1+a1);
      --*v4;
      v8 = result+12LL*v3;
      v9 = v8-v5;
      v10 = v7-v3;
      v11 = v10;
      v12 = 12LL*v10;
      for( i = (byte*)(v8+v12); *(byte*)(v8+v12+1)==255; i = (byte*)(v8+v12) ) {
        *(uint*)(*((uint*)i+1)+v5+8) = *((uint*)i+2);
        *(uint*)(*((uint*)i+2)+v5+4) = *((uint*)i+1);
        v14 = 12LL**((byte*)&__arch_hdr[11]+*i+3);
        --*(uint*)(v14+v1);
        v10 += *(byte*)(v8+v12);
        v11 = v10;
        v12 = 12LL*v10;
      }
      v15 = v10;
      if( v10>0x80 ) {
        v16 = 0;
        v17 = 0;
        v18 = -((__int64)(((qword)((1-v11)>>6)>>57)-v11+1)>>7);
        do {
          v10 = v17+v15-128;
          v17 -= 128;
          ++v16;
        } while( v16<(uint)v18 );
        v19 = *(uint*)(v1+448);
        v20 = 0;
        do {
          *(byte*)(v8+1) = -1;
          *(uint*)(v8+8) = v1-v5+444;
          *(uint*)(v8+4) = v19;
          v19 = v9;
          *(byte*)v8 = 0x80;
          v9 += 1536;
          v8 += 1536;
          *(uint*)(v5+*(uint*)(v1+448)+8) = v19;
          ++*(uint*)(v1+444);
          *(uint*)(v1+448) = v19;
          ++v20;
        } while( v20<(uint)v18 );
      }
      v21 = *((byte*)&__arch_hdr[11]+v10+3);
      LODWORD(v22) = *((byte*)&x1+v21);
      if( v10!=(uint)v22 ) {
        v21 = (uint)(v21-1);
        v22 = *((byte*)&x1+v21);
        v23 = v10-v22;
        v24 = (uint*)(v1+12LL*(uint)(v23-1));
        v25 = v24[1];
        v26 = v8+12*v22;
        *(byte*)v26 = v23;
        *(byte*)(v26+1) = -1;
        *(uint*)(v26+8) = (uint)(uintptr_t)v24-v5;
        *(uint*)(v26+4) = v25;
        LODWORD(v26) = v26-v5;
        *(uint*)((uint)v24[1]+v5+8) = v26;
        v24[1] = v26;
        ++*v24;
      }
      *(byte*)(v8+1) = -1;
      v27 = (uint*)(12*v21+v1);
      v28 = v27[1];
      *(byte*)v8 = v22;
      *(uint*)(v8+8) = (uint)(uintptr_t)v27-v5;
      *(uint*)(v8+4) = v28;
      v29 = v8-v5;
      *(uint*)((uint)v27[1]+v5+8) = v29;
      v27[1] = v29;
      ++*v27;
      return result;
    }
  }
  if( d3 ) {
    if( d4 )
      return 0;
    v30 = 12*v3;
    if( q5-v30<=(qword)q4 ) {
      return 0;
    } else {
      result = q5-v30;
      q5 -= v30;
    }
  } else {
    v31 = (char*)__PPMblock+__PPMblock_size-12;
    v32 = q8;
    v70 = *((uint*)v31+2);
    v71 = *(qword*)v31;
    v33 = (uint)(uintptr_t)__PPMblock+__PPMblock_size-12-q8;
    v72 = v2;
    v34 = 0;
    v35 = 0;
    v36 = q6-q8+444;
    do {
      v31[1] = -2;
      v37 = (uint*)(v1+12*v34);
      v38 = v37[1];
      *v31 = 1;
      v39 = v1+12*v34-v32;
      *((uint*)v31+2) = v39;
      *((uint*)v31+1) = v38;
      *(uint*)((uint)v37[1]+v32+8) = v33;
      v40 = (uint)v37[2];
      ++*v37;
      v37[1] = v33;
      v41 = (char*)(v32+v40);
      v37[2] = *((uint*)v41+2);
      *(uint*)(*((uint*)v41+2)+v32+4) = v39;
      --*v37;
      for(; v41!=v31; --*v37 ) {
        v42 = *((byte*)&__arch_hdr[11]+(uint)(byte)*v41+3);
        _mm_prefetch((const char*)(*(uint*)((uint)v37[2]+v32+8)+v32+4), 1);
        v43 = *((byte*)&x1+v42);
        v44 = *((byte*)&x1+v42);
        if( (byte)v41[12*v44+1]==255 ) {
          v48 = (byte*)&v41[-v32];
          v49 = 12*v44;
          do {
            *(uint*)(*(uint*)&v41[v49+4]+v32+8) = *(uint*)&v41[v49+8];
            *(uint*)(*(uint*)&v41[v49+8]+v32+4) = *(uint*)&v41[v49+4];
            v50 = 12LL**((byte*)&__arch_hdr[11]+(byte)v41[v49]+3);
            --*(uint*)(v50+v1);
            v43 += (byte)v41[v49];
            v49 = 12LL*v43;
          } while( (byte)v41[v49+1]==255 );
          if( v43>0x80 ) {
            v51 = v43;
            v52 = 0;
            v53 = 0;
            v54 = -((__int64)(((qword)((1LL-v43)>>6)>>57)-v43+1)>>7);
            do {
              v43 = v53+v51-128;
              v53 -= 128;
              ++v52;
            } while( v52<(uint)v54 );
            v55 = *(uint*)(v1+448);
            v56 = 0;
            do {
              v41[1] = -1;
              *((uint*)v41+2) = v36;
              *((uint*)v41+1) = v55;
              v55 = (int)(uintptr_t)v48;
              *v41 = 0x80;
              v48 += 1536;
              v41 += 1536;
              *(uint*)(v32+*(uint*)(v1+448)+8) = v55;
              ++*(uint*)(v1+444);
              *(uint*)(v1+448) = v55;
              ++v56;
            } while( v56<(uint)v54 );
          }
          v57 = *((byte*)&__arch_hdr[11]+v43+3);
          LODWORD(v58) = *((byte*)&x1+v57);
          if( v43!=(uint)v58 ) {
            v57 = (uint)(v57-1);
            v58 = *((byte*)&x1+v57);
            v59 = v43-v58;
            v60 = (uint*)(v1+12LL*(uint)(v59-1));
            v61 = (byte*)&v41[12*v58];
            v61[1] = -1;
            *v61 = v59;
            *((uint*)v61+2) = v1+12*(v59-1)-v32;
            *((uint*)v61+1) = v60[1];
            LODWORD(v61) = (uint)(uintptr_t)v61-v32;
            *(uint*)((uint)v60[1]+v32+8) = (uint)(uintptr_t)v61;
            v60[1] = (uint)(uintptr_t)v61;
            ++*v60;
          }
          v41[1] = -1;
          v62 = (uint*)(v1+12*v57);
          v63 = v62[1];
          *v41 = v58;
          *((uint*)v41+2) = (uint)(uintptr_t)v62-v32;
          *((uint*)v41+1) = v63;
          v64 = (uint)(uintptr_t)v41-v32;
          *(uint*)((uint)v62[1]+v32+8) = v64;
          v62[1] = v64;
          ++*v62;
        } else {
          v41[1] = -1;
          v45 = (uint*)(v1+12*v42);
          v46 = v45[1];
          *v41 = v43;
          *((uint*)v41+2) = (uint)(uintptr_t)v45-v32;
          *((uint*)v41+1) = v46;
          v47 = (uint)(uintptr_t)v41-v32;
          *(uint*)((uint)v45[1]+v32+8) = v47;
          v45[1] = v47;
          ++*v45;
        }
        v41 = (char*)(v32+(uint)v37[2]);
        v37[2] = *((uint*)v41+2);
        *(uint*)(*((uint*)v41+2)+v32+4) = v39;
      }
      v34 = ++v35;
    } while( v35<0x26 );
    *(qword*)v31 = v71;
    *((uint*)v31+2) = v70;
    v65 = *((byte*)&x1+v72);
    d3 = 1;
    v66 = *((byte*)&__arch_hdr[12]+(uint)(v65-1));
    v67 = (uint*)(12*v66+v1);
    if( *v67 ) {
      result = v32+(uint)v67[1];
      v67[1] = *(uint*)(result+4);
      *(uint*)(*(uint*)(result+4)+v32+8) = (uint)(uintptr_t)v67-v32;
      --*v67;
    } else {
      result = q2;
      v68 = 12*(uint)*((byte*)&x1+v66);
      v69 = q2+v68==q3;
      if( q2+v68>(qword)q3 ) {
        return sub_140011240((uint)v66);
      } else {
        q2 += v68;
        if( !v69 )
          *(uint*)(v68+result) = 0;
      }
    }
  }
  return result;
}

PROBE_DECL(sub_140010A40)
__int64 sub_140010A40(int a1) {
  PROBE_HIT(sub_140010A40);
  int v2;
  __int64 v3;
  __int64 result;
  uint v5;
  uint* v6;
  __int64 v7;
  uint v8;
  char* v9;
  char* v10;
  char* v11;
  __int64 v12;
  __int64 v13;
  int v14;
  __int64 v15;
  __int64 v16;
  uint* v17;
  __int64 v18;
  __int64 v19;
  bool v20;
  __int64 v21;
  int v22;
  int v23;
  __m128i si128;
  qword v25;
  __int64 v26;
  int v27;
  __int64 v28;
  int v29;
  __int64 v30;
  int v31;
  int v32;
  int v33;
  __int64 v34;
  __int64 v35;
  int v36;
  int v37;
  __int64 v38;
  int v39;
  __int64 v40;
  int v41;
  int v42;
  int v43;
  __int64 v44;
  __int64 v45;
  int v46;
  int v47;
  __int64 v48;
  __int64 v49;
  __int64 v50;
  __int64 v51;
  __int64 v52;
  __int64 v53;
  __int64 v54;
  __int64 v55;
  __int64 v56;
  int v57;
  __int64 v58;
  __int64 v59;
  __int64 v60;
  __int64 v61;
  __int64 v62;
  int v63;
  __int64 v64;
  __int64 i;
  __int64 j;
  __int64 k;
  v2 = 0;
  d5 = 0;
  d7 = 0;
  v3 = 64;
  do {
    x21[v3] = 0;
    x22[v3] = 0;
    x23[v3] = 0;
    x24[v3] = 0;
    v3 -= 4;
  } while( v3*16 );
  d8 = 1;
  if( a1!=1||d9==-100 ) {
    v6 = (uint*)__PPMblock;
    v7 = __PPMblock_size;
    q6 = (__int64)__PPMblock;
    v8 = 0;
    ++d2;
    q4 = (__int64)__PPMblock+456;
    v9 = (char*)__PPMblock+__PPMblock_size;
    d3 = 0;
    d4 = 0;
    v10 = (char*)__PPMblock+__PPMblock_size+ -108*(int)(__PPMblock_size/0x78uLL);
    q5 = (__int64)v10;
    q2 = (__int64)v10;
    *(uint*)v10 = 0;
    v11 = (char*)v6-1;
    q8 = (__int64)v6-1;
    v12 = 0;
    do {
      v13 = 3*v12;
      v14 = 12*v12+1;
      v6[v13+2] = v14;
      v6[v13+1] = v14;
      v12 = ++v8;
      v6[v13] = 0;
    } while( v8<0x26 );
    if( v9==v10 ) {
      v20 = *v6==0;
      q3 = (__int64)v9;
      if( v20 ) {
        v15 = sub_140011240(0);
      } else {
        v15 = (__int64)v6+(uint)v6[2]-1;
        v6[2] = *(uint*)(v15+8);
        *(uint*)&v11[*(uint*)(v15+8)+4] = 1;
        --*v6;
      }
    } else {
      v9 = (char*)v6+v7-12;
      q3 = (__int64)v9;
      v15 = (__int64)v9;
    }
    *(uint*)(v15+8) = 0;
    q11 = v15;
    *(byte*)(v15+1) = -57;
    q10 = v15;
    *(word*)(v15+2) = 256;
    v16 = (byte)b11;
    d10 = 255;
    *(byte*)v15 = -1;
    v17 = &v6[3*v16];
    if( *v17 ) {
      v18 = (__int64)v6+(uint)v17[1]-1;
      v17[1] = *(uint*)(v18+4);
      *(uint*)&v11[*(uint*)(v18+4)+8] = (uint)(uintptr_t)v17-(uint)(uintptr_t)v11;
      --*v17;
    } else {
      v18 = q2;
      v19 = 12*(uint)*((byte*)&x1+v16);
      v20 = q2+v19==(qword)v9;
      if( q2+v19>(qword)v9 ) {
        v18 = sub_140011240(v16);
      } else {
        q2 += v19;
        if( !v20 )
          *(uint*)(v19+v18) = 0;
      }
    }
    q9 = v18;
    *(uint*)(v15+4) = v18-(uint)(uintptr_t)v11;
    v21 = 0;
    v22 = 0;
    result = 0;
    d11 = 0;
    d12 = 0;
    d13 = 0;
    d14 = 0;
    d15 = 0;
    do {
      *((byte*)v6+*(uint*)(v15+4)+result-1) = v22;
      *((byte*)v6+*(uint*)(v15+4)+result) = 1;
      ++v22;
      *(uint*)((char*)v6+*(uint*)(v15+4)+result+1) = 0;
      ++v21;
      result += 6;
    } while( v21<256 );
    if( a1!=2||d9==-100 ) {
      if( d6>=11 ) {
        d16 = -11;
        v23 = -11;
      } else {
        v23 = -d6;
        d16 = -d6;
      }
      d9 = v23;
      memset((int*)b12, 0, 2316496);
      d17 = 1024;
      q12 = (__int64)b12;
      d18 = -1;
      d19 = -1;
      d20 = -1;
      memset((int*)b13, 0x55u, 0x80000);
      memset((int*)b14, 0x55u, 0x20000);
      si128 = _mm_load_si128((const __m128i*)&x25);
      q13 = _mm_cvtsi128_si64(si128);
      v25 = 1008;
      d21 = _mm_cvtsi128_si32(si128);
      do {
        *(__m128i*)&d22[v25/4] = si128;
        x26[v25/0x10] = (__int128)si128;
        x27[v25/0x10] = (__int128)si128;
        x28[v25/0x10] = (__int128)si128;
        x29[v25/0x10] = (__int128)si128;
        x30[v25/0x10] = (__int128)si128;
        x31[v25/0x10] = (__int128)si128;
        v25 -= 112LL;
      } while( v25 );
      d23[0] = _mm_cvtsi128_si32(si128);
      memset(d24, 0x55u, 0x40000);
      v26 = 0;
      v27 = 0;
      do {
        b15[4*v26] = v27;
        b15[4*v26+1] = v27;
        b15[4*v26+++2] = v27++;
      } while( v26<0x10000 );
      d25 = 1;
      memset(d26, 0, 0x20000);
      memset((int*)b16, 0, 0x20000);
      v28 = 0;
      v29 = 0;
      do {
        v30 = 0;
        v31 = 0;
        v32 = v29;
        if( v28 ) {
          do {
            v33 = b17[v30++]*(v32&1);
            v32 >>= 1;
            v31 += v33;
          } while( v32 );
        }
        v34 = 8*v28+0x20000;
        v35 = 0;
        do {
          v36 = (byte)b18[v35];
          *(short*)((char*)&w1+v34) = 18432;
          v37 = v31+v36;
          if( v37>=241 )
            v37 = 241;
          if( v37<9 )
            v37 = 9;
          ++v35;
          *(short*)((char*)w2+v34) = 5120;
          *(int*)((char*)d26+v34) = (v37<<23)|(72*v37);
          v34 += 0x20000;
        } while( v35<14 );
        w3[4*v28] = 1024;
        w2[4*v28] = 1024;
        ++v29;
        ++v28;
      } while( v28<0x4000 );
      memset((int*)d27, 0, 0x20000);
      memset((int*)b19, 0, 0x20000);
      v38 = 0;
      v39 = 0;
      do {
        v40 = 0;
        v41 = 0;
        v42 = v39;
        if( v38 ) {
          do {
            v43 = b20[v40++]*(v42&1);
            v42 >>= 1;
            v41 += v43;
          } while( v42 );
        }
        v44 = 8*v38+0x20000;
        v45 = 0;
        do {
          v46 = (byte)b21[v45];
          *(short*)((char*)&w4+v44) = 18432;
          v47 = v41+v46;
          if( v47>=241 )
            v47 = 241;
          if( v47<9 )
            v47 = 9;
          ++v45;
          *(short*)((char*)w5+v44) = 5120;
          *(int*)((char*)&d27+v44) = (v47<<23)|(72*v47);
          v44 += 0x10000;
        } while( v45<24 );
        w6[4*v38] = 1024;
        w7[4*v38] = 1024;
        w8[4*v38] = 1024;
        ++v39;
        w5[4*v38++] = 1024;
      } while( v38<0x2000 );
      v48 = 0;
      v49 = 0;
      v50 = 0;
      do {
        v51 = 0;
        v52 = v49;
        do {
          *(short*)((char*)w9+v52) = 2048;
          *(short*)((char*)w10+v52) = 2048;
          *(int*)((char*)d28+v52) = 20480;
          v52 += 8;
          ++v51;
        } while( v51<1024 );
        v53 = 0;
        v54 = v50;
        do {
          *(short*)((char*)w11+v54) = 2048;
          *(short*)((char*)&w12+v54) = 2048;
          *(int*)((char*)d29+v54) = 20480;
          v54 += 8;
          ++v53;
        } while( v53<256 );
        v49 += 0x2000;
        v50 += 2048;
        ++v48;
      } while( v48<16 );
      v55 = 0;
      v56 = 0;
      do {
        v57 = 49*(byte)b22[v55];
        v58 = 0;
        v59 = v56;
        do {
          *(int*)((char*)d30+v59) = v57;
          v59 += 4;
          ++v58;
        } while( v58<128 );
        v56 += 512;
        ++v55;
      } while( v55<5 );
      v60 = 0;
      v61 = 0;
      do {
        v62 = 0;
        v63 = 48*(byte)b23[v60];
        v64 = v61;
        do {
          *(int*)((char*)d31+v64) = v63;
          *(int*)((char*)d32+v64) = 15104;
          v64 += 8;
          ++v62;
        } while( v62<256 );
        v61 += 2048;
        ++v60;
      } while( v60<5 );
      d33 = 0;
      for( i = 0; i<196608; ++i ) {
        d34[2*i] = 0x2000;
        d35[2*i] = 24576;
      }
      for( j = 0; j<0x100000; ++j ) {
        d36[2*j] = 0;
        d37[2*j] = 0x40000;
      }
      d38 = 0;
      for( k = 0; k<98304; ++k ) {
        d39[2*k] = 0;
        d40[2*k] = 0x40000;
      }
      d41 = 0;
      for( result = 0; result<86016; ++result ) {
        d42[2*result] = 0;
        d43[2*result] = 0x80000;
      }
    }
  } else {
    result = q11;
    v5 = *(uint*)(q11+8);
    if( v5 ) {
      result = q8;
      do {
        ++v2;
        v5 = *(uint*)(v5+q8+8);
      } while( v5 );
      d5 = v2;
    }
    d7 = v2;
  }
  return result;
}

PROBE_DECL(sub_14000A2B0)
__int64 sub_14000A2B0(int a1, qword a2, __int64 a3) {
  PROBE_HIT(sub_14000A2B0);
  __int64 v3;
  uint v4;
  qword v5;
  int v6;
  uint v7;
  byte* v8;
  uint v9;
  __int64 v10;
  int v11;
  byte* i;
  int v13;
  qword v14;
  qword v15;
  int v16;
  byte* v17;
  qword v18;
  int v19;
  uint* v20;
  __int64 v21;
  __int64 v22;
  uint* v23;
  __int64 result;
  int v25;
  int v26;
  v3 = q8;
  v4 = *(uint*)(*(qword*)a2+2LL);
  v5 = a2;
  while( 1 ) {
    v6 = *(uint*)(*(qword*)v5+2LL);
    if( v6!=v4 ) {
      LODWORD(a3) = q8+v6;
      goto LABEL_15;
    }
    v7 = *(uint*)(a3+8);
    v5 += 8LL;
    if( !v7 )
      goto LABEL_15;
    if( v5>=q14 )
      break;
    a3 = q8+v7;
  }
  v8 = (byte*)q9;
  v9 = *(uint*)(a3+8);
  while( 1 ) {
    v10 = v3+v9;
    if( *(byte*)v10 ) {
      v11 = *v8;
      for( i = (byte*)(v3+*(uint*)(v10+4)); *i!=v11; i += 6 )
        ;
    } else {
      i = (byte*)(v3+v9+2);
    }
    v13 = *(uint*)(i+2);
    if( v13!=v4 )
      break;
    v9 = *(uint*)(v10+8);
    *(qword*)v5 = (qword)(uintptr_t)i;
    v5 += 8LL;
    if( !v9 ) {
      LODWORD(a3) = v10;
      goto LABEL_15;
    }
  }
  LODWORD(a3) = v3+v13;
LABEL_15:
  v14 = a2+8LL*a1;
  if( v5==v14 )
    return (uint)(a3-v3);
  v15 = v14;
  v16 = a3;
  LOBYTE(v25) = 0;
  v17 = (byte*)(v3+v4);
  HIWORD(v25) = *v17;
  v26 = (uint)(uintptr_t)v17-v3+1;
  v18 = v5;
  BYTE1(v25) = *((byte*)x4+*v17);
  v19 = q6-v3;
  v20 = (uint*)q6;
  v21 = q3;
  v22 = v3;
  while( 1 ) {
    if( v21==q2 ) {
      if( *v20 ) {
        v23 = (uint*)(v22+(uint)v20[2]);
        v20[2] = v23[2];
        *(uint*)((uint)v23[2]+v22+4) = v19;
        --*v20;
      } else {
        v23 = (uint*)sub_140011240(0);
      }
    } else {
      v21 -= 12;
      q3 = v21;
      v23 = (uint*)v21;
    }
    if( !v23 )
      break;
    *v23 = v25;
    v23[1] = v26;
    v23[2] = v16-v22;
    v16 = (int)(uintptr_t)v23;
    result = (uint)((uint)(uintptr_t)v23-v22);
    v18 -= 8LL;
    *(uint*)(*(qword*)v18+2LL) = result;
    if( v18==v15 )
      return result;
  }
  return 0;
}

PROBE_DECL(sub_140018048)
void sub_140018048(__int64 a1) {
  PROBE_HIT(sub_140018048);
  q15 = a1;
}

PROBE_DECL(sub_140011920)
char* sub_140011920(__int64 a1, uint a2) {
  PROBE_HIT(sub_140011920);
  __int64 v2;
  __int64 v3;
  __int64 v4;
  __int64 v5;
  __int64 v6;
  byte* i;
  __int64 v8;
  uint v9;
  uint v10;
  int v11;
  __int64 v12;
  int v13;
  uint v14;
  __int64 v15;
  __int64 v16;
  int v17;
  uint* v18;
  __int64 v19;
  uint* v20;
  int v21;
  int v23;
  v2 = q6;
  v3 = q8;
  v4 = a1-q8;
  v5 = a2;
  v6 = 12LL*a2;
  for( i = (byte*)(a1+v6); *(byte*)(a1+v6+1)==255; i = (byte*)(a1+v6) ) {
    *(uint*)(*((uint*)i+1)+v3+8) = *((uint*)i+2);
    *(uint*)(*((uint*)i+2)+v3+4) = *((uint*)i+1);
    v8 = 12LL**((byte*)&__arch_hdr[11]+*i+3);
    --*(uint*)(v8+v2);
    a2 += *(byte*)(a1+v6);
    v5 = a2;
    v6 = 12LL*a2;
  }
  v9 = a2;
  if( a2>0x80 ) {
    v10 = 0;
    v11 = 0;
    v12 = -((__int64)(((qword)((1-v5)>>6)>>57)-v5+1)>>7);
    do {
      a2 = v11+v9-128;
      v11 -= 128;
      ++v10;
    } while( v10<(uint)v12 );
    v13 = *(uint*)(v2+448);
    v14 = 0;
    do {
      *(byte*)(a1+1) = -1;
      *(uint*)(a1+8) = v2-v3+444;
      *(uint*)(a1+4) = v13;
      v13 = v4;
      *(byte*)a1 = 0x80;
      v4 += 1536;
      a1 += 1536;
      *(uint*)(v3+*(uint*)(v2+448)+8) = v13;
      ++*(uint*)(v2+444);
      *(uint*)(v2+448) = v13;
      ++v14;
    } while( v14<(uint)v12 );
  }
  v15 = *((byte*)&__arch_hdr[11]+a2+3);
  LODWORD(v16) = *((byte*)&x1+v15);
  if( a2!=(uint)v16 ) {
    v15 = (uint)(v15-1);
    v16 = *((byte*)&x1+v15);
    v17 = a2-v16;
    v18 = (uint*)(v2+12LL*(uint)(v17-1));
    v19 = a1+12*v16;
    *(byte*)(v19+1) = -1;
    *(byte*)v19 = v17;
    *(uint*)(v19+8) = v2+12*(v17-1)-v3;
    *(uint*)(v19+4) = v18[1];
    LODWORD(v19) = v19-v3;
    *(uint*)((uint)v18[1]+v3+8) = v19;
    v18[1] = v19;
    ++*v18;
  }
  *(byte*)(a1+1) = -1;
  v20 = (uint*)(12*v15+v2);
  v21 = v20[1];
  *(byte*)a1 = v16;
  *(uint*)(a1+8) = (uint)(uintptr_t)v20-v3;
  *(uint*)(a1+4) = v21;
  v23 = a1-v3;
  *(uint*)((uint)v20[1]+v3+8) = v23;
  v20[1] = v23;
  ++*v20;
  return (char*)v20-v3;
}
PROBE_DECL(sub_14000A4B0)
__int64 sub_14000A4B0(byte* a1, uint a2) {
  PROBE_HIT(sub_14000A4B0);
  byte* v3;
  __int64 v4;
  int v5;
  qword v6;
  int v7;
  int v8;
  uint* v9;
  uint* v10;
  byte* v11;
  uint v12;
  __int64 v13;
  __int64 v14;
  uint v15;
  uint v16;
  int v17;
  __int64 v18;
  uint v19;
  __int64 v20;
  __int64 v21;
  int v22;
  __int64 v23;
  uint* v24;
  byte* v25;
  uint* v26;
  int v27;
  uint* v29;
  int v30;
  byte* v31;
  uint v32;
  __int64 v33;
  __int64 v34;
  uint v35;
  uint v36;
  int v37;
  __int64 v38;
  uint v39;
  __int64 v40;
  __int64 v41;
  int v42;
  __int64 v43;
  uint* v44;
  byte* v45;
  uint* v46;
  int v47;
  byte v48;
  __int64 v49;
  byte* v50;
  __int64 v51;
  const char* v52;
  byte* v53;
  __int64 v54;
  int v55;
  uint v56;
  uint v57;
  __int64 v58;
  qword v59;
  byte* v60;
  short v61;
  int v62;
  int v63;
  int v64;
  uint* v65;
  __int64 v66;
  uint v67;
  __int64 v68;
  __int64 v69;
  uint* v70;
  __int64 v71;
  byte* v72;
  __int64 v73;
  uint v74;
  uint v75;
  int v76;
  __int64 v77;
  __int64 v78;
  uint v79;
  __int64 v80;
  __int64 v81;
  int v82;
  uint* v83;
  byte* v84;
  uint* v85;
  int v86;
  uint v87;
  uint v88;
  uint v89;
  __int64 v90;
  uint* v91;
  __int64 v92;
  uint* v93;
  byte* v94;
  uint v95;
  uint v96;
  uint v97;
  uint v98;
  __int64 v99;
  uint* v100;
  __int64 v101;
  byte* v102;
  __int64 v103;
  uint v104;
  uint v105;
  int v106;
  __int64 v107;
  __int64 v108;
  uint v109;
  __int64 v110;
  __int64 v111;
  int v112;
  uint* v113;
  byte* v114;
  uint* v115;
  int v116;
  int v117;
  byte v118;
  int v119;
  uint v120;
  uint v121;
  __int64 v122;
  __int64 v123;
  __int64 v124;
  uint* v125;
  __int64 v126;
  uint v127;
  uint v128;
  uint v129;
  uint v130;
  __int64 v131;
  byte* v132;
  __int64 v133;
  uint v134;
  uint v135;
  int v136;
  __int64 v137;
  __int64 v138;
  uint v139;
  __int64 v140;
  __int64 v141;
  int v142;
  uint* v143;
  byte* v144;
  uint* v145;
  int v146;
  char* v147;
  char* v148;
  __int64 v149;
  __int64 v150;
  uint v151;
  __int64 v152;
  uint* v153;
  __int64 v154;
  byte* v155;
  __int64 v156;
  uint v157;
  uint v158;
  int v159;
  __int64 v160;
  __int64 v161;
  uint v162;
  __int64 v163;
  __int64 v164;
  int v165;
  __int64 v166;
  uint* v167;
  byte* v168;
  uint* v169;
  int v170;
  __int64 v171;
  __int64 v172;
  v3 = a1;
  v4 = q8;
  v5 = *a1;
  if( *a1 ) {
    v48 = a1[1];
    v172 = *((uint*)a1+1);
    v49 = (uint)(v5+1);
    v50 = (byte*)(q8+v172);
    v51 = (uint)(2*v5+2);
    v52 = (const char*)(v51+q8+v172);
    _mm_prefetch((const char*)(q8+v172+v49), 1);
    _mm_prefetch(v52, 1);
    _mm_prefetch(&v52[v49], 1);
    _mm_prefetch(&v52[v51], 1);
    _mm_prefetch(&v52[v51+v49], 1);
    _mm_prefetch(&v52[v51-1+v51], 1);
    v53 = v50;
    v54 = (__int64)v50;
    v55 = v5;
    v56 = v5+2;
    v57 = (uint)(v5+2)>>1;
    v58 = 3LL*(v48&0xF);
    if( !v50[2*v58+1] ) {
      v50[2*v58+1] = 8;
      *((word*)v3+1) += 8;
    }
    v59 = q5;
    v60 = v50;
    do {
      if( v59>v4+(qword)*(uint*)(v53+2) ) {
        *(uint*)(v53+2) = 0;
        --v55;
      } else {
        if( v53!=v60 ) {
          v61 = *(word*)v60;
          v62 = *(uint*)(v60+2);
          *(word*)v60 = *(word*)v53;
          *(uint*)(v60+2) = *(uint*)(v53+2);
          *(word*)v53 = v61;
          *(uint*)(v53+2) = v62;
        }
        _mm_prefetch((const char*)(*(uint*)(v60+2)+v4), 1);
        v60 += 6;
      }
      v53 += 6;
      --v5;
    } while( v5>=0 );
    if( v55==*v3||!a2 ) {
      v122 = *((byte*)&__arch_hdr[11]+v57+3);
      v123 = *((byte*)&x1+v122);
      if( v50[12*v123+1]==255&&(v124 = q6, v125 = (uint*)(q6+12*v122), *v125) ) {
        v171 = (uint)v125[1];
        v65 = (uint*)(v4+v171);
        v125[1] = *(uint*)(v4+v171+4);
        *(uint*)(*(uint*)(v4+v171+4)+v4+8) = (uint)(uintptr_t)v125-v4;
        --*v125;
        v126 = v4+v171;
        if( (v57&1)!=0 ) {
          *v65 = *(uint*)v50;
          v65[1] = *((uint*)v50+1);
          v65[2] = *((uint*)v50+2);
          v126 = v4+v171+12;
          v54 = v4+v172+12;
        }
        v127 = v56>>2;
        if( v127 ) {
          v128 = 0;
          v129 = 0;
          do {
            *(uint*)(v129+v126) = *(uint*)(v54+v129);
            ++v128;
            *(uint*)(v129+v126+4) = *(uint*)(v54+v129+4);
            *(uint*)(v129+v126+8) = *(uint*)(v54+v129+8);
            *(uint*)(v129+v126+12) = *(uint*)(v54+v129+12);
            *(uint*)(v129+v126+16) = *(uint*)(v54+v129+16);
            *(uint*)(v129+v126+20) = *(uint*)(v54+v129+20);
            v129 += 24;
          } while( v128<v127 );
        }
        v130 = v123;
        if( v50[12*v123+1]==255 ) {
          v131 = 12*v123;
          v132 = &v50[12*v123];
          do {
            *(uint*)(*((uint*)v132+1)+v4+8) = *((uint*)v132+2);
            *(uint*)(*((uint*)v132+2)+v4+4) = *((uint*)v132+1);
            v133 = 12LL**((byte*)&__arch_hdr[11]+*v132+3);
            --*(uint*)(v133+v124);
            v130 += v50[v131];
            v123 = v130;
            v131 = 12LL*v130;
            v132 = &v50[v131];
          } while( v50[v131+1]==255 );
        }
        v134 = v130;
        if( v130>0x80 ) {
          v135 = 0;
          v136 = 0;
          v137 = -((__int64)(((qword)((1-v123)>>6)>>57)-v123+1)>>7);
          do {
            v130 = v136+v134-128;
            v136 -= 128;
            ++v135;
          } while( v135<(uint)v137 );
          v138 = v172;
          v139 = 0;
          do {
            v50[1] = -1;
            *((uint*)v50+2) = v124-v4+444;
            ++v139;
            *v50 = 0x80;
            *((uint*)v50+1) = *(uint*)(v124+448);
            *(uint*)(*(uint*)(v124+448)+v4+8) = v138;
            ++*(uint*)(v124+444);
            *(uint*)(v124+448) = v138;
            v138 += 1536;
            v50 += 1536;
          } while( v139<(uint)v137 );
        }
        v140 = *((byte*)&__arch_hdr[11]+v130+3);
        LODWORD(v141) = *((byte*)&x1+v140);
        if( v130!=(uint)v141 ) {
          v140 = (uint)(v140-1);
          v141 = *((byte*)&x1+v140);
          v142 = v130-v141;
          v143 = (uint*)(v124+12LL*(uint)(v142-1));
          v144 = &v50[12*v141];
          *v144 = v142;
          v144[1] = -1;
          *((uint*)v144+2) = (uint)(uintptr_t)v143-v4;
          *((uint*)v144+1) = v143[1];
          LODWORD(v144) = (uint)(uintptr_t)v144-v4;
          *(uint*)((uint)v143[1]+v4+8) = (uint)(uintptr_t)v144;
          ++*v143;
          v143[1] = (uint)(uintptr_t)v144;
        }
        v50[1] = -1;
        v145 = (uint*)(12*v140+v124);
        *v50 = v141;
        *((uint*)v50+2) = (uint)(uintptr_t)v145-v4;
        *((uint*)v50+1) = v145[1];
        v146 = (uint)(uintptr_t)v50-v4;
        *(uint*)((uint)v145[1]+v4+8) = v146;
        v145[1] = v146;
        ++*v145;
      } else {
        v65 = (uint*)v50;
      }
      *((uint*)v3+1) = (uint)(uintptr_t)v65-v4;
    } else {
      *v3 = v55;
      if( v55<0 ) {
        v150 = *((byte*)&__arch_hdr[11]+v57+3);
        v151 = *((byte*)&x1+v150);
        v152 = *((byte*)&x1+v150);
        v29 = (uint*)q6;
        if( v50[12*v152+1]==255 ) {
          v154 = 12*v152;
          v155 = &v50[12*v152];
          do {
            *(uint*)(*((uint*)v155+1)+v4+8) = *((uint*)v155+2);
            *(uint*)(*((uint*)v155+2)+v4+4) = *((uint*)v155+1);
            v156 = *((byte*)&__arch_hdr[11]+*v155+3);
            --v29[3*v156];
            v151 += v50[v154];
            v154 = 12LL*v151;
            v155 = &v50[v154];
          } while( v50[v154+1]==255 );
          v157 = v151;
          if( v151>0x80 ) {
            v158 = 0;
            v159 = 0;
            v160 = -((__int64)(((qword)((1LL-v151)>>6)>>57)-v151+1)>>7);
            do {
              v151 = v159+v157-128;
              v159 -= 128;
              ++v158;
            } while( v158<(uint)v160 );
            v161 = v172;
            v162 = 0;
            do {
              v50[1] = -1;
              *((uint*)v50+2) = (uint)(uintptr_t)v29-v4+444;
              ++v162;
              *v50 = 0x80;
              *((uint*)v50+1) = v29[112];
              *(uint*)((uint)v29[112]+v4+8) = v161;
              ++v29[111];
              v29[112] = v161;
              v161 += 1536;
              v50 += 1536;
            } while( v162<(uint)v160 );
          }
          v163 = *((byte*)&__arch_hdr[11]+v151+3);
          LODWORD(v164) = *((byte*)&x1+v163);
          if( v151!=(uint)v164 ) {
            v163 = (uint)(v163-1);
            v164 = *((byte*)&x1+v163);
            v165 = v151-v164;
            v166 = (uint)(v165-1);
            v167 = &v29[3*v166];
            v168 = &v50[12*v164];
            *v168 = v165;
            v168[1] = -1;
            *((uint*)v168+2) = (uint)(uintptr_t)v29+12*v166-v4;
            *((uint*)v168+1) = v167[1];
            LODWORD(v168) = (uint)(uintptr_t)v168-v4;
            *(uint*)((uint)v167[1]+v4+8) = (uint)(uintptr_t)v168;
            ++*v167;
            v167[1] = (uint)(uintptr_t)v168;
          }
          v50[1] = -1;
          v169 = &v29[3*v163];
          *v50 = v164;
          *((uint*)v50+2) = (uint)(uintptr_t)v29+12*v163-v4;
          *((uint*)v50+1) = v169[1];
          v170 = (uint)(uintptr_t)v50-v4;
          *(uint*)((uint)v169[1]+v4+8) = v170;
          v169[1] = v170;
          ++*v169;
        } else {
          v50[1] = -1;
          v153 = &v29[3*v150];
          *v50 = v151;
          *((uint*)v50+2) = (uint)(uintptr_t)v29+12*v150-v4;
          *((uint*)v50+1) = v153[1];
          *(uint*)((uint)v153[1]+v4+8) = v172;
          v153[1] = v172;
          ++*v153;
        }
        goto LABEL_23;
      }
      if( v55 ) {
        v87 = (v55+2)>>1;
        v88 = *((byte*)&__arch_hdr[11]+v57+3);
        v89 = *((byte*)&__arch_hdr[11]+v87+3);
        if( v88!=v89 ) {
          v90 = q6;
          v91 = (uint*)(q6+12LL**((byte*)&__arch_hdr[11]+v87+3));
          if( *v91 ) {
            v92 = (uint)v91[1];
            v93 = (uint*)(v4+v92);
            v91[1] = *(uint*)(v4+v92+4);
            *(uint*)(*(uint*)(v4+v92+4)+v4+8) = (uint)(uintptr_t)v91-v4;
            --*v91;
            v94 = (byte*)(v4+v92);
            if( (v87&1)!=0 ) {
              *v93 = *(uint*)v50;
              v93[1] = *((uint*)v50+1);
              v93[2] = *((uint*)v50+2);
              v93 = (uint*)(v4+v92+12);
              v54 = v4+v172+12;
            }
            v95 = v87>>1;
            if( v95 ) {
              v96 = 0;
              v97 = 0;
              do {
                v93[v97] = *(uint*)(v54+v97*4);
                ++v96;
                v93[v97+1] = *(uint*)(v54+v97*4+4);
                v93[v97+2] = *(uint*)(v54+v97*4+8);
                v93[v97+3] = *(uint*)(v54+v97*4+12);
                v93[v97+4] = *(uint*)(v54+v97*4+16);
                v93[v97+5] = *(uint*)(v54+v97*4+20);
                v97 += 6;
              } while( v96<v95 );
            }
            v98 = *((byte*)&x1+v88);
            v99 = *((byte*)&x1+v88);
            if( v50[12*v99+1]==255 ) {
              v101 = 12*v99;
              v102 = &v50[12*v99];
              do {
                *(uint*)(*((uint*)v102+1)+v4+8) = *((uint*)v102+2);
                *(uint*)(*((uint*)v102+2)+v4+4) = *((uint*)v102+1);
                v103 = 12LL**((byte*)&__arch_hdr[11]+*v102+3);
                --*(uint*)(v103+v90);
                v98 += v50[v101];
                v101 = 12LL*v98;
                v102 = &v50[v101];
              } while( v50[v101+1]==255 );
              v104 = v98;
              if( v98>0x80 ) {
                v105 = 0;
                v106 = 0;
                v107 = -((__int64)(((qword)((1LL-v98)>>6)>>57)-v98+1)>>7);
                do {
                  v98 = v106+v104-128;
                  v106 -= 128;
                  ++v105;
                } while( v105<(uint)v107 );
                v108 = v172;
                v109 = 0;
                do {
                  v50[1] = -1;
                  *((uint*)v50+2) = v90-v4+444;
                  ++v109;
                  *v50 = 0x80;
                  *((uint*)v50+1) = *(uint*)(v90+448);
                  *(uint*)(*(uint*)(v90+448)+v4+8) = v108;
                  ++*(uint*)(v90+444);
                  *(uint*)(v90+448) = v108;
                  v108 += 1536;
                  v50 += 1536;
                } while( v109<(uint)v107 );
              }
              v110 = *((byte*)&__arch_hdr[11]+v98+3);
              LODWORD(v111) = *((byte*)&x1+v110);
              if( v98!=(uint)v111 ) {
                v110 = (uint)(v110-1);
                v111 = *((byte*)&x1+v110);
                v112 = v98-v111;
                v113 = (uint*)(v90+12LL*(uint)(v112-1));
                v114 = &v50[12*v111];
                *v114 = v112;
                v114[1] = -1;
                *((uint*)v114+2) = (uint)(uintptr_t)v113-v4;
                *((uint*)v114+1) = v113[1];
                LODWORD(v114) = (uint)(uintptr_t)v114-v4;
                *(uint*)((uint)v113[1]+v4+8) = (uint)(uintptr_t)v114;
                ++*v113;
                v113[1] = (uint)(uintptr_t)v114;
              }
              v50[1] = -1;
              v115 = (uint*)(12*v110+v90);
              *v50 = v111;
              *((uint*)v50+2) = (uint)(uintptr_t)v115-v4;
              *((uint*)v50+1) = v115[1];
              v116 = (uint)(uintptr_t)v50-v4;
              *(uint*)((uint)v115[1]+v4+8) = v116;
              v115[1] = v116;
              ++*v115;
            } else {
              v50[1] = -1;
              v100 = (uint*)(12LL*v88+v90);
              *v50 = v98;
              *((uint*)v50+2) = (uint)(uintptr_t)v100-v4;
              *((uint*)v50+1) = v100[1];
              *(uint*)((uint)v100[1]+v4+8) = v172;
              v100[1] = v172;
              ++*v100;
            }
            v50 = v94;
          } else {
            sub_140011920((__int64)&v50[12**((byte*)&x1+v89)], *((byte*)&x1+v88)-*((byte*)&x1+v89));
          }
        }
        v117 = *((word*)v3+1);
        v118 = v3[1];
        v65 = (uint*)v50;
        *((uint*)v3+1) = (uint)(uintptr_t)v50-v4;
        v119 = v117>16*v55;
        v3[1] = (((byte)v119<<6)&v118)+*((byte*)x4+*v50);
        v120 = (v119+(uint)v50[1])>>v119;
        v50[1] = v120;
        *((word*)v3+1) = (byte)v120;
        do {
          v50 += 6;
          v3[1] |= *((byte*)x4+*v50);
          v121 = (v119+(uint)v50[1])>>v119;
          v50[1] = v121;
          *((word*)v3+1) += (byte)v121;
          --v55;
        } while( v55 );
        v55 = *v3;
      } else {
        v3[1] = *((byte*)x4+*v53);
        v63 = v50[1];
        v64 = *((word*)v3+1)-(v63-1)+19;
        if( v64<2*v63-2 )
          LOBYTE(v64) = (v63-1)/v64+2;
        else
          v64 = (uint)(v64-(3*v63-3))>>31;
        v50[1] = v64+1;
        v65 = (uint*)(v3+2);
        *((word*)v3+1) = *(word*)v50;
        *((uint*)v3+1) = *(uint*)(v50+2);
        v66 = *((byte*)&__arch_hdr[11]+v57+3);
        v67 = *((byte*)&x1+v66);
        v68 = q6;
        v69 = *((byte*)&x1+v66);
        if( v50[12*v69+1]==255 ) {
          v71 = 12*v69;
          v72 = &v50[12*v69];
          do {
            *(uint*)(*((uint*)v72+1)+v4+8) = *((uint*)v72+2);
            *(uint*)(*((uint*)v72+2)+v4+4) = *((uint*)v72+1);
            v73 = 12LL**((byte*)&__arch_hdr[11]+*v72+3);
            --*(uint*)(v73+v68);
            v67 += v50[v71];
            v71 = 12LL*v67;
            v72 = &v50[v71];
          } while( v50[v71+1]==255 );
          v74 = v67;
          if( v67>0x80 ) {
            v75 = 0;
            v76 = 0;
            v77 = -((__int64)(((qword)((1LL-v67)>>6)>>57)-v67+1)>>7);
            do {
              v67 = v76+v74-128;
              v76 -= 128;
              ++v75;
            } while( v75<(uint)v77 );
            v78 = v172;
            v79 = 0;
            do {
              v50[1] = -1;
              *((uint*)v50+2) = v68-v4+444;
              ++v79;
              *v50 = 0x80;
              *((uint*)v50+1) = *(uint*)(v68+448);
              *(uint*)(*(uint*)(v68+448)+v4+8) = v78;
              ++*(uint*)(v68+444);
              *(uint*)(v68+448) = v78;
              v78 += 1536;
              v50 += 1536;
            } while( v79<(uint)v77 );
          }
          v80 = *((byte*)&__arch_hdr[11]+v67+3);
          LODWORD(v81) = *((byte*)&x1+v80);
          if( v67!=(uint)v81 ) {
            v80 = (uint)(v80-1);
            v81 = *((byte*)&x1+v80);
            v82 = v67-v81;
            v83 = (uint*)(v68+12LL*(uint)(v82-1));
            v84 = &v50[12*v81];
            *v84 = v82;
            v84[1] = -1;
            *((uint*)v84+2) = (uint)(uintptr_t)v83-v4;
            *((uint*)v84+1) = v83[1];
            LODWORD(v84) = (uint)(uintptr_t)v84-v4;
            *(uint*)((uint)v83[1]+v4+8) = (uint)(uintptr_t)v84;
            ++*v83;
            v83[1] = (uint)(uintptr_t)v84;
          }
          v50[1] = -1;
          v85 = (uint*)(12*v80+v68);
          *v50 = v81;
          *((uint*)v50+2) = (uint)(uintptr_t)v85-v4;
          *((uint*)v50+1) = v85[1];
          v86 = (uint)(uintptr_t)v50-v4;
          *(uint*)((uint)v85[1]+v4+8) = v86;
          v85[1] = v86;
          ++*v85;
        } else {
          v50[1] = -1;
          v70 = (uint*)(v68+12*v66);
          *v50 = v67;
          *((uint*)v50+2) = v68+12*v66-v4;
          *((uint*)v50+1) = v70[1];
          *(uint*)((uint)v70[1]+v4+8) = v172;
          v70[1] = v172;
          ++*v70;
        }
      }
    }
    v7 = d6;
    v147 = (char*)v65+6*v55;
    v148 = v147-6;
    if( a2<d6 ) {
      while( 1 ) {
        v149 = v4+*(uint*)(v147+2);
        _mm_prefetch((const char*)(*(uint*)(v149+4)+v4), 1);
        *(uint*)(v147+2) = sub_14000A4B0((byte*)(uintptr_t)(v149), a2+1);
        if( --v55<0 )
          break;
        v147 = v148;
        v148 -= 6;
      }
    } else {
      *(uint*)(v147+2) = 0;
      while( --v55>=0 ) {
        *(uint*)(v148+2) = 0;
        v148 -= 6;
      }
    }
    goto LABEL_7;
  }
  v6 = q8+*((uint*)a1+1);
  if( v6>=q5 ) {
    v7 = d6;
    if( a2<d6 ) {
      _mm_prefetch((const char*)(*(uint*)(v6+4)+q8), 1);
      v8 = sub_14000A4B0((byte*)(uintptr_t)(v6), a2+1);
      *((uint*)v3+1) = v8;
      if( v8 )
        goto LABEL_7;
    } else {
      *((uint*)v3+1) = 0;
    }
    if( a2<9 ) {
LABEL_7:
      if( a2==v7&&v3[13]==255 ) {
        v9 = (uint*)q6;
        if( *(uint*)q6 ) {
          v10 = (uint*)(v4+*(uint*)(q6+8));
          *(uint*)(q6+8) = v10[2];
          *(uint*)((uint)v10[2]+v4+4) = (uint)(uintptr_t)v9-v4;
          v11 = &v3[-v4];
          v12 = 1;
          --*v9;
          *v10 = *(uint*)v3;
          v10[1] = *((uint*)v3+1);
          v10[2] = *((uint*)v3+2);
          if( v3[13]==255 ) {
            v13 = 12;
            do {
              *(uint*)(*(uint*)&v3[v13+4]+v4+8) = *(uint*)&v3[v13+8];
              *(uint*)(*(uint*)&v3[v13+8]+v4+4) = *(uint*)&v3[v13+4];
              v14 = *((byte*)&__arch_hdr[11]+v3[v13]+3);
              --v9[3*v14];
              v12 += v3[v13];
              v13 = 12LL*v12;
            } while( v3[v13+1]==255 );
            v15 = v12;
            if( v12>0x80 ) {
              v16 = 0;
              v17 = 0;
              v18 = -((__int64)(((qword)((1LL-v12)>>6)>>57)-v12+1)>>7);
              do {
                v12 = v17+v15-128;
                v17 -= 128;
                ++v16;
              } while( v16<(uint)v18 );
              v19 = 0;
              do {
                v3[1] = -1;
                *((uint*)v3+2) = (uint)(uintptr_t)v9-v4+444;
                ++v19;
                *v3 = 0x80;
                *((uint*)v3+1) = v9[112];
                *(uint*)((uint)v9[112]+v4+8) = (uint)(uintptr_t)v11;
                ++v9[111];
                v9[112] = (uint)(uintptr_t)v11;
                v11 += 1536;
                v3 += 1536;
              } while( v19<(uint)v18 );
            }
          }
          v20 = *((byte*)&__arch_hdr[11]+v12+3);
          LODWORD(v21) = *((byte*)&x1+v20);
          if( v12!=(uint)v21 ) {
            v20 = (uint)(v20-1);
            v21 = *((byte*)&x1+v20);
            v22 = v12-v21;
            v23 = (uint)(v22-1);
            v24 = &v9[3*v23];
            v25 = &v3[12*v21];
            *v25 = v22;
            v25[1] = -1;
            *((uint*)v25+2) = (uint)(uintptr_t)v9+12*v23-v4;
            *((uint*)v25+1) = v24[1];
            LODWORD(v25) = (uint)(uintptr_t)v25-v4;
            *(uint*)((uint)v24[1]+v4+8) = (uint)(uintptr_t)v25;
            ++*v24;
            v24[1] = (uint)(uintptr_t)v25;
          }
          v3[1] = -1;
          v26 = &v9[3*v20];
          *v3 = v21;
          *((uint*)v3+2) = (uint)(uintptr_t)v26-v4;
          *((uint*)v3+1) = v26[1];
          v27 = (uint)(uintptr_t)v3-v4;
          *(uint*)((uint)v26[1]+v4+8) = v27;
          v26[1] = v27;
          ++*v26;
          LODWORD(v3) = (uint)(uintptr_t)v10;
        }
      }
      return (uint)((uint)(uintptr_t)v3-v4);
    }
  }
  v29 = (uint*)q6;
LABEL_23:
  if( v3[13]==255 ) {
    v31 = &v3[-v4];
    v32 = 1;
    v33 = 12;
    do {
      *(uint*)(*(uint*)&v3[v33+4]+v4+8) = *(uint*)&v3[v33+8];
      *(uint*)(*(uint*)&v3[v33+8]+v4+4) = *(uint*)&v3[v33+4];
      v34 = *((byte*)&__arch_hdr[11]+v3[v33]+3);
      --v29[3*v34];
      v32 += v3[v33];
      v33 = 12LL*v32;
    } while( v3[v33+1]==255 );
    v35 = v32;
    if( v32>0x80 ) {
      v36 = 0;
      v37 = 0;
      v38 = -((__int64)(((qword)((1LL-v32)>>6)>>57)-v32+1)>>7);
      do {
        v32 = v37+v35-128;
        v37 -= 128;
        ++v36;
      } while( v36<(uint)v38 );
      v39 = 0;
      do {
        v3[1] = -1;
        *((uint*)v3+2) = (uint)(uintptr_t)v29-v4+444;
        ++v39;
        *v3 = 0x80;
        *((uint*)v3+1) = v29[112];
        *(uint*)((uint)v29[112]+v4+8) = (uint)(uintptr_t)v31;
        ++v29[111];
        v29[112] = (uint)(uintptr_t)v31;
        v31 += 1536;
        v3 += 1536;
      } while( v39<(uint)v38 );
    }
    v40 = *((byte*)&__arch_hdr[11]+v32+3);
    LODWORD(v41) = *((byte*)&x1+v40);
    if( v32!=(uint)v41 ) {
      v40 = (uint)(v40-1);
      v41 = *((byte*)&x1+v40);
      v42 = v32-v41;
      v43 = (uint)(v42-1);
      v44 = &v29[3*v43];
      v45 = &v3[12*v41];
      *v45 = v42;
      v45[1] = -1;
      *((uint*)v45+2) = (uint)(uintptr_t)v29+12*v43-v4;
      *((uint*)v45+1) = v44[1];
      LODWORD(v45) = (uint)(uintptr_t)v45-v4;
      *(uint*)((uint)v44[1]+v4+8) = (uint)(uintptr_t)v45;
      ++*v44;
      v44[1] = (uint)(uintptr_t)v45;
    }
    v3[1] = -1;
    v46 = &v29[3*v40];
    *v3 = v41;
    *((uint*)v3+2) = (uint)(uintptr_t)v46-v4;
    *((uint*)v3+1) = v46[1];
    v47 = (uint)(uintptr_t)v3-v4;
    *(uint*)((uint)v46[1]+v4+8) = v47;
    v46[1] = v47;
    ++*v46;
  } else {
    v3[1] = -1;
    *((uint*)v3+1) = (uint)(uintptr_t)v29-v4;
    *v3 = 1;
    *((uint*)v3+2) = v29[2];
    v30 = (uint)(uintptr_t)v3-v4;
    *(uint*)((uint)v29[2]+v4+4) = v30;
    v29[2] = v30;
    ++*v29;
  }
  return 0;
}

PROBE_DECL(sub_140009620)
__int64 sub_140009620() {
  PROBE_HIT(sub_140009620);
  __int64 v0;
  byte* v1;
  int v2;
  int v3;
  uint v4;
  byte* v5;
  char v6;
  uint v7;
  __int64 result;
  __int64 v9;
  __int64 v10;
  qword v11;
  __int64 v12;
  uint v13;
  __int64 v14;
  __int64 v15;
  __int64 v16;
  __int64 v17;
  int v18;
  char v19;
  uint v20;
  uint v21;
  __int64 v22;
  uint* v23;
  uint v24;
  uint v25;
  __int64 v26;
  __int64 v27;
  uint* v28;
  uint* v29;
  qword v30;
  bool v31;
  uint* v32;
  __int64 v33;
  uint v34;
  uint v35;
  uint v36;
  __int64 v37;
  uint* v38;
  __int64 v39;
  __int64 v40;
  uint v41;
  uint v42;
  int v43;
  uint v44;
  __int64 v45;
  __int64 v46;
  int v47;
  uint* v48;
  uint* v49;
  uint* v50;
  int v51;
  uint v52;
  __int64 v53;
  qword v54;
  int v55;
  uint* v56;
  __int64 v57;
  __int64 v58;
  bool v59;
  int v60;
  int v61;
  char v62;
  __int64 v63;
  signed __int64 v64;
  qword v65;
  int v66;
  __int64 v67;
  __int64* v68;
  byte* v69;
  uint v70;
  __int64 v71;
  uint v72;
  __int64 v73;
  __int64 v74;
  uint* v75;
  int v76;
  __int64 v77;
  __int64 v78;
  __int64 v79;
  qword v80;
  uint v81;
  __int64 v82;
  __int64 v83;
  uint v84;
  uint v85;
  int v86;
  __int64 v87;
  uint v88;
  __int64 v89;
  __int64 v90;
  int v91;
  uint* v92;
  __int64 v93;
  uint* v94;
  int v95;
  __int64 v96;
  int v97;
  int v98;
  uint v99;
  __int64 v100;
  uint v101;
  byte* v102;
  byte v103;
  __int64* v104;
  char v105;
  char v106;
  uint v107;
  uint v108;
  uint v109;
  uint v110;
  byte* v111;
  byte* v112;
  byte* v113;
  __int64 v114;
  __int64 v115;
  __int64 v116;
  int v117;
  __int64 v118;
  qword v119;
  __int64 v120;
  __int64 v121;
  __int64 v122;
  short v123;
  v0 = q11;
  v1 = (byte*)q9;
  v2 = d5;
  v3 = d6;
  v4 = *(uint*)(q9+2);
  v123 = *(word*)q9;
  v5 = (byte*)q11;
  v122 = q11;
  v6 = *((byte*)x4+(byte)*(word*)q9);
  if( d5==d6&&v4 ) {
    v7 = sub_14000A2B0(1, (qword)q16, q10);
    *(uint*)(v1+2) = v7;
    if( v7 ) {
      result = q8+v7;
      q11 = result;
      q10 = result;
      return result;
    }
    goto LABEL_73;
  }
  v9 = q4;
  v10 = q8;
  *(byte*)q4 = *(word*)q9;
  v11 = q5;
  v12 = v9+1;
  q4 = v9+1;
  v13 = v9+1-v10;
  if( v9+1>=(qword)q5 )
    goto LABEL_73;
  *(byte*)(v9+1) = 0;
  if( !v4 ) {
    v14 = q10;
    v65 = q14;
    v66 = *v1;
    v112 = v5;
    v67 = q10;
    v115 = v0;
    v68 = q16;
    while( 1 ) {
      if( (qword)v68>=v65 ) {
        if( *(byte*)v67 ) {
          v69 = (byte*)(v10+*(uint*)(v67+4));
          if( *v69!=v66 ) {
            do {
              v97 = v69[6];
              v69 += 6;
            } while( v66!=v97 );
          }
        } else {
          v69 = (byte*)(v67+2);
        }
        *v68++ = (__int64)v69;
      } else {
        v69 = (byte*)*v68++;
      }
      v4 = *(uint*)(v69+2);
      if( v4 )
        break;
      *(uint*)(v69+2) = v13;
      v70 = *(uint*)(v67+8);
      d5 = --v2;
      if( !v70 ) {
        v5 = v112;
        v0 = v115;
        v4 = v67-v10;
        goto LABEL_9;
      }
      v67 = v10+v70;
    }
    v102 = v69;
    v5 = v112;
    v104 = v68;
    v0 = v115;
    if( v4<=v13 ) {
      v100 = v9;
      v110 = v9+1-v10;
      v106 = v6;
      v4 = sub_14000A2B0(0, (qword)(v104-1), v67);
      v6 = v106;
      v13 = v110;
      v9 = v100;
      *(uint*)(v102+2) = v4;
    }
    if( v2==v3-1&&v14==v115 ) {
      *(uint*)(v1+2) = v4;
      v4 = *(uint*)(v102+2);
      v12 = v9;
      q4 = v9;
    }
LABEL_9:
    if( v4 ) {
      v15 = v4;
      goto LABEL_11;
    }
LABEL_73:
    if( !d44 )
      return sub_140010A40(2);
    if( *(byte*)v0==1 ) {
      v71 = q6;
      if( !*v5&&(byte*)v0!=v5 ) {
        v73 = q8;
        v74 = LOBYTE(__arch_hdr[12]);
        v116 = v0;
        v75 = (uint*)(q6+12LL*LOBYTE(__arch_hdr[12]));
        v76 = q6+12*LOBYTE(__arch_hdr[12])-q8;
        v98 = q6-q8+444;
        v77 = v122;
        do {
          v78 = *(uint*)(v77+4);
          v79 = v73+v78;
          v80 = *(byte*)(v73+v78)!=v5[2];
          *(byte*)(v77+1) = *((byte*)x4+*(byte*)(v73+v78+6*v80));
          *(byte*)(v73+v78+6*v80+1) = ((uint)*(byte*)(v73+v78+6*v80+1)+3)>>2;
          *(word*)(v77+2) = *(word*)(v73+v78+6*v80);
          *(uint*)(v77+4) = *(uint*)(v73+v78+6*v80+2);
          v81 = *((byte*)&x1+v74);
          *(byte*)v77 = 0;
          if( *(byte*)(12LL*v81+v73+v78+1)==255 ) {
            v82 = 12LL*v81;
            do {
              *(uint*)(*(uint*)(v82+v79+4)+v73+8) = *(uint*)(v82+v79+8);
              *(uint*)(*(uint*)(v82+v79+8)+v73+4) = *(uint*)(v82+v79+4);
              v83 = 12LL**((byte*)&__arch_hdr[11]+*(byte*)(v82+v79)+3);
              --*(uint*)(v83+v71);
              v81 += *(byte*)(v79+v82);
              v82 = 12LL*v81;
            } while( *(byte*)(v82+v79+1)==255 );
            if( v81>0x80 ) {
              v113 = v5;
              v84 = v81;
              v85 = 0;
              v86 = 0;
              v87 = -((__int64)(((qword)((1LL-v81)>>6)>>57)-v81+1)>>7);
              do {
                v81 = v86+v84-128;
                v86 -= 128;
                ++v85;
              } while( v85<(uint)v87 );
              v88 = 0;
              do {
                *(byte*)(v79+1) = -1;
                *(uint*)(v79+8) = v98;
                ++v88;
                *(byte*)v79 = 0x80;
                *(uint*)(v79+4) = *(uint*)(v71+448);
                *(uint*)(v73+*(uint*)(v71+448)+8) = v78;
                ++*(uint*)(v71+444);
                *(uint*)(v71+448) = v78;
                v78 += 1536;
                v79 += 1536;
              } while( v88<(uint)v87 );
              v5 = v113;
            }
            v89 = *((byte*)&__arch_hdr[11]+v81+3);
            LODWORD(v90) = *((byte*)&x1+v89);
            if( v81!=(uint)v90 ) {
              v89 = (uint)(v89-1);
              v90 = *((byte*)&x1+v89);
              v91 = v81-v90;
              v92 = (uint*)(v71+12LL*(uint)(v91-1));
              v93 = v79+12*v90;
              *(byte*)(v93+1) = -1;
              *(byte*)v93 = v91;
              *(uint*)(v93+8) = v71+12*(v91-1)-v73;
              *(uint*)(v93+4) = v92[1];
              LODWORD(v93) = v93-v73;
              *(uint*)((uint)v92[1]+v73+8) = v93;
              v92[1] = v93;
              ++*v92;
            }
            *(byte*)(v79+1) = -1;
            v94 = (uint*)(v71+12*v89);
            *(byte*)v79 = v90;
            *(uint*)(v79+8) = (uint)(uintptr_t)v94-v73;
            *(uint*)(v79+4) = v94[1];
            v95 = v79-v73;
            *(uint*)((uint)v94[1]+v73+8) = v95;
            v94[1] = v95;
            ++*v94;
          } else {
            *(byte*)(v79+1) = -1;
            *(byte*)v79 = v81;
            *(uint*)(v79+8) = v76;
            *(uint*)(v79+4) = v75[1];
            *(uint*)((uint)v75[1]+v73+8) = v78;
            v75[1] = v78;
            ++*v75;
          }
          v77 = v73+*(uint*)(v77+8);
        } while( (byte*)v77!=v5 );
        v0 = v116;
      }
    } else {
      v71 = q6;
    }
    v72 = *(uint*)(v0+8);
    if( v72 ) {
      do {
        v0 = q8+v72;
        v72 = *(uint*)(v0+8);
      } while( v72 );
      q11 = v0;
    }
    result = sub_14000A4B0((byte*)v0, 0);
    ++d4;
    d3 = 0;
    q4 = v71+456;
    q10 = v0;
    d5 = 0;
    return result;
  }
  v14 = q10;
  v15 = v4;
  if( v11>v10+(qword)v4 ) {
    v107 = v9+1-v10;
    v105 = v6;
    v4 = sub_14000A2B0(0, (qword)q16, q10);
    v6 = v105;
    v13 = v107;
    goto LABEL_9;
  }
LABEL_11:
  v16 = v10+v15;
  v17 = *(uint*)(v10+v15+8);
  d5 = v2+1;
  _mm_prefetch((const char*)(v17+v10), 1);
  if( d5==v3 ) {
    v13 = v4;
    q4 = v12-(v0!=v14);
  }
  result = *(uint*)(v15+v10+4);
  _mm_prefetch((const char*)(result+v10), 1);
  if( v0!=v14 ) {
    v119 = q3;
    v18 = d45+8;
    v120 = q6;
    v121 = v10+v15;
    v114 = v0;
    if( d45+8>=14 )
      v18 = 14;
    if( v18<0 )
      v18 = 0;
    v19 = v6;
    v117 = q6-v10+444;
    v118 = v18;
    v20 = v13;
    while( 1 ) {
      v21 = *v5+1;
      if( *v5 ) {
        if( (v21&1)!=0 ) {
          v53 = *((uint*)v5+1);
        } else {
          v22 = *((uint*)v5+1);
          v23 = (uint*)(v10+v22);
          v24 = v21>>1;
          v25 = *((byte*)&__arch_hdr[11]+(v21>>1)+3);
          v26 = v21>>1;
          if( v25!=*((byte*)&__arch_hdr[12]+v26) ) {
            v27 = *((byte*)&__arch_hdr[12]+v26);
            v28 = (uint*)(v120+12*v27);
            if( *v28 ) {
              v29 = (uint*)(v10+(uint)v28[1]);
              v28[1] = v29[1];
              *(uint*)((uint)v29[1]+v10+8) = (uint)(uintptr_t)v28-v10;
              --*v28;
            } else {
              v29 = (uint*)q2;
              v30 = 12*(uint)*((byte*)&x1+v27);
              v31 = q2+v30==v119;
              if( q2+v30>v119 ) {
                v99 = *((byte*)&__arch_hdr[11]+v24+3);
                v108 = v20;
                v29 = (uint*)sub_140011240(v27);
                v20 = v108;
                v25 = v99;
              } else {
                q2 += v30;
                if( !v31 )
                  v29[v30/4] = 0;
              }
            }
            if( v29 ) {
              v32 = v29;
              v33 = v10+v22;
              if( (v21&2)!=0 ) {
                *v29 = *v23;
                v29[1] = v23[1];
                v29[2] = v23[2];
                v32 = v29+3;
                v33 = v10+v22+12;
              }
              if( (v24&0xFFFFFFFE)!=0 ) {
                v111 = v5;
                v34 = 0;
                v35 = 0;
                do {
                  v32[v35] = *(uint*)(v33+v35*4);
                  ++v34;
                  v32[v35+1] = *(uint*)(v33+v35*4+4);
                  v32[v35+2] = *(uint*)(v33+v35*4+8);
                  v32[v35+3] = *(uint*)(v33+v35*4+12);
                  v32[v35+4] = *(uint*)(v33+v35*4+16);
                  v32[v35+5] = *(uint*)(v33+v35*4+20);
                  v35 += 6;
                } while( v34<v21>>2 );
                v5 = v111;
              }
              v36 = *((byte*)&x1+v25);
              v37 = *((byte*)&x1+v25);
              if( BYTE1(v23[3*v37])==255 ) {
                v39 = 3*v37;
                do {
                  *(uint*)((uint)v23[v39+1]+v10+8) = v23[v39+2];
                  *(uint*)((uint)v23[v39+2]+v10+4) = v23[v39+1];
                  v40 = 12LL**((byte*)&__arch_hdr[11]+LOBYTE(v23[v39])+3);
                  --*(uint*)(v40+v120);
                  v36 += LOBYTE(v23[v39]);
                  v39 = 3LL*v36;
                } while( BYTE1(v23[v39])==255 );
                v41 = v36;
                if( v36>0x80 ) {
                  v42 = 0;
                  v101 = -(int)((__int64)(((qword)((1LL-v36)>>6)>>57)-v36+1)>>7);
                  v43 = 0;
                  do {
                    v36 = v43+v41-128;
                    v43 -= 128;
                    ++v42;
                  } while( v42<v101 );
                  v44 = 0;
                  do {
                    *((byte*)v23+1) = -1;
                    v23[2] = v117;
                    ++v44;
                    *(byte*)v23 = 0x80;
                    v23[1] = *(uint*)(v120+448);
                    *(uint*)(*(uint*)(v120+448)+v10+8) = v22;
                    ++*(uint*)(v120+444);
                    *(uint*)(v120+448) = v22;
                    v22 += 1536;
                    v23 += 384;
                  } while( v44<v101 );
                }
                v45 = *((byte*)&__arch_hdr[11]+v36+3);
                v103 = *((byte*)&x1+v45);
                if( v36!=v103 ) {
                  v45 = (uint)(v45-1);
                  v46 = *((byte*)&x1+v45);
                  v103 = *((byte*)&x1+v45);
                  v47 = v36-v46;
                  v48 = (uint*)(v120+12LL*(uint)(v47-1));
                  v49 = &v23[3*v46];
                  *((byte*)v49+1) = -1;
                  *(byte*)v49 = v47;
                  v49[2] = (uint)(uintptr_t)v48-v10;
                  v49[1] = v48[1];
                  LODWORD(v49) = (uint)(uintptr_t)v49-v10;
                  *(uint*)((uint)v48[1]+v10+8) = (uint)(uintptr_t)v49;
                  ++*v48;
                  v48[1] = (uint)(uintptr_t)v49;
                }
                *((byte*)v23+1) = -1;
                v50 = (uint*)(v120+12*v45);
                *(byte*)v23 = v103;
                v23[2] = (uint)(uintptr_t)v50-v10;
                v23[1] = v50[1];
                v51 = (uint)(uintptr_t)v23-v10;
                *(uint*)((uint)v50[1]+v10+8) = v51;
                ++*v50;
                v50[1] = v51;
              } else {
                *((byte*)v23+1) = -1;
                v38 = (uint*)(v120+12LL*v25);
                *(byte*)v23 = v36;
                v23[2] = (uint)(uintptr_t)v38-v10;
                v23[1] = v38[1];
                *(uint*)((uint)v38[1]+v10+8) = v22;
                ++*v38;
                v38[1] = v22;
              }
            }
            v23 = v29;
          }
          if( !v23 )
            goto LABEL_99;
          v52 = (uint)(uintptr_t)v23-v10;
          v53 = v52;
          *((uint*)v5+1) = v52;
        }
        v54 = v53+v10+6LL*v21;
        if( v54>v10+v53+42 ) {
          do {
            v55 = *(uint*)(v54-4);
            *(word*)v54 = *(word*)(v54-6);
            *(uint*)(v54+2) = v55;
            v54 -= 6LL;
          } while( v54>v10+(qword)*((uint*)v5+1)+42 );
        }
      } else {
        v56 = (uint*)(v120+12LL*LOBYTE(__arch_hdr[12]));
        if( *v56 ) {
          v57 = v10+(uint)v56[1];
          v56[1] = *(uint*)(v57+4);
          *(uint*)(*(uint*)(v57+4)+v10+8) = (uint)(uintptr_t)v56-v10;
          --*v56;
        } else {
          v57 = q2;
          v58 = 12*(uint)*((byte*)&x1+LOBYTE(__arch_hdr[12]));
          v59 = q2+v58==v119;
          if( q2+v58>v119 ) {
            v109 = v20;
            v96 = sub_140011240(LOBYTE(__arch_hdr[12]));
            v20 = v109;
            v57 = v96;
          } else {
            q2 += v58;
            if( !v59 )
              *(uint*)(v58+v57) = 0;
          }
        }
        if( !v57 ) {
LABEL_99:
          v0 = v114;
          goto LABEL_73;
        }
        *(word*)v57 = *((word*)v5+1);
        v60 = b24[v118];
        *(uint*)(v57+2) = *((uint*)v5+1);
        *((uint*)v5+1) = v57-v10;
        v61 = 4**(byte*)(v57+1)+v60;
        if( v61>=238 )
          v61 = 238;
        if( v61<2 )
          LOBYTE(v61) = 2;
        *(byte*)(v57+1) = v61;
        v54 = v57+6;
        *((word*)v5+1) = (byte)v61;
      }
      *(byte*)(v54+1) = 0;
      *(uint*)(v54+2) = v20;
      *(byte*)v54 = v123;
      v62 = v5[1]&0xF0;
      v63 = v10+*((uint*)v5+1);
      ++*v5;
      v64 = v54-v63;
      result = 0x2AAAAAAAAAAAAAABLL*v64;
      v5[1] = v19|(v64/6)|v62;
      v5 = (byte*)(v10+*((uint*)v5+2));
      if( v5==(byte*)v14 ) {
        v16 = v121;
        break;
      }
    }
  }
  q10 = v16;
  q11 = v16;
  return result;
}

PROBE_DECL(sub_140007B80)
qword sub_140007B80(byte* a1) {
  PROBE_HIT(sub_140007B80);
  uint* v1;
  int v2;
  int v3;
  int v4;
  uint* v5;
  int v6;
  uint* v7;
  uint* v8;
  int v9;
  uint* v10;
  int v11;
  int v12;
  uint* v13;
  int v14;
  short v15;
  __int64 v16;
  int v17;
  char* v18;
  int* v19;
  int v20;
  int v21;
  int v22;
  int* v23;
  int v24;
  int v25;
  byte* v26;
  uint v27;
  __int64 v28;
  int v29;
  uint v30;
  int v31;
  int v32;
  int v33;
  uint v34;
  int v35;
  __int64 v36;
  uint v37;
  int v38;
  short v39;
  char v40;
  __int64 v41;
  __int64 v42;
  byte v43;
  __int64 i;
  int v45;
  int v46;
  int v47;
  int v48;
  int v49;
  int v50;
  int v51;
  char v52;
  __int64 v53;
  int v54;
  int v55;
  __int64 v56;
  int v57;
  __int64 v58;
  int v59;
  __int64 v60;
  int v61;
  __int64 v62;
  int v63;
  __int64 v64;
  int v65;
  __int64 v66;
  int v67;
  __int64 v68;
  int v69;
  int v70;
  int v71;
  int v72;
  int v73;
  int v74;
  __int64 v75;
  __int64 v76;
  __int64 v77;
  __int64 v78;
  __int64 v79;
  __int64 v80;
  __int64 v81;
  int v82;
  int v83;
  int v84;
  __int64 v85;
  __int64 v86;
  int v87;
  int v88;
  qword v89;
  int v90;
  __int64 v91;
  __int64 v92;
  bool v93;
  int v94;
  __int64 v95;
  __int64 v96;
  __int64 v97;
  __int64 v98;
  int v99;
  __int64 v100;
  __int64 v101;
  __int64 v102;
  int v103;
  __int64 v104;
  __int64 v105;
  char* v106;
  uint v107;
  uint v108;
  char* v109;
  int v110;
  __int64 v111;
  __int64 v112;
  int v113;
  int v114;
  int v115;
  int v116;
  uint v117;
  __int64* v118;
  int v119;
  qword result;
  __int64 v121;
  byte* v122;
  int v123;
  int v124;
  int v125;
  uint v126;
  int v127;
  __int64 v128;
  byte v129;
  qword v130;
  byte* v131;
  qword v132;
  byte* v133;
  short v134;
  int v135;
  byte* v136;
  __int64 v137;
  __int64 v138;
  __int64 v139;
  byte v140;
  qword v141;
  byte* v142;
  byte* v143;
  int v144;
  char v145;
  byte v146;
  byte v147;
  int v148;
  int v149;
  short v150;
  short v151;
  byte v152;
  int v153;
  int v154;
  int v155;
  int v156;
  __int64 v157;
  int v158;
  int v159;
  int v160;
  int v161;
  int v162;
  int v163;
  int v164;
  uint v165;
  int v167;
  __int64 v168;
  byte* v169;
  int v170;
  short v171;
  short v172;
  int v173;
  int v174;
  int v175;
  v1 = (uint*)q17;
  v2 = d46;
  v3 = d47;
  v4 = d48;
  v5 = (uint*)q18;
  v7 = (uint*)q19;
  v8 = (uint*)q20;
  v170 = d8;
  v9 = d8-1;
  *(uint*)q21 += *(word*)(q21+6);
  v10 = (uint*)q22;
  v11 = d49;
  d8 = v9;
  *v1 += v2;
  v12 = d50;
  rc.CommitRange();
  *v8 += v3;
  v13 = (uint*)q23;
  *v10 += v4;
  v14 = d25;
  v15 = d25;
  v16 = d25&0x1FFFF;
  *v5 += v11;
  *v13 += v12;
  v17 = d51;
  v13[1] -= d52;
  v18 = &b12[v16+133144];
  v19 = (int*)q24;
  *v7 += d53;
  v7[1] -= d54;
  v20 = d55+d56;
  v21 = *v19-(d55+d56);
  *v19 = v21;
  v22 = v19[1]+v20-2*v17;
  if( v22>0x40000000 ) {
    *v19 = v21>>1;
    v22 >>= 1;
  }
  v19[1] = v22;
  v23 = (int*)q25;
  v24 = *(uint*)q25-d55;
  v25 = *(uint*)(q25+4)+d55-v17;
  if( v25<=0x40000000 ) {
    *(uint*)(q25+4) = v25;
    *v23 = v24;
  } else {
    *(uint*)q25 = v24>>1;
    v23[1] = v25>>1;
  }
  v26 = (byte*)q9;
  v169 = (byte*)q9;
  d57[d58] |= d59;
  d60[d61] |= d59;
  v27 = *v26;
  LODWORD(v28) = d62;
  b14[d63] = v27;
  *(v18-0x20000) = v27;
  *v18 = v27;
  d64 = v27;
  d62 = v27;
  d63 = (v27+(d63<<6))&0x1FFFF;
  if( d18>=0&&d18!=d13 )
    b25 += b25+(v27==d18);
  d65 = d66;
  d67 = d68;
  d69 = ((v28&0xFFFFFFF8)<<10)+32*(v27&0xFFFFFFF8);
  v29 = d11;
  v30 = *((byte*)x4+v27);
  d9 += d11;
  d70 = (8*(v27+d70))&0xFFF00;
  v31 = d23[v27];
  v164 = d12+d12+(v30>>7);
  d12 = v164;
  d71[v15&0xFFF] = v31;
  d23[v27] = v14;
  v32 = v14-v31;
  if( (uint)(v14-v31)>=0x104 ) {
    d66 = 0;
    d72 = -1;
  } else {
    v33 = (byte)b26[(v31+1)&0x1FFFF];
    d72 = v33;
    if( v32>=50 )
      v33 = 0;
    v34 = 192;
    d66 = v33;
    v35 = d71[v31&0xFFF];
    if( (uint)(v14-v35)<0xC0 ) {
      do {
        --v34;
        d73[(byte)b26[(v35+1)&0x1FFFF]] = v9;
        v35 = d71[v35&0xFFF];
      } while( v34>v14-v35 );
    }
  }
  v36 = v27+(__int64)(int)((uint)v28<<8);
  v156 = d24[v36];
  d74[(v14-1)&0x1FFFF] = v156;
  d24[v36] = v14-1;
  v37 = v14-v156;
  d75 = v37;
  v162 = (((uint)(v14-v156)<0xE800)+(v37<0xF0)+(v37<7))<<13;
  if( (uint)(v14-v156)>=0x1000 ) {
    d68 = 0;
    v38 = 0;
    d76 = -1;
  } else {
    v38 = (byte)b26[(v156+2)&0x1FFFF];
    d68 = v38;
    d76 = (byte)b26[(d74[v156&0x1FFFF]+2)&0x1FFFF];
    if( v38==d76 ) {
      d77[v38] = v9;
      v38 = d68;
    }
    if( v37>=0x240 )
      v38 = 0;
  }
  v39 = d78;
  v40 = 2*d79;
  d79 *= 2;
  if( v27!=d78 ) {
    v41 = q12;
    *(uint*)(q12+512) += 2;
    v157 = v41;
    v42 = ((word)v27-v39)&0x1FF;
    v43 = *(byte*)(v42+v41)+2;
    *(byte*)(v42+v41) = v43;
    if( v43>0xA7u ) {
      *(uint*)(v41+512) = 0;
      for( i = 0; i<512; ++i ) {
        v45 = *(byte*)(i+v157)>>1;
        *(byte*)(i+v157) >>= 1;
        *(uint*)(v157+512) += v45;
      }
    }
    v40 = ++d79;
    if( v27!=d67 ) {
      b27[d78+(d67<<8)] = v27;
      v40 = d79;
    }
  }
  v28 = (int)v28;
  v168 = v28;
  q12 = (__int64)&b12[516*(v40&3)];
  v175 = d23[(int)v28];
  if( v27==d18&&d17<=256 ) {
    d80 = 4*d17;
  } else if( d80>(uint)(3*d17)&&(v170==d81[v27]||v170==d73[v27]||4*d17-9<(uint)d80) ) {
    d80 -= d17>13;
  } else if( v32>1 ) {
    v46 = d71[v31&0xFFF];
    if( v32==v31-v46&&v32==v46-d71[v46&0xFFF]&&v32<=256 ) {
      if( v27==(byte)v18[-4*v32]||(v47 = d71[v175&0xFFF], v32==v175-v47)&&v32==v47-d71[v47&0xFFF]&&(uint)v28==(byte)v18[-3*v32-1] ) {
        d17 = v32;
        d80 = v32;
        q26 = (__int64)&d82;
      }
    }
  }
  d83 = -1;
  d84 = -1;
  d85 = -1;
  d86 = -1;
  d87 = -1;
  d19 = -1;
  v48 = (byte)(v27-v28);
  d20 = -1;
  v49 = v14+1;
  d25 = v14+1;
  if( v48==d88 ) {
    if( (uint)++ d89>1 ) {
      v38 = (byte)(2*v27-v28);
      d18 = v38;
      goto LABEL_94;
    }
    d18 = -1;
  } else {
    d18 = -1;
    d89 = 0;
    d88 = v48-(v48==0);
  }
  v50 = (byte)*(v18-3);
  v51 = (byte)*(v18-7);
  if( v50==v51||(v52 = *(v18-11), (byte)(v52+(byte)v50-2*(byte)v51))||(byte)(*(v18-15)+(byte)v51-2*v52) ) {
    if( 4*d17-1>(uint)d80 ) {
      LODWORD(v53) = -1;
      v54 = d74[(v14-3)&0x1FFFF];
      if( (uint)(v49-v54)<0x20000 ) {
        v53 = (byte)b26[(v54+4)&0x1FFFF];
        d81[v53] = v9;
        v55 = d74[*(qword*)&v54&0x1FFFFLL];
        if( (uint)(v49-v55)<0x20000 ) {
          v56 = (byte)b26[(v55+4)&0x1FFFF];
          d81[v56] = v9;
          if( (uint)v53==(uint)v56 ) {
            if( d20<0 ) {
              d20 = v53;
            } else if( d20==(uint)v53 ) {
              d19 = v53;
            }
            d77[v53] = v9;
          }
        }
      }
      d86 = v53;
      v57 = d74[(v14-4)&0x1FFFF];
      if( (uint)(v49-v57)<0x20000 ) {
        v58 = (byte)b26[(v57+5)&0x1FFFF];
        d81[v58] = v9;
        v59 = d74[*(qword*)&v57&0x1FFFFLL];
        if( (uint)(v49-v59)<0x20000 ) {
          v60 = (byte)b26[(v59+5)&0x1FFFF];
          d81[v60] = v9;
          if( (uint)v58==(uint)v60 ) {
            if( d20<0 ) {
              d20 = v58;
            } else if( d20==(uint)v58 ) {
              d19 = v58;
            }
            d77[v58] = v9;
          }
        }
      }
      v61 = d74[(v14-5)&0x1FFFF];
      if( (uint)(v49-v61)<0x20000 ) {
        v62 = (byte)b26[(v61+6)&0x1FFFF];
        d81[v62] = v9;
        v63 = d74[*(qword*)&v61&0x1FFFFLL];
        if( (uint)(v49-v63)<0x20000 ) {
          v64 = (byte)b26[(v63+6)&0x1FFFF];
          d81[v64] = v9;
          if( (uint)v62==(uint)v64 ) {
            if( d20<0 ) {
              d20 = v62;
            } else if( d20==(uint)v62 ) {
              d19 = v62;
            }
            d77[v62] = v9;
          }
        }
      }
      v65 = d74[(v14-8)&0x1FFFF];
      if( (uint)(v49-v65)<0x20000 ) {
        v66 = (byte)b26[(v65+9)&0x1FFFF];
        d81[v66] = v9;
        v67 = d74[*(qword*)&v65&0x1FFFFLL];
        if( (uint)(v49-v67)<0x20000 ) {
          v68 = (byte)b26[(v67+9)&0x1FFFF];
          d81[v68] = v9;
          if( (uint)v66==(uint)v68 ) {
            if( d20<0 ) {
              d20 = v66;
            } else if( d20==(uint)v66 ) {
              d19 = v66;
            }
            d77[v66] = v9;
          }
        }
      }
      v69 = d74[(v14-6)&0x1FFFF];
      if( (uint)(v49-v69)<0x10000 ) {
        v158 = (byte)b26[(v69+7)&0x1FFFF];
        d81[(byte)b26[(v69+7)&0x1FFFF]] = v9;
        v70 = d74[*(qword*)&v69&0x1FFFFLL];
        if( (uint)(v49-v70)<0x20000 ) {
          v71 = (byte)b26[(v70+7)&0x1FFFF];
          d81[512*(v71==v158)+v71] = v9;
        }
      }
      v72 = d74[(v14-10)&0x1FFFF];
      if( (uint)(v49-v72)<0x10000 ) {
        v159 = (byte)b26[(v72+11)&0x1FFFF];
        d81[(byte)b26[(v72+11)&0x1FFFF]] = v9;
        v73 = d74[*(qword*)&v72&0x1FFFFLL];
        if( (uint)(v49-v73)<0x20000 ) {
          v74 = (byte)b26[(v73+11)&0x1FFFF];
          d81[512*(v74==v159)+v74] = v9;
        }
      }
      v75 = (byte)b13[256*(byte)*(v18-1)+(byte)*(v18-3)];
      d81[256*(v9==d81[v75])+(int)v75] = v9;
      b13[256*(byte)*(v18-2)+(byte)*(v18-4)] = v27;
      v76 = (byte)b28[256*(byte)*(v18-2)+(byte)*(v18-6)];
      d81[256*(v9==d81[v76])+(int)v76] = v9;
      b28[256*(byte)*(v18-3)+(byte)*(v18-7)] = v27;
      v77 = (byte)b29[256*(byte)*v18+(byte)*(v18-7)];
      d81[256*(v9==d81[v77])+(uint)v77] = v9;
      b29[256*(byte)*(v18-1)+(byte)*(v18-8)] = v27;
      d85 = v77;
      v78 = (byte)b30[256*(byte)*(v18-1)+(byte)*(v18-7)];
      d81[256*(v9==d81[v78])+(int)v78] = v9;
      b30[256*(byte)*(v18-2)+(byte)*(v18-8)] = v27;
      v79 = (uint)(d68+(d66<<8));
      v80 = (byte)b31[v79];
      d83 = (byte)b31[v79];
      d81[v80] = v9;
      if( v27!=d65&&v27!=d67 )
        b31[d67+(d65<<8)] = v27;
      v81 = (byte)b26[(d71[d23[v168]&0xFFFLL]+2)&0x1FFFF];
      d81[256*(v9==d81[v81])+v81] = v9;
    }
  } else {
    v38 = (byte)(2*v50-v51);
    d18 = v38;
  }
LABEL_94:
  v82 = d74[(v14-2)&0x1FFFF];
  if( (uint)(v49-v82)<0x20000 ) {
    v83 = (byte)b26[(v82+3)&0x1FFFF];
    d84 = v83;
    d73[v83] = v9;
    v84 = d74[v82&0x1FFFF];
    if( (uint)(v49-v84)<0x20000 ) {
      v85 = (byte)b26[(v84+3)&0x1FFFF];
      d73[v85] = v9;
      if( v83==(uint)v85 )
        d77[v83] = v9;
      LODWORD(v86) = -1;
      v87 = d74[v84&0x1FFFF];
      if( (uint)(v49-v87)<0x20000 ) {
        v88 = 0;
        do {
          v88 += 6144;
          v86 = (byte)b26[(v87+3)&0x1FFFF];
          d73[v86] = v9;
          v83 &= v86;
          LODWORD(v85) = v86|v85;
          v87 = d74[v87&0x1FFFF];
        } while( (uint)(v88+v49-v87)<0x20000 );
      }
      if( v83==(uint)v85&&(int)v86>=0 ) {
        if( d20<0 ) {
          d20 = v83;
        } else if( d20==v83 ) {
          d19 = v83;
        }
      }
    }
  }
  v89 = (v49&1)==0;
  v90 = d90[v89];
  v91 = (word)(16*LOWORD(d90[v89])+((v27>>2)&0xFFFC))&0xFFFC;
  d90[v89] = v91;
  v92 = (byte)b32[v91];
  v93 = !v89;
  v94 = (byte)b33[v91];
  v95 = (byte)b33[v91];
  v96 = (uint)d90[v93];
  d73[v92] = v9;
  d73[v95] = v9;
  if( v94==(uint)v92 ) {
    if( v94==d20 )
      d19 = v94;
    d77[v95] = v9;
  }
  b32[v96] = b33[v96];
  b33[v96] = v27;
  v97 = (uint)d91;
  d92 = v94;
  v98 = (word)(((v27>>4)&0xFFFE)+8*d91)&0xFFFE;
  v99 = (byte)b34[v98];
  v100 = (byte)b34[v98];
  v101 = (byte)b35[v98];
  d73[v101] = v9;
  d73[v100] = v9;
  if( v99==(uint)v101 ) {
    if( v99==d20 )
      d19 = v99;
    d77[v100] = v9;
  }
  b35[v97] = b34[v97];
  b34[v97] = v27;
  d91 = v98;
  v102 = (uint)d90[v93];
  v103 = (byte)b36[v102];
  v104 = (byte)b37[v102];
  v105 = (byte)b36[v102];
  d73[v104] = v9;
  d73[v105] = v9;
  if( v103==(uint)v104 ) {
    if( v103==d20 )
      d19 = v103;
    d77[v105] = v9;
  }
  b37[v90] = b36[v90];
  b36[v90] = v27;
  if( d80 ) {
    if( v27==*(byte*)q26 ) {
      *(byte*)(q26+3) += *(byte*)(q26+3)-255<0;
    } else {
      *(byte*)(q26+2) = *(byte*)(q26+1);
      *(byte*)(q26+1) = *(byte*)q26;
      *(byte*)q26 = v27;
      *(byte*)(q26+3) = 0;
    }
    v106 = v18+1;
    v107 = (byte)v106[-d17];
    v108 = (byte)v106[-2*d17];
    v160 = (byte)v106[-3*d17];
    if( --d80>(uint)d17 )
      d64 = v107;
    v109 = &v106[-d17];
    v110 = (byte)*(v109-1);
    v111 = ((v109[2]==v106[-2*d17+2])<<12)+(((v108&0x2E)+((byte)v106[-2*d17+1]<(uint)(byte)v109[1]))<<8)+((((v110+32-v27)>>31)+((v110-v27)>>31)+((int)v27>=v110))<<14);
    q26 = (__int64)&b15[4*v111+4*v107];
    d81[(byte)b15[4*v111+2+4*v107]] = v9;
    d81[*(byte*)(q26+1)] = v9;
    v112 = *(byte*)q26;
    d87 = *(byte*)q26;
    d81[v112] = v9;
    if( *(byte*)(q26+3) ) {
      if( *(byte*)(q26+3)>1u||d72<=0 )
        d72 = *(byte*)q26;
      d77[*(byte*)q26] = v9;
    }
    if( v107==v108&&v108==v160 ) {
      d15 = v38;
      d18 = v107;
      d77[(byte)(v107+1)] = v9;
      d73[(byte)(v107-1)] = v9;
    } else if( d18<0 ) {
      if( v107==v108||v108==v160 ) {
        d15 = v38;
        d18 = v107;
        d73[(byte)(v160+1)] = v9;
        d77[(byte)(v107+1)] = v9;
        d73[(byte)(v107-1)] = v9;
        d77[v160] = v9;
      } else if( v107==v160 ) {
        d15 = v38;
        if( v108==(byte)v106[-4*d17]&&(byte)v106[-5*d17]==v160 )
          v107 = v108;
        d18 = v107;
        d77[v108] = v9;
      } else {
        v145 = v107+v160-2*v108;
        if( v145 ) {
          d15 = v38;
          if( *(byte*)(q26+3)<=0x10u ) {
            if( (byte)(v145+19)<=0x26u ) {
              v146 = 2*v107-v108;
              v147 = v146-v145;
              d81[(byte)(v147+2)] = v9;
              d81[(byte)(v147-2)] = v9;
              d81[(byte)(v147+1)] = v9;
              d81[v147] = v9;
              d73[(byte)(v146+2)] = v9;
              d77[(byte)(v146-2)] = v9;
              d77[(byte)(v146+1)] = v9;
              d77[(byte)(v146-1)] = v9;
              d77[v146] = v9;
            }
          } else {
            d18 = *(byte*)q26;
          }
        } else {
          d18 = (byte)(2*v107-v108);
          d15 = (byte)(2*v107-v108);
          d81[(byte)(d18+1)] = v9;
          d81[(byte)(d18-1)] = v9;
          d81[(byte)(d18+2)] = v9;
          d81[(byte)(d18-2)] = v9;
          d77[(byte)(d18+1)] = v9;
          d77[(byte)(d18-1)] = v9;
          d77[d18] = v9;
        }
      }
    } else {
      d15 = v38;
    }
  } else {
    d15 = v38;
    d17 = 1024;
    if( d18<0 )
      d18 = d19;
  }
  v113 = d5;
  q16[0] = (__int64)v169;
  v114 = (d5>0)<<15;
  d33 = v114+v162+4*(d64&0x80);
  v115 = *a1;
  d10 = v115;
  v116 = *v169;
  v117 = *((uint*)a1+2);
  d38 = ((d5>2)<<15)+v114+*((uint*)x20+(v164&0xF))+(v29<<14);
  v118 = &q27;
  d41 = (((d5>32)+(d5>8)+(d5>4)+(d5>3)+(d5>2)+(d5>1))<<12)+64;
  if( v117 ) {
    v121 = q8;
    v122 = (byte*)(q8+v117);
    v123 = d5;
    v124 = 5*d5;
    if( !*v122 ) {
      do {
        if( v118>&q28[1] )
          *(byte*)(*(v118-2)+1) += *(byte*)(*(v118-2)+1)-2<0;
        v136 = v122+2;
        v137 = *((uint*)v122+2);
        *v118++ = (__int64)v136;
        --v123;
        v122 = (byte*)(v121+v137);
      } while( !*v122 );
      v119 = d6;
      if( v113<d6 ) {
        v138 = q16[0];
        if( *(byte*)(q16[0]+1)<7u ) {
          if( v118<=&q28[1] ) {
            ++*(byte*)(q27+1);
          } else {
            ++*(byte*)(q28[0]+1);
            if( v118>(__int64*)b38 )
              *(byte*)(v138+1) += *(byte*)(v138+1)-4<0;
          }
        }
      }
      v139 = *((uint*)v122+1);
      v140 = v122[1];
      goto LABEL_165;
    }
    v125 = 2;
    if( v115 )
      v126 = (-45*v115+(uint)*((word*)a1+1))>>31;
    else
      v126 = d93==0;
    v165 = v126;
    v161 = d5;
    v167 = d5+3;
    v163 = d5+1;
    v127 = 3*d5;
    do {
      v128 = *((uint*)v122+1);
      v129 = v122[1];
      if( !*(byte*)(v121+v128+6LL*(v129&0xF)+1) ) {
        q14 = (__int64)v118;
        sub_14000C0E0(v122);
        v128 = *((uint*)v122+1);
        v129 = v122[1];
      }
      v122[1] = v129&0xF0;
      v130 = v121+v128;
      v131 = (byte*)(v121+v128);
      if( *(byte*)(v121+v128)!=v116 ) {
        do
          v131 += 6;
        while( *v131!=v116 );
        v171 = *(word*)v131;
        v132 = v121+v128+42;
        v173 = *(uint*)(v131+2);
        while( 1 ) {
          if( (qword)v131<=v130||(v133 = v131-6, HIBYTE(v171)+13<=*(v131-5)) ) {
            if( (qword)v131<=v132 ) {
              *(word*)v131 = v171;
              *(uint*)(v131+2) = v173;
              v122[1] |= (__int64)&v131[-v130]/6;
              break;
            }
            v133 = v131-6;
          }
          v134 = *(word*)v133;
          v135 = *(uint*)(v133+2);
          *(word*)v131 = v134;
          *(uint*)(v131+2) = v135;
          v131 -= 6;
        }
      }
      v148 = v169[1];
      *v118++ = (__int64)v131;
      v124 -= 5;
      v127 -= 3;
      --v123;
      if( v148>130||v131[1]>=0xE4u ) {
        v153 = v163;
        v113 = v161;
        v119 = d6;
        v122 = (byte*)(v121+*((uint*)v122+2));
        goto LABEL_201;
      }
      v149 = *((word*)v122+1);
      v150 = v125+(v124>v167);
      v125 >>= 1;
      v151 = (7*v149<4*v131[1]**v122+4*(uint)v131[1])+(2*v123>v163)+v150;
      *((word*)v122+1) = v151+v149;
      v152 = v151+v131[1];
      v131[1] = v152;
      v122 = (byte*)(v121+*((uint*)v122+2));
    } while( v152<0x45u&&v127>v167&&v165 );
    v153 = v163;
    v113 = v161;
    v119 = d6;
LABEL_201:
    while( v153<4*v123 ) {
      v139 = *((uint*)v122+1);
      v140 = v122[1];
      if( *(byte*)(v121+v139+6LL*(v140&0xF))==v116 )
        break;
LABEL_165:
      v141 = v121+v139;
      if( !*(byte*)(v121+v139+6LL*(v140&0xF)+1) ) {
        q14 = (__int64)v118;
        sub_14000C0E0(v122);
        v139 = *((uint*)v122+1);
        v140 = v122[1];
        v141 = v121+v139;
      }
      v122[1] = v140&0xF0;
      v142 = (byte*)v141;
      if( *(byte*)(v139+v121)!=v116 ) {
        do
          v142 += 6;
        while( *v142!=v116 );
        v172 = *(word*)v142;
        v174 = *(uint*)(v142+2);
        while( 1 ) {
          if( (qword)v142<=v141||(v143 = v142-6, HIBYTE(v172)<(int)*(v142-5)) ) {
            if( (qword)v142<=v141+42 ) {
              *(word*)v142 = v172;
              *(uint*)(v142+2) = v174;
              v122[1] |= (__int64)&v142[-v141]/6;
              break;
            }
            v143 = v142-6;
          }
          v144 = *(uint*)(v143+2);
          *(word*)v142 = *(word*)v143;
          *(uint*)(v142+2) = v144;
          v142 -= 6;
        }
      }
      v154 = *(byte*)(v139+v121+1);
      v155 = *(byte*)(v139+v121+7);
      *v118++ = (__int64)v142;
      if( (uint)(v155+v154)>0x5F )
        break;
      v122 = (byte*)(v121+*((uint*)v122+2));
      --v123;
      v153 = v113+1;
    }
    q14 = (__int64)v118;
  } else {
    v119 = d6;
    q14 = (__int64)&q27;
  }
  if( v113==v119&&(result = q8+*(uint*)(v169+2), result>=q5) ) {
    q11 = q8+*(uint*)(v169+2);
    q10 = result;
  } else {
    result = sub_140009620();
    v113 = d5;
  }
  d7 = v113;
  return result;
}

PROBE_DECL(sub_1400037A0)
static void __PPMContextWalk(int v2, int v221, uint* outV246, uint* outV250, int* outV257, __int64* outV547, int* outV549) {
  __int64 v220 = q8;
  byte* v3 = (byte*)q10;
  byte* v222;
  __int64 v223;
  byte* v224;
  __int64 v225;
  __int64 v226;
  __int64 v227;
  byte* v228;
  uint v229;
  byte v230;
  int v231;
  byte* v232;
  byte* v233;
  int v234;
  uint v235;
  uint v236;
  __int64 v237;
  int v238;
  uint v240;
  int v241;
  uint v242;
  uint v243;
  uint v244;
  uint v245;
  uint v246;
  __int64 v247;
  int v248;
  __int64 v249;
  uint v250;
  int v251;
  int v252;
  int v253;
  int v254;
  uint v255;
  uint v256;
  int v257;
  __int64 v258;
  uint v259;
  uint v260;
  int v441;
  byte* v475;
  int v476;
  int v477;
  int v478;
  __int64 v494;
  __int64 v495;
  byte* j;
  byte* v542;
  int v543;
  uint v544;
  uint v545;
  uint v546;
  __int64 v547;
  uint v548;
  int v549;

  v222 = v3;
  v223 = q16;
  d78 = v221;
  d81[(byte)b27[v221+(d68<<8)]] = v2;
  do {
    v224 = v222+2;
    v225 = *((uint*)v222+2);
    *v223++ = (__int64)v224;
    v222 = (byte*)(v220+v225);
    LODWORD(v226) = *v222;
  } while( !*v222 );
  v227 = *((uint*)v222+1);
  v546 = v223-q16;
  v545 = v546;
  v228 = (byte*)(v220+v227+6LL**v222);
  if( !v228[1] ) {
    v229 = v223-q16;
    do {
      if( *v228!=v221 )
        break;
      v222 = (byte*)(v220+*((uint*)v222+2));
      v227 = *((uint*)v222+1);
      v226 = *v222;
      ++v229;
      v228 = (byte*)(v220+v227+6*v226);
    } while( !v228[1] );
    v545 = v229;
  }
  v230 = v222[1];
  v544 = v226+1;
  if( !*(byte*)(v227+v220+6LL*(v230&0xF)+1) ) {
    v494 = v223;
    sub_14000C0E0(v222);
    v223 = v494;
    v227 = *((uint*)v222+1);
    v230 = v222[1];
  }
  v231 = *(byte*)(v227+v220+6LL*(v230&0xF));
  v232 = (byte*)(v220+v227);
  v543 = v231;
  d14 = v231;
  v542 = v232;
  if( v221!=*v232 ) {
    v233 = v232;
    do
      v233 += 6;
    while( v221!=*v233 );
    v542 = v233;
  }
  v234 = v542[1];
  v235 = *((word*)v222+1);
  if( 2*v234+23>v232[7]+v232[1] ) {
    v236 = *((uint*)v222+2);
    if( v236 ) {
      v475 = (byte*)(v220+v236);
      v478 = *((word*)v475+1);
      v476 = *v475;
      if( v235+v235*v476<v544*(v478+15) ) {
        v237 = *((uint*)v475+1);
        v477 = v476+1;
        if( !*(byte*)(v220+v237+6LL*(v475[1]&0xF)+1) ) {
          v495 = v223;
          sub_14000C0E0(v475);
          v223 = v495;
          v234 = v542[1];
          v235 = *((word*)v222+1);
          v237 = *((uint*)v475+1);
          v478 = *((word*)v475+1);
        }
        v238 = 5*v226+5;
        for( j = (byte*)(v220+v237); v221!=*j; j += 6 )
          ;
        v441 = j[1];
        v240 = v235+2*v238;
        v241 = (2*(2*v238<v235)+5)*v477+v478;
        if( v241*v234<(int)(v240*v441) ) {
          if( v234>228 )
            goto LABEL_164;
          v242 = v235-v234;
          v243 = ((v240-v234)*(v441+3*v234)+3*(v240-v234)+v241-v441-4)/(3*(v240-v234)+v241-v441);
          if( v243>v234+11 )
            LOWORD(v243) = v234+11;
          LOWORD(v235) = v243+v242;
          *((word*)v222+1) = v235;
          v542[1] = v243;
          v234 = (byte)v243;
          v235 = (word)v235;
        }
      }
    }
  }
  if( v234<=9 ) {
    v244 = (byte)__algn_141136BC4[v234+1083];
    goto LABEL_166;
  }
LABEL_164:
  v244 = v234-6;
LABEL_166:
  v245 = v235-v244;
  v246 = (6*v245<v244)+(15*v245<4*v244)+(9*v245<4*v244)+(13*v245<8*v244)+(v245<v244)+(v245<2*v244)+(v245<6*v244)+(35*v245<4*v244)+1;
  do {
    v247 = *--v223;
    if( v246>*(byte*)(*v223+1) )
      *(byte*)(v247+1) = v246;
    else
      v246 = *(byte*)(*v223+1);
  } while( v223!=q16 );
  v547 = v247;
  d93 = v246-((6*v245<v244)+(15*v245<4*v244)+(9*v245<4*v244)+(13*v245<8*v244)+(v245<v244)+(v245<2*v244)+(v245<6*v244)+(35*v245<4*v244)+1);
  d59 = 1<<v221;
  d58 = (uint)(v221+d69)>>5;
  d61 = (uint)(v221+d70)>>5;
  v549 = (((1<<v221)&d57[(uint)(v221+d69)>>5])!=0)+2*(((1<<v221)&d60[(uint)(v221+d70)>>5])!=0);
  v248 = d24[v221+(d62<<8)];
  if( (uint)(d25-v248)>=0x20000 ) {
    d107 = 0x20000;
    d108 = 0x20000;
    d109 = 0x20000;
  } else {
    d108 = d24[v221+(d62<<8)];
    d109 = (byte)b26[(v248+2)&0x1FFFF];
    d108 = d25-v248;
    d107 = d25-d24[d109+(v221<<8)];
  }
  v249 = *((uint*)v3+2);
  v250 = *(byte*)(v249+v220);
  v251 = 32*(d9>0)+(v3[1]&0x80);
  d41 += (v250==0)<<7;
  v548 = d5-v545;
  v252 = d11+((4*(d5-v545>3)+8*(v221==v543)+(v221==d18)+2*(v2==d81[v221]))<<9)+v251+((v549==3||v2==d77[v221])<<13);
  v253 = (((d12&6)==6)<<8)+((d12&1)<<6);
  v254 = v252+v253;
  if( v250 ) {
    v255 = v254&0xFFFFFFDE;
    v256 = v246!=(6*v245<v244)+(15*v245<4*v244)+(9*v245<4*v244)+(13*v245<8*v244)+(v245<v244)+(v245<2*v244)+(v245<6*v244)+(35*v245<4*v244)+1||v245<=10*v244;
    v257 = 32*v256+v255+(v221==d15);
  } else {
    v258 = *(uint*)(v249+v220+8);
    v250 = *(byte*)(v258+v220);
    if( *(byte*)(v258+v220)||v548<4||v548<5&&v544<4&&*((word*)v222+1)-v542[1]<36 ) {
      v259 = *(uint*)(v258+v220+8);
      if( v259 )
        v260 = 2*v250<*(byte*)(v259+v220);
      else
        v260 = 0;
      v257 = v254+8*v260+16;
    } else if( v546>=6 ) {
      if( v546>=0xE ) {
        v257 = v252+v253+14;
        if( v546<0x20 )
          v257 = v252+v253+12;
      } else {
        v257 = v252+v253+10;
      }
    } else {
      v257 = v252+v253+8;
    }
  }

  *outV246 = v246;
  *outV250 = v250;
  *outV257 = v257;
  *outV547 = v547;
  *outV549 = v549;
}

int sub_1400037A0(FILE* a1, FILE* a2) {
  PROBE_HIT(sub_1400037A0);
  int v2;
  byte* v3;
  int v4;
  int v5;
  __int64 v6;
  __int64 v7;
  byte v8;
  int v9;
  __int64 v10;
  __int64* v11;
  __int64* v12;
  byte* v13;
  int v14;
  int v15;
  __int64* v16;
  __int64* i;
  __int64 v18;
  int v19;
  int v20;
  int v21;
  int v22;
  byte* v23;
  __int64 v24;
  int v25;
  int v26;
  int v27;
  byte v28;
  int v29;
  int v30;
  byte* v31;
  byte* v32;
  int v33;
  int v34;
  int v35;
  int v36;
  int v37;
  int v38;
  uint v39;
  byte* v40;
  int v41;
  int v42;
  int v43;
  int v44;
  int v45;
  char v46;
  __int64 v47;
  int* v48;
  int v49;
  int v50;
  uint v51;
  int v52;
  int v53;
  int v54;
  uint v55;
  int v56;
  int* v57;
  int v58;
  __int64 v59;
  int v60;
  word v61;
  uint v62;
  uint v63;
  int v64;
  int v65;
  int v66;
  __int64* v67;
  byte* v68;
  uint v69;
  uint v70;
  __int64 v71;
  uint v72;
  bool v73;
  int v74;
  int v75;
  short v76;
  int v77;
  uint v78;
  __int64 v79;
  __int64 v80;
  word* v81;
  int v82;
  int v83;
  word v84;
  int v85;
  int* v86;
  int v87;
  int v88;
  int v89;
  int v90;
  int v91;
  int v92;
  int v93;
  int v94;
  int v95;
  int v96;
  uint v97;
  int* v98;
  int* v99;
  int v100;
  int* v101;
  char v102;
  bool v103;
  int v104;
  bool v105;
  uint v106;
  uint v107;
  uint v108;
  uint v109;
  uint v110;
  __int64 v111;
  uint v112;
  int v113;
  int v114;
  int v115;
  uint v116;
  char v117;
  uint v118;
  char v119;
  uint v120;
  word* v121;
  __int64 v122;
  int v123;
  int v124;
  __int64 v125;
  int v126;
  uint v127;
  int v128;
  char v129;
  word v130;
  uint v131;
  char v132;
  word v133;
  word v134;
  int* v135;
  int v136;
  int v137;
  int v138;
  int v139;
  int v140;
  int v141;
  int v142;
  int v143;
  int v144;
  int v145;
  int v146;
  __int64 v147;
  int* v148;
  int v149;
  int v150;
  int v151;
  int v152;
  int v153;
  int v154;
  int v155;
  __int64 v156;
  int v157;
  uint v158;
  int v159;
  int v160;
  __int64 v161;
  int* v162;
  int v163;
  int v164;
  int v165;
  int v166;
  int v167;
  uint v168;
  int v169;
  uint v170;
  int v171;
  uint v172;
  __int64 v173;
  int v174;
  int v175;
  uint* v176;
  uint v177;
  byte v178;
  short v179;
  short v180;
  qword v181;
  int v182;
  int v183;
  int v184;
  uint v185;
  uint* v186;
  __int64 v187;
  int v188;
  int v189;
  int v190;
  uint v191;
  __int64 v192;
  int v193;
  __int64 v194;
  __int64 v195;
  int v196;
  uint* v197;
  int v198;
  __int64 v199;
  int result;
  int v201;
  uint v202;
  int v203;
  int v204;
  int v205;
  int v206;
  int v207;
  __int64 v208;
  int v209;
  int v210;
  char v211;
  int v212;
  __int64* v213;
  __int64* v214;
  __int64 v215;
  byte* v216;
  __int64* v217;
  int v218;
  __int64 v219;
  __int64 v220;
  int v221;
  byte* v222;
  __int64* v223;
  byte* v224;
  __int64 v225;
  __int64 v226;
  __int64 v227;
  byte* v228;
  uint v229;
  byte v230;
  int v231;
  byte* v232;
  byte* v233;
  int v234;
  uint v235;
  uint v236;
  __int64 v237;
  int v238;
  byte* j;
  uint v240;
  int v241;
  short v242;
  uint v243;
  uint v244;
  uint v245;
  uint v246;
  __int64 v247;
  int v248;
  __int64 v249;
  uint v250;
  int v251;
  int v252;
  int v253;
  int v254;
  uint v255;
  uint v256;
  int v257;
  __int64 v258;
  uint v259;
  uint v260;
  __int64 v261;
  int* v262;
  __int64 v263;
  int* v264;
  int v265;
  int v266;
  word v267;
  int v268;
  int v269;
  int v270;
  int v271;
  int* v272;
  int v273;
  int v274;
  int v275;
  int v276;
  int v277;
  int v278;
  int v279;
  int v280;
  int v281;
  int v282;
  word* v283;
  uint v284;
  int v285;
  int* v286;
  uint v287;
  int v288;
  int v289;
  __int64 v290;
  word v291;
  char v292;
  __int64 v293;
  word v294;
  word* v295;
  int v296;
  int v297;
  int v298;
  uint v299;
  int v300;
  word v301;
  __int64 v302;
  word v303;
  char v304;
  __int64 v305;
  word v306;
  __int64 v307;
  word v308;
  __int64 v309;
  word v310;
  __int64 v311;
  word v312;
  __int64 v313;
  word v314;
  int* v315;
  int v316;
  int v317;
  int v318;
  int v319;
  int v320;
  int v321;
  int v322;
  int v323;
  int v324;
  int v325;
  __int64 v326;
  int* v327;
  int v328;
  int v329;
  int v330;
  int v331;
  int v332;
  int v333;
  int v334;
  __int64 v335;
  uint v336;
  uint v337;
  int v338;
  int v339;
  int v340;
  int* v341;
  int v342;
  int v343;
  int v344;
  int v345;
  int v346;
  uint v347;
  int v348;
  uint v349;
  int v350;
  uint v351;
  int v352;
  bool v353;
  uint* v354;
  int v355;
  uint* v356;
  int v357;
  uint* v358;
  int v359;
  uint* v360;
  int v361;
  byte v362;
  __int64 v363;
  uint* v364;
  int v365;
  uint* v366;
  uint v367;
  __int64 v368;
  int v369;
  bool v370;
  __int64 v371;
  int v372;
  int v373;
  __int64 v374;
  int v375;
  int v376;
  int v377;
  uint v378;
  __int64 v379;
  int v380;
  int v381;
  int v382;
  int v383;
  __int64* v384;
  __int64* k;
  __int64 v386;
  __int64 v387;
  __int64 v388;
  int v389;
  byte* v390;
  byte v391;
  int v392;
  byte* v393;
  int v394;
  int v395;
  int v396;
  int v397;
  uint v398;
  __int64 v399;
  int v400;
  int v401;
  int v402;
  uint v403;
  int v404;
  int v405;
  __int64 v406;
  __int64 v407;
  char* v408;
  uint v409;
  int v410;
  uint v411;
  uint v412;
  uint v413;
  int v414;
  int v415;
  int v416;
  int v417;
  uint v418;
  char* v419;
  int v420;
  word v421;
  uint v422;
  int v423;
  uint v424;
  __int64 v425;
  int v426;
  __int64 v427;
  int v428;
  __int64 v429;
  int v430;
  int v431;
  word* v432;
  word* v433;
  word* v434;
  word* v435;
  word* v436;
  __int64 v437;
  __int64 v438;
  char v439;
  char v440;
  int v441;
  int v442;
  int v443;
  int v444;
  int v445;
  int v446;
  int v447;
  int v448;
  int v449;
  int v450;
  char v451;
  __int64 v452;
  int v453;
  int v454;
  int v455;
  int v456;
  int v457;
  int v458;
  int* v459;
  int* v460;
  int* v461;
  int* v462;
  int v463;
  int v464;
  int v465;
  int v466;
  bool v467;
  char v468;
  char v469;
  char v470;
  uint v471;
  int* v472;
  __int64 v473;
  __int64 v474;
  byte* v475;
  int v476;
  int v477;
  int v478;
  __int64 v479;
  char v480;
  bool v481;
  int* v482;
  char v483;
  uint v484;
  int* v485;
  __int64 v486;
  __int64 v487;
  int v488;
  int v489;
  int v490;
  int v491;
  int v492;
  int v493;
  __int64* v494;
  __int64* v495;
  int v496;
  __int64 v497;
  int v498;
  int v499;
  __int64 v500;
  byte* v501;
  int v502;
  __int64 v503;
  uint v504;
  int v505;
  __int64 v506;
  uint v507;
  uint v508;
  __int64 v509;
  int v510;
  __int64 v511;
  int v512;
  byte v513;
  uint v514;
  uint v515;
  int v516;
  int v517;
  int v518;
  int v519;
  int v520;
  __int64 v521;
  uint v522;
  int v523;
  int v524;
  int v525;
  int v526;
  int v527;
  int v528;
  int v529;
  int v530;
  int v531;
  int v532;
  int v533;
  int v534;
  int v535;
  int v536;
  char v537;
  byte* v538;
  uint v539;
  int v540;
  __int64 v541;
  byte* v542;
  int v543;
  uint v544;
  uint v545;
  uint v546;
  __int64 v547;
  uint v548;
  int v549;
  byte* v550;
  byte* v551;
  byte* v552;
  __int64 v553;
  word* v554;
  word* v555;
  word* v556;
  uint v557;
  byte* v558;
  uint v559;
  __int64 v560;
  int v563;
  int v564;
  short v565;
  int v566;
  int v567;
  int v568;
  byte* v569;
  __int64 v570;
  short v571;
  int v572;
  int v573;
  uint v574;
  int v575;
  __int64 v576;
  v2 = d8;
  do {
    v3 = (byte*)q10;
    v4 = *(byte*)q10;
    v566 = getc(a2);
    if( v4 ) {
      v6 = q8;
      v7 = *((uint*)v3+1);
      v8 = v3[1];
      if( !*(byte*)(q8+v7+6LL*(v8&0xF)+1) ) {
        sub_14000C0E0(v3);
        v7 = *((uint*)v3+1);
        v8 = v3[1];
      }
      v563 = v4;
      v540 = *v3;
      v539 = v540+1;
      v9 = v540+1;
      v541 = v6;
      v550 = v3;
      q16[0] = v6+v7+6LL*(v8&0xF);
      v10 = *(byte*)q16[0];
      v11 = &q27;
      v12 = &q27;
      v567 = *(byte*)q16[0];
      d13 = v567;
      v13 = (byte*)(v6+v7-6);
      d94[v10] = v2;
      while( 1 ) {
        v13 += 6;
        v14 = *v13;
        if( (uint)v10!=v14 ) {
          d94[*v13] = v2;
          *v11++ = (__int64)v13;
          if( v14==d18 ) {
            v15 = 22;
            v16 = &q16[(b25&7)==0];
LABEL_14:
            for( i = v11-1; i>v16; --i ) {
              v18 = *(i-1);
              if( v15**(byte*)(*i+1)<=*(byte*)(v18+1) )
                break;
              *(i-1) = *i;
              *i = v18;
            }
            ++v12;
            goto LABEL_18;
          }
          if( v2==d77[v14]||v2==d73[v14] ) {
            v16 = v12;
            v15 = 11*(v2==d77[v14])+7;
            goto LABEL_14;
          }
        }
LABEL_18:
        if( !--v9 ) {
          v19 = v540+1;
          v20 = v540+1;
          v21 = v540+1;
          v22 = v540;
          v23 = v550;
          d78 = *(byte*)q16[0];
          d81[(byte)b27[d78+(d68<<8)]] = v2;
          q29 = (__int64)&d27;
          q30 = (__int64)&d27;
          q31 = (__int64)&d27;
          q32 = (__int64)&d27;
          q33 = (__int64)&d27;
          if( v540==255 ) {
            v44 = d5;
            v46 = d12;
            v45 = d10;
            v64 = *((word*)v550+1);
            q34 = (__int64)&d27;
            v568 = -1;
            d14 = -1;
            d95 = v64;
            v62 = v64+1;
            d96 = v62;
          } else {
            v24 = *((uint*)v550+2);
            v25 = *(byte*)(v24+v541);
            v26 = *((word*)v550+1);
            v27 = *(word*)(v24+v541+2);
            v28 = v550[1];
            q14 = (__int64)q16;
            v20 = v25+1;
            v505 = v27;
            v513 = v28;
            v29 = v26;
            if( (v25+1)*v26>(int)(v27*v539)||(v479 = *((uint*)v550+1), *(byte*)(v479+v541+1)>=0x6Au) ) {
              v510 = v26;
              v21 = v540+1;
              v506 = v539;
            } else if( (v513&0x40)!=0 ) {
              v510 = v26;
              v21 = v540+1;
              v506 = v539;
            } else {
              v516 = v2;
              v30 = v25+1;
              v31 = (byte*)(v541+*(uint*)(v24+v541+4)-6);
              do {
                v31 += 6;
                --v30;
                b39[*v31] = v31[1];
              } while( v30 );
              v506 = v539;
              v32 = (byte*)(v479+v541+6LL*v539);
              v33 = 9*v540+9+2*v540+2;
              v34 = v29;
              v35 = v20*(((uint)(v33-v29)>>28)|7)+v505;
              do {
                v32 -= 6;
                v37 = v32[1];
                if( v37*v35<(v33+v34)*(byte)b39[*v32]&&v32[1]<=0xE4u ) {
                  v38 = v29-v37;
                  v36 = (byte)b39[*v32];
                  v39 = ((v33+v34-v37)*(3*v37+v36)+3*(v33+v34-v37)+v35-v36-4)/(uint)(3*(v33+v34-v37)+v35-v36);
                  if( v39>v37+11 )
                    v39 = v37+11;
                  v29 = v39+v38;
                  *((word*)v550+1) = v29;
                  v32[1] = v39;
                  v34 = (word)v29;
                }
                --v19;
              } while( v19 );
              v21 = v540+1;
              v2 = v516;
              v24 = *((uint*)v550+2);
              v510 = v34;
              v22 = v540;
              v513 = v550[1];
            }
            v512 = *v550;
            v40 = (byte*)(v541+v24);
            v41 = *(byte*)(v541+*((uint*)v40+1)+6LL*(v40[1]&0xF));
            v42 = v41;
            v568 = v41;
            if( v41==v567 ) {
              v43 = *(byte*)(v541+*((uint*)v40+1)+6LL*(v40[1]&0xF));
              do {
                if( *v40!=v512 )
                  break;
                v40 = (byte*)(v541+*((uint*)v40+2));
                v42 = *(byte*)(v541+*((uint*)v40+1)+6LL*(v40[1]&0xF));
                v43 = v42;
              } while( v42==v567 );
              v568 = v43;
              d14 = v42;
            } else {
              d14 = *(byte*)(v541+*((uint*)v40+1)+6LL*(v40[1]&0xF));
            }
            v508 = v2!=d94[d15];
            v507 = v2!=d94[v41];
            v44 = d5;
            v45 = d10;
            v46 = d12;
            v511 = (byte)b40[v506];
            v47 = (int)(8*(v512+2*v25+2>d10)+(d12&3)+4*(d5<10)+((v2!=d94[v568])<<9)+(v508<<8)+(((uint)(8*v22+8+4*v22+4-v510)>>26)&0xFFFFFFE0)+(v513&0xC0)+(((uint)(3*v25+3-(4*v22+4))>>27)&0xFFFFFFF0));
            v509 = v47;
            v48 = &d28[2048*v511+2*v47];
            v49 = *v48;
            q34 = (__int64)v48;
            v515 = v49;
            d51 = v49;
            v50 = (word)w10[4096*v511+4*v47];
            v514 = v50;
            d97 = v50;
            if( v50>0x8000||v515>0x80000 ) {
              v462 = &d28[2048*v511+2*v47];
              v466 = v47;
              v493 = d10;
              v458 = d5;
              sub_140010730((__int64)(uintptr_t)(v462));
              v44 = v458;
              v45 = v493;
              LODWORD(v47) = v466;
              v48 = v462;
              LOWORD(v50) = *((word*)v462+2);
            }
            v51 = *((word*)v48+3);
            *((word*)v48+2) = v51+v50;
            if( v51>0x80 ) {
              v488 = v45;
              v485 = &d29[512*v511+16*(v507|v508)+2*((byte)v47&0xF3)];
              q29 = (__int64)v485;
              v483 = v51<0x200;
              q33 = (__int64)(v48+2048);
              v52 = *(v48-2048);
              v482 = v48-2048;
              q32 = (__int64)(v48-2048);
              v484 = v48[2048];
              v53 = ((v484+v52)>>(v483+1))+v515;
              v54 = *((word*)v48+4098);
              v515 = v53;
              v55 = (v54+(uint)*((word*)v48-4094))>>(v483+1);
              d51 = v53;
              v45 = v488;
              v514 += v55;
              d97 = v514;
              if( v54>0x8000||v484>0x80000 ) {
                v461 = v48;
                v465 = v47;
                v457 = v44;
                sub_140010730((__int64)(uintptr_t)(v48+2048));
                v44 = v457;
                v45 = v488;
                LODWORD(v47) = v465;
                v48 = v461;
                LOWORD(v54) = *((word*)v461+4098);
              }
              d98 = *((word*)v48+4099)>>v483;
              v56 = *((word*)v48-4094);
              *((word*)v48+4098) = d98+v54;
              if( v56>0x8000||(uint)*(v48-2048)>0x80000 ) {
                v460 = v48;
                v464 = v47;
                v492 = v45;
                v456 = v44;
                sub_140010730((__int64)(uintptr_t)(v482));
                v44 = v456;
                v45 = v492;
                LODWORD(v47) = v464;
                v48 = v460;
                LOWORD(v56) = *((word*)v460-4094);
              }
              d99 = *((word*)v48-4093)>>v483;
              *((word*)v48-4094) = d99+v56;
              v481 = *((word*)v48+3)<0x400u;
              v480 = v483+v481;
              v57 = &v48[-2*v509];
              v58 = LOWORD(v57[2*(int)(v47^0x100)+1]);
              v486 = (__int64)&v57[2*(int)(v47^0x100)];
              q31 = v486;
              if( v58>0x8000||*(uint*)v486>0x80000u ) {
                v459 = v57;
                v463 = v47;
                v491 = v45;
                v455 = v44;
                sub_140010730(v486);
                v44 = v455;
                v45 = v491;
                LODWORD(v47) = v463;
                v57 = v459;
                LOWORD(v58) = *(word*)(v486+4);
              }
              d100 = *(word*)(v486+6)>>v480;
              *(word*)(v486+4) = d100+v58;
              v59 = (__int64)&v57[2*(int)(v47^0x200)];
              v60 = *(word*)(v59+4);
              v487 = v59;
              q30 = v59;
              if( v60>0x8000||*(uint*)v59>0x80000u ) {
                v490 = v45;
                v454 = v44;
                sub_140010730(v59);
                v44 = v454;
                v45 = v490;
                LOWORD(v60) = *(word*)(v487+4);
              }
              d101 = *(word*)(v487+6)>>v480;
              *(word*)(v487+4) = d101+v60;
              v61 = *((word*)v485+2);
              if( v61>0x8000u||(uint)*v485>0x80000 ) {
                v489 = v45;
                v453 = v44;
                sub_140010730((__int64)(uintptr_t)(v485));
                v44 = v453;
                v45 = v489;
                v61 = *((word*)v485+2);
              }
              d102 = *((word*)v485+3)>>(v483+v481+1);
              *((word*)v485+2) = d102+v61;
            }
            d95 = *((word*)v550+1);
            v62 = d95+((v514>>1)+v21*v515)/v514+2;
            d96 = v62;
            if( v21<24 ) {
              d103 = 0;
              v63 = 0;
LABEL_58:
              d104 = 171;
              v574 = v63;
              v65 = 0;
              d105 = 0;
              v66 = 171;
              d106 = 1032*(*((uint*)v550+2)==0)+32*(v46&3)+2*(v44<3);
              v575 = d106;
              v564 = v563+1;
              v67 = q16;
              d41 += (((uint)(v20-v21-4)>>24)&0xFFFFFF80)+28672*((uint)(40-v21)>>31)+32*(*v550>v45)+28672;
              v571 = d33;
              goto LABEL_59;
            }
          }
          v63 = v62>>1;
          d103 = v62>>1;
          goto LABEL_58;
        }
      }
    }
    v220 = q8;
    v221 = v3[2];
    d13 = v221;
    __PPMContextWalk(v2, v221, &v246, &v250, &v257, &v547, &v549);
    v261 = v257+(uint)*((byte*)x9+v250);
    v262 = &d26[0x8000*(qword)(byte)b2[v246]];
    q32 = (__int64)&v262[2*(v261^0x1000)];
    q31 = (__int64)&v262[2*(v261^1)];
    q30 = (__int64)&v262[2*(v261^0x2000)];
    q29 = (__int64)&v262[2*(v261^0x20)];
    q34 = (__int64)&v262[2*(v261^0x200)];
    v263 = (__int64)&v262[2*(v261^0x400)];
    v264 = &v262[2*v261];
    v265 = *((word*)v264+2);
    q35 = v263;
    q21 = (__int64)v264;
    q22 = (__int64)(v264-4);
    q18 = (__int64)(v264+4);
    if( v265>0x8000 ) {
      sub_140010850((__int64)(uintptr_t)(v264));
      v265 = *((word*)v264+2);
    }
    v266 = *(word*)v264;
    v267 = *((word*)v264+3);
    d51 = v266;
    *((word*)v264+2) = v265+v267;
    d97 = v265;
    d52 = v266;
    d50 = v265;
    v268 = (v221&0xE0)+16*(v221==d68)+8*(v221==d66)+4*(v221==d92)+(v221==d83)+(d33&0xE200)+2*(v221==d85);
    v269 = (((word)v221-(word)d62)&0x800)+(((word)d64-(word)v221)&0x400)+((v221==d76)<<8);
    v270 = v549;
    v271 = v269+v268;
    if( v549 )
      v270 = 1;
    d33 = (v270<<12)+v271+(((v265<(uint)(56*v266))+(v265<(uint)(6*v266)))<<16);
    v272 = &d34[2*d33];
    v273 = *v272;
    q23 = (__int64)v272;
    v274 = v266*(__int64)v273/v272[1];
    v275 = d51;
    v276 = 1-d51;
    if( v274>=1-d51 ) {
      v276 = 0x40000;
      if( v274<0x40000 )
        v276 = v274;
    }
    v277 = *v272;
    v278 = v272[1];
    if( (int)abs32(*v272)>0x40000 ) {
      v277 >>= 1;
      *v272 = v277;
      v278 >>= v278>4096;
    }
    v279 = v277-v275;
    *v272 = v279;
    v280 = v275+v278+2;
    if( v280>0x40000000 ) {
      *v272 = v279>>1;
      v280 >>= 1;
    }
    v272[1] = v280;
    v281 = v276+d97;
    v282 = v276+v275;
    v283 = (word*)q21;
    v284 = *(word*)(q21+6);
    if( v284<=0x20 ) {
      v285 = d13;
      d97 = v281;
      d51 = v282;
      q35 = (__int64)&d27;
      q34 = (__int64)&d27;
      q29 = (__int64)&d27;
      q30 = (__int64)&d27;
      q31 = (__int64)&d27;
      q32 = (__int64)&d27;
      q18 = (__int64)&d27;
      q22 = (__int64)&d27;
      q20 = (__int64)&d27;
      q17 = (__int64)&d27;
      q36 = (__int64)&d110;
    } else {
      v285 = d13;
      v286 = &d30[128*(qword)(byte)b6[v246]+64*(d9>-9)+4*(uint)(d73[d13]==d8)+(((uint)v261>>6)&0xBLL)+(uint)(16*v549)];
      v287 = *v286;
      q36 = (__int64)v286;
      v288 = (v287>>(v284<0x230))+v282;
      d51 = v288;
      v289 = (0x3100u>>(v284<0x230))+v281;
      d97 = v289;
      *v286 = v287-((v287+2)>>3);
      v290 = q22;
      v103 = v284<0x220;
      v291 = *(word*)(q22+4);
      v292 = v103;
      if( v291>0x8000u ) {
        v438 = q22;
        v435 = v283;
        sub_140010850(q22);
        v283 = v435;
        v290 = v438;
        v291 = *(word*)(v438+4);
      }
      d48 = *(word*)(v290+6)>>v292;
      *(word*)(v290+4) = d48+v291;
      v293 = q18;
      v294 = *(word*)(q18+4);
      if( v294>0x8000u ) {
        v437 = q18;
        v434 = v283;
        sub_140010850(q18);
        v283 = v434;
        v293 = v437;
        v294 = *(word*)(v437+4);
      }
      d49 = *(word*)(v293+6)>>v292;
      v295 = v283-0x10000;
      q20 = (__int64)(v283-0x10000);
      *(word*)(v293+4) = d49+v294;
      v296 = *(v283-0x10000);
      v297 = v283[0x10000];
      v298 = *(v283-65534);
      q17 = (__int64)(v283+0x10000);
      v299 = v297+v296;
      v300 = v283[65538];
      v282 = (v299>>(v292+1))+v288;
      d51 = v282;
      v281 = ((uint)(v300+v298)>>(v292+1))+v289;
      d97 = v281;
      if( v300>0x8000 ) {
        v436 = v283-0x10000;
        v433 = v283;
        sub_140010850((__int64)(uintptr_t)(v283+0x10000));
        v283 = v433;
        v295 = v436;
        LOWORD(v300) = v433[65538];
      }
      d46 = v283[65539]>>v292;
      v283[65538] = d46+v300;
      v301 = *(v283-65534);
      if( v301>0x8000u ) {
        v432 = v283;
        sub_140010850((__int64)(uintptr_t)(v295));
        v283 = v432;
        v301 = *(v432-65534);
      }
      v302 = q32;
      d47 = *(v283-65533)>>v292;
      *(v283-65534) = d47+v301;
      v303 = *(word*)(v302+4);
      v304 = (v283[3]<0x398u)+v292;
      if( v303>0x8000u ) {
        sub_140010850(v302);
        v303 = *(word*)(v302+4);
      }
      d99 = *(word*)(v302+6)>>v304;
      *(word*)(v302+4) = d99+v303;
      v305 = q31;
      v306 = *(word*)(q31+4);
      if( v306>0x8000u ) {
        sub_140010850(q31);
        v306 = *(word*)(v305+4);
      }
      d100 = *(word*)(v305+6)>>v304;
      *(word*)(v305+4) = d100+v306;
      v307 = q30;
      v308 = *(word*)(q30+4);
      if( v308>0x8000u ) {
        sub_140010850(q30);
        v308 = *(word*)(v307+4);
      }
      d101 = *(word*)(v307+6)>>v304;
      *(word*)(v307+4) = d101+v308;
      v309 = q29;
      v310 = *(word*)(q29+4);
      if( v310>0x8000u ) {
        sub_140010850(q29);
        v310 = *(word*)(v309+4);
      }
      d102 = *(word*)(v309+6)>>v304;
      *(word*)(v309+4) = d102+v310;
      v311 = q34;
      v312 = *(word*)(q34+4);
      if( v312>0x8000u ) {
        sub_140010850(q34);
        v312 = *(word*)(v311+4);
      }
      d105 = *(word*)(v311+6)>>v304;
      *(word*)(v311+4) = d105+v312;
      v313 = q35;
      v314 = *(word*)(q35+4);
      if( v314>0x8000u ) {
        sub_140010850(q35);
        v314 = *(word*)(v313+4);
      }
      d104 = *(word*)(v313+6)>>v304;
      *(word*)(v313+4) = d104+v314;
    }
    d51 = 60416LL*v282/v281;
    d97 = 60416;
    d54 = d51;
    d53 = 60416;
    v315 = &d36[0x100000*(d17<(uint)d80)+0x80000*(v285==d18)+0x20000*d11+512*d64+2*v285+2*(qword)((byte)((uint)d108<0x78)<<17)];
    v316 = *v315;
    q19 = (__int64)v315;
    v317 = d51*(__int64)v316/v315[1];
    v318 = d51;
    v319 = 1-d51;
    if( v317>=1-d51 ) {
      v319 = 0x40000;
      if( v317<0x40000 )
        v319 = (int)(60416LL*v282/v281)*(__int64)v316/v315[1];
    }
    v320 = *v315;
    if( (int)abs32(*v315)>0x80000 ) {
      v321 = v315[1];
      v320 >>= 1;
      *v315 = v320;
      v315[1] = v321>>(v321>0x2000);
    }
    v322 = v315[1];
    v323 = v320-v318;
    *v315 = v323;
    v324 = v318+v322+1120;
    v315[1] = v324;
    if( v324>0x40000000 ) {
      *v315 = v323>>1;
      v315[1] = v324>>1;
    }
    d97 += v319;
    d56 = d97;
    v325 = v319+v318;
    d51 = v325;
    v326 = ((17*v325<(uint)d97)<<13)+(((word)d109-(word)v285)&0x1000)+16*(v285==d15)+8*(v285==d18)+4*(v285==d87)+(v285==d84)+(((uint)d107<0x29)<<7)+d38+(d33&0x800)+2*(*((byte*)x4+v285)+(v285==d86))+32*(((uint)d108<0xA800)+((uint)d108<0x600)+((uint)d108<0xD));
    v327 = &d39[2*v326];
    v328 = *v327;
    q24 = (__int64)v327;
    v329 = (d97-v325)*(__int64)v328/d40[2*v326];
    v330 = v325;
    v331 = v325-d97+1;
    if( v329>=v331 ) {
      v331 = 0x40000;
      if( v329<0x40000 )
        v331 = (d97-v325)*(__int64)v328/d40[2*v326];
    }
    v332 = *v327;
    v333 = v331+d97;
    d97 += v331;
    v334 = d40[2*v326];
    if( (int)abs32(v332)>0x100000 ) {
      v332 >>= 1;
      *v327 = v332;
      v334 >>= v334>0x2000;
    }
    *v327 = v332+2*v325;
    v335 = q12;
    d40[2*v326] = v334+1;
    v336 = q8+*(uint*)(v547+2)-q4;
    v337 = d73[v285]==d8;
    v338 = *(byte*)(v335+(((word)v285-(word)d78)&0x1FF));
    v339 = (*(uint*)(v335+512)<(uint)(384*v338))+(*(uint*)(v335+512)<(uint)(58*v338))+(*(uint*)(v335+512)<(uint)(22*v338));
    v340 = (byte)b14[d63];
    d55 = v333;
    v341 = &d42[2*(int)(4*(v336>=0xFFFFFD20)+v337+(v326&0x700)+d41+(((uint)d108<0x2C00)<<11)+2*(v285==v340)+8*v339)];
    v342 = *v341;
    q25 = (__int64)v341;
    v343 = (v333-v330)*(__int64)v342/v341[1];
    v344 = d51;
    v345 = d51-d97+1;
    if( v343>=v345 ) {
      v345 = 0x80000;
      if( v343<0x80000 )
        v345 = v343;
    }
    v346 = *v341;
    v347 = v345+d97;
    d97 += v345;
    v348 = v341[1];
    if( (int)abs32(v346)>0x100000 ) {
      v346 >>= 1;
      *v341 = v346;
      v348 >>= v348>0x2000;
    }
    *v341 = v346+v330;
    v341[1] = v348+2;
    v350 = v3[2];
    v351 = rc.GetSubRange(v344, v347);
    if( v350!=v566 ) {
      v427 = (uint)d58;
      v428 = ~d59;
      rc.EncodeNotMatched(v351);
      v429 = v3[2];
      d94[v429] = d8;
      v430 = v347+v347*v3[3];
      d57[v427] &= v428;
      q9 = 0;
      d60[d61] &= ~d59;
      v195 = q8;
      v431 = v3[3];
      v23 = (byte*)q10;
      v201 = *(byte*)q10;
      d11 = 0;
      d111 = v201;
      result = (int)(16*(v431*v344+v344-v431*v347))/v430;
      d45 = result;
      do {
LABEL_128:
        v202 = *((uint*)v23+2);
        if( !v202 )
          return result;
        v203 = d5;
        v204 = --d5;
        v23 = (byte*)(v195+v202);
        v205 = *v23;
        q10 = (__int64)v23;
        if( v205&&!*(byte*)(v195+*((uint*)v23+1)+6LL*(v23[1]&0xF)+1) )
          result = sub_14000C0E0(v23);
        v206 = v205-v201;
      } while( !v206 );
      v207 = *v23;
      v208 = *((uint*)v23+1);
      v533 = v204;
      v534 = v203;
      v209 = v207-v201;
      v564 = v206;
      v532 = v207;
      v531 = v207+1;
      v210 = v209;
      v535 = v531;
      v65 = 0x8000;
      v211 = 0;
      v212 = 0;
      v213 = q16;
      v214 = q16;
      v215 = *(byte*)(v195+v208+6LL*(v23[1]&0xF));
      v538 = (byte*)(v195+v208+6LL*(v23[1]&0xF));
      v2 = d8;
      v567 = *v538;
      d13 = v567;
      if( d8==d94[v215] ) {
        d105 = 0x8000;
      } else {
        d94[v215] = d8;
        v211 = 1;
        v213 = &q27;
        q16[0] = (__int64)v538;
        v212 = v538[1];
        v214 = &q27;
        v65 = 0;
        v210 = v209-1;
        d105 = 0;
      }
      v216 = (byte*)(v195+v208-6);
      if( v210 ) {
        v520 = v209;
        v217 = v213;
        v521 = v195;
        v523 = v65;
        v552 = v23;
        while( 1 ) {
          do {
            v216 += 6;
            v218 = *v216;
            v219 = *v216;
          } while( v2==d94[v219] );
          v382 = v216[1];
          d94[v219] = v2;
          *v213++ = (__int64)v216;
          v214 = v213;
          v212 += v382;
          if( v218==d18 )
            break;
          if( v2==d77[v219]||v2==d73[v219] ) {
            v384 = v217;
            v383 = 4*(v2==d77[v219])+12;
            goto LABEL_292;
          }
LABEL_296:
          if( !--v210 ) {
            v209 = v520;
            v195 = v521;
            v65 = v523;
            v23 = v552;
            goto LABEL_298;
          }
        }
        v383 = 22;
        v384 = &q16[(byte)v211&((b25&7)==0)];
LABEL_292:
        for( k = v213-1; k>v384; --k ) {
          v386 = *(k-1);
          if( v383**(byte*)(*k+1)<=*(byte*)(v386+1) )
            break;
          *(k-1) = *k;
          *k = v386;
        }
        ++v217;
        goto LABEL_296;
      }
LABEL_298:
      q29 = (__int64)&d27;
      q30 = (__int64)&d27;
      q31 = (__int64)&d27;
      q32 = (__int64)&d27;
      q33 = (__int64)&d27;
      if( v531==256 ) {
        v568 = -1;
        d14 = -1;
        q34 = (__int64)&d27;
        v422 = 1;
        v537 = d12;
        v536 = d10;
      } else {
        v387 = *((uint*)v23+2);
        v501 = (byte*)(v195+v387);
        v503 = *(uint*)(v195+v387+4);
        v568 = *(byte*)(v195+v503+6LL*(*(byte*)(v195+v387+1)&0xF));
        if( v568==v567 ) {
          v388 = *(uint*)(v195+v387+4);
          v389 = *(byte*)(v195+v388+6LL*(*(byte*)(v195+v387+1)&0xF));
          v518 = v2;
          v390 = (byte*)(v195+v387);
          do {
            if( *v390!=*v23 )
              break;
            v390 = (byte*)(v195+*((uint*)v390+2));
            v388 = *((uint*)v390+1);
            v389 = *(byte*)(v195+v388+6LL*(v390[1]&0xF));
          } while( v389==v567 );
          v503 = v388;
          v501 = v390;
          v2 = v518;
          v568 = v389;
          d14 = v389;
        } else {
          d14 = *(byte*)(v195+*(uint*)(v195+v387+4)+6LL*(*(byte*)(v195+v387+1)&0xF));
        }
        v391 = v23[1];
        if( v212>=16*v209||(v23[1]&0x40)!=0 ) {
          q14 = (__int64)v214;
          v502 = *((word*)v23+1);
        } else {
          v392 = *v501+1;
          v393 = (byte*)(v195+v503);
          v394 = v392;
          do {
            b39[*v393] = v393[1];
            v393 += 6;
            --v394;
          } while( v394 );
          v395 = *((word*)v23+1);
          v522 = v170;
          v212 = 0;
          v524 = v65;
          v519 = v2;
          v396 = v395;
          v397 = 5*v532+5;
          v398 = (5*((uint)(2*v397-v395)>>31)+8)*v392+*((word*)v501+1);
          do {
            v399 = *--v213;
            v401 = *(byte*)(*v213+1);
            v402 = v396+2*v397;
            LOBYTE(v403) = *(byte*)(*v213+1);
            if( (int)(v401*v398)<v402*(byte)b39[*(byte*)*v213]&&*(byte*)(*v213+1)<=0xE4u ) {
              v404 = v395-v401;
              v400 = (byte)b39[*(byte*)*v213];
              v403 = ((v402-v401)*(3*v401+v400)+3*(v402-v401)+v398-v400-4)/(3*(v402-v401)+v398-v400);
              if( v403>v401+11 )
                v403 = v401+11;
              v395 = v403+v404;
              *((word*)v23+1) = v395;
              *(byte*)(v399+1) = v403;
              v396 = (word)v395;
            }
            v212 += (byte)v403;
          } while( v213!=q16 );
          v170 = v522;
          v65 = v524;
          v391 = v23[1];
          v387 = *((uint*)v23+2);
          v502 = v396;
          v2 = v519;
          q14 = (__int64)v213;
        }
        v405 = *(byte*)(v387+v195);
        v535 = v405+1;
        v499 = v2!=d94[d15];
        v496 = v2!=d94[v568];
        v537 = d12;
        v498 = 2*v405+2;
        v500 = (byte)b40[v531];
        v406 = (int)((((16*(uint)*v23-v502)>>26)&0xFFFFFFE0)+(d12&3)+8*(v499|(v2!=d94[*(byte*)(v195+*(uint*)(v387+v195+4)+6LL*(*(byte*)(v387+v195+1)&0xF))]))+4*(v531*v212>v502*v209)+(v391&0xC0)+16*(v209>=2*v405-2*v532));
        v497 = v406;
        v407 = (v500<<11)+8LL*(int)v406;
        v408 = (char*)d29+v407;
        v409 = *(int*)((char*)d29+v407);
        v410 = *(word*)((char*)&w12+v407);
        q34 = (__int64)d29+v407;
        d51 = v409;
        v504 = v410;
        d97 = v410;
        if( v410>0x8000||v409>0x80000 ) {
          v446 = v406;
          v530 = v65;
          sub_140010730((__int64)(uintptr_t)((char*)d29+v407));
          v65 = v530;
          LODWORD(v406) = v446;
          LOWORD(v410) = *((word*)v408+2);
        }
        v411 = *((word*)v408+3);
        *((word*)v408+2) = v411+v410;
        v536 = d10;
        if( v411>0x78 ) {
          v472 = &d28[2048*v500+16*(uint)(*v23+v498>d10)+2*((byte)v406&0xF3)+2*(uint)(v496<<9)+2*(uint)(v499<<8)+2*(qword)(4*(uint)(v533<10))];
          q29 = (__int64)v472;
          v467 = v411<0x100;
          v470 = (v411<0x100)+1;
          v412 = *((uint*)v408+512);
          q33 = (__int64)(v408+2048);
          v413 = (v412+*((uint*)v408-512))>>v470;
          q32 = (__int64)(v408-2048);
          v471 = v412;
          v414 = *((word*)v408-1022);
          v409 += v413;
          v415 = *((word*)v408+1026);
          d51 = v409;
          v504 += (uint)(v415+v414)>>v470;
          d97 = v504;
          if( v415>0x8000||v471>0x80000 ) {
            v445 = v406;
            v529 = v65;
            sub_140010730((__int64)(uintptr_t)(v408+2048));
            v65 = v529;
            LODWORD(v406) = v445;
            v415 = *((word*)v408+1026);
          }
          v416 = *((word*)v408-1022);
          v417 = (*((word*)v408+1027)>>v470)+v415;
          d98 = *((word*)v408+1027)>>v470;
          *((word*)v408+1026) = v417;
          if( v416>0x8000||*((uint*)v408-512)>0x80000u ) {
            v444 = v406;
            v528 = v65;
            sub_140010730((__int64)(uintptr_t)(v408-2048));
            v65 = v528;
            LODWORD(v406) = v444;
            LOWORD(v416) = *((word*)v408-1022);
          }
          d99 = *((word*)v408-1021)>>v470;
          *((word*)v408-1022) = d99+v416;
          v469 = *((word*)v408+3)<0x200u;
          v418 = *((word*)v472+2);
          if( v418>0x8000||(uint)*v472>0x80000 ) {
            v443 = v406;
            v527 = v65;
            sub_140010730((__int64)(uintptr_t)(v472));
            v65 = v527;
            LODWORD(v406) = v443;
            LOWORD(v418) = *((word*)v472+2);
          }
          d102 = *((word*)v472+3)>>v469;
          *((word*)v472+2) = d102+v418;
          v468 = (*((word*)v408+3)<0x400u)+v467+1;
          v419 = &v408[-8*v497];
          v420 = *(word*)&v419[8*(int)(v406^2)+4];
          v473 = (__int64)&v419[8*(int)(v406^2)];
          q31 = v473;
          if( v420>0x8000||*(uint*)v473>0x80000u ) {
            v442 = v406;
            v526 = v65;
            sub_140010730(v473);
            v65 = v526;
            LODWORD(v406) = v442;
            LOWORD(v420) = *(word*)(v473+4);
          }
          d100 = *(word*)(v473+6)>>v468;
          *(word*)(v473+4) = d100+v420;
          v474 = (__int64)&v419[8*(int)(v406^8)];
          q30 = v474;
          v421 = *(word*)(v474+4);
          if( v421>0x8000u||*(uint*)v474>0x80000u ) {
            v525 = v65;
            sub_140010730(v474);
            v65 = v525;
            v421 = *(word*)(v474+4);
          }
          d101 = *(word*)(v474+6)>>v468;
          *(word*)(v474+4) = d101+v421;
        }
        v422 = ((v504>>1)+v531*v409)/v504+2;
        if( v531<24 ) {
          v574 = 0;
          goto LABEL_335;
        }
      }
      v574 = (int)(v422+*((word*)v23+1))>>1;
LABEL_335:
      d103 = v574;
      v62 = v212+v422;
      d96 = v62;
      d9 = d16;
      d95 = v212+v532+1;
      v66 = 0;
      v67 = q16;
      d104 = 0;
      q14 = (__int64)q16;
      v423 = (v533>1)<<15;
      v571 = v423+(d33&0x7FFF);
      d33 = v423+(d33&0xFFFF7FFF);
      d38 = ((v533>3)<<15)+v423+(d38&0x4600);
      v424 = v533 < 3||d7 > v534+22;
      v575 = 1032*(*((uint*)v23+2)==0)+32*(v537&3)+2*v424;
      d106 = v575;
      d41 = 32*(*v23+28>2*v536)+(((uint)(v535-v531-3)>>24)&0xFFFFFF80)+((v533>2)<<13)+(((d7>7)+(d7>2)+7*(((uint)(74-v531)>>31)+((uint)(2-v531)>>31))+(v533>16))<<12);
      while( 1 ) {
        rc.EncodeNormalize(a1);
LABEL_59:
        v68 = (byte*)*v67;
        v569 = v23;
        q14 = (__int64)(v67+1);
        v69 = (v68[1]<<8)-v66;
        q18 = (__int64)&d27;
        q22 = (__int64)&d27;
        q20 = (__int64)&d27;
        q17 = (__int64)&d27;
        v573 = v69/v62;
        v570 = v69/v62;
        v576 = (uint)(byte)b1[v570]+1;
        v70 = *v68;
        d59 = 1<<*v68;
        d58 = ((v70+d69)>>5)+0x2000;
        d61 = v65+((v70+d70)>>5);
        v71 = v70;
        v72 = v2==d73[v70];
        v73 = v2==d77[v70];
        v572 = ((d59&d57[((v70+d69)>>5)+0x2000])!=0)+2*((d59&d60[v65+((v70+d70)>>5)])!=0);
        q37 = (__int64)&q38;
        q39 = (__int64)&q38;
        v74 = v2==d81[v70];
        v75 = v70==d72||*(uint*)(q12+512)<*(byte*)(q12+(((word)v70-(word)d78)&0x1FF))<<7;
        v76 = d62;
        v77 = d24[256*d62+v70];
        if( (uint)(d25-v77)>=0x20000 ) {
          v78 = 0x20000;
          d107 = 0x20000;
          d108 = 0x20000;
          d109 = 0x20000;
        } else {
          d108 = d24[256*d62+v70];
          d109 = (byte)b26[(v77+2)&0x1FFFF];
          d108 = d25-v77;
          v78 = d25-d24[256*v70+d109];
          d107 = v78;
        }
        v79 = v575|(((v78<0x220)<<12)+(v75<<9)+((v62<v574)<<11)+8*(v70==v568)+((v70==v567)<<10)+*((byte*)x4+v70)+(v74<<8)+16*(v70==d18)+4*(uint)v73+v72);
        v80 = (v576<<16)+8*v79;
        v81 = (word*)((char*)&d27+v80);
        v82 = *(word*)((char*)&w4+v80);
        q21 = (__int64)&d27+v80;
        if( v82>0x8000 ) {
          sub_140010850((__int64)(uintptr_t)((char*)&d27+v80));
          v76 = d62;
          v82 = v81[2];
        }
        v83 = *v81;
        v84 = v81[3];
        d51 = v83;
        v81[2] = v82+v84;
        d97 = v82;
        d52 = v83;
        d50 = v82;
        v85 = v572;
        if( v572 )
          v85 = 1;
        d33 = (((word)v70-v76)&0x800)+(v85<<12)+(v571&0xE200)+(((word)d64-(word)v70)&0x400)+((v70==d76)<<8)+(v70&0xE0)+16*(v70==d68)+8*(v70==d66)+4*(v70==d92)+(v70==d83)+2*(v70==d85)+(((v82<(uint)(56*v83))+(v82<(uint)(6*v83)))<<16);
        v86 = &d34[2*d33];
        v87 = *v86;
        q23 = (__int64)v86;
        v88 = v83*(__int64)v87/v86[1];
        v89 = d51;
        v90 = 1-d51;
        if( v88>=1-d51 ) {
          v90 = 0x40000;
          if( v88<0x40000 )
            v90 = v88;
        }
        v91 = *v86;
        v92 = v86[1];
        if( (int)abs32(*v86)>0x40000 ) {
          v91 >>= 1;
          *v86 = v91;
          v92 >>= v92>4096;
        }
        v93 = v91-v89;
        *v86 = v93;
        v94 = v89+v92+2;
        if( v94>0x40000000 ) {
          *v86 = v93>>1;
          v94 >>= 1;
        }
        v86[1] = v94;
        v95 = v90+d97;
        v96 = v90+v89;
        v97 = *(word*)(q21+6);
        if( v97<=8 ) {
          d97 = v95;
          d51 = v96;
        } else {
          v553 = q21;
          v557 = *(word*)(q21+6);
          v551 = v23;
          v558 = v68;
          v559 = v70;
          v560 = v70;
          v98 = &d31[512*(qword)(byte)b8[v570]+2*(uint)(v572<<6)];
          v99 = &v98[2*(v79&0x1F)];
          v100 = v99[1];
          q39 = (__int64)v99;
          v101 = &v98[2*(((uint)v79>>4)&0x1F)+64];
          q37 = (__int64)v101;
          v102 = v97<0x150;
          v103 = v97<0x48;
          v104 = v101[1];
          v105 = v103;
          v106 = v100*(v573+2);
          LODWORD(v98) = v104*(v573+2);
          v107 = v104+v100;
          v108 = *v101;
          v109 = v106>>8;
          v110 = (uint)(uintptr_t)v98>>8;
          v111 = (v107>>11)&0xFFFFFFFE;
          v103 = v107<0xC000;
          v112 = *v99;
          if( !v103 )
            v111 = 24;
          v113 = v109+v110;
          v114 = (byte)b41[v111];
          v115 = (byte)b42[v111];
          if( v109+v110>=0x6000 )
            v113 = 24576;
          v116 = ((uint)(v114*v113+v115*(*v101+*v99))>>(v102+7))+v96;
          v117 = v102+v105+3;
          v118 = ((uint)(192*(v114+v115))>>v102)+v95;
          d112 = 0x3000u>>v117;
          v119 = v102+4;
          d113 = 0x3000u>>v119;
          v70 = v559;
          *v99 = v112-(v112>>v117);
          v23 = v551;
          v99[1] -= (v109+7)>>v119;
          *v101 = v108-(v108>>v117);
          v120 = (v110+7)>>v119;
          v121 = (word*)v553;
          v122 = v553-0x10000;
          v101[1] -= v120;
          v68 = v558;
          v123 = *(word*)(v553-0x10000);
          v124 = *(word*)(v553-0x10000+4);
          v125 = v553+0x10000;
          v126 = *(word*)(v553+0x10000);
          q17 = v553+0x10000;
          v127 = v123+v126;
          v128 = *(word*)(v553+65540);
          q20 = v553-0x10000;
          v96 = (v127>>3)+v116;
          d51 = v96;
          v95 = ((uint)(v128+v124)>>3)+v118;
          v71 = v560;
          v129 = (v557<0x30)+1;
          d97 = v95;
          if( v128>0x8000 ) {
            v450 = v95;
            sub_140010850(v553+0x10000);
            v95 = v450;
            v121 = (word*)v553;
            v122 = v553-0x10000;
            v129 = (v557<0x30)+1;
            v125 = v553+0x10000;
            LOWORD(v128) = *(word*)(v553+65540);
          }
          d46 = *(word*)(v125+6)>>v129;
          *(word*)(v125+4) = d46+v128;
          v130 = *(word*)(v122+4);
          if( v130>0x8000u ) {
            v451 = v129;
            v452 = v122;
            v556 = v121;
            v449 = v95;
            sub_140010850(v122);
            v95 = v449;
            v121 = v556;
            v122 = v452;
            v129 = v451;
            v130 = *(word*)(v452+4);
          }
          d47 = *(word*)(v122+6)>>v129;
          *(word*)(v122+4) = d47+v130;
          v131 = v121[3];
          if( v131>0x48 ) {
            q22 = (__int64)(v121+0x10000);
            v132 = (v131<0x1E0)+(v131<0x3D0)+1;
            v133 = v121[65538];
            if( v133>0x8000u ) {
              v440 = v132;
              v555 = v121;
              v448 = v95;
              sub_140010850((__int64)(uintptr_t)(v121+0x10000));
              v95 = v448;
              v121 = v555;
              v132 = v440;
              v133 = v555[65538];
            }
            d48 = v121[65539]>>v132;
            q18 = (__int64)(v121-0x10000);
            v121[65538] = d48+v133;
            v134 = *(v121-65534);
            if( v134>0x8000u ) {
              v439 = v132;
              v554 = v121;
              v447 = v95;
              sub_140010850((__int64)(uintptr_t)(v121-0x10000));
              v95 = v447;
              v121 = v554;
              v132 = v439;
              v134 = *(v554-65534);
            }
            d49 = *(v121-65533)>>v132;
            *(v121-65534) = d49+v134;
          }
        }
        d51 = 60416LL*v96/v95;
        d97 = 60416;
        d54 = d51;
        d53 = 60416;
        v135 = &d36[0x100000*(d17<(uint)d80)+0x80000*(v70==d18)+0x20000*d11+512*d64+2*v71+2*(qword)((byte)((uint)d108<0x78)<<17)];
        v136 = *v135;
        q19 = (__int64)v135;
        v137 = d51*(__int64)v136/v135[1];
        v138 = d51;
        v139 = 1-d51;
        if( v137>=1-d51 ) {
          v139 = 0x40000;
          if( v137<0x40000 )
            v139 = d51*(__int64)v136/v135[1];
        }
        v140 = *v135;
        if( (int)abs32(*v135)>0x80000 ) {
          v141 = v135[1];
          v140 >>= 1;
          *v135 = v140;
          v135[1] = v141>>(v141>0x2000);
        }
        v142 = v135[1];
        v143 = v140-v138;
        *v135 = v143;
        v144 = v138+v142+1120;
        v135[1] = v144;
        if( v144>0x40000000 ) {
          *v135 = v143>>1;
          v135[1] = v144>>1;
        }
        v145 = *((byte*)x4+v71);
        d97 += v139;
        v146 = v139+v138;
        d51 = v146;
        d56 = d97;
        v147 = ((17*v146<(uint)d97)<<13)+(((word)d109-(word)v70)&0x1000)+16*(v70==d15)+8*(v70==d18)+4*(v70==d87)+(v70==d84)+(((uint)d107<0x29)<<7)+d38+(d33&0x800)+2*(v145+(v70==d86))+32*(((uint)d108<0xA800)+((uint)d108<0x600)+((uint)d108<0xD));
        v148 = &d39[2*v147];
        v149 = *v148;
        q24 = (__int64)v148;
        v150 = (d97-v146)*(__int64)v149/d40[2*v147];
        v151 = v146;
        v152 = v146-d97+1;
        if( v150>=v152 ) {
          v152 = 0x40000;
          if( v150<0x40000 )
            v152 = (d97-v146)*(__int64)v149/d40[2*v147];
        }
        v153 = *v148;
        v154 = v152+d97;
        d97 += v152;
        v155 = d40[2*v147];
        if( (int)abs32(v153)>0x100000 ) {
          v153 >>= 1;
          *v148 = v153;
          v155 >>= v155>0x2000;
        }
        v73 = d73[v71]==d8;
        v156 = q12;
        *v148 = v153+2*v146;
        d40[2*v147] = v155+1;
        v157 = *(byte*)(v156+(((word)v70-(word)d78)&0x1FF));
        v158 = q8+*(uint*)(v68+2)-q4+736;
        v159 = (*(uint*)(v156+512)<(uint)(384*v157))+(*(uint*)(v156+512)<(uint)(58*v157))+(*(uint*)(v156+512)<(uint)(22*v157));
        v160 = (byte)b14[d63];
        d55 = v154;
        v161 = (int)(4*(v158<0x2E0)+v73+(v147&0x700)+d41+(((uint)d108<0x2C00)<<11)+2*(v70==v160)+8*v159);
        v162 = &d42[2*v161];
        v163 = *v162;
        q25 = (__int64)v162;
        v164 = (v154-v151)*(__int64)v163/d43[2*v161];
        v165 = d51;
        v166 = d51-d97+1;
        if( v164>=v166 ) {
          v166 = 0x80000;
          if( v164<0x80000 )
            v166 = v164;
        }
        v167 = *v162;
        v168 = v166+d97;
        d97 += v166;
        v169 = v162[1];
        if( (int)abs32(v167)>0x100000 ) {
          v167 >>= 1;
          *v162 = v167;
          v169 >>= v169>0x2000;
        }
        *v162 = v167+v151;
        v162[1] = v169+2;
        v171 = *v68;
        v172 = rc.GetSubRange(v165, v168);
        if( v171==v566 ) {
          rc.EncodeMatched(v172);
          v173 = q39;
          v174 = d112;
          v175 = d113;
          v176 = (uint*)q37;
          v177 = d108;
          *(uint*)q39 += d112;
          *(uint*)(v173+4) += v175;
          v178 = v23[1];
          *v176 += v174;
          v176[1] += v175;
          v179 = (v178&0x40)==0||v177>0xE00;
          v180 = (v177>0x4800)+(v177>0x380)+(v177>0x80)+v179+4;
          v23[1] = v178&0xF0;
          *((word*)v23+1) += v180;
          v68[1] += v180;
          v181 = q8+*((uint*)v23+1);
          if( v68==(byte*)v181 ) {
            v23 = (byte*)q10;
            if( q10==q11 )
              d11 = v168<2*v165;
          } else {
            v565 = *(word*)v68;
            v182 = *(uint*)(v68+2);
            do {
              v183 = *((uint*)v68-1);
              *(word*)v68 = *((word*)v68-3);
              *(uint*)(v68+2) = v183;
              v68 -= 6;
            } while( (qword)v68>v181 );
            *(word*)v68 = v565;
            v23 = (byte*)q10;
            *(uint*)(v68+2) = v182;
          }
          v184 = v68[1];
          q9 = (__int64)v68;
          if( v184>244 ) {
            sub_14000B750(v569);
            v68 = (byte*)q9;
          }
          if( v68 )
            goto LABEL_250;
        } else {
          v425 = q9;
          rc.EncodeNotMatched(v172);
          v426 = v68[1];
          d96 -= v426;
          d57[d58] &= ~d59;
          d60[d61] &= ~d59;
          if( v425 ) {
            v23 = (byte*)q10;
            q9 = 0;
            d11 = 0;
          } else if( (qword)v68<=q8+(qword)*((uint*)v23+1) ) {
            v23 = (byte*)q10;
          } else {
            v23 = (byte*)q10;
            d105 = 0x8000;
          }
        }
        if( !--v564 ) {
          v185 = d95;
          v186 = (uint*)q34;
          v187 = q33;
          v188 = d98;
          v189 = *v23;
          v190 = *(word*)(q34+6);
          *(word*)(q34+4) -= v190;
          v191 = v185/(v189+1);
          v192 = q32;
          v193 = d99;
          *v186 += v191*v190;
          v194 = q31;
          *(word*)(v187+4) -= v188;
          *(uint*)v187 += v188*v191;
          *(word*)(v192+4) -= v193;
          *(uint*)v192 += v193*v191;
          LODWORD(v192) = d100;
          v195 = q8;
          *(word*)(v194+4) -= d100;
          v196 = v192*v191;
          LODWORD(v192) = d101;
          *(uint*)v194 += v196;
          v197 = (uint*)q30;
          v198 = v192*v191;
          *(word*)(q30+4) -= v192;
          v199 = q29;
          *v197 += v198;
          result = d102*v191;
          *(word*)(v199+4) -= d102;
          *(uint*)v199 += result;
          v201 = *v23;
          d111 = v201;
          goto LABEL_128;
        }
        v65 = d105;
        v66 = d104;
        v567 = d13;
        v2 = d8;
        v568 = d14;
        v67 = (__int64*)q14;
        v571 = d33;
        v62 = d96;
        v574 = d103;
        v575 = d106;
      }
    } else {
      rc.EncodeMatched(v351);
    }
    q9 = (__int64)(v3+2);
    v352 = v3[3];
    v353 = v352==1&&v347<4*v344;
    v354 = (uint*)q31;
    v355 = d100;
    v356 = (uint*)q30;
    v357 = d101;
    v358 = (uint*)q29;
    v359 = d102;
    v360 = (uint*)q34;
    v361 = d105;
    d11 = 1;
    v362 = (v352-127<0)+v352+v353;
    v363 = q36;
    v3[3] = v362;
    v364 = (uint*)q32;
    v365 = d99;
    *(uint*)v363 += 1568;
    LODWORD(v363) = d104;
    *v364 += v365;
    v366 = (uint*)q35;
    *v354 += v355;
    *v356 += v357;
    *v358 += v359;
    *v360 += v361;
    *v366 += v363;
    v23 = (byte*)q10;
LABEL_250:
    result = sub_140007B80(v23);
    rc.EncodeNormalize(a1);
    v2 = d8;
  } while( d8 );
  return result;
}

PROBE_DECL(sub_14000C560)
int sub_14000C560(FILE* a1, FILE* a2) {
  PROBE_HIT(sub_14000C560);
  int v2;
  byte* v3;
  int v4;
  int v5;
  __int64 v6;
  __int64 v7;
  byte v8;
  int v9;
  __int64* v10;
  __int64 v11;
  __int64* v12;
  byte* v13;
  int v14;
  int v15;
  __int64* v16;
  __int64* i;
  __int64 v18;
  int v19;
  int v20;
  int v21;
  int v22;
  int v23;
  __int64 v24;
  int v25;
  int v26;
  int v27;
  byte v28;
  int v29;
  int v30;
  byte* v31;
  byte* v32;
  int v33;
  int v34;
  int v35;
  int v36;
  int v37;
  int v38;
  uint v39;
  byte* v40;
  int v41;
  int v42;
  int v43;
  int v44;
  int v45;
  char v46;
  __int64 v47;
  int* v48;
  int v49;
  int v50;
  uint v51;
  int v52;
  uint v53;
  int v54;
  int v55;
  int v56;
  int v57;
  int v58;
  int* v59;
  int v60;
  __int64 v61;
  int v62;
  word v63;
  uint v64;
  uint v65;
  int v66;
  int v67;
  int v68;
  __int64* v69;
  uint v70;
  byte* v71;
  int v72;
  uint v73;
  __int64 v74;
  __int64 v75;
  bool v76;
  int v77;
  int v78;
  short v79;
  uint v80;
  __int64 v81;
  int* v82;
  int v83;
  int v84;
  short v85;
  int v86;
  int* v87;
  int v88;
  int v89;
  int v90;
  int v91;
  int v92;
  int v93;
  int v94;
  int v95;
  int v96;
  int v97;
  uint v98;
  int* v99;
  int* v100;
  int v101;
  int* v102;
  uint v103;
  char v104;
  bool v105;
  int v106;
  bool v107;
  uint v108;
  uint v109;
  uint v110;
  uint v111;
  __int64 v112;
  uint v113;
  uint v114;
  int v115;
  int v116;
  uint v117;
  char v118;
  uint v119;
  char v120;
  __int64 v121;
  int v122;
  int v123;
  word* v124;
  int v125;
  uint v126;
  int v127;
  char v128;
  word v129;
  uint v130;
  char v131;
  word v132;
  word v133;
  int v134;
  int* v135;
  int v136;
  int v137;
  int v138;
  int v139;
  int v140;
  int v141;
  int v142;
  int v143;
  int v144;
  int v145;
  int v146;
  __int64 v147;
  int* v148;
  int v149;
  int v150;
  int v151;
  int v152;
  int v153;
  int v154;
  int v155;
  __int64 v156;
  int v157;
  uint v158;
  int v159;
  int v160;
  int v161;
  int v162;
  int* v163;
  int v164;
  int v165;
  int v166;
  int v167;
  int v168;
  uint v169;
  int v170;
  uint v171;
  int v172;
  uint v173;
  __int64 v174;
  int v175;
  int v176;
  uint* v177;
  uint v178;
  byte v179;
  short v180;
  short v181;
  qword v182;
  int v183;
  int v184;
  int v185;
  uint v186;
  uint* v187;
  __int64 v188;
  int v189;
  int v190;
  int v191;
  uint v192;
  int v193;
  __int64 v194;
  __int64 v195;
  uint* v196;
  int v197;
  int v198;
  uint* v199;
  int v200;
  __int64 v201;
  __int64 v202;
  int v203;
  uint v204;
  int v205;
  int v206;
  int v207;
  int v208;
  __int64 v209;
  int v210;
  int v211;
  char v212;
  __int64* v213;
  __int64* v214;
  __int64 v215;
  int v216;
  byte* v217;
  __int64* v218;
  int v219;
  int v220;
  __int64 v221;
  __int64 v222;
  int v223;
  byte* v224;
  __int64* v225;
  byte* v226;
  __int64 v227;
  __int64 v228;
  __int64 v229;
  byte* v230;
  uint v231;
  byte v232;
  int v233;
  byte* v234;
  byte* v235;
  int v236;
  uint v237;
  uint v238;
  __int64 v239;
  int v240;
  byte* j;
  uint v242;
  int v243;
  short v244;
  uint v245;
  uint v246;
  uint v247;
  uint v248;
  __int64 v249;
  int v250;
  __int64 v251;
  uint v252;
  int v253;
  int v254;
  int v255;
  int v256;
  uint v257;
  uint v258;
  int v259;
  __int64 v260;
  uint v261;
  uint v262;
  int v263;
  __int64 v264;
  int* v265;
  __int64 v266;
  int* v267;
  int v268;
  int v269;
  word v270;
  int v271;
  uint v272;
  int v273;
  int v274;
  int v275;
  int v276;
  int* v277;
  int v278;
  int v279;
  int v280;
  int v281;
  int v282;
  int v283;
  int v284;
  int v285;
  word* v286;
  uint v287;
  int v288;
  int v289;
  int v290;
  int* v291;
  uint v292;
  int v293;
  int v294;
  __int64 v295;
  word v296;
  char v297;
  __int64 v298;
  word v299;
  __int64 v300;
  int v301;
  int v302;
  int v303;
  uint v304;
  int v305;
  word v306;
  __int64 v307;
  word v308;
  char v309;
  __int64 v310;
  word v311;
  __int64 v312;
  word v313;
  __int64 v314;
  word v315;
  __int64 v316;
  word v317;
  __int64 v318;
  word v319;
  int* v320;
  int v321;
  int v322;
  int v323;
  int v324;
  int v325;
  int v326;
  int v327;
  int v328;
  int v329;
  int v330;
  __int64 v331;
  int* v332;
  int v333;
  int v334;
  int v335;
  int v336;
  int v337;
  int v338;
  int v339;
  int v340;
  short v341;
  uint v342;
  uint v343;
  int v344;
  int v345;
  int v346;
  int* v347;
  int v348;
  int v349;
  int v350;
  int v351;
  int v352;
  uint v353;
  int v354;
  uint v355;
  uint v356;
  int v357;
  bool v358;
  uint* v359;
  uint* v360;
  uint* v361;
  int v362;
  uint* v363;
  int v364;
  byte v365;
  __int64 v366;
  int v367;
  int v368;
  uint* v369;
  int v370;
  uint m;
  int v372;
  int v373;
  int v374;
  uint v375;
  int v376;
  int v377;
  int v378;
  int v379;
  __int64* v380;
  __int64* k;
  __int64 v382;
  __int64 v383;
  __int64 v384;
  byte* v385;
  int v386;
  byte v387;
  int v388;
  byte* v389;
  int v390;
  int v391;
  int v392;
  int v393;
  int v394;
  uint v395;
  __int64 v396;
  int v397;
  int v398;
  int v399;
  uint v400;
  int v401;
  int v402;
  __int64 v403;
  __int64 v404;
  char* v405;
  uint v406;
  int v407;
  uint v408;
  uint v409;
  uint v410;
  int v411;
  int v412;
  int v413;
  int v414;
  uint v415;
  char* v416;
  int v417;
  word v418;
  uint v419;
  int v420;
  uint v421;
  __int64 v422;
  __int64 v423;
  int v424;
  __int64 v425;
  int v426;
  int v427;
  word* v429;
  word* v430;
  word* v431;
  word* v432;
  word* v433;
  __int64 v434;
  __int64 v435;
  char v436;
  char v437;
  int v438;
  int v439;
  int v440;
  int v441;
  int v442;
  int v443;
  int v444;
  int v445;
  int v446;
  int v447;
  int v448;
  int v449;
  int v450;
  int v451;
  char v452;
  __int64 v453;
  int v454;
  int v455;
  int v456;
  int v457;
  int v458;
  int v459;
  int* v460;
  int* v461;
  int* v462;
  int* v463;
  int v464;
  int v465;
  int v466;
  int v467;
  bool v468;
  char v469;
  char v470;
  char v471;
  uint v472;
  int* v473;
  __int64 v474;
  __int64 v475;
  byte* v476;
  int v477;
  int v478;
  int v479;
  __int64 v480;
  char v481;
  bool v482;
  __int64 v483;
  char v484;
  uint v485;
  int* v486;
  __int64 v487;
  __int64 v488;
  int v489;
  int v490;
  int v491;
  int v492;
  int v493;
  int v494;
  __int64* v495;
  __int64* v496;
  byte* v497;
  int v498;
  __int64 v499;
  int v500;
  int v501;
  __int64 v502;
  int v503;
  __int64 v504;
  uint v505;
  int v506;
  __int64 v507;
  uint v508;
  uint v509;
  __int64 v510;
  int v511;
  __int64 v512;
  int v513;
  byte v514;
  uint v515;
  uint v516;
  int v517;
  int v518;
  int v519;
  int v520;
  __int64 v521;
  int v522;
  int v523;
  uint v524;
  uint v525;
  int v526;
  int v527;
  int v528;
  int v529;
  int v530;
  int v531;
  int v532;
  int v533;
  int v534;
  int v535;
  int v536;
  int v537;
  int v538;
  int v539;
  char v540;
  int v541;
  __int64 v542;
  byte* v543;
  int v544;
  uint v545;
  __int64 v546;
  int v547;
  int v548;
  byte* v549;
  int v550;
  uint v551;
  uint v552;
  uint v553;
  __int64 v554;
  uint v555;
  int v556;
  byte* v557;
  byte* v558;
  word* v559;
  uint v560;
  byte* v561;
  uint v562;
  __int64 v563;
  int v566;
  int v567;
  short v568;
  int v569;
  int v570;
  uint v571;
  short v572;
  int v573;
  byte* v574;
  __int64 v575;
  int v576;
  int v577;
  __int64 v578;
  int v579;
  uint v580;
  v2 = d8;
  do {
    v3 = (byte*)q10;
    v4 = *(byte*)q10;
    v5 = v4;
    if( *(byte*)q10 ) {
      v6 = q8;
      v7 = *(uint*)(q10+4);
      v8 = *(byte*)(q10+1);
      if( !*(byte*)(v7+q8+6LL*(v8&0xF)+1) ) {
        sub_14000C0E0((byte*)q10);
        v4 = *v3;
        v7 = *((uint*)v3+1);
        v8 = v3[1];
      }
      v566 = v5;
      v546 = v6;
      v547 = v4;
      v545 = v4+1;
      v9 = v4+1;
      v544 = v4+1;
      v548 = v4+1;
      v10 = &q27;
      q16[0] = v7+v6+6LL*(v8&0xF);
      v11 = *(byte*)q16[0];
      v569 = *(byte*)q16[0];
      d13 = v569;
      v12 = &q27;
      d94[v11] = v2;
      v13 = (byte*)(v6+v7-6);
      while( 1 ) {
        v13 += 6;
        v14 = *v13;
        if( v569!=v14 ) {
          d94[*v13] = v2;
          *v10++ = (__int64)v13;
          if( v14==d18 ) {
            v15 = 22;
            v16 = &q16[(b25&7)==0];
LABEL_12:
            for( i = v10-1; i>v16; --i ) {
              v18 = *(i-1);
              if( v15**(byte*)(*i+1)<=*(byte*)(v18+1) )
                break;
              *(i-1) = *i;
              *i = v18;
            }
            ++v12;
            goto LABEL_16;
          }
          if( v2==d77[v14]||v2==d73[v14] ) {
            v16 = v12;
            v15 = 11*(v2==d77[v14])+7;
            goto LABEL_12;
          }
        }
LABEL_16:
        if( !--v9 ) {
          v19 = v566;
          v20 = v548;
          v21 = v544;
          v22 = v545;
          v23 = v547;
          d78 = *(byte*)q16[0];
          d81[(byte)b27[d78+(d68<<8)]] = v2;
          q29 = (__int64)d27;
          q30 = (__int64)d27;
          q31 = (__int64)d27;
          q32 = (__int64)d27;
          q33 = (__int64)d27;
          if( v545==256 ) {
            v44 = d5;
            v46 = d12;
            v45 = d10;
            v66 = *((word*)v3+1);
            q34 = (__int64)d27;
            v570 = -1;
            d14 = -1;
            d95 = v66;
            v64 = v66+1;
            d96 = v64;
          } else {
            v24 = *((uint*)v3+2);
            v25 = *(byte*)(v24+v546);
            v26 = *((word*)v3+1);
            v27 = *(word*)(v24+v546+2);
            v28 = v3[1];
            q14 = (__int64)q16;
            v21 = v25+1;
            v506 = v27;
            v514 = v28;
            v29 = v26;
            if( (v25+1)*v26>(int)(v27*v545)||(v480 = *((uint*)v3+1), *(byte*)(v480+v546+1)>=0x6Au) ) {
              v511 = v26;
              v507 = v545;
            } else if( (v514&0x40)!=0 ) {
              v511 = v26;
              v507 = v545;
            } else {
              v30 = v25+1;
              v517 = v2;
              v31 = (byte*)(v546+*(uint*)(v24+v546+4)-6);
              do {
                v31 += 6;
                --v30;
                b39[*v31] = v31[1];
              } while( v30 );
              v507 = v545;
              v32 = (byte*)(v480+v546+6LL*v545);
              v33 = 9*v547+9+2*v547+2;
              v34 = v29;
              v35 = v21*(((uint)(v33-v29)>>28)|7)+v506;
              do {
                v32 -= 6;
                v37 = v32[1];
                if( v37*v35<(v33+v34)*(byte)b39[*v32]&&v32[1]<=0xE4u ) {
                  v38 = v29-v37;
                  v36 = (byte)b39[*v32];
                  v39 = ((v33+v34-v37)*(3*v37+v36)+3*(v33+v34-v37)+v35-v36-4)/(uint)(3*(v33+v34-v37)+v35-v36);
                  if( v39>v37+11 )
                    v39 = v37+11;
                  v29 = v39+v38;
                  *((word*)v3+1) = v29;
                  v32[1] = v39;
                  v34 = (word)v29;
                }
                --v20;
              } while( v20 );
              v21 = v25+1;
              v22 = v545;
              v2 = v517;
              v24 = *((uint*)v3+2);
              v511 = v34;
              v23 = v547;
              v514 = v3[1];
            }
            v513 = *v3;
            v40 = (byte*)(v546+v24);
            v41 = *(byte*)(v546+*((uint*)v40+1)+6LL*(v40[1]&0xF));
            v42 = v41;
            v570 = v41;
            if( v41==v569 ) {
              v43 = *(byte*)(v546+*((uint*)v40+1)+6LL*(v40[1]&0xF));
              do {
                if( *v40!=*v3 )
                  break;
                v40 = (byte*)(v546+*((uint*)v40+2));
                v42 = *(byte*)(v546+*((uint*)v40+1)+6LL*(v40[1]&0xF));
                v43 = v42;
              } while( v42==v569 );
              v570 = v43;
              d14 = v42;
            } else {
              d14 = *(byte*)(v546+*((uint*)v40+1)+6LL*(v40[1]&0xF));
            }
            v509 = v2!=d94[d15];
            v508 = v2!=d94[v41];
            v44 = d5;
            v45 = d10;
            v46 = d12;
            v512 = (byte)b40[v507];
            v47 = (int)(8*(v513+2*v25+2>d10)+(d12&3)+4*(d5<10)+((v2!=d94[v570])<<9)+(v509<<8)+(((uint)(8*v23+8+4*v23+4-v511)>>26)&0xFFFFFFE0)+(v514&0xC0)+(((uint)(3*v25+3-(4*v23+4))>>27)&0xFFFFFFF0));
            v510 = v47;
            v48 = &d28[2048*v512+2*v47];
            v49 = *v48;
            q34 = (__int64)v48;
            v516 = v49;
            d51 = v49;
            v50 = (word)w10[4096*v512+4*v47];
            v515 = v50;
            d97 = v50;
            if( v50>0x8000||v516>0x80000 ) {
              v463 = &d28[2048*v512+2*v47];
              v467 = v47;
              v494 = d10;
              v459 = d5;
              sub_140010730((__int64)v463);
              v44 = v459;
              v45 = v494;
              LODWORD(v47) = v467;
              v48 = v463;
              LOWORD(v50) = *((word*)v463+2);
            }
            v51 = *((word*)v48+3);
            *((word*)v48+2) = v51+v50;
            if( v51>0x80 ) {
              v489 = v45;
              v486 = &d29[512*v512+16*(v508|v509)+2*((byte)v47&0xF3)];
              q29 = (__int64)v486;
              v484 = v51<0x200;
              q33 = (__int64)(v48+2048);
              v52 = *(v48-2048);
              v483 = (__int64)(v48-2048);
              v53 = v48[2048];
              q32 = (__int64)(v48-2048);
              v485 = v53;
              v54 = ((v53+v52)>>(v484+1))+v516;
              v55 = *((word*)v48+4098);
              v56 = *((word*)v48-4094);
              v516 = v54;
              d51 = v54;
              v57 = ((uint)(v55+v56)>>(v484+1))+v515;
              v45 = v489;
              v515 = v57;
              d97 = v57;
              v19 = v566;
              if( v55>0x8000||v485>0x80000 ) {
                v462 = v48;
                v466 = v47;
                v458 = v44;
                sub_140010730((__int64)(v48+2048));
                v44 = v458;
                v45 = v489;
                LODWORD(v47) = v466;
                v48 = v462;
                LOWORD(v55) = *((word*)v462+4098);
              }
              d98 = *((word*)v48+4099)>>v484;
              v58 = *((word*)v48-4094);
              *((word*)v48+4098) = d98+v55;
              if( v58>0x8000||(uint)*(v48-2048)>0x80000 ) {
                v461 = v48;
                v465 = v47;
                v493 = v45;
                v457 = v44;
                sub_140010730(v483);
                v44 = v457;
                v45 = v493;
                LODWORD(v47) = v465;
                v48 = v461;
                LOWORD(v58) = *((word*)v461-4094);
              }
              d99 = *((word*)v48-4093)>>v484;
              *((word*)v48-4094) = d99+v58;
              v482 = *((word*)v48+3)<0x400u;
              v481 = v484+v482;
              v59 = &v48[-2*v510];
              v60 = LOWORD(v59[2*(int)(v47^0x100)+1]);
              v487 = (__int64)&v59[2*(int)(v47^0x100)];
              q31 = v487;
              if( v60>0x8000||*(uint*)v487>0x80000u ) {
                v460 = v59;
                v464 = v47;
                v492 = v45;
                v456 = v44;
                sub_140010730(v487);
                v44 = v456;
                v45 = v492;
                LODWORD(v47) = v464;
                v59 = v460;
                LOWORD(v60) = *(word*)(v487+4);
              }
              d100 = *(word*)(v487+6)>>v481;
              *(word*)(v487+4) = d100+v60;
              v61 = (__int64)&v59[2*(int)(v47^0x200)];
              v62 = *(word*)(v61+4);
              v488 = v61;
              q30 = v61;
              if( v62>0x8000||*(uint*)v61>0x80000u ) {
                v491 = v45;
                v455 = v44;
                sub_140010730(v61);
                v44 = v455;
                v45 = v491;
                LOWORD(v62) = *(word*)(v488+4);
              }
              d101 = *(word*)(v488+6)>>v481;
              *(word*)(v488+4) = d101+v62;
              v63 = *((word*)v486+2);
              if( v63>0x8000u||(uint)*v486>0x80000 ) {
                v490 = v45;
                v454 = v44;
                sub_140010730((__int64)v486);
                v44 = v454;
                v45 = v490;
                v63 = *((word*)v486+2);
              }
              d102 = *((word*)v486+3)>>(v484+v482+1);
              *((word*)v486+2) = d102+v63;
            }
            d95 = *((word*)v3+1);
            v64 = d95+((v515>>1)+v22*v516)/v515+2;
            d96 = v64;
            if( v22<24 ) {
              d103 = 0;
              v65 = 0;
LABEL_56:
              d104 = 171;
              v571 = v65;
              v67 = 0;
              d105 = 0;
              v68 = 171;
              d106 = 1032*(*((uint*)v3+2)==0)+32*(v46&3)+2*(v44<3);
              v573 = d106;
              v567 = v19+1;
              v69 = q16;
              d41 += (((uint)(v21-v22-4)>>24)&0xFFFFFF80)+28672*((uint)(40-v22)>>31)+32*(*v3>v45)+28672;
              v70 = v64;
              v572 = d33;
              goto LABEL_57;
            }
          }
          v65 = v64>>1;
          d103 = v64>>1;
          goto LABEL_56;
        }
      }
    }
    v222 = q8;
    v223 = *(byte*)(q10+2);
    d13 = v223;
    __PPMContextWalk(v2, v223, &v248, (uint*)&v252, &v259, &v554, &v556);
    v264 = v259+(uint)*((byte*)x9+v252);
    v265 = &d26[0x8000*(qword)(byte)b2[v248]];
    q32 = (__int64)&v265[2*(v264^0x1000)];
    q31 = (__int64)&v265[2*(v264^1)];
    q30 = (__int64)&v265[2*(v264^0x2000)];
    q29 = (__int64)&v265[2*(v264^0x20)];
    q34 = (__int64)&v265[2*(v264^0x200)];
    v266 = (__int64)&v265[2*(v264^0x400)];
    v267 = &v265[2*v264];
    v268 = *((word*)v267+2);
    q35 = v266;
    q21 = (__int64)v267;
    q22 = (__int64)(v267-4);
    q18 = (__int64)(v267+4);
    if( v268>0x8000 ) {
      sub_140010850((__int64)v267);
      v268 = *((word*)v267+2);
    }
    v269 = *(word*)v267;
    v270 = *((word*)v267+3);
    d51 = v269;
    *((word*)v267+2) = v268+v270;
    d97 = v268;
    d52 = v269;
    d50 = v268;
    v271 = (v223==d83)+(d33&0xE200);
    v272 = v223==d85;
    v273 = (v223&0xE0)+16*(v223==d68)+8*(v223==d66)+4*(v223==d92);
    v274 = (((word)v223-(word)d62)&0x800)+(((word)d64-(word)v223)&0x400)+((v223==d76)<<8);
    v275 = v556;
    v276 = v274+v273+v271+2*v272;
    if( v556 )
      v275 = 1;
    d33 = (v275<<12)+v276+(((v268<(uint)(56*v269))+(v268<(uint)(6*v269)))<<16);
    v277 = &d34[2*d33];
    v278 = *v277;
    q23 = (__int64)v277;
    v279 = v269*(__int64)v278/v277[1];
    v280 = d51;
    v281 = 1-d51;
    if( v279>=1-d51 ) {
      v281 = 0x40000;
      if( v279<0x40000 )
        v281 = v279;
    }
    v282 = *v277;
    v283 = v277[1];
    if( (int)abs32(*v277)>0x40000 ) {
      v282 >>= 1;
      *v277 = v282;
      v283 >>= v283>4096;
    }
    v284 = v282-v280;
    *v277 = v284;
    v285 = v280+v283+2;
    if( v285>0x40000000 ) {
      *v277 = v284>>1;
      v285 >>= 1;
    }
    v277[1] = v285;
    v286 = (word*)q21;
    v287 = *(word*)(q21+6);
    v288 = v281+d97;
    v289 = v281+v280;
    if( v287<=0x20 ) {
      v290 = d13;
      d97 = v288;
      d51 = v289;
      q35 = (__int64)d27;
      q34 = (__int64)d27;
      q29 = (__int64)d27;
      q30 = (__int64)d27;
      q31 = (__int64)d27;
      q32 = (__int64)d27;
      q18 = (__int64)d27;
      q22 = (__int64)d27;
      q20 = (__int64)d27;
      q17 = (__int64)d27;
      q36 = (__int64)&d110;
    } else {
      v290 = d13;
      v291 = &d30[128*(qword)(byte)b6[v248]+64*(d9>-9)+4*(uint)(d73[d13]==d8)+(((uint)v264>>6)&0xBLL)+(uint)(16*v556)];
      v292 = *v291;
      q36 = (__int64)v291;
      v293 = (v292>>(v287<0x230))+v289;
      d51 = v293;
      v294 = (0x3100u>>(v287<0x230))+v288;
      d97 = v294;
      *v291 = v292-((v292+2)>>3);
      v295 = q22;
      v105 = v287<0x220;
      v296 = *(word*)(q22+4);
      v297 = v105;
      if( v296>0x8000u ) {
        v435 = q22;
        v432 = v286;
        sub_140010850(q22);
        v286 = v432;
        v295 = v435;
        v296 = *(word*)(v435+4);
      }
      d48 = *(word*)(v295+6)>>v297;
      *(word*)(v295+4) = d48+v296;
      v298 = q18;
      v299 = *(word*)(q18+4);
      if( v299>0x8000u ) {
        v434 = q18;
        v431 = v286;
        sub_140010850(q18);
        v286 = v431;
        v298 = v434;
        v299 = *(word*)(v434+4);
      }
      d49 = *(word*)(v298+6)>>v297;
      v300 = (__int64)(v286-0x10000);
      q20 = (__int64)(v286-0x10000);
      *(word*)(v298+4) = d49+v299;
      v301 = *(v286-0x10000);
      v302 = v286[0x10000];
      v303 = *(v286-65534);
      q17 = (__int64)(v286+0x10000);
      v304 = v302+v301;
      v305 = v286[65538];
      v289 = (v304>>(v297+1))+v293;
      d51 = v289;
      v288 = ((uint)(v305+v303)>>(v297+1))+v294;
      d97 = v288;
      if( v305>0x8000 ) {
        v433 = v286-0x10000;
        v430 = v286;
        sub_140010850((__int64)(v286+0x10000));
        v286 = v430;
        v300 = (__int64)v433;
        LOWORD(v305) = v430[65538];
      }
      d46 = v286[65539]>>v297;
      v286[65538] = d46+v305;
      v306 = *(v286-65534);
      if( v306>0x8000u ) {
        v429 = v286;
        sub_140010850(v300);
        v286 = v429;
        v306 = *(v429-65534);
      }
      d47 = *(v286-65533)>>v297;
      *(v286-65534) = d47+v306;
      v307 = q32;
      v308 = *(word*)(q32+4);
      v309 = (v286[3]<0x398u)+v297;
      if( v308>0x8000u ) {
        sub_140010850(q32);
        v308 = *(word*)(v307+4);
      }
      d99 = *(word*)(v307+6)>>v309;
      *(word*)(v307+4) = d99+v308;
      v310 = q31;
      v311 = *(word*)(q31+4);
      if( v311>0x8000u ) {
        sub_140010850(q31);
        v311 = *(word*)(v310+4);
      }
      d100 = *(word*)(v310+6)>>v309;
      *(word*)(v310+4) = d100+v311;
      v312 = q30;
      v313 = *(word*)(q30+4);
      if( v313>0x8000u ) {
        sub_140010850(q30);
        v313 = *(word*)(v312+4);
      }
      d101 = *(word*)(v312+6)>>v309;
      *(word*)(v312+4) = d101+v313;
      v314 = q29;
      v315 = *(word*)(q29+4);
      if( v315>0x8000u ) {
        sub_140010850(q29);
        v315 = *(word*)(v314+4);
      }
      d102 = *(word*)(v314+6)>>v309;
      *(word*)(v314+4) = d102+v315;
      v316 = q34;
      v317 = *(word*)(q34+4);
      if( v317>0x8000u ) {
        sub_140010850(q34);
        v317 = *(word*)(v316+4);
      }
      d105 = *(word*)(v316+6)>>v309;
      *(word*)(v316+4) = d105+v317;
      v318 = q35;
      v319 = *(word*)(q35+4);
      if( v319>0x8000u ) {
        sub_140010850(q35);
        v319 = *(word*)(v318+4);
      }
      d104 = *(word*)(v318+6)>>v309;
      *(word*)(v318+4) = d104+v319;
    }
    d51 = 60416LL*v289/v288;
    d97 = 60416;
    d54 = d51;
    d53 = 60416;
    v320 = &d36[0x100000*(d17<(uint)d80)+0x80000*(v290==d18)+0x20000*d11+512*d64+2*v290+2*(qword)((byte)((uint)d108<0x78)<<17)];
    v321 = *v320;
    q19 = (__int64)v320;
    v322 = d51*(__int64)v321/v320[1];
    v323 = d51;
    v324 = 1-d51;
    if( v322>=1-d51 ) {
      v324 = 0x40000;
      if( v322<0x40000 )
        v324 = (int)(60416LL*v289/v288)*(__int64)v321/v320[1];
    }
    v325 = *v320;
    if( (int)abs32(*v320)>0x80000 ) {
      v326 = v320[1];
      v325 >>= 1;
      *v320 = v325;
      v320[1] = v326>>(v326>0x2000);
    }
    v327 = v320[1];
    v328 = v325-v323;
    *v320 = v328;
    v329 = v323+v327+1120;
    v320[1] = v329;
    if( v329>0x40000000 ) {
      *v320 = v328>>1;
      v320[1] = v329>>1;
    }
    d97 += v324;
    d56 = d97;
    v330 = v324+v323;
    d51 = v330;
    v331 = ((17*v330<(uint)d97)<<13)+(((word)d109-(word)v290)&0x1000)+16*(v290==d15)+8*(v290==d18)+4*(v290==d87)+(v290==d84)+(((uint)d107<0x29)<<7)+d38+(d33&0x800)+2*(*((byte*)x4+v290)+(v290==d86))+32*(((uint)d108<0xA800)+((uint)d108<0x600)+((uint)d108<0xD));
    v332 = &d39[2*v331];
    v333 = *v332;
    q24 = (__int64)v332;
    v334 = (d97-v330)*(__int64)v333/d40[2*v331];
    v335 = v330;
    v336 = v330-d97+1;
    if( v334>=v336 ) {
      v336 = 0x40000;
      if( v334<0x40000 )
        v336 = (d97-v330)*(__int64)v333/d40[2*v331];
    }
    v337 = *v332;
    v338 = v336+d97;
    d97 += v336;
    v339 = d40[2*v331];
    if( (int)abs32(v337)>0x100000 ) {
      v337 >>= 1;
      *v332 = v337;
      v339 >>= v339>0x2000;
    }
    v340 = v337+2*v330;
    v341 = d78;
    *v332 = v340;
    d40[2*v331] = v339+1;
    v342 = q8+*(uint*)(v554+2)-q4;
    v343 = d73[v290]==d8;
    v344 = *(byte*)(q12+(((word)v290-v341)&0x1FF));
    v345 = (*(uint*)(q12+512)<(uint)(384*v344))+(*(uint*)(q12+512)<(uint)(58*v344))+(*(uint*)(q12+512)<(uint)(22*v344));
    v346 = (byte)b14[d63];
    d55 = v338;
    v347 = &d42[2*(int)(4*(v342>=0xFFFFFD20)+v343+(v331&0x700)+d41+(((uint)d108<0x2C00)<<11)+2*(v290==v346)+8*v345)];
    v348 = *v347;
    q25 = (__int64)v347;
    v349 = (v338-v335)*(__int64)v348/v347[1];
    v350 = d51;
    v351 = d51-d97+1;
    if( v349>=v351 ) {
      v351 = 0x80000;
      if( v349<0x80000 )
        v351 = (v338-v335)*(__int64)v348/v347[1];
    }
    v352 = *v347;
    v353 = v351+d97;
    d97 += v351;
    v354 = v347[1];
    if( (int)abs32(v352)>0x100000 ) {
      v352 >>= 1;
      *v347 = v352;
      v354 >>= v354>0x2000;
    }
    *v347 = v352+v335;
    v347[1] = v354+2;
    v356 = rc.GetSubRange(v350, v353);
    if( !rc.IsDecodeMatched(v356) ) {
      v423 = (uint)d58;
      v424 = d59;
      v425 = v3[2];
      v426 = v353*v3[3];
      rc.DecodeNotMatched(v356);
      d94[v425] = d8;
      d57[v423] &= ~v424;
      q9 = 0;
      d60[d61] &= ~d59;
      v195 = q8;
      v427 = v3[3];
      v3 = (byte*)q10;
      v203 = *(byte*)q10;
      d11 = 0;
      d111 = v203;
      LODWORD(v202) = (int)(16*(v427*v350+v350-v427*v353))/(int)(v353+v426);
      d45 = v202;
      do {
LABEL_126:
        v204 = *((uint*)v3+2);
        if( !v204 )
          return v202;
        v205 = d5;
        v206 = --d5;
        v3 = (byte*)(v195+v204);
        v207 = *v3;
        q10 = (__int64)v3;
        if( v207 ) {
          v202 = v3[1]&0xF;
          if( !*(byte*)(v195+*((uint*)v3+1)+6*v202+1) )
            LODWORD(v202) = sub_14000C0E0(v3);
        }
        v208 = v207-v203;
      } while( !v208 );
      v209 = *((uint*)v3+1);
      v536 = v206;
      v537 = v205;
      v567 = v208;
      v535 = *v3;
      v67 = 0x8000;
      v534 = v535+1;
      v210 = v535-v203;
      v538 = v535+1;
      v211 = v210;
      v212 = 0;
      v213 = q16;
      v541 = 0;
      v214 = q16;
      v215 = *(byte*)(v195+v209+6LL*(v3[1]&0xF));
      v543 = (byte*)(v195+v209+6LL*(v3[1]&0xF));
      v2 = d8;
      v569 = *v543;
      d13 = v569;
      v542 = v215;
      if( d8==d94[v215] ) {
        d105 = 0x8000;
      } else {
        v216 = v543[1];
        q16[0] = (__int64)v543;
        v212 = 1;
        v213 = &q27;
        d94[v542] = d8;
        v214 = &q27;
        v541 = v216;
        v67 = 0;
        v211 = v210-1;
        d105 = 0;
      }
      v217 = (byte*)(v195+v209-6);
      if( v211 ) {
        v218 = v213;
        v520 = v210;
        v219 = v541;
        v521 = v195;
        v522 = v172;
        v524 = v171;
        v526 = v67;
        v558 = v3;
        while( 1 ) {
          do {
            v217 += 6;
            v220 = *v217;
            v221 = *v217;
          } while( v2==d94[v221] );
          v378 = v217[1];
          d94[v221] = v2;
          *v213++ = (__int64)v217;
          v214 = v213;
          v219 += v378;
          if( v220==d18 )
            break;
          if( v2==d77[v221]||v2==d73[v221] ) {
            v380 = v218;
            v379 = 4*(v2==d77[v221])+12;
            goto LABEL_274;
          }
LABEL_278:
          if( !--v211 ) {
            v195 = v521;
            v172 = v522;
            v171 = v524;
            v67 = v526;
            v3 = v558;
            v541 = v219;
            v210 = v520;
            goto LABEL_280;
          }
        }
        v379 = 22;
        v380 = &q16[(byte)v212&((b25&7)==0)];
LABEL_274:
        for( k = v213-1; k>v380; --k ) {
          v382 = *(k-1);
          if( v379**(byte*)(*k+1)<=*(byte*)(v382+1) )
            break;
          *(k-1) = *k;
          *k = v382;
        }
        ++v218;
        goto LABEL_278;
      }
LABEL_280:
      q29 = (__int64)d27;
      q30 = (__int64)d27;
      q31 = (__int64)d27;
      q32 = (__int64)d27;
      q33 = (__int64)d27;
      if( v535==255 ) {
        v570 = -1;
        d14 = -1;
        q34 = (__int64)d27;
        v419 = 1;
        v540 = d12;
        v539 = d10;
      } else {
        v383 = *((uint*)v3+2);
        v497 = (byte*)(v195+v383);
        v504 = *(uint*)(v195+v383+4);
        v570 = *(byte*)(v195+v504+6LL*(*(byte*)(v195+v383+1)&0xF));
        if( v570==v569 ) {
          v384 = *(uint*)(v195+v383+4);
          v385 = (byte*)(v195+v383);
          v386 = *(byte*)(v195+v384+6LL*(*(byte*)(v195+v383+1)&0xF));
          do {
            if( *v385!=*v3 )
              break;
            v385 = (byte*)(v195+*((uint*)v385+2));
            v384 = *((uint*)v385+1);
            v386 = *(byte*)(v195+v384+6LL*(v385[1]&0xF));
          } while( v386==v569 );
          v504 = v384;
          v497 = v385;
          v570 = v386;
          d14 = v386;
        } else {
          d14 = *(byte*)(v195+*(uint*)(v195+v383+4)+6LL*(*(byte*)(v195+v383+1)&0xF));
        }
        v387 = v3[1];
        if( 16*v210<=v541||(v3[1]&0x40)!=0 ) {
          q14 = (__int64)v214;
          v503 = *((word*)v3+1);
        } else {
          v388 = *v497+1;
          v389 = (byte*)(v195+v504);
          v390 = v388;
          do {
            b39[*v389] = v389[1];
            v389 += 6;
            --v390;
          } while( v390 );
          v391 = *((word*)v3+1);
          v392 = 5*v535+5;
          v523 = v172;
          v525 = v171;
          v527 = v67;
          v393 = v391;
          v519 = v2;
          v394 = 0;
          v395 = (5*((uint)(2*v392-v391)>>31)+8)*v388+*((word*)v497+1);
          do {
            v396 = *--v213;
            v398 = *(byte*)(*v213+1);
            v399 = v393+2*v392;
            LOBYTE(v400) = *(byte*)(*v213+1);
            if( (int)(v398*v395)<v399*(byte)b39[*(byte*)*v213]&&*(byte*)(*v213+1)<=0xE4u ) {
              v401 = v391-v398;
              v397 = (byte)b39[*(byte*)*v213];
              v400 = ((v399-v398)*(3*v398+v397)+3*(v399-v398)+v395-v397-4)/(3*(v399-v398)+v395-v397);
              if( v400>v398+11 )
                v400 = v398+11;
              v391 = v400+v401;
              *((word*)v3+1) = v391;
              *(byte*)(v396+1) = v400;
              v393 = (word)v391;
            }
            v394 += (byte)v400;
          } while( v213!=q16 );
          v172 = v523;
          v171 = v525;
          v387 = v3[1];
          v383 = *((uint*)v3+2);
          v503 = v393;
          v67 = v527;
          v541 = v394;
          v2 = v519;
          q14 = (__int64)v213;
        }
        v402 = *(byte*)(v383+v195);
        v538 = v402+1;
        v501 = v2!=d94[d15];
        v498 = v2!=d94[v570];
        v540 = d12;
        v500 = 2*v402+2;
        v403 = (int)((((16*(uint)*v3-v503)>>26)&0xFFFFFFE0)+(d12&3)+8*(v501|(v2!=d94[*(byte*)(v195+*(uint*)(v383+v195+4)+6LL*(*(byte*)(v383+v195+1)&0xF))]))+4*(v534*v541>v503*v210)+(v387&0xC0)+16*(v210>=2*v402-2*v535));
        v502 = (byte)b40[v534];
        v404 = (v502<<11)+8*v403;
        v499 = v403;
        v405 = (char*)d29+v404;
        v406 = *(int*)((char*)d29+v404);
        v407 = *(word*)((char*)&w12+v404);
        q34 = (__int64)d29+v404;
        d51 = v406;
        v505 = v407;
        d97 = v407;
        if( v407>0x8000||v406>0x80000 ) {
          v443 = v403;
          v533 = v67;
          sub_140010730((__int64)d29+v404);
          v67 = v533;
          LODWORD(v403) = v443;
          LOWORD(v407) = *((word*)v405+2);
        }
        v408 = *((word*)v405+3);
        *((word*)v405+2) = v408+v407;
        v539 = d10;
        if( v408>0x78 ) {
          v473 = &d28[2048*v502+16*(uint)(*v3+v500>d10)+2*((byte)v403&0xF3)+2*(uint)(v498<<9)+2*(uint)(v501<<8)+2*(qword)(4*(uint)(v536<10))];
          q29 = (__int64)v473;
          v468 = v408<0x100;
          v471 = (v408<0x100)+1;
          v409 = *((uint*)v405+512);
          q33 = (__int64)(v405+2048);
          v410 = (v409+*((uint*)v405-512))>>v471;
          q32 = (__int64)(v405-2048);
          v472 = v409;
          v411 = *((word*)v405-1022);
          v406 += v410;
          v412 = *((word*)v405+1026);
          d51 = v406;
          v505 += (uint)(v412+v411)>>v471;
          d97 = v505;
          if( v412>0x8000||v472>0x80000 ) {
            v442 = v403;
            v532 = v67;
            sub_140010730((__int64)(v405+2048));
            v67 = v532;
            LODWORD(v403) = v442;
            v412 = *((word*)v405+1026);
          }
          v413 = *((word*)v405-1022);
          v414 = (*((word*)v405+1027)>>v471)+v412;
          d98 = *((word*)v405+1027)>>v471;
          *((word*)v405+1026) = v414;
          if( v413>0x8000||*((uint*)v405-512)>0x80000u ) {
            v441 = v403;
            v531 = v67;
            sub_140010730((__int64)(v405-2048));
            v67 = v531;
            LODWORD(v403) = v441;
            LOWORD(v413) = *((word*)v405-1022);
          }
          d99 = *((word*)v405-1021)>>v471;
          *((word*)v405-1022) = d99+v413;
          v470 = *((word*)v405+3)<0x200u;
          v415 = *((word*)v473+2);
          if( v415>0x8000||(uint)*v473>0x80000 ) {
            v440 = v403;
            v530 = v67;
            sub_140010730((__int64)v473);
            v67 = v530;
            LODWORD(v403) = v440;
            LOWORD(v415) = *((word*)v473+2);
          }
          d102 = *((word*)v473+3)>>v470;
          *((word*)v473+2) = d102+v415;
          v469 = (*((word*)v405+3)<0x400u)+v468+1;
          v416 = &v405[-8*v499];
          v417 = *(word*)&v416[8*(int)(v403^2)+4];
          v474 = (__int64)&v416[8*(int)(v403^2)];
          q31 = v474;
          if( v417>0x8000||*(uint*)v474>0x80000u ) {
            v439 = v403;
            v529 = v67;
            sub_140010730(v474);
            v67 = v529;
            LODWORD(v403) = v439;
            LOWORD(v417) = *(word*)(v474+4);
          }
          d100 = *(word*)(v474+6)>>v469;
          *(word*)(v474+4) = d100+v417;
          v475 = (__int64)&v416[8*(int)(v403^8)];
          q30 = v475;
          v418 = *(word*)(v475+4);
          if( v418>0x8000u||*(uint*)v475>0x80000u ) {
            v528 = v67;
            sub_140010730(v475);
            v67 = v528;
            v418 = *(word*)(v475+4);
          }
          d101 = *(word*)(v475+6)>>v469;
          *(word*)(v475+4) = d101+v418;
        }
        v419 = ((v505>>1)+v534*v406)/v505+2;
        if( v534<24 ) {
          v571 = 0;
          goto LABEL_317;
        }
      }
      v571 = (int)(v419+*((word*)v3+1))>>1;
LABEL_317:
      d103 = v571;
      v70 = v541+v419;
      d96 = v70;
      d9 = d16;
      d95 = v541+v535+1;
      v68 = 0;
      v69 = q16;
      d104 = 0;
      q14 = (__int64)q16;
      v420 = (v536>1)<<15;
      v572 = v420+(d33&0x7FFF);
      d33 = v420+(d33&0xFFFF7FFF);
      d38 = ((v536>3)<<15)+v420+(d38&0x4600);
      v421 = v536<3||v537+22<d7;
      v573 = 1032*(*((uint*)v3+2)==0)+32*(v540&3)+2*v421;
      d106 = v573;
      d41 = 32*(*v3+28>2*v539)+(((uint)(v538-v534-3)>>24)&0xFFFFFF80)+((v536>2)<<13)+(((d7>7)+(d7>2)+7*(((uint)(74-v534)>>31)+((uint)(2-v534)>>31))+(v536>16))<<12);
      while( 1 ) {
        rc.DecodeNormalize(a2);
LABEL_57:
        v71 = (byte*)*v69;
        v72 = *(byte*)(*v69+1);
        v574 = v3;
        q14 = (__int64)(v69+1);
        q18 = (__int64)d27;
        q22 = (__int64)d27;
        q20 = (__int64)d27;
        q17 = (__int64)d27;
        v73 = *v71;
        v74 = ((v72<<8)-v68)/v70;
        d59 = 1<<*v71;
        v577 = v74;
        v575 = v74;
        v578 = (uint)(byte)b1[v74]+1;
        d58 = ((v73+d69)>>5)+0x2000;
        d61 = v67+((v73+d70)>>5);
        v75 = v73;
        v76 = v2==d73[v73];
        v576 = ((d59&d57[((v73+d69)>>5)+0x2000])!=0)+2*((d59&d60[v67+((v73+d70)>>5)])!=0);
        q37 = (__int64)&q38;
        q39 = (__int64)&q38;
        v77 = v2==d81[v73];
        v580 = v2==d77[v73];
        v78 = v73==d72||*(uint*)(q12+512)<*(byte*)(q12+(((word)v73-(word)d78)&0x1FF))<<7;
        v79 = d62;
        v579 = d24[256*d62+v73];
        if( (uint)(d25-v579)>=0x20000 ) {
          v80 = 0x20000;
          d107 = 0x20000;
          d108 = 0x20000;
          d109 = 0x20000;
        } else {
          d108 = d24[256*d62+v73];
          d109 = (byte)b26[(v579+2)&0x1FFFF];
          d108 = d25-v579;
          v80 = d25-d24[256*v73+d109];
          d107 = v80;
        }
        v81 = v573|(((v80<0x220)<<12)+(v78<<9)+((v70<v571)<<11)+8*(v73==v570)+((v73==v569)<<10)+*((byte*)x4+v73)+(v77<<8)+16*(v73==d18)+4*v580+(uint)v76);
        v82 = &d27[0x4000*v578+2*v81];
        v83 = (word)w4[0x8000*v578+4*v81];
        q21 = (__int64)v82;
        if( v83>0x8000 ) {
          sub_140010850((__int64)&d27[0x4000*v578+2*v81]);
          v79 = d62;
          v83 = *((word*)v82+2);
        }
        v84 = *(word*)v82;
        v85 = *((word*)v82+3);
        d51 = v84;
        *((word*)v82+2) = v83+v85;
        d97 = v83;
        d52 = v84;
        d50 = v83;
        v86 = v576;
        if( v576 )
          v86 = 1;
        d33 = (((word)v73-v79)&0x800)+(v86<<12)+(v572&0xE200)+(((word)d64-(word)v73)&0x400)+((v73==d76)<<8)+(v73&0xE0)+16*(v73==d68)+8*(v73==d66)+4*(v73==d92)+(v73==d83)+2*(v73==d85)+(((v83<(uint)(56*v84))+(v83<(uint)(6*v84)))<<16);
        v87 = &d34[2*d33];
        v88 = *v87;
        q23 = (__int64)v87;
        v89 = v84*(__int64)v88/v87[1];
        v90 = d51;
        v91 = 1-d51;
        if( v89>=1-d51 ) {
          v91 = 0x40000;
          if( v89<0x40000 )
            v91 = v89;
        }
        v92 = *v87;
        v93 = v87[1];
        if( (int)abs32(*v87)>0x40000 ) {
          v92 >>= 1;
          *v87 = v92;
          v93 >>= v93>4096;
        }
        v94 = v92-v90;
        *v87 = v94;
        v95 = v90+v93+2;
        if( v95>0x40000000 ) {
          *v87 = v94>>1;
          v95 >>= 1;
        }
        v87[1] = v95;
        v96 = v91+d97;
        v97 = v91+v90;
        v98 = *(word*)(q21+6);
        if( v98<=8 ) {
          d97 = v96;
          d51 = v97;
        } else {
          v559 = (word*)q21;
          v560 = *(word*)(q21+6);
          v557 = v3;
          v561 = v71;
          v562 = v73;
          v563 = v73;
          v99 = &d31[512*(qword)(byte)b8[v575]+2*(uint)(v576<<6)];
          v100 = &v99[2*(v81&0x1F)];
          v101 = v100[1];
          q39 = (__int64)v100;
          v102 = &v99[2*(((uint)v81>>4)&0x1F)+64];
          v103 = *v102;
          q37 = (__int64)v102;
          v104 = v98<0x150;
          v105 = v98<0x48;
          v106 = v102[1];
          v107 = v105;
          v108 = v101*(v577+2);
          v109 = v106+v101;
          v110 = v108>>8;
          v111 = (uint)(v106*(v577+2))>>8;
          v112 = (v109>>11)&0xFFFFFFFE;
          v105 = v109<0xC000;
          v113 = *v100;
          v114 = v110+v111;
          if( !v105 )
            v112 = 24;
          v115 = (byte)b41[v112];
          v116 = (byte)b42[v112];
          if( v114>=0x6000 )
            v114 = 24576;
          v117 = ((v115*v114+v116*(v103+*v100))>>(v104+7))+v97;
          v118 = v104+v107+3;
          v119 = ((uint)(192*(v115+v116))>>v104)+v96;
          d112 = 0x3000u>>v118;
          v120 = v104+4;
          d113 = 0x3000u>>v120;
          *v100 = v113-(v113>>v118);
          v3 = v557;
          v100[1] -= (v110+7)>>v120;
          v71 = v561;
          *v102 = v103-(v103>>v118);
          v102[1] -= (v111+7)>>v120;
          v121 = (__int64)(v559-0x8000);
          v122 = *(v559-0x8000);
          v75 = v563;
          v123 = *(v559-32766);
          v124 = v559+0x8000;
          v125 = v559[0x8000];
          q17 = (__int64)(v559+0x8000);
          v126 = v122+v125;
          v127 = v559[32770];
          q20 = (__int64)(v559-0x8000);
          v97 = (v126>>3)+v117;
          d51 = v97;
          v96 = ((uint)(v127+v123)>>3)+v119;
          v73 = v562;
          d97 = v96;
          v128 = (v560<0x30)+1;
          if( v127>0x8000 ) {
            v447 = v97;
            v451 = v96;
            sub_140010850((__int64)(v559+0x8000));
            v96 = v451;
            v97 = v447;
            v121 = (__int64)(v559-0x8000);
            v128 = (v560<0x30)+1;
            v124 = v559+0x8000;
            LOWORD(v127) = v559[32770];
          }
          d46 = v124[3]>>v128;
          v124[2] = d46+v127;
          v129 = *(word*)(v121+4);
          if( v129>0x8000u ) {
            v452 = v128;
            v453 = v121;
            v446 = v97;
            v450 = v96;
            sub_140010850(v121);
            v96 = v450;
            v97 = v446;
            v121 = v453;
            v128 = v452;
            v129 = *(word*)(v453+4);
          }
          d47 = *(word*)(v121+6)>>v128;
          *(word*)(v121+4) = d47+v129;
          v130 = v559[3];
          if( v130>0x48 ) {
            q22 = (__int64)(v559+0x10000);
            v131 = (v130<0x1E0)+(v130<0x3D0)+1;
            v132 = v559[65538];
            if( v132>0x8000u ) {
              v437 = v131;
              v445 = v97;
              v449 = v96;
              sub_140010850((__int64)(v559+0x10000));
              v96 = v449;
              v97 = v445;
              v131 = v437;
              v132 = v559[65538];
            }
            d48 = v559[65539]>>v131;
            q18 = (__int64)(v559-0x10000);
            v559[65538] = d48+v132;
            v133 = *(v559-65534);
            if( v133>0x8000u ) {
              v436 = v131;
              v444 = v97;
              v448 = v96;
              sub_140010850((__int64)(v559-0x10000));
              v96 = v448;
              v97 = v444;
              v131 = v436;
              v133 = *(v559-65534);
            }
            d49 = *(v559-65533)>>v131;
            *(v559-65534) = d49+v133;
          }
        }
        v134 = 60416LL*v97/v96;
        d51 = v134;
        d97 = 60416;
        d54 = v134;
        d53 = 60416;
        v135 = &d36[0x100000*(d17<(uint)d80)+0x80000*(v73==d18)+0x20000*d11+512*d64+2*v75+2*(qword)((byte)((uint)d108<0x78)<<17)];
        v136 = *v135;
        q19 = (__int64)v135;
        v137 = v134*(__int64)v136/v135[1];
        v138 = d51;
        v139 = 1-d51;
        if( v137>=1-d51 ) {
          v139 = 0x40000;
          if( v137<0x40000 )
            v139 = v137;
        }
        v140 = *v135;
        if( (int)abs32(*v135)>0x80000 ) {
          v141 = v135[1];
          v140 >>= 1;
          *v135 = v140;
          v135[1] = v141>>(v141>0x2000);
        }
        v142 = v135[1];
        v143 = v140-v138;
        *v135 = v143;
        v144 = v138+v142+1120;
        v135[1] = v144;
        if( v144>0x40000000 ) {
          *v135 = v143>>1;
          v135[1] = v144>>1;
        }
        v145 = *((byte*)x4+v75);
        d97 += v139;
        v146 = v139+v138;
        d51 = v146;
        d56 = d97;
        v147 = ((17*v146<(uint)d97)<<13)+(((word)d109-(word)v73)&0x1000)+16*(v73==d15)+8*(v73==d18)+4*(v73==d87)+(v73==d84)+(((uint)d107<0x29)<<7)+d38+(d33&0x800)+2*(v145+(v73==d86))+32*(((uint)d108<0xA800)+((uint)d108<0x600)+((uint)d108<0xD));
        v148 = &d39[2*v147];
        v149 = *v148;
        q24 = (__int64)v148;
        v150 = (d97-v146)*(__int64)v149/d40[2*v147];
        v151 = v146;
        v152 = v146-d97+1;
        if( v150>=v152 ) {
          v152 = 0x40000;
          if( v150<0x40000 )
            v152 = v150;
        }
        v153 = *v148;
        v154 = v152+d97;
        d97 += v152;
        v155 = d40[2*v147];
        if( (int)abs32(v153)>0x100000 ) {
          v153 >>= 1;
          *v148 = v153;
          v155 >>= v155>0x2000;
        }
        v76 = d73[v75]==d8;
        v156 = q12;
        *v148 = v153+2*v146;
        d40[2*v147] = v155+1;
        v157 = *(byte*)(v156+(((word)v73-(word)d78)&0x1FF));
        LODWORD(v156) = *(uint*)(v156+512);
        v158 = q8+*(uint*)(v71+2)-q4+736;
        v159 = (byte)b14[d63];
        d55 = v154;
        v160 = 8*(((uint)v156<384*v157)+((uint)v156<58*v157)+((uint)v156<22*v157));
        v161 = (v147&0x700)+d41+(((uint)d108<0x2C00)<<11)+2*(v73==v159);
        v162 = 4*(v158<0x2E0)+v76;
        v163 = &d42[2*v162+2*v161+2*v160];
        v164 = *v163;
        q25 = (__int64)v163;
        v165 = (v154-v151)*(__int64)v164/d43[2*v162+2*v161+2*v160];
        v166 = d51;
        v167 = d51-d97+1;
        if( v165>=v167 ) {
          v167 = 0x80000;
          if( v165<0x80000 )
            v167 = v165;
        }
        v168 = *v163;
        v169 = v167+d97;
        d97 += v167;
        v170 = v163[1];
        if( (int)abs32(v168)>0x100000 ) {
          v168 >>= 1;
          *v163 = v168;
          v170 >>= v170>0x2000;
        }
        *v163 = v168+v151;
        v163[1] = v170+2;
        v173 = rc.GetSubRange(v166, v169);
        if( !rc.IsDecodeMatched(v173) ) {
          v422 = q9;
          d96 -= v71[1];
          rc.DecodeNotMatched(v173);
          d57[d58] &= ~d59;
          d60[d61] &= ~d59;
          if( v422 ) {
            v3 = (byte*)q10;
            q9 = 0;
            d11 = 0;
          } else if( (qword)v71<=q8+(qword)*((uint*)v3+1) ) {
            v3 = (byte*)q10;
          } else {
            v3 = (byte*)q10;
            d105 = 0x8000;
          }
        } else {
          v174 = q39;
          v175 = d112;
          v176 = d113;
          v177 = (uint*)q37;
          v178 = d108;
          *(uint*)q39 += d112;
          *(uint*)(v174+4) += v176;
          *v177 += v175;
          v179 = v3[1];
          v177[1] += v176;
          v180 = (v179&0x40)==0||v178>0xE00;
          v181 = (v178>0x4800)+(v178>0x380)+(v178>0x80)+v180+4;
          v3[1] = v179&0xF0;
          *((word*)v3+1) += v181;
          v71[1] += v181;
          v182 = q8+*((uint*)v3+1);
          if( v71==(byte*)v182 ) {
            v3 = (byte*)q10;
            if( q10==q11 )
              d11 = v169<2*v166;
          } else {
            v568 = *(word*)v71;
            v183 = *(uint*)(v71+2);
            do {
              v184 = *((uint*)v71-1);
              *(word*)v71 = *((word*)v71-3);
              *(uint*)(v71+2) = v184;
              v71 -= 6;
            } while( (qword)v71>v182 );
            v3 = (byte*)q10;
            *(word*)v71 = v568;
            *(uint*)(v71+2) = v183;
          }
          v185 = v71[1];
          q9 = (__int64)v71;
          if( v185>244 ) {
            sub_14000B750(v574);
            v71 = (byte*)q9;
          }
          if( v71 )
            goto LABEL_248;
        }
        if( !--v567 ) {
          v186 = d95;
          v187 = (uint*)q34;
          v188 = q33;
          v189 = d98;
          v190 = *v3;
          v191 = *(word*)(q34+6);
          *(word*)(q34+4) -= v191;
          v192 = v186/(v190+1);
          v193 = d99;
          *v187 += v192*v191;
          v194 = q32;
          *(uint*)v188 += v189*v192;
          *(word*)(v188+4) -= v189;
          v195 = q8;
          *(uint*)v194 += v193*v192;
          *(word*)(v194+4) -= v193;
          v196 = (uint*)q31;
          v197 = d100*v192;
          *(word*)(q31+4) -= d100;
          v198 = d101;
          *v196 += v197;
          v199 = (uint*)q30;
          v200 = v198*v192;
          *(word*)(q30+4) -= v198;
          v201 = q29;
          *v199 += v200;
          LODWORD(v202) = d102*v192;
          *(word*)(v201+4) -= d102;
          *(uint*)v201 += v202;
          v203 = *v3;
          d111 = v203;
          goto LABEL_126;
        }
        v67 = d105;
        v68 = d104;
        v569 = d13;
        v2 = d8;
        v570 = d14;
        v69 = (__int64*)q14;
        v572 = d33;
        v70 = d96;
        v571 = d103;
        v573 = d106;
      }
    }
    v71 = v3+2;
    v357 = v3[3];
    q9 = (__int64)(v3+2);
    v358 = v357==1&&v353<4*v350;
    v359 = (uint*)q32;
    v360 = (uint*)q31;
    v361 = (uint*)q30;
    v362 = d101;
    v363 = (uint*)q29;
    v364 = d102;
    d11 = 1;
    v365 = (v357-127<0)+v357+v358;
    v366 = q36;
    v3[3] = v365;
    v367 = d99;
    v368 = d100;
    *(uint*)v366 += 1568;
    LODWORD(v366) = d105;
    *v359 += v367;
    v369 = (uint*)q34;
    *v360 += v368;
    v3 = (byte*)q10;
    *v361 += v362;
    *v363 += v364;
    *v369 += v366;
    *(uint*)q35 += d104;
LABEL_248:
    putc(v71[0], a1);
    LODWORD(v202) = sub_140007B80(v3);
    rc.DecodeNormalize(a2);
    v2 = d8;
  } while( d8 );
  return v202;
}

PROBE_DECL(sub_140003530)
__int64 sub_140003530(FILE* File, FILE* a2, __int64 (*a3)(FILE*, FILE*, __int64), int a4) {
  PROBE_HIT(sub_140003530);
  int v8;
  __int64 v10;
  int v11;
  int v12;
  __int64 v14;
  int v15;
  int v16;

  if( !__PPMblock_size ) return 0;

  if( d114 ) {
    d114 = 0;
    d8 = 1;
  } else {
    rc.InitEncode();
    sub_140010A40(a4);
  }

  while( 1 ) {
    sub_1400037A0(File, a2);
    if( d8 ) break;
    if( a3 ) v8 = a3(a2, File, 0); else v8 = -1;
    d8 = v8;
    //memset((void*)(blob1+0x140029540-0x1400227B0), 0, 0x400);
    memset( &blob1[0x140029540-0x1400227B0], 0, 0x400);
    memset(d81, 0, 3072);
    if( !v8 ) {
      d114 = 1;
      return 0;
    }
  }
  rc.Flush(File);

  if( a3 ) a3(a2, File, 1);

  return 1;
}

PROBE_DECL(sub_14000C310)
__int64 sub_14000C310(FILE* a1, FILE* a2, __int64 (*a3_printstats)(FILE*, FILE*, __int64), int a4) {
  PROBE_HIT(sub_14000C310);
  int v9;
  __int64 v10;
  int cnt;
  int v12;
  int v13;
  int v14;
  int v15;
  int v16;
  int v17;
  int v18;
  int v19;
  int v20;
  int v21;
  int v22;
  int v23;
  int v24;

  if( !__PPMblock_size ) return 0;

  if( d114 ) {
    d114 = 0;
    d8 = 1;
  } else {
    rc.InitDecode(a2);
    sub_140010A40(a4);
  }

  while(1) {
    sub_14000C560(a1, a2);

    if( d8 ) break;

    if( a3_printstats ) v9 = a3_printstats(a1, a2, 0); else v9 = -1;

    d8 = v9;

    v10 = 64;
    do {
      x21[v10] = 0;
      x22[v10] = 0;
      x23[v10] = 0;
      x24[v10] = 0;
      v10 -= 4;
    } while( v10*16 );

    memset(d81, 0, 3072);

    if( !v9 ) {
      d114 = 1;
      return 0;
    }
  }

  if( a3_printstats ) a3_printstats(a1, a2, 1);

  return 1;
}

PROBE_DECL(__Alloc_PPMblock)
__int64 __Alloc_PPMblock(uint memsize_mb, int a2_order, int a3) {
  PROBE_HIT(__Alloc_PPMblock);
  int v4;
  size_t memsize_b;
  v4 = a2_order;
  if( !memsize_mb ) return 0;
  if( memsize_mb>0xFFF ) return 0;
  if( a2_order<2 ) return 0;
  if( a2_order>16 ) return 0;
  if( __PPMblock_size ) return 0;
  memsize_b = memsize_mb<<20;
  __PPMblock = new char[memsize_b]; // malloc(memsize_b);
  //  __PPMblock = VAlloc<char>(memsize_b);
  // printf( "!p=%I64X size=%I64X!\n", memsize_b, qword(memsize_b) );
  if( !__PPMblock ) return 0;
  d9 = -100;
  __PPMblock_size = memsize_b;
  d2 = 0;
  d44 = a3;
  d114 = 0;
  if( v4>12 )v4 = 16<<((v4+19)&31);
  d6 = v4;
  return 1;
}
