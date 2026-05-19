

#include "rc.inc"

Rangecoder rc;

PROBE_DECL(__sub_140002AD0)
__int64 __sub_140002AD0() {
  PROBE_HIT(__sub_140002AD0);
  unsigned int v0;
  char v1;
  int v2;
  unsigned int v3;
  int v4;
  int v5;
  unsigned int v6;
  unsigned int v7;
  unsigned int v8;
  unsigned int i;
  __m128i si128;
  unsigned int v11;
  __m128i v12;
  unsigned int v13;
  unsigned int v14;
  unsigned int v15;
  unsigned int v16;
  unsigned int v17;
  unsigned int v18;
  unsigned int v19;
  unsigned int v20;
  unsigned int v21;
  unsigned int v22;
  unsigned int v23;
  unsigned int v24;
  unsigned int v25;
  unsigned int v26;
  unsigned int v27;
  __m128i v28;
  unsigned int v29;
  __m128i v30;
  __m128i v31;
  __m128i v32;
  unsigned int v33;
  __m128i v34;
  unsigned int v35;
  unsigned int j;
  int v37;
  unsigned int v38;
  unsigned int v39;
  unsigned int v40;
  unsigned int k;
  __m128i v42;
  unsigned int v43;
  unsigned int v44;
  unsigned int v45;
  unsigned int v46;
  unsigned int v47;
  unsigned int v48;
  unsigned int v49;
  unsigned int v50;
  unsigned int v51;
  unsigned int v52;
  unsigned int v53;
  unsigned int v54;
  unsigned int v55;
  unsigned int v56;
  unsigned int v57;
  __m128i v58;
  unsigned int v59;
  __m128i v60;
  __m128i v61;
  __m128i v62;
  __m128i v63;
  __m128i v64;
  unsigned int v65;
  unsigned int v66;
  __m128i v67;
  int v68;
  unsigned int m;
  unsigned int v70;
  unsigned int n;
  __m128i v72;
  __int64 v73;
  __int64 v74;
  char v75;
  int v76;
  __int64 result;
  int v78;
  unsigned int ii;
  __m128i v80;
  __int64 v81;
  __m128i v82;
  __m128i v83;
  __m128i v84;
  __m128i v85;
  __m128i v86;
  __m128i v87;
  unsigned int v88;
  __m128i v89;
  __m128i v90;
  __m128i v91;
  __m128i v92;
  __m128i v93;
  __m128i v94;
  __m128i v95;
  unsigned int v96;
  int v97;
  int v98;
  unsigned int v99;
  int v100;
  int v101;
  unsigned int v102;
  int v103;
  int v104;
  unsigned int v105;
  int v106;
  int v107;
  unsigned int v108;
  int v109;
  unsigned int v110;
  unsigned int v111;
  int v112;
  unsigned int v113;
  unsigned int v114;
  unsigned int v115;
  unsigned int v116;
  unsigned int v117;
  unsigned int v118;
  unsigned int v119;
  unsigned int v120;
  int v121;
  unsigned int v122;
  LODWORD(__xmmword_141133520) = 0x4030201;
  BYTE4(__xmmword_141133520) = 5;
  v108 = 0;
  v0 = 5;
  do {
    v1 = 2 * v108 + 7;
    v2 = 2 * v108++ + 9;
    *((_BYTE *)&__xmmword_141133520 + v0++) = v1;
    v3 = v0;
  } while(v108 < 3);
  v109 = v2;
  v4 = v2 + 1;
  v5 = v2 + 1;
  if(v0 < 0xB) {
    v6 = 11 - v0;
    v7 = ((_BYTE)v0 + (unsigned __int8)&__xmmword_141133520) & 0xF;
    if((((_BYTE)v0 + (unsigned __int8)&__xmmword_141133520) & 0xF) != 0)
      v7 = 16 - (((_BYTE)v0 + (unsigned __int8)&__xmmword_141133520) & 0xF);
    if(v6 < v7 + 16) {
      v29 = 0;
    } else {
      v8 = 0;
      for(i = v0; v8 < v7; ++v8) {
        *((_BYTE *)&__xmmword_141133520 + i) = v4;
        v4 += 3;
        ++i;
      }
      si128 = _mm_load_si128((const __m128i *)&__xmmword_140022850);
      v111 = v7;
      v119 = v0;
      v113 = 11 - v0;
      v115 = v6 - (((_BYTE)v6 - (_BYTE)v7) & 0xF);
      v11 = 3 * v7 + v109 + 1;
      v117 = v0;
      v12 = _mm_cvtsi32_si128(v11);
      v121 = v5;
      v13 = v11 + 3;
      v14 = v11 + 6;
      v15 = v11 + 9;
      v16 = v11 + 12;
      v17 = v11 + 15;
      v18 = v11 + 18;
      v19 = v11 + 21;
      v20 = v11 + 24;
      v21 = v11 + 27;
      v22 = v11 + 30;
      v23 = v11 + 33;
      v24 = v11 + 36;
      v25 = v11 + 39;
      v122 = v11 + 42;
      v26 = v11 + 45;
      v27 = v111;
      v28 = _mm_cvtsi32_si128(v13);
      v29 = v115;
      v30 = _mm_unpacklo_epi8(_mm_cvtsi32_si128(v14), _mm_cvtsi32_si128(v15));
      v5 = v121;
      v6 = v113;
      v31 = _mm_cvtsi32_si128(v16);
      v3 = v117;
      v32 = _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(v12, v28), v30), _mm_unpacklo_epi16(_mm_unpacklo_epi8(v31, _mm_cvtsi32_si128(v17)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v18), _mm_cvtsi32_si128(v19))));
      v33 = v119 + v111;
      v34 = _mm_unpacklo_epi64(v32, _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v20), _mm_cvtsi32_si128(v21)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v22), _mm_cvtsi32_si128(v23))), _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v24), _mm_cvtsi32_si128(v25)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v122), _mm_cvtsi32_si128(v26)))));
      do {
        *(__int128 *)((char *)&__xmmword_141133520 + v33) = (__int128)v34;
        v34 = _mm_add_epi8(v34, si128);
        v33 += 16;
        v27 += 16;
      } while(v27 < v115);
      v0 = v27 + v119;
      v4 = 3 * v27 + v109 + 1;
    }
    v35 = v29 + v3;
    for(j = 3 * v29 + v5; v29 < v6; ++v35) {
      *((_BYTE *)&__xmmword_141133520 + v35) = j;
      v0 = v35 + 1;
      v4 = j + 3;
      ++v29;
      j += 3;
    }
  }
  v37 = v4 + 1;
  if(v0 < 0x26) {
    v38 = 38 - v0;
    v39 = ((_BYTE)v0 + (unsigned __int8)&__xmmword_141133520) & 0xF;
    if((((_BYTE)v0 + (unsigned __int8)&__xmmword_141133520) & 0xF) != 0)
      v39 = 16 - (((_BYTE)v0 + (unsigned __int8)&__xmmword_141133520) & 0xF);
    if(v38 < v39 + 16) {
      v59 = 0;
    } else {
      v40 = 0;
      for(k = v0; v40 < v39; ++v40) {
        *((_BYTE *)&__xmmword_141133520 + k++) = v37;
        v37 += 4;
      }
      v42 = _mm_load_si128((const __m128i *)&__xmmword_140022860);
      v110 = v39;
      v120 = v0;
      v112 = v4;
      v114 = 38 - v0;
      v116 = v38 - (((_BYTE)v38 - (_BYTE)v39) & 0xF);
      v43 = 4 * v39 + v4 + 1;
      v44 = 4 * v39 + v4 + 5;
      v45 = 4 * v39 + v4 + 9;
      v46 = 4 * v39 + v4 + 13;
      v47 = 4 * v39 + v4 + 17;
      v48 = 4 * v39 + v4 + 21;
      v49 = 4 * v39 + v4 + 25;
      v50 = v43 + 28;
      v51 = v43 + 32;
      v52 = v43 + 36;
      v53 = v43 + 40;
      v54 = v43 + 44;
      v55 = v43 + 48;
      v56 = v43 + 52;
      v118 = v43 + 56;
      v57 = v43 + 60;
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
      v66 = v120 + v110;
      v67 = _mm_unpacklo_epi64(_mm_unpacklo_epi32(v61, _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v47), v62), _mm_unpacklo_epi8(v63, v64))), _mm_unpacklo_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v51), _mm_cvtsi32_si128(v52)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v53), _mm_cvtsi32_si128(v54))), _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(v55), _mm_cvtsi32_si128(v56)), _mm_unpacklo_epi8(_mm_cvtsi32_si128(v118), _mm_cvtsi32_si128(v57)))));
      do {
        *(__int128 *)((char *)&__xmmword_141133520 + v66) = (__int128)v67;
        v67 = _mm_add_epi8(v67, v42);
        v66 += 16;
        v65 += 16;
      } while(v65 < v116);
    }
    v68 = 4 * v59 + v4 + 1;
    for(m = v59 + v0; v59 < v38; ++v59) {
      *((_BYTE *)&__xmmword_141133520 + m++) = v68;
      v68 += 4;
    }
  }
  v70 = 0;
  for(n = 0; n < 0x80; ++n) {
    v70 += *((unsigned __int8 *)&__xmmword_141133520 + v70) < n + 1;
    *((_BYTE *)&__arch_hdr[12] + n) = v70;
  }
  __xmmword_140028F00[0] = 0;
  __xmmword_140028F10[0] = 0;
  __xmmword_140028F20[0] = 0;
  __xmmword_140028F30[0] = 0;
  v72 = _mm_load_si128((const __m128i *)&__xmmword_1400228D0);
  v73 = 12;
  do {
    __xmmword_140028F30[v73] = (__int128)v72;
    __xmmword_140028F20[v73] = (__int128)v72;
    __xmmword_140028F10[v73] = (__int128)v72;
    __xmmword_140028F00[v73] = (__int128)v72;
    v73 -= 4;
  } while(v73 * 16);
  __byte_140029000[0] = 1;
  v74 = 2;
  v75 = 2;
  *(char*)(blob1+ 0x140029001 -0x1400227B0) = 2;
  v76 = 1;
  result = 1;
  v78 = 1;
  for(ii = 2; ii < 0x100; ++ii) {
    __byte_140029000[v74] = v75 + 1;
    if(!--v78) {
      ++v75;
      v78 = ++v76;
    }
    v74 = ii + 1;
  }
  *(char*)(blob1+ 0x1400290FF -0x1400227B0) = 24;
  BYTE2(__xmmword_140029100[0]) = 2;
  LOWORD(__xmmword_140029100[0]) = 512;
  *(_QWORD *)((char *)__xmmword_140029100 + 3) = 0x404040404040404LL;
  *(_QWORD *)((char *)__xmmword_140029100 + 11) = 0x404040404040404LL;
  *(__int64*)(blob1+ 0x140029113 -0x1400227B0) = 0x404040404040404LL;
  *(int*)(blob1+ 0x14002911B -0x1400227B0) = 67372036;
  *(char*)(blob1+ 0x14002911F -0x1400227B0) = 4;
  LODWORD(__xmmword_140029120[0]) = 67372036;
  WORD2(__xmmword_140029120[0]) = 1028;
  v80 = _mm_load_si128((const __m128i *)&__xmmword_1400228E0);
  v81 = 12;
  do {
    __xmmword_140029130[v81] = (__int128)v80;
    __xmmword_140029120[v81] = (__int128)v80;
    *(__m128i *)((char *)&__xmmword_140029110 + v81 * 16) = v80;
    __xmmword_140029100[v81] = (__int128)v80;
    v81 -= 4;
  } while(v81 * 16);
  *(_QWORD *)((char *)__xmmword_140029120 + 6) = _mm_cvtsi128_si64(v80);
  *(_QWORD *)((char *)__xmmword_140029120 + 14) = _mm_cvtsi128_si64(v80);
  *(_QWORD *)((char *)__xmmword_140029130 + 6) = _mm_cvtsi128_si64(v80);
  HIWORD(__xmmword_140029130[0]) = 1542;
  v82 = _mm_load_si128((const __m128i *)&__xmmword_140022870);
  v83 = _mm_load_si128((const __m128i *)&__xmmword_140022880);
  v84 = _mm_load_si128((const __m128i *)&__xmmword_140022890);
  v85 = _mm_load_si128((const __m128i *)&__xmmword_1400228A0);
  v86 = _mm_load_si128((const __m128i *)&__xmmword_1400228B0);
  v87 = _mm_load_si128((const __m128i *)&__xmmword_1400228C0);
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
    __xmmword_140029200[v88++] = (__int128)_mm_slli_epi32(v89, 9u);
  } while(v88 < 4);
  v96 = 0;
  v97 = 0;
  do {
    __byte_140029240[v96] = v97 + 1;
    v98 = v96++ == (unsigned __int8)__byte_140022900[v97];
    v97 += v98;
  } while(v96 < 0x80);
  v99 = 0;
  v100 = 0;
  do {
    __byte_1400292C0[v99] = v100 + 1;
    v101 = v99++ == (unsigned __int8)__byte_140022910[v100];
    v100 += v101;
  } while(v99 < 0x100);
  v102 = 0;
  v103 = 0;
  do {
    __byte_1400293C0[v102] = v103;
    v104 = v102++ == (unsigned __int8)__byte_140022920[v103];
    v103 += v104;
  } while(v102 < 0x80);
  v105 = 0;
  v106 = 0;
  do {
    __byte_140029440[v105] = v106;
    v107 = v105++ == (unsigned __int8)__byte_140022928[v106];
    v106 += v107;
  } while(v105 < 0x100);
  return result;
}
PROBE_DECL(__sub_140014894)
void __sub_140014894(void *a1) {
  PROBE_HIT(__sub_140014894);
  __qword_1411337D0 = a1;
}
PROBE_DECL(__sub_14001A5C0)
void __sub_14001A5C0() {
  PROBE_HIT(__sub_14001A5C0); __dword_141134C28 = 0; }

PROBE_DECL(__sub_140003370)
__int64 __sub_140003370() {
  PROBE_HIT(__sub_140003370);
  __int64 v0;
  void *v1;
  unsigned __int64 v2;
  __int64 v3;
  v0 = __PPMblock_size;
  if(!__PPMblock_size)
    return 0;
  if(__dword_141133648 == 1) {
    v1 = __PPMblock;
    if(!(__dword_141133650 + __dword_14113364C)) {
      if((__qword_141133558 + (unsigned __int64)(108 * (unsigned int)(__PPMblock_size / 0x78uLL)) - __qword_141133550) / 9 <= __qword_141133560 - (__int64)__PPMblock)
        v2 = __qword_141133560 - (_QWORD)__PPMblock;
      else
        v2 = (__qword_141133558 + (unsigned __int64)(108 * (unsigned int)(__PPMblock_size / 0x78uLL)) - __qword_141133550) / 9;
      v3 = 5 * v2;
      if(__PPMblock_size > (unsigned __int64)(2 * v3 + 12))
        v0 = 2 * v3 + 12;
    }
  } else {
    v1 = __PPMblock;
  }
  __PPMblock_size = 0;
  free(v1);
  //VFree(v1);
  return (unsigned int)((unsigned __int64)(v0 + 0xFFFFF) >> 20);
}

PROBE_DECL(__sub_1400032F0)
__int64 __sub_1400032F0() {
  PROBE_HIT(__sub_1400032F0);
  __int64 result;
  __int64 v1;
  unsigned int i;
  int v3;
  if(!__PPMblock_size)
    return 0;
  v1 = 0;
  LODWORD(result) = __qword_141133560 - __qword_141133568 + __qword_141133558 + __PPMblock_size - __qword_141133550;
  for(i = 0; i < 0x26; ++i) {
    v3 = *(_DWORD *)(__qword_141133570 + 12 * v1) * 12 * *((unsigned __int8 *)&__xmmword_141133520 + v1);
    v1 = i + 1;
    result = (unsigned int)(result - v3);
  }
  return result;
}
PROBE_DECL(__sub_140012804)
char (**__sub_140012804())[4096] {
  PROBE_HIT(__sub_140012804);
  return &__off_140027060;
}
PROBE_DECL(__sub_140017A58)
void __sub_140017A58() {
  PROBE_HIT(__sub_140017A58);
  void (**i)(void);
  for(i = (void (**)(void))&__qword_140025600; i < (void (**)(void))&__qword_140025600; ++i) {
    if(*i)
      (*i)();
  }
}
PROBE_DECL(__sub_14000C0E0)
__int64 __sub_14000C0E0(unsigned __int8 *a1) {
  PROBE_HIT(__sub_14000C0E0);
  __int64 v2;
  int v3;
  int v4;
  unsigned __int8 *v5;
  unsigned __int8 *v6;
  __int64 v7;
  __int64 v8;
  unsigned __int8 *i;
  int v10;
  unsigned __int8 *j;
  unsigned int v12;
  int v13;
  unsigned int v14;
  unsigned int v15;
  int v16;
  int v17;
  int v18;
  int v19;
  unsigned int v20;
  unsigned __int8 v21;
  unsigned int v22;
  __int64 v23;
  __int16 v24;
  int v25;
  int v26;
  unsigned __int8 *v27;
  v2 = __qword_141133588;
  v3 = *a1;
  v4 = v3 + 1;
  v5 = (unsigned __int8 *)(__qword_141133588 + *((unsigned int *)a1 + 1) + 6LL * (a1[1] & 0xF));
  v6 = (unsigned __int8 *)(__qword_141133588 + *((unsigned int *)a1 + 2));
  v7 = *((unsigned int *)v6 + 1);
  v8 = *v6;
  for(i = (unsigned __int8 *)(__qword_141133588 + v7 + 6 * v8); !i[1]; i = (unsigned __int8 *)(__qword_141133588 + v7 + 6 * v8)) {
    if(*i != *v5)
      break;
    v6 = (unsigned __int8 *)(__qword_141133588 + *((unsigned int *)v6 + 2));
    v7 = *((unsigned int *)v6 + 1);
    v8 = *v6;
  }
  if(!*(_BYTE *)(v7 + __qword_141133588 + 6LL * (v6[1] & 0xF) + 1)) {
    __sub_14000C0E0(v6);
    v7 = *((unsigned int *)v6 + 1);
  }
  v10 = *v5;
  for(j = (unsigned __int8 *)(v2 + v7); *j != v10; j += 6)
    ;
  v12 = j[1];
  v13 = *((unsigned __int16 *)a1 + 1);
  v14 = v13 - v12 + 2 * v8 + 2 + *((unsigned __int16 *)v6 + 1);
  if(v12 >= 6)
    v15 = v12 - 1;
  else
    v15 = ((3 - v12) >> 30) | 1;
  v16 = 4 * v3 + 4;
  v17 = v8 + 1 - v16;
  if(v17 < -4) {
    v19 = 0;
  } else {
    v18 = v17 + 4;
    v19 = 16 * v4;
    if(v18 < 16 * v4)
      v19 = v18;
  }
  v20 = 3 * (v19 + v13 + v16) * v15 / v14;
  if(v20 >= 0x24) {
    v21 = 11;
  } else {
    v21 = __byte_1400227C0[v20];
    if(v20 <= 0x21) {
    LABEL_18:
      v5[1] = v21;
      goto LABEL_26;
    }
  }
  if(v13 >= 32 * v4)
    goto LABEL_18;
  v22 = 23;
  if(v13 >= 2 * (3 * v3 + 3))
    v22 = 7;
  if(v20 / 6 - 4 < v22)
    v22 = v20 / 6 - 4;
  v21 += v22;
  v5[1] = v21;
LABEL_26:
  v23 = *((unsigned int *)a1 + 1);
  for(*((_WORD *)a1 + 1) = v21 + (_WORD)v13; v5 != (unsigned __int8 *)(v2 + v23); v5 -= 6) {
    if(v5[1] <= *(v5 - 5) + 1)
      break;
    v24 = *(_WORD *)v5;
    v25 = *(_DWORD *)(v5 + 2);
    v26 = *((_DWORD *)v5 - 1);
    *(_WORD *)v5 = *((_WORD *)v5 - 3);
    *(_DWORD *)(v5 + 2) = v26;
    *((_WORD *)v5 - 3) = v24;
    *((_DWORD *)v5 - 1) = v25;
    v23 = *((unsigned int *)a1 + 1);
  }
  v27 = &v5[-v23 - v2];
  a1[1] = ((__int64)v27 / 6) | a1[1] & 0xF0;
  return 0x2AAAAAAAAAAAAAABLL * (_QWORD)v27;
}
PROBE_DECL(__sub_14000B750)
__int64 __sub_14000B750(unsigned __int8 *a1) {
  PROBE_HIT(__sub_14000B750);
  int v2;
  __int64 v3;
  __int64 v4;
  unsigned int v5;
  int v6;
  char v7;
  int v8;
  int v9;
  int v10;
  __int64 v11;
  __int16 v12;
  int v13;
  int v14;
  unsigned __int8 *v15;
  unsigned __int8 *v16;
  int v17;
  unsigned __int8 *i;
  int v19;
  __int64 result;
  unsigned __int8 v21;
  __int64 v22;
  __int64 v23;
  unsigned __int8 v24;
  unsigned int v25;
  __int64 v26;
  __int64 v27;
  unsigned int v28;
  __int64 v29;
  _DWORD *v30;
  __int64 v31;
  unsigned __int8 *v32;
  __int64 v33;
  unsigned int v34;
  unsigned int v35;
  int v36;
  __int64 v37;
  unsigned int v38;
  __int64 v39;
  __int64 v40;
  int v41;
  _DWORD *v42;
  __int64 v43;
  _DWORD *v44;
  int v45;
  __int64 v46;
  __int64 v47;
  unsigned int v48;
  unsigned int v49;
  unsigned int v50;
  __int64 v51;
  _DWORD *v52;
  __int64 v53;
  _DWORD *v54;
  int v55;
  __int64 v56;
  unsigned int v57;
  unsigned int v58;
  unsigned int v59;
  unsigned int v60;
  __int64 v61;
  __int64 v62;
  __int64 v63;
  unsigned int v64;
  unsigned int v65;
  int v66;
  __int64 v67;
  unsigned int v68;
  __int64 v69;
  __int64 v70;
  int v71;
  _DWORD *v72;
  __int64 v73;
  int v74;
  __int64 v75;
  __int64 v76;
  __int64 v77;
  unsigned int v78;
  __int64 v79;
  __int64 v80;
  unsigned __int8 *j;
  __int64 v82;
  unsigned int v83;
  unsigned int v84;
  int v85;
  __int64 v86;
  unsigned int v87;
  __int64 v88;
  __int64 v89;
  int v90;
  _DWORD *v91;
  __int64 v92;
  int v93;
  unsigned int v94;
  unsigned __int16 v95;
  int v96;
  v2 = *a1;
  v3 = __qword_141133588;
  v4 = *((unsigned int *)a1 + 1);
  v5 = v2 + 1;
  v6 = (unsigned __int8)(a1[1] & 0x40) >> 6;
  v7 = v6 + 1;
  v8 = 2 * v6 + 2;
  v9 = (1 << (v6 + 1)) - 1;
  v10 = -v6;
  if(__dword_141133690 != __dword_141133654)
    v10 = v8;
  v11 = __qword_141133610;
  if(__qword_141133610 != __qword_141133588 + v4) {
    do {
      v12 = *(_WORD *)v11;
      v13 = *(_DWORD *)(v11 + 2);
      v14 = *(_DWORD *)(v11 - 4);
      *(_WORD *)v11 = *(_WORD *)(v11 - 6);
      *(_DWORD *)(v11 + 2) = v14;
      *(_WORD *)(v11 - 6) = v12;
      *(_DWORD *)(v11 - 4) = v13;
      v11 -= 6;
    } while(v11 != v3 + *((unsigned int *)a1 + 1));
  }
  v15 = (unsigned __int8 *)(v11 + 6LL * v5);
  v16 = v15 - 6;
  a1[1] = 0;
  *((_WORD *)a1 + 1) = 0;
  do {
    v15 -= 6;
    v17 = (v10 + v9 * v15[1]) >> v7;
    v15[1] = v17;
    *((_WORD *)a1 + 1) += (unsigned __int8)v17;
    if(v15[1]) {
      a1[1] |= *((_BYTE *)__xmmword_140028F00 + *v15);
    } else {
      for(i = v15; i < v16; i += 6) {
        v19 = *((_DWORD *)i + 2);
        *(_WORD *)i = *((_WORD *)i + 3);
        *(_DWORD *)(i + 2) = v19;
      }
      i[1] = 0;
    }
    --v5;
  } while(v5);
  result = v16[1];
  if(v16[1]) {
    v94 = *((_DWORD *)a1 + 1);
  LABEL_61:
    a1[1] |= 0x40u;
    __qword_141133610 = v94 + v3;
    return result;
  }
  do {
    LOBYTE(v5) = v5 + 1;
    v16 -= 6;
  } while(!v16[1]);
  v21 = *a1 - v5;
  *a1 = v21;
  if(v21) {
    v46 = *((unsigned int *)a1 + 1);
    v47 = v3 + v46;
    v48 = v21 + 2;
    v49 = *((unsigned __int8 *)&__arch_hdr[11] + ((unsigned int)(v2 + 2) >> 1) + 3);
    result = (v48 >> 1) - 1;
    v50 = *((unsigned __int8 *)&__arch_hdr[12] + result);
    if(v49 != v50) {
      v51 = __qword_141133570;
      v52 = (_DWORD *)(__qword_141133570 + 12LL * v50);
      if(*v52) {
        v53 = (unsigned int)v52[1];
        v54 = (_DWORD *)(v3 + v53);
        v52[1] = *(_DWORD *)(v3 + v53 + 4);
        *(_DWORD *)(*(unsigned int *)(v3 + v53 + 4) + v3 + 8) = (_DWORD)v52 - v3;
        v55 = v3 + v53;
        --*v52;
        v56 = v3 + v46;
        if(((v48 >> 1) & 1) != 0) {
          *v54 = *(_DWORD *)v47;
          v54[1] = *(_DWORD *)(v47 + 4);
          v54[2] = *(_DWORD *)(v47 + 8);
          v54 = (_DWORD *)(v3 + v53 + 12);
          v56 = v3 + v46 + 12;
        }
        v57 = v48 >> 2;
        if(v57) {
          v58 = 0;
          v59 = 0;
          do {
            v54[v59] = *(_DWORD *)(v56 + v59 * 4);
            ++v58;
            v54[v59 + 1] = *(_DWORD *)(v56 + v59 * 4 + 4);
            v54[v59 + 2] = *(_DWORD *)(v56 + v59 * 4 + 8);
            v54[v59 + 3] = *(_DWORD *)(v56 + v59 * 4 + 12);
            v54[v59 + 4] = *(_DWORD *)(v56 + v59 * 4 + 16);
            v54[v59 + 5] = *(_DWORD *)(v56 + v59 * 4 + 20);
            v59 += 6;
          } while(v58 < v57);
        }
        v60 = *((unsigned __int8 *)&__xmmword_141133520 + v49);
        v61 = *((unsigned __int8 *)&__xmmword_141133520 + v49);
        if(*(unsigned __int8 *)(12 * v61 + v47 + 1) == 255) {
          v62 = 12 * v61;
          do {
            *(_DWORD *)(*(unsigned int *)(v62 + v47 + 4) + v3 + 8) = *(_DWORD *)(v62 + v47 + 8);
            *(_DWORD *)(*(unsigned int *)(v62 + v47 + 8) + v3 + 4) = *(_DWORD *)(v62 + v47 + 4);
            v63 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *(unsigned __int8 *)(v62 + v47) + 3);
            --*(_DWORD *)(v63 + v51);
            v60 += *(unsigned __int8 *)(v47 + v62);
            v62 = 12LL * v60;
          } while(*(unsigned __int8 *)(v62 + v47 + 1) == 255);
          v64 = v60;
          if(v60 > 0x80) {
            v65 = 0;
            v66 = 0;
            v67 = -((__int64)(((unsigned __int64)((1LL - v60) >> 6) >> 57) - v60 + 1) >> 7);
            do {
              v60 = v66 + v64 - 128;
              v66 -= 128;
              ++v65;
            } while(v65 < (unsigned int)v67);
            v68 = 0;
            do {
              *(_BYTE *)(v47 + 1) = -1;
              *(_DWORD *)(v47 + 8) = v51 - v3 + 444;
              ++v68;
              *(_BYTE *)v47 = 0x80;
              *(_DWORD *)(v47 + 4) = *(_DWORD *)(v51 + 448);
              *(_DWORD *)(*(unsigned int *)(v51 + 448) + v3 + 8) = v46;
              ++*(_DWORD *)(v51 + 444);
              *(_DWORD *)(v51 + 448) = v46;
              v46 += 1536;
              v47 += 1536;
            } while(v68 < (unsigned int)v67);
          }
          v69 = *((unsigned __int8 *)&__arch_hdr[11] + v60 + 3);
          LODWORD(v70) = *((unsigned __int8 *)&__xmmword_141133520 + v69);
          if(v60 != (_DWORD)v70) {
            v69 = (unsigned int)(v69 - 1);
            v70 = *((unsigned __int8 *)&__xmmword_141133520 + v69);
            v71 = v60 - v70;
            v72 = (_DWORD *)(v51 + 12LL * (unsigned int)(v71 - 1));
            v73 = v47 + 12 * v70;
            *(_BYTE *)(v73 + 1) = -1;
            *(_BYTE *)v73 = v71;
            *(_DWORD *)(v73 + 8) = (_DWORD)v72 - v3;
            *(_DWORD *)(v73 + 4) = v72[1];
            LODWORD(v73) = v73 - v3;
            *(_DWORD *)((unsigned int)v72[1] + v3 + 8) = v73;
            v72[1] = v73;
            ++*v72;
          }
          *(_BYTE *)(v47 + 1) = -1;
          result = 12 * v69 + v51;
          *(_BYTE *)v47 = v70;
          *(_DWORD *)(v47 + 8) = result - v3;
          *(_DWORD *)(v47 + 4) = *(_DWORD *)(result + 4);
          v74 = v47 - v3;
          *(_DWORD *)(*(unsigned int *)(result + 4) + v3 + 8) = v74;
          *(_DWORD *)(result + 4) = v74;
          ++*(_DWORD *)result;
        } else {
          *(_BYTE *)(v47 + 1) = -1;
          result = 12LL * v49 + v51;
          *(_BYTE *)v47 = v60;
          *(_DWORD *)(v47 + 8) = result - v3;
          *(_DWORD *)(v47 + 4) = *(_DWORD *)(result + 4);
          *(_DWORD *)(*(unsigned int *)(result + 4) + v3 + 8) = v46;
          *(_DWORD *)(result + 4) = v46;
          ++*(_DWORD *)result;
        }
        LODWORD(v47) = v55;
      } else {
        v75 = *((unsigned __int8 *)&__xmmword_141133520 + v50);
        v76 = v47 + 12 * v75;
        v77 = v76 - v3;
        v78 = *((unsigned __int8 *)&__xmmword_141133520 + v49) - (_DWORD)v75;
        v79 = v78;
        v80 = 12LL * v78;
        for(j = (unsigned __int8 *)(v76 + v80); *(unsigned __int8 *)(v76 + v80 + 1) == 255; j = (unsigned __int8 *)(v76 + v80)) {
          *(_DWORD *)(*((unsigned int *)j + 1) + v3 + 8) = *((_DWORD *)j + 2);
          *(_DWORD *)(*((unsigned int *)j + 2) + v3 + 4) = *((_DWORD *)j + 1);
          v82 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *j + 3);
          --*(_DWORD *)(v82 + v51);
          v78 += *(unsigned __int8 *)(v76 + v80);
          v79 = v78;
          v80 = 12LL * v78;
        }
        v83 = v78;
        if(v78 > 0x80) {
          v84 = 0;
          v85 = 0;
          v86 = -((__int64)(((unsigned __int64)((1 - v79) >> 6) >> 57) - v79 + 1) >> 7);
          do {
            v78 = v85 + v83 - 128;
            v85 -= 128;
            ++v84;
          } while(v84 < (unsigned int)v86);
          v87 = 0;
          do {
            *(_BYTE *)(v76 + 1) = -1;
            *(_DWORD *)(v76 + 8) = v51 - v3 + 444;
            ++v87;
            *(_BYTE *)v76 = 0x80;
            *(_DWORD *)(v76 + 4) = *(_DWORD *)(v51 + 448);
            *(_DWORD *)(*(unsigned int *)(v51 + 448) + v3 + 8) = v77;
            ++*(_DWORD *)(v51 + 444);
            *(_DWORD *)(v51 + 448) = v77;
            v77 += 1536;
            v76 += 1536;
          } while(v87 < (unsigned int)v86);
        }
        v88 = *((unsigned __int8 *)&__arch_hdr[11] + v78 + 3);
        LODWORD(v89) = *((unsigned __int8 *)&__xmmword_141133520 + v88);
        if(v78 != (_DWORD)v89) {
          v88 = (unsigned int)(v88 - 1);
          v89 = *((unsigned __int8 *)&__xmmword_141133520 + v88);
          v90 = v78 - v89;
          v91 = (_DWORD *)(v51 + 12LL * (unsigned int)(v90 - 1));
          v92 = v76 + 12 * v89;
          *(_BYTE *)(v92 + 1) = -1;
          *(_BYTE *)v92 = v90;
          *(_DWORD *)(v92 + 8) = (_DWORD)v91 - v3;
          *(_DWORD *)(v92 + 4) = v91[1];
          LODWORD(v92) = v92 - v3;
          *(_DWORD *)((unsigned int)v91[1] + v3 + 8) = v92;
          v91[1] = v92;
          ++*v91;
        }
        *(_BYTE *)(v76 + 1) = -1;
        result = 12 * v88 + v51;
        *(_BYTE *)v76 = v89;
        *(_DWORD *)(v76 + 8) = result - v3;
        *(_DWORD *)(v76 + 4) = *(_DWORD *)(result + 4);
        v93 = v76 - v3;
        *(_DWORD *)(*(unsigned int *)(result + 4) + v3 + 8) = v93;
        *(_DWORD *)(result + 4) = v93;
        ++*(_DWORD *)result;
      }
    }
    v94 = v47 - v3;
    *((_DWORD *)a1 + 1) = v94;
    goto LABEL_61;
  }
  v22 = *((unsigned int *)a1 + 1);
  v23 = v3 + v22;
  v95 = *(_WORD *)(v3 + v22);
  v96 = *(_DWORD *)(v3 + v22 + 2);
  v24 = (v2 + ((v95 >> 7) & 0xFFFFFFFE)) / (__int64)(v2 + 1);
  v25 = v2 + 2;
  if(v24 >= 0x2Cu)
    v24 = 44;
  HIBYTE(v95) = v24;
  v26 = __qword_141133570;
  v27 = *((unsigned __int8 *)&__arch_hdr[11] + (v25 >> 1) + 3);
  v28 = *((unsigned __int8 *)&__xmmword_141133520 + v27);
  v29 = *((unsigned __int8 *)&__xmmword_141133520 + v27);
  if(*(unsigned __int8 *)(12 * v29 + v23 + 1) == 255) {
    v31 = 12 * v29;
    v32 = (unsigned __int8 *)(v23 + v31);
    do {
      *(_DWORD *)(*((unsigned int *)v32 + 1) + v3 + 8) = *((_DWORD *)v32 + 2);
      *(_DWORD *)(*((unsigned int *)v32 + 2) + v3 + 4) = *((_DWORD *)v32 + 1);
      v33 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *v32 + 3);
      --*(_DWORD *)(v33 + v26);
      v28 += *(unsigned __int8 *)(v23 + v31);
      v31 = 12LL * v28;
      v32 = (unsigned __int8 *)(v23 + v31);
    } while(*(unsigned __int8 *)(v23 + v31 + 1) == 255);
    v34 = v28;
    if(v28 > 0x80) {
      v35 = 0;
      v36 = 0;
      v37 = -((__int64)(((unsigned __int64)((1LL - v28) >> 6) >> 57) - v28 + 1) >> 7);
      do {
        v28 = v36 + v34 - 128;
        v36 -= 128;
        ++v35;
      } while(v35 < (unsigned int)v37);
      v38 = 0;
      do {
        *(_BYTE *)(v23 + 1) = -1;
        *(_DWORD *)(v23 + 8) = v26 - v3 + 444;
        ++v38;
        *(_BYTE *)v23 = 0x80;
        *(_DWORD *)(v23 + 4) = *(_DWORD *)(v26 + 448);
        *(_DWORD *)(*(unsigned int *)(v26 + 448) + v3 + 8) = v22;
        ++*(_DWORD *)(v26 + 444);
        *(_DWORD *)(v26 + 448) = v22;
        v22 += 1536;
        v23 += 1536;
      } while(v38 < (unsigned int)v37);
    }
    v39 = *((unsigned __int8 *)&__arch_hdr[11] + v28 + 3);
    LODWORD(v40) = *((unsigned __int8 *)&__xmmword_141133520 + v39);
    if(v28 != (_DWORD)v40) {
      v39 = (unsigned int)(v39 - 1);
      v40 = *((unsigned __int8 *)&__xmmword_141133520 + v39);
      v41 = v28 - v40;
      v42 = (_DWORD *)(v26 + 12LL * (unsigned int)(v41 - 1));
      v43 = v23 + 12 * v40;
      *(_BYTE *)(v43 + 1) = -1;
      *(_BYTE *)v43 = v41;
      *(_DWORD *)(v43 + 8) = (_DWORD)v42 - v3;
      *(_DWORD *)(v43 + 4) = v42[1];
      LODWORD(v43) = v43 - v3;
      *(_DWORD *)((unsigned int)v42[1] + v3 + 8) = v43;
      v42[1] = v43;
      ++*v42;
    }
    *(_BYTE *)(v23 + 1) = -1;
    v44 = (_DWORD *)(12 * v39 + v26);
    *(_BYTE *)v23 = v40;
    *(_DWORD *)(v23 + 8) = (_DWORD)v44 - v3;
    *(_DWORD *)(v23 + 4) = v44[1];
    v45 = v23 - v3;
    *(_DWORD *)((unsigned int)v44[1] + v3 + 8) = v45;
    v44[1] = v45;
    ++*v44;
  } else {
    *(_BYTE *)(v23 + 1) = -1;
    v30 = (_DWORD *)(v26 + 12 * v27);
    *(_BYTE *)v23 = v28;
    *(_DWORD *)(v23 + 8) = (_DWORD)v30 - v3;
    *(_DWORD *)(v23 + 4) = v30[1];
    *(_DWORD *)((unsigned int)v30[1] + v3 + 8) = v22;
    v30[1] = v22;
    ++*v30;
  }
  __qword_141133610 = (__int64)(a1 + 2);
  *((_WORD *)a1 + 1) = v95;
  *((_DWORD *)a1 + 1) = v96;
  a1[1] = *((_BYTE *)__xmmword_140028F00 + (unsigned __int8)v95);
  return v95;
}
PROBE_DECL(__sub_140010730)
__int64 __sub_140010730(__int64 a1) {
  PROBE_HIT(__sub_140010730);
  unsigned int v1;
  unsigned int v2;
  unsigned int v3;
  unsigned int v4;
  unsigned int v5;
  unsigned int v6;
  unsigned int v7;
  __int64 result;
  unsigned int v9;
  __int16 v10;
  v1 = *(unsigned __int16 *)(a1 + 4);
  v2 = *(unsigned __int16 *)(a1 + 6) * (*(_DWORD *)a1 / (11 * v1) + 1);
  if(v2 >= 0x800)
    LOWORD(v2) = 2048;
  v3 = *(unsigned __int8 *)__qword_141133580 + 1;
  v4 = *(unsigned __int16 *)(a1 + 4) / v3;
  v5 = (256 - v3) * *(unsigned __int16 *)(__qword_141133580 + 2) / v3;
  v6 = 16 * v3;
  v7 = v4 * v5;
  if(*(_DWORD *)a1 < v7)
    v7 = *(_DWORD *)a1;
  if(v1 > v6)
    v6 = *(unsigned __int16 *)(a1 + 4);
  result = v7 - (v7 >> 1);
  v9 = v6 - (v6 >> 1);
  if((unsigned __int16)v2 <= 0x400u) {
    *(_WORD *)(a1 + 4) = v9;
    *(_DWORD *)a1 = result;
    if((unsigned __int16)v2 <= 0x200u) {
      if((unsigned __int16)v2 <= 0x40u) {
        result = (32 - (unsigned int)(unsigned __int16)v2) >> 31;
        v10 = v2 - result;
      } else {
        v10 = v2 - 32;
      }
      *(_WORD *)(a1 + 6) = v10;
    } else {
      *(_WORD *)(a1 + 6) = v2 - 128;
    }
  } else {
    *(_DWORD *)a1 = result - ((unsigned int)result >> 1);
    *(_WORD *)(a1 + 4) = v9 - ((unsigned __int16)v9 >> 1);
    *(_WORD *)(a1 + 6) = (unsigned __int16)v2 >> 1;
    return (unsigned __int16)v9 >> 1;
  }
  return result;
}
PROBE_DECL(__sub_140010850)
__int64 __sub_140010850(__int64 a1) {
  PROBE_HIT(__sub_140010850);
  int v2;
  int v3;
  int v4;
  unsigned int v5;
  char v6;
  unsigned int v7;
  int v8;
  unsigned int v9;
  int v10;
  unsigned int v11;
  __int64 result;
  int v13;
  unsigned int v14;
  int v15;
  BOOL v16;
  int v17;
  v2 = *(unsigned __int16 *)(a1 + 6);
  v3 = HIWORD(*(_DWORD *)a1);
  v4 = (unsigned __int16)*(_DWORD *)a1;
  v5 = *(unsigned __int16 *)(a1 + 4);
  v6 = 1;
  v7 = ((v5 >> 1) + (v4 << 15)) / v5;
  if(v7 >= 0x7FFF)
    v7 = 0x7FFF;
  if(v7 <= 1)
    v7 = 1;
  v8 = (v3 - v7) * (v3 - v7);
  v9 = (0x10000 - (v7 + v3)) * (v7 + v3);
  v10 = 4096;
  if(v2 < 4096)
    v10 = v2;
  v11 = (v2 * (v9 >> 11)) >> 4;
  if(v10 <= 24 || v10 == 4096)
    v6 = 14;
  if(2 * v8 <= 3 * v11) {
    result = v10 * (((v11 >> v6) + 5 * v8) / v11 + 1);
    if((unsigned int)result >= 0x2000)
      result = 0x2000;
  } else {
    result = 0x2000;
  }
  if((unsigned __int16)result <= 0x40u && (unsigned __int16)result <= (unsigned int)v2) {
    if((unsigned __int16)result <= 0x20u)
      LOWORD(v13) = result - ((1 - (unsigned int)(unsigned __int16)result) >> 31) - ((24 - (unsigned int)(unsigned __int16)result) >> 31);
    else
      LOWORD(v13) = result - 8;
    *(_WORD *)(a1 + 6) = v13;
  } else {
    v13 = (unsigned __int16)result >> 1;
    *(_WORD *)(a1 + 6) = v13;
  }
  if((unsigned __int16)v13 <= 2u) {
    v15 = 17 * (8 * v4 < v5);
    v16 = 8 * (v5 - v4) < v5;
    v17 = 2 * v5;
    result = (unsigned int)(1431655766 * v17);
    v14 = (-6 * v16 + v15 + 2 * v4) / 3u;
    *(_WORD *)(a1 + 4) = (unsigned)v17 / 3u;
  } else {
    *(_WORD *)(a1 + 4) = v5 >> 1;
    v14 = (unsigned int)(v4 + -12 * (6 * (v5 - v4) < v5) + 16 * (unsigned int)(4 * v4 < v5)) >> 1;
  }
  *(_DWORD *)a1 = (v7 << 16) | v14;
  return result;
}
PROBE_DECL(__sub_140011240)
__int64 __sub_140011240(unsigned int a1) {
  PROBE_HIT(__sub_140011240);
  __int64 v1;
  __int64 v2;
  unsigned int v3;
  _DWORD *v4;
  __int64 v5;
  __int64 result;
  int v7;
  __int64 v8;
  __int64 v9;
  unsigned int v10;
  __int64 v11;
  __int64 v12;
  unsigned __int8 *i;
  __int64 v14;
  unsigned int v15;
  unsigned int v16;
  int v17;
  __int64 v18;
  int v19;
  unsigned int v20;
  __int64 v21;
  __int64 v22;
  int v23;
  _DWORD *v24;
  int v25;
  __int64 v26;
  _DWORD *v27;
  int v28;
  int v29;
  __int64 v30;
  char *v31;
  __int64 v32;
  int v33;
  __int64 v34;
  unsigned int v35;
  int v36;
  _DWORD *v37;
  int v38;
  int v39;
  __int64 v40;
  char *v41;
  __int64 v42;
  unsigned int v43;
  __int64 v44;
  _DWORD *v45;
  int v46;
  int v47;
  unsigned __int8 *v48;
  __int64 v49;
  __int64 v50;
  unsigned int v51;
  unsigned int v52;
  int v53;
  __int64 v54;
  int v55;
  unsigned int v56;
  __int64 v57;
  __int64 v58;
  int v59;
  _DWORD *v60;
  unsigned __int8 *v61;
  _DWORD *v62;
  int v63;
  int v64;
  int v65;
  __int64 v66;
  _DWORD *v67;
  __int64 v68;
  bool v69;
  int v70;
  __int64 v71;
  __int64 v72;
  v1 = __qword_141133570;
  v2 = a1;
  v3 = *((unsigned __int8 *)&__xmmword_141133520 + a1);
  while(++a1 != 38) {
    v4 = (_DWORD *)(__qword_141133570 + 12LL * a1);
    if(*v4) {
      v5 = __qword_141133588;
      result = __qword_141133588 + (unsigned int)v4[1];
      v4[1] = *(_DWORD *)(result + 4);
      *(_DWORD *)(v5 + *(unsigned int *)(result + 4) + 8) = (_DWORD)v4 - v5;
      v7 = *((unsigned __int8 *)&__xmmword_141133520 + a1);
      --*v4;
      v8 = result + 12LL * v3;
      v9 = v8 - v5;
      v10 = v7 - v3;
      v11 = v10;
      v12 = 12LL * v10;
      for(i = (unsigned __int8 *)(v8 + v12); *(unsigned __int8 *)(v8 + v12 + 1) == 255; i = (unsigned __int8 *)(v8 + v12)) {
        *(_DWORD *)(*((unsigned int *)i + 1) + v5 + 8) = *((_DWORD *)i + 2);
        *(_DWORD *)(*((unsigned int *)i + 2) + v5 + 4) = *((_DWORD *)i + 1);
        v14 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *i + 3);
        --*(_DWORD *)(v14 + v1);
        v10 += *(unsigned __int8 *)(v8 + v12);
        v11 = v10;
        v12 = 12LL * v10;
      }
      v15 = v10;
      if(v10 > 0x80) {
        v16 = 0;
        v17 = 0;
        v18 = -((__int64)(((unsigned __int64)((1 - v11) >> 6) >> 57) - v11 + 1) >> 7);
        do {
          v10 = v17 + v15 - 128;
          v17 -= 128;
          ++v16;
        } while(v16 < (unsigned int)v18);
        v19 = *(_DWORD *)(v1 + 448);
        v20 = 0;
        do {
          *(_BYTE *)(v8 + 1) = -1;
          *(_DWORD *)(v8 + 8) = v1 - v5 + 444;
          *(_DWORD *)(v8 + 4) = v19;
          v19 = v9;
          *(_BYTE *)v8 = 0x80;
          v9 += 1536;
          v8 += 1536;
          *(_DWORD *)(v5 + *(unsigned int *)(v1 + 448) + 8) = v19;
          ++*(_DWORD *)(v1 + 444);
          *(_DWORD *)(v1 + 448) = v19;
          ++v20;
        } while(v20 < (unsigned int)v18);
      }
      v21 = *((unsigned __int8 *)&__arch_hdr[11] + v10 + 3);
      LODWORD(v22) = *((unsigned __int8 *)&__xmmword_141133520 + v21);
      if(v10 != (_DWORD)v22) {
        v21 = (unsigned int)(v21 - 1);
        v22 = *((unsigned __int8 *)&__xmmword_141133520 + v21);
        v23 = v10 - v22;
        v24 = (_DWORD *)(v1 + 12LL * (unsigned int)(v23 - 1));
        v25 = v24[1];
        v26 = v8 + 12 * v22;
        *(_BYTE *)v26 = v23;
        *(_BYTE *)(v26 + 1) = -1;
        *(_DWORD *)(v26 + 8) = (_DWORD)v24 - v5;
        *(_DWORD *)(v26 + 4) = v25;
        LODWORD(v26) = v26 - v5;
        *(_DWORD *)((unsigned int)v24[1] + v5 + 8) = v26;
        v24[1] = v26;
        ++*v24;
      }
      *(_BYTE *)(v8 + 1) = -1;
      v27 = (_DWORD *)(12 * v21 + v1);
      v28 = v27[1];
      *(_BYTE *)v8 = v22;
      *(_DWORD *)(v8 + 8) = (_DWORD)v27 - v5;
      *(_DWORD *)(v8 + 4) = v28;
      v29 = v8 - v5;
      *(_DWORD *)((unsigned int)v27[1] + v5 + 8) = v29;
      v27[1] = v29;
      ++*v27;
      return result;
    }
  }
  if(__dword_141133650) {
    if(__dword_14113364C)
      return 0;
    v30 = 12 * v3;
    if(__qword_141133568 - v30 <= (unsigned __int64)__qword_141133560) {
      return 0;
    } else {
      result = __qword_141133568 - v30;
      __qword_141133568 -= v30;
    }
  } else {
    v31 = (char *)__PPMblock + __PPMblock_size - 12;
    v32 = __qword_141133588;
    v70 = *((_DWORD *)v31 + 2);
    v71 = *(_QWORD *)v31;
    v33 = (_DWORD)__PPMblock + __PPMblock_size - 12 - __qword_141133588;
    v72 = v2;
    v34 = 0;
    v35 = 0;
    v36 = __qword_141133570 - __qword_141133588 + 444;
    do {
      v31[1] = -2;
      v37 = (_DWORD *)(v1 + 12 * v34);
      v38 = v37[1];
      *v31 = 1;
      v39 = v1 + 12 * v34 - v32;
      *((_DWORD *)v31 + 2) = v39;
      *((_DWORD *)v31 + 1) = v38;
      *(_DWORD *)((unsigned int)v37[1] + v32 + 8) = v33;
      v40 = (unsigned int)v37[2];
      ++*v37;
      v37[1] = v33;
      v41 = (char *)(v32 + v40);
      v37[2] = *((_DWORD *)v41 + 2);
      *(_DWORD *)(*((unsigned int *)v41 + 2) + v32 + 4) = v39;
      --*v37;
      for(; v41 != v31; --*v37) {
        v42 = *((unsigned __int8 *)&__arch_hdr[11] + (unsigned int)(unsigned __int8)*v41 + 3);
        _mm_prefetch((const char *)(*(unsigned int *)((unsigned int)v37[2] + v32 + 8) + v32 + 4), 1);
        v43 = *((unsigned __int8 *)&__xmmword_141133520 + v42);
        v44 = *((unsigned __int8 *)&__xmmword_141133520 + v42);
        if((unsigned __int8)v41[12 * v44 + 1] == 255) {
          v48 = (unsigned __int8 *)&v41[-v32];
          v49 = 12 * v44;
          do {
            *(_DWORD *)(*(unsigned int *)&v41[v49 + 4] + v32 + 8) = *(_DWORD *)&v41[v49 + 8];
            *(_DWORD *)(*(unsigned int *)&v41[v49 + 8] + v32 + 4) = *(_DWORD *)&v41[v49 + 4];
            v50 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + (unsigned __int8)v41[v49] + 3);
            --*(_DWORD *)(v50 + v1);
            v43 += (unsigned __int8)v41[v49];
            v49 = 12LL * v43;
          } while((unsigned __int8)v41[v49 + 1] == 255);
          if(v43 > 0x80) {
            v51 = v43;
            v52 = 0;
            v53 = 0;
            v54 = -((__int64)(((unsigned __int64)((1LL - v43) >> 6) >> 57) - v43 + 1) >> 7);
            do {
              v43 = v53 + v51 - 128;
              v53 -= 128;
              ++v52;
            } while(v52 < (unsigned int)v54);
            v55 = *(_DWORD *)(v1 + 448);
            v56 = 0;
            do {
              v41[1] = -1;
              *((_DWORD *)v41 + 2) = v36;
              *((_DWORD *)v41 + 1) = v55;
              v55 = (int)v48;
              *v41 = 0x80;
              v48 += 1536;
              v41 += 1536;
              *(_DWORD *)(v32 + *(unsigned int *)(v1 + 448) + 8) = v55;
              ++*(_DWORD *)(v1 + 444);
              *(_DWORD *)(v1 + 448) = v55;
              ++v56;
            } while(v56 < (unsigned int)v54);
          }
          v57 = *((unsigned __int8 *)&__arch_hdr[11] + v43 + 3);
          LODWORD(v58) = *((unsigned __int8 *)&__xmmword_141133520 + v57);
          if(v43 != (_DWORD)v58) {
            v57 = (unsigned int)(v57 - 1);
            v58 = *((unsigned __int8 *)&__xmmword_141133520 + v57);
            v59 = v43 - v58;
            v60 = (_DWORD *)(v1 + 12LL * (unsigned int)(v59 - 1));
            v61 = (unsigned __int8 *)&v41[12 * v58];
            v61[1] = -1;
            *v61 = v59;
            *((_DWORD *)v61 + 2) = v1 + 12 * (v59 - 1) - v32;
            *((_DWORD *)v61 + 1) = v60[1];
            LODWORD(v61) = (_DWORD)v61 - v32;
            *(_DWORD *)((unsigned int)v60[1] + v32 + 8) = (_DWORD)v61;
            v60[1] = (_DWORD)v61;
            ++*v60;
          }
          v41[1] = -1;
          v62 = (_DWORD *)(v1 + 12 * v57);
          v63 = v62[1];
          *v41 = v58;
          *((_DWORD *)v41 + 2) = (_DWORD)v62 - v32;
          *((_DWORD *)v41 + 1) = v63;
          v64 = (_DWORD)v41 - v32;
          *(_DWORD *)((unsigned int)v62[1] + v32 + 8) = v64;
          v62[1] = v64;
          ++*v62;
        } else {
          v41[1] = -1;
          v45 = (_DWORD *)(v1 + 12 * v42);
          v46 = v45[1];
          *v41 = v43;
          *((_DWORD *)v41 + 2) = (_DWORD)v45 - v32;
          *((_DWORD *)v41 + 1) = v46;
          v47 = (_DWORD)v41 - v32;
          *(_DWORD *)((unsigned int)v45[1] + v32 + 8) = v47;
          v45[1] = v47;
          ++*v45;
        }
        v41 = (char *)(v32 + (unsigned int)v37[2]);
        v37[2] = *((_DWORD *)v41 + 2);
        *(_DWORD *)(*((unsigned int *)v41 + 2) + v32 + 4) = v39;
      }
      v34 = ++v35;
    } while(v35 < 0x26);
    *(_QWORD *)v31 = v71;
    *((_DWORD *)v31 + 2) = v70;
    v65 = *((unsigned __int8 *)&__xmmword_141133520 + v72);
    __dword_141133650 = 1;
    v66 = *((unsigned __int8 *)&__arch_hdr[12] + (unsigned int)(v65 - 1));
    v67 = (_DWORD *)(12 * v66 + v1);
    if(*v67) {
      result = v32 + (unsigned int)v67[1];
      v67[1] = *(_DWORD *)(result + 4);
      *(_DWORD *)(*(unsigned int *)(result + 4) + v32 + 8) = (_DWORD)v67 - v32;
      --*v67;
    } else {
      result = __qword_141133558;
      v68 = 12 * (unsigned int)*((unsigned __int8 *)&__xmmword_141133520 + v66);
      v69 = __qword_141133558 + v68 == __qword_141133550;
      if(__qword_141133558 + v68 > (unsigned __int64)__qword_141133550) {
        return __sub_140011240((unsigned int)v66);
      } else {
        __qword_141133558 += v68;
        if(!v69)
          *(_DWORD *)(v68 + result) = 0;
      }
    }
  }
  return result;
}
PROBE_DECL(__sub_140010A40)
__int64 __sub_140010A40(int a1) {
  PROBE_HIT(__sub_140010A40);
  int v2;
  __int64 v3;
  __int64 result;
  unsigned int v5;
  _DWORD *v6;
  __int64 v7;
  unsigned int v8;
  char *v9;
  char *v10;
  char *v11;
  __int64 v12;
  __int64 v13;
  int v14;
  __int64 v15;
  __int64 v16;
  _DWORD *v17;
  __int64 v18;
  __int64 v19;
  bool v20;
  __int64 v21;
  int v22;
  int v23;
  __m128i si128;
  unsigned __int64 v25;
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
  __dword_141133690 = 0;
  __dword_14113370C = 0;
  v3 = 64;
  do {
    __xmmword_140029530[v3] = 0;
    __xmmword_140029520[v3] = 0;
    __xmmword_140029510[v3] = 0;
    __xmmword_140029500[v3] = 0;
    v3 -= 4;
  } while(v3 * 16);
  __dword_141133664 = 1;
  if(a1 != 1 || __dword_141133660 == -100) {
    v6 = (_DWORD*)__PPMblock;
    v7 = __PPMblock_size;
    __qword_141133570 = (__int64)__PPMblock;
    v8 = 0;
    ++__dword_141133648;
    __qword_141133560 = (__int64)__PPMblock + 456;
    v9 = (char *)__PPMblock + __PPMblock_size;
    __dword_141133650 = 0;
    __dword_14113364C = 0;
    v10 = (char *)__PPMblock + __PPMblock_size + -108 * (int)(__PPMblock_size / 0x78uLL);
    __qword_141133568 = (__int64)v10;
    __qword_141133558 = (__int64)v10;
    *(_DWORD *)v10 = 0;
    v11 = (char *)v6 - 1;
    __qword_141133588 = (__int64)v6 - 1;
    v12 = 0;
    do {
      v13 = 3 * v12;
      v14 = 12 * v12 + 1;
      v6[v13 + 2] = v14;
      v6[v13 + 1] = v14;
      v12 = ++v8;
      v6[v13] = 0;
    } while(v8 < 0x26);
    if(v9 == v10) {
      v20 = *v6 == 0;
      __qword_141133550 = (__int64)v9;
      if(v20) {
        v15 = __sub_140011240(0);
      } else {
        v15 = (__int64)v6 + (unsigned int)v6[2] - 1;
        v6[2] = *(_DWORD *)(v15 + 8);
        *(_DWORD *)&v11[*(unsigned int *)(v15 + 8) + 4] = 1;
        --*v6;
      }
    } else {
      v9 = (char *)v6 + v7 - 12;
      __qword_141133550 = (__int64)v9;
      v15 = (__int64)v9;
    }
    *(_DWORD *)(v15 + 8) = 0;
    __qword_141133638 = v15;
    *(_BYTE *)(v15 + 1) = -57;
    __qword_141133580 = v15;
    *(_WORD *)(v15 + 2) = 256;
    v16 = (unsigned __int8)__byte_140028EFF;
    __dword_1411336E0 = 255;
    *(_BYTE *)v15 = -1;
    v17 = &v6[3 * v16];
    if(*v17) {
      v18 = (__int64)v6 + (unsigned int)v17[1] - 1;
      v17[1] = *(_DWORD *)(v18 + 4);
      *(_DWORD *)&v11[*(unsigned int *)(v18 + 4) + 8] = (_DWORD)v17 - (_DWORD)v11;
      --*v17;
    } else {
      v18 = __qword_141133558;
      v19 = 12 * (unsigned int)*((unsigned __int8 *)&__xmmword_141133520 + v16);
      v20 = __qword_141133558 + v19 == (_QWORD)v9;
      if(__qword_141133558 + v19 > (unsigned __int64)v9) {
        v18 = __sub_140011240(v16);
      } else {
        __qword_141133558 += v19;
        if(!v20)
          *(_DWORD *)(v19 + v18) = 0;
      }
    }
    __qword_141133610 = v18;
    *(_DWORD *)(v15 + 4) = v18 - (_DWORD)v11;
    v21 = 0;
    v22 = 0;
    result = 0;
    __dword_141133694 = 0;
    __dword_141133698 = 0;
    __dword_141133674 = 0;
    __dword_141133678 = 0;
    __dword_14113369C = 0;
    do {
      *((_BYTE *)v6 + *(unsigned int *)(v15 + 4) + result - 1) = v22;
      *((_BYTE *)v6 + *(unsigned int *)(v15 + 4) + result) = 1;
      ++v22;
      *(_DWORD *)((char *)v6 + *(unsigned int *)(v15 + 4) + result + 1) = 0;
      ++v21;
      result += 6;
    } while(v21 < 256);
    if(a1 != 2 || __dword_141133660 == -100) {
      if(__dword_141133654 >= 11) {
        __dword_141133708 = -11;
        v23 = -11;
      } else {
        v23 = -__dword_141133654;
        __dword_141133708 = -__dword_141133654;
      }
      __dword_141133660 = v23;
      memset((int *)__byte_140029940, 0, 2316496);
      __dword_14014AD68 = 1024;
      __qword_14002A150 = (__int64)__byte_140029940;
      __dword_14014AD70 = -1;
      __dword_14014AD78 = -1;
      __dword_14014AD74 = -1;
      memset((int *)__byte_14006A158, 0x55u, 0x80000);
      memset((int *)__byte_1400EA158, 0x55u, 0x20000);
      si128 = _mm_load_si128((const __m128i *)&__xmmword_1400228F0);
      __qword_14014F1F0 = _mm_cvtsi128_si64(si128);
      v25 = 1008;
      __dword_14014F1F8 = _mm_cvtsi128_si32(si128);
      do {
        *(__m128i *)&__dword_14014EDF0[v25 / 4] = si128;
        __xmmword_14014EDE0[v25 / 0x10] = (__int128)si128;
        __xmmword_14014EDD0[v25 / 0x10] = (__int128)si128;
        __xmmword_14014EDC0[v25 / 0x10] = (__int128)si128;
        __xmmword_14014EDB0[v25 / 0x10] = (__int128)si128;
        __xmmword_14014EDA0[v25 / 0x10] = (__int128)si128;
        __xmmword_14014ED90[v25 / 0x10] = (__int128)si128;
        v25 -= 112LL;
      } while(v25);
      __dword_14014EDFC[0] = _mm_cvtsi128_si32(si128);
      memset(__dword_1401CF1FC, 0x55u, 0x40000);
      v26 = 0;
      v27 = 0;
      do {
        __byte_14010AD58[4 * v26] = v27;
        __byte_14010AD58[4 * v26 + 1] = v27;
        __byte_14010AD58[4 * v26++ + 2] = v27++;
      } while(v26 < 0x10000);
      __dword_141133688 = 1;
      memset(__dword_14025FA20, 0, 0x20000);
      memset((int *)__byte_14043FA20, 0, 0x20000);
      v28 = 0;
      v29 = 0;
      do {
        v30 = 0;
        v31 = 0;
        v32 = v29;
        if(v28) {
          do {
            v33 = __byte_1400227E4[v30++] * (v32 & 1);
            v32 >>= 1;
            v31 += v33;
          } while(v32);
        }
        v34 = 8 * v28 + 0x20000;
        v35 = 0;
        do {
          v36 = (unsigned __int8)__byte_1400227F4[v35];
          *(__int16 *)((char *)&__word_14025FA24 + v34) = 18432;
          v37 = v31 + v36;
          if(v37 >= 241)
            v37 = 241;
          if(v37 < 9)
            v37 = 9;
          ++v35;
          *(__int16 *)((char *)__word_14025FA26 + v34) = 5120;
          *(int *)((char *)__dword_14025FA20 + v34) = (v37 << 23) | (72 * v37);
          v34 += 0x20000;
        } while(v35 < 14);
        __word_14043FA26[4 * v28] = 1024;
        __word_14025FA26[4 * v28] = 1024;
        ++v29;
        ++v28;
      } while(v28 < 0x4000);
      memset((int *)__dword_1405DFC20, 0, 0x20000);
      memset((int *)__byte_14077FC20, 0, 0x20000);
      v38 = 0;
      v39 = 0;
      do {
        v40 = 0;
        v41 = 0;
        v42 = v39;
        if(v38) {
          do {
            v43 = __byte_140022804[v40++] * (v42 & 1);
            v42 >>= 1;
            v41 += v43;
          } while(v42);
        }
        v44 = 8 * v38 + 0x20000;
        v45 = 0;
        do {
          v46 = (unsigned __int8)__byte_140022814[v45];
          *(__int16 *)((char *)&__word_1405DFC24 + v44) = 18432;
          v47 = v41 + v46;
          if(v47 >= 241)
            v47 = 241;
          if(v47 < 9)
            v47 = 9;
          ++v45;
          *(__int16 *)((char *)__word_1405DFC26 + v44) = 5120;
          *(int *)((char *)&__dword_1405DFC20 + v44) = (v47 << 23) | (72 * v47);
          v44 += 0x10000;
        } while(v45 < 24);
        __word_14078FC26[4 * v38] = 1024;
        __word_14077FC26[4 * v38] = 1024;
        __word_1405EFC26[4 * v38] = 1024;
        ++v39;
        __word_1405DFC26[4 * v38++] = 1024;
      } while(v38 < 0x2000);
      v48 = 0;
      v49 = 0;
      v50 = 0;
      do {
        v51 = 0;
        v52 = v49;
        do {
          *(__int16 *)((char *)__word_141108B26 + v52) = 2048;
          *(__int16 *)((char *)__word_141108B24 + v52) = 2048;
          *(int *)((char *)__dword_141108B20 + v52) = 20480;
          v52 += 8;
          ++v51;
        } while(v51 < 1024);
        v53 = 0;
        v54 = v50;
        do {
          *(__int16 *)((char *)__word_141128C26 + v54) = 2048;
          *(__int16 *)((char *)&__word_141128C24 + v54) = 2048;
          *(int *)((char *)__dword_141128C20 + v54) = 20480;
          v54 += 8;
          ++v53;
        } while(v53 < 256);
        v49 += 0x2000;
        v50 += 2048;
        ++v48;
      } while(v48 < 16);
      v55 = 0;
      v56 = 0;
      do {
        v57 = 49 * (unsigned __int8)__byte_14002282C[v55];
        v58 = 0;
        v59 = v56;
        do {
          *(int *)((char *)__dword_14079FD20 + v59) = v57;
          v59 += 4;
          ++v58;
        } while(v58 < 128);
        v56 += 512;
        ++v55;
      } while(v55 < 5);
      v60 = 0;
      v61 = 0;
      do {
        v62 = 0;
        v63 = 48 * (unsigned __int8)__byte_140022834[v60];
        v64 = v61;
        do {
          *(int *)((char *)__dword_141130D20 + v64) = v63;
          *(int *)((char *)__dword_141130D24 + v64) = 15104;
          v64 += 8;
          ++v62;
        } while(v62 < 256);
        v61 += 2048;
        ++v60;
      } while(v60 < 5);
      __dword_1411336A0 = 0;
      for(i = 0; i < 196608; ++i) {
        __dword_14045FB20[2 * i] = 0x2000;
        __dword_14045FB24[2 * i] = 24576;
      }
      for(j = 0; j < 0x100000; ++j) {
        __dword_1407A0720[2 * j] = 0;
        __dword_1407A0724[2 * j] = 0x40000;
      }
      __dword_1411336D0 = 0;
      for(k = 0; k < 98304; ++k) {
        __dword_140FA0820[2 * k] = 0;
        __dword_140FA0824[2 * k] = 0x40000;
      }
      __dword_14113368C = 0;
      for(result = 0; result < 86016; ++result) {
        __dword_141060920[2 * result] = 0;
        __dword_141060924[2 * result] = 0x80000;
      }
    }
  } else {
    result = __qword_141133638;
    v5 = *(_DWORD *)(__qword_141133638 + 8);
    if(v5) {
      result = __qword_141133588;
      do {
        ++v2;
        v5 = *(_DWORD *)(v5 + __qword_141133588 + 8);
      } while(v5);
      __dword_141133690 = v2;
    }
    __dword_14113370C = v2;
  }
  return result;
}
PROBE_DECL(__sub_14000A2B0)
__int64 __sub_14000A2B0(int a1, unsigned __int64 a2, __int64 a3) {
  PROBE_HIT(__sub_14000A2B0);
  __int64 v3;
  unsigned int v4;
  unsigned __int64 v5;
  int v6;
  unsigned int v7;
  unsigned __int8 *v8;
  unsigned int v9;
  __int64 v10;
  int v11;
  unsigned __int8 *i;
  int v13;
  unsigned __int64 v14;
  unsigned __int64 v15;
  int v16;
  unsigned __int8 *v17;
  unsigned __int64 v18;
  int v19;
  _DWORD *v20;
  __int64 v21;
  __int64 v22;
  _DWORD *v23;
  __int64 result;
  int v25;
  int v26;
  v3 = __qword_141133588;
  v4 = *(_DWORD *)(*(_QWORD *)a2 + 2LL);
  v5 = a2;
  while(1) {
    v6 = *(_DWORD *)(*(_QWORD *)v5 + 2LL);
    if(v6 != v4) {
      LODWORD(a3) = __qword_141133588 + v6;
      goto LABEL_15;
    }
    v7 = *(_DWORD *)(a3 + 8);
    v5 += 8LL;
    if(!v7)
      goto LABEL_15;
    if(v5 >= __qword_141133620)
      break;
    a3 = __qword_141133588 + v7;
  }
  v8 = (unsigned __int8 *)__qword_141133610;
  v9 = *(_DWORD *)(a3 + 8);
  while(1) {
    v10 = v3 + v9;
    if(*(_BYTE *)v10) {
      v11 = *v8;
      for(i = (unsigned __int8 *)(v3 + *(unsigned int *)(v10 + 4)); *i != v11; i += 6)
        ;
    } else {
      i = (unsigned __int8 *)(v3 + v9 + 2);
    }
    v13 = *(_DWORD *)(i + 2);
    if(v13 != v4)
      break;
    v9 = *(_DWORD *)(v10 + 8);
    *(_QWORD *)v5 = i;
    v5 += 8LL;
    if(!v9) {
      LODWORD(a3) = v10;
      goto LABEL_15;
    }
  }
  LODWORD(a3) = v3 + v13;
LABEL_15:
  v14 = a2 + 8LL * a1;
  if(v5 == v14)
    return (unsigned int)(a3 - v3);
  v15 = v14;
  v16 = a3;
  LOBYTE(v25) = 0;
  v17 = (unsigned __int8 *)(v3 + v4);
  HIWORD(v25) = *v17;
  v26 = (_DWORD)v17 - v3 + 1;
  v18 = v5;
  BYTE1(v25) = *((_BYTE *)__xmmword_140028F00 + *v17);
  v19 = __qword_141133570 - v3;
  v20 = (_DWORD *)__qword_141133570;
  v21 = __qword_141133550;
  v22 = v3;
  while(1) {
    if(v21 == __qword_141133558) {
      if(*v20) {
        v23 = (_DWORD *)(v22 + (unsigned int)v20[2]);
        v20[2] = v23[2];
        *(_DWORD *)((unsigned int)v23[2] + v22 + 4) = v19;
        --*v20;
      } else {
        v23 = (_DWORD *)__sub_140011240(0);
      }
    } else {
      v21 -= 12;
      __qword_141133550 = v21;
      v23 = (_DWORD *)v21;
    }
    if(!v23)
      break;
    *v23 = v25;
    v23[1] = v26;
    v23[2] = v16 - v22;
    v16 = (int)v23;
    result = (unsigned int)((_DWORD)v23 - v22);
    v18 -= 8LL;
    *(_DWORD *)(*(_QWORD *)v18 + 2LL) = result;
    if(v18 == v15)
      return result;
  }
  return 0;
}
PROBE_DECL(__sub_140018048)
void __sub_140018048(__int64 a1) {
  PROBE_HIT(__sub_140018048); __qword_141133DB0 = a1; }
PROBE_DECL(__sub_140011920)
char * __sub_140011920(__int64 a1, unsigned int a2) {
  PROBE_HIT(__sub_140011920);
  __int64 v2;
  __int64 v3;
  __int64 v4;
  __int64 v5;
  __int64 v6;
  unsigned __int8 *i;
  __int64 v8;
  unsigned int v9;
  unsigned int v10;
  int v11;
  __int64 v12;
  int v13;
  unsigned int v14;
  __int64 v15;
  __int64 v16;
  int v17;
  _DWORD *v18;
  __int64 v19;
  _DWORD *v20;
  int v21;
  int v23;
  v2 = __qword_141133570;
  v3 = __qword_141133588;
  v4 = a1 - __qword_141133588;
  v5 = a2;
  v6 = 12LL * a2;
  for(i = (unsigned __int8 *)(a1 + v6); *(unsigned __int8 *)(a1 + v6 + 1) == 255; i = (unsigned __int8 *)(a1 + v6)) {
    *(_DWORD *)(*((unsigned int *)i + 1) + v3 + 8) = *((_DWORD *)i + 2);
    *(_DWORD *)(*((unsigned int *)i + 2) + v3 + 4) = *((_DWORD *)i + 1);
    v8 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *i + 3);
    --*(_DWORD *)(v8 + v2);
    a2 += *(unsigned __int8 *)(a1 + v6);
    v5 = a2;
    v6 = 12LL * a2;
  }
  v9 = a2;
  if(a2 > 0x80) {
    v10 = 0;
    v11 = 0;
    v12 = -((__int64)(((unsigned __int64)((1 - v5) >> 6) >> 57) - v5 + 1) >> 7);
    do {
      a2 = v11 + v9 - 128;
      v11 -= 128;
      ++v10;
    } while(v10 < (unsigned int)v12);
    v13 = *(_DWORD *)(v2 + 448);
    v14 = 0;
    do {
      *(_BYTE *)(a1 + 1) = -1;
      *(_DWORD *)(a1 + 8) = v2 - v3 + 444;
      *(_DWORD *)(a1 + 4) = v13;
      v13 = v4;
      *(_BYTE *)a1 = 0x80;
      v4 += 1536;
      a1 += 1536;
      *(_DWORD *)(v3 + *(unsigned int *)(v2 + 448) + 8) = v13;
      ++*(_DWORD *)(v2 + 444);
      *(_DWORD *)(v2 + 448) = v13;
      ++v14;
    } while(v14 < (unsigned int)v12);
  }
  v15 = *((unsigned __int8 *)&__arch_hdr[11] + a2 + 3);
  LODWORD(v16) = *((unsigned __int8 *)&__xmmword_141133520 + v15);
  if(a2 != (_DWORD)v16) {
    v15 = (unsigned int)(v15 - 1);
    v16 = *((unsigned __int8 *)&__xmmword_141133520 + v15);
    v17 = a2 - v16;
    v18 = (_DWORD *)(v2 + 12LL * (unsigned int)(v17 - 1));
    v19 = a1 + 12 * v16;
    *(_BYTE *)(v19 + 1) = -1;
    *(_BYTE *)v19 = v17;
    *(_DWORD *)(v19 + 8) = v2 + 12 * (v17 - 1) - v3;
    *(_DWORD *)(v19 + 4) = v18[1];
    LODWORD(v19) = v19 - v3;
    *(_DWORD *)((unsigned int)v18[1] + v3 + 8) = v19;
    v18[1] = v19;
    ++*v18;
  }
  *(_BYTE *)(a1 + 1) = -1;
  v20 = (_DWORD *)(12 * v15 + v2);
  v21 = v20[1];
  *(_BYTE *)a1 = v16;
  *(_DWORD *)(a1 + 8) = (_DWORD)v20 - v3;
  *(_DWORD *)(a1 + 4) = v21;
  v23 = a1 - v3;
  *(_DWORD *)((unsigned int)v20[1] + v3 + 8) = v23;
  v20[1] = v23;
  ++*v20;
  return (char *)v20 - v3;
}
PROBE_DECL(__sub_14000A4B0)
__int64 __sub_14000A4B0(unsigned __int8 *a1, unsigned int a2) {
  PROBE_HIT(__sub_14000A4B0);
  unsigned __int8 *v3;
  __int64 v4;
  int v5;
  unsigned __int64 v6;
  int v7;
  int v8;
  _DWORD *v9;
  _DWORD *v10;
  unsigned __int8 *v11;
  unsigned int v12;
  __int64 v13;
  __int64 v14;
  unsigned int v15;
  unsigned int v16;
  int v17;
  __int64 v18;
  unsigned int v19;
  __int64 v20;
  __int64 v21;
  int v22;
  __int64 v23;
  _DWORD *v24;
  unsigned __int8 *v25;
  _DWORD *v26;
  int v27;
  _DWORD *v29;
  int v30;
  unsigned __int8 *v31;
  unsigned int v32;
  __int64 v33;
  __int64 v34;
  unsigned int v35;
  unsigned int v36;
  int v37;
  __int64 v38;
  unsigned int v39;
  __int64 v40;
  __int64 v41;
  int v42;
  __int64 v43;
  _DWORD *v44;
  unsigned __int8 *v45;
  _DWORD *v46;
  int v47;
  unsigned __int8 v48;
  __int64 v49;
  unsigned __int8 *v50;
  __int64 v51;
  const char *v52;
  unsigned __int8 *v53;
  __int64 v54;
  int v55;
  unsigned int v56;
  unsigned int v57;
  __int64 v58;
  unsigned __int64 v59;
  unsigned __int8 *v60;
  __int16 v61;
  int v62;
  int v63;
  signed int v64;
  _DWORD *v65;
  __int64 v66;
  unsigned int v67;
  __int64 v68;
  __int64 v69;
  _DWORD *v70;
  __int64 v71;
  unsigned __int8 *v72;
  __int64 v73;
  unsigned int v74;
  unsigned int v75;
  int v76;
  __int64 v77;
  __int64 v78;
  unsigned int v79;
  __int64 v80;
  __int64 v81;
  int v82;
  _DWORD *v83;
  unsigned __int8 *v84;
  _DWORD *v85;
  int v86;
  unsigned int v87;
  unsigned int v88;
  unsigned int v89;
  __int64 v90;
  _DWORD *v91;
  __int64 v92;
  _DWORD *v93;
  unsigned __int8 *v94;
  unsigned int v95;
  unsigned int v96;
  unsigned int v97;
  unsigned int v98;
  __int64 v99;
  _DWORD *v100;
  __int64 v101;
  unsigned __int8 *v102;
  __int64 v103;
  unsigned int v104;
  unsigned int v105;
  int v106;
  __int64 v107;
  __int64 v108;
  unsigned int v109;
  __int64 v110;
  __int64 v111;
  int v112;
  _DWORD *v113;
  unsigned __int8 *v114;
  _DWORD *v115;
  int v116;
  int v117;
  unsigned __int8 v118;
  int v119;
  unsigned int v120;
  unsigned int v121;
  __int64 v122;
  __int64 v123;
  __int64 v124;
  _DWORD *v125;
  __int64 v126;
  unsigned int v127;
  unsigned int v128;
  unsigned int v129;
  unsigned int v130;
  __int64 v131;
  unsigned __int8 *v132;
  __int64 v133;
  unsigned int v134;
  unsigned int v135;
  int v136;
  __int64 v137;
  __int64 v138;
  unsigned int v139;
  __int64 v140;
  __int64 v141;
  int v142;
  _DWORD *v143;
  unsigned __int8 *v144;
  _DWORD *v145;
  int v146;
  char *v147;
  char *v148;
  __int64 v149;
  __int64 v150;
  unsigned int v151;
  __int64 v152;
  _DWORD *v153;
  __int64 v154;
  unsigned __int8 *v155;
  __int64 v156;
  unsigned int v157;
  unsigned int v158;
  int v159;
  __int64 v160;
  __int64 v161;
  unsigned int v162;
  __int64 v163;
  __int64 v164;
  int v165;
  __int64 v166;
  _DWORD *v167;
  unsigned __int8 *v168;
  _DWORD *v169;
  int v170;
  __int64 v171;
  __int64 v172;
  v3 = a1;
  v4 = __qword_141133588;
  v5 = *a1;
  if(*a1) {
    v48 = a1[1];
    v172 = *((unsigned int *)a1 + 1);
    v49 = (unsigned int)(v5 + 1);
    v50 = (unsigned __int8 *)(__qword_141133588 + v172);
    v51 = (unsigned int)(2 * v5 + 2);
    v52 = (const char *)(v51 + __qword_141133588 + v172);
    _mm_prefetch((const char *)(__qword_141133588 + v172 + v49), 1);
    _mm_prefetch(v52, 1);
    _mm_prefetch(&v52[v49], 1);
    _mm_prefetch(&v52[v51], 1);
    _mm_prefetch(&v52[v51 + v49], 1);
    _mm_prefetch(&v52[v51 - 1 + v51], 1);
    v53 = v50;
    v54 = (__int64)v50;
    v55 = v5;
    v56 = v5 + 2;
    v57 = (unsigned int)(v5 + 2) >> 1;
    v58 = 3LL * (v48 & 0xF);
    if(!v50[2 * v58 + 1]) {
      v50[2 * v58 + 1] = 8;
      *((_WORD *)v3 + 1) += 8;
    }
    v59 = __qword_141133568;
    v60 = v50;
    do {
      if(v59 > v4 + (unsigned __int64)*(unsigned int *)(v53 + 2)) {
        *(_DWORD *)(v53 + 2) = 0;
        --v55;
      } else {
        if(v53 != v60) {
          v61 = *(_WORD *)v60;
          v62 = *(_DWORD *)(v60 + 2);
          *(_WORD *)v60 = *(_WORD *)v53;
          *(_DWORD *)(v60 + 2) = *(_DWORD *)(v53 + 2);
          *(_WORD *)v53 = v61;
          *(_DWORD *)(v53 + 2) = v62;
        }
        _mm_prefetch((const char *)(*(unsigned int *)(v60 + 2) + v4), 1);
        v60 += 6;
      }
      v53 += 6;
      --v5;
    } while(v5 >= 0);
    if(v55 == *v3 || !a2) {
      v122 = *((unsigned __int8 *)&__arch_hdr[11] + v57 + 3);
      v123 = *((unsigned __int8 *)&__xmmword_141133520 + v122);
      if(v50[12 * v123 + 1] == 255 && (v124 = __qword_141133570, v125 = (_DWORD *)(__qword_141133570 + 12 * v122), *v125)) {
        v171 = (unsigned int)v125[1];
        v65 = (_DWORD *)(v4 + v171);
        v125[1] = *(_DWORD *)(v4 + v171 + 4);
        *(_DWORD *)(*(unsigned int *)(v4 + v171 + 4) + v4 + 8) = (_DWORD)v125 - v4;
        --*v125;
        v126 = v4 + v171;
        if((v57 & 1) != 0) {
          *v65 = *(_DWORD *)v50;
          v65[1] = *((_DWORD *)v50 + 1);
          v65[2] = *((_DWORD *)v50 + 2);
          v126 = v4 + v171 + 12;
          v54 = v4 + v172 + 12;
        }
        v127 = v56 >> 2;
        if(v127) {
          v128 = 0;
          v129 = 0;
          do {
            *(_DWORD *)(v129 + v126) = *(_DWORD *)(v54 + v129);
            ++v128;
            *(_DWORD *)(v129 + v126 + 4) = *(_DWORD *)(v54 + v129 + 4);
            *(_DWORD *)(v129 + v126 + 8) = *(_DWORD *)(v54 + v129 + 8);
            *(_DWORD *)(v129 + v126 + 12) = *(_DWORD *)(v54 + v129 + 12);
            *(_DWORD *)(v129 + v126 + 16) = *(_DWORD *)(v54 + v129 + 16);
            *(_DWORD *)(v129 + v126 + 20) = *(_DWORD *)(v54 + v129 + 20);
            v129 += 24;
          } while(v128 < v127);
        }
        v130 = v123;
        if(v50[12 * v123 + 1] == 255) {
          v131 = 12 * v123;
          v132 = &v50[12 * v123];
          do {
            *(_DWORD *)(*((unsigned int *)v132 + 1) + v4 + 8) = *((_DWORD *)v132 + 2);
            *(_DWORD *)(*((unsigned int *)v132 + 2) + v4 + 4) = *((_DWORD *)v132 + 1);
            v133 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *v132 + 3);
            --*(_DWORD *)(v133 + v124);
            v130 += v50[v131];
            v123 = v130;
            v131 = 12LL * v130;
            v132 = &v50[v131];
          } while(v50[v131 + 1] == 255);
        }
        v134 = v130;
        if(v130 > 0x80) {
          v135 = 0;
          v136 = 0;
          v137 = -((__int64)(((unsigned __int64)((1 - v123) >> 6) >> 57) - v123 + 1) >> 7);
          do {
            v130 = v136 + v134 - 128;
            v136 -= 128;
            ++v135;
          } while(v135 < (unsigned int)v137);
          v138 = v172;
          v139 = 0;
          do {
            v50[1] = -1;
            *((_DWORD *)v50 + 2) = v124 - v4 + 444;
            ++v139;
            *v50 = 0x80;
            *((_DWORD *)v50 + 1) = *(_DWORD *)(v124 + 448);
            *(_DWORD *)(*(unsigned int *)(v124 + 448) + v4 + 8) = v138;
            ++*(_DWORD *)(v124 + 444);
            *(_DWORD *)(v124 + 448) = v138;
            v138 += 1536;
            v50 += 1536;
          } while(v139 < (unsigned int)v137);
        }
        v140 = *((unsigned __int8 *)&__arch_hdr[11] + v130 + 3);
        LODWORD(v141) = *((unsigned __int8 *)&__xmmword_141133520 + v140);
        if(v130 != (_DWORD)v141) {
          v140 = (unsigned int)(v140 - 1);
          v141 = *((unsigned __int8 *)&__xmmword_141133520 + v140);
          v142 = v130 - v141;
          v143 = (_DWORD *)(v124 + 12LL * (unsigned int)(v142 - 1));
          v144 = &v50[12 * v141];
          *v144 = v142;
          v144[1] = -1;
          *((_DWORD *)v144 + 2) = (_DWORD)v143 - v4;
          *((_DWORD *)v144 + 1) = v143[1];
          LODWORD(v144) = (_DWORD)v144 - v4;
          *(_DWORD *)((unsigned int)v143[1] + v4 + 8) = (_DWORD)v144;
          ++*v143;
          v143[1] = (_DWORD)v144;
        }
        v50[1] = -1;
        v145 = (_DWORD *)(12 * v140 + v124);
        *v50 = v141;
        *((_DWORD *)v50 + 2) = (_DWORD)v145 - v4;
        *((_DWORD *)v50 + 1) = v145[1];
        v146 = (_DWORD)v50 - v4;
        *(_DWORD *)((unsigned int)v145[1] + v4 + 8) = v146;
        v145[1] = v146;
        ++*v145;
      } else {
        v65 = (_DWORD *)v50;
      }
      *((_DWORD *)v3 + 1) = (_DWORD)v65 - v4;
    } else {
      *v3 = v55;
      if(v55 < 0) {
        v150 = *((unsigned __int8 *)&__arch_hdr[11] + v57 + 3);
        v151 = *((unsigned __int8 *)&__xmmword_141133520 + v150);
        v152 = *((unsigned __int8 *)&__xmmword_141133520 + v150);
        v29 = (_DWORD *)__qword_141133570;
        if(v50[12 * v152 + 1] == 255) {
          v154 = 12 * v152;
          v155 = &v50[12 * v152];
          do {
            *(_DWORD *)(*((unsigned int *)v155 + 1) + v4 + 8) = *((_DWORD *)v155 + 2);
            *(_DWORD *)(*((unsigned int *)v155 + 2) + v4 + 4) = *((_DWORD *)v155 + 1);
            v156 = *((unsigned __int8 *)&__arch_hdr[11] + *v155 + 3);
            --v29[3 * v156];
            v151 += v50[v154];
            v154 = 12LL * v151;
            v155 = &v50[v154];
          } while(v50[v154 + 1] == 255);
          v157 = v151;
          if(v151 > 0x80) {
            v158 = 0;
            v159 = 0;
            v160 = -((__int64)(((unsigned __int64)((1LL - v151) >> 6) >> 57) - v151 + 1) >> 7);
            do {
              v151 = v159 + v157 - 128;
              v159 -= 128;
              ++v158;
            } while(v158 < (unsigned int)v160);
            v161 = v172;
            v162 = 0;
            do {
              v50[1] = -1;
              *((_DWORD *)v50 + 2) = (_DWORD)v29 - v4 + 444;
              ++v162;
              *v50 = 0x80;
              *((_DWORD *)v50 + 1) = v29[112];
              *(_DWORD *)((unsigned int)v29[112] + v4 + 8) = v161;
              ++v29[111];
              v29[112] = v161;
              v161 += 1536;
              v50 += 1536;
            } while(v162 < (unsigned int)v160);
          }
          v163 = *((unsigned __int8 *)&__arch_hdr[11] + v151 + 3);
          LODWORD(v164) = *((unsigned __int8 *)&__xmmword_141133520 + v163);
          if(v151 != (_DWORD)v164) {
            v163 = (unsigned int)(v163 - 1);
            v164 = *((unsigned __int8 *)&__xmmword_141133520 + v163);
            v165 = v151 - v164;
            v166 = (unsigned int)(v165 - 1);
            v167 = &v29[3 * v166];
            v168 = &v50[12 * v164];
            *v168 = v165;
            v168[1] = -1;
            *((_DWORD *)v168 + 2) = (_DWORD)v29 + 12 * v166 - v4;
            *((_DWORD *)v168 + 1) = v167[1];
            LODWORD(v168) = (_DWORD)v168 - v4;
            *(_DWORD *)((unsigned int)v167[1] + v4 + 8) = (_DWORD)v168;
            ++*v167;
            v167[1] = (_DWORD)v168;
          }
          v50[1] = -1;
          v169 = &v29[3 * v163];
          *v50 = v164;
          *((_DWORD *)v50 + 2) = (_DWORD)v29 + 12 * v163 - v4;
          *((_DWORD *)v50 + 1) = v169[1];
          v170 = (_DWORD)v50 - v4;
          *(_DWORD *)((unsigned int)v169[1] + v4 + 8) = v170;
          v169[1] = v170;
          ++*v169;
        } else {
          v50[1] = -1;
          v153 = &v29[3 * v150];
          *v50 = v151;
          *((_DWORD *)v50 + 2) = (_DWORD)v29 + 12 * v150 - v4;
          *((_DWORD *)v50 + 1) = v153[1];
          *(_DWORD *)((unsigned int)v153[1] + v4 + 8) = v172;
          v153[1] = v172;
          ++*v153;
        }
        goto LABEL_23;
      }
      if(v55) {
        v87 = (v55 + 2) >> 1;
        v88 = *((unsigned __int8 *)&__arch_hdr[11] + v57 + 3);
        v89 = *((unsigned __int8 *)&__arch_hdr[11] + v87 + 3);
        if(v88 != v89) {
          v90 = __qword_141133570;
          v91 = (_DWORD *)(__qword_141133570 + 12LL * *((unsigned __int8 *)&__arch_hdr[11] + v87 + 3));
          if(*v91) {
            v92 = (unsigned int)v91[1];
            v93 = (_DWORD *)(v4 + v92);
            v91[1] = *(_DWORD *)(v4 + v92 + 4);
            *(_DWORD *)(*(unsigned int *)(v4 + v92 + 4) + v4 + 8) = (_DWORD)v91 - v4;
            --*v91;
            v94 = (unsigned __int8 *)(v4 + v92);
            if((v87 & 1) != 0) {
              *v93 = *(_DWORD *)v50;
              v93[1] = *((_DWORD *)v50 + 1);
              v93[2] = *((_DWORD *)v50 + 2);
              v93 = (_DWORD *)(v4 + v92 + 12);
              v54 = v4 + v172 + 12;
            }
            v95 = v87 >> 1;
            if(v95) {
              v96 = 0;
              v97 = 0;
              do {
                v93[v97] = *(_DWORD *)(v54 + v97 * 4);
                ++v96;
                v93[v97 + 1] = *(_DWORD *)(v54 + v97 * 4 + 4);
                v93[v97 + 2] = *(_DWORD *)(v54 + v97 * 4 + 8);
                v93[v97 + 3] = *(_DWORD *)(v54 + v97 * 4 + 12);
                v93[v97 + 4] = *(_DWORD *)(v54 + v97 * 4 + 16);
                v93[v97 + 5] = *(_DWORD *)(v54 + v97 * 4 + 20);
                v97 += 6;
              } while(v96 < v95);
            }
            v98 = *((unsigned __int8 *)&__xmmword_141133520 + v88);
            v99 = *((unsigned __int8 *)&__xmmword_141133520 + v88);
            if(v50[12 * v99 + 1] == 255) {
              v101 = 12 * v99;
              v102 = &v50[12 * v99];
              do {
                *(_DWORD *)(*((unsigned int *)v102 + 1) + v4 + 8) = *((_DWORD *)v102 + 2);
                *(_DWORD *)(*((unsigned int *)v102 + 2) + v4 + 4) = *((_DWORD *)v102 + 1);
                v103 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *v102 + 3);
                --*(_DWORD *)(v103 + v90);
                v98 += v50[v101];
                v101 = 12LL * v98;
                v102 = &v50[v101];
              } while(v50[v101 + 1] == 255);
              v104 = v98;
              if(v98 > 0x80) {
                v105 = 0;
                v106 = 0;
                v107 = -((__int64)(((unsigned __int64)((1LL - v98) >> 6) >> 57) - v98 + 1) >> 7);
                do {
                  v98 = v106 + v104 - 128;
                  v106 -= 128;
                  ++v105;
                } while(v105 < (unsigned int)v107);
                v108 = v172;
                v109 = 0;
                do {
                  v50[1] = -1;
                  *((_DWORD *)v50 + 2) = v90 - v4 + 444;
                  ++v109;
                  *v50 = 0x80;
                  *((_DWORD *)v50 + 1) = *(_DWORD *)(v90 + 448);
                  *(_DWORD *)(*(unsigned int *)(v90 + 448) + v4 + 8) = v108;
                  ++*(_DWORD *)(v90 + 444);
                  *(_DWORD *)(v90 + 448) = v108;
                  v108 += 1536;
                  v50 += 1536;
                } while(v109 < (unsigned int)v107);
              }
              v110 = *((unsigned __int8 *)&__arch_hdr[11] + v98 + 3);
              LODWORD(v111) = *((unsigned __int8 *)&__xmmword_141133520 + v110);
              if(v98 != (_DWORD)v111) {
                v110 = (unsigned int)(v110 - 1);
                v111 = *((unsigned __int8 *)&__xmmword_141133520 + v110);
                v112 = v98 - v111;
                v113 = (_DWORD *)(v90 + 12LL * (unsigned int)(v112 - 1));
                v114 = &v50[12 * v111];
                *v114 = v112;
                v114[1] = -1;
                *((_DWORD *)v114 + 2) = (_DWORD)v113 - v4;
                *((_DWORD *)v114 + 1) = v113[1];
                LODWORD(v114) = (_DWORD)v114 - v4;
                *(_DWORD *)((unsigned int)v113[1] + v4 + 8) = (_DWORD)v114;
                ++*v113;
                v113[1] = (_DWORD)v114;
              }
              v50[1] = -1;
              v115 = (_DWORD *)(12 * v110 + v90);
              *v50 = v111;
              *((_DWORD *)v50 + 2) = (_DWORD)v115 - v4;
              *((_DWORD *)v50 + 1) = v115[1];
              v116 = (_DWORD)v50 - v4;
              *(_DWORD *)((unsigned int)v115[1] + v4 + 8) = v116;
              v115[1] = v116;
              ++*v115;
            } else {
              v50[1] = -1;
              v100 = (_DWORD *)(12LL * v88 + v90);
              *v50 = v98;
              *((_DWORD *)v50 + 2) = (_DWORD)v100 - v4;
              *((_DWORD *)v50 + 1) = v100[1];
              *(_DWORD *)((unsigned int)v100[1] + v4 + 8) = v172;
              v100[1] = v172;
              ++*v100;
            }
            v50 = v94;
          } else {
            __sub_140011920((__int64)&v50[12 * *((unsigned __int8 *)&__xmmword_141133520 + v89)], *((unsigned __int8 *)&__xmmword_141133520 + v88) - *((unsigned __int8 *)&__xmmword_141133520 + v89));
          }
        }
        v117 = *((unsigned __int16 *)v3 + 1);
        v118 = v3[1];
        v65 = (_DWORD *)v50;
        *((_DWORD *)v3 + 1) = (_DWORD)v50 - v4;
        v119 = v117 > 16 * v55;
        v3[1] = (((_BYTE)v119 << 6) & v118) + *((_BYTE *)__xmmword_140028F00 + *v50);
        v120 = (v119 + (unsigned int)v50[1]) >> v119;
        v50[1] = v120;
        *((_WORD *)v3 + 1) = (unsigned __int8)v120;
        do {
          v50 += 6;
          v3[1] |= *((_BYTE *)__xmmword_140028F00 + *v50);
          v121 = (v119 + (unsigned int)v50[1]) >> v119;
          v50[1] = v121;
          *((_WORD *)v3 + 1) += (unsigned __int8)v121;
          --v55;
        } while(v55);
        v55 = *v3;
      } else {
        v3[1] = *((_BYTE *)__xmmword_140028F00 + *v53);
        v63 = v50[1];
        v64 = *((unsigned __int16 *)v3 + 1) - (v63 - 1) + 19;
        if(v64 < 2 * v63 - 2)
          LOBYTE(v64) = (v63 - 1) / v64 + 2;
        else
          v64 = (unsigned int)(v64 - (3 * v63 - 3)) >> 31;
        v50[1] = v64 + 1;
        v65 = (_DWORD *)(v3 + 2);
        *((_WORD *)v3 + 1) = *(_WORD *)v50;
        *((_DWORD *)v3 + 1) = *(_DWORD *)(v50 + 2);
        v66 = *((unsigned __int8 *)&__arch_hdr[11] + v57 + 3);
        v67 = *((unsigned __int8 *)&__xmmword_141133520 + v66);
        v68 = __qword_141133570;
        v69 = *((unsigned __int8 *)&__xmmword_141133520 + v66);
        if(v50[12 * v69 + 1] == 255) {
          v71 = 12 * v69;
          v72 = &v50[12 * v69];
          do {
            *(_DWORD *)(*((unsigned int *)v72 + 1) + v4 + 8) = *((_DWORD *)v72 + 2);
            *(_DWORD *)(*((unsigned int *)v72 + 2) + v4 + 4) = *((_DWORD *)v72 + 1);
            v73 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *v72 + 3);
            --*(_DWORD *)(v73 + v68);
            v67 += v50[v71];
            v71 = 12LL * v67;
            v72 = &v50[v71];
          } while(v50[v71 + 1] == 255);
          v74 = v67;
          if(v67 > 0x80) {
            v75 = 0;
            v76 = 0;
            v77 = -((__int64)(((unsigned __int64)((1LL - v67) >> 6) >> 57) - v67 + 1) >> 7);
            do {
              v67 = v76 + v74 - 128;
              v76 -= 128;
              ++v75;
            } while(v75 < (unsigned int)v77);
            v78 = v172;
            v79 = 0;
            do {
              v50[1] = -1;
              *((_DWORD *)v50 + 2) = v68 - v4 + 444;
              ++v79;
              *v50 = 0x80;
              *((_DWORD *)v50 + 1) = *(_DWORD *)(v68 + 448);
              *(_DWORD *)(*(unsigned int *)(v68 + 448) + v4 + 8) = v78;
              ++*(_DWORD *)(v68 + 444);
              *(_DWORD *)(v68 + 448) = v78;
              v78 += 1536;
              v50 += 1536;
            } while(v79 < (unsigned int)v77);
          }
          v80 = *((unsigned __int8 *)&__arch_hdr[11] + v67 + 3);
          LODWORD(v81) = *((unsigned __int8 *)&__xmmword_141133520 + v80);
          if(v67 != (_DWORD)v81) {
            v80 = (unsigned int)(v80 - 1);
            v81 = *((unsigned __int8 *)&__xmmword_141133520 + v80);
            v82 = v67 - v81;
            v83 = (_DWORD *)(v68 + 12LL * (unsigned int)(v82 - 1));
            v84 = &v50[12 * v81];
            *v84 = v82;
            v84[1] = -1;
            *((_DWORD *)v84 + 2) = (_DWORD)v83 - v4;
            *((_DWORD *)v84 + 1) = v83[1];
            LODWORD(v84) = (_DWORD)v84 - v4;
            *(_DWORD *)((unsigned int)v83[1] + v4 + 8) = (_DWORD)v84;
            ++*v83;
            v83[1] = (_DWORD)v84;
          }
          v50[1] = -1;
          v85 = (_DWORD *)(12 * v80 + v68);
          *v50 = v81;
          *((_DWORD *)v50 + 2) = (_DWORD)v85 - v4;
          *((_DWORD *)v50 + 1) = v85[1];
          v86 = (_DWORD)v50 - v4;
          *(_DWORD *)((unsigned int)v85[1] + v4 + 8) = v86;
          v85[1] = v86;
          ++*v85;
        } else {
          v50[1] = -1;
          v70 = (_DWORD *)(v68 + 12 * v66);
          *v50 = v67;
          *((_DWORD *)v50 + 2) = v68 + 12 * v66 - v4;
          *((_DWORD *)v50 + 1) = v70[1];
          *(_DWORD *)((unsigned int)v70[1] + v4 + 8) = v172;
          v70[1] = v172;
          ++*v70;
        }
      }
    }
    v7 = __dword_141133654;
    v147 = (char *)v65 + 6 * v55;
    v148 = v147 - 6;
    if(a2 < __dword_141133654) {
      while(1) {
        v149 = v4 + *(unsigned int *)(v147 + 2);
        _mm_prefetch((const char *)(*(unsigned int *)(v149 + 4) + v4), 1);
        *(_DWORD *)(v147 + 2) = __sub_14000A4B0(v149, a2 + 1);
        if(--v55 < 0)
          break;
        v147 = v148;
        v148 -= 6;
      }
    } else {
      *(_DWORD *)(v147 + 2) = 0;
      while(--v55 >= 0) {
        *(_DWORD *)(v148 + 2) = 0;
        v148 -= 6;
      }
    }
    goto LABEL_7;
  }
  v6 = __qword_141133588 + *((unsigned int *)a1 + 1);
  if(v6 >= __qword_141133568) {
    v7 = __dword_141133654;
    if(a2 < __dword_141133654) {
      _mm_prefetch((const char *)(*(unsigned int *)(v6 + 4) + __qword_141133588), 1);
      v8 = __sub_14000A4B0(v6, a2 + 1);
      *((_DWORD *)v3 + 1) = v8;
      if(v8)
        goto LABEL_7;
    } else {
      *((_DWORD *)v3 + 1) = 0;
    }
    if(a2 < 9) {
    LABEL_7:
      if(a2 == v7 && v3[13] == 255) {
        v9 = (_DWORD *)__qword_141133570;
        if(*(_DWORD *)__qword_141133570) {
          v10 = (_DWORD *)(v4 + *(unsigned int *)(__qword_141133570 + 8));
          *(_DWORD *)(__qword_141133570 + 8) = v10[2];
          *(_DWORD *)((unsigned int)v10[2] + v4 + 4) = (_DWORD)v9 - v4;
          v11 = &v3[-v4];
          v12 = 1;
          --*v9;
          *v10 = *(_DWORD *)v3;
          v10[1] = *((_DWORD *)v3 + 1);
          v10[2] = *((_DWORD *)v3 + 2);
          if(v3[13] == 255) {
            v13 = 12;
            do {
              *(_DWORD *)(*(unsigned int *)&v3[v13 + 4] + v4 + 8) = *(_DWORD *)&v3[v13 + 8];
              *(_DWORD *)(*(unsigned int *)&v3[v13 + 8] + v4 + 4) = *(_DWORD *)&v3[v13 + 4];
              v14 = *((unsigned __int8 *)&__arch_hdr[11] + v3[v13] + 3);
              --v9[3 * v14];
              v12 += v3[v13];
              v13 = 12LL * v12;
            } while(v3[v13 + 1] == 255);
            v15 = v12;
            if(v12 > 0x80) {
              v16 = 0;
              v17 = 0;
              v18 = -((__int64)(((unsigned __int64)((1LL - v12) >> 6) >> 57) - v12 + 1) >> 7);
              do {
                v12 = v17 + v15 - 128;
                v17 -= 128;
                ++v16;
              } while(v16 < (unsigned int)v18);
              v19 = 0;
              do {
                v3[1] = -1;
                *((_DWORD *)v3 + 2) = (_DWORD)v9 - v4 + 444;
                ++v19;
                *v3 = 0x80;
                *((_DWORD *)v3 + 1) = v9[112];
                *(_DWORD *)((unsigned int)v9[112] + v4 + 8) = (_DWORD)v11;
                ++v9[111];
                v9[112] = (_DWORD)v11;
                v11 += 1536;
                v3 += 1536;
              } while(v19 < (unsigned int)v18);
            }
          }
          v20 = *((unsigned __int8 *)&__arch_hdr[11] + v12 + 3);
          LODWORD(v21) = *((unsigned __int8 *)&__xmmword_141133520 + v20);
          if(v12 != (_DWORD)v21) {
            v20 = (unsigned int)(v20 - 1);
            v21 = *((unsigned __int8 *)&__xmmword_141133520 + v20);
            v22 = v12 - v21;
            v23 = (unsigned int)(v22 - 1);
            v24 = &v9[3 * v23];
            v25 = &v3[12 * v21];
            *v25 = v22;
            v25[1] = -1;
            *((_DWORD *)v25 + 2) = (_DWORD)v9 + 12 * v23 - v4;
            *((_DWORD *)v25 + 1) = v24[1];
            LODWORD(v25) = (_DWORD)v25 - v4;
            *(_DWORD *)((unsigned int)v24[1] + v4 + 8) = (_DWORD)v25;
            ++*v24;
            v24[1] = (_DWORD)v25;
          }
          v3[1] = -1;
          v26 = &v9[3 * v20];
          *v3 = v21;
          *((_DWORD *)v3 + 2) = (_DWORD)v26 - v4;
          *((_DWORD *)v3 + 1) = v26[1];
          v27 = (_DWORD)v3 - v4;
          *(_DWORD *)((unsigned int)v26[1] + v4 + 8) = v27;
          v26[1] = v27;
          ++*v26;
          LODWORD(v3) = (_DWORD)v10;
        }
      }
      return (unsigned int)((_DWORD)v3 - v4);
    }
  }
  v29 = (_DWORD *)__qword_141133570;
LABEL_23:
  if(v3[13] == 255) {
    v31 = &v3[-v4];
    v32 = 1;
    v33 = 12;
    do {
      *(_DWORD *)(*(unsigned int *)&v3[v33 + 4] + v4 + 8) = *(_DWORD *)&v3[v33 + 8];
      *(_DWORD *)(*(unsigned int *)&v3[v33 + 8] + v4 + 4) = *(_DWORD *)&v3[v33 + 4];
      v34 = *((unsigned __int8 *)&__arch_hdr[11] + v3[v33] + 3);
      --v29[3 * v34];
      v32 += v3[v33];
      v33 = 12LL * v32;
    } while(v3[v33 + 1] == 255);
    v35 = v32;
    if(v32 > 0x80) {
      v36 = 0;
      v37 = 0;
      v38 = -((__int64)(((unsigned __int64)((1LL - v32) >> 6) >> 57) - v32 + 1) >> 7);
      do {
        v32 = v37 + v35 - 128;
        v37 -= 128;
        ++v36;
      } while(v36 < (unsigned int)v38);
      v39 = 0;
      do {
        v3[1] = -1;
        *((_DWORD *)v3 + 2) = (_DWORD)v29 - v4 + 444;
        ++v39;
        *v3 = 0x80;
        *((_DWORD *)v3 + 1) = v29[112];
        *(_DWORD *)((unsigned int)v29[112] + v4 + 8) = (_DWORD)v31;
        ++v29[111];
        v29[112] = (_DWORD)v31;
        v31 += 1536;
        v3 += 1536;
      } while(v39 < (unsigned int)v38);
    }
    v40 = *((unsigned __int8 *)&__arch_hdr[11] + v32 + 3);
    LODWORD(v41) = *((unsigned __int8 *)&__xmmword_141133520 + v40);
    if(v32 != (_DWORD)v41) {
      v40 = (unsigned int)(v40 - 1);
      v41 = *((unsigned __int8 *)&__xmmword_141133520 + v40);
      v42 = v32 - v41;
      v43 = (unsigned int)(v42 - 1);
      v44 = &v29[3 * v43];
      v45 = &v3[12 * v41];
      *v45 = v42;
      v45[1] = -1;
      *((_DWORD *)v45 + 2) = (_DWORD)v29 + 12 * v43 - v4;
      *((_DWORD *)v45 + 1) = v44[1];
      LODWORD(v45) = (_DWORD)v45 - v4;
      *(_DWORD *)((unsigned int)v44[1] + v4 + 8) = (_DWORD)v45;
      ++*v44;
      v44[1] = (_DWORD)v45;
    }
    v3[1] = -1;
    v46 = &v29[3 * v40];
    *v3 = v41;
    *((_DWORD *)v3 + 2) = (_DWORD)v46 - v4;
    *((_DWORD *)v3 + 1) = v46[1];
    v47 = (_DWORD)v3 - v4;
    *(_DWORD *)((unsigned int)v46[1] + v4 + 8) = v47;
    v46[1] = v47;
    ++*v46;
  } else {
    v3[1] = -1;
    *((_DWORD *)v3 + 1) = (_DWORD)v29 - v4;
    *v3 = 1;
    *((_DWORD *)v3 + 2) = v29[2];
    v30 = (_DWORD)v3 - v4;
    *(_DWORD *)((unsigned int)v29[2] + v4 + 4) = v30;
    v29[2] = v30;
    ++*v29;
  }
  return 0;
}
PROBE_DECL(__sub_140009620)
__int64 __sub_140009620() {
  PROBE_HIT(__sub_140009620);
  __int64 v0;
  unsigned __int8 *v1;
  int v2;
  int v3;
  unsigned int v4;
  unsigned __int8 *v5;
  char v6;
  unsigned int v7;
  __int64 result;
  __int64 v9;
  __int64 v10;
  unsigned __int64 v11;
  __int64 v12;
  unsigned int v13;
  __int64 v14;
  __int64 v15;
  __int64 v16;
  __int64 v17;
  int v18;
  char v19;
  unsigned int v20;
  unsigned int v21;
  __int64 v22;
  _DWORD *v23;
  unsigned int v24;
  unsigned int v25;
  __int64 v26;
  __int64 v27;
  _DWORD *v28;
  _DWORD *v29;
  unsigned __int64 v30;
  bool v31;
  _DWORD *v32;
  __int64 v33;
  unsigned int v34;
  unsigned int v35;
  unsigned int v36;
  __int64 v37;
  _DWORD *v38;
  __int64 v39;
  __int64 v40;
  unsigned int v41;
  unsigned int v42;
  int v43;
  unsigned int v44;
  __int64 v45;
  __int64 v46;
  int v47;
  _DWORD *v48;
  _DWORD *v49;
  _DWORD *v50;
  int v51;
  unsigned int v52;
  __int64 v53;
  unsigned __int64 v54;
  int v55;
  _DWORD *v56;
  __int64 v57;
  __int64 v58;
  bool v59;
  int v60;
  int v61;
  char v62;
  __int64 v63;
  signed __int64 v64;
  unsigned __int64 v65;
  int v66;
  __int64 v67;
  __int64 *v68;
  unsigned __int8 *v69;
  unsigned int v70;
  __int64 v71;
  unsigned int v72;
  __int64 v73;
  __int64 v74;
  _DWORD *v75;
  int v76;
  __int64 v77;
  __int64 v78;
  __int64 v79;
  _BOOL8 v80;
  unsigned int v81;
  __int64 v82;
  __int64 v83;
  unsigned int v84;
  unsigned int v85;
  int v86;
  __int64 v87;
  unsigned int v88;
  __int64 v89;
  __int64 v90;
  int v91;
  _DWORD *v92;
  __int64 v93;
  _DWORD *v94;
  int v95;
  __int64 v96;
  int v97;
  int v98;
  unsigned int v99;
  __int64 v100;
  unsigned int v101;
  unsigned __int8 *v102;
  unsigned __int8 v103;
  __int64 *v104;
  char v105;
  char v106;
  unsigned int v107;
  unsigned int v108;
  unsigned int v109;
  unsigned int v110;
  unsigned __int8 *v111;
  unsigned __int8 *v112;
  unsigned __int8 *v113;
  __int64 v114;
  __int64 v115;
  __int64 v116;
  int v117;
  __int64 v118;
  unsigned __int64 v119;
  __int64 v120;
  __int64 v121;
  __int64 v122;
  __int16 v123;
  v0 = __qword_141133638;
  v1 = (unsigned __int8 *)__qword_141133610;
  v2 = __dword_141133690;
  v3 = __dword_141133654;
  v4 = *(_DWORD *)(__qword_141133610 + 2);
  v123 = *(_WORD *)__qword_141133610;
  v5 = (unsigned __int8 *)__qword_141133638;
  v122 = __qword_141133638;
  v6 = *((_BYTE *)__xmmword_140028F00 + (unsigned __int8)*(_WORD *)__qword_141133610);
  if(__dword_141133690 == __dword_141133654 && v4) {
    v7 = __sub_14000A2B0(1, (unsigned __int64)__qword_14025F220, __qword_141133580);
    *(_DWORD *)(v1 + 2) = v7;
    if(v7) {
      result = __qword_141133588 + v7;
      __qword_141133638 = result;
      __qword_141133580 = result;
      return result;
    }
    goto LABEL_73;
  }
  v9 = __qword_141133560;
  v10 = __qword_141133588;
  *(_BYTE *)__qword_141133560 = *(_WORD *)__qword_141133610;
  v11 = __qword_141133568;
  v12 = v9 + 1;
  __qword_141133560 = v9 + 1;
  v13 = v9 + 1 - v10;
  if(v9 + 1 >= (unsigned __int64)__qword_141133568)
    goto LABEL_73;
  *(_BYTE *)(v9 + 1) = 0;
  if(!v4) {
    v14 = __qword_141133580;
    v65 = __qword_141133620;
    v66 = *v1;
    v112 = v5;
    v67 = __qword_141133580;
    v115 = v0;
    v68 = __qword_14025F220;
    while(1) {
      if((unsigned __int64)v68 >= v65) {
        if(*(_BYTE *)v67) {
          v69 = (unsigned __int8 *)(v10 + *(unsigned int *)(v67 + 4));
          if(*v69 != v66) {
            do {
              v97 = v69[6];
              v69 += 6;
            } while(v66 != v97);
          }
        } else {
          v69 = (unsigned __int8 *)(v67 + 2);
        }
        *v68++ = (__int64)v69;
      } else {
        v69 = (unsigned __int8 *)*v68++;
      }
      v4 = *(_DWORD *)(v69 + 2);
      if(v4)
        break;
      *(_DWORD *)(v69 + 2) = v13;
      v70 = *(_DWORD *)(v67 + 8);
      __dword_141133690 = --v2;
      if(!v70) {
        v5 = v112;
        v0 = v115;
        v4 = v67 - v10;
        goto LABEL_9;
      }
      v67 = v10 + v70;
    }
    v102 = v69;
    v5 = v112;
    v104 = v68;
    v0 = v115;
    if(v4 <= v13) {
      v100 = v9;
      v110 = v9 + 1 - v10;
      v106 = v6;
      v4 = __sub_14000A2B0(0, (unsigned __int64)(v104 - 1), v67);
      v6 = v106;
      v13 = v110;
      v9 = v100;
      *(_DWORD *)(v102 + 2) = v4;
    }
    if(v2 == v3 - 1 && v14 == v115) {
      *(_DWORD *)(v1 + 2) = v4;
      v4 = *(_DWORD *)(v102 + 2);
      v12 = v9;
      __qword_141133560 = v9;
    }
  LABEL_9:
    if(v4) {
      v15 = v4;
      goto LABEL_11;
    }
  LABEL_73:
    if(!__dword_141133658)
      return __sub_140010A40(2);
    if(*(_BYTE *)v0 == 1) {
      v71 = __qword_141133570;
      if(!*v5 && (unsigned __int8 *)v0 != v5) {
        v73 = __qword_141133588;
        v74 = LOBYTE(__arch_hdr[12]);
        v116 = v0;
        v75 = (_DWORD *)(__qword_141133570 + 12LL * LOBYTE(__arch_hdr[12]));
        v76 = __qword_141133570 + 12 * LOBYTE(__arch_hdr[12]) - __qword_141133588;
        v98 = __qword_141133570 - __qword_141133588 + 444;
        v77 = v122;
        do {
          v78 = *(unsigned int *)(v77 + 4);
          v79 = v73 + v78;
          v80 = *(unsigned __int8 *)(v73 + v78) != v5[2];
          *(_BYTE *)(v77 + 1) = *((_BYTE *)__xmmword_140028F00 + *(unsigned __int8 *)(v73 + v78 + 6 * v80));
          *(_BYTE *)(v73 + v78 + 6 * v80 + 1) = ((unsigned int)*(unsigned __int8 *)(v73 + v78 + 6 * v80 + 1) + 3) >> 2;
          *(_WORD *)(v77 + 2) = *(_WORD *)(v73 + v78 + 6 * v80);
          *(_DWORD *)(v77 + 4) = *(_DWORD *)(v73 + v78 + 6 * v80 + 2);
          v81 = *((unsigned __int8 *)&__xmmword_141133520 + v74);
          *(_BYTE *)v77 = 0;
          if(*(unsigned __int8 *)(12LL * v81 + v73 + v78 + 1) == 255) {
            v82 = 12LL * v81;
            do {
              *(_DWORD *)(*(unsigned int *)(v82 + v79 + 4) + v73 + 8) = *(_DWORD *)(v82 + v79 + 8);
              *(_DWORD *)(*(unsigned int *)(v82 + v79 + 8) + v73 + 4) = *(_DWORD *)(v82 + v79 + 4);
              v83 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + *(unsigned __int8 *)(v82 + v79) + 3);
              --*(_DWORD *)(v83 + v71);
              v81 += *(unsigned __int8 *)(v79 + v82);
              v82 = 12LL * v81;
            } while(*(unsigned __int8 *)(v82 + v79 + 1) == 255);
            if(v81 > 0x80) {
              v113 = v5;
              v84 = v81;
              v85 = 0;
              v86 = 0;
              v87 = -((__int64)(((unsigned __int64)((1LL - v81) >> 6) >> 57) - v81 + 1) >> 7);
              do {
                v81 = v86 + v84 - 128;
                v86 -= 128;
                ++v85;
              } while(v85 < (unsigned int)v87);
              v88 = 0;
              do {
                *(_BYTE *)(v79 + 1) = -1;
                *(_DWORD *)(v79 + 8) = v98;
                ++v88;
                *(_BYTE *)v79 = 0x80;
                *(_DWORD *)(v79 + 4) = *(_DWORD *)(v71 + 448);
                *(_DWORD *)(v73 + *(unsigned int *)(v71 + 448) + 8) = v78;
                ++*(_DWORD *)(v71 + 444);
                *(_DWORD *)(v71 + 448) = v78;
                v78 += 1536;
                v79 += 1536;
              } while(v88 < (unsigned int)v87);
              v5 = v113;
            }
            v89 = *((unsigned __int8 *)&__arch_hdr[11] + v81 + 3);
            LODWORD(v90) = *((unsigned __int8 *)&__xmmword_141133520 + v89);
            if(v81 != (_DWORD)v90) {
              v89 = (unsigned int)(v89 - 1);
              v90 = *((unsigned __int8 *)&__xmmword_141133520 + v89);
              v91 = v81 - v90;
              v92 = (_DWORD *)(v71 + 12LL * (unsigned int)(v91 - 1));
              v93 = v79 + 12 * v90;
              *(_BYTE *)(v93 + 1) = -1;
              *(_BYTE *)v93 = v91;
              *(_DWORD *)(v93 + 8) = v71 + 12 * (v91 - 1) - v73;
              *(_DWORD *)(v93 + 4) = v92[1];
              LODWORD(v93) = v93 - v73;
              *(_DWORD *)((unsigned int)v92[1] + v73 + 8) = v93;
              v92[1] = v93;
              ++*v92;
            }
            *(_BYTE *)(v79 + 1) = -1;
            v94 = (_DWORD *)(v71 + 12 * v89);
            *(_BYTE *)v79 = v90;
            *(_DWORD *)(v79 + 8) = (_DWORD)v94 - v73;
            *(_DWORD *)(v79 + 4) = v94[1];
            v95 = v79 - v73;
            *(_DWORD *)((unsigned int)v94[1] + v73 + 8) = v95;
            v94[1] = v95;
            ++*v94;
          } else {
            *(_BYTE *)(v79 + 1) = -1;
            *(_BYTE *)v79 = v81;
            *(_DWORD *)(v79 + 8) = v76;
            *(_DWORD *)(v79 + 4) = v75[1];
            *(_DWORD *)((unsigned int)v75[1] + v73 + 8) = v78;
            v75[1] = v78;
            ++*v75;
          }
          v77 = v73 + *(unsigned int *)(v77 + 8);
        } while((unsigned __int8 *)v77 != v5);
        v0 = v116;
      }
    } else {
      v71 = __qword_141133570;
    }
    v72 = *(_DWORD *)(v0 + 8);
    if(v72) {
      do {
        v0 = __qword_141133588 + v72;
        v72 = *(_DWORD *)(v0 + 8);
      } while(v72);
      __qword_141133638 = v0;
    }
    result = __sub_14000A4B0((unsigned __int8 *)v0, 0);
    ++__dword_14113364C;
    __dword_141133650 = 0;
    __qword_141133560 = v71 + 456;
    __qword_141133580 = v0;
    __dword_141133690 = 0;
    return result;
  }
  v14 = __qword_141133580;
  v15 = v4;
  if(v11 > v10 + (unsigned __int64)v4) {
    v107 = v9 + 1 - v10;
    v105 = v6;
    v4 = __sub_14000A2B0(0, (unsigned __int64)__qword_14025F220, __qword_141133580);
    v6 = v105;
    v13 = v107;
    goto LABEL_9;
  }
LABEL_11:
  v16 = v10 + v15;
  v17 = *(unsigned int *)(v10 + v15 + 8);
  __dword_141133690 = v2 + 1;
  _mm_prefetch((const char *)(v17 + v10), 1);
  if(__dword_141133690 == v3) {
    v13 = v4;
    __qword_141133560 = v12 - (v0 != v14);
  }
  result = *(unsigned int *)(v15 + v10 + 4);
  _mm_prefetch((const char *)(result + v10), 1);
  if(v0 != v14) {
    v119 = __qword_141133550;
    v18 = __dword_1411336D8 + 8;
    v120 = __qword_141133570;
    v121 = v10 + v15;
    v114 = v0;
    if(__dword_1411336D8 + 8 >= 14)
      v18 = 14;
    if(v18 < 0)
      v18 = 0;
    v19 = v6;
    v117 = __qword_141133570 - v10 + 444;
    v118 = v18;
    v20 = v13;
    while(1) {
      v21 = *v5 + 1;
      if(*v5) {
        if((v21 & 1) != 0) {
          v53 = *((unsigned int *)v5 + 1);
        } else {
          v22 = *((unsigned int *)v5 + 1);
          v23 = (_DWORD *)(v10 + v22);
          v24 = v21 >> 1;
          v25 = *((unsigned __int8 *)&__arch_hdr[11] + (v21 >> 1) + 3);
          v26 = v21 >> 1;
          if(v25 != *((unsigned __int8 *)&__arch_hdr[12] + v26)) {
            v27 = *((unsigned __int8 *)&__arch_hdr[12] + v26);
            v28 = (_DWORD *)(v120 + 12 * v27);
            if(*v28) {
              v29 = (_DWORD *)(v10 + (unsigned int)v28[1]);
              v28[1] = v29[1];
              *(_DWORD *)((unsigned int)v29[1] + v10 + 8) = (_DWORD)v28 - v10;
              --*v28;
            } else {
              v29 = (_DWORD *)__qword_141133558;
              v30 = 12 * (unsigned int)*((unsigned __int8 *)&__xmmword_141133520 + v27);
              v31 = __qword_141133558 + v30 == v119;
              if(__qword_141133558 + v30 > v119) {
                v99 = *((unsigned __int8 *)&__arch_hdr[11] + v24 + 3);
                v108 = v20;
                v29 = (_DWORD *)__sub_140011240(v27);
                v20 = v108;
                v25 = v99;
              } else {
                __qword_141133558 += v30;
                if(!v31)
                  v29[v30 / 4] = 0;
              }
            }
            if(v29) {
              v32 = v29;
              v33 = v10 + v22;
              if((v21 & 2) != 0) {
                *v29 = *v23;
                v29[1] = v23[1];
                v29[2] = v23[2];
                v32 = v29 + 3;
                v33 = v10 + v22 + 12;
              }
              if((v24 & 0xFFFFFFFE) != 0) {
                v111 = v5;
                v34 = 0;
                v35 = 0;
                do {
                  v32[v35] = *(_DWORD *)(v33 + v35 * 4);
                  ++v34;
                  v32[v35 + 1] = *(_DWORD *)(v33 + v35 * 4 + 4);
                  v32[v35 + 2] = *(_DWORD *)(v33 + v35 * 4 + 8);
                  v32[v35 + 3] = *(_DWORD *)(v33 + v35 * 4 + 12);
                  v32[v35 + 4] = *(_DWORD *)(v33 + v35 * 4 + 16);
                  v32[v35 + 5] = *(_DWORD *)(v33 + v35 * 4 + 20);
                  v35 += 6;
                } while(v34 < v21 >> 2);
                v5 = v111;
              }
              v36 = *((unsigned __int8 *)&__xmmword_141133520 + v25);
              v37 = *((unsigned __int8 *)&__xmmword_141133520 + v25);
              if(BYTE1(v23[3 * v37]) == 255) {
                v39 = 3 * v37;
                do {
                  *(_DWORD *)((unsigned int)v23[v39 + 1] + v10 + 8) = v23[v39 + 2];
                  *(_DWORD *)((unsigned int)v23[v39 + 2] + v10 + 4) = v23[v39 + 1];
                  v40 = 12LL * *((unsigned __int8 *)&__arch_hdr[11] + LOBYTE(v23[v39]) + 3);
                  --*(_DWORD *)(v40 + v120);
                  v36 += LOBYTE(v23[v39]);
                  v39 = 3LL * v36;
                } while(BYTE1(v23[v39]) == 255);
                v41 = v36;
                if(v36 > 0x80) {
                  v42 = 0;
                  v101 = -(int)((__int64)(((unsigned __int64)((1LL - v36) >> 6) >> 57) - v36 + 1) >> 7);
                  v43 = 0;
                  do {
                    v36 = v43 + v41 - 128;
                    v43 -= 128;
                    ++v42;
                  } while(v42 < v101);
                  v44 = 0;
                  do {
                    *((_BYTE *)v23 + 1) = -1;
                    v23[2] = v117;
                    ++v44;
                    *(_BYTE *)v23 = 0x80;
                    v23[1] = *(_DWORD *)(v120 + 448);
                    *(_DWORD *)(*(unsigned int *)(v120 + 448) + v10 + 8) = v22;
                    ++*(_DWORD *)(v120 + 444);
                    *(_DWORD *)(v120 + 448) = v22;
                    v22 += 1536;
                    v23 += 384;
                  } while(v44 < v101);
                }
                v45 = *((unsigned __int8 *)&__arch_hdr[11] + v36 + 3);
                v103 = *((_BYTE *)&__xmmword_141133520 + v45);
                if(v36 != v103) {
                  v45 = (unsigned int)(v45 - 1);
                  v46 = *((unsigned __int8 *)&__xmmword_141133520 + v45);
                  v103 = *((_BYTE *)&__xmmword_141133520 + v45);
                  v47 = v36 - v46;
                  v48 = (_DWORD *)(v120 + 12LL * (unsigned int)(v47 - 1));
                  v49 = &v23[3 * v46];
                  *((_BYTE *)v49 + 1) = -1;
                  *(_BYTE *)v49 = v47;
                  v49[2] = (_DWORD)v48 - v10;
                  v49[1] = v48[1];
                  LODWORD(v49) = (_DWORD)v49 - v10;
                  *(_DWORD *)((unsigned int)v48[1] + v10 + 8) = (_DWORD)v49;
                  ++*v48;
                  v48[1] = (_DWORD)v49;
                }
                *((_BYTE *)v23 + 1) = -1;
                v50 = (_DWORD *)(v120 + 12 * v45);
                *(_BYTE *)v23 = v103;
                v23[2] = (_DWORD)v50 - v10;
                v23[1] = v50[1];
                v51 = (_DWORD)v23 - v10;
                *(_DWORD *)((unsigned int)v50[1] + v10 + 8) = v51;
                ++*v50;
                v50[1] = v51;
              } else {
                *((_BYTE *)v23 + 1) = -1;
                v38 = (_DWORD *)(v120 + 12LL * v25);
                *(_BYTE *)v23 = v36;
                v23[2] = (_DWORD)v38 - v10;
                v23[1] = v38[1];
                *(_DWORD *)((unsigned int)v38[1] + v10 + 8) = v22;
                ++*v38;
                v38[1] = v22;
              }
            }
            v23 = v29;
          }
          if(!v23)
            goto LABEL_99;
          v52 = (_DWORD)v23 - v10;
          v53 = v52;
          *((_DWORD *)v5 + 1) = v52;
        }
        v54 = v53 + v10 + 6LL * v21;
        if(v54 > v10 + v53 + 42) {
          do {
            v55 = *(_DWORD *)(v54 - 4);
            *(_WORD *)v54 = *(_WORD *)(v54 - 6);
            *(_DWORD *)(v54 + 2) = v55;
            v54 -= 6LL;
          } while(v54 > v10 + (unsigned __int64)*((unsigned int *)v5 + 1) + 42);
        }
      } else {
        v56 = (_DWORD *)(v120 + 12LL * LOBYTE(__arch_hdr[12]));
        if(*v56) {
          v57 = v10 + (unsigned int)v56[1];
          v56[1] = *(_DWORD *)(v57 + 4);
          *(_DWORD *)(*(unsigned int *)(v57 + 4) + v10 + 8) = (_DWORD)v56 - v10;
          --*v56;
        } else {
          v57 = __qword_141133558;
          v58 = 12 * (unsigned int)*((unsigned __int8 *)&__xmmword_141133520 + LOBYTE(__arch_hdr[12]));
          v59 = __qword_141133558 + v58 == v119;
          if(__qword_141133558 + v58 > v119) {
            v109 = v20;
            v96 = __sub_140011240(LOBYTE(__arch_hdr[12]));
            v20 = v109;
            v57 = v96;
          } else {
            __qword_141133558 += v58;
            if(!v59)
              *(_DWORD *)(v58 + v57) = 0;
          }
        }
        if(!v57) {
        LABEL_99:
          v0 = v114;
          goto LABEL_73;
        }
        *(_WORD *)v57 = *((_WORD *)v5 + 1);
        v60 = __byte_1400227B0[v118];
        *(_DWORD *)(v57 + 2) = *((_DWORD *)v5 + 1);
        *((_DWORD *)v5 + 1) = v57 - v10;
        v61 = 4 * *(unsigned __int8 *)(v57 + 1) + v60;
        if(v61 >= 238)
          v61 = 238;
        if(v61 < 2)
          LOBYTE(v61) = 2;
        *(_BYTE *)(v57 + 1) = v61;
        v54 = v57 + 6;
        *((_WORD *)v5 + 1) = (unsigned __int8)v61;
      }
      *(_BYTE *)(v54 + 1) = 0;
      *(_DWORD *)(v54 + 2) = v20;
      *(_BYTE *)v54 = v123;
      v62 = v5[1] & 0xF0;
      v63 = v10 + *((unsigned int *)v5 + 1);
      ++*v5;
      v64 = v54 - v63;
      result = 0x2AAAAAAAAAAAAAABLL * v64;
      v5[1] = v19 | (v64 / 6) | v62;
      v5 = (unsigned __int8 *)(v10 + *((unsigned int *)v5 + 2));
      if(v5 == (unsigned __int8 *)v14) {
        v16 = v121;
        break;
      }
    }
  }
  __qword_141133580 = v16;
  __qword_141133638 = v16;
  return result;
}
PROBE_DECL(__sub_140007B80)
unsigned __int64 __sub_140007B80(unsigned __int8 *a1) {
  PROBE_HIT(__sub_140007B80);
  _DWORD *v1;
  int v2;
  int v3;
  int v4;
  _DWORD *v5;
  int v6;
  _DWORD *v7;
  _DWORD *v8;
  int v9;
  _DWORD *v10;
  int v11;
  int v12;
  _DWORD *v13;
  int v14;
  __int16 v15;
  __int64 v16;
  int v17;
  char *v18;
  int *v19;
  int v20;
  int v21;
  int v22;
  int *v23;
  int v24;
  int v25;
  unsigned __int8 *v26;
  unsigned int v27;
  __int64 v28;
  int v29;
  unsigned int v30;
  int v31;
  int v32;
  int v33;
  unsigned int v34;
  int v35;
  __int64 v36;
  unsigned int v37;
  int v38;
  __int16 v39;
  char v40;
  __int64 v41;
  __int64 v42;
  unsigned __int8 v43;
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
  _BOOL8 v89;
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
  char *v106;
  unsigned int v107;
  unsigned int v108;
  char *v109;
  int v110;
  __int64 v111;
  __int64 v112;
  int v113;
  int v114;
  int v115;
  int v116;
  unsigned int v117;
  __int64 *v118;
  int v119;
  unsigned __int64 result;
  __int64 v121;
  unsigned __int8 *v122;
  int v123;
  int v124;
  int v125;
  unsigned int v126;
  int v127;
  __int64 v128;
  unsigned __int8 v129;
  unsigned __int64 v130;
  unsigned __int8 *v131;
  unsigned __int64 v132;
  unsigned __int8 *v133;
  __int16 v134;
  int v135;
  unsigned __int8 *v136;
  __int64 v137;
  __int64 v138;
  __int64 v139;
  unsigned __int8 v140;
  unsigned __int64 v141;
  unsigned __int8 *v142;
  unsigned __int8 *v143;
  int v144;
  char v145;
  unsigned __int8 v146;
  unsigned __int8 v147;
  int v148;
  int v149;
  __int16 v150;
  __int16 v151;
  unsigned __int8 v152;
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
  unsigned int v165;
  int v167;
  __int64 v168;
  unsigned __int8 *v169;
  int v170;
  __int16 v171;
  __int16 v172;
  int v173;
  int v174;
  int v175;
  v1 = (_DWORD *)__qword_1411335E8;
  v2 = __dword_1411336B0;
  v3 = __dword_1411336B4;
  v4 = __dword_1411336A8;
  v5 = (_DWORD *)__qword_1411335D0;
  v7 = (_DWORD *)__qword_1411335F8;
  v8 = (_DWORD *)__qword_1411335E0;
  v170 = __dword_141133664;
  v9 = __dword_141133664 - 1;
  *(_DWORD *)__qword_1411335C0 += *(unsigned __int16 *)(__qword_1411335C0 + 6);
  v10 = (_DWORD *)__qword_1411335C8;
  v11 = __dword_1411336AC;
  __dword_141133664 = v9;
  *v1 += v2;
  v12 = __dword_14014AD9C;
  rc.CommitRange();
  *v8 += v3;
  v13 = (_DWORD *)__qword_1411335D8;
  *v10 += v4;
  v14 = __dword_141133688;
  v15 = __dword_141133688;
  v16 = __dword_141133688 & 0x1FFFF;
  *v5 += v11;
  *v13 += v12;
  v17 = __dword_141133680;
  v13[1] -= __dword_14014AD98;
  v18 = &__byte_140029940[v16 + 133144];
  v19 = (int *)__qword_141133600;
  *v7 += __dword_14014ADA4;
  v7[1] -= __dword_14014ADA0;
  v20 = __dword_14014ADAC + __dword_14014ADA8;
  v21 = *v19 - (__dword_14014ADAC + __dword_14014ADA8);
  *v19 = v21;
  v22 = v19[1] + v20 - 2 * v17;
  if(v22 > 0x40000000) {
    *v19 = v21 >> 1;
    v22 >>= 1;
  }
  v19[1] = v22;
  v23 = (int *)__qword_141133608;
  v24 = *(_DWORD *)__qword_141133608 - __dword_14014ADAC;
  v25 = *(_DWORD *)(__qword_141133608 + 4) + __dword_14014ADAC - v17;
  if(v25 <= 0x40000000) {
    *(_DWORD *)(__qword_141133608 + 4) = v25;
    *v23 = v24;
  } else {
    *(_DWORD *)__qword_141133608 = v24 >> 1;
    v23[1] = v25 >> 1;
  }
  v26 = (unsigned __int8 *)__qword_141133610;
  v169 = (unsigned __int8 *)__qword_141133610;
  __dword_14020F1FC[__dword_14025F200] |= __dword_14025F20C;
  __dword_14021F1FC[__dword_14025F208] |= __dword_14025F20C;
  v27 = *v26;
  LODWORD(v28) = __dword_14014ADB0;
  __byte_1400EA158[__dword_14014ADF8] = v27;
  *(v18 - 0x20000) = v27;
  *v18 = v27;
  __dword_14014ADB4 = v27;
  __dword_14014ADB0 = v27;
  __dword_14014ADF8 = (v27 + (__dword_14014ADF8 << 6)) & 0x1FFFF;
  if(__dword_14014AD70 >= 0 && __dword_14014AD70 != __dword_141133674)
    __byte_14014AD5C += __byte_14014AD5C + (v27 == __dword_14014AD70);
  __dword_14014ADD4 = __dword_14014ADCC;
  __dword_14014ADD8 = __dword_14014ADD0;
  __dword_14025F1FC = ((v28 & 0xFFFFFFF8) << 10) + 32 * (v27 & 0xFFFFFFF8);
  v29 = __dword_141133694;
  v30 = *((unsigned __int8 *)__xmmword_140028F00 + v27);
  __dword_141133660 += __dword_141133694;
  __dword_14025F204 = (8 * (v27 + __dword_14025F204)) & 0xFFF00;
  v31 = __dword_14014EDFC[v27];
  v164 = __dword_141133698 + __dword_141133698 + (v30 >> 7);
  __dword_141133698 = v164;
  __dword_14014ADFC[v15 & 0xFFF] = v31;
  __dword_14014EDFC[v27] = v14;
  v32 = v14 - v31;
  if((unsigned int)(v14 - v31) >= 0x104) {
    __dword_14014ADCC = 0;
    __dword_14014AD6C = -1;
  } else {
    v33 = (unsigned __int8)__byte_14002A158[(v31 + 1) & 0x1FFFF];
    __dword_14014AD6C = v33;
    if(v32 >= 50)
      v33 = 0;
    v34 = 192;
    __dword_14014ADCC = v33;
    v35 = __dword_14014ADFC[v31 & 0xFFF];
    if((unsigned int)(v14 - v35) < 0xC0) {
      do {
        --v34;
        __dword_14010A558[(unsigned __int8)__byte_14002A158[(v35 + 1) & 0x1FFFF]] = v9;
        v35 = __dword_14014ADFC[v35 & 0xFFF];
      } while(v34 > v14 - v35);
    }
  }
  v36 = v27 + (__int64)(int)((_DWORD)v28 << 8);
  v156 = __dword_1401CF1FC[v36];
  __dword_14014F1FC[(v14 - 1) & 0x1FFFF] = v156;
  __dword_1401CF1FC[v36] = v14 - 1;
  v37 = v14 - v156;
  __dword_14014ADF4 = v37;
  v162 = (((unsigned int)(v14 - v156) < 0xE800) + (v37 < 0xF0) + (v37 < 7)) << 13;
  if((unsigned int)(v14 - v156) >= 0x1000) {
    __dword_14014ADD0 = 0;
    v38 = 0;
    __dword_14014ADDC = -1;
  } else {
    v38 = (unsigned __int8)__byte_14002A158[(v156 + 2) & 0x1FFFF];
    __dword_14014ADD0 = v38;
    __dword_14014ADDC = (unsigned __int8)__byte_14002A158[(__dword_14014F1FC[v156 & 0x1FFFF] + 2) & 0x1FFFF];
    if(v38 == __dword_14014ADDC) {
      __dword_14010A958[v38] = v9;
      v38 = __dword_14014ADD0;
    }
    if(v37 >= 0x240)
      v38 = 0;
  }
  v39 = __dword_14014AD7C;
  v40 = 2 * __dword_14014ADC8;
  __dword_14014ADC8 *= 2;
  if(v27 != __dword_14014AD7C) {
    v41 = __qword_14002A150;
    *(_DWORD *)(__qword_14002A150 + 512) += 2;
    v157 = v41;
    v42 = ((_WORD)v27 - v39) & 0x1FF;
    v43 = *(_BYTE *)(v42 + v41) + 2;
    *(_BYTE *)(v42 + v41) = v43;
    if(v43 > 0xA7u) {
      *(_DWORD *)(v41 + 512) = 0;
      for(i = 0; i < 512; ++i) {
        v45 = *(unsigned __int8 *)(i + v157) >> 1;
        *(_BYTE *)(i + v157) >>= 1;
        *(_DWORD *)(v157 + 512) += v45;
      }
    }
    v40 = ++__dword_14014ADC8;
    if(v27 != __dword_14014ADD8) {
      __byte_1400DA158[__dword_14014AD7C + (__dword_14014ADD8 << 8)] = v27;
      v40 = __dword_14014ADC8;
    }
  }
  v28 = (int)v28;
  v168 = v28;
  __qword_14002A150 = (__int64)&__byte_140029940[516 * (v40 & 3)];
  v175 = __dword_14014EDFC[(int)v28];
  if(v27 == __dword_14014AD70 && __dword_14014AD68 <= 256) {
    __dword_14014ADB8 = 4 * __dword_14014AD68;
  } else if(__dword_14014ADB8 > (unsigned int)(3 * __dword_14014AD68) && (v170 == __dword_14010A158[v27] || v170 == __dword_14010A558[v27] || 4 * __dword_14014AD68 - 9 < (unsigned int)__dword_14014ADB8)) {
    __dword_14014ADB8 -= __dword_14014AD68 > 13;
  } else if(v32 > 1) {
    v46 = __dword_14014ADFC[v31 & 0xFFF];
    if(v32 == v31 - v46 && v32 == v46 - __dword_14014ADFC[v46 & 0xFFF] && v32 <= 256) {
      if(v27 == (unsigned __int8)v18[-4 * v32] || (v47 = __dword_14014ADFC[v175 & 0xFFF], v32 == v175 - v47) && v32 == v47 - __dword_14014ADFC[v47 & 0xFFF] && (_DWORD)v28 == (unsigned __int8)v18[-3 * v32 - 1]) {
        __dword_14014AD68 = v32;
        __dword_14014ADB8 = v32;
        __qword_14014AD60 = (__int64)&__dword_14014AD58;
      }
    }
  }
  __dword_14014AD80 = -1;
  __dword_14014AD84 = -1;
  __dword_14014AD88 = -1;
  __dword_14014AD90 = -1;
  __dword_14014AD94 = -1;
  __dword_14014AD78 = -1;
  v48 = (unsigned __int8)(v27 - v28);
  __dword_14014AD74 = -1;
  v49 = v14 + 1;
  __dword_141133688 = v14 + 1;
  if(v48 == __dword_14014ADE0) {
    if((unsigned int)++__dword_14014ADE4 > 1) {
      v38 = (unsigned __int8)(2 * v27 - v28);
      __dword_14014AD70 = v38;
      goto LABEL_94;
    }
    __dword_14014AD70 = -1;
  } else {
    __dword_14014AD70 = -1;
    __dword_14014ADE4 = 0;
    __dword_14014ADE0 = v48 - (v48 == 0);
  }
  v50 = (unsigned __int8)*(v18 - 3);
  v51 = (unsigned __int8)*(v18 - 7);
  if(v50 == v51 || (v52 = *(v18 - 11), (_BYTE)(v52 + (_BYTE)v50 - 2 * (_BYTE)v51)) || (_BYTE)(*(v18 - 15) + (_BYTE)v51 - 2 * v52)) {
    if(4 * __dword_14014AD68 - 1 > (unsigned int)__dword_14014ADB8) {
      LODWORD(v53) = -1;
      v54 = __dword_14014F1FC[(v14 - 3) & 0x1FFFF];
      if((unsigned int)(v49 - v54) < 0x20000) {
        v53 = (unsigned __int8)__byte_14002A158[(v54 + 4) & 0x1FFFF];
        __dword_14010A158[v53] = v9;
        v55 = __dword_14014F1FC[*(_QWORD *)&v54 & 0x1FFFFLL];
        if((unsigned int)(v49 - v55) < 0x20000) {
          v56 = (unsigned __int8)__byte_14002A158[(v55 + 4) & 0x1FFFF];
          __dword_14010A158[v56] = v9;
          if((_DWORD)v53 == (_DWORD)v56) {
            if(__dword_14014AD74 < 0) {
              __dword_14014AD74 = v53;
            } else if(__dword_14014AD74 == (_DWORD)v53) {
              __dword_14014AD78 = v53;
            }
            __dword_14010A958[v53] = v9;
          }
        }
      }
      __dword_14014AD90 = v53;
      v57 = __dword_14014F1FC[(v14 - 4) & 0x1FFFF];
      if((unsigned int)(v49 - v57) < 0x20000) {
        v58 = (unsigned __int8)__byte_14002A158[(v57 + 5) & 0x1FFFF];
        __dword_14010A158[v58] = v9;
        v59 = __dword_14014F1FC[*(_QWORD *)&v57 & 0x1FFFFLL];
        if((unsigned int)(v49 - v59) < 0x20000) {
          v60 = (unsigned __int8)__byte_14002A158[(v59 + 5) & 0x1FFFF];
          __dword_14010A158[v60] = v9;
          if((_DWORD)v58 == (_DWORD)v60) {
            if(__dword_14014AD74 < 0) {
              __dword_14014AD74 = v58;
            } else if(__dword_14014AD74 == (_DWORD)v58) {
              __dword_14014AD78 = v58;
            }
            __dword_14010A958[v58] = v9;
          }
        }
      }
      v61 = __dword_14014F1FC[(v14 - 5) & 0x1FFFF];
      if((unsigned int)(v49 - v61) < 0x20000) {
        v62 = (unsigned __int8)__byte_14002A158[(v61 + 6) & 0x1FFFF];
        __dword_14010A158[v62] = v9;
        v63 = __dword_14014F1FC[*(_QWORD *)&v61 & 0x1FFFFLL];
        if((unsigned int)(v49 - v63) < 0x20000) {
          v64 = (unsigned __int8)__byte_14002A158[(v63 + 6) & 0x1FFFF];
          __dword_14010A158[v64] = v9;
          if((_DWORD)v62 == (_DWORD)v64) {
            if(__dword_14014AD74 < 0) {
              __dword_14014AD74 = v62;
            } else if(__dword_14014AD74 == (_DWORD)v62) {
              __dword_14014AD78 = v62;
            }
            __dword_14010A958[v62] = v9;
          }
        }
      }
      v65 = __dword_14014F1FC[(v14 - 8) & 0x1FFFF];
      if((unsigned int)(v49 - v65) < 0x20000) {
        v66 = (unsigned __int8)__byte_14002A158[(v65 + 9) & 0x1FFFF];
        __dword_14010A158[v66] = v9;
        v67 = __dword_14014F1FC[*(_QWORD *)&v65 & 0x1FFFFLL];
        if((unsigned int)(v49 - v67) < 0x20000) {
          v68 = (unsigned __int8)__byte_14002A158[(v67 + 9) & 0x1FFFF];
          __dword_14010A158[v68] = v9;
          if((_DWORD)v66 == (_DWORD)v68) {
            if(__dword_14014AD74 < 0) {
              __dword_14014AD74 = v66;
            } else if(__dword_14014AD74 == (_DWORD)v66) {
              __dword_14014AD78 = v66;
            }
            __dword_14010A958[v66] = v9;
          }
        }
      }
      v69 = __dword_14014F1FC[(v14 - 6) & 0x1FFFF];
      if((unsigned int)(v49 - v69) < 0x10000) {
        v158 = (unsigned __int8)__byte_14002A158[(v69 + 7) & 0x1FFFF];
        __dword_14010A158[(unsigned __int8)__byte_14002A158[(v69 + 7) & 0x1FFFF]] = v9;
        v70 = __dword_14014F1FC[*(_QWORD *)&v69 & 0x1FFFFLL];
        if((unsigned int)(v49 - v70) < 0x20000) {
          v71 = (unsigned __int8)__byte_14002A158[(v70 + 7) & 0x1FFFF];
          __dword_14010A158[512 * (v71 == v158) + v71] = v9;
        }
      }
      v72 = __dword_14014F1FC[(v14 - 10) & 0x1FFFF];
      if((unsigned int)(v49 - v72) < 0x10000) {
        v159 = (unsigned __int8)__byte_14002A158[(v72 + 11) & 0x1FFFF];
        __dword_14010A158[(unsigned __int8)__byte_14002A158[(v72 + 11) & 0x1FFFF]] = v9;
        v73 = __dword_14014F1FC[*(_QWORD *)&v72 & 0x1FFFFLL];
        if((unsigned int)(v49 - v73) < 0x20000) {
          v74 = (unsigned __int8)__byte_14002A158[(v73 + 11) & 0x1FFFF];
          __dword_14010A158[512 * (v74 == v159) + v74] = v9;
        }
      }
      v75 = (unsigned __int8)__byte_14006A158[256 * (unsigned __int8)*(v18 - 1) + (unsigned __int8)*(v18 - 3)];
      __dword_14010A158[256 * (v9 == __dword_14010A158[v75]) + (int)v75] = v9;
      __byte_14006A158[256 * (unsigned __int8)*(v18 - 2) + (unsigned __int8)*(v18 - 4)] = v27;
      v76 = (unsigned __int8)__byte_14007A158[256 * (unsigned __int8)*(v18 - 2) + (unsigned __int8)*(v18 - 6)];
      __dword_14010A158[256 * (v9 == __dword_14010A158[v76]) + (int)v76] = v9;
      __byte_14007A158[256 * (unsigned __int8)*(v18 - 3) + (unsigned __int8)*(v18 - 7)] = v27;
      v77 = (unsigned __int8)__byte_14008A158[256 * (unsigned __int8)*v18 + (unsigned __int8)*(v18 - 7)];
      __dword_14010A158[256 * (v9 == __dword_14010A158[v77]) + (unsigned int)v77] = v9;
      __byte_14008A158[256 * (unsigned __int8)*(v18 - 1) + (unsigned __int8)*(v18 - 8)] = v27;
      __dword_14014AD88 = v77;
      v78 = (unsigned __int8)__byte_14009A158[256 * (unsigned __int8)*(v18 - 1) + (unsigned __int8)*(v18 - 7)];
      __dword_14010A158[256 * (v9 == __dword_14010A158[v78]) + (int)v78] = v9;
      __byte_14009A158[256 * (unsigned __int8)*(v18 - 2) + (unsigned __int8)*(v18 - 8)] = v27;
      v79 = (unsigned int)(__dword_14014ADD0 + (__dword_14014ADCC << 8));
      v80 = (unsigned __int8)__byte_1400AA158[v79];
      __dword_14014AD80 = (unsigned __int8)__byte_1400AA158[v79];
      __dword_14010A158[v80] = v9;
      if(v27 != __dword_14014ADD4 && v27 != __dword_14014ADD8)
        __byte_1400AA158[__dword_14014ADD8 + (__dword_14014ADD4 << 8)] = v27;
      v81 = (unsigned __int8)__byte_14002A158[(__dword_14014ADFC[__dword_14014EDFC[v168] & 0xFFFLL] + 2) & 0x1FFFF];
      __dword_14010A158[256 * (v9 == __dword_14010A158[v81]) + v81] = v9;
    }
  } else {
    v38 = (unsigned __int8)(2 * v50 - v51);
    __dword_14014AD70 = v38;
  }
LABEL_94:
  v82 = __dword_14014F1FC[(v14 - 2) & 0x1FFFF];
  if((unsigned int)(v49 - v82) < 0x20000) {
    v83 = (unsigned __int8)__byte_14002A158[(v82 + 3) & 0x1FFFF];
    __dword_14014AD84 = v83;
    __dword_14010A558[v83] = v9;
    v84 = __dword_14014F1FC[v82 & 0x1FFFF];
    if((unsigned int)(v49 - v84) < 0x20000) {
      v85 = (unsigned __int8)__byte_14002A158[(v84 + 3) & 0x1FFFF];
      __dword_14010A558[v85] = v9;
      if(v83 == (_DWORD)v85)
        __dword_14010A958[v83] = v9;
      LODWORD(v86) = -1;
      v87 = __dword_14014F1FC[v84 & 0x1FFFF];
      if((unsigned int)(v49 - v87) < 0x20000) {
        v88 = 0;
        do {
          v88 += 6144;
          v86 = (unsigned __int8)__byte_14002A158[(v87 + 3) & 0x1FFFF];
          __dword_14010A558[v86] = v9;
          v83 &= v86;
          LODWORD(v85) = v86 | v85;
          v87 = __dword_14014F1FC[v87 & 0x1FFFF];
        } while((unsigned int)(v88 + v49 - v87) < 0x20000);
      }
      if(v83 == (_DWORD)v85 && (int)v86 >= 0) {
        if(__dword_14014AD74 < 0) {
          __dword_14014AD74 = v83;
        } else if(__dword_14014AD74 == v83) {
          __dword_14014AD78 = v83;
        }
      }
    }
  }
  v89 = (v49 & 1) == 0;
  v90 = __dword_14014ADC0[v89];
  v91 = (unsigned __int16)(16 * LOWORD(__dword_14014ADC0[v89]) + ((v27 >> 2) & 0xFFFC)) & 0xFFFC;
  __dword_14014ADC0[v89] = v91;
  v92 = (unsigned __int8)__byte_1400CA15B[v91];
  v93 = !v89;
  v94 = (unsigned __int8)__byte_1400CA15A[v91];
  v95 = (unsigned __int8)__byte_1400CA15A[v91];
  v96 = (unsigned int)__dword_14014ADC0[v93];
  __dword_14010A558[v92] = v9;
  __dword_14010A558[v95] = v9;
  if(v94 == (_DWORD)v92) {
    if(v94 == __dword_14014AD74)
      __dword_14014AD78 = v94;
    __dword_14010A958[v95] = v9;
  }
  __byte_1400CA15B[v96] = __byte_1400CA15A[v96];
  __byte_1400CA15A[v96] = v27;
  v97 = (unsigned int)__dword_14014ADBC;
  __dword_14014AD8C = v94;
  v98 = (unsigned __int16)(((v27 >> 4) & 0xFFFE) + 8 * __dword_14014ADBC) & 0xFFFE;
  v99 = (unsigned __int8)__byte_1400BA158[v98];
  v100 = (unsigned __int8)__byte_1400BA158[v98];
  v101 = (unsigned __int8)__byte_1400BA159[v98];
  __dword_14010A558[v101] = v9;
  __dword_14010A558[v100] = v9;
  if(v99 == (_DWORD)v101) {
    if(v99 == __dword_14014AD74)
      __dword_14014AD78 = v99;
    __dword_14010A958[v100] = v9;
  }
  __byte_1400BA159[v97] = __byte_1400BA158[v97];
  __byte_1400BA158[v97] = v27;
  __dword_14014ADBC = v98;
  v102 = (unsigned int)__dword_14014ADC0[v93];
  v103 = (unsigned __int8)__byte_1400CA158[v102];
  v104 = (unsigned __int8)__byte_1400CA159[v102];
  v105 = (unsigned __int8)__byte_1400CA158[v102];
  __dword_14010A558[v104] = v9;
  __dword_14010A558[v105] = v9;
  if(v103 == (_DWORD)v104) {
    if(v103 == __dword_14014AD74)
      __dword_14014AD78 = v103;
    __dword_14010A958[v105] = v9;
  }
  __byte_1400CA159[v90] = __byte_1400CA158[v90];
  __byte_1400CA158[v90] = v27;
  if(__dword_14014ADB8) {
    if(v27 == *(unsigned __int8 *)__qword_14014AD60) {
      *(_BYTE *)(__qword_14014AD60 + 3) += *(unsigned __int8 *)(__qword_14014AD60 + 3) - 255 < 0;
    } else {
      *(_BYTE *)(__qword_14014AD60 + 2) = *(_BYTE *)(__qword_14014AD60 + 1);
      *(_BYTE *)(__qword_14014AD60 + 1) = *(_BYTE *)__qword_14014AD60;
      *(_BYTE *)__qword_14014AD60 = v27;
      *(_BYTE *)(__qword_14014AD60 + 3) = 0;
    }
    v106 = v18 + 1;
    v107 = (unsigned __int8)v106[-__dword_14014AD68];
    v108 = (unsigned __int8)v106[-2 * __dword_14014AD68];
    v160 = (unsigned __int8)v106[-3 * __dword_14014AD68];
    if(--__dword_14014ADB8 > (unsigned int)__dword_14014AD68)
      __dword_14014ADB4 = v107;
    v109 = &v106[-__dword_14014AD68];
    v110 = (unsigned __int8)*(v109 - 1);
    v111 = ((v109[2] == v106[-2 * __dword_14014AD68 + 2]) << 12) + (((v108 & 0x2E) + ((unsigned __int8)v106[-2 * __dword_14014AD68 + 1] < (unsigned int)(unsigned __int8)v109[1])) << 8) + ((((v110 + 32 - v27) >> 31) + ((v110 - v27) >> 31) + ((int)v27 >= v110)) << 14);
    __qword_14014AD60 = (__int64)&__byte_14010AD58[4 * v111 + 4 * v107];
    __dword_14010A158[(unsigned __int8)__byte_14010AD58[4 * v111 + 2 + 4 * v107]] = v9;
    __dword_14010A158[*(unsigned __int8 *)(__qword_14014AD60 + 1)] = v9;
    v112 = *(unsigned __int8 *)__qword_14014AD60;
    __dword_14014AD94 = *(unsigned __int8 *)__qword_14014AD60;
    __dword_14010A158[v112] = v9;
    if(*(_BYTE *)(__qword_14014AD60 + 3)) {
      if(*(unsigned __int8 *)(__qword_14014AD60 + 3) > 1u || __dword_14014AD6C <= 0)
        __dword_14014AD6C = *(unsigned __int8 *)__qword_14014AD60;
      __dword_14010A958[*(unsigned __int8 *)__qword_14014AD60] = v9;
    }
    if(v107 == v108 && v108 == v160) {
      __dword_14113369C = v38;
      __dword_14014AD70 = v107;
      __dword_14010A958[(unsigned __int8)(v107 + 1)] = v9;
      __dword_14010A558[(unsigned __int8)(v107 - 1)] = v9;
    } else if(__dword_14014AD70 < 0) {
      if(v107 == v108 || v108 == v160) {
        __dword_14113369C = v38;
        __dword_14014AD70 = v107;
        __dword_14010A558[(unsigned __int8)(v160 + 1)] = v9;
        __dword_14010A958[(unsigned __int8)(v107 + 1)] = v9;
        __dword_14010A558[(unsigned __int8)(v107 - 1)] = v9;
        __dword_14010A958[v160] = v9;
      } else if(v107 == v160) {
        __dword_14113369C = v38;
        if(v108 == (unsigned __int8)v106[-4 * __dword_14014AD68] && (unsigned __int8)v106[-5 * __dword_14014AD68] == v160)
          v107 = v108;
        __dword_14014AD70 = v107;
        __dword_14010A958[v108] = v9;
      } else {
        v145 = v107 + v160 - 2 * v108;
        if(v145) {
          __dword_14113369C = v38;
          if(*(unsigned __int8 *)(__qword_14014AD60 + 3) <= 0x10u) {
            if((unsigned __int8)(v145 + 19) <= 0x26u) {
              v146 = 2 * v107 - v108;
              v147 = v146 - v145;
              __dword_14010A158[(unsigned __int8)(v147 + 2)] = v9;
              __dword_14010A158[(unsigned __int8)(v147 - 2)] = v9;
              __dword_14010A158[(unsigned __int8)(v147 + 1)] = v9;
              __dword_14010A158[v147] = v9;
              __dword_14010A558[(unsigned __int8)(v146 + 2)] = v9;
              __dword_14010A958[(unsigned __int8)(v146 - 2)] = v9;
              __dword_14010A958[(unsigned __int8)(v146 + 1)] = v9;
              __dword_14010A958[(unsigned __int8)(v146 - 1)] = v9;
              __dword_14010A958[v146] = v9;
            }
          } else {
            __dword_14014AD70 = *(unsigned __int8 *)__qword_14014AD60;
          }
        } else {
          __dword_14014AD70 = (unsigned __int8)(2 * v107 - v108);
          __dword_14113369C = (unsigned __int8)(2 * v107 - v108);
          __dword_14010A158[(unsigned __int8)(__dword_14014AD70 + 1)] = v9;
          __dword_14010A158[(unsigned __int8)(__dword_14014AD70 - 1)] = v9;
          __dword_14010A158[(unsigned __int8)(__dword_14014AD70 + 2)] = v9;
          __dword_14010A158[(unsigned __int8)(__dword_14014AD70 - 2)] = v9;
          __dword_14010A958[(unsigned __int8)(__dword_14014AD70 + 1)] = v9;
          __dword_14010A958[(unsigned __int8)(__dword_14014AD70 - 1)] = v9;
          __dword_14010A958[__dword_14014AD70] = v9;
        }
      }
    } else {
      __dword_14113369C = v38;
    }
  } else {
    __dword_14113369C = v38;
    __dword_14014AD68 = 1024;
    if(__dword_14014AD70 < 0)
      __dword_14014AD70 = __dword_14014AD78;
  }
  v113 = __dword_141133690;
  __qword_14025F220[0] = (__int64)v169;
  v114 = (__dword_141133690 > 0) << 15;
  __dword_1411336A0 = v114 + v162 + 4 * (__dword_14014ADB4 & 0x80);
  v115 = *a1;
  __dword_1411336E0 = v115;
  v116 = *v169;
  v117 = *((_DWORD *)a1 + 2);
  __dword_1411336D0 = ((__dword_141133690 > 2) << 15) + v114 + *((_DWORD *)__xmmword_140029200 + (v164 & 0xF)) + (v29 << 14);
  v118 = &__qword_14025F228;
  __dword_14113368C = (((__dword_141133690 > 32) + (__dword_141133690 > 8) + (__dword_141133690 > 4) + (__dword_141133690 > 3) + (__dword_141133690 > 2) + (__dword_141133690 > 1)) << 12) + 64;
  if(v117) {
    v121 = __qword_141133588;
    v122 = (unsigned __int8 *)(__qword_141133588 + v117);
    v123 = __dword_141133690;
    v124 = 5 * __dword_141133690;
    if(!*v122) {
      do {
        if(v118 > &__qword_14025F230[1])
          *(_BYTE *)(*(v118 - 2) + 1) += *(unsigned __int8 *)(*(v118 - 2) + 1) - 2 < 0;
        v136 = v122 + 2;
        v137 = *((unsigned int *)v122 + 2);
        *v118++ = (__int64)v136;
        --v123;
        v122 = (unsigned __int8 *)(v121 + v137);
      } while(!*v122);
      v119 = __dword_141133654;
      if(v113 < __dword_141133654) {
        v138 = __qword_14025F220[0];
        if(*(unsigned __int8 *)(__qword_14025F220[0] + 1) < 7u) {
          if(v118 <= &__qword_14025F230[1]) {
            ++*(_BYTE *)(__qword_14025F228 + 1);
          } else {
            ++*(_BYTE *)(__qword_14025F230[0] + 1);
            if(v118 > (__int64 *)__byte_14025F240)
              *(_BYTE *)(v138 + 1) += *(unsigned __int8 *)(v138 + 1) - 4 < 0;
          }
        }
      }
      v139 = *((unsigned int *)v122 + 1);
      v140 = v122[1];
      goto LABEL_165;
    }
    v125 = 2;
    if(v115)
      v126 = (-45 * v115 + (unsigned int)*((unsigned __int16 *)a1 + 1)) >> 31;
    else
      v126 = __dword_141133684 == 0;
    v165 = v126;
    v161 = __dword_141133690;
    v167 = __dword_141133690 + 3;
    v163 = __dword_141133690 + 1;
    v127 = 3 * __dword_141133690;
    do {
      v128 = *((unsigned int *)v122 + 1);
      v129 = v122[1];
      if(!*(_BYTE *)(v121 + v128 + 6LL * (v129 & 0xF) + 1)) {
        __qword_141133620 = (__int64)v118;
        __sub_14000C0E0(v122);
        v128 = *((unsigned int *)v122 + 1);
        v129 = v122[1];
      }
      v122[1] = v129 & 0xF0;
      v130 = v121 + v128;
      v131 = (unsigned __int8 *)(v121 + v128);
      if(*(unsigned __int8 *)(v121 + v128) != v116) {
        do
          v131 += 6;
        while(*v131 != v116);
        v171 = *(_WORD *)v131;
        v132 = v121 + v128 + 42;
        v173 = *(_DWORD *)(v131 + 2);
        while(1) {
          if((unsigned __int64)v131 <= v130 || (v133 = v131 - 6, HIBYTE(v171) + 13 <= *(v131 - 5))) {
            if((unsigned __int64)v131 <= v132) {
              *(_WORD *)v131 = v171;
              *(_DWORD *)(v131 + 2) = v173;
              v122[1] |= (__int64)&v131[-v130] / 6;
              break;
            }
            v133 = v131 - 6;
          }
          v134 = *(_WORD *)v133;
          v135 = *(_DWORD *)(v133 + 2);
          *(_WORD *)v131 = v134;
          *(_DWORD *)(v131 + 2) = v135;
          v131 -= 6;
        }
      }
      v148 = v169[1];
      *v118++ = (__int64)v131;
      v124 -= 5;
      v127 -= 3;
      --v123;
      if(v148 > 130 || v131[1] >= 0xE4u) {
        v153 = v163;
        v113 = v161;
        v119 = __dword_141133654;
        v122 = (unsigned __int8 *)(v121 + *((unsigned int *)v122 + 2));
        goto LABEL_201;
      }
      v149 = *((unsigned __int16 *)v122 + 1);
      v150 = v125 + (v124 > v167);
      v125 >>= 1;
      v151 = (7 * v149 < 4 * v131[1] * *v122 + 4 * (unsigned int)v131[1]) + (2 * v123 > v163) + v150;
      *((_WORD *)v122 + 1) = v151 + v149;
      v152 = v151 + v131[1];
      v131[1] = v152;
      v122 = (unsigned __int8 *)(v121 + *((unsigned int *)v122 + 2));
    } while(v152 < 0x45u && v127 > v167 && v165);
    v153 = v163;
    v113 = v161;
    v119 = __dword_141133654;
  LABEL_201:
    while(v153 < 4 * v123) {
      v139 = *((unsigned int *)v122 + 1);
      v140 = v122[1];
      if(*(unsigned __int8 *)(v121 + v139 + 6LL * (v140 & 0xF)) == v116)
        break;
    LABEL_165:
      v141 = v121 + v139;
      if(!*(_BYTE *)(v121 + v139 + 6LL * (v140 & 0xF) + 1)) {
        __qword_141133620 = (__int64)v118;
        __sub_14000C0E0(v122);
        v139 = *((unsigned int *)v122 + 1);
        v140 = v122[1];
        v141 = v121 + v139;
      }
      v122[1] = v140 & 0xF0;
      v142 = (unsigned __int8 *)v141;
      if(*(unsigned __int8 *)(v139 + v121) != v116) {
        do
          v142 += 6;
        while(*v142 != v116);
        v172 = *(_WORD *)v142;
        v174 = *(_DWORD *)(v142 + 2);
        while(1) {
          if((unsigned __int64)v142 <= v141 || (v143 = v142 - 6, HIBYTE(v172) < (int)*(v142 - 5))) {
            if((unsigned __int64)v142 <= v141 + 42) {
              *(_WORD *)v142 = v172;
              *(_DWORD *)(v142 + 2) = v174;
              v122[1] |= (__int64)&v142[-v141] / 6;
              break;
            }
            v143 = v142 - 6;
          }
          v144 = *(_DWORD *)(v143 + 2);
          *(_WORD *)v142 = *(_WORD *)v143;
          *(_DWORD *)(v142 + 2) = v144;
          v142 -= 6;
        }
      }
      v154 = *(unsigned __int8 *)(v139 + v121 + 1);
      v155 = *(unsigned __int8 *)(v139 + v121 + 7);
      *v118++ = (__int64)v142;
      if((unsigned int)(v155 + v154) > 0x5F)
        break;
      v122 = (unsigned __int8 *)(v121 + *((unsigned int *)v122 + 2));
      --v123;
      v153 = v113 + 1;
    }
    __qword_141133620 = (__int64)v118;
  } else {
    v119 = __dword_141133654;
    __qword_141133620 = (__int64)&__qword_14025F228;
  }
  if(v113 == v119 && (result = __qword_141133588 + *(unsigned int *)(v169 + 2), result >= __qword_141133568)) {
    __qword_141133638 = __qword_141133588 + *(unsigned int *)(v169 + 2);
    __qword_141133580 = result;
  } else {
    result = __sub_140009620();
    v113 = __dword_141133690;
  }
  __dword_14113370C = v113;
  return result;
}


PROBE_DECL(__sub_1400037A0)
int __sub_1400037A0(FILE *a1, FILE *a2) {
  PROBE_HIT(__sub_1400037A0);
  int v2;
  unsigned __int8 *v3;
  int v4;
  int v5;
  __int64 v6;
  __int64 v7;
  unsigned __int8 v8;
  int v9;
  __int64 v10;
  __int64 *v11;
  __int64 *v12;
  unsigned __int8 *v13;
  int v14;
  int v15;
  __int64 *v16;
  __int64 *i;
  __int64 v18;
  int v19;
  int v20;
  int v21;
  int v22;
  unsigned __int8 *v23;
  __int64 v24;
  int v25;
  int v26;
  int v27;
  unsigned __int8 v28;
  int v29;
  int v30;
  unsigned __int8 *v31;
  unsigned __int8 *v32;
  int v33;
  int v34;
  int v35;
  int v36;
  int v37;
  int v38;
  unsigned int v39;
  unsigned __int8 *v40;
  int v41;
  int v42;
  int v43;
  int v44;
  int v45;
  char v46;
  __int64 v47;
  int *v48;
  int v49;
  int v50;
  unsigned int v51;
  int v52;
  int v53;
  int v54;
  unsigned int v55;
  int v56;
  int *v57;
  int v58;
  __int64 v59;
  int v60;
  unsigned __int16 v61;
  unsigned int v62;
  unsigned int v63;
  int v64;
  int v65;
  int v66;
  __int64 *v67;
  unsigned __int8 *v68;
  unsigned int v69;
  unsigned int v70;
  __int64 v71;
  BOOL v72;
  bool v73;
  int v74;
  int v75;
  __int16 v76;
  int v77;
  unsigned int v78;
  __int64 v79;
  __int64 v80;
  unsigned __int16 *v81;
  int v82;
  int v83;
  unsigned __int16 v84;
  int v85;
  int *v86;
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
  unsigned int v97;
  int *v98;
  int *v99;
  int v100;
  int *v101;
  char v102;
  bool v103;
  int v104;
  bool v105;
  unsigned int v106;
  unsigned int v107;
  unsigned int v108;
  unsigned int v109;
  unsigned int v110;
  __int64 v111;
  unsigned int v112;
  int v113;
  int v114;
  int v115;
  unsigned int v116;
  char v117;
  unsigned int v118;
  char v119;
  unsigned int v120;
  unsigned __int16 *v121;
  __int64 v122;
  int v123;
  int v124;
  __int64 v125;
  int v126;
  unsigned int v127;
  int v128;
  char v129;
  unsigned __int16 v130;
  unsigned int v131;
  char v132;
  unsigned __int16 v133;
  unsigned __int16 v134;
  int *v135;
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
  int *v148;
  int v149;
  int v150;
  int v151;
  int v152;
  int v153;
  int v154;
  int v155;
  __int64 v156;
  int v157;
  unsigned int v158;
  int v159;
  int v160;
  __int64 v161;
  int *v162;
  int v163;
  int v164;
  int v165;
  int v166;
  int v167;
  unsigned int v168;
  int v169;
  unsigned int v170;
  int v171;
  unsigned int v172;
  __int64 v173;
  int v174;
  int v175;
  _DWORD *v176;
  unsigned int v177;
  unsigned __int8 v178;
  __int16 v179;
  __int16 v180;
  unsigned __int64 v181;
  int v182;
  int v183;
  int v184;
  unsigned int v185;
  _DWORD *v186;
  __int64 v187;
  int v188;
  int v189;
  int v190;
  unsigned int v191;
  __int64 v192;
  int v193;
  __int64 v194;
  __int64 v195;
  int v196;
  _DWORD *v197;
  int v198;
  __int64 v199;
  int result;
  int v201;
  unsigned int v202;
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
  __int64 *v213;
  __int64 *v214;
  __int64 v215;
  unsigned __int8 *v216;
  __int64 *v217;
  int v218;
  __int64 v219;
  __int64 v220;
  int v221;
  unsigned __int8 *v222;
  __int64 *v223;
  unsigned __int8 *v224;
  __int64 v225;
  __int64 v226;
  __int64 v227;
  unsigned __int8 *v228;
  unsigned int v229;
  unsigned __int8 v230;
  int v231;
  unsigned __int8 *v232;
  unsigned __int8 *v233;
  int v234;
  unsigned int v235;
  unsigned int v236;
  __int64 v237;
  int v238;
  unsigned __int8 *j;
  unsigned int v240;
  int v241;
  __int16 v242;
  unsigned int v243;
  unsigned int v244;
  unsigned int v245;
  unsigned int v246;
  __int64 v247;
  int v248;
  __int64 v249;
  unsigned int v250;
  int v251;
  int v252;
  int v253;
  int v254;
  unsigned int v255;
  BOOL v256;
  int v257;
  __int64 v258;
  unsigned int v259;
  BOOL v260;
  __int64 v261;
  int *v262;
  __int64 v263;
  int *v264;
  int v265;
  int v266;
  unsigned __int16 v267;
  int v268;
  int v269;
  int v270;
  int v271;
  int *v272;
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
  unsigned __int16 *v283;
  unsigned int v284;
  int v285;
  int *v286;
  unsigned int v287;
  int v288;
  int v289;
  __int64 v290;
  unsigned __int16 v291;
  char v292;
  __int64 v293;
  unsigned __int16 v294;
  unsigned __int16 *v295;
  int v296;
  int v297;
  int v298;
  unsigned int v299;
  int v300;
  unsigned __int16 v301;
  __int64 v302;
  unsigned __int16 v303;
  char v304;
  __int64 v305;
  unsigned __int16 v306;
  __int64 v307;
  unsigned __int16 v308;
  __int64 v309;
  unsigned __int16 v310;
  __int64 v311;
  unsigned __int16 v312;
  __int64 v313;
  unsigned __int16 v314;
  int *v315;
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
  int *v327;
  int v328;
  int v329;
  int v330;
  int v331;
  int v332;
  int v333;
  int v334;
  __int64 v335;
  unsigned int v336;
  BOOL v337;
  int v338;
  int v339;
  int v340;
  int *v341;
  int v342;
  int v343;
  int v344;
  int v345;
  int v346;
  unsigned int v347;
  int v348;
  unsigned int v349;
  int v350;
  unsigned int v351;
  int v352;
  bool v353;
  _DWORD *v354;
  int v355;
  _DWORD *v356;
  int v357;
  _DWORD *v358;
  int v359;
  _DWORD *v360;
  int v361;
  unsigned __int8 v362;
  __int64 v363;
  _DWORD *v364;
  int v365;
  _DWORD *v366;
  unsigned int v367;
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
  unsigned int v378;
  __int64 v379;
  int v380;
  int v381;
  int v382;
  int v383;
  __int64 *v384;
  __int64 *k;
  __int64 v386;
  __int64 v387;
  __int64 v388;
  int v389;
  unsigned __int8 *v390;
  unsigned __int8 v391;
  int v392;
  unsigned __int8 *v393;
  int v394;
  int v395;
  int v396;
  int v397;
  unsigned int v398;
  __int64 v399;
  int v400;
  int v401;
  int v402;
  unsigned int v403;
  int v404;
  int v405;
  __int64 v406;
  __int64 v407;
  char *v408;
  unsigned int v409;
  int v410;
  unsigned int v411;
  unsigned int v412;
  unsigned int v413;
  int v414;
  int v415;
  int v416;
  int v417;
  unsigned int v418;
  char *v419;
  int v420;
  unsigned __int16 v421;
  unsigned int v422;
  int v423;
  BOOL v424;
  __int64 v425;
  int v426;
  __int64 v427;
  int v428;
  __int64 v429;
  signed int v430;
  int v431;
  unsigned __int16 *v432;
  unsigned __int16 *v433;
  unsigned __int16 *v434;
  unsigned __int16 *v435;
  unsigned __int16 *v436;
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
  int *v459;
  int *v460;
  int *v461;
  int *v462;
  int v463;
  int v464;
  int v465;
  int v466;
  bool v467;
  char v468;
  char v469;
  char v470;
  unsigned int v471;
  int *v472;
  __int64 v473;
  __int64 v474;
  unsigned __int8 *v475;
  int v476;
  int v477;
  int v478;
  __int64 v479;
  char v480;
  bool v481;
  int *v482;
  char v483;
  unsigned int v484;
  int *v485;
  __int64 v486;
  __int64 v487;
  int v488;
  int v489;
  int v490;
  int v491;
  int v492;
  int v493;
  __int64 *v494;
  __int64 *v495;
  int v496;
  __int64 v497;
  int v498;
  int v499;
  __int64 v500;
  unsigned __int8 *v501;
  int v502;
  __int64 v503;
  unsigned int v504;
  int v505;
  __int64 v506;
  BOOL v507;
  unsigned int v508;
  __int64 v509;
  int v510;
  __int64 v511;
  int v512;
  unsigned __int8 v513;
  unsigned int v514;
  unsigned int v515;
  int v516;
  int v517;
  int v518;
  int v519;
  int v520;
  __int64 v521;
  unsigned int v522;
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
  unsigned __int8 *v538;
  unsigned int v539;
  int v540;
  __int64 v541;
  unsigned __int8 *v542;
  int v543;
  unsigned int v544;
  unsigned int v545;
  unsigned int v546;
  __int64 v547;
  unsigned int v548;
  int v549;
  unsigned __int8 *v550;
  unsigned __int8 *v551;
  unsigned __int8 *v552;
  __int64 v553;
  unsigned __int16 *v554;
  unsigned __int16 *v555;
  unsigned __int16 *v556;
  unsigned int v557;
  unsigned __int8 *v558;
  unsigned int v559;
  __int64 v560;
  int v563;
  int v564;
  __int16 v565;
  int v566;
  int v567;
  int v568;
  unsigned __int8 *v569;
  __int64 v570;
  __int16 v571;
  int v572;
  int v573;
  unsigned int v574;
  int v575;
  __int64 v576;
  v2 = __dword_141133664;
  do {
    v3 = (unsigned __int8 *)__qword_141133580;
    v4 = *(unsigned __int8 *)__qword_141133580;
    v566 = getc(a2);
    if(v4) {
      v6 = __qword_141133588;
      v7 = *((unsigned int *)v3 + 1);
      v8 = v3[1];
      if(!*(_BYTE *)(__qword_141133588 + v7 + 6LL * (v8 & 0xF) + 1)) {
        __sub_14000C0E0(v3);
        v7 = *((unsigned int *)v3 + 1);
        v8 = v3[1];
      }
      v563 = v4;
      v540 = *v3;
      v539 = v540 + 1;
      v9 = v540 + 1;
      v541 = v6;
      v550 = v3;
      __qword_14025F220[0] = v6 + v7 + 6LL * (v8 & 0xF);
      v10 = *(unsigned __int8 *)__qword_14025F220[0];
      v11 = &__qword_14025F228;
      v12 = &__qword_14025F228;
      v567 = *(unsigned __int8 *)__qword_14025F220[0];
      __dword_141133674 = v567;
      v13 = (unsigned __int8 *)(v6 + v7 - 6);
      __dword_140029540[v10] = v2;
      while(1) {
        v13 += 6;
        v14 = *v13;
        if((_DWORD)v10 != v14) {
          __dword_140029540[*v13] = v2;
          *v11++ = (__int64)v13;
          if(v14 == __dword_14014AD70) {
            v15 = 22;
            v16 = &__qword_14025F220[(__byte_14014AD5C & 7) == 0];
          LABEL_14:
            for(i = v11 - 1; i > v16; --i) {
              v18 = *(i - 1);
              if(v15 * *(unsigned __int8 *)(*i + 1) <= *(unsigned __int8 *)(v18 + 1))
                break;
              *(i - 1) = *i;
              *i = v18;
            }
            ++v12;
            goto LABEL_18;
          }
          if(v2 == __dword_14010A958[v14] || v2 == __dword_14010A558[v14]) {
            v16 = v12;
            v15 = 11 * (v2 == __dword_14010A958[v14]) + 7;
            goto LABEL_14;
          }
        }
      LABEL_18:
        if(!--v9) {
          v19 = v540 + 1;
          v20 = v540 + 1;
          v21 = v540 + 1;
          v22 = v540;
          v23 = v550;
          __dword_14014AD7C = *(unsigned __int8 *)__qword_14025F220[0];
          __dword_14010A158[(unsigned __int8)__byte_1400DA158[__dword_14014AD7C + (__dword_14014ADD0 << 8)]] = v2;
          __qword_1411335A8 = (__int64)&__dword_1405DFC20;
          __qword_1411335A0 = (__int64)&__dword_1405DFC20;
          __qword_141133598 = (__int64)&__dword_1405DFC20;
          __qword_141133590 = (__int64)&__dword_1405DFC20;
          __qword_141133618 = (__int64)&__dword_1405DFC20;
          if(v540 == 255) {
            v44 = __dword_141133690;
            v46 = __dword_141133698;
            v45 = __dword_1411336E0;
            v64 = *((unsigned __int16 *)v550 + 1);
            __qword_1411335B0 = (__int64)&__dword_1405DFC20;
            v568 = -1;
            __dword_141133678 = -1;
            __dword_1411336E4 = v64;
            v62 = v64 + 1;
            __dword_1411336E8 = v62;
          } else {
            v24 = *((unsigned int *)v550 + 2);
            v25 = *(unsigned __int8 *)(v24 + v541);
            v26 = *((unsigned __int16 *)v550 + 1);
            v27 = *(unsigned __int16 *)(v24 + v541 + 2);
            v28 = v550[1];
            __qword_141133620 = (__int64)__qword_14025F220;
            v20 = v25 + 1;
            v505 = v27;
            v513 = v28;
            v29 = v26;
            if((v25 + 1) * v26 > (int)(v27 * v539) || (v479 = *((unsigned int *)v550 + 1), *(unsigned __int8 *)(v479 + v541 + 1) >= 0x6Au)) {
              v510 = v26;
              v21 = v540 + 1;
              v506 = v539;
            } else if((v513 & 0x40) != 0) {
              v510 = v26;
              v21 = v540 + 1;
              v506 = v539;
            } else {
              v516 = v2;
              v30 = v25 + 1;
              v31 = (unsigned __int8 *)(v541 + *(unsigned int *)(v24 + v541 + 4) - 6);
              do {
                v31 += 6;
                --v30;
                __byte_141108A20[*v31] = v31[1];
              } while(v30);
              v506 = v539;
              v32 = (unsigned __int8 *)(v479 + v541 + 6LL * v539);
              v33 = 9 * v540 + 9 + 2 * v540 + 2;
              v34 = v29;
              v35 = v20 * (((unsigned int)(v33 - v29) >> 28) | 7) + v505;
              do {
                v32 -= 6;
                v37 = v32[1];
                if(v37 * v35 < (v33 + v34) * (unsigned __int8)__byte_141108A20[*v32] && v32[1] <= 0xE4u) {
                  v38 = v29 - v37;
                  v36 = (unsigned __int8)__byte_141108A20[*v32];
                  v39 = ((v33 + v34 - v37) * (3 * v37 + v36) + 3 * (v33 + v34 - v37) + v35 - v36 - 4) / (unsigned int)(3 * (v33 + v34 - v37) + v35 - v36);
                  if(v39 > v37 + 11)
                    v39 = v37 + 11;
                  v29 = v39 + v38;
                  *((_WORD *)v550 + 1) = v29;
                  v32[1] = v39;
                  v34 = (unsigned __int16)v29;
                }
                --v19;
              } while(v19);
              v21 = v540 + 1;
              v2 = v516;
              v24 = *((unsigned int *)v550 + 2);
              v510 = v34;
              v22 = v540;
              v513 = v550[1];
            }
            v512 = *v550;
            v40 = (unsigned __int8 *)(v541 + v24);
            v41 = *(unsigned __int8 *)(v541 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
            v42 = v41;
            v568 = v41;
            if(v41 == v567) {
              v43 = *(unsigned __int8 *)(v541 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
              do {
                if(*v40 != v512)
                  break;
                v40 = (unsigned __int8 *)(v541 + *((unsigned int *)v40 + 2));
                v42 = *(unsigned __int8 *)(v541 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
                v43 = v42;
              } while(v42 == v567);
              v568 = v43;
              __dword_141133678 = v42;
            } else {
              __dword_141133678 = *(unsigned __int8 *)(v541 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
            }
            v508 = v2 != __dword_140029540[__dword_14113369C];
            v507 = v2 != __dword_140029540[v41];
            v44 = __dword_141133690;
            v45 = __dword_1411336E0;
            v46 = __dword_141133698;
            v511 = (unsigned __int8)__byte_1400292BE[v506];
            v47 = (int)(8 * (v512 + 2 * v25 + 2 > __dword_1411336E0) + (__dword_141133698 & 3) + 4 * (__dword_141133690 < 10) + ((v2 != __dword_140029540[v568]) << 9) + (v508 << 8) + (((unsigned int)(8 * v22 + 8 + 4 * v22 + 4 - v510) >> 26) & 0xFFFFFFE0) + (v513 & 0xC0) + (((unsigned int)(3 * v25 + 3 - (4 * v22 + 4)) >> 27) & 0xFFFFFFF0));
            v509 = v47;
            v48 = &__dword_141108B20[2048 * v511 + 2 * v47];
            v49 = *v48;
            __qword_1411335B0 = (__int64)v48;
            v515 = v49;
            __dword_141133680 = v49;
            v50 = (unsigned __int16)__word_141108B24[4096 * v511 + 4 * v47];
            v514 = v50;
            __dword_14113367C = v50;
            if(v50 > 0x8000 || v515 > 0x80000) {
              v462 = &__dword_141108B20[2048 * v511 + 2 * v47];
              v466 = v47;
              v493 = __dword_1411336E0;
              v458 = __dword_141133690;
              __sub_140010730(v462);
              v44 = v458;
              v45 = v493;
              LODWORD(v47) = v466;
              v48 = v462;
              LOWORD(v50) = *((_WORD *)v462 + 2);
            }
            v51 = *((unsigned __int16 *)v48 + 3);
            *((_WORD *)v48 + 2) = v51 + v50;
            if(v51 > 0x80) {
              v488 = v45;
              v485 = &__dword_141128C20[512 * v511 + 16 * (v507 | v508) + 2 * ((unsigned __int8)v47 & 0xF3)];
              __qword_1411335A8 = (__int64)v485;
              v483 = v51 < 0x200;
              __qword_141133618 = (__int64)(v48 + 2048);
              v52 = *(v48 - 2048);
              v482 = v48 - 2048;
              __qword_141133590 = (__int64)(v48 - 2048);
              v484 = v48[2048];
              v53 = ((v484 + v52) >> (v483 + 1)) + v515;
              v54 = *((unsigned __int16 *)v48 + 4098);
              v515 = v53;
              v55 = (v54 + (unsigned int)*((unsigned __int16 *)v48 - 4094)) >> (v483 + 1);
              __dword_141133680 = v53;
              v45 = v488;
              v514 += v55;
              __dword_14113367C = v514;
              if(v54 > 0x8000 || v484 > 0x80000) {
                v461 = v48;
                v465 = v47;
                v457 = v44;
                __sub_140010730(v48 + 2048);
                v44 = v457;
                v45 = v488;
                LODWORD(v47) = v465;
                v48 = v461;
                LOWORD(v54) = *((_WORD *)v461 + 4098);
              }
              __dword_1411336EC = *((unsigned __int16 *)v48 + 4099) >> v483;
              v56 = *((unsigned __int16 *)v48 - 4094);
              *((_WORD *)v48 + 4098) = __dword_1411336EC + v54;
              if(v56 > 0x8000 || (unsigned int)*(v48 - 2048) > 0x80000) {
                v460 = v48;
                v464 = v47;
                v492 = v45;
                v456 = v44;
                __sub_140010730(v482);
                v44 = v456;
                v45 = v492;
                LODWORD(v47) = v464;
                v48 = v460;
                LOWORD(v56) = *((_WORD *)v460 - 4094);
              }
              __dword_1411336B8 = *((unsigned __int16 *)v48 - 4093) >> v483;
              *((_WORD *)v48 - 4094) = __dword_1411336B8 + v56;
              v481 = *((unsigned __int16 *)v48 + 3) < 0x400u;
              v480 = v483 + v481;
              v57 = &v48[-2 * v509];
              v58 = LOWORD(v57[2 * (int)(v47 ^ 0x100) + 1]);
              v486 = (__int64)&v57[2 * (int)(v47 ^ 0x100)];
              __qword_141133598 = v486;
              if(v58 > 0x8000 || *(_DWORD *)v486 > 0x80000u) {
                v459 = v57;
                v463 = v47;
                v491 = v45;
                v455 = v44;
                __sub_140010730(v486);
                v44 = v455;
                v45 = v491;
                LODWORD(v47) = v463;
                v57 = v459;
                LOWORD(v58) = *(_WORD *)(v486 + 4);
              }
              __dword_1411336BC = *(unsigned __int16 *)(v486 + 6) >> v480;
              *(_WORD *)(v486 + 4) = __dword_1411336BC + v58;
              v59 = (__int64)&v57[2 * (int)(v47 ^ 0x200)];
              v60 = *(unsigned __int16 *)(v59 + 4);
              v487 = v59;
              __qword_1411335A0 = v59;
              if(v60 > 0x8000 || *(_DWORD *)v59 > 0x80000u) {
                v490 = v45;
                v454 = v44;
                __sub_140010730(v59);
                v44 = v454;
                v45 = v490;
                LOWORD(v60) = *(_WORD *)(v487 + 4);
              }
              __dword_1411336C0 = *(unsigned __int16 *)(v487 + 6) >> v480;
              *(_WORD *)(v487 + 4) = __dword_1411336C0 + v60;
              v61 = *((_WORD *)v485 + 2);
              if(v61 > 0x8000u || (unsigned int)*v485 > 0x80000) {
                v489 = v45;
                v453 = v44;
                __sub_140010730(v485);
                v44 = v453;
                v45 = v489;
                v61 = *((_WORD *)v485 + 2);
              }
              __dword_1411336C4 = *((unsigned __int16 *)v485 + 3) >> (v483 + v481 + 1);
              *((_WORD *)v485 + 2) = __dword_1411336C4 + v61;
            }
            __dword_1411336E4 = *((unsigned __int16 *)v550 + 1);
            v62 = __dword_1411336E4 + ((v514 >> 1) + v21 * v515) / v514 + 2;
            __dword_1411336E8 = v62;
            if(v21 < 24) {
              __dword_1411336F0 = 0;
              v63 = 0;
            LABEL_58:
              __dword_1411336CC = 171;
              v574 = v63;
              v65 = 0;
              __dword_1411336C8 = 0;
              v66 = 171;
              __dword_1411336F4 = 1032 * (*((_DWORD *)v550 + 2) == 0) + 32 * (v46 & 3) + 2 * (v44 < 3);
              v575 = __dword_1411336F4;
              v564 = v563 + 1;
              v67 = __qword_14025F220;
              __dword_14113368C += (((unsigned int)(v20 - v21 - 4) >> 24) & 0xFFFFFF80) + 28672 * ((unsigned int)(40 - v21) >> 31) + 32 * (*v550 > v45) + 28672;
              v571 = __dword_1411336A0;
              goto LABEL_59;
            }
          }
          v63 = v62 >> 1;
          __dword_1411336F0 = v62 >> 1;
          goto LABEL_58;
        }
      }
    }
    v220 = __qword_141133588;
    v221 = v3[2];
    __dword_141133674 = v221;
    v222 = v3;
    v223 = __qword_14025F220;
    __dword_14014AD7C = v221;
    __dword_14010A158[(unsigned __int8)__byte_1400DA158[v221 + (__dword_14014ADD0 << 8)]] = v2;
    do {
      v224 = v222 + 2;
      v225 = *((unsigned int *)v222 + 2);
      *v223++ = (__int64)v224;
      v222 = (unsigned __int8 *)(v220 + v225);
      LODWORD(v226) = *v222;
    } while(!*v222);
    v227 = *((unsigned int *)v222 + 1);
    v546 = v223 - __qword_14025F220;
    v545 = v546;
    v228 = (unsigned __int8 *)(v220 + v227 + 6LL * *v222);
    if(!v228[1]) {
      v229 = v223 - __qword_14025F220;
      do {
        if(*v228 != v221)
          break;
        v222 = (unsigned __int8 *)(v220 + *((unsigned int *)v222 + 2));
        v227 = *((unsigned int *)v222 + 1);
        v226 = *v222;
        ++v229;
        v228 = (unsigned __int8 *)(v220 + v227 + 6 * v226);
      } while(!v228[1]);
      v545 = v229;
    }
    v230 = v222[1];
    v544 = v226 + 1;
    if(!*(_BYTE *)(v227 + v220 + 6LL * (v230 & 0xF) + 1)) {
      v494 = v223;
      __sub_14000C0E0(v222);
      v223 = v494;
      v227 = *((unsigned int *)v222 + 1);
      v230 = v222[1];
    }
    v231 = *(unsigned __int8 *)(v227 + v220 + 6LL * (v230 & 0xF));
    v232 = (unsigned __int8 *)(v220 + v227);
    v543 = v231;
    __dword_141133678 = v231;
    v542 = v232;
    if(v221 != *v232) {
      v233 = v232;
      do
        v233 += 6;
      while(v221 != *v233);
      v542 = v233;
    }
    v234 = v542[1];
    v235 = *((unsigned __int16 *)v222 + 1);
    if(2 * v234 + 23 > v232[7] + v232[1]) {
      v236 = *((_DWORD *)v222 + 2);
      if(v236) {
        v475 = (unsigned __int8 *)(v220 + v236);
        v478 = *((unsigned __int16 *)v475 + 1);
        v476 = *v475;
        if(v235 + v235 * v476 < v544 * (v478 + 15)) {
          v237 = *((unsigned int *)v475 + 1);
          v477 = v476 + 1;
          if(!*(_BYTE *)(v220 + v237 + 6LL * (v475[1] & 0xF) + 1)) {
            v495 = v223;
            __sub_14000C0E0(v475);
            v223 = v495;
            v234 = v542[1];
            v235 = *((unsigned __int16 *)v222 + 1);
            v237 = *((unsigned int *)v475 + 1);
            v478 = *((unsigned __int16 *)v475 + 1);
          }
          v238 = 5 * v226 + 5;
          for(j = (unsigned __int8 *)(v220 + v237); v221 != *j; j += 6)
            ;
          v441 = j[1];
          v240 = v235 + 2 * v238;
          v241 = (2 * (2 * v238 < v235) + 5) * v477 + v478;
          if(v241 * v234 < (int)(v240 * v441)) {
            if(v234 > 228)
              goto LABEL_164;
            v242 = v235 - v234;
            v243 = ((v240 - v234) * (v441 + 3 * v234) + 3 * (v240 - v234) + v241 - v441 - 4) / (3 * (v240 - v234) + v241 - v441);
            if(v243 > v234 + 11)
              LOWORD(v243) = v234 + 11;
            LOWORD(v235) = v243 + v242;
            *((_WORD *)v222 + 1) = v235;
            v542[1] = v243;
            v234 = (unsigned __int8)v243;
            v235 = (unsigned __int16)v235;
          }
        }
      }
    }
    if(v234 <= 9) {
      v244 = (unsigned __int8)__algn_141136BC4[v234 + 1083];
      goto LABEL_166;
    }
  LABEL_164:
    v244 = v234 - 6;
  LABEL_166:
    v245 = v235 - v244;
    v246 = (6 * v245 < v244) + (15 * v245 < 4 * v244) + (9 * v245 < 4 * v244) + (13 * v245 < 8 * v244) + (v245 < v244) + (v245 < 2 * v244) + (v245 < 6 * v244) + (35 * v245 < 4 * v244) + 1;
    do {
      v247 = *--v223;
      if(v246 > *(unsigned __int8 *)(*v223 + 1))
        *(_BYTE *)(v247 + 1) = v246;
      else
        v246 = *(unsigned __int8 *)(*v223 + 1);
    } while(v223 != __qword_14025F220);
    v547 = v247;
    __dword_141133684 = v246 - ((6 * v245 < v244) + (15 * v245 < 4 * v244) + (9 * v245 < 4 * v244) + (13 * v245 < 8 * v244) + (v245 < v244) + (v245 < 2 * v244) + (v245 < 6 * v244) + (35 * v245 < 4 * v244) + 1);
    __dword_14025F20C = 1 << v221;
    __dword_14025F200 = (unsigned int)(v221 + __dword_14025F1FC) >> 5;
    __dword_14025F208 = (unsigned int)(v221 + __dword_14025F204) >> 5;
    v549 = (((1 << v221) & __dword_14020F1FC[(unsigned int)(v221 + __dword_14025F1FC) >> 5]) != 0) + 2 * (((1 << v221) & __dword_14021F1FC[(unsigned int)(v221 + __dword_14025F204) >> 5]) != 0);
    v248 = __dword_1401CF1FC[v221 + (__dword_14014ADB0 << 8)];
    if((unsigned int)(__dword_141133688 - v248) >= 0x20000) {
      __dword_14014ADF0 = 0x20000;
      __dword_14014ADEC = 0x20000;
      __dword_14014ADE8 = 0x20000;
    } else {
      __dword_14014ADEC = __dword_1401CF1FC[v221 + (__dword_14014ADB0 << 8)];
      __dword_14014ADE8 = (unsigned __int8)__byte_14002A158[(v248 + 2) & 0x1FFFF];
      __dword_14014ADEC = __dword_141133688 - v248;
      __dword_14014ADF0 = __dword_141133688 - __dword_1401CF1FC[__dword_14014ADE8 + (v221 << 8)];
    }
    v249 = *((unsigned int *)v3 + 2);
    v250 = *(unsigned __int8 *)(v249 + v220);
    v251 = 32 * (__dword_141133660 > 0) + (v3[1] & 0x80);
    __dword_14113368C += (v250 == 0) << 7;
    v548 = __dword_141133690 - v545;
    v252 = __dword_141133694 + ((4 * (__dword_141133690 - v545 > 3) + 8 * (v221 == v543) + (v221 == __dword_14014AD70) + 2 * (v2 == __dword_14010A158[v221])) << 9) + v251 + ((v549 == 3 || v2 == __dword_14010A958[v221]) << 13);
    v253 = (((__dword_141133698 & 6) == 6) << 8) + ((__dword_141133698 & 1) << 6);
    v254 = v252 + v253;
    if(v250) {
      v255 = v254 & 0xFFFFFFDE;
      v256 = v246 != (6 * v245 < v244) + (15 * v245 < 4 * v244) + (9 * v245 < 4 * v244) + (13 * v245 < 8 * v244) + (v245 < v244) + (v245 < 2 * v244) + (v245 < 6 * v244) + (35 * v245 < 4 * v244) + 1 || v245 <= 10 * v244;
      v257 = 32 * v256 + v255 + (v221 == __dword_14113369C);
    } else {
      v258 = *(unsigned int *)(v249 + v220 + 8);
      v250 = *(unsigned __int8 *)(v258 + v220);
      if(*(_BYTE *)(v258 + v220) || v548 < 4 || v548 < 5 && v544 < 4 && *((unsigned __int16 *)v222 + 1) - v542[1] < 36) {
        v259 = *(_DWORD *)(v258 + v220 + 8);
        if(v259)
          v260 = 2 * v250 < *(unsigned __int8 *)(v259 + v220);
        else
          v260 = 0;
        v257 = v254 + 8 * v260 + 16;
      } else if(v546 >= 6) {
        if(v546 >= 0xE) {
          v257 = v252 + v253 + 14;
          if(v546 < 0x20)
            v257 = v252 + v253 + 12;
        } else {
          v257 = v252 + v253 + 10;
        }
      } else {
        v257 = v252 + v253 + 8;
      }
    }
    v261 = v257 + (unsigned int)*((unsigned __int8 *)__xmmword_140029100 + v250);
    v262 = &__dword_14025FA20[0x8000 * (unsigned __int64)(unsigned __int8)__byte_140029240[v246]];
    __qword_141133590 = (__int64)&v262[2 * (v261 ^ 0x1000)];
    __qword_141133598 = (__int64)&v262[2 * (v261 ^ 1)];
    __qword_1411335A0 = (__int64)&v262[2 * (v261 ^ 0x2000)];
    __qword_1411335A8 = (__int64)&v262[2 * (v261 ^ 0x20)];
    __qword_1411335B0 = (__int64)&v262[2 * (v261 ^ 0x200)];
    v263 = (__int64)&v262[2 * (v261 ^ 0x400)];
    v264 = &v262[2 * v261];
    v265 = *((unsigned __int16 *)v264 + 2);
    __qword_1411335B8 = v263;
    __qword_1411335C0 = (__int64)v264;
    __qword_1411335C8 = (__int64)(v264 - 4);
    __qword_1411335D0 = (__int64)(v264 + 4);
    if(v265 > 0x8000) {
      __sub_140010850(v264);
      v265 = *((unsigned __int16 *)v264 + 2);
    }
    v266 = *(unsigned __int16 *)v264;
    v267 = *((_WORD *)v264 + 3);
    __dword_141133680 = v266;
    *((_WORD *)v264 + 2) = v265 + v267;
    __dword_14113367C = v265;
    __dword_14014AD98 = v266;
    __dword_14014AD9C = v265;
    v268 = (v221 & 0xE0) + 16 * (v221 == __dword_14014ADD0) + 8 * (v221 == __dword_14014ADCC) + 4 * (v221 == __dword_14014AD8C) + (v221 == __dword_14014AD80) + (__dword_1411336A0 & 0xE200) + 2 * (v221 == __dword_14014AD88);
    v269 = (((_WORD)v221 - (_WORD)__dword_14014ADB0) & 0x800) + (((_WORD)__dword_14014ADB4 - (_WORD)v221) & 0x400) + ((v221 == __dword_14014ADDC) << 8);
    v270 = v549;
    v271 = v269 + v268;
    if(v549)
      v270 = 1;
    __dword_1411336A0 = (v270 << 12) + v271 + (((v265 < (unsigned int)(56 * v266)) + (v265 < (unsigned int)(6 * v266))) << 16);
    v272 = &__dword_14045FB20[2 * __dword_1411336A0];
    v273 = *v272;
    __qword_1411335D8 = (__int64)v272;
    v274 = v266 * (__int64)v273 / v272[1];
    v275 = __dword_141133680;
    v276 = 1 - __dword_141133680;
    if(v274 >= 1 - __dword_141133680) {
      v276 = 0x40000;
      if(v274 < 0x40000)
        v276 = v274;
    }
    v277 = *v272;
    v278 = v272[1];
    if((int)abs32(*v272) > 0x40000) {
      v277 >>= 1;
      *v272 = v277;
      v278 >>= v278 > 4096;
    }
    v279 = v277 - v275;
    *v272 = v279;
    v280 = v275 + v278 + 2;
    if(v280 > 0x40000000) {
      *v272 = v279 >> 1;
      v280 >>= 1;
    }
    v272[1] = v280;
    v281 = v276 + __dword_14113367C;
    v282 = v276 + v275;
    v283 = (unsigned __int16 *)__qword_1411335C0;
    v284 = *(unsigned __int16 *)(__qword_1411335C0 + 6);
    if(v284 <= 0x20) {
      v285 = __dword_141133674;
      __dword_14113367C = v281;
      __dword_141133680 = v282;
      __qword_1411335B8 = (__int64)&__dword_1405DFC20;
      __qword_1411335B0 = (__int64)&__dword_1405DFC20;
      __qword_1411335A8 = (__int64)&__dword_1405DFC20;
      __qword_1411335A0 = (__int64)&__dword_1405DFC20;
      __qword_141133598 = (__int64)&__dword_1405DFC20;
      __qword_141133590 = (__int64)&__dword_1405DFC20;
      __qword_1411335D0 = (__int64)&__dword_1405DFC20;
      __qword_1411335C8 = (__int64)&__dword_1405DFC20;
      __qword_1411335E0 = (__int64)&__dword_1405DFC20;
      __qword_1411335E8 = (__int64)&__dword_1405DFC20;
      __qword_1411335F0 = (__int64)&__dword_1411336A4;
    } else {
      v285 = __dword_141133674;
      v286 = &__dword_14079FD20[128 * (unsigned __int64)(unsigned __int8)__byte_1400293C0[v246] + 64 * (__dword_141133660 > -9) + 4 * (unsigned int)(__dword_14010A558[__dword_141133674] == __dword_141133664) + (((unsigned int)v261 >> 6) & 0xBLL) + (unsigned int)(16 * v549)];
      v287 = *v286;
      __qword_1411335F0 = (__int64)v286;
      v288 = (v287 >> (v284 < 0x230)) + v282;
      __dword_141133680 = v288;
      v289 = (0x3100u >> (v284 < 0x230)) + v281;
      __dword_14113367C = v289;
      *v286 = v287 - ((v287 + 2) >> 3);
      v290 = __qword_1411335C8;
      v103 = v284 < 0x220;
      v291 = *(_WORD *)(__qword_1411335C8 + 4);
      v292 = v103;
      if(v291 > 0x8000u) {
        v438 = __qword_1411335C8;
        v435 = v283;
        __sub_140010850(__qword_1411335C8);
        v283 = v435;
        v290 = v438;
        v291 = *(_WORD *)(v438 + 4);
      }
      __dword_1411336A8 = *(unsigned __int16 *)(v290 + 6) >> v292;
      *(_WORD *)(v290 + 4) = __dword_1411336A8 + v291;
      v293 = __qword_1411335D0;
      v294 = *(_WORD *)(__qword_1411335D0 + 4);
      if(v294 > 0x8000u) {
        v437 = __qword_1411335D0;
        v434 = v283;
        __sub_140010850(__qword_1411335D0);
        v283 = v434;
        v293 = v437;
        v294 = *(_WORD *)(v437 + 4);
      }
      __dword_1411336AC = *(unsigned __int16 *)(v293 + 6) >> v292;
      v295 = v283 - 0x10000;
      __qword_1411335E0 = (__int64)(v283 - 0x10000);
      *(_WORD *)(v293 + 4) = __dword_1411336AC + v294;
      v296 = *(v283 - 0x10000);
      v297 = v283[0x10000];
      v298 = *(v283 - 65534);
      __qword_1411335E8 = (__int64)(v283 + 0x10000);
      v299 = v297 + v296;
      v300 = v283[65538];
      v282 = (v299 >> (v292 + 1)) + v288;
      __dword_141133680 = v282;
      v281 = ((unsigned int)(v300 + v298) >> (v292 + 1)) + v289;
      __dword_14113367C = v281;
      if(v300 > 0x8000) {
        v436 = v283 - 0x10000;
        v433 = v283;
        __sub_140010850(v283 + 0x10000);
        v283 = v433;
        v295 = v436;
        LOWORD(v300) = v433[65538];
      }
      __dword_1411336B0 = v283[65539] >> v292;
      v283[65538] = __dword_1411336B0 + v300;
      v301 = *(v283 - 65534);
      if(v301 > 0x8000u) {
        v432 = v283;
        __sub_140010850(v295);
        v283 = v432;
        v301 = *(v432 - 65534);
      }
      v302 = __qword_141133590;
      __dword_1411336B4 = *(v283 - 65533) >> v292;
      *(v283 - 65534) = __dword_1411336B4 + v301;
      v303 = *(_WORD *)(v302 + 4);
      v304 = (v283[3] < 0x398u) + v292;
      if(v303 > 0x8000u) {
        __sub_140010850(v302);
        v303 = *(_WORD *)(v302 + 4);
      }
      __dword_1411336B8 = *(unsigned __int16 *)(v302 + 6) >> v304;
      *(_WORD *)(v302 + 4) = __dword_1411336B8 + v303;
      v305 = __qword_141133598;
      v306 = *(_WORD *)(__qword_141133598 + 4);
      if(v306 > 0x8000u) {
        __sub_140010850(__qword_141133598);
        v306 = *(_WORD *)(v305 + 4);
      }
      __dword_1411336BC = *(unsigned __int16 *)(v305 + 6) >> v304;
      *(_WORD *)(v305 + 4) = __dword_1411336BC + v306;
      v307 = __qword_1411335A0;
      v308 = *(_WORD *)(__qword_1411335A0 + 4);
      if(v308 > 0x8000u) {
        __sub_140010850(__qword_1411335A0);
        v308 = *(_WORD *)(v307 + 4);
      }
      __dword_1411336C0 = *(unsigned __int16 *)(v307 + 6) >> v304;
      *(_WORD *)(v307 + 4) = __dword_1411336C0 + v308;
      v309 = __qword_1411335A8;
      v310 = *(_WORD *)(__qword_1411335A8 + 4);
      if(v310 > 0x8000u) {
        __sub_140010850(__qword_1411335A8);
        v310 = *(_WORD *)(v309 + 4);
      }
      __dword_1411336C4 = *(unsigned __int16 *)(v309 + 6) >> v304;
      *(_WORD *)(v309 + 4) = __dword_1411336C4 + v310;
      v311 = __qword_1411335B0;
      v312 = *(_WORD *)(__qword_1411335B0 + 4);
      if(v312 > 0x8000u) {
        __sub_140010850(__qword_1411335B0);
        v312 = *(_WORD *)(v311 + 4);
      }
      __dword_1411336C8 = *(unsigned __int16 *)(v311 + 6) >> v304;
      *(_WORD *)(v311 + 4) = __dword_1411336C8 + v312;
      v313 = __qword_1411335B8;
      v314 = *(_WORD *)(__qword_1411335B8 + 4);
      if(v314 > 0x8000u) {
        __sub_140010850(__qword_1411335B8);
        v314 = *(_WORD *)(v313 + 4);
      }
      __dword_1411336CC = *(unsigned __int16 *)(v313 + 6) >> v304;
      *(_WORD *)(v313 + 4) = __dword_1411336CC + v314;
    }
    __dword_141133680 = 60416LL * v282 / v281;
    __dword_14113367C = 60416;
    __dword_14014ADA0 = __dword_141133680;
    __dword_14014ADA4 = 60416;
    v315 = &__dword_1407A0720[0x100000 * (__dword_14014AD68 < (unsigned int)__dword_14014ADB8) + 0x80000 * (v285 == __dword_14014AD70) + 0x20000 * __dword_141133694 + 512 * __dword_14014ADB4 + 2 * v285 + 2 * (unsigned __int64)((unsigned __int8)((unsigned int)__dword_14014ADEC < 0x78) << 17)];
    v316 = *v315;
    __qword_1411335F8 = (__int64)v315;
    v317 = __dword_141133680 * (__int64)v316 / v315[1];
    v318 = __dword_141133680;
    v319 = 1 - __dword_141133680;
    if(v317 >= 1 - __dword_141133680) {
      v319 = 0x40000;
      if(v317 < 0x40000)
        v319 = (int)(60416LL * v282 / v281) * (__int64)v316 / v315[1];
    }
    v320 = *v315;
    if((int)abs32(*v315) > 0x80000) {
      v321 = v315[1];
      v320 >>= 1;
      *v315 = v320;
      v315[1] = v321 >> (v321 > 0x2000);
    }
    v322 = v315[1];
    v323 = v320 - v318;
    *v315 = v323;
    v324 = v318 + v322 + 1120;
    v315[1] = v324;
    if(v324 > 0x40000000) {
      *v315 = v323 >> 1;
      v315[1] = v324 >> 1;
    }
    __dword_14113367C += v319;
    __dword_14014ADA8 = __dword_14113367C;
    v325 = v319 + v318;
    __dword_141133680 = v325;
    v326 = ((17 * v325 < (unsigned int)__dword_14113367C) << 13) + (((unsigned __int16)__dword_14014ADE8 - (unsigned __int16)v285) & 0x1000) + 16 * (v285 == __dword_14113369C) + 8 * (v285 == __dword_14014AD70) + 4 * (v285 == __dword_14014AD94) + (v285 == __dword_14014AD84) + (((unsigned int)__dword_14014ADF0 < 0x29) << 7) + __dword_1411336D0 + (__dword_1411336A0 & 0x800) + 2 * (*((unsigned __int8 *)__xmmword_140028F00 + v285) + (v285 == __dword_14014AD90)) + 32 * (((unsigned int)__dword_14014ADEC < 0xA800) + ((unsigned int)__dword_14014ADEC < 0x600) + ((unsigned int)__dword_14014ADEC < 0xD));
    v327 = &__dword_140FA0820[2 * v326];
    v328 = *v327;
    __qword_141133600 = (__int64)v327;
    v329 = (__dword_14113367C - v325) * (__int64)v328 / __dword_140FA0824[2 * v326];
    v330 = v325;
    v331 = v325 - __dword_14113367C + 1;
    if(v329 >= v331) {
      v331 = 0x40000;
      if(v329 < 0x40000)
        v331 = (__dword_14113367C - v325) * (__int64)v328 / __dword_140FA0824[2 * v326];
    }
    v332 = *v327;
    v333 = v331 + __dword_14113367C;
    __dword_14113367C += v331;
    v334 = __dword_140FA0824[2 * v326];
    if((int)abs32(v332) > 0x100000) {
      v332 >>= 1;
      *v327 = v332;
      v334 >>= v334 > 0x2000;
    }
    *v327 = v332 + 2 * v325;
    v335 = __qword_14002A150;
    __dword_140FA0824[2 * v326] = v334 + 1;
    v336 = __qword_141133588 + *(_DWORD *)(v547 + 2) - __qword_141133560;
    v337 = __dword_14010A558[v285] == __dword_141133664;
    v338 = *(unsigned __int8 *)(v335 + (((_WORD)v285 - (_WORD)__dword_14014AD7C) & 0x1FF));
    v339 = (*(_DWORD *)(v335 + 512) < (unsigned int)(384 * v338)) + (*(_DWORD *)(v335 + 512) < (unsigned int)(58 * v338)) + (*(_DWORD *)(v335 + 512) < (unsigned int)(22 * v338));
    v340 = (unsigned __int8)__byte_1400EA158[__dword_14014ADF8];
    __dword_14014ADAC = v333;
    v341 = &__dword_141060920[2 * (int)(4 * (v336 >= 0xFFFFFD20) + v337 + (v326 & 0x700) + __dword_14113368C + (((unsigned int)__dword_14014ADEC < 0x2C00) << 11) + 2 * (v285 == v340) + 8 * v339)];
    v342 = *v341;
    __qword_141133608 = (__int64)v341;
    v343 = (v333 - v330) * (__int64)v342 / v341[1];
    v344 = __dword_141133680;
    v345 = __dword_141133680 - __dword_14113367C + 1;
    if(v343 >= v345) {
      v345 = 0x80000;
      if(v343 < 0x80000)
        v345 = v343;
    }
    v346 = *v341;
    v347 = v345 + __dword_14113367C;
    __dword_14113367C += v345;
    v348 = v341[1];
    if((int)abs32(v346) > 0x100000) {
      v346 >>= 1;
      *v341 = v346;
      v348 >>= v348 > 0x2000;
    }
    *v341 = v346 + v330;
    v341[1] = v348 + 2;
    v350 = v3[2];
    v351 = rc.GetSubRange(v344, v347);
    if(v350 != v566) {
      v427 = (unsigned int)__dword_14025F200;
      v428 = ~__dword_14025F20C;
      rc.EncodeNotMatched(v351);
      v429 = v3[2];
      __dword_140029540[v429] = __dword_141133664;
      v430 = v347 + v347 * v3[3];
      __dword_14020F1FC[v427] &= v428;
      __qword_141133610 = 0;
      __dword_14021F1FC[__dword_14025F208] &= ~__dword_14025F20C;
      v195 = __qword_141133588;
      v431 = v3[3];
      v23 = (unsigned __int8 *)__qword_141133580;
      v201 = *(unsigned __int8 *)__qword_141133580;
      __dword_141133694 = 0;
      __dword_1411336DC = v201;
      result = (int)(16 * (v431 * v344 + v344 - v431 * v347)) / v430;
      __dword_1411336D8 = result;
      do {
      LABEL_128:
        v202 = *((_DWORD *)v23 + 2);
        if(!v202)
          return result;
        v203 = __dword_141133690;
        v204 = --__dword_141133690;
        v23 = (unsigned __int8 *)(v195 + v202);
        v205 = *v23;
        __qword_141133580 = (__int64)v23;
        if(v205 && !*(_BYTE *)(v195 + *((unsigned int *)v23 + 1) + 6LL * (v23[1] & 0xF) + 1))
          result = __sub_14000C0E0(v23);
        v206 = v205 - v201;
      } while(!v206);
      v207 = *v23;
      v208 = *((unsigned int *)v23 + 1);
      v533 = v204;
      v534 = v203;
      v209 = v207 - v201;
      v564 = v206;
      v532 = v207;
      v531 = v207 + 1;
      v210 = v209;
      v535 = v531;
      v65 = 0x8000;
      v211 = 0;
      v212 = 0;
      v213 = __qword_14025F220;
      v214 = __qword_14025F220;
      v215 = *(unsigned __int8 *)(v195 + v208 + 6LL * (v23[1] & 0xF));
      v538 = (unsigned __int8 *)(v195 + v208 + 6LL * (v23[1] & 0xF));
      v2 = __dword_141133664;
      v567 = *v538;
      __dword_141133674 = v567;
      if(__dword_141133664 == __dword_140029540[v215]) {
        __dword_1411336C8 = 0x8000;
      } else {
        __dword_140029540[v215] = __dword_141133664;
        v211 = 1;
        v213 = &__qword_14025F228;
        __qword_14025F220[0] = (__int64)v538;
        v212 = v538[1];
        v214 = &__qword_14025F228;
        v65 = 0;
        v210 = v209 - 1;
        __dword_1411336C8 = 0;
      }
      v216 = (unsigned __int8 *)(v195 + v208 - 6);
      if(v210) {
        v520 = v209;
        v217 = v213;
        v521 = v195;
        v523 = v65;
        v552 = v23;
        while(1) {
          do {
            v216 += 6;
            v218 = *v216;
            v219 = *v216;
          } while(v2 == __dword_140029540[v219]);
          v382 = v216[1];
          __dword_140029540[v219] = v2;
          *v213++ = (__int64)v216;
          v214 = v213;
          v212 += v382;
          if(v218 == __dword_14014AD70)
            break;
          if(v2 == __dword_14010A958[v219] || v2 == __dword_14010A558[v219]) {
            v384 = v217;
            v383 = 4 * (v2 == __dword_14010A958[v219]) + 12;
            goto LABEL_292;
          }
        LABEL_296:
          if(!--v210) {
            v209 = v520;
            v195 = v521;
            v65 = v523;
            v23 = v552;
            goto LABEL_298;
          }
        }
        v383 = 22;
        v384 = &__qword_14025F220[(unsigned __int8)v211 & ((__byte_14014AD5C & 7) == 0)];
      LABEL_292:
        for(k = v213 - 1; k > v384; --k) {
          v386 = *(k - 1);
          if(v383 * *(unsigned __int8 *)(*k + 1) <= *(unsigned __int8 *)(v386 + 1))
            break;
          *(k - 1) = *k;
          *k = v386;
        }
        ++v217;
        goto LABEL_296;
      }
    LABEL_298:
      __qword_1411335A8 = (__int64)&__dword_1405DFC20;
      __qword_1411335A0 = (__int64)&__dword_1405DFC20;
      __qword_141133598 = (__int64)&__dword_1405DFC20;
      __qword_141133590 = (__int64)&__dword_1405DFC20;
      __qword_141133618 = (__int64)&__dword_1405DFC20;
      if(v531 == 256) {
        v568 = -1;
        __dword_141133678 = -1;
        __qword_1411335B0 = (__int64)&__dword_1405DFC20;
        v422 = 1;
        v537 = __dword_141133698;
        v536 = __dword_1411336E0;
      } else {
        v387 = *((unsigned int *)v23 + 2);
        v501 = (unsigned __int8 *)(v195 + v387);
        v503 = *(unsigned int *)(v195 + v387 + 4);
        v568 = *(unsigned __int8 *)(v195 + v503 + 6LL * (*(_BYTE *)(v195 + v387 + 1) & 0xF));
        if(v568 == v567) {
          v388 = *(unsigned int *)(v195 + v387 + 4);
          v389 = *(unsigned __int8 *)(v195 + v388 + 6LL * (*(_BYTE *)(v195 + v387 + 1) & 0xF));
          v518 = v2;
          v390 = (unsigned __int8 *)(v195 + v387);
          do {
            if(*v390 != *v23)
              break;
            v390 = (unsigned __int8 *)(v195 + *((unsigned int *)v390 + 2));
            v388 = *((unsigned int *)v390 + 1);
            v389 = *(unsigned __int8 *)(v195 + v388 + 6LL * (v390[1] & 0xF));
          } while(v389 == v567);
          v503 = v388;
          v501 = v390;
          v2 = v518;
          v568 = v389;
          __dword_141133678 = v389;
        } else {
          __dword_141133678 = *(unsigned __int8 *)(v195 + *(unsigned int *)(v195 + v387 + 4) + 6LL * (*(_BYTE *)(v195 + v387 + 1) & 0xF));
        }
        v391 = v23[1];
        if(v212 >= 16 * v209 || (v23[1] & 0x40) != 0) {
          __qword_141133620 = (__int64)v214;
          v502 = *((unsigned __int16 *)v23 + 1);
        } else {
          v392 = *v501 + 1;
          v393 = (unsigned __int8 *)(v195 + v503);
          v394 = v392;
          do {
            __byte_141108A20[*v393] = v393[1];
            v393 += 6;
            --v394;
          } while(v394);
          v395 = *((unsigned __int16 *)v23 + 1);
          v522 = v170;
          v212 = 0;
          v524 = v65;
          v519 = v2;
          v396 = v395;
          v397 = 5 * v532 + 5;
          v398 = (5 * ((unsigned int)(2 * v397 - v395) >> 31) + 8) * v392 + *((unsigned __int16 *)v501 + 1);
          do {
            v399 = *--v213;
            v401 = *(unsigned __int8 *)(*v213 + 1);
            v402 = v396 + 2 * v397;
            LOBYTE(v403) = *(_BYTE *)(*v213 + 1);
            if((int)(v401 * v398) < v402 * (unsigned __int8)__byte_141108A20[*(unsigned __int8 *)*v213] && *(unsigned __int8 *)(*v213 + 1) <= 0xE4u) {
              v404 = v395 - v401;
              v400 = (unsigned __int8)__byte_141108A20[*(unsigned __int8 *)*v213];
              v403 = ((v402 - v401) * (3 * v401 + v400) + 3 * (v402 - v401) + v398 - v400 - 4) / (3 * (v402 - v401) + v398 - v400);
              if(v403 > v401 + 11)
                v403 = v401 + 11;
              v395 = v403 + v404;
              *((_WORD *)v23 + 1) = v395;
              *(_BYTE *)(v399 + 1) = v403;
              v396 = (unsigned __int16)v395;
            }
            v212 += (unsigned __int8)v403;
          } while(v213 != __qword_14025F220);
          v170 = v522;
          v65 = v524;
          v391 = v23[1];
          v387 = *((unsigned int *)v23 + 2);
          v502 = v396;
          v2 = v519;
          __qword_141133620 = (__int64)v213;
        }
        v405 = *(unsigned __int8 *)(v387 + v195);
        v535 = v405 + 1;
        v499 = v2 != __dword_140029540[__dword_14113369C];
        v496 = v2 != __dword_140029540[v568];
        v537 = __dword_141133698;
        v498 = 2 * v405 + 2;
        v500 = (unsigned __int8)__byte_1400292BE[v531];
        v406 = (int)((((16 * (unsigned int)*v23 - v502) >> 26) & 0xFFFFFFE0) + (__dword_141133698 & 3) + 8 * (v499 | (v2 != __dword_140029540[*(unsigned __int8 *)(v195 + *(unsigned int *)(v387 + v195 + 4) + 6LL * (*(_BYTE *)(v387 + v195 + 1) & 0xF))])) + 4 * (v531 * v212 > v502 * v209) + (v391 & 0xC0) + 16 * (v209 >= 2 * v405 - 2 * v532));
        v497 = v406;
        v407 = (v500 << 11) + 8LL * (int)v406;
        v408 = (char *)__dword_141128C20 + v407;
        v409 = *(int *)((char *)__dword_141128C20 + v407);
        v410 = *(unsigned __int16 *)((char *)&__word_141128C24 + v407);
        __qword_1411335B0 = (__int64)__dword_141128C20 + v407;
        __dword_141133680 = v409;
        v504 = v410;
        __dword_14113367C = v410;
        if(v410 > 0x8000 || v409 > 0x80000) {
          v446 = v406;
          v530 = v65;
          __sub_140010730((char *)__dword_141128C20 + v407);
          v65 = v530;
          LODWORD(v406) = v446;
          LOWORD(v410) = *((_WORD *)v408 + 2);
        }
        v411 = *((unsigned __int16 *)v408 + 3);
        *((_WORD *)v408 + 2) = v411 + v410;
        v536 = __dword_1411336E0;
        if(v411 > 0x78) {
          v472 = &__dword_141108B20[2048 * v500 + 16 * (unsigned int)(*v23 + v498 > __dword_1411336E0) + 2 * ((unsigned __int8)v406 & 0xF3) + 2 * (unsigned int)(v496 << 9) + 2 * (unsigned int)(v499 << 8) + 2 * (unsigned __int64)(4 * (unsigned int)(v533 < 10))];
          __qword_1411335A8 = (__int64)v472;
          v467 = v411 < 0x100;
          v470 = (v411 < 0x100) + 1;
          v412 = *((_DWORD *)v408 + 512);
          __qword_141133618 = (__int64)(v408 + 2048);
          v413 = (v412 + *((_DWORD *)v408 - 512)) >> v470;
          __qword_141133590 = (__int64)(v408 - 2048);
          v471 = v412;
          v414 = *((unsigned __int16 *)v408 - 1022);
          v409 += v413;
          v415 = *((unsigned __int16 *)v408 + 1026);
          __dword_141133680 = v409;
          v504 += (unsigned int)(v415 + v414) >> v470;
          __dword_14113367C = v504;
          if(v415 > 0x8000 || v471 > 0x80000) {
            v445 = v406;
            v529 = v65;
            __sub_140010730(v408 + 2048);
            v65 = v529;
            LODWORD(v406) = v445;
            v415 = *((unsigned __int16 *)v408 + 1026);
          }
          v416 = *((unsigned __int16 *)v408 - 1022);
          v417 = (*((unsigned __int16 *)v408 + 1027) >> v470) + v415;
          __dword_1411336EC = *((unsigned __int16 *)v408 + 1027) >> v470;
          *((_WORD *)v408 + 1026) = v417;
          if(v416 > 0x8000 || *((_DWORD *)v408 - 512) > 0x80000u) {
            v444 = v406;
            v528 = v65;
            __sub_140010730(v408 - 2048);
            v65 = v528;
            LODWORD(v406) = v444;
            LOWORD(v416) = *((_WORD *)v408 - 1022);
          }
          __dword_1411336B8 = *((unsigned __int16 *)v408 - 1021) >> v470;
          *((_WORD *)v408 - 1022) = __dword_1411336B8 + v416;
          v469 = *((unsigned __int16 *)v408 + 3) < 0x200u;
          v418 = *((unsigned __int16 *)v472 + 2);
          if(v418 > 0x8000 || (unsigned int)*v472 > 0x80000) {
            v443 = v406;
            v527 = v65;
            __sub_140010730(v472);
            v65 = v527;
            LODWORD(v406) = v443;
            LOWORD(v418) = *((_WORD *)v472 + 2);
          }
          __dword_1411336C4 = *((unsigned __int16 *)v472 + 3) >> v469;
          *((_WORD *)v472 + 2) = __dword_1411336C4 + v418;
          v468 = (*((unsigned __int16 *)v408 + 3) < 0x400u) + v467 + 1;
          v419 = &v408[-8 * v497];
          v420 = *(unsigned __int16 *)&v419[8 * (int)(v406 ^ 2) + 4];
          v473 = (__int64)&v419[8 * (int)(v406 ^ 2)];
          __qword_141133598 = v473;
          if(v420 > 0x8000 || *(_DWORD *)v473 > 0x80000u) {
            v442 = v406;
            v526 = v65;
            __sub_140010730(v473);
            v65 = v526;
            LODWORD(v406) = v442;
            LOWORD(v420) = *(_WORD *)(v473 + 4);
          }
          __dword_1411336BC = *(unsigned __int16 *)(v473 + 6) >> v468;
          *(_WORD *)(v473 + 4) = __dword_1411336BC + v420;
          v474 = (__int64)&v419[8 * (int)(v406 ^ 8)];
          __qword_1411335A0 = v474;
          v421 = *(_WORD *)(v474 + 4);
          if(v421 > 0x8000u || *(_DWORD *)v474 > 0x80000u) {
            v525 = v65;
            __sub_140010730(v474);
            v65 = v525;
            v421 = *(_WORD *)(v474 + 4);
          }
          __dword_1411336C0 = *(unsigned __int16 *)(v474 + 6) >> v468;
          *(_WORD *)(v474 + 4) = __dword_1411336C0 + v421;
        }
        v422 = ((v504 >> 1) + v531 * v409) / v504 + 2;
        if(v531 < 24) {
          v574 = 0;
          goto LABEL_335;
        }
      }
      v574 = (int)(v422 + *((unsigned __int16 *)v23 + 1)) >> 1;
    LABEL_335:
      __dword_1411336F0 = v574;
      v62 = v212 + v422;
      __dword_1411336E8 = v62;
      __dword_141133660 = __dword_141133708;
      __dword_1411336E4 = v212 + v532 + 1;
      v66 = 0;
      v67 = __qword_14025F220;
      __dword_1411336CC = 0;
      __qword_141133620 = (__int64)__qword_14025F220;
      v423 = (v533 > 1) << 15;
      v571 = v423 + (__dword_1411336A0 & 0x7FFF);
      __dword_1411336A0 = v423 + (__dword_1411336A0 & 0xFFFF7FFF);
      __dword_1411336D0 = ((v533 > 3) << 15) + v423 + (__dword_1411336D0 & 0x4600);
      v424 = v533 < 3 || __dword_14113370C > v534 + 22;
      v575 = 1032 * (*((_DWORD *)v23 + 2) == 0) + 32 * (v537 & 3) + 2 * v424;
      __dword_1411336F4 = v575;
      __dword_14113368C = 32 * (*v23 + 28 > 2 * v536) + (((unsigned int)(v535 - v531 - 3) >> 24) & 0xFFFFFF80) + ((v533 > 2) << 13) + (((__dword_14113370C > 7) + (__dword_14113370C > 2) + 7 * (((unsigned int)(74 - v531) >> 31) + ((unsigned int)(2 - v531) >> 31)) + (v533 > 16)) << 12);
      while(1) {
        rc.EncodeNormalize(a1);
      LABEL_59:
        v68 = (unsigned __int8 *)*v67;
        v569 = v23;
        __qword_141133620 = (__int64)(v67 + 1);
        v69 = (v68[1] << 8) - v66;
        __qword_1411335D0 = (__int64)&__dword_1405DFC20;
        __qword_1411335C8 = (__int64)&__dword_1405DFC20;
        __qword_1411335E0 = (__int64)&__dword_1405DFC20;
        __qword_1411335E8 = (__int64)&__dword_1405DFC20;
        v573 = v69 / v62;
        v570 = v69 / v62;
        v576 = (unsigned int)(unsigned __int8)__byte_140029000[v570] + 1;
        v70 = *v68;
        __dword_14025F20C = 1 << *v68;
        __dword_14025F200 = ((v70 + __dword_14025F1FC) >> 5) + 0x2000;
        __dword_14025F208 = v65 + ((v70 + __dword_14025F204) >> 5);
        v71 = v70;
        v72 = v2 == __dword_14010A558[v70];
        v73 = v2 == __dword_14010A958[v70];
        v572 = ((__dword_14025F20C & __dword_14020F1FC[((v70 + __dword_14025F1FC) >> 5) + 0x2000]) != 0) + 2 * ((__dword_14025F20C & __dword_14021F1FC[v65 + ((v70 + __dword_14025F204) >> 5)]) != 0);
        __qword_141133628 = (__int64)&__qword_1411336F8;
        __qword_141133630 = (__int64)&__qword_1411336F8;
        v74 = v2 == __dword_14010A158[v70];
        v75 = v70 == __dword_14014AD6C || *(_DWORD *)(__qword_14002A150 + 512) < *(unsigned __int8 *)(__qword_14002A150 + (((_WORD)v70 - (_WORD)__dword_14014AD7C) & 0x1FF)) << 7;
        v76 = __dword_14014ADB0;
        v77 = __dword_1401CF1FC[256 * __dword_14014ADB0 + v70];
        if((unsigned int)(__dword_141133688 - v77) >= 0x20000) {
          v78 = 0x20000;
          __dword_14014ADF0 = 0x20000;
          __dword_14014ADEC = 0x20000;
          __dword_14014ADE8 = 0x20000;
        } else {
          __dword_14014ADEC = __dword_1401CF1FC[256 * __dword_14014ADB0 + v70];
          __dword_14014ADE8 = (unsigned __int8)__byte_14002A158[(v77 + 2) & 0x1FFFF];
          __dword_14014ADEC = __dword_141133688 - v77;
          v78 = __dword_141133688 - __dword_1401CF1FC[256 * v70 + __dword_14014ADE8];
          __dword_14014ADF0 = v78;
        }
        v79 = v575 | (((v78 < 0x220) << 12) + (v75 << 9) + ((v62 < v574) << 11) + 8 * (v70 == v568) + ((v70 == v567) << 10) + *((unsigned __int8 *)__xmmword_140028F00 + v70) + (v74 << 8) + 16 * (v70 == __dword_14014AD70) + 4 * (unsigned int)v73 + v72);
        v80 = (v576 << 16) + 8 * v79;
        v81 = (unsigned __int16 *)((char *)&__dword_1405DFC20 + v80);
        v82 = *(unsigned __int16 *)((char *)&__word_1405DFC24 + v80);
        __qword_1411335C0 = (__int64)&__dword_1405DFC20 + v80;
        if(v82 > 0x8000) {
          __sub_140010850((char *)&__dword_1405DFC20 + v80);
          v76 = __dword_14014ADB0;
          v82 = v81[2];
        }
        v83 = *v81;
        v84 = v81[3];
        __dword_141133680 = v83;
        v81[2] = v82 + v84;
        __dword_14113367C = v82;
        __dword_14014AD98 = v83;
        __dword_14014AD9C = v82;
        v85 = v572;
        if(v572)
          v85 = 1;
        __dword_1411336A0 = (((_WORD)v70 - v76) & 0x800) + (v85 << 12) + (v571 & 0xE200) + (((_WORD)__dword_14014ADB4 - (_WORD)v70) & 0x400) + ((v70 == __dword_14014ADDC) << 8) + (v70 & 0xE0) + 16 * (v70 == __dword_14014ADD0) + 8 * (v70 == __dword_14014ADCC) + 4 * (v70 == __dword_14014AD8C) + (v70 == __dword_14014AD80) + 2 * (v70 == __dword_14014AD88) + (((v82 < (unsigned int)(56 * v83)) + (v82 < (unsigned int)(6 * v83))) << 16);
        v86 = &__dword_14045FB20[2 * __dword_1411336A0];
        v87 = *v86;
        __qword_1411335D8 = (__int64)v86;
        v88 = v83 * (__int64)v87 / v86[1];
        v89 = __dword_141133680;
        v90 = 1 - __dword_141133680;
        if(v88 >= 1 - __dword_141133680) {
          v90 = 0x40000;
          if(v88 < 0x40000)
            v90 = v88;
        }
        v91 = *v86;
        v92 = v86[1];
        if((int)abs32(*v86) > 0x40000) {
          v91 >>= 1;
          *v86 = v91;
          v92 >>= v92 > 4096;
        }
        v93 = v91 - v89;
        *v86 = v93;
        v94 = v89 + v92 + 2;
        if(v94 > 0x40000000) {
          *v86 = v93 >> 1;
          v94 >>= 1;
        }
        v86[1] = v94;
        v95 = v90 + __dword_14113367C;
        v96 = v90 + v89;
        v97 = *(unsigned __int16 *)(__qword_1411335C0 + 6);
        if(v97 <= 8) {
          __dword_14113367C = v95;
          __dword_141133680 = v96;
        } else {
          v553 = __qword_1411335C0;
          v557 = *(unsigned __int16 *)(__qword_1411335C0 + 6);
          v551 = v23;
          v558 = v68;
          v559 = v70;
          v560 = v70;
          v98 = &__dword_141130D20[512 * (unsigned __int64)(unsigned __int8)__byte_140029440[v570] + 2 * (unsigned int)(v572 << 6)];
          v99 = &v98[2 * (v79 & 0x1F)];
          v100 = v99[1];
          __qword_141133630 = (__int64)v99;
          v101 = &v98[2 * (((unsigned int)v79 >> 4) & 0x1F) + 64];
          __qword_141133628 = (__int64)v101;
          v102 = v97 < 0x150;
          v103 = v97 < 0x48;
          v104 = v101[1];
          v105 = v103;
          v106 = v100 * (v573 + 2);
          LODWORD(v98) = v104 * (v573 + 2);
          v107 = v104 + v100;
          v108 = *v101;
          v109 = v106 >> 8;
          v110 = (unsigned int)v98 >> 8;
          v111 = (v107 >> 11) & 0xFFFFFFFE;
          v103 = v107 < 0xC000;
          v112 = *v99;
          if(!v103)
            v111 = 24;
          v113 = v109 + v110;
          v114 = (unsigned __int8)__byte_140022930[v111];
          v115 = (unsigned __int8)__byte_140022931[v111];
          if(v109 + v110 >= 0x6000)
            v113 = 24576;
          v116 = ((unsigned int)(v114 * v113 + v115 * (*v101 + *v99)) >> (v102 + 7)) + v96;
          v117 = v102 + v105 + 3;
          v118 = ((unsigned int)(192 * (v114 + v115)) >> v102) + v95;
          __dword_141133700 = 0x3000u >> v117;
          v119 = v102 + 4;
          __dword_141133704 = 0x3000u >> v119;
          v70 = v559;
          *v99 = v112 - (v112 >> v117);
          v23 = v551;
          v99[1] -= (v109 + 7) >> v119;
          *v101 = v108 - (v108 >> v117);
          v120 = (v110 + 7) >> v119;
          v121 = (unsigned __int16 *)v553;
          v122 = v553 - 0x10000;
          v101[1] -= v120;
          v68 = v558;
          v123 = *(unsigned __int16 *)(v553 - 0x10000);
          v124 = *(unsigned __int16 *)(v553 - 0x10000 + 4);
          v125 = v553 + 0x10000;
          v126 = *(unsigned __int16 *)(v553 + 0x10000);
          __qword_1411335E8 = v553 + 0x10000;
          v127 = v123 + v126;
          v128 = *(unsigned __int16 *)(v553 + 65540);
          __qword_1411335E0 = v553 - 0x10000;
          v96 = (v127 >> 3) + v116;
          __dword_141133680 = v96;
          v95 = ((unsigned int)(v128 + v124) >> 3) + v118;
          v71 = v560;
          v129 = (v557 < 0x30) + 1;
          __dword_14113367C = v95;
          if(v128 > 0x8000) {
            v450 = v95;
            __sub_140010850(v553 + 0x10000);
            v95 = v450;
            v121 = (unsigned __int16 *)v553;
            v122 = v553 - 0x10000;
            v129 = (v557 < 0x30) + 1;
            v125 = v553 + 0x10000;
            LOWORD(v128) = *(_WORD *)(v553 + 65540);
          }
          __dword_1411336B0 = *(unsigned __int16 *)(v125 + 6) >> v129;
          *(_WORD *)(v125 + 4) = __dword_1411336B0 + v128;
          v130 = *(_WORD *)(v122 + 4);
          if(v130 > 0x8000u) {
            v451 = v129;
            v452 = v122;
            v556 = v121;
            v449 = v95;
            __sub_140010850(v122);
            v95 = v449;
            v121 = v556;
            v122 = v452;
            v129 = v451;
            v130 = *(_WORD *)(v452 + 4);
          }
          __dword_1411336B4 = *(unsigned __int16 *)(v122 + 6) >> v129;
          *(_WORD *)(v122 + 4) = __dword_1411336B4 + v130;
          v131 = v121[3];
          if(v131 > 0x48) {
            __qword_1411335C8 = (__int64)(v121 + 0x10000);
            v132 = (v131 < 0x1E0) + (v131 < 0x3D0) + 1;
            v133 = v121[65538];
            if(v133 > 0x8000u) {
              v440 = v132;
              v555 = v121;
              v448 = v95;
              __sub_140010850(v121 + 0x10000);
              v95 = v448;
              v121 = v555;
              v132 = v440;
              v133 = v555[65538];
            }
            __dword_1411336A8 = v121[65539] >> v132;
            __qword_1411335D0 = (__int64)(v121 - 0x10000);
            v121[65538] = __dword_1411336A8 + v133;
            v134 = *(v121 - 65534);
            if(v134 > 0x8000u) {
              v439 = v132;
              v554 = v121;
              v447 = v95;
              __sub_140010850(v121 - 0x10000);
              v95 = v447;
              v121 = v554;
              v132 = v439;
              v134 = *(v554 - 65534);
            }
            __dword_1411336AC = *(v121 - 65533) >> v132;
            *(v121 - 65534) = __dword_1411336AC + v134;
          }
        }
        __dword_141133680 = 60416LL * v96 / v95;
        __dword_14113367C = 60416;
        __dword_14014ADA0 = __dword_141133680;
        __dword_14014ADA4 = 60416;
        v135 = &__dword_1407A0720[0x100000 * (__dword_14014AD68 < (unsigned int)__dword_14014ADB8) + 0x80000 * (v70 == __dword_14014AD70) + 0x20000 * __dword_141133694 + 512 * __dword_14014ADB4 + 2 * v71 + 2 * (unsigned __int64)((unsigned __int8)((unsigned int)__dword_14014ADEC < 0x78) << 17)];
        v136 = *v135;
        __qword_1411335F8 = (__int64)v135;
        v137 = __dword_141133680 * (__int64)v136 / v135[1];
        v138 = __dword_141133680;
        v139 = 1 - __dword_141133680;
        if(v137 >= 1 - __dword_141133680) {
          v139 = 0x40000;
          if(v137 < 0x40000)
            v139 = __dword_141133680 * (__int64)v136 / v135[1];
        }
        v140 = *v135;
        if((int)abs32(*v135) > 0x80000) {
          v141 = v135[1];
          v140 >>= 1;
          *v135 = v140;
          v135[1] = v141 >> (v141 > 0x2000);
        }
        v142 = v135[1];
        v143 = v140 - v138;
        *v135 = v143;
        v144 = v138 + v142 + 1120;
        v135[1] = v144;
        if(v144 > 0x40000000) {
          *v135 = v143 >> 1;
          v135[1] = v144 >> 1;
        }
        v145 = *((unsigned __int8 *)__xmmword_140028F00 + v71);
        __dword_14113367C += v139;
        v146 = v139 + v138;
        __dword_141133680 = v146;
        __dword_14014ADA8 = __dword_14113367C;
        v147 = ((17 * v146 < (unsigned int)__dword_14113367C) << 13) + (((unsigned __int16)__dword_14014ADE8 - (unsigned __int16)v70) & 0x1000) + 16 * (v70 == __dword_14113369C) + 8 * (v70 == __dword_14014AD70) + 4 * (v70 == __dword_14014AD94) + (v70 == __dword_14014AD84) + (((unsigned int)__dword_14014ADF0 < 0x29) << 7) + __dword_1411336D0 + (__dword_1411336A0 & 0x800) + 2 * (v145 + (v70 == __dword_14014AD90)) + 32 * (((unsigned int)__dword_14014ADEC < 0xA800) + ((unsigned int)__dword_14014ADEC < 0x600) + ((unsigned int)__dword_14014ADEC < 0xD));
        v148 = &__dword_140FA0820[2 * v147];
        v149 = *v148;
        __qword_141133600 = (__int64)v148;
        v150 = (__dword_14113367C - v146) * (__int64)v149 / __dword_140FA0824[2 * v147];
        v151 = v146;
        v152 = v146 - __dword_14113367C + 1;
        if(v150 >= v152) {
          v152 = 0x40000;
          if(v150 < 0x40000)
            v152 = (__dword_14113367C - v146) * (__int64)v149 / __dword_140FA0824[2 * v147];
        }
        v153 = *v148;
        v154 = v152 + __dword_14113367C;
        __dword_14113367C += v152;
        v155 = __dword_140FA0824[2 * v147];
        if((int)abs32(v153) > 0x100000) {
          v153 >>= 1;
          *v148 = v153;
          v155 >>= v155 > 0x2000;
        }
        v73 = __dword_14010A558[v71] == __dword_141133664;
        v156 = __qword_14002A150;
        *v148 = v153 + 2 * v146;
        __dword_140FA0824[2 * v147] = v155 + 1;
        v157 = *(unsigned __int8 *)(v156 + (((_WORD)v70 - (_WORD)__dword_14014AD7C) & 0x1FF));
        v158 = __qword_141133588 + *(_DWORD *)(v68 + 2) - __qword_141133560 + 736;
        v159 = (*(_DWORD *)(v156 + 512) < (unsigned int)(384 * v157)) + (*(_DWORD *)(v156 + 512) < (unsigned int)(58 * v157)) + (*(_DWORD *)(v156 + 512) < (unsigned int)(22 * v157));
        v160 = (unsigned __int8)__byte_1400EA158[__dword_14014ADF8];
        __dword_14014ADAC = v154;
        v161 = (int)(4 * (v158 < 0x2E0) + v73 + (v147 & 0x700) + __dword_14113368C + (((unsigned int)__dword_14014ADEC < 0x2C00) << 11) + 2 * (v70 == v160) + 8 * v159);
        v162 = &__dword_141060920[2 * v161];
        v163 = *v162;
        __qword_141133608 = (__int64)v162;
        v164 = (v154 - v151) * (__int64)v163 / __dword_141060924[2 * v161];
        v165 = __dword_141133680;
        v166 = __dword_141133680 - __dword_14113367C + 1;
        if(v164 >= v166) {
          v166 = 0x80000;
          if(v164 < 0x80000)
            v166 = v164;
        }
        v167 = *v162;
        v168 = v166 + __dword_14113367C;
        __dword_14113367C += v166;
        v169 = v162[1];
        if((int)abs32(v167) > 0x100000) {
          v167 >>= 1;
          *v162 = v167;
          v169 >>= v169 > 0x2000;
        }
        *v162 = v167 + v151;
        v162[1] = v169 + 2;
        v171 = *v68;
        v172 = rc.GetSubRange(v165, v168);
        if(v171 == v566) {
          rc.EncodeMatched(v172);
          v173 = __qword_141133630;
          v174 = __dword_141133700;
          v175 = __dword_141133704;
          v176 = (_DWORD *)__qword_141133628;
          v177 = __dword_14014ADEC;
          *(_DWORD *)__qword_141133630 += __dword_141133700;
          *(_DWORD *)(v173 + 4) += v175;
          v178 = v23[1];
          *v176 += v174;
          v176[1] += v175;
          v179 = (v178 & 0x40) == 0 || v177 > 0xE00;
          v180 = (v177 > 0x4800) + (v177 > 0x380) + (v177 > 0x80) + v179 + 4;
          v23[1] = v178 & 0xF0;
          *((_WORD *)v23 + 1) += v180;
          v68[1] += v180;
          v181 = __qword_141133588 + *((unsigned int *)v23 + 1);
          if(v68 == (unsigned __int8 *)v181) {
            v23 = (unsigned __int8 *)__qword_141133580;
            if(__qword_141133580 == __qword_141133638)
              __dword_141133694 = v168 < 2 * v165;
          } else {
            v565 = *(_WORD *)v68;
            v182 = *(_DWORD *)(v68 + 2);
            do {
              v183 = *((_DWORD *)v68 - 1);
              *(_WORD *)v68 = *((_WORD *)v68 - 3);
              *(_DWORD *)(v68 + 2) = v183;
              v68 -= 6;
            } while((unsigned __int64)v68 > v181);
            *(_WORD *)v68 = v565;
            v23 = (unsigned __int8 *)__qword_141133580;
            *(_DWORD *)(v68 + 2) = v182;
          }
          v184 = v68[1];
          __qword_141133610 = (__int64)v68;
          if(v184 > 244) {
            __sub_14000B750(v569);
            v68 = (unsigned __int8 *)__qword_141133610;
          }
          if(v68)
            goto LABEL_250;
        } else {
          v425 = __qword_141133610;
          rc.EncodeNotMatched(v172);
          v426 = v68[1];
          __dword_1411336E8 -= v426;
          __dword_14020F1FC[__dword_14025F200] &= ~__dword_14025F20C;
          __dword_14021F1FC[__dword_14025F208] &= ~__dword_14025F20C;
          if(v425) {
            v23 = (unsigned __int8 *)__qword_141133580;
            __qword_141133610 = 0;
            __dword_141133694 = 0;
          } else if((unsigned __int64)v68 <= __qword_141133588 + (unsigned __int64)*((unsigned int *)v23 + 1)) {
            v23 = (unsigned __int8 *)__qword_141133580;
          } else {
            v23 = (unsigned __int8 *)__qword_141133580;
            __dword_1411336C8 = 0x8000;
          }
        }
        if(!--v564) {
          v185 = __dword_1411336E4;
          v186 = (_DWORD *)__qword_1411335B0;
          v187 = __qword_141133618;
          v188 = __dword_1411336EC;
          v189 = *v23;
          v190 = *(unsigned __int16 *)(__qword_1411335B0 + 6);
          *(_WORD *)(__qword_1411335B0 + 4) -= v190;
          v191 = v185 / (v189 + 1);
          v192 = __qword_141133590;
          v193 = __dword_1411336B8;
          *v186 += v191 * v190;
          v194 = __qword_141133598;
          *(_WORD *)(v187 + 4) -= v188;
          *(_DWORD *)v187 += v188 * v191;
          *(_WORD *)(v192 + 4) -= v193;
          *(_DWORD *)v192 += v193 * v191;
          LODWORD(v192) = __dword_1411336BC;
          v195 = __qword_141133588;
          *(_WORD *)(v194 + 4) -= __dword_1411336BC;
          v196 = v192 * v191;
          LODWORD(v192) = __dword_1411336C0;
          *(_DWORD *)v194 += v196;
          v197 = (_DWORD *)__qword_1411335A0;
          v198 = v192 * v191;
          *(_WORD *)(__qword_1411335A0 + 4) -= v192;
          v199 = __qword_1411335A8;
          *v197 += v198;
          result = __dword_1411336C4 * v191;
          *(_WORD *)(v199 + 4) -= __dword_1411336C4;
          *(_DWORD *)v199 += result;
          v201 = *v23;
          __dword_1411336DC = v201;
          goto LABEL_128;
        }
        v65 = __dword_1411336C8;
        v66 = __dword_1411336CC;
        v567 = __dword_141133674;
        v2 = __dword_141133664;
        v568 = __dword_141133678;
        v67 = (__int64 *)__qword_141133620;
        v571 = __dword_1411336A0;
        v62 = __dword_1411336E8;
        v574 = __dword_1411336F0;
        v575 = __dword_1411336F4;
      }
    } else {
      rc.EncodeMatched(v351);
    }
    __qword_141133610 = (__int64)(v3 + 2);
    v352 = v3[3];
    v353 = v352 == 1 && v347 < 4 * v344;
    v354 = (_DWORD *)__qword_141133598;
    v355 = __dword_1411336BC;
    v356 = (_DWORD *)__qword_1411335A0;
    v357 = __dword_1411336C0;
    v358 = (_DWORD *)__qword_1411335A8;
    v359 = __dword_1411336C4;
    v360 = (_DWORD *)__qword_1411335B0;
    v361 = __dword_1411336C8;
    __dword_141133694 = 1;
    v362 = (v352 - 127 < 0) + v352 + v353;
    v363 = __qword_1411335F0;
    v3[3] = v362;
    v364 = (_DWORD *)__qword_141133590;
    v365 = __dword_1411336B8;
    *(_DWORD *)v363 += 1568;
    LODWORD(v363) = __dword_1411336CC;
    *v364 += v365;
    v366 = (_DWORD *)__qword_1411335B8;
    *v354 += v355;
    *v356 += v357;
    *v358 += v359;
    *v360 += v361;
    *v366 += v363;
    v23 = (unsigned __int8 *)__qword_141133580;
  LABEL_250:
    result = __sub_140007B80(v23);
    rc.EncodeNormalize(a1);
    v2 = __dword_141133664;
  } while(__dword_141133664);
  return result;
}
PROBE_DECL(__sub_14000C560)
int __sub_14000C560(FILE *a1, FILE *a2) {
  PROBE_HIT(__sub_14000C560);
  int v2;
  unsigned __int8 *v3;
  int v4;
  int v5;
  __int64 v6;
  __int64 v7;
  unsigned __int8 v8;
  int v9;
  __int64 *v10;
  __int64 v11;
  __int64 *v12;
  unsigned __int8 *v13;
  int v14;
  int v15;
  __int64 *v16;
  __int64 *i;
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
  unsigned __int8 v28;
  int v29;
  int v30;
  unsigned __int8 *v31;
  unsigned __int8 *v32;
  int v33;
  int v34;
  int v35;
  int v36;
  int v37;
  int v38;
  unsigned int v39;
  unsigned __int8 *v40;
  int v41;
  int v42;
  int v43;
  int v44;
  int v45;
  char v46;
  __int64 v47;
  int *v48;
  int v49;
  int v50;
  unsigned int v51;
  int v52;
  unsigned int v53;
  int v54;
  int v55;
  int v56;
  int v57;
  int v58;
  int *v59;
  int v60;
  __int64 v61;
  int v62;
  unsigned __int16 v63;
  unsigned int v64;
  unsigned int v65;
  int v66;
  int v67;
  int v68;
  __int64 *v69;
  unsigned int v70;
  unsigned __int8 *v71;
  int v72;
  unsigned int v73;
  __int64 v74;
  __int64 v75;
  bool v76;
  int v77;
  int v78;
  __int16 v79;
  unsigned int v80;
  __int64 v81;
  int *v82;
  int v83;
  int v84;
  __int16 v85;
  int v86;
  int *v87;
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
  unsigned int v98;
  int *v99;
  int *v100;
  int v101;
  int *v102;
  unsigned int v103;
  char v104;
  bool v105;
  int v106;
  bool v107;
  unsigned int v108;
  unsigned int v109;
  unsigned int v110;
  unsigned int v111;
  __int64 v112;
  unsigned int v113;
  unsigned int v114;
  int v115;
  int v116;
  unsigned int v117;
  char v118;
  unsigned int v119;
  char v120;
  __int64 v121;
  int v122;
  int v123;
  unsigned __int16 *v124;
  int v125;
  unsigned int v126;
  int v127;
  char v128;
  unsigned __int16 v129;
  unsigned int v130;
  char v131;
  unsigned __int16 v132;
  unsigned __int16 v133;
  int v134;
  int *v135;
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
  int *v148;
  int v149;
  int v150;
  int v151;
  int v152;
  int v153;
  int v154;
  int v155;
  __int64 v156;
  int v157;
  unsigned int v158;
  int v159;
  int v160;
  int v161;
  int v162;
  int *v163;
  int v164;
  int v165;
  int v166;
  int v167;
  int v168;
  unsigned int v169;
  int v170;
  unsigned int v171;
  int v172;
  unsigned int v173;
  __int64 v174;
  int v175;
  int v176;
  _DWORD *v177;
  unsigned int v178;
  unsigned __int8 v179;
  __int16 v180;
  __int16 v181;
  unsigned __int64 v182;
  int v183;
  int v184;
  int v185;
  unsigned int v186;
  _DWORD *v187;
  __int64 v188;
  int v189;
  int v190;
  int v191;
  unsigned int v192;
  int v193;
  __int64 v194;
  __int64 v195;
  _DWORD *v196;
  int v197;
  int v198;
  _DWORD *v199;
  int v200;
  __int64 v201;
  __int64 v202;
  int v203;
  unsigned int v204;
  int v205;
  int v206;
  int v207;
  int v208;
  __int64 v209;
  int v210;
  int v211;
  char v212;
  __int64 *v213;
  __int64 *v214;
  __int64 v215;
  int v216;
  unsigned __int8 *v217;
  __int64 *v218;
  int v219;
  int v220;
  __int64 v221;
  __int64 v222;
  int v223;
  unsigned __int8 *v224;
  __int64 *v225;
  unsigned __int8 *v226;
  __int64 v227;
  __int64 v228;
  __int64 v229;
  unsigned __int8 *v230;
  unsigned int v231;
  unsigned __int8 v232;
  int v233;
  unsigned __int8 *v234;
  unsigned __int8 *v235;
  int v236;
  unsigned int v237;
  unsigned int v238;
  __int64 v239;
  int v240;
  unsigned __int8 *j;
  unsigned int v242;
  int v243;
  __int16 v244;
  unsigned int v245;
  unsigned int v246;
  unsigned int v247;
  unsigned int v248;
  __int64 v249;
  int v250;
  __int64 v251;
  unsigned int v252;
  int v253;
  int v254;
  int v255;
  int v256;
  unsigned int v257;
  BOOL v258;
  int v259;
  __int64 v260;
  unsigned int v261;
  BOOL v262;
  int v263;
  __int64 v264;
  int *v265;
  __int64 v266;
  int *v267;
  int v268;
  int v269;
  unsigned __int16 v270;
  int v271;
  BOOL v272;
  int v273;
  int v274;
  int v275;
  int v276;
  int *v277;
  int v278;
  int v279;
  int v280;
  int v281;
  int v282;
  int v283;
  int v284;
  int v285;
  unsigned __int16 *v286;
  unsigned int v287;
  int v288;
  int v289;
  int v290;
  int *v291;
  unsigned int v292;
  int v293;
  int v294;
  __int64 v295;
  unsigned __int16 v296;
  char v297;
  __int64 v298;
  unsigned __int16 v299;
  __int64 v300;
  int v301;
  int v302;
  int v303;
  unsigned int v304;
  int v305;
  unsigned __int16 v306;
  __int64 v307;
  unsigned __int16 v308;
  char v309;
  __int64 v310;
  unsigned __int16 v311;
  __int64 v312;
  unsigned __int16 v313;
  __int64 v314;
  unsigned __int16 v315;
  __int64 v316;
  unsigned __int16 v317;
  __int64 v318;
  unsigned __int16 v319;
  int *v320;
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
  int *v332;
  int v333;
  int v334;
  int v335;
  int v336;
  int v337;
  int v338;
  int v339;
  int v340;
  __int16 v341;
  unsigned int v342;
  BOOL v343;
  int v344;
  int v345;
  int v346;
  int *v347;
  int v348;
  int v349;
  int v350;
  int v351;
  int v352;
  unsigned int v353;
  int v354;
  unsigned int v355;
  unsigned int v356;
  int v357;
  bool v358;
  _DWORD *v359;
  _DWORD *v360;
  _DWORD *v361;
  int v362;
  _DWORD *v363;
  int v364;
  unsigned __int8 v365;
  __int64 v366;
  int v367;
  int v368;
  _DWORD *v369;
  int v370;
  unsigned int m;
  int v372;
  int v373;
  int v374;
  unsigned int v375;
  int v376;
  int v377;
  int v378;
  int v379;
  __int64 *v380;
  __int64 *k;
  __int64 v382;
  __int64 v383;
  __int64 v384;
  unsigned __int8 *v385;
  int v386;
  unsigned __int8 v387;
  int v388;
  unsigned __int8 *v389;
  int v390;
  int v391;
  int v392;
  int v393;
  int v394;
  unsigned int v395;
  __int64 v396;
  int v397;
  int v398;
  int v399;
  unsigned int v400;
  int v401;
  int v402;
  __int64 v403;
  __int64 v404;
  char *v405;
  unsigned int v406;
  int v407;
  unsigned int v408;
  unsigned int v409;
  unsigned int v410;
  int v411;
  int v412;
  int v413;
  int v414;
  unsigned int v415;
  char *v416;
  int v417;
  unsigned __int16 v418;
  unsigned int v419;
  int v420;
  BOOL v421;
  __int64 v422;
  __int64 v423;
  int v424;
  __int64 v425;
  int v426;
  int v427;
  unsigned __int16 *v429;
  unsigned __int16 *v430;
  unsigned __int16 *v431;
  unsigned __int16 *v432;
  unsigned __int16 *v433;
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
  int *v460;
  int *v461;
  int *v462;
  int *v463;
  int v464;
  int v465;
  int v466;
  int v467;
  bool v468;
  char v469;
  char v470;
  char v471;
  unsigned int v472;
  int *v473;
  __int64 v474;
  __int64 v475;
  unsigned __int8 *v476;
  int v477;
  int v478;
  int v479;
  __int64 v480;
  char v481;
  bool v482;
  __int64 v483;
  char v484;
  unsigned int v485;
  int *v486;
  __int64 v487;
  __int64 v488;
  int v489;
  int v490;
  int v491;
  int v492;
  int v493;
  int v494;
  __int64 *v495;
  __int64 *v496;
  unsigned __int8 *v497;
  int v498;
  __int64 v499;
  int v500;
  int v501;
  __int64 v502;
  int v503;
  __int64 v504;
  unsigned int v505;
  int v506;
  __int64 v507;
  BOOL v508;
  unsigned int v509;
  __int64 v510;
  int v511;
  __int64 v512;
  int v513;
  unsigned __int8 v514;
  unsigned int v515;
  unsigned int v516;
  int v517;
  int v518;
  int v519;
  int v520;
  __int64 v521;
  int v522;
  int v523;
  unsigned int v524;
  unsigned int v525;
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
  unsigned __int8 *v543;
  int v544;
  unsigned int v545;
  __int64 v546;
  int v547;
  int v548;
  unsigned __int8 *v549;
  int v550;
  unsigned int v551;
  unsigned int v552;
  unsigned int v553;
  __int64 v554;
  unsigned int v555;
  int v556;
  unsigned __int8 *v557;
  unsigned __int8 *v558;
  unsigned __int16 *v559;
  unsigned int v560;
  unsigned __int8 *v561;
  unsigned int v562;
  __int64 v563;
  int v566;
  int v567;
  __int16 v568;
  int v569;
  int v570;
  unsigned int v571;
  __int16 v572;
  int v573;
  unsigned __int8 *v574;
  __int64 v575;
  int v576;
  int v577;
  __int64 v578;
  int v579;
  BOOL v580;
  v2 = __dword_141133664;
  do {
    v3 = (unsigned __int8 *)__qword_141133580;
    v4 = *(unsigned __int8 *)__qword_141133580;
    v5 = v4;
    if(*(_BYTE *)__qword_141133580) {
      v6 = __qword_141133588;
      v7 = *(unsigned int *)(__qword_141133580 + 4);
      v8 = *(_BYTE *)(__qword_141133580 + 1);
      if(!*(_BYTE *)(v7 + __qword_141133588 + 6LL * (v8 & 0xF) + 1)) {
        __sub_14000C0E0((unsigned __int8 *)__qword_141133580);
        v4 = *v3;
        v7 = *((unsigned int *)v3 + 1);
        v8 = v3[1];
      }
      v566 = v5;
      v546 = v6;
      v547 = v4;
      v545 = v4 + 1;
      v9 = v4 + 1;
      v544 = v4 + 1;
      v548 = v4 + 1;
      v10 = &__qword_14025F228;
      __qword_14025F220[0] = v7 + v6 + 6LL * (v8 & 0xF);
      v11 = *(unsigned __int8 *)__qword_14025F220[0];
      v569 = *(unsigned __int8 *)__qword_14025F220[0];
      __dword_141133674 = v569;
      v12 = &__qword_14025F228;
      __dword_140029540[v11] = v2;
      v13 = (unsigned __int8 *)(v6 + v7 - 6);
      while(1) {
        v13 += 6;
        v14 = *v13;
        if(v569 != v14) {
          __dword_140029540[*v13] = v2;
          *v10++ = (__int64)v13;
          if(v14 == __dword_14014AD70) {
            v15 = 22;
            v16 = &__qword_14025F220[(__byte_14014AD5C & 7) == 0];
          LABEL_12:
            for(i = v10 - 1; i > v16; --i) {
              v18 = *(i - 1);
              if(v15 * *(unsigned __int8 *)(*i + 1) <= *(unsigned __int8 *)(v18 + 1))
                break;
              *(i - 1) = *i;
              *i = v18;
            }
            ++v12;
            goto LABEL_16;
          }
          if(v2 == __dword_14010A958[v14] || v2 == __dword_14010A558[v14]) {
            v16 = v12;
            v15 = 11 * (v2 == __dword_14010A958[v14]) + 7;
            goto LABEL_12;
          }
        }
      LABEL_16:
        if(!--v9) {
          v19 = v566;
          v20 = v548;
          v21 = v544;
          v22 = v545;
          v23 = v547;
          __dword_14014AD7C = *(unsigned __int8 *)__qword_14025F220[0];
          __dword_14010A158[(unsigned __int8)__byte_1400DA158[__dword_14014AD7C + (__dword_14014ADD0 << 8)]] = v2;
          __qword_1411335A8 = (__int64)__dword_1405DFC20;
          __qword_1411335A0 = (__int64)__dword_1405DFC20;
          __qword_141133598 = (__int64)__dword_1405DFC20;
          __qword_141133590 = (__int64)__dword_1405DFC20;
          __qword_141133618 = (__int64)__dword_1405DFC20;
          if(v545 == 256) {
            v44 = __dword_141133690;
            v46 = __dword_141133698;
            v45 = __dword_1411336E0;
            v66 = *((unsigned __int16 *)v3 + 1);
            __qword_1411335B0 = (__int64)__dword_1405DFC20;
            v570 = -1;
            __dword_141133678 = -1;
            __dword_1411336E4 = v66;
            v64 = v66 + 1;
            __dword_1411336E8 = v64;
          } else {
            v24 = *((unsigned int *)v3 + 2);
            v25 = *(unsigned __int8 *)(v24 + v546);
            v26 = *((unsigned __int16 *)v3 + 1);
            v27 = *(unsigned __int16 *)(v24 + v546 + 2);
            v28 = v3[1];
            __qword_141133620 = (__int64)__qword_14025F220;
            v21 = v25 + 1;
            v506 = v27;
            v514 = v28;
            v29 = v26;
            if((v25 + 1) * v26 > (int)(v27 * v545) || (v480 = *((unsigned int *)v3 + 1), *(unsigned __int8 *)(v480 + v546 + 1) >= 0x6Au)) {
              v511 = v26;
              v507 = v545;
            } else if((v514 & 0x40) != 0) {
              v511 = v26;
              v507 = v545;
            } else {
              v30 = v25 + 1;
              v517 = v2;
              v31 = (unsigned __int8 *)(v546 + *(unsigned int *)(v24 + v546 + 4) - 6);
              do {
                v31 += 6;
                --v30;
                __byte_141108A20[*v31] = v31[1];
              } while(v30);
              v507 = v545;
              v32 = (unsigned __int8 *)(v480 + v546 + 6LL * v545);
              v33 = 9 * v547 + 9 + 2 * v547 + 2;
              v34 = v29;
              v35 = v21 * (((unsigned int)(v33 - v29) >> 28) | 7) + v506;
              do {
                v32 -= 6;
                v37 = v32[1];
                if(v37 * v35 < (v33 + v34) * (unsigned __int8)__byte_141108A20[*v32] && v32[1] <= 0xE4u) {
                  v38 = v29 - v37;
                  v36 = (unsigned __int8)__byte_141108A20[*v32];
                  v39 = ((v33 + v34 - v37) * (3 * v37 + v36) + 3 * (v33 + v34 - v37) + v35 - v36 - 4) / (unsigned int)(3 * (v33 + v34 - v37) + v35 - v36);
                  if(v39 > v37 + 11)
                    v39 = v37 + 11;
                  v29 = v39 + v38;
                  *((_WORD *)v3 + 1) = v29;
                  v32[1] = v39;
                  v34 = (unsigned __int16)v29;
                }
                --v20;
              } while(v20);
              v21 = v25 + 1;
              v22 = v545;
              v2 = v517;
              v24 = *((unsigned int *)v3 + 2);
              v511 = v34;
              v23 = v547;
              v514 = v3[1];
            }
            v513 = *v3;
            v40 = (unsigned __int8 *)(v546 + v24);
            v41 = *(unsigned __int8 *)(v546 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
            v42 = v41;
            v570 = v41;
            if(v41 == v569) {
              v43 = *(unsigned __int8 *)(v546 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
              do {
                if(*v40 != *v3)
                  break;
                v40 = (unsigned __int8 *)(v546 + *((unsigned int *)v40 + 2));
                v42 = *(unsigned __int8 *)(v546 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
                v43 = v42;
              } while(v42 == v569);
              v570 = v43;
              __dword_141133678 = v42;
            } else {
              __dword_141133678 = *(unsigned __int8 *)(v546 + *((unsigned int *)v40 + 1) + 6LL * (v40[1] & 0xF));
            }
            v509 = v2 != __dword_140029540[__dword_14113369C];
            v508 = v2 != __dword_140029540[v41];
            v44 = __dword_141133690;
            v45 = __dword_1411336E0;
            v46 = __dword_141133698;
            v512 = (unsigned __int8)__byte_1400292BE[v507];
            v47 = (int)(8 * (v513 + 2 * v25 + 2 > __dword_1411336E0) + (__dword_141133698 & 3) + 4 * (__dword_141133690 < 10) + ((v2 != __dword_140029540[v570]) << 9) + (v509 << 8) + (((unsigned int)(8 * v23 + 8 + 4 * v23 + 4 - v511) >> 26) & 0xFFFFFFE0) + (v514 & 0xC0) + (((unsigned int)(3 * v25 + 3 - (4 * v23 + 4)) >> 27) & 0xFFFFFFF0));
            v510 = v47;
            v48 = &__dword_141108B20[2048 * v512 + 2 * v47];
            v49 = *v48;
            __qword_1411335B0 = (__int64)v48;
            v516 = v49;
            __dword_141133680 = v49;
            v50 = (unsigned __int16)__word_141108B24[4096 * v512 + 4 * v47];
            v515 = v50;
            __dword_14113367C = v50;
            if(v50 > 0x8000 || v516 > 0x80000) {
              v463 = &__dword_141108B20[2048 * v512 + 2 * v47];
              v467 = v47;
              v494 = __dword_1411336E0;
              v459 = __dword_141133690;
              __sub_140010730((__int64)v463);
              v44 = v459;
              v45 = v494;
              LODWORD(v47) = v467;
              v48 = v463;
              LOWORD(v50) = *((_WORD *)v463 + 2);
            }
            v51 = *((unsigned __int16 *)v48 + 3);
            *((_WORD *)v48 + 2) = v51 + v50;
            if(v51 > 0x80) {
              v489 = v45;
              v486 = &__dword_141128C20[512 * v512 + 16 * (v508 | v509) + 2 * ((unsigned __int8)v47 & 0xF3)];
              __qword_1411335A8 = (__int64)v486;
              v484 = v51 < 0x200;
              __qword_141133618 = (__int64)(v48 + 2048);
              v52 = *(v48 - 2048);
              v483 = (__int64)(v48 - 2048);
              v53 = v48[2048];
              __qword_141133590 = (__int64)(v48 - 2048);
              v485 = v53;
              v54 = ((v53 + v52) >> (v484 + 1)) + v516;
              v55 = *((unsigned __int16 *)v48 + 4098);
              v56 = *((unsigned __int16 *)v48 - 4094);
              v516 = v54;
              __dword_141133680 = v54;
              v57 = ((unsigned int)(v55 + v56) >> (v484 + 1)) + v515;
              v45 = v489;
              v515 = v57;
              __dword_14113367C = v57;
              v19 = v566;
              if(v55 > 0x8000 || v485 > 0x80000) {
                v462 = v48;
                v466 = v47;
                v458 = v44;
                __sub_140010730((__int64)(v48 + 2048));
                v44 = v458;
                v45 = v489;
                LODWORD(v47) = v466;
                v48 = v462;
                LOWORD(v55) = *((_WORD *)v462 + 4098);
              }
              __dword_1411336EC = *((unsigned __int16 *)v48 + 4099) >> v484;
              v58 = *((unsigned __int16 *)v48 - 4094);
              *((_WORD *)v48 + 4098) = __dword_1411336EC + v55;
              if(v58 > 0x8000 || (unsigned int)*(v48 - 2048) > 0x80000) {
                v461 = v48;
                v465 = v47;
                v493 = v45;
                v457 = v44;
                __sub_140010730(v483);
                v44 = v457;
                v45 = v493;
                LODWORD(v47) = v465;
                v48 = v461;
                LOWORD(v58) = *((_WORD *)v461 - 4094);
              }
              __dword_1411336B8 = *((unsigned __int16 *)v48 - 4093) >> v484;
              *((_WORD *)v48 - 4094) = __dword_1411336B8 + v58;
              v482 = *((unsigned __int16 *)v48 + 3) < 0x400u;
              v481 = v484 + v482;
              v59 = &v48[-2 * v510];
              v60 = LOWORD(v59[2 * (int)(v47 ^ 0x100) + 1]);
              v487 = (__int64)&v59[2 * (int)(v47 ^ 0x100)];
              __qword_141133598 = v487;
              if(v60 > 0x8000 || *(_DWORD *)v487 > 0x80000u) {
                v460 = v59;
                v464 = v47;
                v492 = v45;
                v456 = v44;
                __sub_140010730(v487);
                v44 = v456;
                v45 = v492;
                LODWORD(v47) = v464;
                v59 = v460;
                LOWORD(v60) = *(_WORD *)(v487 + 4);
              }
              __dword_1411336BC = *(unsigned __int16 *)(v487 + 6) >> v481;
              *(_WORD *)(v487 + 4) = __dword_1411336BC + v60;
              v61 = (__int64)&v59[2 * (int)(v47 ^ 0x200)];
              v62 = *(unsigned __int16 *)(v61 + 4);
              v488 = v61;
              __qword_1411335A0 = v61;
              if(v62 > 0x8000 || *(_DWORD *)v61 > 0x80000u) {
                v491 = v45;
                v455 = v44;
                __sub_140010730(v61);
                v44 = v455;
                v45 = v491;
                LOWORD(v62) = *(_WORD *)(v488 + 4);
              }
              __dword_1411336C0 = *(unsigned __int16 *)(v488 + 6) >> v481;
              *(_WORD *)(v488 + 4) = __dword_1411336C0 + v62;
              v63 = *((_WORD *)v486 + 2);
              if(v63 > 0x8000u || (unsigned int)*v486 > 0x80000) {
                v490 = v45;
                v454 = v44;
                __sub_140010730((__int64)v486);
                v44 = v454;
                v45 = v490;
                v63 = *((_WORD *)v486 + 2);
              }
              __dword_1411336C4 = *((unsigned __int16 *)v486 + 3) >> (v484 + v482 + 1);
              *((_WORD *)v486 + 2) = __dword_1411336C4 + v63;
            }
            __dword_1411336E4 = *((unsigned __int16 *)v3 + 1);
            v64 = __dword_1411336E4 + ((v515 >> 1) + v22 * v516) / v515 + 2;
            __dword_1411336E8 = v64;
            if(v22 < 24) {
              __dword_1411336F0 = 0;
              v65 = 0;
            LABEL_56:
              __dword_1411336CC = 171;
              v571 = v65;
              v67 = 0;
              __dword_1411336C8 = 0;
              v68 = 171;
              __dword_1411336F4 = 1032 * (*((_DWORD *)v3 + 2) == 0) + 32 * (v46 & 3) + 2 * (v44 < 3);
              v573 = __dword_1411336F4;
              v567 = v19 + 1;
              v69 = __qword_14025F220;
              __dword_14113368C += (((unsigned int)(v21 - v22 - 4) >> 24) & 0xFFFFFF80) + 28672 * ((unsigned int)(40 - v22) >> 31) + 32 * (*v3 > v45) + 28672;
              v70 = v64;
              v572 = __dword_1411336A0;
              goto LABEL_57;
            }
          }
          v65 = v64 >> 1;
          __dword_1411336F0 = v64 >> 1;
          goto LABEL_56;
        }
      }
    }
    v222 = __qword_141133588;
    v223 = *(unsigned __int8 *)(__qword_141133580 + 2);
    __dword_141133674 = v223;
    v224 = (unsigned __int8 *)__qword_141133580;
    v225 = __qword_14025F220;
    __dword_14014AD7C = v223;
    __dword_14010A158[(unsigned __int8)__byte_1400DA158[v223 + (__dword_14014ADD0 << 8)]] = v2;
    do {
      v226 = v224 + 2;
      v227 = *((unsigned int *)v224 + 2);
      *v225++ = (__int64)v226;
      v224 = (unsigned __int8 *)(v222 + v227);
      LODWORD(v228) = *v224;
    } while(!*v224);
    v229 = *((unsigned int *)v224 + 1);
    v553 = v225 - __qword_14025F220;
    v552 = v553;
    v230 = (unsigned __int8 *)(v222 + v229 + 6LL * *v224);
    if(!v230[1]) {
      v231 = v225 - __qword_14025F220;
      do {
        if(*v230 != v223)
          break;
        v224 = (unsigned __int8 *)(v222 + *((unsigned int *)v224 + 2));
        v229 = *((unsigned int *)v224 + 1);
        v228 = *v224;
        ++v231;
        v230 = (unsigned __int8 *)(v222 + v229 + 6 * v228);
      } while(!v230[1]);
      v552 = v231;
    }
    v232 = v224[1];
    v551 = v228 + 1;
    if(!*(_BYTE *)(v229 + v222 + 6LL * (v232 & 0xF) + 1)) {
      v495 = v225;
      __sub_14000C0E0(v224);
      v225 = v495;
      v229 = *((unsigned int *)v224 + 1);
      v232 = v224[1];
    }
    v233 = *(unsigned __int8 *)(v229 + v222 + 6LL * (v232 & 0xF));
    v234 = (unsigned __int8 *)(v222 + v229);
    v550 = v233;
    __dword_141133678 = v233;
    v549 = v234;
    if(v223 != *v234) {
      v235 = v234;
      do
        v235 += 6;
      while(v223 != *v235);
      v549 = v235;
    }
    v236 = v549[1];
    v237 = *((unsigned __int16 *)v224 + 1);
    if(2 * v236 + 23 > v234[7] + v234[1]) {
      v238 = *((_DWORD *)v224 + 2);
      if(v238) {
        v476 = (unsigned __int8 *)(v222 + v238);
        v479 = *((unsigned __int16 *)v476 + 1);
        v477 = *v476;
        if(v237 + v237 * v477 < v551 * (v479 + 15)) {
          v239 = *((unsigned int *)v476 + 1);
          v478 = v477 + 1;
          if(!*(_BYTE *)(v222 + v239 + 6LL * (v476[1] & 0xF) + 1)) {
            v496 = v225;
            __sub_14000C0E0(v476);
            v225 = v496;
            v236 = v549[1];
            v237 = *((unsigned __int16 *)v224 + 1);
            v239 = *((unsigned int *)v476 + 1);
            v479 = *((unsigned __int16 *)v476 + 1);
          }
          v240 = 5 * v228 + 5;
          for(j = (unsigned __int8 *)(v222 + v239); v223 != *j; j += 6)
            ;
          v438 = j[1];
          v242 = v237 + 2 * v240;
          v243 = (2 * (2 * v240 < v237) + 5) * v478 + v479;
          if(v243 * v236 < (int)(v242 * v438)) {
            if(v236 > 228)
              goto LABEL_162;
            v244 = v237 - v236;
            v245 = ((v242 - v236) * (v438 + 3 * v236) + 3 * (v242 - v236) + v243 - v438 - 4) / (3 * (v242 - v236) + v243 - v438);
            if(v245 > v236 + 11)
              LOWORD(v245) = v236 + 11;
            LOWORD(v237) = v245 + v244;
            *((_WORD *)v224 + 1) = v237;
            v549[1] = v245;
            v236 = (unsigned __int8)v245;
            v237 = (unsigned __int16)v237;
          }
        }
      }
    }
    if(v236 <= 9) {
      v246 = (unsigned __int8)__algn_141136BC4[v236 + 1083];
      goto LABEL_164;
    }
  LABEL_162:
    v246 = v236 - 6;
  LABEL_164:
    v247 = v237 - v246;
    v248 = (6 * v247 < v246) + (15 * v247 < 4 * v246) + (9 * v247 < 4 * v246) + (13 * v247 < 8 * v246) + (v247 < v246) + (v247 < 2 * v246) + (v247 < 6 * v246) + (35 * v247 < 4 * v246) + 1;
    do {
      v249 = *--v225;
      if(v248 > *(unsigned __int8 *)(*v225 + 1))
        *(_BYTE *)(v249 + 1) = v248;
      else
        v248 = *(unsigned __int8 *)(*v225 + 1);
    } while(v225 != __qword_14025F220);
    v554 = v249;
    __dword_141133684 = v248 - ((6 * v247 < v246) + (15 * v247 < 4 * v246) + (9 * v247 < 4 * v246) + (13 * v247 < 8 * v246) + (v247 < v246) + (v247 < 2 * v246) + (v247 < 6 * v246) + (35 * v247 < 4 * v246) + 1);
    __dword_14025F20C = 1 << v223;
    __dword_14025F200 = (unsigned int)(v223 + __dword_14025F1FC) >> 5;
    __dword_14025F208 = (unsigned int)(v223 + __dword_14025F204) >> 5;
    v556 = (((1 << v223) & __dword_14020F1FC[(unsigned int)(v223 + __dword_14025F1FC) >> 5]) != 0) + 2 * (((1 << v223) & __dword_14021F1FC[(unsigned int)(v223 + __dword_14025F204) >> 5]) != 0);
    v250 = __dword_1401CF1FC[v223 + (__dword_14014ADB0 << 8)];
    if((unsigned int)(__dword_141133688 - v250) >= 0x20000) {
      __dword_14014ADF0 = 0x20000;
      __dword_14014ADEC = 0x20000;
      __dword_14014ADE8 = 0x20000;
    } else {
      __dword_14014ADEC = __dword_1401CF1FC[v223 + (__dword_14014ADB0 << 8)];
      __dword_14014ADE8 = (unsigned __int8)__byte_14002A158[(v250 + 2) & 0x1FFFF];
      __dword_14014ADEC = __dword_141133688 - v250;
      __dword_14014ADF0 = __dword_141133688 - __dword_1401CF1FC[__dword_14014ADE8 + (v223 << 8)];
    }
    v251 = *((unsigned int *)v3 + 2);
    v252 = *(unsigned __int8 *)(v251 + v222);
    v253 = 32 * (__dword_141133660 > 0) + (v3[1] & 0x80);
    __dword_14113368C += (v252 == 0) << 7;
    v555 = __dword_141133690 - v552;
    v254 = __dword_141133694 + ((4 * (__dword_141133690 - v552 > 3) + 8 * (v223 == v550) + (v223 == __dword_14014AD70) + 2 * (v2 == __dword_14010A158[v223])) << 9) + v253 + ((v556 == 3 || v2 == __dword_14010A958[v223]) << 13);
    v255 = (((__dword_141133698 & 6) == 6) << 8) + ((__dword_141133698 & 1) << 6);
    v256 = v254 + v255;
    if(v252) {
      v257 = v256 & 0xFFFFFFDE;
      v258 = v248 != (6 * v247 < v246) + (15 * v247 < 4 * v246) + (9 * v247 < 4 * v246) + (13 * v247 < 8 * v246) + (v247 < v246) + (v247 < 2 * v246) + (v247 < 6 * v246) + (35 * v247 < 4 * v246) + 1 || v247 <= 10 * v246;
      v259 = 32 * v258 + v257 + (v223 == __dword_14113369C);
    } else {
      v260 = *(unsigned int *)(v251 + v222 + 8);
      v252 = *(unsigned __int8 *)(v260 + v222);
      if(*(_BYTE *)(v260 + v222) || v555 < 4 || v555 < 5 && v551 < 4 && *((unsigned __int16 *)v224 + 1) - v549[1] < 36) {
        v261 = *(_DWORD *)(v260 + v222 + 8);
        if(v261)
          v262 = 2 * v252 < *(unsigned __int8 *)(v261 + v222);
        else
          v262 = 0;
        v259 = v256 + 8 * v262 + 16;
      } else if(v553 >= 6) {
        if(v553 >= 0xE) {
          v263 = v254 + v255 + 12;
          v259 = v254 + v255 + 14;
          if(v553 < 0x20)
            v259 = v263;
        } else {
          v259 = v254 + v255 + 10;
        }
      } else {
        v259 = v254 + v255 + 8;
      }
    }
    v264 = v259 + (unsigned int)*((unsigned __int8 *)__xmmword_140029100 + v252);
    v265 = &__dword_14025FA20[0x8000 * (unsigned __int64)(unsigned __int8)__byte_140029240[v248]];
    __qword_141133590 = (__int64)&v265[2 * (v264 ^ 0x1000)];
    __qword_141133598 = (__int64)&v265[2 * (v264 ^ 1)];
    __qword_1411335A0 = (__int64)&v265[2 * (v264 ^ 0x2000)];
    __qword_1411335A8 = (__int64)&v265[2 * (v264 ^ 0x20)];
    __qword_1411335B0 = (__int64)&v265[2 * (v264 ^ 0x200)];
    v266 = (__int64)&v265[2 * (v264 ^ 0x400)];
    v267 = &v265[2 * v264];
    v268 = *((unsigned __int16 *)v267 + 2);
    __qword_1411335B8 = v266;
    __qword_1411335C0 = (__int64)v267;
    __qword_1411335C8 = (__int64)(v267 - 4);
    __qword_1411335D0 = (__int64)(v267 + 4);
    if(v268 > 0x8000) {
      __sub_140010850((__int64)v267);
      v268 = *((unsigned __int16 *)v267 + 2);
    }
    v269 = *(unsigned __int16 *)v267;
    v270 = *((_WORD *)v267 + 3);
    __dword_141133680 = v269;
    *((_WORD *)v267 + 2) = v268 + v270;
    __dword_14113367C = v268;
    __dword_14014AD98 = v269;
    __dword_14014AD9C = v268;
    v271 = (v223 == __dword_14014AD80) + (__dword_1411336A0 & 0xE200);
    v272 = v223 == __dword_14014AD88;
    v273 = (v223 & 0xE0) + 16 * (v223 == __dword_14014ADD0) + 8 * (v223 == __dword_14014ADCC) + 4 * (v223 == __dword_14014AD8C);
    v274 = (((_WORD)v223 - (_WORD)__dword_14014ADB0) & 0x800) + (((_WORD)__dword_14014ADB4 - (_WORD)v223) & 0x400) + ((v223 == __dword_14014ADDC) << 8);
    v275 = v556;
    v276 = v274 + v273 + v271 + 2 * v272;
    if(v556)
      v275 = 1;
    __dword_1411336A0 = (v275 << 12) + v276 + (((v268 < (unsigned int)(56 * v269)) + (v268 < (unsigned int)(6 * v269))) << 16);
    v277 = &__dword_14045FB20[2 * __dword_1411336A0];
    v278 = *v277;
    __qword_1411335D8 = (__int64)v277;
    v279 = v269 * (__int64)v278 / v277[1];
    v280 = __dword_141133680;
    v281 = 1 - __dword_141133680;
    if(v279 >= 1 - __dword_141133680) {
      v281 = 0x40000;
      if(v279 < 0x40000)
        v281 = v279;
    }
    v282 = *v277;
    v283 = v277[1];
    if((int)abs32(*v277) > 0x40000) {
      v282 >>= 1;
      *v277 = v282;
      v283 >>= v283 > 4096;
    }
    v284 = v282 - v280;
    *v277 = v284;
    v285 = v280 + v283 + 2;
    if(v285 > 0x40000000) {
      *v277 = v284 >> 1;
      v285 >>= 1;
    }
    v277[1] = v285;
    v286 = (unsigned __int16 *)__qword_1411335C0;
    v287 = *(unsigned __int16 *)(__qword_1411335C0 + 6);
    v288 = v281 + __dword_14113367C;
    v289 = v281 + v280;
    if(v287 <= 0x20) {
      v290 = __dword_141133674;
      __dword_14113367C = v288;
      __dword_141133680 = v289;
      __qword_1411335B8 = (__int64)__dword_1405DFC20;
      __qword_1411335B0 = (__int64)__dword_1405DFC20;
      __qword_1411335A8 = (__int64)__dword_1405DFC20;
      __qword_1411335A0 = (__int64)__dword_1405DFC20;
      __qword_141133598 = (__int64)__dword_1405DFC20;
      __qword_141133590 = (__int64)__dword_1405DFC20;
      __qword_1411335D0 = (__int64)__dword_1405DFC20;
      __qword_1411335C8 = (__int64)__dword_1405DFC20;
      __qword_1411335E0 = (__int64)__dword_1405DFC20;
      __qword_1411335E8 = (__int64)__dword_1405DFC20;
      __qword_1411335F0 = (__int64)&__dword_1411336A4;
    } else {
      v290 = __dword_141133674;
      v291 = &__dword_14079FD20[128 * (unsigned __int64)(unsigned __int8)__byte_1400293C0[v248] + 64 * (__dword_141133660 > -9) + 4 * (unsigned int)(__dword_14010A558[__dword_141133674] == __dword_141133664) + (((unsigned int)v264 >> 6) & 0xBLL) + (unsigned int)(16 * v556)];
      v292 = *v291;
      __qword_1411335F0 = (__int64)v291;
      v293 = (v292 >> (v287 < 0x230)) + v289;
      __dword_141133680 = v293;
      v294 = (0x3100u >> (v287 < 0x230)) + v288;
      __dword_14113367C = v294;
      *v291 = v292 - ((v292 + 2) >> 3);
      v295 = __qword_1411335C8;
      v105 = v287 < 0x220;
      v296 = *(_WORD *)(__qword_1411335C8 + 4);
      v297 = v105;
      if(v296 > 0x8000u) {
        v435 = __qword_1411335C8;
        v432 = v286;
        __sub_140010850(__qword_1411335C8);
        v286 = v432;
        v295 = v435;
        v296 = *(_WORD *)(v435 + 4);
      }
      __dword_1411336A8 = *(unsigned __int16 *)(v295 + 6) >> v297;
      *(_WORD *)(v295 + 4) = __dword_1411336A8 + v296;
      v298 = __qword_1411335D0;
      v299 = *(_WORD *)(__qword_1411335D0 + 4);
      if(v299 > 0x8000u) {
        v434 = __qword_1411335D0;
        v431 = v286;
        __sub_140010850(__qword_1411335D0);
        v286 = v431;
        v298 = v434;
        v299 = *(_WORD *)(v434 + 4);
      }
      __dword_1411336AC = *(unsigned __int16 *)(v298 + 6) >> v297;
      v300 = (__int64)(v286 - 0x10000);
      __qword_1411335E0 = (__int64)(v286 - 0x10000);
      *(_WORD *)(v298 + 4) = __dword_1411336AC + v299;
      v301 = *(v286 - 0x10000);
      v302 = v286[0x10000];
      v303 = *(v286 - 65534);
      __qword_1411335E8 = (__int64)(v286 + 0x10000);
      v304 = v302 + v301;
      v305 = v286[65538];
      v289 = (v304 >> (v297 + 1)) + v293;
      __dword_141133680 = v289;
      v288 = ((unsigned int)(v305 + v303) >> (v297 + 1)) + v294;
      __dword_14113367C = v288;
      if(v305 > 0x8000) {
        v433 = v286 - 0x10000;
        v430 = v286;
        __sub_140010850((__int64)(v286 + 0x10000));
        v286 = v430;
        v300 = (__int64)v433;
        LOWORD(v305) = v430[65538];
      }
      __dword_1411336B0 = v286[65539] >> v297;
      v286[65538] = __dword_1411336B0 + v305;
      v306 = *(v286 - 65534);
      if(v306 > 0x8000u) {
        v429 = v286;
        __sub_140010850(v300);
        v286 = v429;
        v306 = *(v429 - 65534);
      }
      __dword_1411336B4 = *(v286 - 65533) >> v297;
      *(v286 - 65534) = __dword_1411336B4 + v306;
      v307 = __qword_141133590;
      v308 = *(_WORD *)(__qword_141133590 + 4);
      v309 = (v286[3] < 0x398u) + v297;
      if(v308 > 0x8000u) {
        __sub_140010850(__qword_141133590);
        v308 = *(_WORD *)(v307 + 4);
      }
      __dword_1411336B8 = *(unsigned __int16 *)(v307 + 6) >> v309;
      *(_WORD *)(v307 + 4) = __dword_1411336B8 + v308;
      v310 = __qword_141133598;
      v311 = *(_WORD *)(__qword_141133598 + 4);
      if(v311 > 0x8000u) {
        __sub_140010850(__qword_141133598);
        v311 = *(_WORD *)(v310 + 4);
      }
      __dword_1411336BC = *(unsigned __int16 *)(v310 + 6) >> v309;
      *(_WORD *)(v310 + 4) = __dword_1411336BC + v311;
      v312 = __qword_1411335A0;
      v313 = *(_WORD *)(__qword_1411335A0 + 4);
      if(v313 > 0x8000u) {
        __sub_140010850(__qword_1411335A0);
        v313 = *(_WORD *)(v312 + 4);
      }
      __dword_1411336C0 = *(unsigned __int16 *)(v312 + 6) >> v309;
      *(_WORD *)(v312 + 4) = __dword_1411336C0 + v313;
      v314 = __qword_1411335A8;
      v315 = *(_WORD *)(__qword_1411335A8 + 4);
      if(v315 > 0x8000u) {
        __sub_140010850(__qword_1411335A8);
        v315 = *(_WORD *)(v314 + 4);
      }
      __dword_1411336C4 = *(unsigned __int16 *)(v314 + 6) >> v309;
      *(_WORD *)(v314 + 4) = __dword_1411336C4 + v315;
      v316 = __qword_1411335B0;
      v317 = *(_WORD *)(__qword_1411335B0 + 4);
      if(v317 > 0x8000u) {
        __sub_140010850(__qword_1411335B0);
        v317 = *(_WORD *)(v316 + 4);
      }
      __dword_1411336C8 = *(unsigned __int16 *)(v316 + 6) >> v309;
      *(_WORD *)(v316 + 4) = __dword_1411336C8 + v317;
      v318 = __qword_1411335B8;
      v319 = *(_WORD *)(__qword_1411335B8 + 4);
      if(v319 > 0x8000u) {
        __sub_140010850(__qword_1411335B8);
        v319 = *(_WORD *)(v318 + 4);
      }
      __dword_1411336CC = *(unsigned __int16 *)(v318 + 6) >> v309;
      *(_WORD *)(v318 + 4) = __dword_1411336CC + v319;
    }
    __dword_141133680 = 60416LL * v289 / v288;
    __dword_14113367C = 60416;
    __dword_14014ADA0 = __dword_141133680;
    __dword_14014ADA4 = 60416;
    v320 = &__dword_1407A0720[0x100000 * (__dword_14014AD68 < (unsigned int)__dword_14014ADB8) + 0x80000 * (v290 == __dword_14014AD70) + 0x20000 * __dword_141133694 + 512 * __dword_14014ADB4 + 2 * v290 + 2 * (unsigned __int64)((unsigned __int8)((unsigned int)__dword_14014ADEC < 0x78) << 17)];
    v321 = *v320;
    __qword_1411335F8 = (__int64)v320;
    v322 = __dword_141133680 * (__int64)v321 / v320[1];
    v323 = __dword_141133680;
    v324 = 1 - __dword_141133680;
    if(v322 >= 1 - __dword_141133680) {
      v324 = 0x40000;
      if(v322 < 0x40000)
        v324 = (int)(60416LL * v289 / v288) * (__int64)v321 / v320[1];
    }
    v325 = *v320;
    if((int)abs32(*v320) > 0x80000) {
      v326 = v320[1];
      v325 >>= 1;
      *v320 = v325;
      v320[1] = v326 >> (v326 > 0x2000);
    }
    v327 = v320[1];
    v328 = v325 - v323;
    *v320 = v328;
    v329 = v323 + v327 + 1120;
    v320[1] = v329;
    if(v329 > 0x40000000) {
      *v320 = v328 >> 1;
      v320[1] = v329 >> 1;
    }
    __dword_14113367C += v324;
    __dword_14014ADA8 = __dword_14113367C;
    v330 = v324 + v323;
    __dword_141133680 = v330;
    v331 = ((17 * v330 < (unsigned int)__dword_14113367C) << 13) + (((unsigned __int16)__dword_14014ADE8 - (unsigned __int16)v290) & 0x1000) + 16 * (v290 == __dword_14113369C) + 8 * (v290 == __dword_14014AD70) + 4 * (v290 == __dword_14014AD94) + (v290 == __dword_14014AD84) + (((unsigned int)__dword_14014ADF0 < 0x29) << 7) + __dword_1411336D0 + (__dword_1411336A0 & 0x800) + 2 * (*((unsigned __int8 *)__xmmword_140028F00 + v290) + (v290 == __dword_14014AD90)) + 32 * (((unsigned int)__dword_14014ADEC < 0xA800) + ((unsigned int)__dword_14014ADEC < 0x600) + ((unsigned int)__dword_14014ADEC < 0xD));
    v332 = &__dword_140FA0820[2 * v331];
    v333 = *v332;
    __qword_141133600 = (__int64)v332;
    v334 = (__dword_14113367C - v330) * (__int64)v333 / __dword_140FA0824[2 * v331];
    v335 = v330;
    v336 = v330 - __dword_14113367C + 1;
    if(v334 >= v336) {
      v336 = 0x40000;
      if(v334 < 0x40000)
        v336 = (__dword_14113367C - v330) * (__int64)v333 / __dword_140FA0824[2 * v331];
    }
    v337 = *v332;
    v338 = v336 + __dword_14113367C;
    __dword_14113367C += v336;
    v339 = __dword_140FA0824[2 * v331];
    if((int)abs32(v337) > 0x100000) {
      v337 >>= 1;
      *v332 = v337;
      v339 >>= v339 > 0x2000;
    }
    v340 = v337 + 2 * v330;
    v341 = __dword_14014AD7C;
    *v332 = v340;
    __dword_140FA0824[2 * v331] = v339 + 1;
    v342 = __qword_141133588 + *(_DWORD *)(v554 + 2) - __qword_141133560;
    v343 = __dword_14010A558[v290] == __dword_141133664;
    v344 = *(unsigned __int8 *)(__qword_14002A150 + (((_WORD)v290 - v341) & 0x1FF));
    v345 = (*(_DWORD *)(__qword_14002A150 + 512) < (unsigned int)(384 * v344)) + (*(_DWORD *)(__qword_14002A150 + 512) < (unsigned int)(58 * v344)) + (*(_DWORD *)(__qword_14002A150 + 512) < (unsigned int)(22 * v344));
    v346 = (unsigned __int8)__byte_1400EA158[__dword_14014ADF8];
    __dword_14014ADAC = v338;
    v347 = &__dword_141060920[2 * (int)(4 * (v342 >= 0xFFFFFD20) + v343 + (v331 & 0x700) + __dword_14113368C + (((unsigned int)__dword_14014ADEC < 0x2C00) << 11) + 2 * (v290 == v346) + 8 * v345)];
    v348 = *v347;
    __qword_141133608 = (__int64)v347;
    v349 = (v338 - v335) * (__int64)v348 / v347[1];
    v350 = __dword_141133680;
    v351 = __dword_141133680 - __dword_14113367C + 1;
    if(v349 >= v351) {
      v351 = 0x80000;
      if(v349 < 0x80000)
        v351 = (v338 - v335) * (__int64)v348 / v347[1];
    }
    v352 = *v347;
    v353 = v351 + __dword_14113367C;
    __dword_14113367C += v351;
    v354 = v347[1];
    if((int)abs32(v352) > 0x100000) {
      v352 >>= 1;
      *v347 = v352;
      v354 >>= v354 > 0x2000;
    }
    *v347 = v352 + v335;
    v347[1] = v354 + 2;
    v356 = rc.GetSubRange(v350, v353);
    if(!rc.IsDecodeMatched(v356)) {
      v423 = (unsigned int)__dword_14025F200;
      v424 = __dword_14025F20C;
      v425 = v3[2];
      v426 = v353 * v3[3];
      rc.DecodeNotMatched(v356);
      __dword_140029540[v425] = __dword_141133664;
      __dword_14020F1FC[v423] &= ~v424;
      __qword_141133610 = 0;
      __dword_14021F1FC[__dword_14025F208] &= ~__dword_14025F20C;
      v195 = __qword_141133588;
      v427 = v3[3];
      v3 = (unsigned __int8 *)__qword_141133580;
      v203 = *(unsigned __int8 *)__qword_141133580;
      __dword_141133694 = 0;
      __dword_1411336DC = v203;
      LODWORD(v202) = (int)(16 * (v427 * v350 + v350 - v427 * v353)) / (int)(v353 + v426);
      __dword_1411336D8 = v202;
      do {
      LABEL_126:
        v204 = *((_DWORD *)v3 + 2);
        if(!v204)
          return v202;
        v205 = __dword_141133690;
        v206 = --__dword_141133690;
        v3 = (unsigned __int8 *)(v195 + v204);
        v207 = *v3;
        __qword_141133580 = (__int64)v3;
        if(v207) {
          v202 = v3[1] & 0xF;
          if(!*(_BYTE *)(v195 + *((unsigned int *)v3 + 1) + 6 * v202 + 1))
            LODWORD(v202) = __sub_14000C0E0(v3);
        }
        v208 = v207 - v203;
      } while(!v208);
      v209 = *((unsigned int *)v3 + 1);
      v536 = v206;
      v537 = v205;
      v567 = v208;
      v535 = *v3;
      v67 = 0x8000;
      v534 = v535 + 1;
      v210 = v535 - v203;
      v538 = v535 + 1;
      v211 = v210;
      v212 = 0;
      v213 = __qword_14025F220;
      v541 = 0;
      v214 = __qword_14025F220;
      v215 = *(unsigned __int8 *)(v195 + v209 + 6LL * (v3[1] & 0xF));
      v543 = (unsigned __int8 *)(v195 + v209 + 6LL * (v3[1] & 0xF));
      v2 = __dword_141133664;
      v569 = *v543;
      __dword_141133674 = v569;
      v542 = v215;
      if(__dword_141133664 == __dword_140029540[v215]) {
        __dword_1411336C8 = 0x8000;
      } else {
        v216 = v543[1];
        __qword_14025F220[0] = (__int64)v543;
        v212 = 1;
        v213 = &__qword_14025F228;
        __dword_140029540[v542] = __dword_141133664;
        v214 = &__qword_14025F228;
        v541 = v216;
        v67 = 0;
        v211 = v210 - 1;
        __dword_1411336C8 = 0;
      }
      v217 = (unsigned __int8 *)(v195 + v209 - 6);
      if(v211) {
        v218 = v213;
        v520 = v210;
        v219 = v541;
        v521 = v195;
        v522 = v172;
        v524 = v171;
        v526 = v67;
        v558 = v3;
        while(1) {
          do {
            v217 += 6;
            v220 = *v217;
            v221 = *v217;
          } while(v2 == __dword_140029540[v221]);
          v378 = v217[1];
          __dword_140029540[v221] = v2;
          *v213++ = (__int64)v217;
          v214 = v213;
          v219 += v378;
          if(v220 == __dword_14014AD70)
            break;
          if(v2 == __dword_14010A958[v221] || v2 == __dword_14010A558[v221]) {
            v380 = v218;
            v379 = 4 * (v2 == __dword_14010A958[v221]) + 12;
            goto LABEL_274;
          }
        LABEL_278:
          if(!--v211) {
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
        v380 = &__qword_14025F220[(unsigned __int8)v212 & ((__byte_14014AD5C & 7) == 0)];
      LABEL_274:
        for(k = v213 - 1; k > v380; --k) {
          v382 = *(k - 1);
          if(v379 * *(unsigned __int8 *)(*k + 1) <= *(unsigned __int8 *)(v382 + 1))
            break;
          *(k - 1) = *k;
          *k = v382;
        }
        ++v218;
        goto LABEL_278;
      }
    LABEL_280:
      __qword_1411335A8 = (__int64)__dword_1405DFC20;
      __qword_1411335A0 = (__int64)__dword_1405DFC20;
      __qword_141133598 = (__int64)__dword_1405DFC20;
      __qword_141133590 = (__int64)__dword_1405DFC20;
      __qword_141133618 = (__int64)__dword_1405DFC20;
      if(v535 == 255) {
        v570 = -1;
        __dword_141133678 = -1;
        __qword_1411335B0 = (__int64)__dword_1405DFC20;
        v419 = 1;
        v540 = __dword_141133698;
        v539 = __dword_1411336E0;
      } else {
        v383 = *((unsigned int *)v3 + 2);
        v497 = (unsigned __int8 *)(v195 + v383);
        v504 = *(unsigned int *)(v195 + v383 + 4);
        v570 = *(unsigned __int8 *)(v195 + v504 + 6LL * (*(_BYTE *)(v195 + v383 + 1) & 0xF));
        if(v570 == v569) {
          v384 = *(unsigned int *)(v195 + v383 + 4);
          v385 = (unsigned __int8 *)(v195 + v383);
          v386 = *(unsigned __int8 *)(v195 + v384 + 6LL * (*(_BYTE *)(v195 + v383 + 1) & 0xF));
          do {
            if(*v385 != *v3)
              break;
            v385 = (unsigned __int8 *)(v195 + *((unsigned int *)v385 + 2));
            v384 = *((unsigned int *)v385 + 1);
            v386 = *(unsigned __int8 *)(v195 + v384 + 6LL * (v385[1] & 0xF));
          } while(v386 == v569);
          v504 = v384;
          v497 = v385;
          v570 = v386;
          __dword_141133678 = v386;
        } else {
          __dword_141133678 = *(unsigned __int8 *)(v195 + *(unsigned int *)(v195 + v383 + 4) + 6LL * (*(_BYTE *)(v195 + v383 + 1) & 0xF));
        }
        v387 = v3[1];
        if(16 * v210 <= v541 || (v3[1] & 0x40) != 0) {
          __qword_141133620 = (__int64)v214;
          v503 = *((unsigned __int16 *)v3 + 1);
        } else {
          v388 = *v497 + 1;
          v389 = (unsigned __int8 *)(v195 + v504);
          v390 = v388;
          do {
            __byte_141108A20[*v389] = v389[1];
            v389 += 6;
            --v390;
          } while(v390);
          v391 = *((unsigned __int16 *)v3 + 1);
          v392 = 5 * v535 + 5;
          v523 = v172;
          v525 = v171;
          v527 = v67;
          v393 = v391;
          v519 = v2;
          v394 = 0;
          v395 = (5 * ((unsigned int)(2 * v392 - v391) >> 31) + 8) * v388 + *((unsigned __int16 *)v497 + 1);
          do {
            v396 = *--v213;
            v398 = *(unsigned __int8 *)(*v213 + 1);
            v399 = v393 + 2 * v392;
            LOBYTE(v400) = *(_BYTE *)(*v213 + 1);
            if((int)(v398 * v395) < v399 * (unsigned __int8)__byte_141108A20[*(unsigned __int8 *)*v213] && *(unsigned __int8 *)(*v213 + 1) <= 0xE4u) {
              v401 = v391 - v398;
              v397 = (unsigned __int8)__byte_141108A20[*(unsigned __int8 *)*v213];
              v400 = ((v399 - v398) * (3 * v398 + v397) + 3 * (v399 - v398) + v395 - v397 - 4) / (3 * (v399 - v398) + v395 - v397);
              if(v400 > v398 + 11)
                v400 = v398 + 11;
              v391 = v400 + v401;
              *((_WORD *)v3 + 1) = v391;
              *(_BYTE *)(v396 + 1) = v400;
              v393 = (unsigned __int16)v391;
            }
            v394 += (unsigned __int8)v400;
          } while(v213 != __qword_14025F220);
          v172 = v523;
          v171 = v525;
          v387 = v3[1];
          v383 = *((unsigned int *)v3 + 2);
          v503 = v393;
          v67 = v527;
          v541 = v394;
          v2 = v519;
          __qword_141133620 = (__int64)v213;
        }
        v402 = *(unsigned __int8 *)(v383 + v195);
        v538 = v402 + 1;
        v501 = v2 != __dword_140029540[__dword_14113369C];
        v498 = v2 != __dword_140029540[v570];
        v540 = __dword_141133698;
        v500 = 2 * v402 + 2;
        v403 = (int)((((16 * (unsigned int)*v3 - v503) >> 26) & 0xFFFFFFE0) + (__dword_141133698 & 3) + 8 * (v501 | (v2 != __dword_140029540[*(unsigned __int8 *)(v195 + *(unsigned int *)(v383 + v195 + 4) + 6LL * (*(_BYTE *)(v383 + v195 + 1) & 0xF))])) + 4 * (v534 * v541 > v503 * v210) + (v387 & 0xC0) + 16 * (v210 >= 2 * v402 - 2 * v535));
        v502 = (unsigned __int8)__byte_1400292BE[v534];
        v404 = (v502 << 11) + 8 * v403;
        v499 = v403;
        v405 = (char *)__dword_141128C20 + v404;
        v406 = *(int *)((char *)__dword_141128C20 + v404);
        v407 = *(unsigned __int16 *)((char *)&__word_141128C24 + v404);
        __qword_1411335B0 = (__int64)__dword_141128C20 + v404;
        __dword_141133680 = v406;
        v505 = v407;
        __dword_14113367C = v407;
        if(v407 > 0x8000 || v406 > 0x80000) {
          v443 = v403;
          v533 = v67;
          __sub_140010730((__int64)__dword_141128C20 + v404);
          v67 = v533;
          LODWORD(v403) = v443;
          LOWORD(v407) = *((_WORD *)v405 + 2);
        }
        v408 = *((unsigned __int16 *)v405 + 3);
        *((_WORD *)v405 + 2) = v408 + v407;
        v539 = __dword_1411336E0;
        if(v408 > 0x78) {
          v473 = &__dword_141108B20[2048 * v502 + 16 * (unsigned int)(*v3 + v500 > __dword_1411336E0) + 2 * ((unsigned __int8)v403 & 0xF3) + 2 * (unsigned int)(v498 << 9) + 2 * (unsigned int)(v501 << 8) + 2 * (unsigned __int64)(4 * (unsigned int)(v536 < 10))];
          __qword_1411335A8 = (__int64)v473;
          v468 = v408 < 0x100;
          v471 = (v408 < 0x100) + 1;
          v409 = *((_DWORD *)v405 + 512);
          __qword_141133618 = (__int64)(v405 + 2048);
          v410 = (v409 + *((_DWORD *)v405 - 512)) >> v471;
          __qword_141133590 = (__int64)(v405 - 2048);
          v472 = v409;
          v411 = *((unsigned __int16 *)v405 - 1022);
          v406 += v410;
          v412 = *((unsigned __int16 *)v405 + 1026);
          __dword_141133680 = v406;
          v505 += (unsigned int)(v412 + v411) >> v471;
          __dword_14113367C = v505;
          if(v412 > 0x8000 || v472 > 0x80000) {
            v442 = v403;
            v532 = v67;
            __sub_140010730((__int64)(v405 + 2048));
            v67 = v532;
            LODWORD(v403) = v442;
            v412 = *((unsigned __int16 *)v405 + 1026);
          }
          v413 = *((unsigned __int16 *)v405 - 1022);
          v414 = (*((unsigned __int16 *)v405 + 1027) >> v471) + v412;
          __dword_1411336EC = *((unsigned __int16 *)v405 + 1027) >> v471;
          *((_WORD *)v405 + 1026) = v414;
          if(v413 > 0x8000 || *((_DWORD *)v405 - 512) > 0x80000u) {
            v441 = v403;
            v531 = v67;
            __sub_140010730((__int64)(v405 - 2048));
            v67 = v531;
            LODWORD(v403) = v441;
            LOWORD(v413) = *((_WORD *)v405 - 1022);
          }
          __dword_1411336B8 = *((unsigned __int16 *)v405 - 1021) >> v471;
          *((_WORD *)v405 - 1022) = __dword_1411336B8 + v413;
          v470 = *((unsigned __int16 *)v405 + 3) < 0x200u;
          v415 = *((unsigned __int16 *)v473 + 2);
          if(v415 > 0x8000 || (unsigned int)*v473 > 0x80000) {
            v440 = v403;
            v530 = v67;
            __sub_140010730((__int64)v473);
            v67 = v530;
            LODWORD(v403) = v440;
            LOWORD(v415) = *((_WORD *)v473 + 2);
          }
          __dword_1411336C4 = *((unsigned __int16 *)v473 + 3) >> v470;
          *((_WORD *)v473 + 2) = __dword_1411336C4 + v415;
          v469 = (*((unsigned __int16 *)v405 + 3) < 0x400u) + v468 + 1;
          v416 = &v405[-8 * v499];
          v417 = *(unsigned __int16 *)&v416[8 * (int)(v403 ^ 2) + 4];
          v474 = (__int64)&v416[8 * (int)(v403 ^ 2)];
          __qword_141133598 = v474;
          if(v417 > 0x8000 || *(_DWORD *)v474 > 0x80000u) {
            v439 = v403;
            v529 = v67;
            __sub_140010730(v474);
            v67 = v529;
            LODWORD(v403) = v439;
            LOWORD(v417) = *(_WORD *)(v474 + 4);
          }
          __dword_1411336BC = *(unsigned __int16 *)(v474 + 6) >> v469;
          *(_WORD *)(v474 + 4) = __dword_1411336BC + v417;
          v475 = (__int64)&v416[8 * (int)(v403 ^ 8)];
          __qword_1411335A0 = v475;
          v418 = *(_WORD *)(v475 + 4);
          if(v418 > 0x8000u || *(_DWORD *)v475 > 0x80000u) {
            v528 = v67;
            __sub_140010730(v475);
            v67 = v528;
            v418 = *(_WORD *)(v475 + 4);
          }
          __dword_1411336C0 = *(unsigned __int16 *)(v475 + 6) >> v469;
          *(_WORD *)(v475 + 4) = __dword_1411336C0 + v418;
        }
        v419 = ((v505 >> 1) + v534 * v406) / v505 + 2;
        if(v534 < 24) {
          v571 = 0;
          goto LABEL_317;
        }
      }
      v571 = (int)(v419 + *((unsigned __int16 *)v3 + 1)) >> 1;
    LABEL_317:
      __dword_1411336F0 = v571;
      v70 = v541 + v419;
      __dword_1411336E8 = v70;
      __dword_141133660 = __dword_141133708;
      __dword_1411336E4 = v541 + v535 + 1;
      v68 = 0;
      v69 = __qword_14025F220;
      __dword_1411336CC = 0;
      __qword_141133620 = (__int64)__qword_14025F220;
      v420 = (v536 > 1) << 15;
      v572 = v420 + (__dword_1411336A0 & 0x7FFF);
      __dword_1411336A0 = v420 + (__dword_1411336A0 & 0xFFFF7FFF);
      __dword_1411336D0 = ((v536 > 3) << 15) + v420 + (__dword_1411336D0 & 0x4600);
      v421 = v536 < 3 || v537 + 22 < __dword_14113370C;
      v573 = 1032 * (*((_DWORD *)v3 + 2) == 0) + 32 * (v540 & 3) + 2 * v421;
      __dword_1411336F4 = v573;
      __dword_14113368C = 32 * (*v3 + 28 > 2 * v539) + (((unsigned int)(v538 - v534 - 3) >> 24) & 0xFFFFFF80) + ((v536 > 2) << 13) + (((__dword_14113370C > 7) + (__dword_14113370C > 2) + 7 * (((unsigned int)(74 - v534) >> 31) + ((unsigned int)(2 - v534) >> 31)) + (v536 > 16)) << 12);
      while(1) {
        rc.DecodeNormalize(a2);
      LABEL_57:
        v71 = (unsigned __int8 *)*v69;
        v72 = *(unsigned __int8 *)(*v69 + 1);
        v574 = v3;
        __qword_141133620 = (__int64)(v69 + 1);
        __qword_1411335D0 = (__int64)__dword_1405DFC20;
        __qword_1411335C8 = (__int64)__dword_1405DFC20;
        __qword_1411335E0 = (__int64)__dword_1405DFC20;
        __qword_1411335E8 = (__int64)__dword_1405DFC20;
        v73 = *v71;
        v74 = ((v72 << 8) - v68) / v70;
        __dword_14025F20C = 1 << *v71;
        v577 = v74;
        v575 = v74;
        v578 = (unsigned int)(unsigned __int8)__byte_140029000[v74] + 1;
        __dword_14025F200 = ((v73 + __dword_14025F1FC) >> 5) + 0x2000;
        __dword_14025F208 = v67 + ((v73 + __dword_14025F204) >> 5);
        v75 = v73;
        v76 = v2 == __dword_14010A558[v73];
        v576 = ((__dword_14025F20C & __dword_14020F1FC[((v73 + __dword_14025F1FC) >> 5) + 0x2000]) != 0) + 2 * ((__dword_14025F20C & __dword_14021F1FC[v67 + ((v73 + __dword_14025F204) >> 5)]) != 0);
        __qword_141133628 = (__int64)&__qword_1411336F8;
        __qword_141133630 = (__int64)&__qword_1411336F8;
        v77 = v2 == __dword_14010A158[v73];
        v580 = v2 == __dword_14010A958[v73];
        v78 = v73 == __dword_14014AD6C || *(_DWORD *)(__qword_14002A150 + 512) < *(unsigned __int8 *)(__qword_14002A150 + (((_WORD)v73 - (_WORD)__dword_14014AD7C) & 0x1FF)) << 7;
        v79 = __dword_14014ADB0;
        v579 = __dword_1401CF1FC[256 * __dword_14014ADB0 + v73];
        if((unsigned int)(__dword_141133688 - v579) >= 0x20000) {
          v80 = 0x20000;
          __dword_14014ADF0 = 0x20000;
          __dword_14014ADEC = 0x20000;
          __dword_14014ADE8 = 0x20000;
        } else {
          __dword_14014ADEC = __dword_1401CF1FC[256 * __dword_14014ADB0 + v73];
          __dword_14014ADE8 = (unsigned __int8)__byte_14002A158[(v579 + 2) & 0x1FFFF];
          __dword_14014ADEC = __dword_141133688 - v579;
          v80 = __dword_141133688 - __dword_1401CF1FC[256 * v73 + __dword_14014ADE8];
          __dword_14014ADF0 = v80;
        }
        v81 = v573 | (((v80 < 0x220) << 12) + (v78 << 9) + ((v70 < v571) << 11) + 8 * (v73 == v570) + ((v73 == v569) << 10) + *((unsigned __int8 *)__xmmword_140028F00 + v73) + (v77 << 8) + 16 * (v73 == __dword_14014AD70) + 4 * v580 + (unsigned int)v76);
        v82 = &__dword_1405DFC20[0x4000 * v578 + 2 * v81];
        v83 = (unsigned __int16)__word_1405DFC24[0x8000 * v578 + 4 * v81];
        __qword_1411335C0 = (__int64)v82;
        if(v83 > 0x8000) {
          __sub_140010850((__int64)&__dword_1405DFC20[0x4000 * v578 + 2 * v81]);
          v79 = __dword_14014ADB0;
          v83 = *((unsigned __int16 *)v82 + 2);
        }
        v84 = *(unsigned __int16 *)v82;
        v85 = *((_WORD *)v82 + 3);
        __dword_141133680 = v84;
        *((_WORD *)v82 + 2) = v83 + v85;
        __dword_14113367C = v83;
        __dword_14014AD98 = v84;
        __dword_14014AD9C = v83;
        v86 = v576;
        if(v576)
          v86 = 1;
        __dword_1411336A0 = (((_WORD)v73 - v79) & 0x800) + (v86 << 12) + (v572 & 0xE200) + (((_WORD)__dword_14014ADB4 - (_WORD)v73) & 0x400) + ((v73 == __dword_14014ADDC) << 8) + (v73 & 0xE0) + 16 * (v73 == __dword_14014ADD0) + 8 * (v73 == __dword_14014ADCC) + 4 * (v73 == __dword_14014AD8C) + (v73 == __dword_14014AD80) + 2 * (v73 == __dword_14014AD88) + (((v83 < (unsigned int)(56 * v84)) + (v83 < (unsigned int)(6 * v84))) << 16);
        v87 = &__dword_14045FB20[2 * __dword_1411336A0];
        v88 = *v87;
        __qword_1411335D8 = (__int64)v87;
        v89 = v84 * (__int64)v88 / v87[1];
        v90 = __dword_141133680;
        v91 = 1 - __dword_141133680;
        if(v89 >= 1 - __dword_141133680) {
          v91 = 0x40000;
          if(v89 < 0x40000)
            v91 = v89;
        }
        v92 = *v87;
        v93 = v87[1];
        if((int)abs32(*v87) > 0x40000) {
          v92 >>= 1;
          *v87 = v92;
          v93 >>= v93 > 4096;
        }
        v94 = v92 - v90;
        *v87 = v94;
        v95 = v90 + v93 + 2;
        if(v95 > 0x40000000) {
          *v87 = v94 >> 1;
          v95 >>= 1;
        }
        v87[1] = v95;
        v96 = v91 + __dword_14113367C;
        v97 = v91 + v90;
        v98 = *(unsigned __int16 *)(__qword_1411335C0 + 6);
        if(v98 <= 8) {
          __dword_14113367C = v96;
          __dword_141133680 = v97;
        } else {
          v559 = (unsigned __int16 *)__qword_1411335C0;
          v560 = *(unsigned __int16 *)(__qword_1411335C0 + 6);
          v557 = v3;
          v561 = v71;
          v562 = v73;
          v563 = v73;
          v99 = &__dword_141130D20[512 * (unsigned __int64)(unsigned __int8)__byte_140029440[v575] + 2 * (unsigned int)(v576 << 6)];
          v100 = &v99[2 * (v81 & 0x1F)];
          v101 = v100[1];
          __qword_141133630 = (__int64)v100;
          v102 = &v99[2 * (((unsigned int)v81 >> 4) & 0x1F) + 64];
          v103 = *v102;
          __qword_141133628 = (__int64)v102;
          v104 = v98 < 0x150;
          v105 = v98 < 0x48;
          v106 = v102[1];
          v107 = v105;
          v108 = v101 * (v577 + 2);
          v109 = v106 + v101;
          v110 = v108 >> 8;
          v111 = (unsigned int)(v106 * (v577 + 2)) >> 8;
          v112 = (v109 >> 11) & 0xFFFFFFFE;
          v105 = v109 < 0xC000;
          v113 = *v100;
          v114 = v110 + v111;
          if(!v105)
            v112 = 24;
          v115 = (unsigned __int8)__byte_140022930[v112];
          v116 = (unsigned __int8)__byte_140022931[v112];
          if(v114 >= 0x6000)
            v114 = 24576;
          v117 = ((v115 * v114 + v116 * (v103 + *v100)) >> (v104 + 7)) + v97;
          v118 = v104 + v107 + 3;
          v119 = ((unsigned int)(192 * (v115 + v116)) >> v104) + v96;
          __dword_141133700 = 0x3000u >> v118;
          v120 = v104 + 4;
          __dword_141133704 = 0x3000u >> v120;
          *v100 = v113 - (v113 >> v118);
          v3 = v557;
          v100[1] -= (v110 + 7) >> v120;
          v71 = v561;
          *v102 = v103 - (v103 >> v118);
          v102[1] -= (v111 + 7) >> v120;
          v121 = (__int64)(v559 - 0x8000);
          v122 = *(v559 - 0x8000);
          v75 = v563;
          v123 = *(v559 - 32766);
          v124 = v559 + 0x8000;
          v125 = v559[0x8000];
          __qword_1411335E8 = (__int64)(v559 + 0x8000);
          v126 = v122 + v125;
          v127 = v559[32770];
          __qword_1411335E0 = (__int64)(v559 - 0x8000);
          v97 = (v126 >> 3) + v117;
          __dword_141133680 = v97;
          v96 = ((unsigned int)(v127 + v123) >> 3) + v119;
          v73 = v562;
          __dword_14113367C = v96;
          v128 = (v560 < 0x30) + 1;
          if(v127 > 0x8000) {
            v447 = v97;
            v451 = v96;
            __sub_140010850((__int64)(v559 + 0x8000));
            v96 = v451;
            v97 = v447;
            v121 = (__int64)(v559 - 0x8000);
            v128 = (v560 < 0x30) + 1;
            v124 = v559 + 0x8000;
            LOWORD(v127) = v559[32770];
          }
          __dword_1411336B0 = v124[3] >> v128;
          v124[2] = __dword_1411336B0 + v127;
          v129 = *(_WORD *)(v121 + 4);
          if(v129 > 0x8000u) {
            v452 = v128;
            v453 = v121;
            v446 = v97;
            v450 = v96;
            __sub_140010850(v121);
            v96 = v450;
            v97 = v446;
            v121 = v453;
            v128 = v452;
            v129 = *(_WORD *)(v453 + 4);
          }
          __dword_1411336B4 = *(unsigned __int16 *)(v121 + 6) >> v128;
          *(_WORD *)(v121 + 4) = __dword_1411336B4 + v129;
          v130 = v559[3];
          if(v130 > 0x48) {
            __qword_1411335C8 = (__int64)(v559 + 0x10000);
            v131 = (v130 < 0x1E0) + (v130 < 0x3D0) + 1;
            v132 = v559[65538];
            if(v132 > 0x8000u) {
              v437 = v131;
              v445 = v97;
              v449 = v96;
              __sub_140010850((__int64)(v559 + 0x10000));
              v96 = v449;
              v97 = v445;
              v131 = v437;
              v132 = v559[65538];
            }
            __dword_1411336A8 = v559[65539] >> v131;
            __qword_1411335D0 = (__int64)(v559 - 0x10000);
            v559[65538] = __dword_1411336A8 + v132;
            v133 = *(v559 - 65534);
            if(v133 > 0x8000u) {
              v436 = v131;
              v444 = v97;
              v448 = v96;
              __sub_140010850((__int64)(v559 - 0x10000));
              v96 = v448;
              v97 = v444;
              v131 = v436;
              v133 = *(v559 - 65534);
            }
            __dword_1411336AC = *(v559 - 65533) >> v131;
            *(v559 - 65534) = __dword_1411336AC + v133;
          }
        }
        v134 = 60416LL * v97 / v96;
        __dword_141133680 = v134;
        __dword_14113367C = 60416;
        __dword_14014ADA0 = v134;
        __dword_14014ADA4 = 60416;
        v135 = &__dword_1407A0720[0x100000 * (__dword_14014AD68 < (unsigned int)__dword_14014ADB8) + 0x80000 * (v73 == __dword_14014AD70) + 0x20000 * __dword_141133694 + 512 * __dword_14014ADB4 + 2 * v75 + 2 * (unsigned __int64)((unsigned __int8)((unsigned int)__dword_14014ADEC < 0x78) << 17)];
        v136 = *v135;
        __qword_1411335F8 = (__int64)v135;
        v137 = v134 * (__int64)v136 / v135[1];
        v138 = __dword_141133680;
        v139 = 1 - __dword_141133680;
        if(v137 >= 1 - __dword_141133680) {
          v139 = 0x40000;
          if(v137 < 0x40000)
            v139 = v137;
        }
        v140 = *v135;
        if((int)abs32(*v135) > 0x80000) {
          v141 = v135[1];
          v140 >>= 1;
          *v135 = v140;
          v135[1] = v141 >> (v141 > 0x2000);
        }
        v142 = v135[1];
        v143 = v140 - v138;
        *v135 = v143;
        v144 = v138 + v142 + 1120;
        v135[1] = v144;
        if(v144 > 0x40000000) {
          *v135 = v143 >> 1;
          v135[1] = v144 >> 1;
        }
        v145 = *((unsigned __int8 *)__xmmword_140028F00 + v75);
        __dword_14113367C += v139;
        v146 = v139 + v138;
        __dword_141133680 = v146;
        __dword_14014ADA8 = __dword_14113367C;
        v147 = ((17 * v146 < (unsigned int)__dword_14113367C) << 13) + (((unsigned __int16)__dword_14014ADE8 - (unsigned __int16)v73) & 0x1000) + 16 * (v73 == __dword_14113369C) + 8 * (v73 == __dword_14014AD70) + 4 * (v73 == __dword_14014AD94) + (v73 == __dword_14014AD84) + (((unsigned int)__dword_14014ADF0 < 0x29) << 7) + __dword_1411336D0 + (__dword_1411336A0 & 0x800) + 2 * (v145 + (v73 == __dword_14014AD90)) + 32 * (((unsigned int)__dword_14014ADEC < 0xA800) + ((unsigned int)__dword_14014ADEC < 0x600) + ((unsigned int)__dword_14014ADEC < 0xD));
        v148 = &__dword_140FA0820[2 * v147];
        v149 = *v148;
        __qword_141133600 = (__int64)v148;
        v150 = (__dword_14113367C - v146) * (__int64)v149 / __dword_140FA0824[2 * v147];
        v151 = v146;
        v152 = v146 - __dword_14113367C + 1;
        if(v150 >= v152) {
          v152 = 0x40000;
          if(v150 < 0x40000)
            v152 = v150;
        }
        v153 = *v148;
        v154 = v152 + __dword_14113367C;
        __dword_14113367C += v152;
        v155 = __dword_140FA0824[2 * v147];
        if((int)abs32(v153) > 0x100000) {
          v153 >>= 1;
          *v148 = v153;
          v155 >>= v155 > 0x2000;
        }
        v76 = __dword_14010A558[v75] == __dword_141133664;
        v156 = __qword_14002A150;
        *v148 = v153 + 2 * v146;
        __dword_140FA0824[2 * v147] = v155 + 1;
        v157 = *(unsigned __int8 *)(v156 + (((_WORD)v73 - (_WORD)__dword_14014AD7C) & 0x1FF));
        LODWORD(v156) = *(_DWORD *)(v156 + 512);
        v158 = __qword_141133588 + *(_DWORD *)(v71 + 2) - __qword_141133560 + 736;
        v159 = (unsigned __int8)__byte_1400EA158[__dword_14014ADF8];
        __dword_14014ADAC = v154;
        v160 = 8 * (((unsigned int)v156 < 384 * v157) + ((unsigned int)v156 < 58 * v157) + ((unsigned int)v156 < 22 * v157));
        v161 = (v147 & 0x700) + __dword_14113368C + (((unsigned int)__dword_14014ADEC < 0x2C00) << 11) + 2 * (v73 == v159);
        v162 = 4 * (v158 < 0x2E0) + v76;
        v163 = &__dword_141060920[2 * v162 + 2 * v161 + 2 * v160];
        v164 = *v163;
        __qword_141133608 = (__int64)v163;
        v165 = (v154 - v151) * (__int64)v164 / __dword_141060924[2 * v162 + 2 * v161 + 2 * v160];
        v166 = __dword_141133680;
        v167 = __dword_141133680 - __dword_14113367C + 1;
        if(v165 >= v167) {
          v167 = 0x80000;
          if(v165 < 0x80000)
            v167 = v165;
        }
        v168 = *v163;
        v169 = v167 + __dword_14113367C;
        __dword_14113367C += v167;
        v170 = v163[1];
        if((int)abs32(v168) > 0x100000) {
          v168 >>= 1;
          *v163 = v168;
          v170 >>= v170 > 0x2000;
        }
        *v163 = v168 + v151;
        v163[1] = v170 + 2;
        v173 = rc.GetSubRange(v166, v169);
        if(!rc.IsDecodeMatched(v173)) {
          v422 = __qword_141133610;
          __dword_1411336E8 -= v71[1];
          rc.DecodeNotMatched(v173);
          __dword_14020F1FC[__dword_14025F200] &= ~__dword_14025F20C;
          __dword_14021F1FC[__dword_14025F208] &= ~__dword_14025F20C;
          if(v422) {
            v3 = (unsigned __int8 *)__qword_141133580;
            __qword_141133610 = 0;
            __dword_141133694 = 0;
          } else if((unsigned __int64)v71 <= __qword_141133588 + (unsigned __int64)*((unsigned int *)v3 + 1)) {
            v3 = (unsigned __int8 *)__qword_141133580;
          } else {
            v3 = (unsigned __int8 *)__qword_141133580;
            __dword_1411336C8 = 0x8000;
          }
        } else {
          v174 = __qword_141133630;
          v175 = __dword_141133700;
          v176 = __dword_141133704;
          v177 = (_DWORD *)__qword_141133628;
          v178 = __dword_14014ADEC;
          *(_DWORD *)__qword_141133630 += __dword_141133700;
          *(_DWORD *)(v174 + 4) += v176;
          *v177 += v175;
          v179 = v3[1];
          v177[1] += v176;
          v180 = (v179 & 0x40) == 0 || v178 > 0xE00;
          v181 = (v178 > 0x4800) + (v178 > 0x380) + (v178 > 0x80) + v180 + 4;
          v3[1] = v179 & 0xF0;
          *((_WORD *)v3 + 1) += v181;
          v71[1] += v181;
          v182 = __qword_141133588 + *((unsigned int *)v3 + 1);
          if(v71 == (unsigned __int8 *)v182) {
            v3 = (unsigned __int8 *)__qword_141133580;
            if(__qword_141133580 == __qword_141133638)
              __dword_141133694 = v169 < 2 * v166;
          } else {
            v568 = *(_WORD *)v71;
            v183 = *(_DWORD *)(v71 + 2);
            do {
              v184 = *((_DWORD *)v71 - 1);
              *(_WORD *)v71 = *((_WORD *)v71 - 3);
              *(_DWORD *)(v71 + 2) = v184;
              v71 -= 6;
            } while((unsigned __int64)v71 > v182);
            v3 = (unsigned __int8 *)__qword_141133580;
            *(_WORD *)v71 = v568;
            *(_DWORD *)(v71 + 2) = v183;
          }
          v185 = v71[1];
          __qword_141133610 = (__int64)v71;
          if(v185 > 244) {
            __sub_14000B750(v574);
            v71 = (unsigned __int8 *)__qword_141133610;
          }
          if(v71)
            goto LABEL_248;
        }
        if(!--v567) {
          v186 = __dword_1411336E4;
          v187 = (_DWORD *)__qword_1411335B0;
          v188 = __qword_141133618;
          v189 = __dword_1411336EC;
          v190 = *v3;
          v191 = *(unsigned __int16 *)(__qword_1411335B0 + 6);
          *(_WORD *)(__qword_1411335B0 + 4) -= v191;
          v192 = v186 / (v190 + 1);
          v193 = __dword_1411336B8;
          *v187 += v192 * v191;
          v194 = __qword_141133590;
          *(_DWORD *)v188 += v189 * v192;
          *(_WORD *)(v188 + 4) -= v189;
          v195 = __qword_141133588;
          *(_DWORD *)v194 += v193 * v192;
          *(_WORD *)(v194 + 4) -= v193;
          v196 = (_DWORD *)__qword_141133598;
          v197 = __dword_1411336BC * v192;
          *(_WORD *)(__qword_141133598 + 4) -= __dword_1411336BC;
          v198 = __dword_1411336C0;
          *v196 += v197;
          v199 = (_DWORD *)__qword_1411335A0;
          v200 = v198 * v192;
          *(_WORD *)(__qword_1411335A0 + 4) -= v198;
          v201 = __qword_1411335A8;
          *v199 += v200;
          LODWORD(v202) = __dword_1411336C4 * v192;
          *(_WORD *)(v201 + 4) -= __dword_1411336C4;
          *(_DWORD *)v201 += v202;
          v203 = *v3;
          __dword_1411336DC = v203;
          goto LABEL_126;
        }
        v67 = __dword_1411336C8;
        v68 = __dword_1411336CC;
        v569 = __dword_141133674;
        v2 = __dword_141133664;
        v570 = __dword_141133678;
        v69 = (__int64 *)__qword_141133620;
        v572 = __dword_1411336A0;
        v70 = __dword_1411336E8;
        v571 = __dword_1411336F0;
        v573 = __dword_1411336F4;
      }
    }
    v71 = v3 + 2;
    v357 = v3[3];
    __qword_141133610 = (__int64)(v3 + 2);
    v358 = v357 == 1 && v353 < 4 * v350;
    v359 = (_DWORD *)__qword_141133590;
    v360 = (_DWORD *)__qword_141133598;
    v361 = (_DWORD *)__qword_1411335A0;
    v362 = __dword_1411336C0;
    v363 = (_DWORD *)__qword_1411335A8;
    v364 = __dword_1411336C4;
    __dword_141133694 = 1;
    v365 = (v357 - 127 < 0) + v357 + v358;
    v366 = __qword_1411335F0;
    v3[3] = v365;
    v367 = __dword_1411336B8;
    v368 = __dword_1411336BC;
    *(_DWORD *)v366 += 1568;
    LODWORD(v366) = __dword_1411336C8;
    *v359 += v367;
    v369 = (_DWORD *)__qword_1411335B0;
    *v360 += v368;
    v3 = (unsigned __int8 *)__qword_141133580;
    *v361 += v362;
    *v363 += v364;
    *v369 += v366;
    *(_DWORD *)__qword_1411335B8 += __dword_1411336CC;
  LABEL_248:
    putc(v71[0], a1);
    LODWORD(v202) = __sub_140007B80(v3);
    rc.DecodeNormalize(a2);
    v2 = __dword_141133664;
  } while(__dword_141133664);
  return v202;
}
PROBE_DECL(__sub_140003530)
__int64 __sub_140003530(
    FILE *File,
    FILE *a2,
    __int64 (*a3)(FILE *, FILE *, __int64),
    int a4) {
  PROBE_HIT(__sub_140003530);
  int v8;
  __int64 v10;
  int v11;
  int v12;
  __int64 v14;
  int v15;
  int v16;
  if(!__PPMblock_size)
    return 0;
  if(__dword_14113365C) {
    __dword_14113365C = 0;
    __dword_141133664 = 1;
  } else {
    rc.InitEncode();
    __sub_140010A40(a4);
  }
  while(1) {
    __sub_1400037A0(File, a2);
    if(__dword_141133664)
      break;
    if(a3)
      v8 = a3(a2, File, 0);
    else
      v8 = -1;
    __dword_141133664 = v8;
    memset((void*)(blob1+ 0x140029540 -0x1400227B0), 0, 0x400);
    memset(__dword_14010A158, 0, 3072);
    if(!v8) {
      __dword_14113365C = 1;
      return 0;
    }
  }
  rc.Flush(File);
  if(a3)
    a3(a2, File, 1);
  return 1;
}
PROBE_DECL(__sub_14000C310)
__int64 __sub_14000C310(FILE *a1, FILE *a2, __int64(*a3_printstats)(FILE *, FILE *, __int64), int a4) {
  PROBE_HIT(__sub_14000C310);
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
  if(!__PPMblock_size)
    return 0;
  if(__dword_14113365C) {
    __dword_14113365C = 0;
    __dword_141133664 = 1;
  } else {
    rc.InitDecode(a2);
    __sub_140010A40(a4);
  }
  while(1) {
    __sub_14000C560(a1, a2);
    if(__dword_141133664)
      break;
    if(a3_printstats)
      v9 = a3_printstats(a1, a2, 0);
    else
      v9 = -1;
    __dword_141133664 = v9;
    v10 = 64;
    do {
      __xmmword_140029530[v10] = 0;
      __xmmword_140029520[v10] = 0;
      __xmmword_140029510[v10] = 0;
      __xmmword_140029500[v10] = 0;
      v10 -= 4;
    } while(v10 * 16);
    memset(__dword_14010A158, 0, 3072);
    if(!v9) {
      __dword_14113365C = 1;
      return 0;
    }
  }
  if(a3_printstats)
    a3_printstats(a1, a2, 1);
  return 1;
}

PROBE_DECL(__Alloc_PPMblock)
__int64 __Alloc_PPMblock(unsigned int memsize_mb, int a2_order, int a3) {
  PROBE_HIT(__Alloc_PPMblock);
  int v4;
  size_t memsize_b;
  v4 = a2_order;
  if(!memsize_mb)
    return 0;
  if(memsize_mb > 0xFFF)
    return 0;
  if(a2_order < 2)
    return 0;
  if(a2_order > 16)
    return 0;
  if(__PPMblock_size)
    return 0;
  memsize_b = memsize_mb << 20;
  __PPMblock = new char[memsize_b]; //malloc(memsize_b);
//  __PPMblock = VAlloc<char>(memsize_b);
//printf( "!p=%I64X size=%I64X!\n", memsize_b, qword(memsize_b) );
  if(!__PPMblock)
    return 0;
  __dword_141133660 = -100;
  __PPMblock_size = memsize_b;
  __dword_141133648 = 0;
  __dword_141133658 = a3;
  __dword_14113365C = 0;
  if(v4 > 12)
    v4 = 16 << ((v4 + 19) & 31);
  __dword_141133654 = v4;
  return 1;
}

#include "stats.inc"
#include "main1.inc"

