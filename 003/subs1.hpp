// ---------------------------------------------------------------------------
// BMF's globals.
//
// Every global the decompiled bodies touch is a reference bound to a fixed
// offset inside blob.inc, BMF.exe's data segment (see bmf.cpp's BMF_BLOB).
// Hex-Rays named each one after the function it was recovered in, so the same
// object arrived under one name per user -- 904 declarations for 336 objects --
// each sitting just above its function.  They are collected here instead,
// sorted by address, one declaration per object, under the name IDA gave the
// address with the function prefix dropped.  302 are left: the other 34 were
// only ever touched by bodies the `c`/`d` command line cannot reach.
//
// The typedefs carry the array shapes, which cannot be written inline in the
// reference declaration.  Where one body reads an address as a bare scalar and
// another indexes it, the array shape is the one declared and the scalar's
// users say [0].
//
// A few functions declare a local with the same name as the global they use
// and reach the global through `::`; those locals still carry their original
// `__sub_XXXXXX_` names, which is now what tells the two apart.
//
// incdec.md §6.1 is why the addresses are still the names: giving these
// objects real names is a much larger job than moving them, because the same
// address is an int to one function and a char[] to the next.
// ---------------------------------------------------------------------------
typedef int32_t t_dword_438920[24];
static t_dword_438920& __dword_438920 = *(t_dword_438920*)(blob1 + 0x00438920 - BMF_BLOB_BASE);
typedef int32_t t_dword_438980[32];
static t_dword_438980& __dword_438980 = *(t_dword_438980*)(blob1 + 0x00438980 - BMF_BLOB_BASE);
typedef uint8_t t_byte_438A00[252];
static t_byte_438A00& __byte_438A00 = *(t_byte_438A00*)(blob1 + 0x00438A00 - BMF_BLOB_BASE);
typedef int32_t t_dword_438AFC[0x10000];
static t_dword_438AFC& __dword_438AFC = *(t_dword_438AFC*)(blob1 + 0x00438AFC - BMF_BLOB_BASE);
typedef int32_t t_dword_438B00[0x10000];
static t_dword_438B00& __dword_438B00 = *(t_dword_438B00*)(blob1 + 0x00438B00 - BMF_BLOB_BASE);
typedef int32_t t_dword_438B04[23];
static t_dword_438B04& __dword_438B04 = *(t_dword_438B04*)(blob1 + 0x00438B04 - BMF_BLOB_BASE);
typedef int32_t t_dword_439300[24];
static t_dword_439300& __dword_439300 = *(t_dword_439300*)(blob1 + 0x00439300 - BMF_BLOB_BASE);
typedef int32_t t_dword_439360[32];
static t_dword_439360& __dword_439360 = *(t_dword_439360*)(blob1 + 0x00439360 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439408[0x10000];
static t_byte_439408& __byte_439408 = *(t_byte_439408*)(blob1 + 0x00439408 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439409[15];
static t_byte_439409& __byte_439409 = *(t_byte_439409*)(blob1 + 0x00439409 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439418[16];
static t_byte_439418& __byte_439418 = *(t_byte_439418*)(blob1 + 0x00439418 - BMF_BLOB_BASE);
typedef uint8_t t_byte_43945C[0x10000];
static t_byte_43945C& __byte_43945C = *(t_byte_43945C*)(blob1 + 0x0043945C - BMF_BLOB_BASE);
typedef uint8_t t_byte_439470[0x10000];
static t_byte_439470& __byte_439470 = *(t_byte_439470*)(blob1 + 0x00439470 - BMF_BLOB_BASE);
typedef int32_t t_dword_439480[6];
static t_dword_439480& __dword_439480 = *(t_dword_439480*)(blob1 + 0x00439480 - BMF_BLOB_BASE);
typedef int32_t t_dword_43949C[0x10000];
static t_dword_43949C& __dword_43949C = *(t_dword_43949C*)(blob1 + 0x0043949C - BMF_BLOB_BASE);
typedef int32_t t_dword_4394A0[4];
static t_dword_4394A0& __dword_4394A0 = *(t_dword_4394A0*)(blob1 + 0x004394A0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4394B0[12];
static t_dword_4394B0& __dword_4394B0 = *(t_dword_4394B0*)(blob1 + 0x004394B0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4394E0[0x10000];
static t_dword_4394E0& __dword_4394E0 = *(t_dword_4394E0*)(blob1 + 0x004394E0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4394E4[0x10000];
static t_dword_4394E4& __dword_4394E4 = *(t_dword_4394E4*)(blob1 + 0x004394E4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4394E8[0x10000];
static t_dword_4394E8& __dword_4394E8 = *(t_dword_4394E8*)(blob1 + 0x004394E8 - BMF_BLOB_BASE);
typedef int32_t t_dword_4394EC[13];
static t_dword_4394EC& __dword_4394EC = *(t_dword_4394EC*)(blob1 + 0x004394EC - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439520;
static t_xmmword_439520& __xmmword_439520 = *(t_xmmword_439520*)(blob1 + 0x00439520 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439530;
static t_xmmword_439530& __xmmword_439530 = *(t_xmmword_439530*)(blob1 + 0x00439530 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439540;
static t_xmmword_439540& __xmmword_439540 = *(t_xmmword_439540*)(blob1 + 0x00439540 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439550;
static t_xmmword_439550& __xmmword_439550 = *(t_xmmword_439550*)(blob1 + 0x00439550 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439560;
static t_xmmword_439560& __xmmword_439560 = *(t_xmmword_439560*)(blob1 + 0x00439560 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439570;
static t_xmmword_439570& __xmmword_439570 = *(t_xmmword_439570*)(blob1 + 0x00439570 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439580;
static t_xmmword_439580& __xmmword_439580 = *(t_xmmword_439580*)(blob1 + 0x00439580 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439590;
static t_xmmword_439590& __xmmword_439590 = *(t_xmmword_439590*)(blob1 + 0x00439590 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4395A0;
static t_xmmword_4395A0& __xmmword_4395A0 = *(t_xmmword_4395A0*)(blob1 + 0x004395A0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4395B0;
static t_xmmword_4395B0& __xmmword_4395B0 = *(t_xmmword_4395B0*)(blob1 + 0x004395B0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4395C0;
static t_xmmword_4395C0& __xmmword_4395C0 = *(t_xmmword_4395C0*)(blob1 + 0x004395C0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4395D0;
static t_xmmword_4395D0& __xmmword_4395D0 = *(t_xmmword_4395D0*)(blob1 + 0x004395D0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4395E0;
static t_xmmword_4395E0& __xmmword_4395E0 = *(t_xmmword_4395E0*)(blob1 + 0x004395E0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4395F0;
static t_xmmword_4395F0& __xmmword_4395F0 = *(t_xmmword_4395F0*)(blob1 + 0x004395F0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439600;
static t_xmmword_439600& __xmmword_439600 = *(t_xmmword_439600*)(blob1 + 0x00439600 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439610;
static t_xmmword_439610& __xmmword_439610 = *(t_xmmword_439610*)(blob1 + 0x00439610 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439620;
static t_xmmword_439620& __xmmword_439620 = *(t_xmmword_439620*)(blob1 + 0x00439620 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439630;
static t_xmmword_439630& __xmmword_439630 = *(t_xmmword_439630*)(blob1 + 0x00439630 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439640;
static t_xmmword_439640& __xmmword_439640 = *(t_xmmword_439640*)(blob1 + 0x00439640 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439650;
static t_xmmword_439650& __xmmword_439650 = *(t_xmmword_439650*)(blob1 + 0x00439650 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439660;
static t_xmmword_439660& __xmmword_439660 = *(t_xmmword_439660*)(blob1 + 0x00439660 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439670;
static t_xmmword_439670& __xmmword_439670 = *(t_xmmword_439670*)(blob1 + 0x00439670 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439680;
static t_xmmword_439680& __xmmword_439680 = *(t_xmmword_439680*)(blob1 + 0x00439680 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439690;
static t_xmmword_439690& __xmmword_439690 = *(t_xmmword_439690*)(blob1 + 0x00439690 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4396A0;
static t_xmmword_4396A0& __xmmword_4396A0 = *(t_xmmword_4396A0*)(blob1 + 0x004396A0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4396B0;
static t_xmmword_4396B0& __xmmword_4396B0 = *(t_xmmword_4396B0*)(blob1 + 0x004396B0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4396C0;
static t_xmmword_4396C0& __xmmword_4396C0 = *(t_xmmword_4396C0*)(blob1 + 0x004396C0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4396D0;
static t_xmmword_4396D0& __xmmword_4396D0 = *(t_xmmword_4396D0*)(blob1 + 0x004396D0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4396E0;
static t_xmmword_4396E0& __xmmword_4396E0 = *(t_xmmword_4396E0*)(blob1 + 0x004396E0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4396F0;
static t_xmmword_4396F0& __xmmword_4396F0 = *(t_xmmword_4396F0*)(blob1 + 0x004396F0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439700;
static t_xmmword_439700& __xmmword_439700 = *(t_xmmword_439700*)(blob1 + 0x00439700 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439710;
static t_xmmword_439710& __xmmword_439710 = *(t_xmmword_439710*)(blob1 + 0x00439710 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439720;
static t_xmmword_439720& __xmmword_439720 = *(t_xmmword_439720*)(blob1 + 0x00439720 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439730;
static t_xmmword_439730& __xmmword_439730 = *(t_xmmword_439730*)(blob1 + 0x00439730 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439740;
static t_xmmword_439740& __xmmword_439740 = *(t_xmmword_439740*)(blob1 + 0x00439740 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439750;
static t_xmmword_439750& __xmmword_439750 = *(t_xmmword_439750*)(blob1 + 0x00439750 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439760;
static t_xmmword_439760& __xmmword_439760 = *(t_xmmword_439760*)(blob1 + 0x00439760 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439770;
static t_xmmword_439770& __xmmword_439770 = *(t_xmmword_439770*)(blob1 + 0x00439770 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439780;
static t_xmmword_439780& __xmmword_439780 = *(t_xmmword_439780*)(blob1 + 0x00439780 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439790;
static t_xmmword_439790& __xmmword_439790 = *(t_xmmword_439790*)(blob1 + 0x00439790 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4397A0;
static t_xmmword_4397A0& __xmmword_4397A0 = *(t_xmmword_4397A0*)(blob1 + 0x004397A0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4397B0;
static t_xmmword_4397B0& __xmmword_4397B0 = *(t_xmmword_4397B0*)(blob1 + 0x004397B0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4397F0;
static t_xmmword_4397F0& __xmmword_4397F0 = *(t_xmmword_4397F0*)(blob1 + 0x004397F0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439800;
static t_xmmword_439800& __xmmword_439800 = *(t_xmmword_439800*)(blob1 + 0x00439800 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439860[32];
static t_byte_439860& __byte_439860 = *(t_byte_439860*)(blob1 + 0x00439860 - BMF_BLOB_BASE);
typedef int32_t t_dword_439880[4];
static t_dword_439880& __dword_439880 = *(t_dword_439880*)(blob1 + 0x00439880 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439890[16];
static t_byte_439890& __byte_439890 = *(t_byte_439890*)(blob1 + 0x00439890 - BMF_BLOB_BASE);
typedef uint8_t t_byte_4398A0[32];
static t_byte_4398A0& __byte_4398A0 = *(t_byte_4398A0*)(blob1 + 0x004398A0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398C0[0x10000];
static t_dword_4398C0& __dword_4398C0 = *(t_dword_4398C0*)(blob1 + 0x004398C0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398C4[0x10000];
static t_dword_4398C4& __dword_4398C4 = *(t_dword_4398C4*)(blob1 + 0x004398C4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398C8[0x10000];
static t_dword_4398C8& __dword_4398C8 = *(t_dword_4398C8*)(blob1 + 0x004398C8 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398CC[0x10000];
static t_dword_4398CC& __dword_4398CC = *(t_dword_4398CC*)(blob1 + 0x004398CC - BMF_BLOB_BASE);
typedef int32_t t_dword_4398D0[0x10000];
static t_dword_4398D0& __dword_4398D0 = *(t_dword_4398D0*)(blob1 + 0x004398D0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398D4[0x10000];
static t_dword_4398D4& __dword_4398D4 = *(t_dword_4398D4*)(blob1 + 0x004398D4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398D8[0x10000];
static t_dword_4398D8& __dword_4398D8 = *(t_dword_4398D8*)(blob1 + 0x004398D8 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398DC[0x10000];
static t_dword_4398DC& __dword_4398DC = *(t_dword_4398DC*)(blob1 + 0x004398DC - BMF_BLOB_BASE);
typedef int32_t t_dword_4398E0[0x10000];
static t_dword_4398E0& __dword_4398E0 = *(t_dword_4398E0*)(blob1 + 0x004398E0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398E4[0x10000];
static t_dword_4398E4& __dword_4398E4 = *(t_dword_4398E4*)(blob1 + 0x004398E4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398E8[0x10000];
static t_dword_4398E8& __dword_4398E8 = *(t_dword_4398E8*)(blob1 + 0x004398E8 - BMF_BLOB_BASE);
typedef int32_t t_dword_4398EC[0x10000];
static t_dword_4398EC& __dword_4398EC = *(t_dword_4398EC*)(blob1 + 0x004398EC - BMF_BLOB_BASE);
typedef int32_t t_dword_4398F0[0x10000];
static t_dword_4398F0& __dword_4398F0 = *(t_dword_4398F0*)(blob1 + 0x004398F0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A00;
static t_xmmword_439A00& __xmmword_439A00 = *(t_xmmword_439A00*)(blob1 + 0x00439A00 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A10;
static t_xmmword_439A10& __xmmword_439A10 = *(t_xmmword_439A10*)(blob1 + 0x00439A10 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A20;
static t_xmmword_439A20& __xmmword_439A20 = *(t_xmmword_439A20*)(blob1 + 0x00439A20 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A30;
static t_xmmword_439A30& __xmmword_439A30 = *(t_xmmword_439A30*)(blob1 + 0x00439A30 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A40;
static t_xmmword_439A40& __xmmword_439A40 = *(t_xmmword_439A40*)(blob1 + 0x00439A40 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A50;
static t_xmmword_439A50& __xmmword_439A50 = *(t_xmmword_439A50*)(blob1 + 0x00439A50 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A60;
static t_xmmword_439A60& __xmmword_439A60 = *(t_xmmword_439A60*)(blob1 + 0x00439A60 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A70;
static t_xmmword_439A70& __xmmword_439A70 = *(t_xmmword_439A70*)(blob1 + 0x00439A70 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A80;
static t_xmmword_439A80& __xmmword_439A80 = *(t_xmmword_439A80*)(blob1 + 0x00439A80 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439A90;
static t_xmmword_439A90& __xmmword_439A90 = *(t_xmmword_439A90*)(blob1 + 0x00439A90 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439AA0;
static t_xmmword_439AA0& __xmmword_439AA0 = *(t_xmmword_439AA0*)(blob1 + 0x00439AA0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439AB0;
static t_xmmword_439AB0& __xmmword_439AB0 = *(t_xmmword_439AB0*)(blob1 + 0x00439AB0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439AC0;
static t_xmmword_439AC0& __xmmword_439AC0 = *(t_xmmword_439AC0*)(blob1 + 0x00439AC0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439AD0;
static t_xmmword_439AD0& __xmmword_439AD0 = *(t_xmmword_439AD0*)(blob1 + 0x00439AD0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439AE0;
static t_xmmword_439AE0& __xmmword_439AE0 = *(t_xmmword_439AE0*)(blob1 + 0x00439AE0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439AF0;
static t_xmmword_439AF0& __xmmword_439AF0 = *(t_xmmword_439AF0*)(blob1 + 0x00439AF0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B00;
static t_xmmword_439B00& __xmmword_439B00 = *(t_xmmword_439B00*)(blob1 + 0x00439B00 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B10;
static t_xmmword_439B10& __xmmword_439B10 = *(t_xmmword_439B10*)(blob1 + 0x00439B10 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B20;
static t_xmmword_439B20& __xmmword_439B20 = *(t_xmmword_439B20*)(blob1 + 0x00439B20 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B30;
static t_xmmword_439B30& __xmmword_439B30 = *(t_xmmword_439B30*)(blob1 + 0x00439B30 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B40;
static t_xmmword_439B40& __xmmword_439B40 = *(t_xmmword_439B40*)(blob1 + 0x00439B40 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B50;
static t_xmmword_439B50& __xmmword_439B50 = *(t_xmmword_439B50*)(blob1 + 0x00439B50 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_439B60;
static t_xmmword_439B60& __xmmword_439B60 = *(t_xmmword_439B60*)(blob1 + 0x00439B60 - BMF_BLOB_BASE);
typedef int32_t t_dword_439B7C;
static t_dword_439B7C& __dword_439B7C = *(t_dword_439B7C*)(blob1 + 0x00439B7C - BMF_BLOB_BASE);
typedef uint8_t t_byte_439BC0[8];
static t_byte_439BC0& __byte_439BC0 = *(t_byte_439BC0*)(blob1 + 0x00439BC0 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439BC8[8];
static t_byte_439BC8& __byte_439BC8 = *(t_byte_439BC8*)(blob1 + 0x00439BC8 - BMF_BLOB_BASE);
typedef uint8_t t_byte_439BD0[8];
static t_byte_439BD0& __byte_439BD0 = *(t_byte_439BD0*)(blob1 + 0x00439BD0 - BMF_BLOB_BASE);
typedef int32_t t_dword_439BD8[8];
static t_dword_439BD8& __dword_439BD8 = *(t_dword_439BD8*)(blob1 + 0x00439BD8 - BMF_BLOB_BASE);
static __m128d &__bmf_half_half = *(__m128d *)(blob1 + 0x0043B480 - BMF_BLOB_BASE);
typedef const char * t_off_441068;
static t_off_441068& __off_441068 = *(t_off_441068*)(blob1 + 0x00441068 - BMF_BLOB_BASE);
typedef int32_t t_dword_44108C;
static t_dword_44108C& __dword_44108C = *(t_dword_44108C*)(blob1 + 0x0044108C - BMF_BLOB_BASE);
typedef int32_t t_dword_441090;
static t_dword_441090& __dword_441090 = *(t_dword_441090*)(blob1 + 0x00441090 - BMF_BLOB_BASE);
typedef int32_t t_n2_4;
static t_n2_4& __n2_4 = *(t_n2_4*)(blob1 + 0x00441094 - BMF_BLOB_BASE);
typedef int32_t t_dword_441098;
static t_dword_441098& __dword_441098 = *(t_dword_441098*)(blob1 + 0x00441098 - BMF_BLOB_BASE);
typedef int32_t t_n7_0;
static t_n7_0& __n7_0 = *(t_n7_0*)(blob1 + 0x0044109C - BMF_BLOB_BASE);
typedef int32_t t_n7_1;
static t_n7_1& __n7_1 = *(t_n7_1*)(blob1 + 0x004410A0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4410A4[6];
static t_dword_4410A4& __dword_4410A4 = *(t_dword_4410A4*)(blob1 + 0x004410A4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4410BC[0x10000];
static t_dword_4410BC& __dword_4410BC = *(t_dword_4410BC*)(blob1 + 0x004410BC - BMF_BLOB_BASE);
typedef void * t_off_4410C0[0x10000];
static t_off_4410C0& __off_4410C0 = *(t_off_4410C0*)(blob1 + 0x004410C0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441120;
static t_xmmword_441120& __xmmword_441120 = *(t_xmmword_441120*)(blob1 + 0x00441120 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441130;
static t_xmmword_441130& __xmmword_441130 = *(t_xmmword_441130*)(blob1 + 0x00441130 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441140;
static t_xmmword_441140& __xmmword_441140 = *(t_xmmword_441140*)(blob1 + 0x00441140 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441150;
static t_xmmword_441150& __xmmword_441150 = *(t_xmmword_441150*)(blob1 + 0x00441150 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441160;
static t_xmmword_441160& __xmmword_441160 = *(t_xmmword_441160*)(blob1 + 0x00441160 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441170;
static t_xmmword_441170& __xmmword_441170 = *(t_xmmword_441170*)(blob1 + 0x00441170 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441180;
static t_xmmword_441180& __xmmword_441180 = *(t_xmmword_441180*)(blob1 + 0x00441180 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_441190;
static t_xmmword_441190& __xmmword_441190 = *(t_xmmword_441190*)(blob1 + 0x00441190 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4411A0;
static t_xmmword_4411A0& __xmmword_4411A0 = *(t_xmmword_4411A0*)(blob1 + 0x004411A0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4411B0;
static t_xmmword_4411B0& __xmmword_4411B0 = *(t_xmmword_4411B0*)(blob1 + 0x004411B0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4411C0;
static t_xmmword_4411C0& __xmmword_4411C0 = *(t_xmmword_4411C0*)(blob1 + 0x004411C0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4411D0;
static t_xmmword_4411D0& __xmmword_4411D0 = *(t_xmmword_4411D0*)(blob1 + 0x004411D0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4411E0;
static t_xmmword_4411E0& __xmmword_4411E0 = *(t_xmmword_4411E0*)(blob1 + 0x004411E0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4411F0;
static t_xmmword_4411F0& __xmmword_4411F0 = *(t_xmmword_4411F0*)(blob1 + 0x004411F0 - BMF_BLOB_BASE);
typedef int32_t t_dwLowDateTime;
static t_dwLowDateTime& __dwLowDateTime = *(t_dwLowDateTime*)(blob1 + 0x00442BB0 - BMF_BLOB_BASE);
typedef int32_t t_dword_442BB8;
static t_dword_442BB8& __dword_442BB8 = *(t_dword_442BB8*)(blob1 + 0x00442BB8 - BMF_BLOB_BASE);
typedef int32_t t_dword_442BBC;
static t_dword_442BBC& __dword_442BBC = *(t_dword_442BBC*)(blob1 + 0x00442BBC - BMF_BLOB_BASE);
typedef int32_t t_m[0x10000];
static t_m& __m = *(t_m*)(blob1 + 0x00442BC0 - BMF_BLOB_BASE);
typedef int32_t t_dword_442BC4[0x10000];
static t_dword_442BC4& __dword_442BC4 = *(t_dword_442BC4*)(blob1 + 0x00442BC4 - BMF_BLOB_BASE);
typedef int32_t t_dword_442BC8[13];
static t_dword_442BC8& __dword_442BC8 = *(t_dword_442BC8*)(blob1 + 0x00442BC8 - BMF_BLOB_BASE);
typedef int32_t t_dword_442BFC[2];
static t_dword_442BFC& __dword_442BFC = *(t_dword_442BFC*)(blob1 + 0x00442BFC - BMF_BLOB_BASE);
typedef int32_t t_dword_442C04;
static t_dword_442C04& __dword_442C04 = *(t_dword_442C04*)(blob1 + 0x00442C04 - BMF_BLOB_BASE);
typedef int32_t t_dword_442C80[78];
static t_dword_442C80& __dword_442C80 = *(t_dword_442C80*)(blob1 + 0x00442C80 - BMF_BLOB_BASE);
typedef int32_t t_dword_442DB8[3];
static t_dword_442DB8& __dword_442DB8 = *(t_dword_442DB8*)(blob1 + 0x00442DB8 - BMF_BLOB_BASE);
typedef int32_t t_dword_442DC4[6];
static t_dword_442DC4& __dword_442DC4 = *(t_dword_442DC4*)(blob1 + 0x00442DC4 - BMF_BLOB_BASE);
typedef int32_t t_dword_442DDC[0x10000];
static t_dword_442DDC& __dword_442DDC = *(t_dword_442DDC*)(blob1 + 0x00442DDC - BMF_BLOB_BASE);
typedef int32_t t_dword_442DE0;
static t_dword_442DE0& __dword_442DE0 = *(t_dword_442DE0*)(blob1 + 0x00442DE0 - BMF_BLOB_BASE);
typedef int32_t t_dword_442DE4[0x10000];
static t_dword_442DE4& __dword_442DE4 = *(t_dword_442DE4*)(blob1 + 0x00442DE4 - BMF_BLOB_BASE);
typedef int32_t t_dword_442DE8[21];
static t_dword_442DE8& __dword_442DE8 = *(t_dword_442DE8*)(blob1 + 0x00442DE8 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E3C[0x10000];
static t_dword_442E3C& __dword_442E3C = *(t_dword_442E3C*)(blob1 + 0x00442E3C - BMF_BLOB_BASE);
typedef int32_t t_n633;
static t_n633& __n633 = *(t_n633*)(blob1 + 0x00442E40 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E44[0x10000];
static t_dword_442E44& __dword_442E44 = *(t_dword_442E44*)(blob1 + 0x00442E44 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E48[3];
static t_dword_442E48& __dword_442E48 = *(t_dword_442E48*)(blob1 + 0x00442E48 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E54[5];
static t_dword_442E54& __dword_442E54 = *(t_dword_442E54*)(blob1 + 0x00442E54 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E68;
static t_dword_442E68& __dword_442E68 = *(t_dword_442E68*)(blob1 + 0x00442E68 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E6C;
static t_dword_442E6C& __dword_442E6C = *(t_dword_442E6C*)(blob1 + 0x00442E6C - BMF_BLOB_BASE);
typedef int32_t t_Src;
static t_Src& __Src = *(t_Src*)(blob1 + 0x00442E70 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E74;
static t_dword_442E74& __dword_442E74 = *(t_dword_442E74*)(blob1 + 0x00442E74 - BMF_BLOB_BASE);
typedef int32_t t_dword_442E88;
static t_dword_442E88& __dword_442E88 = *(t_dword_442E88*)(blob1 + 0x00442E88 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_442EA0;
static t_xmmword_442EA0& __xmmword_442EA0 = *(t_xmmword_442EA0*)(blob1 + 0x00442EA0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_442EB0;
static t_xmmword_442EB0& __xmmword_442EB0 = *(t_xmmword_442EB0*)(blob1 + 0x00442EB0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_442EC0;
static t_xmmword_442EC0& __xmmword_442EC0 = *(t_xmmword_442EC0*)(blob1 + 0x00442EC0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_442ED0;
static t_xmmword_442ED0& __xmmword_442ED0 = *(t_xmmword_442ED0*)(blob1 + 0x00442ED0 - BMF_BLOB_BASE);
typedef int32_t t_dword_442EE0[0x10000];
static t_dword_442EE0& __dword_442EE0 = *(t_dword_442EE0*)(blob1 + 0x00442EE0 - BMF_BLOB_BASE);
typedef int32_t t_dword_442EE4[262];
static t_dword_442EE4& __dword_442EE4 = *(t_dword_442EE4*)(blob1 + 0x00442EE4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4432FC[0x10000];
static t_dword_4432FC& __dword_4432FC = *(t_dword_4432FC*)(blob1 + 0x004432FC - BMF_BLOB_BASE);
typedef int32_t t_dword_443300[0x10000];
static t_dword_443300& __dword_443300 = *(t_dword_443300*)(blob1 + 0x00443300 - BMF_BLOB_BASE);
typedef int32_t t_dword_443310[6];
static t_dword_443310& __dword_443310 = *(t_dword_443310*)(blob1 + 0x00443310 - BMF_BLOB_BASE);
typedef int32_t t_dword_443328;
static t_dword_443328& __dword_443328 = *(t_dword_443328*)(blob1 + 0x00443328 - BMF_BLOB_BASE);
typedef int32_t t_dword_44332C;
static t_dword_44332C& __dword_44332C = *(t_dword_44332C*)(blob1 + 0x0044332C - BMF_BLOB_BASE);
typedef int32_t t_dword_443330[6];
static t_dword_443330& __dword_443330 = *(t_dword_443330*)(blob1 + 0x00443330 - BMF_BLOB_BASE);
typedef int32_t t_dword_443348;
static t_dword_443348& __dword_443348 = *(t_dword_443348*)(blob1 + 0x00443348 - BMF_BLOB_BASE);
typedef int32_t t_n2;
static t_n2& __n2 = *(t_n2*)(blob1 + 0x00443360 - BMF_BLOB_BASE);
typedef int32_t t_dword_443364;
static t_dword_443364& __dword_443364 = *(t_dword_443364*)(blob1 + 0x00443364 - BMF_BLOB_BASE);
typedef int32_t t_n8;
static t_n8& __n8 = *(t_n8*)(blob1 + 0x00443368 - BMF_BLOB_BASE);
typedef int32_t t_n256;
static t_n256& __n256 = *(t_n256*)(blob1 + 0x0044336C - BMF_BLOB_BASE);
typedef int32_t t_ElementCount_0;
static t_ElementCount_0& __ElementCount_0 = *(t_ElementCount_0*)(blob1 + 0x00443370 - BMF_BLOB_BASE);
typedef int32_t t_Buffer_1;
static t_Buffer_1& __Buffer_1 = *(t_Buffer_1*)(blob1 + 0x00443374 - BMF_BLOB_BASE);
typedef int32_t t_Buffer_0;
static t_Buffer_0& __Buffer_0 = *(t_Buffer_0*)(blob1 + 0x00443378 - BMF_BLOB_BASE);
typedef int32_t t_Buffer;
static t_Buffer& __Buffer = *(t_Buffer*)(blob1 + 0x0044337C - BMF_BLOB_BASE);
typedef int32_t t_buf[0x10000];
static t_buf& __buf = *(t_buf*)(blob1 + 0x00443380 - BMF_BLOB_BASE);
typedef int32_t t_dword_443384;
static t_dword_443384& __dword_443384 = *(t_dword_443384*)(blob1 + 0x00443384 - BMF_BLOB_BASE);
typedef int32_t t_dword_443388;
static t_dword_443388& __dword_443388 = *(t_dword_443388*)(blob1 + 0x00443388 - BMF_BLOB_BASE);
typedef int32_t t_n256_2[0x10000];
static t_n256_2& __n256_2 = *(t_n256_2*)(blob1 + 0x0044338C - BMF_BLOB_BASE);
typedef int32_t t_n512[0x10000];
static t_n512& __n512 = *(t_n512*)(blob1 + 0x00443390 - BMF_BLOB_BASE);
typedef int32_t t_n4_5;
static t_n4_5& __n4_5 = *(t_n4_5*)(blob1 + 0x00443394 - BMF_BLOB_BASE);
typedef int32_t t_n256_0[0x10000];
static t_n256_0& __n256_0 = *(t_n256_0*)(blob1 + 0x00443398 - BMF_BLOB_BASE);
typedef uint8_t t_byte_44339C[0x10000];
static t_byte_44339C& __byte_44339C = *(t_byte_44339C*)(blob1 + 0x0044339C - BMF_BLOB_BASE);
typedef uint8_t t_byte_44339D[0x10000];
static t_byte_44339D& __byte_44339D = *(t_byte_44339D*)(blob1 + 0x0044339D - BMF_BLOB_BASE);
typedef uint8_t t_byte_44339E[0x10000];
static t_byte_44339E& __byte_44339E = *(t_byte_44339E*)(blob1 + 0x0044339E - BMF_BLOB_BASE);
typedef uint8_t t_byte_44339F[0x10000];
static t_byte_44339F& __byte_44339F = *(t_byte_44339F*)(blob1 + 0x0044339F - BMF_BLOB_BASE);
typedef int32_t t_dword_4433A0[0x10000];
static t_dword_4433A0& __dword_4433A0 = *(t_dword_4433A0*)(blob1 + 0x004433A0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4433A4[0x10000];
static t_dword_4433A4& __dword_4433A4 = *(t_dword_4433A4*)(blob1 + 0x004433A4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4433A8[0x10000];
static t_dword_4433A8& __dword_4433A8 = *(t_dword_4433A8*)(blob1 + 0x004433A8 - BMF_BLOB_BASE);
typedef uint8_t t_byte_4433AC[0x10000];
static t_byte_4433AC& __byte_4433AC = *(t_byte_4433AC*)(blob1 + 0x004433AC - BMF_BLOB_BASE);
typedef uint8_t t_byte_4433AD[16];
static t_byte_4433AD& __byte_4433AD = *(t_byte_4433AD*)(blob1 + 0x004433AD - BMF_BLOB_BASE);
typedef uint8_t t_byte_4433BD[15];
static t_byte_4433BD& __byte_4433BD = *(t_byte_4433BD*)(blob1 + 0x004433BD - BMF_BLOB_BASE);
typedef char t_n3_1;
static t_n3_1& __n3_1 = *(t_n3_1*)(blob1 + 0x004433CC - BMF_BLOB_BASE);
typedef char t_n3_0;
static t_n3_0& __n3_0 = *(t_n3_0*)(blob1 + 0x004433CD - BMF_BLOB_BASE);
typedef char t_byte_4433CF;
static t_byte_4433CF& __byte_4433CF = *(t_byte_4433CF*)(blob1 + 0x004433CF - BMF_BLOB_BASE);
typedef int32_t t_n191;
static t_n191& __n191 = *(t_n191*)(blob1 + 0x004433D0 - BMF_BLOB_BASE);
typedef int32_t t_n191_0;
static t_n191_0& __n191_0 = *(t_n191_0*)(blob1 + 0x004433D4 - BMF_BLOB_BASE);
typedef int32_t t_n191_1;
static t_n191_1& __n191_1 = *(t_n191_1*)(blob1 + 0x004433D8 - BMF_BLOB_BASE);
typedef int32_t t_n4_1;
static t_n4_1& __n4_1 = *(t_n4_1*)(blob1 + 0x004433DC - BMF_BLOB_BASE);
typedef int32_t t_n0xFFFFFF;
static t_n0xFFFFFF& __n0xFFFFFF = *(t_n0xFFFFFF*)(blob1 + 0x004433E0 - BMF_BLOB_BASE);
typedef int32_t t_n2_3;
static t_n2_3& __n2_3 = *(t_n2_3*)(blob1 + 0x004433E4 - BMF_BLOB_BASE);
typedef int32_t t_n3;
static t_n3& __n3 = *(t_n3*)(blob1 + 0x004433E8 - BMF_BLOB_BASE);
typedef int32_t t_n4_2;
static t_n4_2& __n4_2 = *(t_n4_2*)(blob1 + 0x004433EC - BMF_BLOB_BASE);
typedef int32_t t_n5;
static t_n5& __n5 = *(t_n5*)(blob1 + 0x004433F0 - BMF_BLOB_BASE);
typedef int32_t t_p_n2;
static t_p_n2& __p_n2 = *(t_p_n2*)(blob1 + 0x004433F4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4433F8;
static t_dword_4433F8& __dword_4433F8 = *(t_dword_4433F8*)(blob1 + 0x004433F8 - BMF_BLOB_BASE);
typedef int32_t t_buf_2;
static t_buf_2& __buf_2 = *(t_buf_2*)(blob1 + 0x004433FC - BMF_BLOB_BASE);
typedef int32_t t_Src_0;
static t_Src_0& __Src_0 = *(t_Src_0*)(blob1 + 0x00443400 - BMF_BLOB_BASE);
typedef int32_t t_Src_1;
static t_Src_1& __Src_1 = *(t_Src_1*)(blob1 + 0x00443404 - BMF_BLOB_BASE);
typedef int32_t t_dword_443408;
static t_dword_443408& __dword_443408 = *(t_dword_443408*)(blob1 + 0x00443408 - BMF_BLOB_BASE);
typedef int32_t t_dword_44340C[0x10000];
static t_dword_44340C& __dword_44340C = *(t_dword_44340C*)(blob1 + 0x0044340C - BMF_BLOB_BASE);
typedef int32_t t_n24;
static t_n24& __n24 = *(t_n24*)(blob1 + 0x00443410 - BMF_BLOB_BASE);
typedef int32_t t_n14;
static t_n14& __n14 = *(t_n14*)(blob1 + 0x00443414 - BMF_BLOB_BASE);
typedef int32_t t_n9;
static t_n9& __n9 = *(t_n9*)(blob1 + 0x00443418 - BMF_BLOB_BASE);
typedef int32_t t_n7;
static t_n7& __n7 = *(t_n7*)(blob1 + 0x0044341C - BMF_BLOB_BASE);
typedef int32_t t_dword_443420;
static t_dword_443420& __dword_443420 = *(t_dword_443420*)(blob1 + 0x00443420 - BMF_BLOB_BASE);
typedef int32_t t_dword_443424;
static t_dword_443424& __dword_443424 = *(t_dword_443424*)(blob1 + 0x00443424 - BMF_BLOB_BASE);
typedef int32_t t_dword_443428;
static t_dword_443428& __dword_443428 = *(t_dword_443428*)(blob1 + 0x00443428 - BMF_BLOB_BASE);
typedef int32_t t_dword_44342C;
static t_dword_44342C& __dword_44342C = *(t_dword_44342C*)(blob1 + 0x0044342C - BMF_BLOB_BASE);
typedef int32_t t_buf_1;
static t_buf_1& __buf_1 = *(t_buf_1*)(blob1 + 0x00443430 - BMF_BLOB_BASE);
typedef int32_t t_dword_443434;
static t_dword_443434& __dword_443434 = *(t_dword_443434*)(blob1 + 0x00443434 - BMF_BLOB_BASE);
typedef int32_t t_dword_443438;
static t_dword_443438& __dword_443438 = *(t_dword_443438*)(blob1 + 0x00443438 - BMF_BLOB_BASE);
typedef char t_buf_0[0x10000];
static t_buf_0& __buf_0 = *(t_buf_0*)(blob1 + 0x00443440 - BMF_BLOB_BASE);
typedef uint8_t t_byte_445440[544];
static t_byte_445440& __byte_445440 = *(t_byte_445440*)(blob1 + 0x00445440 - BMF_BLOB_BASE);
typedef int32_t t_dword_445660[32];
static t_dword_445660& __dword_445660 = *(t_dword_445660*)(blob1 + 0x00445660 - BMF_BLOB_BASE);
typedef char t_byte_445700;
static t_byte_445700& __byte_445700 = *(t_byte_445700*)(blob1 + 0x00445700 - BMF_BLOB_BASE);
typedef int32_t t_n256_1;
static t_n256_1& __n256_1 = *(t_n256_1*)(blob1 + 0x00445708 - BMF_BLOB_BASE);
typedef int32_t t_n8_1;
static t_n8_1& __n8_1 = *(t_n8_1*)(blob1 + 0x0044570C - BMF_BLOB_BASE);
typedef int32_t t_n8_0;
static t_n8_0& __n8_0 = *(t_n8_0*)(blob1 + 0x00445710 - BMF_BLOB_BASE);
typedef uint8_t t_byte_445714[0x10000];
static t_byte_445714& __byte_445714 = *(t_byte_445714*)(blob1 + 0x00445714 - BMF_BLOB_BASE);
typedef uint8_t t_byte_445715[0x10000];
static t_byte_445715& __byte_445715 = *(t_byte_445715*)(blob1 + 0x00445715 - BMF_BLOB_BASE);
typedef uint8_t t_byte_445716[0x10000];
static t_byte_445716& __byte_445716 = *(t_byte_445716*)(blob1 + 0x00445716 - BMF_BLOB_BASE);
typedef char t_n2_0;
static t_n2_0& __n2_0 = *(t_n2_0*)(blob1 + 0x0044571C - BMF_BLOB_BASE);
typedef char t_byte_44571D;
static t_byte_44571D& __byte_44571D = *(t_byte_44571D*)(blob1 + 0x0044571D - BMF_BLOB_BASE);
typedef char t_byte_44571E;
static t_byte_44571E& __byte_44571E = *(t_byte_44571E*)(blob1 + 0x0044571E - BMF_BLOB_BASE);
typedef char t_n4;
static t_n4& __n4 = *(t_n4*)(blob1 + 0x00445720 - BMF_BLOB_BASE);
typedef char t_n2_1;
static t_n2_1& __n2_1 = *(t_n2_1*)(blob1 + 0x00445721 - BMF_BLOB_BASE);
typedef char t_byte_445722;
static t_byte_445722& __byte_445722 = *(t_byte_445722*)(blob1 + 0x00445722 - BMF_BLOB_BASE);
typedef char t_byte_445724;
static t_byte_445724& __byte_445724 = *(t_byte_445724*)(blob1 + 0x00445724 - BMF_BLOB_BASE);
typedef char t_n4_0;
static t_n4_0& __n4_0 = *(t_n4_0*)(blob1 + 0x00445725 - BMF_BLOB_BASE);
typedef char t_byte_445726;
static t_byte_445726& __byte_445726 = *(t_byte_445726*)(blob1 + 0x00445726 - BMF_BLOB_BASE);
typedef char t_byte_445728;
static t_byte_445728& __byte_445728 = *(t_byte_445728*)(blob1 + 0x00445728 - BMF_BLOB_BASE);
typedef char t_n8_2;
static t_n8_2& __n8_2 = *(t_n8_2*)(blob1 + 0x00445729 - BMF_BLOB_BASE);
typedef char t_byte_44572A;
static t_byte_44572A& __byte_44572A = *(t_byte_44572A*)(blob1 + 0x0044572A - BMF_BLOB_BASE);
typedef char t_byte_44572C;
static t_byte_44572C& __byte_44572C = *(t_byte_44572C*)(blob1 + 0x0044572C - BMF_BLOB_BASE);
typedef char t_n16;
static t_n16& __n16 = *(t_n16*)(blob1 + 0x0044572D - BMF_BLOB_BASE);
typedef char t_byte_44572E;
static t_byte_44572E& __byte_44572E = *(t_byte_44572E*)(blob1 + 0x0044572E - BMF_BLOB_BASE);
typedef char t_byte_445730;
static t_byte_445730& __byte_445730 = *(t_byte_445730*)(blob1 + 0x00445730 - BMF_BLOB_BASE);
typedef char t_n32;
static t_n32& __n32 = *(t_n32*)(blob1 + 0x00445731 - BMF_BLOB_BASE);
typedef uint8_t t_byte_445732[10];
static t_byte_445732& __byte_445732 = *(t_byte_445732*)(blob1 + 0x00445732 - BMF_BLOB_BASE);
typedef int32_t t_dword_44573C[0x10000];
static t_dword_44573C& __dword_44573C = *(t_dword_44573C*)(blob1 + 0x0044573C - BMF_BLOB_BASE);
typedef int32_t t_n4_4;
static t_n4_4& __n4_4 = *(t_n4_4*)(blob1 + 0x00445740 - BMF_BLOB_BASE);
typedef int32_t t_n4_3;
static t_n4_3& __n4_3 = *(t_n4_3*)(blob1 + 0x00445744 - BMF_BLOB_BASE);
typedef int32_t t_n15;
static t_n15& __n15 = *(t_n15*)(blob1 + 0x00445748 - BMF_BLOB_BASE);
typedef int32_t t_n15_0;
static t_n15_0& __n15_0 = *(t_n15_0*)(blob1 + 0x0044574C - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445760[0x10000];
static t_xmmword_445760& __xmmword_445760 = *(t_xmmword_445760*)(blob1 + 0x00445760 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445770[0x10000];
static t_xmmword_445770& __xmmword_445770 = *(t_xmmword_445770*)(blob1 + 0x00445770 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445780[0x10000];
static t_xmmword_445780& __xmmword_445780 = *(t_xmmword_445780*)(blob1 + 0x00445780 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445790[0x10000];
static t_xmmword_445790& __xmmword_445790 = *(t_xmmword_445790*)(blob1 + 0x00445790 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4457A0[0x10000];
static t_xmmword_4457A0& __xmmword_4457A0 = *(t_xmmword_4457A0*)(blob1 + 0x004457A0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4457B0[0x10000];
static t_xmmword_4457B0& __xmmword_4457B0 = *(t_xmmword_4457B0*)(blob1 + 0x004457B0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4457C0;
static t_xmmword_4457C0& __xmmword_4457C0 = *(t_xmmword_4457C0*)(blob1 + 0x004457C0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4457D0;
static t_xmmword_4457D0& __xmmword_4457D0 = *(t_xmmword_4457D0*)(blob1 + 0x004457D0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4457E0;
static t_xmmword_4457E0& __xmmword_4457E0 = *(t_xmmword_4457E0*)(blob1 + 0x004457E0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4457F0;
static t_xmmword_4457F0& __xmmword_4457F0 = *(t_xmmword_4457F0*)(blob1 + 0x004457F0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445800;
static t_xmmword_445800& __xmmword_445800 = *(t_xmmword_445800*)(blob1 + 0x00445800 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445810;
static t_xmmword_445810& __xmmword_445810 = *(t_xmmword_445810*)(blob1 + 0x00445810 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445820;
static t_xmmword_445820& __xmmword_445820 = *(t_xmmword_445820*)(blob1 + 0x00445820 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445830;
static t_xmmword_445830& __xmmword_445830 = *(t_xmmword_445830*)(blob1 + 0x00445830 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445840;
static t_xmmword_445840& __xmmword_445840 = *(t_xmmword_445840*)(blob1 + 0x00445840 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445850;
static t_xmmword_445850& __xmmword_445850 = *(t_xmmword_445850*)(blob1 + 0x00445850 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445860;
static t_xmmword_445860& __xmmword_445860 = *(t_xmmword_445860*)(blob1 + 0x00445860 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445870;
static t_xmmword_445870& __xmmword_445870 = *(t_xmmword_445870*)(blob1 + 0x00445870 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445880;
static t_xmmword_445880& __xmmword_445880 = *(t_xmmword_445880*)(blob1 + 0x00445880 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_445890;
static t_xmmword_445890& __xmmword_445890 = *(t_xmmword_445890*)(blob1 + 0x00445890 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4458A0;
static t_xmmword_4458A0& __xmmword_4458A0 = *(t_xmmword_4458A0*)(blob1 + 0x004458A0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4458B0;
static t_xmmword_4458B0& __xmmword_4458B0 = *(t_xmmword_4458B0*)(blob1 + 0x004458B0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4458C0;
static t_xmmword_4458C0& __xmmword_4458C0 = *(t_xmmword_4458C0*)(blob1 + 0x004458C0 - BMF_BLOB_BASE);
typedef __m128i t_xmmword_4458D0;
static t_xmmword_4458D0& __xmmword_4458D0 = *(t_xmmword_4458D0*)(blob1 + 0x004458D0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4458E0;
static t_dword_4458E0& __dword_4458E0 = *(t_dword_4458E0*)(blob1 + 0x004458E0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4458E4;
static t_dword_4458E4& __dword_4458E4 = *(t_dword_4458E4*)(blob1 + 0x004458E4 - BMF_BLOB_BASE);
typedef int32_t t_dword_4458E8;
static t_dword_4458E8& __dword_4458E8 = *(t_dword_4458E8*)(blob1 + 0x004458E8 - BMF_BLOB_BASE);
typedef int32_t t_dword_4458EC;
static t_dword_4458EC& __dword_4458EC = *(t_dword_4458EC*)(blob1 + 0x004458EC - BMF_BLOB_BASE);
typedef int32_t t_dword_4458F0;
static t_dword_4458F0& __dword_4458F0 = *(t_dword_4458F0*)(blob1 + 0x004458F0 - BMF_BLOB_BASE);
typedef int32_t t_dword_4458F4;
static t_dword_4458F4& __dword_4458F4 = *(t_dword_4458F4*)(blob1 + 0x004458F4 - BMF_BLOB_BASE);
typedef int32_t t_psub_402E30;
static t_psub_402E30& __psub_402E30 = *(t_psub_402E30*)(blob1 + 0x00445930 - BMF_BLOB_BASE);

// ---------------------------------------------------------------------------
// The range coder.
//
// BMF's entropy coder, as it implements it: a carry-counting range coder over
// a 31-bit `low`, renormalised a byte at a time, in the Subbotin lineage.
// ALGORITHM.md §5 describes it in prose; this is the same thing as code.
//
// The state used to be six globals in the block above -- __n0x800000,
// __n0x7F800000, __dword_4456E8, __dword_4456EC, __byte_4456F0 -- and the
// arguments of a coding step three more, __n0x2000_1 / __n0x2000_0 /
// __n0x2000, which every caller assigned before calling an entry that took no
// parameters.  Both sets are members here, and the entries take arguments.
//
// It lives in this file rather than in bmf.cpp because it shares its output
// cursor with the bit packer: __Buffer_0 is the one position both advance
// through, and that is a blob global declared above.
//
// Encoder and decoder never both run, so `low` doubles as the decoder's
// `code`, and `rdiv` occupies what is `pending` while encoding -- exactly as
// the donor overlapped them in one word.
// ---------------------------------------------------------------------------
__attribute__((noreturn)) void __exit_402E40(int32_t Code, ...);

struct RangeCoder {
  static const uint32_t kTop    = 0x00800000;   // renormalise at or below this
  static const uint32_t kPend   = 0x7F800000;   // low here still has a live carry
  static const uint32_t kCarry  = 0x80000000;   // the carry bit itself
  static const uint32_t kMask   = 0x7FFFFFFF;   // low is 31 bits
  static const uint8_t  kMarker = 0x97;         // section marker == the initial cache

  // Private, so that "does anything outside still touch the coder's state?"
  // is a question the compiler answers rather than a grep.
 private:
  uint32_t range;      // (0x00800000, 0x80000000]
  uint32_t low;        // encoding: low.  decoding: code.  One word, two jobs.
  uint8_t  cache;      // encoding: the byte a carry could still reach
                       // decoding: the previous input byte, for its dropped bit
  uint32_t pending;    // encoding: deferred 0xFF bytes behind the cache
  uint32_t rdiv;       // decoding: the range/tot from the last get_freq
  uint32_t bytes;      // encoding: emitted so far; the flush writes it out

  uint8_t *p()             { return (uint8_t *)__Buffer_0; }
  void     set_p(uint8_t *q) { __Buffer_0 = (int32_t)q; }

 public:

  // ---- encoder ---------------------------------------------------------

  void enc_init() {
    range = 0x80000000; low = 0; pending = 0; bytes = 0; cache = kMarker;
  }

  // Emit the held cache byte plus `carry`, then the run of bytes the carry
  // rippled through: 0xFF each if it did not carry, 0x00 each if it did.
  void emit(uint32_t carry) {
    uint8_t *q = p();
    *q++ = (uint8_t)(cache + carry);
    for (; pending; --pending)
      *q++ = (uint8_t)(carry - 1);
    set_p(q);
    cache = (uint8_t)(low >> 23);
  }

  void enc_normalise() {
    while (range <= kTop) {
      ++bytes;
      if      (low < kPend)    emit(0);      // the top byte is settled
      else if (low & kCarry)   emit(1);      // it carried
      else                     ++pending;    // it is 0xFF and might still carry
      range <<= 8;
      low = (low << 8) & kMask;
    }
  }

  // Encode(cumFreq, cumFreq + freq, totFreq).
  uint32_t encode(uint32_t cum, uint32_t high, uint32_t tot) {
    enc_normalise();
    uint32_t r = range / tot;
    uint32_t below = r * cum;
    uint32_t width = r * (high - cum);
    range = (high < tot) ? width : range - below;
    low  += below;
    return width;                        // freq * r; one caller reads it
  }

  // One bit against two frequencies.  Returns the width of the interval it
  // chose -- which is the new `range`, and is what one caller passes on as its
  // own result.
  uint32_t encode_bit(uint32_t f0, uint32_t f1, int32_t bit) {
    enc_normalise();
    uint32_t rt = f0 * (range / (f0 + f1));
    if (bit) { low += rt; range -= rt; }
    else     { range = rt; }
    return range;
  }

  // Close the section: the last emit, a rounded final byte, the section
  // length, padding, and the marker.  Leaves the bit packer word-aligned.
  void flush() {
    enc_normalise();
    bytes += 5;
    uint32_t len     = bytes;
    uint32_t rounded = ((low & 0x7FFFFF) >= ((len & 0xFFFFFF) >> 1)) + (low >> 23);
    uint32_t carry   = (rounded > 0xFF);
    uint8_t *q = p();
    *q++ = (uint8_t)(cache + carry);
    for (; pending; --pending)
      *q++ = (uint8_t)(carry - 1);
    *q++ = (uint8_t)rounded;
    *q++ = (uint8_t)(len >> 16);
    *q++ = (uint8_t)(len >> 8);
    *q++ = (uint8_t)len;
    while ((uint32_t)(q - (uint8_t *)__Buffer) % 4 != 3)
      *q++ = 0;
    *q++ = kMarker;
    set_p(q);
    __n8 = 0; __n256 = 0; __Buffer_1 = __Buffer_0;
  }

  // ---- decoder ---------------------------------------------------------

  // Consume the marker and prime `code` with 31 bits.  range = 2^7 makes the
  // first normalise run three times, which is what fills it.
  void dec_init() {
    uint8_t *q = p();
    if (*q++ != kMarker)
      __exit_402E40(4);
    cache = *q++;
    set_p(q);
    range = 128;
    low   = cache >> 1;
  }

  // Eight more bits of a byte-aligned stream, through a window that sits one
  // bit short of a byte boundary: put back the bit `dec_init`'s >> 1 dropped
  // from the previous byte, then take seven from the next.
  void dec_normalise() {
    uint8_t *q = p();
    while (range <= kTop) {
      uint32_t head = (uint32_t)(uint8_t)(cache << 7) | (low << 8);
      cache = *q++;
      low   = (cache >> 1) | head;
      range <<= 8;
    }
    set_p(q);
  }

  // Which slot of `tot` the code falls in.  decode() finishes the step once
  // the caller has turned that into a (cumFreq, cumFreq + freq) pair.
  uint32_t get_freq(uint32_t tot) {
    dec_normalise();
    rdiv = range / tot;
    uint32_t count = low / rdiv;
    return (count >= tot) ? tot - 1 : count;
  }

  void decode(uint32_t cum, uint32_t high, uint32_t tot) {
    uint32_t below = cum * rdiv;
    low  -= below;
    range = (high < tot) ? (high - cum) * rdiv : range - below;
  }

  int32_t decode_bit(uint32_t f0, uint32_t f1) {
    dec_normalise();
    uint32_t rt = f0 * (range / (f0 + f1));
    if (low >= rt) { range -= rt; low -= rt; return 1; }
    range = rt;
    return 0;
  }

  // Skip to the marker that closed the section, and hand the position back to
  // the bit packer.
  void finish() {
    dec_normalise();
    uint8_t *q = p();
    while (*q++ != kMarker) { }
    set_p(q);
    __Buffer_1 = __Buffer_0;
    __n8 = 0; __n256 = 0;
  }
};

static RangeCoder rc;

FILE *__sub_402FB0(FILE **_this)
{
  ;
  FILE *Stream_v;
  Stream_v = *(_this + 1);
  if ( Stream_v )
  {
    fseek(Stream_v, 0, 2);
    return (FILE *)fclose(*(_this + 1));
  }
  return Stream_v;
}
static inline FILE * __fwd_sub_402DF0_sub_402FB0(void *a0) { return __sub_402FB0((FILE **)a0); }

FILE **__sub_402DF0(FILE **Block, char a2)
{
  ;
  __fwd_sub_402DF0_sub_402FB0(Block);
  if ( (a2 & 1) != 0 )
    free(Block);
  return Block;
}

 BMF_SSE void __sub_410650(uint32_t Src, int32_t i, int32_t a3)
{
  ;
  __m128i si128, v7, v8, v9, v11, v12, v19, v20, v22, v23, v24, v25, v26, v27, v28;
  char v16, v17, v18, v31, v32, v40;
  int32_t n256, n256_1, n128_1, n128, v15, v29, n256_2, v33, v35, v36, n128_2;
  uint32_t n0x70, j, k, v34;
  alignas(16) uint8_t v39[255];
  n256 = __n256_0[0];
  n256_1 = __n256_0[0];
  if ( __n256_0[0] )
  {
    n128_1 = 2 * __n256_0[0] + 1;
    if ( __n2 )
    {
      si128 = _mm_load_si128((const __m128i *)&__xmmword_439690);
      v7 = _mm_load_si128((const __m128i *)&__xmmword_439670);
      v8 = _mm_load_si128((const __m128i *)&__xmmword_4396A0);
      v9 = _mm_load_si128((const __m128i *)&__xmmword_4396B0);
      n0x70 = 0;
      v39[0] = 0;
      v40 = 0x80;
      do
      {
        v11 = v8;
        v12 = v8;
        v8 = _mm_add_epi8(v8, si128);
        *(__m128i *)&v39[2 * n0x70 + 1] = _mm_unpacklo_epi8(v11, v9);
        *(__m128i *)&v39[2 * n0x70 + 17] = _mm_unpackhi_epi8(v12, v9);
        v9 = _mm_add_epi8(v9, v7);
        n0x70 += 16;
      }
      while ( n0x70 < 0x70 );
      for ( j = 0; j < 7; ++j )
      {
        v39[4 * j + 225] = -2 * j - 113;
        v39[4 * j + 227] = -2 * j - 114;
        v39[4 * j + 226] = 2 * j + 113;
        v39[4 * j + 228] = 2 * j + 114;
      }
      v39[253] = -127;
      n128 = n128_1;
      v39[254] = 127;
      if ( n128_1 < 128 )
      {
        n128_2 = n128_1;
        v15 = 1;
        do
        {
          v16 = v39[2 * v15];
          v39[2 * v15] = v39[2 * n128];
          v17 = v39[2 * n128 - 1];
          v39[2 * n128] = v16;
          v18 = v39[2 * v15 - 1];
          v39[2 * v15 - 1] = v17;
          v39[2 * n128 - 1] = v18;
          n128 += n128_2;
          ++v15;
        }
        while ( n128 < 128 );
      }
    }
    else
    {
      v19 = _mm_load_si128((const __m128i *)&__xmmword_439670);
      v20 = _mm_load_si128((const __m128i *)&__xmmword_439680);
      for ( k = 0; k < 0x100; k += 128 )
      {
        *(__m128i *)&v39[k] = v20;
        v22 = _mm_add_epi8(v20, v19);
        *(__m128i *)&v39[k + 16] = v22;
        v23 = _mm_add_epi8(v22, v19);
        *(__m128i *)&v39[k + 32] = v23;
        v24 = _mm_add_epi8(v23, v19);
        *(__m128i *)&v39[k + 48] = v24;
        v25 = _mm_add_epi8(v24, v19);
        *(__m128i *)&v39[k + 64] = v25;
        v26 = _mm_add_epi8(v25, v19);
        *(__m128i *)&v39[k + 80] = v26;
        v27 = _mm_add_epi8(v26, v19);
        *(__m128i *)&v39[k + 96] = v27;
        v28 = _mm_add_epi8(v27, v19);
        *(__m128i *)&v39[k + 112] = v28;
        v20 = _mm_add_epi8(v28, v19);
      }
      v29 = 0;
      if ( n256 < 256 )
      {
        n256_2 = n256;
        do
        {
          v31 = v39[v29];
          v39[v29++] = v39[n256_2];
          v39[n256_2] = v31;
          n256_2 += n128_1;
        }
        while ( n256_2 < 256 );
        n256_1 = __n256_0[0];
        n256 = n256_2;
      }
      if ( n256 - n256_1 < 256 )
      {
        v32 = v39[v29];
        v39[v29] = v40;
        v40 = v32;
      }
    }
    v33 = a3 * i;
    if ( Src < Src + a3 * i )
    {
      if ( v33 / 2 )
      {
        v34 = 0;
        do
        {
          v35 = *(uint8_t *)(Src + 2 * v34 + 1);
          *(uint8_t *)(Src + 2 * v34) = v39[*(uint8_t *)(Src + 2 * v34)];
          *(uint8_t *)(Src + 2 * v34++ + 1) = v39[v35];
        }
        while ( v34 < v33 / 2 );
        v36 = 2 * v34 + 1;
      }
      else
      {
        v36 = 1;
      }
      if ( v33 > (uint32_t)(v36 - 1) )
        *(uint8_t *)(Src + v36 - 1) = v39[*(uint8_t *)(Src + v36 - 1)];
    }
  }
}

 BMF_SSE uint32_t __sub_4108C0(int32_t Src, int32_t i, int32_t a3)
{
  ;
  __m128i si128, v7, v13, v14;
  char v11, v26;
  int32_t v9, i_1, v16, v17, v18, v19, v20, v21, v22, v24, v27, v29;
  uint32_t j, k, m, n15, v23, v25, n15_1;
  uint8_t *v3, *v4, *v28;
  alignas(16) uint8_t v31[272];
  v31[0] = 0;
  v3 = (uint8_t *)(Src + a3 * i);
  v4 = &v3[-i];
  v31[128] = -1;
  for ( j = 0; j < 7; ++j )
  {
    v31[2 * j + 1] = 4 * j + 2;
    v31[2 * j + 2] = 4 * j + 4;
  }
  si128 = _mm_load_si128((const __m128i *)&__xmmword_4396C0);
  v7 = _mm_load_si128((const __m128i *)&__xmmword_4396D0);
  v31[15] = 30;
  for ( k = 15; k < 0x7F; k += 16 )
  {
    *(__m128i *)&v31[k + 1] = v7;
    v7 = _mm_add_epi8(v7, si128);
  }
  v9 = 0;
  for ( m = 0; m < 7; ++m )
  {
    v31[2 * m + 129] = 2 * v9 - 3;
    v11 = 2 * v9 - 5;
    v9 -= 2;
    v31[2 * m + 130] = v11;
  }
  i_1 = i;
  v13 = _mm_load_si128((const __m128i *)&__xmmword_4396E0);
  v14 = _mm_load_si128((const __m128i *)&__xmmword_4396F0);
  n15 = 15;
  v31[143] = -31;
  do
  {
    *(__m128i *)&v31[n15 + 129] = v14;
    v14 = _mm_add_epi8(v14, v13);
    n15 += 16;
  }
  while ( n15 < 0x7F );
  v16 = a3 - 1;
  if ( a3 == 1 )
    goto LABEL_24;
  do
  {
    v17 = i - 1;
    if ( i_1 == 1 )
      goto LABEL_23;
    v29 = v16;
    do
    {
      v18 = (uint8_t)*--v4;
      v19 = (uint8_t)*(--v3 - 1);
      v20 = (uint8_t)v3[-i - 1];
      if ( v19 < v18 )
      {
        if ( v20 < v19 )
        {
          LOBYTE(v19) = *v4;
          goto LABEL_21;
        }
        if ( v20 <= v18 )
LABEL_20:
          LOBYTE(v19) = v18 + v19 - v20;
      }
      else
      {
        if ( v20 > v19 )
        {
          LOBYTE(v19) = *v4;
          goto LABEL_21;
        }
        if ( v20 >= v18 )
          goto LABEL_20;
      }
LABEL_21:
      v21 = (uint8_t)v31[(uint8_t)(*v3 - v19)];
      *v3 = v21;
      ++*(uint32_t *)&__buf[4 * v21];
      --v17;
    }
    while ( v17 );
    v16 = v29;
    i_1 = i;
LABEL_23:
    --v4;
    --v3;
    n15 = (uint8_t)v31[(uint8_t)(*v3 - v3[-i_1])];
    *v3 = n15;
    ++*(uint32_t *)&__buf[4 * n15];
    --v16;
  }
  while ( v16 );
LABEL_24:
  if ( i_1 != 1 )
  {
    n15 = i_1 - 1;
    v22 = (i_1 - 1) / 2;
    if ( v22 )
    {
      n15_1 = i_1 - 1;
      v23 = 0;
      v24 = 0;
      v25 = v22;
      do
      {
        v26 = v3[v24 - 2];
        ++v23;
        v3[v24 - 1] = v31[(uint8_t)(v3[v24 - 1] - v26)];
        v3[v24 - 2] = v31[(uint8_t)(v26 - v3[v24 - 3])];
        v24 -= 2;
      }
      while ( v23 < v25 );
      n15 = n15_1;
      v27 = 2 * v23 + 1;
    }
    else
    {
      v27 = 1;
    }
    if ( n15 > v27 - 1 )
    {
      v28 = &v3[-v27];
      n15 = (uint8_t)(*v28 - *(v28 - 1));
      *v28 = v31[(uint8_t)n15];
    }
  }
  return n15;
}

 BMF_SSE uint32_t __sub_4118A0(uint8_t *a1, char *a2)
{
  ;
  __m128i si128, v7;
  char *v10, *v12, v13, *v14, *v15, v16, v17, *v18, *v19, v25, v26, v27, v42, *v44, v48, v53;
  int32_t n128_1, v8, v9, v11, n128, v24, n128_6, v29, v30, n128_5, v33, n128_3, n128_4, v36,
          n128_10, v38, n128_8, n128_9, v45, n128_11, v47, n128_2, n128_7;
  uint32_t n0x100_1, n0x100_2, n0x100, j, k, i, v32, m, n0x80, v49, v52;
  uint8_t *v50, *v51;
  n128_1 = 2 * __n256_0[0] + 1;
  if ( !__n2 )
  {
    n0x100_1 = (uint8_t)a2 & 0xF;
    if ( ((uint8_t)a2 & 0xF) != 0 )
    {
      n0x100_1 = 16 - n0x100_1;
      n0x100_2 = 0;
      do
      {
        a2[n0x100_2] = n0x100_2;
        ++n0x100_2;
      }
      while ( n0x100_2 < n0x100_1 );
    }
    si128 = _mm_load_si128((const __m128i *)&__xmmword_4397B0);
    n0x100 = 256 - (-n0x100_1 & 0xF);
    v7 = _mm_unpacklo_epi64(
           _mm_unpacklo_epi32(
             _mm_unpacklo_epi16(
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1), _mm_cvtsi32_si128(n0x100_1 + 1)),
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 2), _mm_cvtsi32_si128(n0x100_1 + 3))),
             _mm_unpacklo_epi16(
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 4), _mm_cvtsi32_si128(n0x100_1 + 5)),
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 6), _mm_cvtsi32_si128(n0x100_1 + 7)))),
           _mm_unpacklo_epi32(
             _mm_unpacklo_epi16(
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 8), _mm_cvtsi32_si128(n0x100_1 + 9)),
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 10), _mm_cvtsi32_si128(n0x100_1 + 11))),
             _mm_unpacklo_epi16(
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 12), _mm_cvtsi32_si128(n0x100_1 + 13)),
               _mm_unpacklo_epi8(_mm_cvtsi32_si128(n0x100_1 + 14), _mm_cvtsi32_si128(n0x100_1 + 15)))));
    do
    {
      *(__m128i *)&a2[n0x100_1] = v7;
      v7 = _mm_add_epi8(v7, si128);
      n0x100_1 += 16;
    }
    while ( n0x100_1 < n0x100 );
    for ( ; n0x100 < 0x100; ++n0x100 )
      a2[n0x100] = n0x100;
    v8 = __n256_0[0];
    v9 = __n256_0[0];
    if ( __n256_0[0] >= 256 )
    {
      v11 = 0;
LABEL_29:
      if ( v9 - v8 < 256 )
      {
        v17 = a2[v11];
        a2[v11] = a2[255];
        a2[255] = v17;
      }
      goto LABEL_47;
    }
    v44 = &a2[__n256_0[0]];
    if ( n128_1 <= 0 )
    {
      v47 = (n128_1 - (int64_t)__n256_0[0] + 255) / n128_1;
      goto LABEL_26;
    }
    v47 = (n128_1 - (int64_t)__n256_0[0] + 255) / n128_1;
    if ( a2 >= v44 || __n256_0[0] < (uint32_t)v47 )
    {
      if ( n128_1 > 1 || a2 <= v44 || -__n256_0[0] < (uint32_t)(v47 * n128_1) )
        goto LABEL_26;
    }
    else if ( n128_1 > 1 || a2 <= v44 )
    {
LABEL_20:
      if ( a2 < v44 && __n256_0[0] >= (uint32_t)v47 )
        goto LABEL_22;
LABEL_26:
      v14 = a2;
      v45 = __n256_0[0];
      v11 = 0;
      v15 = &a2[__n256_0[0]];
      do
      {
        v16 = *v14;
        *v14 = *v15;
        *v15 = v16;
        v15 += n128_1;
        ++v14;
        ++v11;
      }
      while ( v11 < v47 );
      goto LABEL_28;
    }
    if ( -__n256_0[0] >= (uint32_t)(v47 * n128_1) )
    {
LABEL_22:
      v10 = a2;
      v45 = __n256_0[0];
      v11 = 0;
      v12 = &a2[__n256_0[0]];
      do
      {
        v13 = *v10;
        *v10 = *v12;
        *v12 = v13;
        v12 += n128_1;
        ++v10;
        ++v11;
      }
      while ( v11 < v47 );
LABEL_28:
      v9 = v45 + v11 * n128_1;
      v8 = __n256_0[0];
      goto LABEL_29;
    }
    goto LABEL_20;
  }
  *a2 = 0;
  v18 = a2 + 2;
  v19 = a2 + 1;
  a2[255] = 0x80;
  if ( (a2 + 1 <= a2 + 2 || (uint32_t)(v19 - v18) < 0xFE) && (v18 <= v19 || (uint32_t)(v18 - v19) < 0xFE) )
  {
    for ( i = 0; i < 0x3F; ++i )
    {
      a2[4 * i + 2] = 2 * i + 1;
      a2[4 * i + 1] = -2 * i - 1;
      a2[4 * i + 4] = 2 * i + 2;
      a2[4 * i + 3] = -2 * i - 2;
    }
    a2[254] = 127;
    a2[253] = -127;
  }
  else
  {
    for ( j = 0; j < 0x3F; ++j )
    {
      a2[4 * j + 2] = 2 * j + 1;
      a2[4 * j + 4] = 2 * j + 2;
    }
    a2[254] = 127;
    for ( k = 0; k < 0x3F; ++k )
    {
      a2[4 * k + 1] = -2 * k - 1;
      a2[4 * k + 3] = -2 * k - 2;
    }
    a2[253] = -127;
  }
  if ( __n256_0[0] )
  {
    n128 = n128_1;
    if ( n128_1 < 128 )
    {
      v24 = 1;
      do
      {
        v25 = a2[2 * v24];
        a2[2 * v24] = a2[2 * n128];
        v26 = a2[2 * n128 - 1];
        a2[2 * n128] = v25;
        v27 = a2[2 * v24 - 1];
        a2[2 * v24 - 1] = v26;
        a2[2 * n128 - 1] = v27;
        ++v24;
        n128 += n128_1;
      }
      while ( n128 < 128 );
    }
  }
LABEL_47:
  if ( __n2 )
  {
    *a1 = 0;
    n128_6 = 1;
    a1[128] = -1;
    if ( __n256_0[0] <= 0 )
      goto LABEL_52;
    v29 = -1;
    do
    {
      a1[v29 + 256] = 0;
      a1[n128_6] = 0;
      --v29;
      ++n128_6;
    }
    while ( n128_6 <= __n256_0[0] );
    if ( n128_6 < 128 )
    {
LABEL_52:
      v49 = 128 - n128_6;
      v52 = (128 - n128_6) / 2;
      v30 = 0;
      n128_5 = 1;
      if ( v52 )
      {
        n128_11 = n128_6;
        v32 = 0;
        n128_2 = n128_1;
        v33 = 0;
        v50 = &a1[n128_6];
        v51 = &a1[-n128_6];
        do
        {
          n128_3 = n128_2;
          n128_4 = n128_5 - 1;
          if ( n128_4 )
            n128_3 = n128_4;
          else
            ++v30;
          v48 = 2 * v30;
          v50[2 * v32] = 2 * v30;
          v53 = 2 * v30 - 1;
          n128_5 = n128_3 - 1;
          v51[v33 + 256] = v53;
          if ( n128_3 == 1 )
          {
            n128_5 = n128_2;
            v48 = 2 * ++v30;
            v53 = 2 * v30 - 1;
          }
          v50[2 * v32++ + 1] = v48;
          v51[v33 + 255] = v53;
          v33 -= 2;
        }
        while ( v32 < v52 );
        n128_6 = n128_11;
        v36 = 2 * v32 + 1;
      }
      else
      {
        v36 = 1;
      }
      if ( v36 - 1 < v49 )
      {
        if ( n128_5 == 1 )
          LOBYTE(v30) = v30 + 1;
        a1[n128_6 - 1 + v36] = 2 * v30;
        a1[-n128_6 - v36 + 257] = 2 * v30 - 1;
      }
    }
  }
  else
  {
    n128_7 = n128_1;
    n128_10 = n128_1 + 1;
    v38 = 0;
    for ( m = 0; m < 0x80; ++m )
    {
      n128_8 = n128_7;
      n128_9 = n128_10 - 1;
      if ( n128_9 )
        n128_8 = n128_9;
      else
        ++v38;
      v42 = v38;
      a1[2 * m] = v38;
      n128_10 = n128_8 - 1;
      if ( n128_8 == 1 )
      {
        n128_10 = n128_7;
        v42 = ++v38;
      }
      a1[2 * m + 1] = v42;
    }
  }
  for ( n0x80 = 0; n0x80 < 0x80; ++n0x80 )
  {
    a1[(uint8_t)a2[2 * n0x80] + 256] = 2 * n0x80;
    a1[(uint8_t)a2[2 * n0x80 + 1] + 256] = 2 * n0x80 + 1;
  }
  return n0x80;
}uint32_t __sub_4123E0(uint32_t tot)
{
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym + 1, tot);
  return sym;
}

int32_t __sub_412490(uint16_t *_this, uint16_t *a2, int32_t n15)
{
  ;
  char v41, v42;
  int32_t v3, v4, __sub_412490_n0x7F800000, v7, v9, v10, n0x800000_1, n0x4000, result, v18, v19,
          n0x7F800000_1, v21, v23, v24, n0x800000_5, n0x4000_1, v31, v33, v35, v36, v37, v38,
          v45, v46;
  uint16_t *this_1, *this_2;
  uint32_t __sub_412490_n0x800000, v8, v11, __sub_412490_n0x88, n0x800000_3, v22, v25, n0x88_1,
           v30, v32, v34, n0x800000_2, n0x800000_4;
  uint8_t *v12, *v26;
  v3 = *_this;
  if ( *_this )
  {
    v4 = *(_this + 1);
    if ( !*(_this + 1) )
    {
      v31 = *a2;
      v32 = v31 + a2[1];
      *_this = (v32 + (v31 << 6) - 64) / v32;
      *(_this + 1) = ((a2[1] << 6) + v32 - 64) / v32;
      *(_this + v3 - 1) += 4;
      *(_this + 2) = 512;
      v33 = a2[v3 - 1];
      a2[v3 - 1] = -3 * ((uint32_t)(3 - v33) >> 31) + v33;
      v3 = *_this;
      v4 = *(_this + 1);
    }
    __sub_412490_n0x88 = v3 + v4;
    rc.encode_bit(v3, v4, n15);
    n0x4000 = *(_this + 2);
    if ( __sub_412490_n0x88 > n0x4000 )
    {
      v30 = *(_this + 1);
      *_this -= *_this >> 1;
      *(_this + 1) = v30 - (v30 >> 1);
      if ( n0x4000 < 0x4000 )
        *(_this + 2) = n0x4000 + 64;
    }
    result = *(_this + n15) + 8;
    *(_this + n15) = result;
    a2[n15] += (uint32_t)__sub_412490_n0x88 < 0x88;
    return result;
  }
  v18 = *a2;
  v19 = a2[1];
  n0x88_1 = v18 + v19;
  rc.encode_bit(v18, v19, n15);
  n0x4000_1 = a2[2];
  if ( n0x88_1 > n0x4000_1 )
  {
    v34 = a2[1];
    *a2 -= *a2 >> 1;
    a2[1] = v34 - (v34 >> 1);
    if ( n0x4000_1 < 0x4000 )
      a2[2] = n0x4000_1 + 64;
  }
  result = a2[n15] + 8;
  a2[n15] = result;
  *_this = n15 + 1;
  return result;
}

int32_t __sub_412850(uint16_t *_this, uint16_t *a2)
{
  ;
  int32_t v2, v3, v8, result, n0x4000, v13, v14, n0x7F800000_3, v19, n0x4000_1, v24, v26;
  uint16_t *this_1;
  uint32_t __sub_412850_n0x7F800000, v5, n0x800000_1, __sub_412850_n0x88, n0x7F800000_1, v15,
           n0x800000_3, n0x88_1, n0x7F800000_4, v23, v25, v27, __sub_412850_n0x800000,
           n0x800000_2, n0x7F800000_2;
  uint8_t *v6, *v16;
  v2 = *_this;
  if ( *_this )
  {
    v3 = *(_this + 1);
    if ( !*(_this + 1) )
    {
      v24 = *a2;
      v25 = v24 + a2[1];
      *_this = (v25 + (v24 << 6) - 64) / v25;
      *(_this + 1) = ((a2[1] << 6) + v25 - 64) / v25;
      *(_this + v2 - 1) += 4;
      *(_this + 2) = 512;
      v26 = a2[v2 - 1];
      a2[v2 - 1] = -3 * ((uint32_t)(3 - v26) >> 31) + v26;
      v2 = *_this;
      v3 = *(_this + 1);
    }
    __sub_412850_n0x88 = v2 + v3;
    result = rc.decode_bit(v2, v3);
    n0x4000 = *(_this + 2);
    if ( __sub_412850_n0x88 > n0x4000 )
    {
      v23 = *(_this + 1);
      *_this -= *_this >> 1;
      *(_this + 1) = v23 - (v23 >> 1);
      if ( n0x4000 < 0x4000 )
        *(_this + 2) = n0x4000 + 64;
    }
    *(_this + result) += 8;
    a2[result] += (uint32_t)__sub_412850_n0x88 < 0x88;
  }
  else
  {
    v13 = *a2;
    v14 = a2[1];
    n0x88_1 = v13 + v14;
    result = rc.decode_bit(v13, v14);
    n0x4000_1 = a2[2];
    if ( n0x88_1 > n0x4000_1 )
    {
      v27 = a2[1];
      *a2 -= *a2 >> 1;
      a2[1] = v27 - (v27 >> 1);
      if ( n0x4000_1 < 0x4000 )
        a2[2] = n0x4000_1 + 64;
    }
    a2[result] += 8;
    *_this = result + 1;
  }
  return result;
}

int32_t __sub_412B10(uint32_t *_this, int32_t a2)
{
  ;
  char v2, v27, *v28, v29, v32, v33, *v36, *v37, *v39, *v40, *v49, v50;
  int16_t v51;
  int32_t enc_cum, enc_high, enc_tot, v3, v5, v6, v7, v8, __sub_412B10_n0x7F800000, v11, v13,
          v14, n0x800000_2, n0x800000_3, v24, n251, v35, v38, v41, v43, v45, v47, v52, v53, v54;
  uint16_t *v4, *v25, v26, v31;
  uint32_t __sub_412B10_n0x800000, v12, v15, v17, v18, i_1, i, v34, v42, v44, *this_1,
           n0x800000_1;
  uint8_t *v16;
  v2 = __byte_445700;
  v3 = *(_this + 1);
  v4 = (uint16_t *)(*(_this + 5) - 3);
  v5 = 0;
  while ( 1 )
  {
    v4 = (uint16_t *)((char *)v4 + 3);
    v6 = *v4;
    if ( __buf_0[v6] != __byte_445700 )
    {
      v7 = *((uint8_t *)v4 + 2);
      v5 += v7;
      if ( v6 == a2 )
        break;
    }
    if ( !--v3 )
    {
      if ( !v5 )
        return 0;
      enc_cum = v5;
      enc_tot = *(_this + 2) + v5;
      enc_high = enc_tot;
      do
      {
        __buf_0[*v4] = v2;
        v4 = (uint16_t *)((char *)v4 - 3);
      }
      while ( (uint32_t)v4 >= *(_this + 5) );
      v8 = 0;
      goto LABEL_9;
    }
  }
  enc_high = v5;
  i_1 = v3 - 1;
  enc_cum = v5 - v7;
  if ( i_1 )
  {
    this_1 = _this;
    for ( i = 0; i < i_1; ++i )
    {
      if ( __buf_0[*(uint16_t *)((char *)v4 + 3 * i + 3)] == __byte_445700 )
        v24 = 0;
      else
        v24 = *((uint8_t *)v4 + 3 * i + 5);
      v5 += v24;
    }
    _this = this_1;
  }
  enc_tot = *(_this + 2) + v5;
  *((uint8_t *)v4 + 2) += 4;
  v25 = (uint16_t *)*(_this + 5);
  *(_this + 3) += 4;
  if ( v4 == v25 )
  {
LABEL_37:
    n251 = *((uint8_t *)v4 + 2);
  }
  else
  {
    v26 = *v4;
    v27 = *((uint8_t *)v4 + 2);
    v28 = (char *)v4 - 3;
    v29 = *((uint8_t *)v4 - 1);
    *v4 = *(uint16_t *)((char *)v4 - 3);
    *((uint8_t *)v4 + 2) = v29;
    *(uint16_t *)v28 = v26;
    v28[2] = v27;
    v25 = (uint16_t *)*(_this + 5);
    if ( (uint16_t *)((char *)v4 - 3) == v25 )
    {
      n251 = *((uint8_t *)v4 - 1);
    }
    else
    {
      while ( 1 )
      {
        n251 = (uint8_t)v28[2];
        v4 = (uint16_t *)(v28 - 3);
        if ( n251 <= (uint8_t)*(v28 - 1) )
          break;
        v31 = *(uint16_t *)v28;
        v32 = v28[2];
        v33 = *(v28 - 1);
        *(uint16_t *)v28 = *v4;
        v28[2] = v33;
        *v4 = v31;
        *(v28 - 1) = v32;
        v25 = (uint16_t *)*(_this + 5);
        v28 -= 3;
        if ( v4 == v25 )
          goto LABEL_37;
      }
    }
  }
  v34 = *(_this + 4);
  if ( n251 > 251 || v34 < *(_this + 3) )
  {
    v35 = *(_this + 1);
    v54 = v34 < 20 * *_this;
    v36 = (char *)v25 - 3;
    do
    {
      v37 = v36;
      v36 += 3;
      v38 = (v54 + (uint32_t)(uint8_t)v36[2]) >> 1;
      v36[2] = v38;
      if ( v36 != (char *)*(_this + 5) )
      {
        v39 = v36 - 3;
        v53 = (uint8_t)*(v36 - 1);
        if ( v38 > v53 )
        {
          v51 = *(uint16_t *)v36;
          *(uint16_t *)v36 = *(uint16_t *)v39;
          v36[2] = v53;
          if ( v39 != (char *)*(_this + 5) )
          {
            v49 = v36;
            v47 = v35;
            do
            {
              v40 = v39 - 3;
              v41 = (uint8_t)*(v39 - 1);
              if ( v38 <= v41 )
                break;
              *(uint16_t *)v39 = *(uint16_t *)v40;
              v39[2] = v41;
              v39 -= 3;
            }
            while ( v40 != (char *)*(_this + 5) );
            v36 = v49;
            v35 = v47;
          }
          *(uint16_t *)v39 = v51;
          v39[2] = v38;
        }
      }
      --v35;
    }
    while ( v35 );
    v42 = *(_this + 2);
    if ( !v36[2] )
    {
      do
      {
        ++v35;
        *(_this + 2) = ++v42;
        v43 = (uint8_t)v37[2];
        v37 -= 3;
      }
      while ( !v43 );
      *(_this + 1) -= v35;
    }
    v44 = *(_this + 3);
    *(_this + 2) = v42 - (v42 >> 1);
    *(_this + 3) = v44 - (v44 >> 1);
    v8 = 1;
  }
  else
  {
    v8 = 1;
  }
LABEL_9:
  rc.encode(enc_cum, enc_high, enc_tot);
  return v8;
}

int32_t __sub_413230(uint32_t *_this, int32_t a2, uint32_t a3)
{
  ;
  bool v7;
  char v11, v12, v15, *v16, v17, v19, v20, *v23, *v24, *v26, *v27, *v33;
  int16_t v10, v34;
  int32_t n251, v8, v9, v22, v25, v28, v30, v32, v35, v36;
  uint16_t *n251_1, *n251_2, v14, v18;
  uint32_t v4, v6, v21, v29, v31;
  n251 = *(_this + 5);
  v4 = *(_this + 1);
  n251_1 = (uint16_t *)n251;
  v6 = v4;
  if ( v4 )
  {
    while ( *n251_1 != a2 )
    {
      n251_1 = (uint16_t *)((char *)n251_1 + 3);
      if ( !--v6 )
        goto LABEL_4;
    }
    *((uint8_t *)n251_1 + 2) += a3;
    *(_this + 3) += a3;
    n251_2 = (uint16_t *)*(_this + 5);
    if ( n251_1 == n251_2 )
    {
LABEL_16:
      n251 = *((uint8_t *)n251_1 + 2);
    }
    else
    {
      v14 = *n251_1;
      v15 = *((uint8_t *)n251_1 + 2);
      v16 = (char *)n251_1 - 3;
      v17 = *((uint8_t *)n251_1 - 1);
      *n251_1 = *(uint16_t *)((char *)n251_1 - 3);
      *((uint8_t *)n251_1 + 2) = v17;
      *(uint16_t *)v16 = v14;
      v16[2] = v15;
      n251_2 = (uint16_t *)*(_this + 5);
      if ( (uint16_t *)((char *)n251_1 - 3) == n251_2 )
      {
        n251 = *((uint8_t *)n251_1 - 1);
      }
      else
      {
        while ( 1 )
        {
          n251 = (uint8_t)v16[2];
          n251_1 = (uint16_t *)(v16 - 3);
          if ( n251 <= (uint8_t)*(v16 - 1) )
            break;
          v18 = *(uint16_t *)v16;
          v19 = v16[2];
          v20 = *(v16 - 1);
          *(uint16_t *)v16 = *n251_1;
          v16[2] = v20;
          *n251_1 = v18;
          *(v16 - 1) = v19;
          n251_2 = (uint16_t *)*(_this + 5);
          v16 -= 3;
          if ( n251_1 == n251_2 )
            goto LABEL_16;
        }
      }
    }
    v21 = *(_this + 4);
    if ( n251 > 251 || v21 < *(_this + 3) )
    {
      v22 = *(_this + 1);
      v36 = v21 < 20 * *_this;
      v23 = (char *)n251_2 - 3;
      do
      {
        v24 = v23;
        v23 += 3;
        v25 = (v36 + (uint32_t)(uint8_t)v23[2]) >> 1;
        v23[2] = v25;
        if ( v23 != (char *)*(_this + 5) )
        {
          v26 = v23 - 3;
          v35 = (uint8_t)*(v23 - 1);
          if ( v25 > v35 )
          {
            v34 = *(uint16_t *)v23;
            *(uint16_t *)v23 = *(uint16_t *)v26;
            v23[2] = v35;
            if ( v26 != (char *)*(_this + 5) )
            {
              v33 = v23;
              v32 = v22;
              do
              {
                v27 = v26 - 3;
                v28 = (uint8_t)*(v26 - 1);
                if ( v25 <= v28 )
                  break;
                *(uint16_t *)v26 = *(uint16_t *)v27;
                v26[2] = v28;
                v26 -= 3;
              }
              while ( v27 != (char *)*(_this + 5) );
              v23 = v33;
              v22 = v32;
            }
            *(uint16_t *)v26 = v34;
            v26[2] = v25;
          }
        }
        --v22;
      }
      while ( v22 );
      v29 = *(_this + 2);
      if ( !v23[2] )
      {
        do
        {
          ++v22;
          *(_this + 2) = ++v29;
          v30 = (uint8_t)v24[2];
          v24 -= 3;
        }
        while ( !v30 );
        *(_this + 1) -= v22;
      }
      v31 = *(_this + 3);
      *(_this + 2) = v29 - (v29 >> 1);
      *(_this + 3) = v31 - (v31 >> 1);
      return v31 >> 1;
    }
  }
  else
  {
LABEL_4:
    v7 = v4 == *_this;
    if ( v4 >= *_this )
    {
      if ( a3 <= 1 )
        return n251;
      v7 = v4 == *_this;
    }
    if ( v7 )
    {
      *(_this + 1) = --v4;
      v8 = *(uint8_t *)(3 * v4 + n251 + 2);
    }
    else
    {
      v8 = 1;
    }
    v9 = *(_this + 2);
    n251 += 3 * v4;
    *(_this + 1) = v4 + 1;
    *(_this + 2) = v8 + v9 + 1;
    *(uint8_t *)(n251 + 2) = 2;
    *(uint16_t *)n251 = a2;
    *(_this + 3) += 4;
    if ( n251 != *(_this + 5) )
    {
      v10 = *(uint16_t *)n251;
      v11 = *(uint8_t *)(n251 + 2);
      v12 = *(uint8_t *)(n251 - 1);
      *(uint16_t *)n251 = *(uint16_t *)(n251 - 3);
      *(uint8_t *)(n251 + 2) = v12;
      *(uint16_t *)(n251 - 3) = v10;
      *(uint8_t *)(n251 - 1) = v11;
    }
  }
  return n251;
}

uint16_t *__sub_413560(uint16_t *_this)
{
  ;
  *(_this + 1) = 8;
  *(_this + 2) = 2;
  *(_this + 3) = 2;
  *(_this + 4) = 2;
  *(_this + 5) = 2;
  *(_this + 6) = 3;
  *(_this + 7) = 3;
  *_this = 22;
  return _this;
}

BMF_SSE int32_t __sub_413900(uint16_t *_this, int32_t n2) {
  ;
  __m128i v6, v7;
  bool v47;
  char v55, v70;
  int16_t v24, v42;
  int32_t n4, v8, v9, __sub_413900_n0x7F800000, v12, v14, v15, v19, v20, n0x800000_6,
          n0x7F800000_6, n0x800000_5, v25, v27, v28, v29, n0x7F800000_3, v32, v33, v46, v50, v53,
          v56, n0x7F800000_1, n0x7F800000_2, v59, v60, i, v65, v66, n0x7F800000_5, n0x7F800000_4,
          n0x7F800000_7, n0x800000_1, v73;
  uint16_t *v3, *v26, n0x4000, v39, v41, *v49, *v51, *this_1, *this_2;
  uint32_t n4_2, n4_3, __sub_413900_n0x800000, v13, v16, n0x800000_2, v31, v34, n0x800000_4, v38,
           v40, v43, v44, v45, v48, v52, v54, n4_1, n0x800000_3;
  uint8_t *v17, *v35;
  n4 = *((uint8_t *)__dword_445660 + n2);
  n4_1 = n4;
  v3 = _this + 2;
  v51 = _this + 2;
  if ( n4 )
  {
    if ( n4 < 4 )
    {
      v8 = 0;
      n4_2 = 0;
    }
    else
    {
      n4_2 = n4 - (n4 & 3);
      n4_3 = 0;
      v6 = 0;
      do
      {
        v6 = _mm_add_epi32(v6, _mm_unpacklo_epi16(_mm_loadl_epi64((const __m128i *)&v3[n4_3]), (__m128i)0LL));
        n4_3 += 4;
      }
      while ( n4_3 < n4_2 );
      v51 = _this + n4_3 + 2;
      v7 = _mm_add_epi32(v6, _mm_srli_si128(v6, 8));
      v8 = _mm_cvtsi128_si32(_mm_add_epi32(v7, _mm_srli_si128(v7, 4)));
    }
    if ( n4_2 < n4_1 )
    {
      do
      {
        v8 += v3[n4_2];
        v9 = (int32_t)&v3[++n4_2];
      }
      while ( n4_2 < n4_1 );
      v51 = (uint16_t *)v9;
    }
  }
  else
  {
    v8 = 0;
  }
  v52 = v8 + *v51;
  v48 = *_this;
  n0x800000_5 = rc.encode(v8, v52, v48);
  if ( *_this > 0x4000u )
  {
    n0x7F800000_7 = n0x7F800000_6;
    v39 = *(_this + 2) - (*(_this + 2) >> 1);
    v40 = *(_this + 3);
    *(_this + 2) = v39;
    LOWORD(v40) = v40 - (v40 >> 1);
    *(_this + 3) = v40;
    LOWORD(v40) = v39 + v40;
    v41 = *(_this + 4) - (*(_this + 4) >> 1);
    *(_this + 4) = v41;
    v42 = v40 + v41;
    LOWORD(v40) = *(_this + 5) - (*(_this + 5) >> 1);
    v43 = *(_this + 6);
    *(_this + 5) = v40;
    LOWORD(v43) = v43 - (v43 >> 1);
    *(_this + 6) = v43;
    LOWORD(v43) = v42 + v40 + v43;
    LOWORD(v40) = *(_this + 7) - (*(_this + 7) >> 1);
    v44 = *(_this + 8);
    *(_this + 7) = v40;
    LOWORD(v40) = v43 + v40;
    LOWORD(v44) = v44 - (v44 >> 1);
    v45 = *(_this + 9);
    *(_this + 8) = v44;
    LOWORD(v44) = v40 + v44;
    LOWORD(v45) = v45 - (v45 >> 1);
    v46 = *(_this + 1);
    v50 = v46;
    *(_this + 9) = v45;
    *_this = v44 + v45;
    v47 = v46 <= 4 * __n8_1;
    n0x7F800000_6 = n0x7F800000_7;
    if ( v47 )
    {
      n0x800000_5 = 4 * (v50 > __n8_1);
      v24 = v50 - n0x800000_5;
      *(_this + 1) = v50 - n0x800000_5;
    }
    else
    {
      v24 = v50 - 16;
      *(_this + 1) = v50 - 16;
    }
  }
  else
  {
    v24 = *(_this + 1);
  }
  *v51 += v24;
  *_this += *(_this + 1);
  if ( n2 >= 2 )
  {
    n0x7F800000_5 = n0x7F800000_6;
    v66 = (uint8_t)__byte_445715[4 * n4_1];
    v56 = n2 - (uint8_t)__byte_445714[4 * n4_1];
    v59 = 0;
    n0x800000_3 = n0x800000_6;
    this_2 = _this;
    for ( i = 1; ; i *= 2 )
    {
      v25 = (int32_t)&this_2[2 * (uint8_t)__byte_445716[4 * n4_1] + 8];
      v26 = (uint16_t *)(v25 + 4 * (i + v59));
      v65 = v26[1];
      v27 = (v66 & v56) != 0;
      v28 = *v26;
      n0x800000_5 = rc.encode_bit(v28, v65, v27);
      n0x4000 = v26[v27];
      if ( n0x4000 > 0x4000u )
      {
        v38 = v26[1];
        *v26 -= *v26 >> 1;
        v26[1] = v38 - (v38 >> 1);
        n0x4000 = v26[v27];
      }
      v26[v27] = __n8_0 + n0x4000;
      v66 >>= 1;
      v59 = v27 + 2 * v59;
      if ( !v66 )
        return n0x800000_5;
    }
  }
  return n0x800000_5;
}
static inline int32_t __fwd_sub_4135A0_sub_413900(void *a0, int32_t a1) { return __sub_413900((uint16_t *)a0, a1); }

 BMF_SSE int32_t __sub_4135A0(uint16_t *a1, int32_t n5, int32_t a3, int32_t n5a)
{
  ;
  __m128i v9, v10;
  bool v29;
  char v39;
  int16_t v4;
  int32_t n5a_1, n0x2000_2, v14, n0x7F800000_1, v17, v18, v22, v23, n0x800000_3, n0x800000_4,
          result, n256, n5a_2, __sub_4135A0_n0x7F800000, n0x7F800000_2, v42, n0x800000_1;
  uint16_t *v12, *v27, *v30, v31, *v35, *v37;
  uint32_t __sub_4135A0_n0x2000, n5a_3, n5a_4, __sub_4135A0_n0x800000, v16, v19, n0x800000_2,
           __sub_4135A0_n0x2000_1, n0x2000_3, n0x2000_4;
  uint8_t *v20;
  v4 = *a1;
  n5a_1 = 6 - (n5a & 1);
  if ( n5a < 5 )
    n5a_1 = n5a;
  n5a_2 = n5a_1;
  v37 = a1 + 1;
  v35 = a1 + 1;
  __sub_4135A0_n0x2000 = v4 & 0x7FFF;
  if ( n5a_1 )
  {
    if ( n5a_1 < 4 )
    {
      n0x2000_2 = 0;
      n5a_3 = 0;
    }
    else
    {
      n5a_3 = n5a_1 - (n5a_1 & 3);
      n5a_4 = 0;
      v9 = 0;
      do
      {
        v9 = _mm_add_epi32(v9, _mm_unpacklo_epi16(_mm_loadl_epi64((const __m128i *)&v37[n5a_4]), (__m128i)0LL));
        n5a_4 += 4;
      }
      while ( n5a_4 < n5a_3 );
      v35 = &a1[n5a_4 + 1];
      v10 = _mm_add_epi32(v9, _mm_srli_si128(v9, 8));
      n0x2000_2 = _mm_cvtsi128_si32(_mm_add_epi32(v10, _mm_srli_si128(v10, 4)));
    }
    if ( n5a_3 < n5a_2 )
    {
      __sub_4135A0_n0x2000_1 = __sub_4135A0_n0x2000;
      do
      {
        n0x2000_2 += v37[n5a_3];
        v12 = &a1[n5a_3++ + 2];
      }
      while ( n5a_3 < n5a_2 );
      v35 = v12;
      __sub_4135A0_n0x2000 = __sub_4135A0_n0x2000_1;
    }
  }
  else
  {
    n0x2000_2 = 0;
  }
  n0x2000_3 = n0x2000_2 + *v35;
  rc.encode(n0x2000_2, n0x2000_3, __sub_4135A0_n0x2000);
  if ( __sub_4135A0_n0x2000 > 0x2000 )
  {
    *a1 = 0x8000;
    v27 = a1 + 7;
    n256 = 256;
    v29 = a1 + 7 < v37;
    v30 = a1 + 7;
    if ( a1 + 7 >= v37 )
    {
      do
      {
        if ( *v30 < n256 )
          n256 = *v30;
        --v30;
      }
      while ( v30 >= v37 );
      v29 = v27 < v37;
    }
    if ( !v29 )
    {
      do
      {
        if ( (uint16_t)n256 <= 1u )
          v31 = *v27 - (*v27 >> 1);
        else
          v31 = (*v27 + 2) / 3;
        *v27 = v31;
        *a1 += v31;
        --v27;
      }
      while ( v27 >= v37 );
    }
  }
  *v35 += 32;
  result = *a1 + 32;
  *a1 = result;
  if ( n5a_2 >= 5 )
    return __fwd_sub_4135A0_sub_413900(
             (uint16_t *)__n256_1
           + 32512 * (n5a_2 & 1)
           + 254 * (((a1[1] + (result & 0x7FFF) + 96 - 2 * (uint32_t)a1[n5a_2 + 1]) >> 25) & 0xFFFFFFC0)
           + 254 * a3,
             (n5a - 5) >> 1);
  return result;
}

int32_t __sub_414060(uint16_t *_this)
{
  ;
  int16_t v31, v33;
  int32_t v5, v6, n2_1, v11, v12, n0x800000_2, v18, v21, v23, v25, v26, v36, v38, v40, n2, v43,
          v44;
  uint16_t *v8, v16, *v17, n0x4000, v28, v30, v32, v34, *v37;
  uint32_t __sub_414060_n0x800000, __sub_414060_n0x7F800000, v3, v7, v9, n0x7F800000_1,
           n0x800000_1, v19, n0x7F800000_2, v29, v35, v39;
  uint8_t *v4, *v20;
  v39 = *_this;
  n2 = 0;
  v7 = rc.get_freq(v39);
  v8 = _this + 2;
  v9 = *(_this + 2);
  if ( v9 <= v7 )
  {
    n2_1 = 0;
    do
    {
      ++v8;
      ++n2_1;
      v9 += *v8;
    }
    while ( v9 <= v7 );
    n2 = n2_1;
  }
  v37 = v8;
  v11 = v9 - *v8;
  rc.decode(v11, v9, v39);
  if ( *_this > 0x4000u )
  {
    v28 = *(_this + 2) - (*(_this + 2) >> 1);
    v29 = *(_this + 3);
    *(_this + 2) = v28;
    LOWORD(v29) = v29 - (v29 >> 1);
    *(_this + 3) = v29;
    LOWORD(v29) = v28 + v29;
    v30 = *(_this + 4) - (*(_this + 4) >> 1);
    *(_this + 4) = v30;
    v31 = v29 + v30;
    LOWORD(v29) = *(_this + 5) - (*(_this + 5) >> 1);
    *(_this + 5) = v29;
    LOWORD(v29) = v31 + v29;
    v32 = *(_this + 6) - (*(_this + 6) >> 1);
    *(_this + 6) = v32;
    v33 = v29 + v32;
    LOWORD(v29) = *(_this + 7) - (*(_this + 7) >> 1);
    *(_this + 7) = v29;
    LOWORD(v29) = v33 + v29;
    v34 = *(_this + 8) - (*(_this + 8) >> 1);
    v35 = *(_this + 9);
    *(_this + 8) = v34;
    LOWORD(v35) = v35 - (v35 >> 1);
    *(_this + 9) = v35;
    LOWORD(v35) = v29 + v34 + v35;
    v36 = *(_this + 1);
    *_this = v35;
    if ( v36 <= 4 * __n8_1 )
      v16 = v36 - 4 * (v36 > __n8_1);
    else
      v16 = v36 - 16;
    *(_this + 1) = v16;
  }
  else
  {
    v16 = *(_this + 1);
  }
  *v37 += v16;
  *_this += *(_this + 1);
  if ( n2 < 2 )
    return n2;
  else
  {
    v38 = (uint8_t)__byte_445715[4 * n2];
    v40 = 0;
    v43 = 1;
    do
    {
      v17 = _this + 2 * (uint8_t)__byte_445716[4 * n2] + 2 * v43 + 2 * v40 + 8;
      v18 = *v17;
      v44 = v17[1];
      v23 = rc.decode_bit(v18, v44);
      n0x4000 = v17[v23];
      if ( n0x4000 > 0x4000u )
      {
        *v17 -= *v17 >> 1;
        v17[1] -= v17[1] >> 1;
        n0x4000 = v17[v23];
      }
      v17[v23] = __n8_0 + n0x4000;
      v38 >>= 1;
      v43 *= 2;
      v25 = v23 + 2 * v40;
      v40 = v25;
    }
    while ( v38 );
    v26 = (uint8_t)__byte_445714[4 * n2];
    return v25 + v26;
  }
}
static inline int32_t __fwd_sub_413E60_sub_414060(void *a0) { return __sub_414060((uint16_t *)a0); }

 int32_t __sub_413E60(uint16_t *a1, int32_t a2, int32_t a3)
{
  ;
  bool v21;
  int16_t v16, v23;
  int32_t n0x7F800000_1, v8, v9, v13, n0x800000_2, n0x800000_1, n5, n256;
  uint16_t *v11, *v19, *v22;
  uint32_t __sub_413E60_n0x800000, __sub_413E60_n0x2000, v5, __sub_413E60_n0x2000_1, n0x2000_2,
           v24, __sub_413E60_n0x7F800000;
  uint8_t *v6;
  __sub_413E60_n0x2000 = *a1 & 0x7FFF;
  __sub_413E60_n0x2000_1 = rc.get_freq(__sub_413E60_n0x2000);
  v11 = a1 + 1;
  n0x2000_2 = (uint16_t)a1[1];
  v24 = (uint32_t)(a1 + 1);
  while ( n0x2000_2 <= __sub_413E60_n0x2000_1 )
    n0x2000_2 += (uint16_t)*++v11;
  rc.decode(n0x2000_2 - (uint16_t)*v11, n0x2000_2, __sub_413E60_n0x2000);
  if ( __sub_413E60_n0x2000 > 0x2000 )
  {
    *a1 = 0x8000;
    v19 = a1 + 7;
    n256 = 256;
    v21 = (uint32_t)(a1 + 7) < v24;
    v22 = a1 + 7;
    if ( (uint32_t)(a1 + 7) >= v24 )
    {
      do
      {
        if ( *v22 < n256 )
          n256 = *v22;
        --v22;
      }
      while ( (uint32_t)v22 >= v24 );
      v21 = (uint32_t)v19 < v24;
    }
    if ( !v21 )
    {
      do
      {
        if ( (uint16_t)n256 <= 1u )
          v23 = *v19 - (*v19 >> 1);
        else
          v23 = ((uint16_t)*v19 + 2) / 3;
        *v19 = v23;
        *a1 += v23;
        --v19;
      }
      while ( (uint32_t)v19 >= v24 );
    }
  }
  *v11 += 32;
  v16 = *a1 + 32;
  *a1 = v16;
  n5 = (int32_t)((int32_t)v11 - v24) >> 1;
  if ( n5 >= 5 )
    n5 += 2
        * __fwd_sub_413E60_sub_414060(
            (uint16_t *)__n256_1
          + 32512 * (n5 & 1)
          + 254
          * ((((uint16_t)a1[1] + (v16 & 0x7FFF) + 96 - 2 * (uint32_t)(uint16_t)a1[n5 + 1]) >> 25)
           & 0xFFFFFFC0)
          + 254 * a3);
  return n5;
}
static inline int32_t __fwd_sub_414390_sub_413900(void *a0, int32_t a1) { return __sub_413900((uint16_t *)a0, a1); }

int32_t __sub_414390(uint16_t *_this, int32_t a2, int32_t a3)
{
  ;
  char v29;
  int16_t v18;
  int32_t v3, v8, n0x7F800000_1, v11, v12, v15, n0x800000_3, n0x800000_2, n32, result,
          __sub_414390_n0x7F800000, v30;
  uint16_t *v5, *v25;
  uint32_t __sub_414390_n0x2000, __sub_414390_n0x800000, __sub_414390_n0x2000_1, v10, v13, v21,
           v22, v23, n0x2000_2, n0x800000_1;
  uint8_t *v14;
  v3 = *(_this + 2) + *(_this + 1);
  __sub_414390_n0x2000 = v3 + *(_this + 3);
  if ( a3 )
  {
    if ( (a3 & 1) != 0 )
    {
      v3 = *(_this + 1);
      v5 = _this + 2;
      v25 = _this + 2;
    }
    else
    {
      v5 = _this + 3;
      v25 = _this + 3;
    }
  }
  else
  {
    v5 = _this + 1;
    v3 = 0;
    v25 = _this + 1;
  }
  __sub_414390_n0x2000_1 = v3 + *v5;
  rc.encode(v3, __sub_414390_n0x2000_1, __sub_414390_n0x2000);
  v18 = *v25;
  if ( *v25 > 0x4000u )
  {
    v21 = *(_this + 2);
    v22 = *(_this + 3);
    *(_this + 1) -= *(_this + 1) >> 1;
    n32 = *_this;
    *(_this + 2) = v21 - (v21 >> 1);
    *(_this + 3) = v22 - (v22 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        v23 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(v23) = 32;
      LOWORD(n32) = n32 - v23;
      *_this = n32;
      v18 = *v25;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      *_this = n32;
      v18 = *v25;
    }
  }
  else
  {
    LOWORD(n32) = *_this;
  }
  result = (int32_t)v25;
  *v25 = n32 + v18;
  if ( a3 > 0 )
    return __fwd_sub_414390_sub_413900((uint16_t *)__n256_1 + 254 * *(uint32_t *)(a2 + 4 * (a3 & 1)), (a3 - 1) >> 1);
  return result;
}
static inline int32_t __fwd_sub_414620_sub_414060(void *a0) { return __sub_414060((uint16_t *)a0); }

int32_t __sub_414620(uint16_t *_this, int32_t a2)
{
  ;
  int32_t v6, v7, v8, v10, v12, n0x800000_1, n0x4000, n32, v16, v21, v23, v25;
  uint16_t *v9, *v24;
  uint32_t __sub_414620_n0x800000, __sub_414620_n0x7F800000, v4, v11, v18, v19, v20;
  uint8_t *v5;
  v23 = *(_this + 3);
  v21 = *(_this + 2) + *(_this + 1);
  v20 = v23 + v21;
  v7 = rc.get_freq(v20);
  v8 = *(_this + 1);
  if ( v7 >= v8 )
  {
    if ( v7 >= v21 )
    {
      v8 = v21;
      v9 = _this + 3;
    }
    else
    {
      v9 = _this + 2;
    }
    v24 = _this + 1;
  }
  else
  {
    v9 = _this + 1;
    v8 = 0;
    v24 = _this + 1;
  }
  v10 = (uint16_t)*v9;
  v11 = v8 + v10;
  rc.decode(v8, v11, v20);
  n0x4000 = (uint16_t)*v9;
  if ( n0x4000 > 0x4000 )
  {
    v18 = *(_this + 2);
    v19 = *(_this + 3);
    *(_this + 1) -= *(_this + 1) >> 1;
    n32 = *_this;
    *(_this + 2) = v18 - (v18 >> 1);
    *(_this + 3) = v19 - (v19 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        n0x4000 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(n0x4000) = 32;
      LOWORD(n32) = n32 - n0x4000;
      *_this = n32;
      LOWORD(n0x4000) = *v9;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      *_this = n32;
      LOWORD(n0x4000) = *v9;
    }
  }
  else
  {
    LOWORD(n32) = *_this;
  }
  *v9 = n32 + n0x4000;
  v16 = v9 - v24;
  if ( v16 )
    return v16 + 2 * __fwd_sub_414620_sub_414060((uint16_t *)__n256_1 + 254 * *(uint32_t *)(a2 + 4 * (v16 & 1)));
  else
    return 0;
}

uint32_t __sub_414800(uint16_t *_this)
{
  ;
  uint32_t n32, n0x100;
  *(_this + 1) -= *(_this + 1) >> 1;
  *(_this + 2) -= *(_this + 2) >> 1;
  n32 = *(_this + 3) >> 1;
  *(_this + 3) -= n32;
  n0x100 = *_this;
  if ( n0x100 <= 0x100 )
  {
    if ( *_this <= 0x20u )
      n32 = ((16 - n0x100) >> 30) & 0xFFFFFFFE;
    else
      n32 = 32;
    *_this = n0x100 - n32;
  }
  else
  {
    *_this >>= 1;
  }
  return n32;
}

int32_t __sub_4148F0(uint16_t *_this)
{
  ;
  int32_t n0x4000;
  *_this -= *_this >> 1;
  n0x4000 = (uint16_t)*(_this + 2);
  *(_this + 1) -= *(_this + 1) >> 1;
  if ( n0x4000 < 0x4000 )
  {
    n0x4000 += 64;
    *(_this + 2) = n0x4000;
  }
  return n0x4000;
}
void __sub_414920()
{
  rc.finish();
  if ( __dword_443364 )
    free(__n256_1);
}
void __sub_414CE0()
{
  rc.flush();
  if ( __dword_443364 )
    free(__n256_1);
}

void **__sub_415270(void **Blocka, char a2)
{
  ;
  int32_t v4, v6, v10, v13, i;
  uint32_t *v3, *v7, *v9, *v12;
  void **Blocka_1, **Blocka_2, **v8, **Blocka_3, **v14;
  Blocka_1 = Blocka;
  free(*(Blocka + 269560));
  free(Blocka_1[269559]);
  free(Blocka_1[269671]);
  free(Blocka_1[269672]);
  v3 = Blocka_1[269552];
  if ( v3 )
  {
    v4 = *(v3 - 1);
    if ( v4 )
    {
      Blocka_2 = Blocka_1;
      v6 = *(v3 - 1);
      v7 = Blocka_1[269552];
      v8 = (void **)&v3[6 * v4];
      do
      {
        v8 -= 6;
        free(v8[5]);
        --v6;
      }
      while ( v6 );
      v3 = v7;
      Blocka_1 = Blocka_2;
    }
    free(v3 - 1);
  }
  v9 = Blocka_1[269553];
  if ( v9 )
  {
    v10 = *(v9 - 1);
    if ( v10 )
    {
      Blocka_3 = Blocka_1;
      v12 = Blocka_1[269553];
      v13 = *(v9 - 1);
      v14 = (void **)&v9[6 * v10];
      do
      {
        v14 -= 6;
        free(v14[5]);
        --v13;
      }
      while ( v13 );
      v9 = v12;
      Blocka_1 = Blocka_3;
    }
    free(v9 - 1);
  }
  for ( i = 0; i < 5; ++i )
    free(Blocka_1[i + 14]);
  free(Blocka_1[269551]);
  if ( (a2 & 1) != 0 )
    free(Blocka_1);
  return Blocka_1;
}

int32_t __sub_416860(int32_t *_this, uint32_t *p_n15)
{
  ;
  bool v5;
  int32_t result, v3, v4, v6, v7, v9, v13, v14, v16, n6;
  uint16_t *v8, *v10, *v11, *v12;
  n6 = *(_this + 11);
  result = p_n15[n6];
  if ( __buf_0[result] == __byte_445700 )
    return -1;
  v3 = 32
     * (result == p_n15[15] || result == p_n15[14] || result == p_n15[13] || result == p_n15[12] || result == p_n15[11])
     + ((result == p_n15[10]) << 6);
  v4 = 16
     * (result == p_n15[31]
     || result == p_n15[30]
     || result == p_n15[29]
     || result == p_n15[28]
     || result == p_n15[27]
     || result == p_n15[26]
     || result == p_n15[25]
     || result == p_n15[24]
     || result == p_n15[23]
     || result == p_n15[22]
     || result == p_n15[21]
     || result == p_n15[20]
     || result == p_n15[19]
     || result == p_n15[18]
     || result == p_n15[17]
     || result == p_n15[16]);
  v5 = v4 + v3 == 0;
  v6 = v4 + v3;
  *(_this + 13) = v6;
  if ( v5 && n6 > 6 )
    return -1;
  v7 = *(_this + 269553);
  v16 = v7;
  v8 = *(uint16_t **)(v7 + 24 * __n4_4 + 20);
  v9 = v6
     + 8
     * (result == *(uint16_t *)((char *)v8 + 27)
     || result == v8[12]
     || result == *(uint16_t *)((char *)v8 + 21)
     || result == v8[9]
     || result == *(uint16_t *)((char *)v8 + 15)
     || result == v8[6]
     || result == *(uint16_t *)((char *)v8 + 9)
     || result == v8[3]
     || result == *(uint16_t *)((char *)v8 + 3)
     || result == *v8);
  *(_this + 13) = v9;
  v10 = *(uint16_t **)(v7 + 24 * __n4_3 + 20);
  v11 = *(uint16_t **)(v7 + 24 * result + 20);
  v12 = *(uint16_t **)(v16 + 24 * __n15 + 20);
  v13 = (result == *(uint16_t *)((char *)v12 + 15)
      || result == v12[6]
      || result == *(uint16_t *)((char *)v12 + 9)
      || result == v12[3]
      || result == *(uint16_t *)((char *)v12 + 3)
      || result == *v12)
      + 2
      * (__n4_3 == *(uint16_t *)((char *)v11 + 27)
      || __n4_3 == v11[12]
      || __n4_3 == *(uint16_t *)((char *)v11 + 21)
      || __n4_3 == v11[9]
      || __n4_3 == *(uint16_t *)((char *)v11 + 15)
      || __n4_3 == v11[6]
      || __n4_3 == *(uint16_t *)((char *)v11 + 9)
      || __n4_3 == v11[3]
      || __n4_3 == *(uint16_t *)((char *)v11 + 3)
      || __n4_3 == *v11)
      + 4
      * (result == *(uint16_t *)((char *)v10 + 9)
      || result == v10[3]
      || result == *(uint16_t *)((char *)v10 + 3)
      || result == *v10);
  v14 = v13 + v9;
  if ( n6 <= 14 || (v14 & 0xB) != 0 )
  {
    *(_this + 13) = (n6 << 7) + v14;
    *(_this + 12) = v13 + 8 * (n6 > 9);
  }
  else
  {
    *(_this + 12) = v13;
    return -1;
  }
  return result;
}
static inline int32_t __fwd_sub_416C90_sub_413230(void *a0, int32_t a1, uint32_t a2) { return __sub_413230((uint32_t *)a0, a1, a2); }

int32_t __sub_416C90(int32_t _this)
{
  ;
  char v13, v14, *buf;
  int16_t v6, *v11, v12;
  int32_t n2_1, v3, v7, v8, v9, v10, v15, v17, n2, v19, v21, v22, v23, v24, v25, v26, v27, v28,
          v29, v30, result, v32;
  uint16_t *v20;
  uint32_t **v4, *v5;
  n2_1 = *(uint32_t *)(_this + 32);
  if ( !n2_1 )
  {
    if ( _this + 1078216 == *(uint32_t *)(_this + 1078232) )
    {
      if ( *(uint32_t *)(_this + 1078216) )
      {
        __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * __n4_4), **(uint16_t **)(_this + 76), 3u);
        __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * **(uint16_t **)(_this + 76)), __n4_3, 2u);
        __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078208) + 24 * __n4_4), **(uint16_t **)(_this + 76), 4u);
        __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078208) + 24 * **(uint16_t **)(_this + 76)), __n4_4, 2u);
      }
      else
      {
        __fwd_sub_416C90_sub_413230(
          (uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * __n4_3),
          **(uint16_t **)(_this + 76),
          (*(uint32_t *)(_this + 44) > 3) + 2);
      }
    }
    else
    {
      __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * __n4_4), **(uint16_t **)(_this + 76), 3u);
      __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * **(uint16_t **)(_this + 76)), __n4_3, 2u);
      __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * **(uint16_t **)(_this + 76)), __n4_4, 1u);
      __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078208) + 24 * **(uint16_t **)(_this + 76)), __n4_4, 2u);
      v3 = *(uint32_t *)(_this + 1078232);
      do
      {
        v4 = (uint32_t **)(v3 - 4);
        *(uint32_t *)(_this + 1078232) = v4;
        v5 = *v4;
        v6 = **(uint16_t **)(_this + 76);
        v7 = v5[1];
        v32 = v5[5];
        if ( v7 == *v5 )
        {
          v8 = v5[5];
          v5[1] = --v7;
          v9 = *(uint8_t *)(v8 + 3 * v7 + 2);
        }
        else
        {
          v9 = 1;
        }
        v10 = v5[2];
        v11 = (int16_t *)(v32 + 3 * v7);
        v5[1] = v7 + 1;
        v5[2] = v9 + v10 + 1;
        *v11 = v6;
        *((uint8_t *)v11 + 2) = 2;
        v5[3] += 4;
        if ( v11 != (int16_t *)v5[5] )
        {
          v12 = *v11;
          v13 = *((uint8_t *)v11 + 2);
          v14 = *((uint8_t *)v11 - 1);
          *v11 = *(int16_t *)((char *)v11 - 3);
          *((uint8_t *)v11 + 2) = v14;
          *(int16_t *)((char *)v11 - 3) = v12;
          *((uint8_t *)v11 - 1) = v13;
        }
        v3 = *(uint32_t *)(_this + 1078232);
      }
      while ( v3 != _this + 1078216 );
    }
    if ( __byte_445700 == -1 )
    {
      v15 = *(uint32_t *)(_this + 16);
      __byte_445700 = 1;
      buf = __buf_0;
      v17 = (v15 + 15) >> 4;
      do
      {
        *((uint32_t *)buf + 3) = 0;
        *((uint32_t *)buf + 2) = 0;
        *((uint32_t *)buf + 1) = 0;
        *(uint32_t *)buf = 0;
        buf += 16;
        --v17;
      }
      while ( v17 );
      n2 = *(uint32_t *)(_this + 32);
    }
    else
    {
      ++__byte_445700;
      n2 = *(uint32_t *)(_this + 32);
    }
LABEL_19:
    if ( n2 && n2 <= 2 )
      goto LABEL_37;
    goto LABEL_21;
  }
  if ( n2_1 <= 2 )
    goto LABEL_37;
  if ( __n15 != __n15_0 )
  {
    __fwd_sub_416C90_sub_413230((uint32_t *)(*(uint32_t *)(_this + 1078212) + 24 * __n4_3), **(uint16_t **)(_this + 76), 1u);
    n2 = *(uint32_t *)(_this + 32);
    goto LABEL_19;
  }
LABEL_21:
  v19 = **(uint16_t **)(_this + 76);
  v20 = *(uint16_t **)(_this + 6059432);
  v21 = *v20;
  if ( v19 != v21 )
  {
    v22 = v20[1];
    if ( v19 == v22 )
    {
      v20[1] = v21;
    }
    else
    {
      v23 = v20[2];
      if ( v19 == v23 )
      {
        v20[2] = v22;
        *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2) = **(uint16_t **)(_this + 6059432);
      }
      else
      {
        v24 = v20[3];
        if ( v19 == v24 )
        {
          v20[3] = v23;
          *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2);
          *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2) = **(uint16_t **)(_this + 6059432);
        }
        else
        {
          v25 = v20[4];
          if ( v19 == v25 )
          {
            v20[4] = v24;
            *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 6) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4);
            *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2);
            *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2) = **(uint16_t **)(_this + 6059432);
          }
          else
          {
            v26 = v20[5];
            if ( v19 == v26 )
            {
              v20[5] = v25;
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 8) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 6);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 6) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2) = **(uint16_t **)(_this + 6059432);
            }
            else
            {
              v27 = v20[6];
              if ( v19 == v27 )
              {
                v20[6] = v26;
              }
              else
              {
                v20[7] = v27;
                *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 12) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 10);
              }
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 10) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 8);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 8) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 6);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 6) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 4) = *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2);
              *(uint16_t *)(*(uint32_t *)(_this + 6059432) + 2) = **(uint16_t **)(_this + 6059432);
            }
          }
        }
      }
    }
    **(uint16_t **)(_this + 6059432) = v19;
  }
LABEL_37:
  *(uint16_t *)(*(uint32_t *)(_this + 6059436) + 2) = **(uint16_t **)(_this + 6059436);
  **(uint16_t **)(_this + 6059436) = **(uint16_t **)(_this + 76);
  *(uint8_t *)(*(uint32_t *)(_this + 76) + 2) = **(uint16_t **)(_this + 76) == **(uint16_t **)(_this + 80);
  *(uint8_t *)(*(uint32_t *)(_this + 76) + 3) = **(uint16_t **)(_this + 76) == *(uint16_t *)(*(uint32_t *)(_this + 76) - 8);
  *(uint8_t *)(*(uint32_t *)(_this + 76) + 4) = **(uint16_t **)(_this + 76) == *(uint16_t *)(*(uint32_t *)(_this + 80) + 8);
  *(uint8_t *)(*(uint32_t *)(_this + 76) + 5) = **(uint16_t **)(_this + 76) == *(uint16_t *)(*(uint32_t *)(_this + 80) - 8);
  *(uint8_t *)(*(uint32_t *)(_this + 76) + 6) = **(uint16_t **)(_this + 76) == *(uint16_t *)(*(uint32_t *)(_this + 80) + 16);
  *(uint8_t *)(*(uint32_t *)(_this + 76) + 7) = **(uint16_t **)(_this + 76) == *(uint16_t *)(*(uint32_t *)(_this + 80) + 24);
  v28 = *(uint32_t *)(_this + 80);
  v29 = *(uint32_t *)(_this + 84);
  v30 = *(uint32_t *)(_this + 76) + 8;
  *(uint32_t *)(_this + 76) = v30;
  *(uint32_t *)(_this + 88) += 8;
  v28 += 8;
  *(uint32_t *)(_this + 80) = v28;
  v29 += 8;
  *(uint32_t *)(_this + 92) += 8;
  *(uint32_t *)(_this + 84) = v29;
  *(uint8_t *)(_this + 1078692) += *(uint8_t *)(v28 + 34) - *(uint8_t *)(v28 - 30);
  *(uint8_t *)(_this + 1078693) += *(uint8_t *)(v29 + 34) - *(uint8_t *)(v29 - 30);
  *(uint8_t *)(_this + 1078694) += *(uint8_t *)(v30 - 5) - *(uint8_t *)(v30 - 37);
  result = *(uint8_t *)(v30 - 6) - *(uint8_t *)(v30 - 62);
  *(uint8_t *)(_this + 1078695) += result;
  return result;
}

void **__sub_419610(void **lpAddress, char a2)
{
  ;
  free(*(lpAddress + 69689));
  free(*(lpAddress + 69690));
  free(*(lpAddress + 69691));
  free(*(lpAddress + 69692));
  free(*(lpAddress + 69693));
  free(*(lpAddress + 69665));
  free(*(lpAddress + 69666));
  if ( (a2 & 1) != 0 )
    bmf_page_free(lpAddress);
  return lpAddress;
}

 void __sub_419680()
{
  ;
  __xmmword_445760[0] = __xmmword_439A00;
  __xmmword_445770[0] = __xmmword_439A10;
  __xmmword_445780[0] = __xmmword_439A20;
  __xmmword_445790[0] = __xmmword_439A30;
  __xmmword_4457A0[0] = __xmmword_439A40;
  __xmmword_4457B0[0] = __xmmword_439A50;
  __xmmword_4457C0 = __xmmword_439A60;
  __xmmword_4457D0 = __xmmword_439A30;
  __xmmword_4457E0 = __xmmword_439A70;
  __xmmword_4457F0 = __xmmword_439A80;
  __xmmword_445800 = __xmmword_439A20;
  __xmmword_445810 = __xmmword_439A80;
  __xmmword_445820 = __xmmword_439A90;
  __xmmword_445830 = __xmmword_439AA0;
  __xmmword_445840 = __xmmword_439A60;
  __xmmword_445850 = __xmmword_439AB0;
  __xmmword_445860 = __xmmword_439A80;
  __xmmword_445870 = __xmmword_439AC0;
  __xmmword_445880 = __xmmword_439AD0;
  __xmmword_445890 = __xmmword_439AE0;
  __xmmword_4458A0 = __xmmword_439AB0;
  __xmmword_4458B0 = __xmmword_439AA0;
  __xmmword_4458C0 = __xmmword_439AF0;
  __xmmword_4458D0 = __xmmword_439B00;
}

void **__sub_4244A0(void **Block, char a2)
{
  ;
  free(*(Block + 44));
  free(*(Block + 45));
  free(*(Block + 46));
  free(*(Block + 47));
  free(*(Block + 48));
  if ( (a2 & 1) != 0 )
    free(Block);
  return Block;
}

int32_t __sub_4248D0(uint8_t **_this, uint32_t *a2, int32_t a3)
{
  ;
  bool v9, v19, v20;
  int32_t v5, v6, v7, v11, v12, v13, v14, v15, v16, v18, v21, v22, v23, v24, v25, v26, v27, v28,
          v29, result, v32, v34, v35, v36, v39, v40, v41, v42;
  uint32_t v37, v38;
  uint8_t *v3, *v4, *v8, *v10, *v17, *v31, *v33;
  v3 = *(_this + 50);
  v4 = (uint8_t *)*v3;
  v5 = (int32_t)*(_this + 49);
  v6 = *(uint8_t *)(v5 - 2);
  v7 = *(v3 - 2);
  if ( v6 < (int32_t)v4 )
  {
    if ( v7 >= v6 )
    {
      v10 = &v4[v6 - v7];
      v9 = v7 < (int32_t)v4;
      v4 = (uint8_t *)*(uint8_t *)(v5 - 2);
      if ( v9 )
        v4 = v10;
    }
  }
  else if ( v7 <= v6 )
  {
    v8 = &v4[v6 - v7];
    v9 = v7 <= (int32_t)v4;
    v4 = (uint8_t *)*(uint8_t *)(v5 - 2);
    if ( !v9 )
      v4 = v8;
  }
  *(_this + 2) = v4;
  v33 = *(_this + 51);
  v11 = *(v3 - 1)
      + *(uint8_t *)(v5 - 5)
      + 3 * (v3[3] + v33[1])
      + 6 * *(uint8_t *)(v5 - 1)
      + 4 * (v3[1] + *(uint8_t *)(v5 - 3))
      + 2 * (v33[5] + v3[5] + *(uint8_t *)(v5 - 7));
  if ( a2 )
  {
    if ( a3 )
    {
      v34 = v5;
      v12 = a2[49];
      v36 = v11 + 2 * (*(uint8_t *)(*(uint32_t *)(a3 + 196) - 1) + *(uint8_t *)(v12 - 1));
      *(_this + 28) = (uint8_t *)(*v3
                                       - (uint32_t)v4
                                       + *(uint8_t *)(v12 - 2)
                                       - *(uint8_t *)(a2[50] - 2));
      v13 = a2[49];
      v14 = *(uint8_t *)(v13 - 2);
      v15 = *(uint8_t *)(v13 - 4);
      v5 = v34;
      *(_this + 32) = (uint8_t *)(*(uint8_t *)(v34 - 2) - (uint32_t)v4 + v14 - v15);
      *(_this + 36) = (uint8_t *)(*(uint8_t *)(v34 - 2)
                                       - (uint32_t)v4
                                       + *(uint8_t *)(*(uint32_t *)(a3 + 196) - 2)
                                       - *(uint8_t *)(*(uint32_t *)(a3 + 196) - 4));
      *(_this + 40) = (uint8_t *)(*(uint8_t *)(*(uint32_t *)(a3 + 196) - 2) - *(uint32_t *)(a3 + 8));
      *(_this + 20) = (uint8_t *)(*(uint8_t *)(a2[49] - 2) - a2[2]);
      v37 = (*(uint8_t *)(*(uint32_t *)(a3 + 196) - 1) + (uint32_t)*(uint8_t *)(a2[49] - 1) - 16) >> 31;
    }
    else
    {
      v36 = v3[7] + v11 + 3 * *(uint8_t *)(a2[49] - 1);
      *(_this + 28) = (uint8_t *)(2 * *(uint8_t *)(v5 - 2) - *(uint8_t *)(v5 - 4) - (uint32_t)v4);
      *(_this + 32) = (uint8_t *)(2 * *(uint8_t *)(v5 - 2) - *(uint8_t *)(v5 - 4) - (uint32_t)v4);
      *(_this + 36) = (uint8_t *)(-(int32_t)v4 - *v3 + v3[2] + *(uint8_t *)(v5 - 2));
      *(_this + 40) = (uint8_t *)(*(uint8_t *)(v5 - 2)
                                       - (uint32_t)v4
                                       + *(uint8_t *)(a2[49] - 2)
                                       - *(uint8_t *)(a2[49] - 4));
      *(_this + 20) = (uint8_t *)(*(uint8_t *)(a2[49] - 2) - a2[2]);
      v37 = ((uint32_t)*(uint8_t *)(a2[49] - 1) - 8) >> 31;
    }
  }
  else
  {
    v32 = (int32_t)*(_this + 53);
    v36 = *(uint8_t *)(v32 + 1) + *(v33 - 3) + v3[7] + v11 + *(uint8_t *)(v32 + 5);
    *(_this + 28) = (uint8_t *)(2 * *(uint8_t *)(v5 - 2) - *(uint8_t *)(v5 - 4) - (uint32_t)v4);
    *(_this + 32) = (uint8_t *)(2 * *v3 - *v33 - (uint32_t)v4);
    *(_this + 36) = (uint8_t *)(-(int32_t)v4 - *v3 + v3[2] + *(uint8_t *)(v5 - 2));
    *(_this + 40) = (uint8_t *)(-3 * (*(uint8_t *)(v5 - 4) - *(uint8_t *)(v5 - 2))
                                     + *(uint8_t *)(v5 - 6)
                                     - (uint32_t)v4);
    *(_this + 20) = (uint8_t *)(v3[4] - (uint32_t)v4);
    v37 = *(uint8_t *)(v5 + 1) + *(uint8_t *)(v32 + 1) + (*(_this + 52))[1] + v33[1] + v3[1] == 0;
  }
  v16 = (v36 + 7) >> 4;
  v17 = (uint8_t *)*((uint8_t *)_this + v16 + 216);
  v31 = v17;
  v35 = __dword_439BD8[(uint32_t)v17];
  *(_this + 3) = v17;
  *(_this + 4) = &(*(_this + v16 + 438))[*((uint8_t *)_this + (uint32_t)v4 + 728)];
  *(_this + 8) = (uint8_t *)(((uint32_t)(216 - (uint32_t)v4) >> 31) + ((uint32_t)(22 - (uint32_t)v4) >> 31));
  v38 = ((uint32_t)(216 - (uint32_t)v4) >> 31) + ((uint32_t)(22 - (uint32_t)v4) >> 31);
  v18 = (*(v3 - 2) - *v3 >= 0) + (*(v3 - 2) > (int32_t)*v3);
  *(_this + 12) = (uint8_t *)v18;
  v39 = (*(v3 - 2) - *(uint8_t *)(v5 - 2) >= 0) + (*(v3 - 2) > (int32_t)*(uint8_t *)(v5 - 2));
  *(_this + 16) = (uint8_t *)v39;
  v19 = *(_this + 20) == nullptr;
  v20 = (int32_t)*(_this + 20) < 0;
  v40 = (v3[2] - (int32_t)v4 >= -v35) + (v3[2] - (int32_t)v4 > v35);
  *(_this + 24) = (uint8_t *)v40;
  v21 = v35 < (int32_t)*(_this + 28);
  v22 = -v35 <= (int32_t)*(_this + 28);
  v23 = !v20 + (!v20 && !v19);
  *(_this + 20) = (uint8_t *)v23;
  v24 = v22 + v21;
  v19 = *(_this + 32) == nullptr;
  v20 = (int32_t)*(_this + 32) < 0;
  *(_this + 28) = (uint8_t *)v24;
  v25 = !v20 && !v19;
  v26 = !v20;
  v19 = *(_this + 36) == nullptr;
  v20 = (int32_t)*(_this + 36) < 0;
  v41 = v26 + v25;
  *(_this + 32) = (uint8_t *)(v26 + v25);
  v27 = !v20 && !v19;
  v28 = !v20;
  v19 = *(_this + 40) == nullptr;
  v20 = (int32_t)*(_this + 40) < 0;
  v42 = v28 + v27;
  *(_this + 36) = (uint8_t *)(v28 + v27);
  v29 = !v20 + (!v20 && !v19);
  *(_this + 40) = (uint8_t *)v29;
  result = (int32_t)&(*(_this + v29 + 41))[16 * v37
                                    + 8 * (*(_this + (uint32_t)*(_this + 5) + 6) == nullptr)
                                    + (uint32_t)&(*(_this + v41 + 33))[(uint32_t)*(_this + v42 + 37)
                                                                        + (uint32_t)&(*(_this + v40 + 25))[(uint32_t)*(_this + v24 + 29)]
                                                                        + (uint32_t)&(*(_this + v39 + 17))[(uint32_t)*(_this + v23 + 21) + (uint32_t)&(*(_this + v38 + 9))[(uint32_t)*(_this + v18 + 13)]]]
                                    + (uint32_t)v31];
  *(_this + 3) = (uint8_t *)result;
  return result;
}

int32_t __sub_413430(uint16_t *_this, int32_t __n2)
{
  ;
  int32_t n0x8000, v3, v5, v6, v7, v11, v12, v13;
  uint16_t *v8;
  uint32_t v4, n0x2000, v10;
  n0x8000 = *_this;
  if ( (uint32_t)n0x8000 <= 0x8000 )
  {
    v3 = *((uint8_t *)__dword_445660 + __n2);
    v4 = (*(_this + 1) >> 2) & 0xFFFFFFE0;
    if ( ::__n2 == 2 )
      v4 = 15 * (*(_this + 1) >> 5);
    *(_this + v3 + 2) += v4 + 4;
    n0x8000 = *_this + v4 + 4;
    *_this = n0x8000;
    if ( __n2 >= 2 )
    {
      n0x8000 = (uint8_t)__byte_445715[4 * v3];
      v11 = __n2 - (uint8_t)__byte_445714[4 * v3];
      v5 = (int32_t)(_this + 2 * (uint8_t)__byte_445716[4 * v3] + 8);
      v6 = 0;
      v12 = v5;
      v7 = 1;
      do
      {
        v8 = (uint16_t *)(v12 + 4 * (v6 + v7));
        n0x2000 = (uint16_t)v8[(n0x8000 & v11) != 0];
        v13 = (n0x8000 & v11) != 0;
        if ( n0x2000 > 0x2000 )
        {
          v10 = (uint16_t)v8[1];
          *v8 -= *v8 >> 1;
          v8[1] = v10 - (v10 >> 1);
          LOWORD(n0x2000) = v8[v13];
        }
        v7 *= 2;
        n0x8000 >>= 1;
        v6 = v13 + 2 * v6;
        v8[v13] = n0x2000 + ((__n8_0 * ((uint32_t)(::__n2 == 2) + 5)) >> 3);
      }
      while ( n0x8000 );
    }
  }
  return n0x8000;
}
static inline int32_t __fwd_sub_4259F0_sub_413430(void *a0, int32_t a1) { return __sub_413430((uint16_t *)a0, a1); }

int32_t __sub_4259F0(int32_t _this)
{
  ;
  int16_t v12, v15;
  int32_t v3, n5_1, v5, n5_2, n5_3, v8, v9, result, v11, v13, v14, v16, v17, v18, v21, v22, v23,
          v24, v25, v26, v29, v30, v31, v32, v33, v34, v37, v38, v39, v40, v41, v42, v45, v46,
          v47, v48, v49, v50, v53, v54, v55, v56, v57, v58, v61, v62, v63, v64, v65, v66, v69,
          v70, v71, v72, v73, v74, v77, v78, v79, v80, v82, v83, v85, v86, v89, v92, v95, v98,
          v101, v104, v107, v111, n2, n5_4;
  uint16_t *v19, *v20, *v27, *v28, *v35, *v36, *v43, *v44, *v51, *v52, *v59, *v60, *v67, *v68,
           *v75, *v76, *v81, *v84, *v87, *v88, *v90, *v91, *v93, *v94, *v96, *v97, *v99, *v100,
           *v102, *v103, *v105, *v106, *v108, *v109;
  uint32_t n5, v110, v112;
  n5 = *(uint8_t *)((uint8_t)(**(uint8_t **)(_this + 196) - *(uint8_t *)(_this + 8)) + _this + 984);
  v3 = *(uint32_t *)(_this + 48);
  n5_1 = *(uint8_t *)((uint8_t)(*(uint8_t *)(_this + 8) - **(uint8_t **)(_this + 196)) + _this + 984);
  v5 = *(uint32_t *)(_this + 64);
  n2 = (int32_t)(n5 - 5) >> 1;
  n5_2 = 6 - (n5 & 1);
  if ( n5 < 5 )
    n5_2 = *(uint8_t *)((uint8_t)(**(uint8_t **)(_this + 196) - *(uint8_t *)(_this + 8)) + _this + 984);
  n5_3 = 6 - (n5_1 & 1);
  if ( n5_1 < 5 )
    n5_3 = n5_1;
  n5_4 = n5_3;
  v8 = *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 160) + 172)
     + *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 144) + 156)
     + *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 128) + 140)
     + *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 112) + 124)
     + *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 96) + 108)
     + *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 80) + 92)
     + *(uint32_t *)(_this - 4 * v5 + 76)
     + *(uint32_t *)(_this - 4 * v3 + 60)
     + *(uint32_t *)(_this + 40)
     + (*(uint32_t *)(_this + 12) & 0x1F);
  v9 = _this + 16 * v8;
  v111 = v9;
  *(uint16_t *)(v9 + 2 * n5_3 + 3802) += 17;
  *(uint16_t *)(v9 + 3800) += 17;
  result = *(uint32_t *)(_this + 12);
  if ( (result & 7) != 7 )
  {
    v11 = _this + 16 * result;
    v110 = (((*(uint32_t *)(_this + 16) & 7u) - 7) >> 31) + *(uint32_t *)(_this + 16);
    *(uint16_t *)(v11 + 2 * n5_2 + 3818) += 11;
    v12 = *(uint16_t *)(v11 + 3816) + 11;
    *(uint16_t *)(v11 + 3816) = v12;
    if ( n5_2 >= 5 )
      __fwd_sub_4259F0_sub_413430(
        (uint16_t *)__n256_1
      + 32512 * (n5_2 & 1)
      + 254 * v110
      + 254
      * ((((v12 & 0x7FFF)
         + *(uint16_t *)(v11 + 3818)
         - 2 * (uint32_t)*(uint16_t *)(v11 + 2 * n5_2 + 3818)) >> 25)
       & 0xFFFFFFC0),
        (int32_t)(n5 - 5) >> 1);
    result = *(uint32_t *)(_this + 12);
  }
  if ( (result & 7) != 0 )
  {
    v13 = _this + 16 * result;
    v14 = *(uint32_t *)(_this + 16) - ((*(uint32_t *)(_this + 16) & 7) != 0);
    *(uint16_t *)(v13 + 2 * n5_2 + 3786) += 13;
    v15 = *(uint16_t *)(v13 + 3784) + 13;
    *(uint16_t *)(v13 + 3784) = v15;
    if ( n5_2 >= 5 )
      __fwd_sub_4259F0_sub_413430(
        (uint16_t *)__n256_1
      + 32512 * (n5_2 & 1)
      + 254 * v14
      + 254
      * ((((v15 & 0x7FFF)
         + *(uint16_t *)(v13 + 3786)
         - 2 * (uint32_t)*(uint16_t *)(v13 + 2 * n5_2 + 3786)) >> 25)
       & 0xFFFFFFC0),
        n2);
    result = *(uint32_t *)(_this + 12);
  }
  if ( *(uint16_t *)(16 * result + _this + 3800) < 0xCCCu )
  {
    if ( (result & 7u) < 7 )
    {
      *(uint16_t *)(v111 + 2 * n5_4 + 3818) += 7;
      *(uint16_t *)(v111 + 3816) += 7;
      result = *(uint32_t *)(_this + 12);
    }
    if ( (result & 7) != 0 )
    {
      *(uint16_t *)(v111 + 2 * n5_4 + 3786) += 5;
      *(uint16_t *)(v111 + 3784) += 5;
      result = *(uint32_t *)(_this + 12);
    }
    if ( n5_2 >= 5 )
    {
      v112 = *(uint32_t *)(_this + 16)
           + (((*(uint16_t *)(_this + 16 * result + 3802)
              + (*(uint16_t *)(_this + 16 * result + 3800) & 0x7FFF)
              - 2 * (uint32_t)*(uint16_t *)(_this + 16 * result + 2 * n5_2 + 3802)) >> 25)
            & 0xFFFFFFC0)
           + ((n5_2 & 1) << 7);
      if ( (v112 & 0x38) >= 0x38
        || (__fwd_sub_4259F0_sub_413430(
              (uint16_t *)__n256_1
            + 32512 * (n5_2 & 1)
            + 254 * *(uint32_t *)(_this + 16)
            + 254
            * (((*(uint16_t *)(_this + 16 * result + 3802)
               + (*(uint16_t *)(_this + 16 * result + 3800) & 0x7FFF)
               - 2 * (uint32_t)*(uint16_t *)(_this + 16 * result + 2 * n5_2 + 3802)) >> 25)
             & 0xFFFFFFC0)
            + 2032,
              n2),
            (v112 & 0x38) != 0) )
      {
        __fwd_sub_4259F0_sub_413430((uint16_t *)__n256_1 + 254 * v112 - 2032, n2);
      }
      result = *(uint32_t *)(_this + 12);
    }
    v16 = *(uint32_t *)(_this + 32);
    if ( v16 == 1 )
    {
      v107 = result - *(uint32_t *)(_this + 40);
      v108 = (uint16_t *)(_this + 16 * (v107 + *(uint32_t *)(_this + 36)));
      v109 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 44) + v107));
      v108[n5_2 + 1901] += 6;
      v108[1900] += 6;
      v109[n5_2 + 1901] += 6;
      v109[1900] += 6;
      v21 = *(uint32_t *)(_this + 12);
      if ( (v21 & 7) != 7 )
      {
        v108[n5_2 + 1909] += 4;
        v108[1908] += 4;
        v109[n5_2 + 1909] += 4;
        v109[1908] += 4;
        v21 = *(uint32_t *)(_this + 12);
      }
      if ( (v21 & 7) != 0 )
      {
        v108[n5_2 + 1893] += 3;
        v108[1892] += 3;
        v109[n5_2 + 1893] += 3;
        v109[1892] += 3;
        v21 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v17 = _this + 16 * (*(uint32_t *)(_this - 4 * v16 + 44) + result - *(uint32_t *)(_this + 4 * v16 + 36));
      *(uint16_t *)(v17 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v17 + 3800) += 7;
      v18 = v8 + *(uint32_t *)(_this + 36) - *(uint32_t *)(_this + 40);
      v19 = (uint16_t *)(_this
                    + 16
                    * (*(uint32_t *)(_this + 40)
                     + *(uint32_t *)(_this + 12)
                     - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 32) + 36)));
      v19[n5_2 + 1901] += 6;
      v19[1900] += 6;
      v20 = (uint16_t *)(_this + 16 * v18);
      v20[n5_4 + 1901] += 4;
      v20[1900] += 4;
      v21 = *(uint32_t *)(_this + 12);
      if ( (v21 & 7) != 7 )
      {
        v19[n5_2 + 1909] += 4;
        v19[1908] += 4;
        v20[n5_4 + 1909] += 2;
        v20[1908] += 2;
        v21 = *(uint32_t *)(_this + 12);
      }
      if ( (v21 & 7) != 0 )
      {
        v19[n5_2 + 1893] += 3;
        v19[1892] += 3;
        v20[n5_4 + 1893] += 2;
        v20[1892] += 2;
        v21 = *(uint32_t *)(_this + 12);
      }
    }
    v22 = *(uint32_t *)(_this + 48);
    if ( v22 == 1 )
    {
      v104 = v21 - *(uint32_t *)(_this + 56);
      v105 = (uint16_t *)(_this + 16 * (v104 + *(uint32_t *)(_this + 52)));
      v106 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 60) + v104));
      v105[n5_2 + 1901] += 6;
      v105[1900] += 6;
      v106[n5_2 + 1901] += 6;
      v106[1900] += 6;
      v29 = *(uint32_t *)(_this + 12);
      if ( (v29 & 7) != 7 )
      {
        v105[n5_2 + 1909] += 4;
        v105[1908] += 4;
        v106[n5_2 + 1909] += 4;
        v106[1908] += 4;
        v29 = *(uint32_t *)(_this + 12);
      }
      if ( (v29 & 7) != 0 )
      {
        v105[n5_2 + 1893] += 3;
        v105[1892] += 3;
        v106[n5_2 + 1893] += 3;
        v106[1892] += 3;
        v29 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v23 = _this + 16 * (*(uint32_t *)(_this - 4 * v22 + 60) + v21 - *(uint32_t *)(_this + 4 * v22 + 52));
      *(uint16_t *)(v23 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v23 + 3800) += 7;
      v24 = *(uint32_t *)(_this + 56);
      v25 = v24 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 48) + 52);
      v26 = v8 + v24 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 48) + 60);
      v27 = (uint16_t *)(_this + 16 * v25);
      v27[n5_2 + 1901] += 6;
      v27[1900] += 6;
      v28 = (uint16_t *)(_this + 16 * v26);
      v28[n5_4 + 1901] += 4;
      v28[1900] += 4;
      v29 = *(uint32_t *)(_this + 12);
      if ( (v29 & 7) != 7 )
      {
        v27[n5_2 + 1909] += 4;
        v27[1908] += 4;
        v28[n5_4 + 1909] += 2;
        v28[1908] += 2;
        v29 = *(uint32_t *)(_this + 12);
      }
      if ( (v29 & 7) != 0 )
      {
        v27[n5_2 + 1893] += 3;
        v27[1892] += 3;
        v28[n5_4 + 1893] += 2;
        v28[1892] += 2;
        v29 = *(uint32_t *)(_this + 12);
      }
    }
    v30 = *(uint32_t *)(_this + 64);
    if ( v30 == 1 )
    {
      v101 = v29 - *(uint32_t *)(_this + 72);
      v102 = (uint16_t *)(_this + 16 * (v101 + *(uint32_t *)(_this + 68)));
      v103 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 76) + v101));
      v102[n5_2 + 1901] += 6;
      v102[1900] += 6;
      v103[n5_2 + 1901] += 6;
      v103[1900] += 6;
      v37 = *(uint32_t *)(_this + 12);
      if ( (v37 & 7) != 7 )
      {
        v102[n5_2 + 1909] += 4;
        v102[1908] += 4;
        v103[n5_2 + 1909] += 4;
        v103[1908] += 4;
        v37 = *(uint32_t *)(_this + 12);
      }
      if ( (v37 & 7) != 0 )
      {
        v102[n5_2 + 1893] += 3;
        v102[1892] += 3;
        v103[n5_2 + 1893] += 3;
        v103[1892] += 3;
        v37 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v31 = _this + 16 * (*(uint32_t *)(_this - 4 * v30 + 76) + v29 - *(uint32_t *)(_this + 4 * v30 + 68));
      *(uint16_t *)(v31 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v31 + 3800) += 7;
      v32 = *(uint32_t *)(_this + 72);
      v33 = v32 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 64) + 68);
      v34 = v8 + v32 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 64) + 76);
      v35 = (uint16_t *)(_this + 16 * v33);
      v35[n5_2 + 1901] += 6;
      v35[1900] += 6;
      v36 = (uint16_t *)(_this + 16 * v34);
      v36[n5_4 + 1901] += 4;
      v36[1900] += 4;
      v37 = *(uint32_t *)(_this + 12);
      if ( (v37 & 7) != 7 )
      {
        v35[n5_2 + 1909] += 4;
        v35[1908] += 4;
        v36[n5_4 + 1909] += 2;
        v36[1908] += 2;
        v37 = *(uint32_t *)(_this + 12);
      }
      if ( (v37 & 7) != 0 )
      {
        v35[n5_2 + 1893] += 3;
        v35[1892] += 3;
        v36[n5_4 + 1893] += 2;
        v36[1892] += 2;
        v37 = *(uint32_t *)(_this + 12);
      }
    }
    v38 = *(uint32_t *)(_this + 80);
    if ( v38 == 1 )
    {
      v98 = v37 - *(uint32_t *)(_this + 88);
      v99 = (uint16_t *)(_this + 16 * (v98 + *(uint32_t *)(_this + 84)));
      v100 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 92) + v98));
      v99[n5_2 + 1901] += 6;
      v99[1900] += 6;
      v100[n5_2 + 1901] += 6;
      v100[1900] += 6;
      v45 = *(uint32_t *)(_this + 12);
      if ( (v45 & 7) != 7 )
      {
        v99[n5_2 + 1909] += 4;
        v99[1908] += 4;
        v100[n5_2 + 1909] += 4;
        v100[1908] += 4;
        v45 = *(uint32_t *)(_this + 12);
      }
      if ( (v45 & 7) != 0 )
      {
        v99[n5_2 + 1893] += 3;
        v99[1892] += 3;
        v100[n5_2 + 1893] += 3;
        v100[1892] += 3;
        v45 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v39 = _this + 16 * (*(uint32_t *)(_this - 4 * v38 + 92) + v37 - *(uint32_t *)(_this + 4 * v38 + 84));
      *(uint16_t *)(v39 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v39 + 3800) += 7;
      v40 = *(uint32_t *)(_this + 88);
      v41 = v40 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 80) + 84);
      v42 = v8 + v40 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 80) + 92);
      v43 = (uint16_t *)(_this + 16 * v41);
      v43[n5_2 + 1901] += 6;
      v43[1900] += 6;
      v44 = (uint16_t *)(_this + 16 * v42);
      v44[n5_4 + 1901] += 4;
      v44[1900] += 4;
      v45 = *(uint32_t *)(_this + 12);
      if ( (v45 & 7) != 7 )
      {
        v43[n5_2 + 1909] += 4;
        v43[1908] += 4;
        v44[n5_4 + 1909] += 2;
        v44[1908] += 2;
        v45 = *(uint32_t *)(_this + 12);
      }
      if ( (v45 & 7) != 0 )
      {
        v43[n5_2 + 1893] += 3;
        v43[1892] += 3;
        v44[n5_4 + 1893] += 2;
        v44[1892] += 2;
        v45 = *(uint32_t *)(_this + 12);
      }
    }
    v46 = *(uint32_t *)(_this + 96);
    if ( v46 == 1 )
    {
      v95 = v45 - *(uint32_t *)(_this + 104);
      v96 = (uint16_t *)(_this + 16 * (v95 + *(uint32_t *)(_this + 100)));
      v97 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 108) + v95));
      v96[n5_2 + 1901] += 6;
      v96[1900] += 6;
      v97[n5_2 + 1901] += 6;
      v97[1900] += 6;
      v53 = *(uint32_t *)(_this + 12);
      if ( (v53 & 7) != 7 )
      {
        v96[n5_2 + 1909] += 4;
        v96[1908] += 4;
        v97[n5_2 + 1909] += 4;
        v97[1908] += 4;
        v53 = *(uint32_t *)(_this + 12);
      }
      if ( (v53 & 7) != 0 )
      {
        v96[n5_2 + 1893] += 3;
        v96[1892] += 3;
        v97[n5_2 + 1893] += 3;
        v97[1892] += 3;
        v53 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v47 = _this + 16 * (*(uint32_t *)(_this - 4 * v46 + 108) + v45 - *(uint32_t *)(_this + 4 * v46 + 100));
      *(uint16_t *)(v47 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v47 + 3800) += 7;
      v48 = *(uint32_t *)(_this + 104);
      v49 = v48 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 96) + 100);
      v50 = v8 + v48 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 96) + 108);
      v51 = (uint16_t *)(_this + 16 * v49);
      v51[n5_2 + 1901] += 6;
      v51[1900] += 6;
      v52 = (uint16_t *)(_this + 16 * v50);
      v52[n5_4 + 1901] += 4;
      v52[1900] += 4;
      v53 = *(uint32_t *)(_this + 12);
      if ( (v53 & 7) != 7 )
      {
        v51[n5_2 + 1909] += 4;
        v51[1908] += 4;
        v52[n5_4 + 1909] += 2;
        v52[1908] += 2;
        v53 = *(uint32_t *)(_this + 12);
      }
      if ( (v53 & 7) != 0 )
      {
        v51[n5_2 + 1893] += 3;
        v51[1892] += 3;
        v52[n5_4 + 1893] += 2;
        v52[1892] += 2;
        v53 = *(uint32_t *)(_this + 12);
      }
    }
    v54 = *(uint32_t *)(_this + 112);
    if ( v54 == 1 )
    {
      v92 = v53 - *(uint32_t *)(_this + 120);
      v93 = (uint16_t *)(_this + 16 * (v92 + *(uint32_t *)(_this + 116)));
      v94 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 124) + v92));
      v93[n5_2 + 1901] += 6;
      v93[1900] += 6;
      v94[n5_2 + 1901] += 6;
      v94[1900] += 6;
      v61 = *(uint32_t *)(_this + 12);
      if ( (v61 & 7) != 7 )
      {
        v93[n5_2 + 1909] += 4;
        v93[1908] += 4;
        v94[n5_2 + 1909] += 4;
        v94[1908] += 4;
        v61 = *(uint32_t *)(_this + 12);
      }
      if ( (v61 & 7) != 0 )
      {
        v93[n5_2 + 1893] += 3;
        v93[1892] += 3;
        v94[n5_2 + 1893] += 3;
        v94[1892] += 3;
        v61 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v55 = _this + 16 * (*(uint32_t *)(_this - 4 * v54 + 124) + v53 - *(uint32_t *)(_this + 4 * v54 + 116));
      *(uint16_t *)(v55 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v55 + 3800) += 7;
      v56 = *(uint32_t *)(_this + 120);
      v57 = v56 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 112) + 116);
      v58 = v8 + v56 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 112) + 124);
      v59 = (uint16_t *)(_this + 16 * v57);
      v59[n5_2 + 1901] += 6;
      v59[1900] += 6;
      v60 = (uint16_t *)(_this + 16 * v58);
      v60[n5_4 + 1901] += 4;
      v60[1900] += 4;
      v61 = *(uint32_t *)(_this + 12);
      if ( (v61 & 7) != 7 )
      {
        v59[n5_2 + 1909] += 4;
        v59[1908] += 4;
        v60[n5_4 + 1909] += 2;
        v60[1908] += 2;
        v61 = *(uint32_t *)(_this + 12);
      }
      if ( (v61 & 7) != 0 )
      {
        v59[n5_2 + 1893] += 3;
        v59[1892] += 3;
        v60[n5_4 + 1893] += 2;
        v60[1892] += 2;
        v61 = *(uint32_t *)(_this + 12);
      }
    }
    v62 = *(uint32_t *)(_this + 128);
    if ( v62 == 1 )
    {
      v89 = v61 - *(uint32_t *)(_this + 136);
      v90 = (uint16_t *)(_this + 16 * (v89 + *(uint32_t *)(_this + 132)));
      v91 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 140) + v89));
      v90[n5_2 + 1901] += 6;
      v90[1900] += 6;
      v91[n5_2 + 1901] += 6;
      v91[1900] += 6;
      v69 = *(uint32_t *)(_this + 12);
      if ( (v69 & 7) != 7 )
      {
        v90[n5_2 + 1909] += 4;
        v90[1908] += 4;
        v91[n5_2 + 1909] += 4;
        v91[1908] += 4;
        v69 = *(uint32_t *)(_this + 12);
      }
      if ( (v69 & 7) != 0 )
      {
        v90[n5_2 + 1893] += 3;
        v90[1892] += 3;
        v91[n5_2 + 1893] += 3;
        v91[1892] += 3;
        v69 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v63 = _this + 16 * (*(uint32_t *)(_this - 4 * v62 + 140) + v61 - *(uint32_t *)(_this + 4 * v62 + 132));
      *(uint16_t *)(v63 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v63 + 3800) += 7;
      v64 = *(uint32_t *)(_this + 136);
      v65 = v64 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 128) + 132);
      v66 = v8 + v64 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 128) + 140);
      v67 = (uint16_t *)(_this + 16 * v65);
      v67[n5_2 + 1901] += 6;
      v67[1900] += 6;
      v68 = (uint16_t *)(_this + 16 * v66);
      v68[n5_4 + 1901] += 4;
      v68[1900] += 4;
      v69 = *(uint32_t *)(_this + 12);
      if ( (v69 & 7) != 7 )
      {
        v67[n5_2 + 1909] += 4;
        v67[1908] += 4;
        v68[n5_4 + 1909] += 2;
        v68[1908] += 2;
        v69 = *(uint32_t *)(_this + 12);
      }
      if ( (v69 & 7) != 0 )
      {
        v67[n5_2 + 1893] += 3;
        v67[1892] += 3;
        v68[n5_4 + 1893] += 2;
        v68[1892] += 2;
        v69 = *(uint32_t *)(_this + 12);
      }
    }
    v70 = *(uint32_t *)(_this + 144);
    if ( v70 == 1 )
    {
      v86 = v69 - *(uint32_t *)(_this + 152);
      v87 = (uint16_t *)(_this + 16 * (v86 + *(uint32_t *)(_this + 148)));
      v88 = (uint16_t *)(_this + 16 * (*(uint32_t *)(_this + 156) + v86));
      v87[n5_2 + 1901] += 6;
      v87[1900] += 6;
      v88[n5_2 + 1901] += 6;
      v88[1900] += 6;
      v77 = *(uint32_t *)(_this + 12);
      if ( (v77 & 7) != 7 )
      {
        v87[n5_2 + 1909] += 4;
        v87[1908] += 4;
        v88[n5_2 + 1909] += 4;
        v88[1908] += 4;
        v77 = *(uint32_t *)(_this + 12);
      }
      if ( (v77 & 7) != 0 )
      {
        v87[n5_2 + 1893] += 3;
        v87[1892] += 3;
        v88[n5_2 + 1893] += 3;
        v88[1892] += 3;
        v77 = *(uint32_t *)(_this + 12);
      }
    }
    else
    {
      v71 = _this + 16 * (*(uint32_t *)(_this - 4 * v70 + 156) + v69 - *(uint32_t *)(_this + 4 * v70 + 148));
      *(uint16_t *)(v71 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v71 + 3800) += 7;
      v72 = *(uint32_t *)(_this + 152);
      v73 = v72 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 144) + 148);
      v74 = v8 + v72 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 144) + 156);
      v75 = (uint16_t *)(_this + 16 * v73);
      v75[n5_2 + 1901] += 6;
      v75[1900] += 6;
      v76 = (uint16_t *)(_this + 16 * v74);
      v76[n5_4 + 1901] += 4;
      v76[1900] += 4;
      v77 = *(uint32_t *)(_this + 12);
      if ( (v77 & 7) != 7 )
      {
        v75[n5_2 + 1909] += 4;
        v75[1908] += 4;
        v76[n5_4 + 1909] += 2;
        v76[1908] += 2;
        v77 = *(uint32_t *)(_this + 12);
      }
      if ( (v77 & 7) != 0 )
      {
        v75[n5_2 + 1893] += 3;
        v75[1892] += 3;
        v76[n5_4 + 1893] += 2;
        v76[1892] += 2;
        v77 = *(uint32_t *)(_this + 12);
      }
    }
    v78 = *(uint32_t *)(_this + 160);
    if ( v78 == 1 )
    {
      v83 = v77 - *(uint32_t *)(_this + 168);
      v84 = (uint16_t *)(_this + 16 * (v83 + *(uint32_t *)(_this + 164)));
      result = _this + 16 * (*(uint32_t *)(_this + 172) + v83);
      v84[n5_2 + 1901] += 6;
      v84[1900] += 6;
      *(uint16_t *)(result + 2 * n5_2 + 3802) += 6;
      *(uint16_t *)(result + 3800) += 6;
      v85 = *(uint32_t *)(_this + 12);
      if ( (v85 & 7) != 7 )
      {
        v84[n5_2 + 1909] += 4;
        v84[1908] += 4;
        *(uint16_t *)(result + 2 * n5_2 + 3818) += 4;
        *(uint16_t *)(result + 3816) += 4;
        v85 = *(uint32_t *)(_this + 12);
      }
      if ( (v85 & 7) != 0 )
      {
        v84[n5_2 + 1893] += 3;
        v84[1892] += 3;
        *(uint16_t *)(result + 2 * n5_2 + 3786) += 3;
        *(uint16_t *)(result + 3784) += 3;
      }
    }
    else
    {
      v79 = _this + 16 * (*(uint32_t *)(_this - 4 * v78 + 172) + v77 - *(uint32_t *)(_this + 4 * v78 + 164));
      *(uint16_t *)(v79 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v79 + 3800) += 7;
      v80 = *(uint32_t *)(_this + 168);
      result = _this + 16 * (v80 + *(uint32_t *)(_this + 12) - *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 160) + 164));
      v81 = (uint16_t *)(_this + 16 * (v80 - *(uint32_t *)(_this - 4 * *(uint32_t *)(_this + 160) + 172) + v8));
      *(uint16_t *)(result + 2 * n5_2 + 3802) += 6;
      *(uint16_t *)(result + 3800) += 6;
      v81[n5_4 + 1901] += 4;
      v81[1900] += 4;
      v82 = *(uint32_t *)(_this + 12);
      if ( (v82 & 7) != 7 )
      {
        *(uint16_t *)(result + 2 * n5_2 + 3818) += 4;
        *(uint16_t *)(result + 3816) += 4;
        v81[n5_4 + 1909] += 2;
        v81[1908] += 2;
        v82 = *(uint32_t *)(_this + 12);
      }
      if ( (v82 & 7) != 0 )
      {
        *(uint16_t *)(result + 2 * n5_2 + 3786) += 3;
        *(uint16_t *)(result + 3784) += 3;
        result = (uint16_t)v81[n5_4 + 1893] + 2;
        v81[n5_4 + 1893] = result;
        v81[1892] += 2;
      }
    }
  }
  return result;
}
static inline uint32_t __fwd_sub_4256F0_sub_4118A0(void *a0, void *a1) { return __sub_4118A0((uint8_t *)a0, (char *)a1); }
static inline uint16_t * __fwd_sub_4256F0_sub_413560(void *a0) { return __sub_413560((uint16_t *)a0); }

int32_t *__sub_4256F0(int32_t *_this, int32_t i, int32_t a3, int32_t n4)
{
  ;
  bool v11;
  int32_t v6, v7, v8, v9, v10, v12, v13, v15, v16, v17, v18, v20, v21, v22, v23, v24, v25, v27,
          v28;
  uint32_t n0x99C60, n0x80, n5;
  n0x99C60 = 0;
  *_this = i;
  *(_this + 1) = a3;
  do
    __fwd_sub_4256F0_sub_413560((uint16_t *)_this + 8 * n0x99C60++ + 1900);
  while ( n0x99C60 < 0x99C60 );
  *(_this + 2) = 0;
  v6 = 0;
  v7 = 0;
  v8 = 0;
  v27 = n4 << 8;
  do
  {
    v9 = (uint8_t)__byte_439BC0[v6];
    v28 = v8;
    *((uint8_t *)_this + 2 * v8 + 216) = v6;
    v10 = (2 * v8 == v9) + v6;
    *(_this + 2 * v8 + 438) = v27 | v7;
    v11 = 2 * v8 == (uint8_t)__byte_439BC8[v7];
    *((uint8_t *)_this + 2 * v8 + 217) = v10;
    v12 = 2 * v8 + 1;
    v13 = v11 + v7;
    v6 = (v12 == (uint8_t)__byte_439BC0[v10]) + v10;
    *(_this + 2 * v8 + 439) = v13 | v27;
    v7 = (v12 == (uint8_t)__byte_439BC8[v13]) + v13;
    ++v8;
  }
  while ( (uint32_t)(v28 + 1) < 0x100 );
  n0x80 = 0;
  v15 = 0;
  do
  {
    v16 = (uint8_t)__byte_439BD0[v15];
    *((uint8_t *)_this + 2 * n0x80 + 728) = 8 * v15;
    v17 = (2 * n0x80 == v16) + v15;
    *((uint8_t *)_this + 2 * n0x80 + 729) = 8 * v17;
    v18 = 2 * n0x80++ + 1 == (uint8_t)__byte_439BD0[v17];
    v15 = v18 + v17;
  }
  while ( n0x80 < 0x80 );
  *(_this + 10) = 32;
  *(_this + 41) = 0;
  *(_this + 37) = 0;
  *(_this + 11) = 64;
  *(_this + 33) = 0;
  *(_this + 29) = 0;
  *(_this + 25) = 0;
  *(_this + 14) = 96;
  *(_this + 21) = 0;
  *(_this + 17) = 0;
  *(_this + 13) = 0;
  *(_this + 15) = 192;
  *(_this + 9) = 0;
  n5 = 0;
  *(_this + 18) = 288;
  *(_this + 19) = 576;
  *(_this + 22) = 864;
  *(_this + 23) = 1728;
  *(_this + 26) = 2592;
  *(_this + 27) = 5184;
  *(_this + 30) = 7776;
  *(_this + 31) = 15552;
  *(_this + 34) = 23328;
  *(_this + 35) = 46656;
  *(_this + 38) = 69984;
  *(_this + 39) = 139968;
  *(_this + 42) = 209952;
  *(_this + 43) = 419904;
  do
    *(_this + n5++ + 44) = (int32_t)malloc(2 * *_this + 20);
  while ( n5 < 5 );
  __fwd_sub_4256F0_sub_4118A0((uint8_t *)_this + 984, (char *)_this + 1496);
  v20 = *_this;
  if ( *_this > -10 )
  {
    v21 = 0;
    do
    {
      *(uint8_t *)(*(_this + 48) + 2 * v21) = 72;
      *(uint8_t *)(*(_this + 47) + 2 * v21) = 72;
      *(uint8_t *)(*(_this + 46) + 2 * v21) = 72;
      *(uint8_t *)(*(_this + 45) + 2 * v21) = 72;
      *(uint8_t *)(*(_this + 44) + 2 * v21) = 72;
      *(uint8_t *)(*(_this + 48) + 2 * v21 + 1) = 0;
      *(uint8_t *)(*(_this + 47) + 2 * v21 + 1) = 0;
      *(uint8_t *)(*(_this + 46) + 2 * v21 + 1) = 0;
      *(uint8_t *)(*(_this + 45) + 2 * v21 + 1) = 0;
      *(uint8_t *)(*(_this + 44) + 2 * v21 + 1) = 0;
      v20 = *_this;
      ++v21;
    }
    while ( v21 < *_this + 10 );
  }
  v22 = *(_this + 45);
  *(_this + 49) = *(_this + 44) + 2 * v20 + 8;
  v23 = *(_this + 46);
  *(_this + 50) = v22 + 2 * v20 + 8;
  v24 = *(_this + 47);
  *(_this + 51) = v23 + 2 * v20 + 8;
  v25 = *(_this + 48);
  *(_this + 52) = v24 + 2 * v20 + 8;
  *(_this + 53) = v25 + 2 * v20 + 8;
  return _this;
}

 BMF_SSE char *__sub_414F60()
{
  ;
  __m128i si128, *v8;
  char *__sub_414F60_n256, *v6, *v7, *__sub_414F60_n256_1;
  int32_t Buffer, v2, v3, v9, v10, v13;
  uint32_t i;
  uint64_t *v4;
  __sub_414F60_n256 = (char *)::__n256;
  *(uint32_t *)__Buffer_1 = ::__n256;
  Buffer = __Buffer_0;
  if ( __Buffer_0 != __Buffer_1 )
  {
    __sub_414F60_n256 = (char *)(__n8 - 8);
    if ( __n8 - 8 < 0 )
    {
      __n8 -= 8;
    }
    else
    {
      do
      {
        --Buffer;
        __sub_414F60_n256 = (char *)((uintptr_t)__sub_414F60_n256 - (8));
      }
      while ( (int32_t)__sub_414F60_n256 >= 0 );
      __Buffer_0 = Buffer;
      __n8 = (int32_t)__sub_414F60_n256;
    }
  }
  if ( __dword_443364 )
  {
    if ( __n2 == 2 )
    {
      __n8_0 = 8;
      __n8_1 = 8;
    }
    else
    {
      __n8_1 = 16;
      __n8_0 = 64;
    }
    __n2_0 = 2;
    __dword_445660[0] = 33685760;
    __byte_44571E = 0;
    __byte_44571D = 1;
    __n2_1 = 2;
    __n4 = 4;
    __byte_445722 = 1;
    __dword_445660[1] = 50529027;
    __n4_0 = 4;
    v2 = 2 * (uint8_t)__n2_1 + 4;
    __byte_445724 = 2 * __n2_1 + 4;
    __byte_445726 = 2 * __n2_1;
    *(uint64_t *)(v2 + BMF_BLOB(0x00445660)) = 0x404040404040404LL;
    __n8_2 = 8;
    v3 = v2 + 2 * (uint8_t)__n4_0;
    __byte_445728 = v3;
    __byte_44572A = v3 - 5;
    *(uint64_t *)(v3 + BMF_BLOB(0x00445660)) = 0x505050505050505LL;
    *(uint64_t *)(v3 + BMF_BLOB(0x00445668)) = 0x505050505050505LL;
    __n16 = 16;
    v4 = (uint64_t *)(v3 + 2 * (uint8_t)__n8_2);
    __byte_44572C = (char)v4;
    __byte_44572E = (uint8_t)v4 - 6;
    *(uint64_t *)((char *)v4 + BMF_BLOB(0x00445660)) = 0x606060606060606LL;
    *(uint64_t *)((char *)v4 + BMF_BLOB(0x00445668)) = 0x606060606060606LL;
    *(uint64_t *)((char *)v4 + BMF_BLOB(0x00445670)) = 0x606060606060606LL;
    *(uint64_t *)((char *)v4 + BMF_BLOB(0x00445678)) = 0x606060606060606LL;
    si128 = _mm_load_si128((const __m128i *)&__xmmword_439800);
    __n32 = 32;
    v6 = (char *)v4 + 2 * (uint8_t)__n16;
    __byte_445730 = (char)v6;
    __byte_445732[0] = (uint8_t)v6 - 7;
    v7 = (char *)__dword_445660 + (uint32_t)v6;
    *(uint64_t *)v7 = si128.m128i_i64[0];
    v8 = (__m128i *)(((int32_t)&__dword_445660[3] + (uint32_t)v6 + 3) & 0xFFFFFFF0);
    *((uint32_t *)v7 + 2) = 117901063;
    *((uint16_t *)v7 + 6) = 1799;
    v7[14] = 7;
    *((uint64_t *)v7 + 6) = si128.m128i_i64[0];
    *((uint64_t *)v7 + 7) = si128.m128i_i64[0];
    *v8 = si128;
    v8[1] = si128;
    v8[2] = si128;
    __sub_414F60_n256 = (char *)malloc(0x7F000u);
    if ( __sub_414F60_n256 )
    {
      v9 = 0;
      v10 = 0;
      __sub_414F60_n256_1 = __sub_414F60_n256;
      do
      {
        v13 = v9;
        *((uint16_t *)__sub_414F60_n256_1 + 1) = 24 * __n8_1;
        *(uint16_t *)&__sub_414F60_n256[v10 + 4] = 205;
        *(uint16_t *)&__sub_414F60_n256[v10 + 12] = 48;
        *(uint16_t *)&__sub_414F60_n256[v10 + 6] = 124;
        *(uint16_t *)&__sub_414F60_n256[v10 + 14] = 16;
        *(uint16_t *)&__sub_414F60_n256[v10 + 8] = 147;
        *(uint16_t *)&__sub_414F60_n256[v10 + 10] = 83;
        *(uint16_t *)&__sub_414F60_n256[v10 + 16] = 8;
        *(uint16_t *)&__sub_414F60_n256[v10 + 18] = 4;
        *(uint16_t *)__sub_414F60_n256_1 = 635;
        for ( i = 0; i < 0x7A; ++i )
        {
          *(uint16_t *)&__sub_414F60_n256_1[4 * i + 20] = 60;
          *(uint16_t *)&__sub_414F60_n256_1[4 * i + 22] = 36;
        }
        __sub_414F60_n256_1 += 508;
        v10 += 508;
        ++v9;
      }
      while ( (uint32_t)(v13 + 1) < 0x400 );
    }
    else
    {
      __sub_414F60_n256 = nullptr;
    }
    ::__n256_1 = __sub_414F60_n256;
  }
  rc.enc_init();
  return __sub_414F60_n256;
}
static inline int32_t __fwd_sub_424550_sub_4135A0(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_4135A0((uint16_t *)a0, a1, a2, a3); }
static inline int32_t __fwd_sub_424550_sub_4248D0(void *a0, void *a1, int32_t a2) { return __sub_4248D0((uint8_t **)a0, (uint32_t *)a1, a2); }

void __sub_424550(int32_t _this, uint8_t *a2, uint8_t *a3)
{
  ;
  bool v31;
  char v33, v35;
  int32_t v4, v5, v6, v7, v8, v9, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23,
          v24, v25, v26, v27, v28, v29, v30, v34, n5, n16, v38, v40, v42;
  int64_t v39;
  uint8_t *v10, *v11, *v32;
  __sub_414F60();
  if ( *(int32_t *)(_this + 4) > 0 )
  {
    v4 = 0;
    do
    {
      ++v4;
      **(uint16_t **)(_this + 196) = *(uint16_t *)(*(uint32_t *)(_this + 196) - 2);
      *(uint16_t *)(*(uint32_t *)(_this + 196) + 2) = *(uint16_t *)(*(uint32_t *)(_this + 196) - 4);
      *(uint16_t *)(*(uint32_t *)(_this + 196) + 4) = *(uint16_t *)(*(uint32_t *)(_this + 196) - 6);
      *(uint16_t *)(*(uint32_t *)(_this + 196) + 6) = *(uint16_t *)(*(uint32_t *)(_this + 196) - 8);
      *(uint16_t *)(*(uint32_t *)(_this + 196) + 8) = *(uint16_t *)(*(uint32_t *)(_this + 196) - 10);
      *(uint16_t *)(*(uint32_t *)(_this + 196) + 10) = *(uint16_t *)(*(uint32_t *)(_this + 196) - 12);
      v5 = *(uint32_t *)(_this + 192);
      v6 = *(uint32_t *)(_this + 188);
      v7 = *(uint32_t *)(_this + 184);
      v8 = *(uint32_t *)(_this + 180);
      v9 = *(uint32_t *)(_this + 176);
      *(uint32_t *)(_this + 192) = v6;
      *(uint32_t *)(_this + 188) = v7;
      *(uint32_t *)(_this + 184) = v8;
      *(uint32_t *)(_this + 180) = v9;
      *(uint32_t *)(_this + 176) = v5;
      v5 += 8;
      *(uint32_t *)(_this + 196) = v5;
      v9 += 8;
      *(uint32_t *)(_this + 200) = v9;
      *(uint32_t *)(_this + 204) = v8 + 8;
      *(uint32_t *)(_this + 208) = v7 + 8;
      *(uint32_t *)(_this + 212) = v6 + 8;
      *(uint16_t *)(v5 - 8) = *(uint16_t *)(v9 + 6);
      *(uint16_t *)(*(uint32_t *)(_this + 196) - 6) = *(uint16_t *)(*(uint32_t *)(_this + 200) + 4);
      *(uint16_t *)(*(uint32_t *)(_this + 196) - 4) = *(uint16_t *)(*(uint32_t *)(_this + 200) + 2);
      *(uint16_t *)(*(uint32_t *)(_this + 196) - 2) = **(uint16_t **)(_this + 200);
      v10 = *(uint8_t **)(_this + 204);
      v11 = *(uint8_t **)(_this + 212);
      *(uint32_t *)(_this + 20) = 0;
      *(uint32_t *)(_this + 24) = 0;
      *(uint32_t *)(_this + 28) = 0;
      v12 = *(v10 - 3);
      *(uint32_t *)(_this + 24) = v12;
      v13 = *(v10 - 1);
      *(uint32_t *)(_this + 28) = v13;
      v14 = *(v11 - 3) + v12;
      *(uint32_t *)(_this + 24) = v14;
      v15 = *(v11 - 1) + v13;
      *(uint32_t *)(_this + 28) = v15;
      v16 = v10[1] + v14;
      *(uint32_t *)(_this + 24) = v16;
      v17 = v10[3] + v15;
      *(uint32_t *)(_this + 28) = v17;
      v18 = v11[1] + v16;
      *(uint32_t *)(_this + 24) = v18;
      v19 = v11[3] + v17;
      *(uint32_t *)(_this + 28) = v19;
      v20 = v10[5] + v18;
      *(uint32_t *)(_this + 24) = v20;
      v21 = v10[7] + v19;
      *(uint32_t *)(_this + 28) = v21;
      v22 = v11[5] + v20;
      *(uint32_t *)(_this + 24) = v22;
      v23 = v11[7] + v21;
      *(uint32_t *)(_this + 28) = v23;
      v24 = v10[9] + v22;
      *(uint32_t *)(_this + 24) = v24;
      v25 = v10[11] + v23;
      *(uint32_t *)(_this + 28) = v25;
      v26 = v11[9] + v24;
      v27 = *(uint32_t *)(_this + 196);
      *(uint32_t *)(_this + 24) = v26;
      v28 = v11[11] + v25;
      *(uint32_t *)(_this + 28) = v28;
      v29 = *(uint8_t *)(v27 - 7) + v26;
      *(uint32_t *)(_this + 24) = v29;
      v30 = *(uint8_t *)(v27 - 5) + v28;
      *(uint32_t *)(_this + 28) = v30;
      *(uint32_t *)(_this + 24) = *(uint8_t *)(v27 - 3) + v29;
      v31 = *(uint32_t *)_this <= 0;
      *(uint32_t *)(_this + 28) = *(uint8_t *)(v27 - 1) + v30;
      if ( !v31 )
      {
        v32 = a3;
        v40 = v4;
        v42 = 0;
        do
        {
          ++v42;
          __fwd_sub_424550_sub_4248D0((uint8_t **)_this, nullptr, 0);
          v33 = *(uint8_t *)(_this + 8);
          v34 = (uint8_t)(*a2 - v33);
          v35 = *(uint8_t *)(*(uint8_t *)(_this + v34 + 984) + _this + 1496) + v33;
          n5 = *(uint8_t *)(_this + v34 + 984);
          n16 = (uint8_t)*v32 - (uint8_t)(v35 + *v32 - *a2);
          if ( n16 < -16 || n16 > 16 )
          {
            *v32 = *a2;
            n5 = *(uint8_t *)(_this + v34 + 1240);
          }
          else
          {
            *v32 = v35;
          }
          __fwd_sub_424550_sub_4135A0(
            (uint16_t *)(_this + 16 * *(uint32_t *)(_this + 12) + 3800),
            16 * *(uint32_t *)(_this + 12),
            *(uint32_t *)(_this + 16),
            n5);
          v38 = (uint8_t)*v32;
          v39 = v38 - *(int32_t *)(_this + 8);
          **(uint8_t **)(_this + 196) = v38;
          *(uint8_t *)(*(uint32_t *)(_this + 196) + 1) = (BYTE4(v39) ^ v39) - BYTE4(v39);
          *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 20) + 24) = *(uint32_t *)(_this + 4 * *(uint32_t *)(_this + 20) + 24)
                                                              + *(uint8_t *)(*(uint32_t *)(_this + 196) + 1)
                                                              - *(uint8_t *)(*(uint32_t *)(_this + 196) - 7)
                                                              - (*(uint8_t *)(*(uint32_t *)(_this + 212) - 3)
                                                               - *(uint8_t *)(*(uint32_t *)(_this + 212) + 13)
                                                               + *(uint8_t *)(*(uint32_t *)(_this + 204) - 3)
                                                               - *(uint8_t *)(*(uint32_t *)(_this + 204) + 13));
          LODWORD(v39) = 16 * *(uint32_t *)(_this + 12);
          *(uint32_t *)(_this + 20) = *(uint32_t *)(_this + 20) == 0;
          if ( *(uint16_t *)(v39 + _this + 3800) < 0x4000u )
            __sub_4259F0(_this);
          *(uint32_t *)(_this + 196) += 2;
          ++v32;
          *(uint32_t *)(_this + 200) += 2;
          *(uint32_t *)(_this + 204) += 2;
          *(uint32_t *)(_this + 208) += 2;
          *(uint32_t *)(_this + 212) += 2;
          ++a2;
        }
        while ( v42 < *(uint32_t *)_this );
        v4 = v40;
        a3 = v32;
      }
    }
    while ( v4 < *(uint32_t *)(_this + 4) );
  }
  __sub_414CE0();
}
static inline void ** __fwd_sub_424500_sub_4244A0(void *a0, char a1) { return __sub_4244A0((void **)a0, a1); }
static inline void __fwd_sub_424500_sub_424550(int32_t a0, void *a1, void *a2) { __sub_424550(a0, (uint8_t *)a1, (uint8_t *)a2); }
static inline int32_t * __fwd_sub_424500_sub_4256F0(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_4256F0((int32_t *)a0, a1, a2, a3); }

 void __sub_424500(uint8_t *a1, int32_t i, int32_t a3, uint8_t *a4)
{
  ;
  int32_t *v4;
  void **v5;
  v4 = (int32_t *)malloc(0x99D4D8u);
  if ( v4 )
    v5 = (void **)__fwd_sub_424500_sub_4256F0(v4, i, a3, 0);
  else
    v5 = nullptr;
  __fwd_sub_424500_sub_424550((int32_t)v5, a1, a4);
  if ( v5 )
    __fwd_sub_424500_sub_4244A0(v5, 1);
}
 int32_t __sub_42CBB0(int32_t __sub_402E30)
{
  ;
  int32_t __sub_42CBB0_psub_402E30;
  __sub_42CBB0_psub_402E30 = ::__psub_402E30;
  ::__psub_402E30 = __sub_402E30;
  return __sub_42CBB0_psub_402E30;
}
// BMF ran its filters with the SSE unit in a particular mode: denormal
// results flushed to zero, and denormal inputs treated as zero.  It got there
// through Intel's dispatcher -- sub_4346D0, which read a CPU-feature level out
// of a global that sub_434A30 had filled in from CPUID, took an FXSAVE to ask
// whether the part could do DAZ at all, and printed an Intel runtime error and
// exited on a machine without SSE2.
//
// SSE2 is a given here -- the bodies are full of it and would fault long
// before this ran -- which settles all three questions: the level test is
// always taken, DAZ has been available on every part that has SSE2, and the
// no-SSE2 exit is unreachable.  What is left is the two mode bits, and
// <xmmintrin.h> and <pmmintrin.h> already name them.
//
// main passed 3 -- flush-to-zero and denormals-are-zero, not the third bit --
// so these two lines are the whole of what it did.
static void bmf_set_denormal_mode()
{
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

static inline uint32_t __fwd_sub_4229E0_sub_4118A0(void *a0, void *a1) { return __sub_4118A0((uint8_t *)a0, (char *)a1); }

int32_t __sub_4229E0(int32_t _this, int32_t i, int32_t n4)
{
  ;
  char v16;
  int32_t v7, v8, v9, v13, Size, v17, v18, v20, v21, v22, v23, v24, v26, v27, v28, v29;
  uint32_t j, k, n0x1E60, m_1, m, n5, n0x82, n;
  void *v10;
  *(uint32_t *)(_this + 278728) = n4;
  for ( j = 0; j < 0x14000; ++j )
  {
    *(uint16_t *)(_this + 8 * j + 284714) = 0;
    *(uint16_t *)(_this + 8 * j + 284718) = 0;
  }
  for ( k = 0; k < 0x14000; ++k )
  {
    *(uint8_t *)(_this + 8 * k + 284712) = 5;
    *(uint8_t *)(_this + 8 * k + 284713) = 2;
    *(uint8_t *)(_this + 8 * k + 284716) = 5;
    *(uint8_t *)(_this + 8 * k + 284717) = 2;
  }
  n0x1E60 = 0;
  do
  {
    v7 = 16 * n0x1E60;
    *(uint16_t *)(_this + v7 + 940074) = 2048;
    ++n0x1E60;
    *(uint16_t *)(_this + v7 + 940076) = 2816;
    *(uint16_t *)(_this + v7 + 940078) = 2816;
    *(uint16_t *)(_this + v7 + 940072) = 4096;
    *(uint16_t *)(_this + v7 + 940082) = 2048;
    *(uint16_t *)(_this + v7 + 940084) = 2816;
    *(uint16_t *)(_this + v7 + 940086) = 2816;
    *(uint16_t *)(_this + v7 + 940080) = 4096;
  }
  while ( n0x1E60 < 0x1E60 );
  v8 = 4 * __n256_0[0] + 1;
  v9 = 16 * __n256_0[0];
  *(uint32_t *)(_this + 278732) = (uint8_t)(__byte_44339E[16
                                                           * (uint8_t)__byte_44339D[16
                                                                                        * *(uint32_t *)(_this + 278728)]]
                                               & 8) >> 3;
  __dword_4458F0 = v8;
  __dword_4458F4 = -v8;
  *(uint32_t *)(_this + 278720) = -v9 - 7;
  *(uint32_t *)(_this + 278724) = v9 + 8;
  *(uint32_t *)(_this + 278660) = malloc(4 * i + 16);
  v10 = malloc(4 * i + 16);
  *(uint32_t *)(_this + 232) = 1065353216;
  *(uint32_t *)(_this + 278664) = v10;
  *(uint32_t *)(_this + 278668) = *(uint32_t *)(_this + 278660) + 4 * i + 8;
  if ( i > -4 )
  {
    m_1 = (i + 4) / 2;
    if ( m_1 )
    {
      for ( m = 0; m < m_1; ++m )
      {
        *(uint32_t *)(*(uint32_t *)(_this + 278664) + 8 * m) = _this;
        *(uint32_t *)(*(uint32_t *)(_this + 278660) + 8 * m) = _this;
        *(uint32_t *)(*(uint32_t *)(_this + 278664) + 8 * m + 4) = _this;
        *(uint32_t *)(*(uint32_t *)(_this + 278660) + 8 * m + 4) = _this;
      }
      v13 = 2 * m + 1;
    }
    else
    {
      v13 = 1;
    }
    if ( i + 4 > (uint32_t)(v13 - 1) )
    {
      *(uint32_t *)(*(uint32_t *)(_this + 278664) + 4 * v13 - 4) = _this;
      *(uint32_t *)(*(uint32_t *)(_this + 278660) + 4 * v13 - 4) = _this;
    }
  }
  n5 = 0;
  Size = 18 * i + 234;
  do
    *(uint32_t *)(_this + 4 * n5++ + 278756) = malloc(Size);
  while ( n5 < 5 );
  memset(*(char **)(_this + 278756),0,Size);
  v17 = *(uint32_t *)(_this + 278756);
  __dword_4458EC = 0;
  v18 = 0;
  __dword_4458E8 = 0;
  __dword_4458E4 = 0;
  n0x82 = 0;
  __dword_4458E0 = 0;
  *(uint32_t *)(_this + 278736) = v17 + 144;
  do
  {
    v20 = (uint8_t)__byte_439890[v18];
    *(uint32_t *)(_this + 8 * n0x82 + 278944) = (*(uint32_t *)(_this + 278728) << 8) | (16 * v18);
    v21 = (2 * n0x82 == v20) + v18;
    v22 = (uint8_t)__byte_439890[v21];
    *(uint32_t *)(_this + 8 * n0x82 + 278948) = (*(uint32_t *)(_this + 278728) << 8) | (16 * v21);
    v23 = 2 * n0x82++ + 1 == v22;
    v18 = v23 + v21;
  }
  while ( n0x82 < 0x82 );
  v24 = 0;
  for ( n = 0; n < 0x3C; ++n )
  {
    v26 = (uint8_t)__byte_4398A0[v24];
    *(uint8_t *)(_this + 2 * n + 280752) = v24;
    v27 = (2 * n == v26) + v24;
    v28 = (uint8_t)__byte_4398A0[v27];
    *(uint8_t *)(_this + 2 * n + 280753) = v27;
    v29 = 2 * n + 1 == v28;
    v24 = v29 + v27;
  }
  *(uint32_t *)(_this + 278704) = 15;
  __fwd_sub_4229E0_sub_4118A0((uint8_t *)(_this + 279984), (char *)(_this + 280496));
  *(uint32_t *)(_this + 278844) = 0;
  *(uint32_t *)(_this + 278784) = 64;
  *(uint32_t *)(_this + 278828) = 0;
  *(uint32_t *)(_this + 278788) = 128;
  *(uint32_t *)(_this + 278812) = 0;
  *(uint32_t *)(_this + 278800) = 192;
  *(uint32_t *)(_this + 278796) = 0;
  *(uint32_t *)(_this + 278804) = 384;
  *(uint32_t *)(_this + 278780) = 0;
  *(uint32_t *)(_this + 278816) = 576;
  *(uint32_t *)(_this + 278820) = 1152;
  *(uint32_t *)(_this + 278832) = 1728;
  *(uint32_t *)(_this + 278836) = 3456;
  *(uint32_t *)(_this + 278848) = 5184;
  *(uint32_t *)(_this + 278852) = 10368;
  return _this;
}

 int32_t *__sub_42B830(int32_t a1, int32_t a2, int32_t n5, int32_t a4, int32_t a5)
{
  ;
  char v13, *buf;
  int32_t n4, v7, v8, v9, v10, Size, *result, *v15;
  uint32_t v5;
  LOWORD(v5) = a1;
  n4 = n5;
  v7 = (uint8_t)n5 << 16;
  if ( n5 >= 5 && n5 <= 7 )
    goto LABEL_19;
  if ( n5 == 3 )
  {
    n4 = 4;
  }
  else
  {
    if ( !a5 )
    {
      n4 = 4;
      if ( n5 > 4 )
        n4 = n5;
    }
    if ( n4 >= 8 )
      goto LABEL_18;
  }
  if ( !a5 )
  {
    if ( n4 == 4 )
    {
      v5 = ((a1 + 7) >> 1) & 0xFFFFFFFC;
LABEL_19:
      v9 = (uint16_t)v5;
      goto LABEL_20;
    }
LABEL_18:
    LOWORD(v5) = ((n4 + 7) >> 3) * a1;
    goto LABEL_19;
  }
  if ( n4 == 1 )
  {
    v8 = (int32_t)(((uint32_t)((a1 + 7) >> 2) >> 29) + a1 + 7) >> 3;
  }
  else if ( n4 == 2 )
  {
    v8 = (int32_t)(((uint32_t)((a1 + 3) >> 1) >> 30) + a1 + 3) >> 2;
  }
  else
  {
    v8 = (int32_t)(a1 + ((uint32_t)(a1 + 1) >> 31) + 1) >> 1;
  }
  v7 = ((uint8_t)n5 << 16) | 0x40000000;
  v9 = (uint16_t)v8;
LABEL_20:
  v10 = v9 * a2;
  if ( a4 )
  {
    Size = 3 << (n5 & 31);
    if ( n5 > 8 )
      Size = 0;
    else
      v7 = ((BYTE2(v7) | 0xFFFF0080) << 16) | v7 & 0xFF00FFFF;
  }
  else
  {
    Size = 0;
  }
  result = (int32_t *)malloc(v10 + Size + 19);
  if ( !result )
    return nullptr;
  *result = (a2 << 16) | (uint16_t)a1;
  result[1] = v9;
  result[2] = v7;
  result[3] = v10;
  if ( Size )
  {
    if ( (*((uint8_t *)result + 10) & 0x80) != 0 )
      buf = (char *)result + result[3] + 16;
    else
      buf = nullptr;
    v15 = result;
    memset(buf,0,Size);
    return v15;
  }
  return result;
}

 int32_t __sub_42B0C0(int32_t p_i, char *FileName, int32_t a3)
{
  alignas(16) uint8_t __hexrays_frame[96];
  char &Buffer = *(char *)(__hexrays_frame + 0);
  uint32_t &Buffera = *(uint32_t *)(__hexrays_frame + 0);
  char * &Bufferb = *(char * *)(__hexrays_frame + 0);
  char * &Bufferd = *(char * *)(__hexrays_frame + 0);
  char * &Bufferc = *(char * *)(__hexrays_frame + 0);
  uint32_t &v62 = *(uint32_t *)(__hexrays_frame + 4);
  uint32_t &v63 = *(uint32_t *)(__hexrays_frame + 4);
  uint32_t &Size_2 = *(uint32_t *)(__hexrays_frame + 4);
  char * &v65 = *(char * *)(__hexrays_frame + 8);
  int32_t &v66 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v67 = *(int32_t *)(__hexrays_frame + 12);
  uint8_t * &v68 = *(uint8_t * *)(__hexrays_frame + 12);
  FILE * &Stream_v = *(FILE * *)(__hexrays_frame + 16);
  int32_t &Buffer_2 = *(int32_t *)(__hexrays_frame + 20);
  char * &buf = *(char * *)(__hexrays_frame + 24);
  uint8_t * &v72 = *(uint8_t * *)(__hexrays_frame + 28);
  int32_t &Buffer_5 = *(int32_t *)(__hexrays_frame + 36);
  int32_t &n2_2 = *(int32_t *)(__hexrays_frame + 44);
  int32_t &v75 = *(int32_t *)(__hexrays_frame + 48);
  char * &buf_2 = *(char * *)(__hexrays_frame + 52);
  uint32_t &v77 = *(uint32_t *)(__hexrays_frame + 60);
  ;
  FILE *Stream_1, *Stream_2;
  bool v33;
  char *Bufferc_3, v7, *Bufferc_1, *Bufferb_1, *v19, *v24, *buf_1, *v31, *v39, v42, v43, v44,
       v45, v46, v47, *buf_3, *Bufferc_2, v56;
  int32_t v3, p_i_1, i, Buffer_1, n8, v14, v16, v17, v20, v21, v22, v23, v25, v26, v28, n4, n2,
          Buffer_3, Buffer_4, Size, v40, n2_1, v49, Size_1, v55;
  uint16_t *p_i_2;
  uint32_t v15, v18, Bufferb_2, Size_3, ElementCount, v53;
  v3 = a3;
  Stream_1 = fopen(FileName, "wb");
  if ( !Stream_1 )
    return 0;
  Bufferc_3 = (char *)malloc(*(uint32_t *)(p_i + 12)
                                 + 8 * *(uint16_t *)(p_i + 2)
                                 + (*(uint32_t *)(p_i + 12) >> 5) + 2048);
  p_i_1 = p_i;
  Bufferc_1 = Bufferc_3;
  i = *(uint16_t *)p_i;
  *(uint32_t *)(Bufferc_3 + 14) = 40;
  *(uint16_t *)Bufferc_3 = 19778;
  Buffer_1 = *(uint16_t *)(p_i + 2);
  Buffer_2 = Buffer_1;
  *((uint16_t *)Bufferc_3 + 4) = 0;
  *((uint16_t *)Bufferc_3 + 3) = 0;
  *(uint32_t *)(Bufferc_3 + 18) = i;
  *(uint32_t *)(Bufferc_3 + 22) = Buffer_1;
  LOBYTE(Buffer_1) = *(uint8_t *)(p_i + 10);
  Buffer = Buffer_1;
  *((uint16_t *)Bufferc_3 + 13) = 1;
  n8 = Buffer_1 & 0x3F;
  *((uint16_t *)Bufferc_3 + 14) = n8;
  *(uint32_t *)(Bufferc_3 + 50) = 0;
  *(uint32_t *)(Bufferc_3 + 46) = 0;
  *(uint32_t *)(Bufferc_3 + 42) = 0;
  *(uint32_t *)(Bufferc_3 + 38) = 0;
  buf = Bufferc_3 + 54;
  if ( n8 <= 8 )
  {
    v14 = 1 << (n8 & 31);
    v66 = 1 << (n8 & 31);
    if ( (Buffer & 0x40) != 0 )
    {
      Buffera = 0x100u >> (n8 & 31);
      if ( v66 > 0 )
      {
        if ( v66 / 2 )
        {
          v15 = 0;
          v16 = 0;
          do
          {
            *(uint32_t *)&Bufferc_1[8 * v15 + 54] = ((uint8_t)v16 << 16)
                                                | (uint8_t)v16
                                                | ((uint8_t)v16 << 8);
            *(uint32_t *)&Bufferc_1[8 * v15 + 58] = ((uint8_t)(v16 - Buffera + 2 * Buffera) << 16)
                                                | (uint8_t)(v16 - Buffera + 2 * Buffera)
                                                | ((uint8_t)(v16 - Buffera + 2 * Buffera) << 8);
            v16 += 2 * Buffera;
            ++v15;
          }
          while ( v15 < v66 / 2 );
          p_i_1 = p_i;
          v17 = 2 * v15 + 1;
        }
        else
        {
          v17 = 1;
        }
        if ( v17 - 1 < (uint32_t)v66 )
          *(uint32_t *)&Bufferc_1[4 * v17 + 50] = ((uint8_t)(Buffera * (v17 - 1)) << 16)
                                              | (uint8_t)(Buffera * (v17 - 1))
                                              | ((uint8_t)(Buffera * (v17 - 1)) << 8);
      }
      Bufferb_1 = *(char **)(p_i_1 + 12);
      v67 = 4 * v66;
    }
    else if ( Buffer < 0 )
    {
      Bufferb_1 = *(char **)(p_i + 12);
      if ( v66 <= 0 )
      {
        v67 = 4 * v14;
      }
      else
      {
        v62 = v14 / 2;
        if ( v14 / 2 )
        {
          Bufferb = *(char **)(p_i + 12);
          v18 = 0;
          v19 = &Bufferb_1[p_i];
          do
          {
            v20 = 2 * v18;
            v21 = *(uint16_t *)&v19[6 * v18 + 19];
            v22 = (uint8_t)v19[6 * v18 + 21];
            *(uint32_t *)&Bufferc_1[4 * v20 + 54] = ((uint8_t)v19[6 * v18 + 18] << 16)
                                                | *(uint16_t *)&v19[6 * v18 + 16];
            *(uint32_t *)&Bufferc_1[4 * v20 + 58] = (v22 << 16) | v21;
            ++v18;
          }
          while ( v18 < v62 );
          Bufferb_1 = Bufferb;
          p_i_1 = p_i;
          v3 = a3;
          v23 = 2 * v18 + 1;
          v63 = v23;
        }
        else
        {
          v23 = 1;
          v63 = 1;
        }
        if ( v23 - 1 < (uint32_t)v66 )
        {
          Bufferd = Bufferb_1;
          v24 = &Bufferb_1[p_i_1];
          v25 = *(uint16_t *)&v24[3 * v63 + 13];
          v26 = (uint8_t)v24[3 * v63 + 15];
          Bufferb_1 = Bufferd;
          *(uint32_t *)&Bufferc_1[4 * v63 + 50] = (v26 << 16) | v25;
          p_i_1 = p_i;
        }
        v67 = 4 * v66;
      }
    }
    else
    {
      v67 = 4 * v14;
      memset(buf,0,4 * v14);
      p_i_1 = p_i;
      Bufferb_1 = *(char **)(p_i + 12);
      Buffer_2 = *(uint16_t *)(p_i + 2);
    }
    buf = &Bufferc_1[v67 + 54];
  }
  else
  {
    Bufferb_1 = *(char **)(p_i + 12);
  }
  Size_2 = *(uint16_t *)(p_i_1 + 4);
  Bufferc = Bufferc_1;
  v65 = (char *)(buf - Bufferc_1);
  Stream_v = Stream_1;
  p_i_2 = (uint16_t *)p_i_1;
  v28 = v3;
  Bufferb_2 = (uint32_t)Bufferb_1;
  while ( 1 )
  {
    buf_1 = buf;
    v31 = (char *)p_i_2 + Bufferb_2 - Size_2 + 16;
    *(uint32_t *)(Bufferc + 10) = v65;
    if ( !v28 )
      break;
    n4 = p_i_2[5] & 0x3F;
    v33 = n4 == 4;
    if ( n4 != 4 )
    {
      if ( n4 != 8 )
        break;
      v33 = 0;
    }
    n2 = 2;
    if ( !v33 )
      n2 = 1;
    *(uint32_t *)(Bufferc + 30) = n2;
    v75 = n2 - 1;
    n2_2 = (0x100u >> ((n2 - 1) & 31)) - 1;
    if ( Buffer_2 > 0 )
    {
      Buffer_3 = Buffer_2;
      Size_3 = Size_2;
      Buffer_4 = 0;
      while ( 1 )
      {
        if ( v31 >= &v31[Size_3] )
          goto LABEL_72;
        buf_2 = buf_1;
        v77 = (uint32_t)&v31[Size_3];
        Buffer_5 = Buffer_4;
        Size = 0;
        do
        {
          while ( 1 )
          {
            while ( 1 )
            {
              v39 = v31 + 1;
              if ( (uint32_t)(v31 + 1) >= v77 )
                break;
              v40 = (uint8_t)*v31;
              n2_1 = 1;
              do
              {
                if ( v40 != (uint8_t)v31[n2_1] )
                  break;
                ++n2_1;
              }
              while ( v77 > (uint32_t)&v31[n2_1] );
              if ( n2_1 <= 2 && (n2_1 != 2 || Size) )
                break;
              if ( n2_2 < n2_1 )
                n2_1 = n2_2;
              if ( !Size )
                goto LABEL_70;
              if ( v75 )
              {
                if ( Size != 1 )
                {
LABEL_67:
                  *buf_2 = 0;
                  v72 = buf_2 + 2;
                  buf_2[1] = Size << (v75 & 31);
                  memcpy(buf_2 + 2,&v31[-Size],Size);
                  buf_2 += Size + 2;
                  if ( (Size & 1) != 0 )
                  {
                    v72[Size] = 0;
                    ++buf_2;
                  }
                  goto LABEL_69;
                }
                v47 = *(v31 - 1);
                *buf_2 = 2;
                buf_2[1] = v47;
                buf_2 += 2;
              }
              else
              {
                if ( Size >= 3 )
                  goto LABEL_67;
                if ( Size == 2 )
                {
                  v45 = *(v31 - 2);
                  *buf_2 = 1;
                  buf_2[1] = v45;
                  buf_2 += 2;
                }
                v46 = *(v31 - 1);
                *buf_2 = 1;
                buf_2[1] = v46;
                buf_2 += 2;
              }
LABEL_69:
              Size = 0;
              LOBYTE(v40) = *v31;
LABEL_70:
              buf_2[1] = v40;
              v31 += n2_1;
              *buf_2 = n2_1 << (v75 & 31);
              buf_3 = buf_2 + 2;
              buf_2 += 2;
              if ( (uint32_t)v31 >= v77 )
              {
                buf_1 = buf_3;
                Buffer_4 = Buffer_5;
                Buffer_3 = *(uint16_t *)(p_i + 2);
                Size_3 = *(uint16_t *)(p_i + 4);
                goto LABEL_72;
              }
            }
            ++v31;
            if ( ++Size != n2_2 )
              break;
            if ( v75 )
            {
              if ( Size == 1 )
              {
                v44 = *(v39 - 1);
                *buf_2 = 2;
                buf_2[1] = v44;
                buf_2 += 2;
                goto LABEL_55;
              }
            }
            else if ( Size < 3 )
            {
              if ( Size == 2 )
              {
                v42 = *(v39 - 2);
                *buf_2 = 1;
                buf_2[1] = v42;
                buf_2 += 2;
              }
              v43 = *(v39 - 1);
              *buf_2 = 1;
              buf_2[1] = v43;
              buf_2 += 2;
              goto LABEL_55;
            }
            *buf_2 = 0;
            v68 = buf_2 + 2;
            buf_2[1] = Size << (v75 & 31);
            memcpy(buf_2 + 2,&v39[-Size],Size);
            buf_2 += Size + 2;
            if ( (Size & 1) != 0 )
            {
              v68[Size] = 0;
              ++buf_2;
            }
LABEL_55:
            if ( (uint32_t)v39 >= v77 )
            {
              buf_1 = buf_2;
              Buffer_4 = Buffer_5;
              Buffer_3 = *(uint16_t *)(p_i + 2);
              Size_3 = *(uint16_t *)(p_i + 4);
              goto LABEL_72;
            }
            Size = 0;
          }
        }
        while ( (uint32_t)v39 < v77 );
        buf_1 = buf_2;
        Buffer_4 = Buffer_5;
        if ( !Size )
          goto LABEL_89;
        if ( v75 )
        {
          if ( Size == 1 )
          {
            buf_2[1] = *(v39 - 1);
            *buf_2 = 2;
            Buffer_3 = *(uint16_t *)(p_i + 2);
            Size_3 = *(uint16_t *)(p_i + 4);
            buf_1 = buf_2 + 2;
            goto LABEL_72;
          }
LABEL_97:
          *buf_2 = 0;
          buf_2[1] = Size << (v75 & 31);
          memcpy(buf_2 + 2,&v39[-Size],Size);
          Buffer_4 = Buffer_5;
          buf_1 = &buf_2[Size + 2];
          if ( (Size & 1) != 0 )
          {
            buf_2[Size + 2] = 0;
            Buffer_3 = *(uint16_t *)(p_i + 2);
            Size_3 = *(uint16_t *)(p_i + 4);
            ++buf_1;
            goto LABEL_72;
          }
LABEL_89:
          Buffer_3 = *(uint16_t *)(p_i + 2);
          Size_3 = *(uint16_t *)(p_i + 4);
          goto LABEL_72;
        }
        if ( Size >= 3 )
          goto LABEL_97;
        if ( Size == 2 )
        {
          v56 = *(v39 - 2);
          *buf_2 = 1;
          buf_2[1] = v56;
          buf_1 = buf_2 + 2;
        }
        buf_1[1] = *(v39 - 1);
        *buf_1 = 1;
        Buffer_3 = *(uint16_t *)(p_i + 2);
        Size_3 = *(uint16_t *)(p_i + 4);
        buf_1 += 2;
LABEL_72:
        *buf_1 = 0;
        buf_1[1] = 0;
        buf_1 += 2;
        ++Buffer_4;
        v31 -= 2 * Size_3;
        if ( Buffer_4 >= Buffer_3 )
        {
          Buffer_2 = Buffer_3;
          Size_2 = Size_3;
          p_i_2 = (uint16_t *)p_i;
          Bufferb_2 = *(uint32_t *)(p_i + 12);
          break;
        }
      }
    }
    *buf_1 = 0;
    buf_1[1] = 1;
    buf_1 += 2;
    v49 = buf_1 - buf;
    if ( Bufferb_2 > buf_1 - buf )
    {
      Bufferc_2 = Bufferc;
      Stream_2 = Stream_v;
      goto LABEL_76;
    }
    v28 = 0;
  }
  Stream_2 = Stream_v;
  Bufferc_2 = Bufferc;
  *(uint32_t *)(Bufferc + 30) = 0;
  if ( Buffer_2 <= 0 )
  {
    v49 = 0;
  }
  else
  {
    v53 = ((Size_2 + 3) & 0xFFFFFFFC) - Size_2;
    Size_1 = Size_2;
    v55 = 0;
    do
    {
      memcpy(buf_1,v31,Size_1);
      Size_1 = *(uint16_t *)(p_i + 4);
      buf_1 += Size_1;
      v31 -= Size_1;
      if ( v53 )
      {
        *(uint32_t *)buf_1 = 0;
        buf_1 += v53;
      }
      ++v55;
    }
    while ( v55 < *(uint16_t *)(p_i + 2) );
    Bufferc_2 = Bufferc;
    Stream_2 = Stream_v;
    v49 = buf_1 - buf;
  }
LABEL_76:
  *(uint32_t *)(Bufferc_2 + 34) = v49;
  ElementCount = buf_1 - Bufferc_2;
  *(uint32_t *)(Bufferc_2 + 2) = ElementCount;
  if ( fwrite(Bufferc_2, 1u, ElementCount, Stream_2) != *(uint32_t *)(Bufferc_2 + 2) )
    return 0;
  free(Bufferc_2);
  fclose(Stream_2);
  return 1;
}
uint32_t __sub_414860(int32_t *a1, int32_t a2, int32_t a3, int32_t a4)
{
  ;
  char *buf, v6;
  int32_t v7, v9, v10;
  uint32_t result;
  *a1 = a3;
  buf = (char *)malloc(3 * a3);
  a1[5] = (int32_t)buf;
  if ( a4 )
  {
    v7 = *a1;
    a1[2] = 0;
    a1[1] = v7;
    result = 12 * v7;
    a1[3] = 12 * v7;
    a1[4] = 8 * v7;
    if ( v7 )
    {
      result = 0;
      do
      {
        v9 = 3 * result;
        *(uint16_t *)(a1[5] + v9) = result++;
        *(uint8_t *)(a1[5] + v9 + 2) = 1;
      }
      while ( result < a1[1] );
    }
  }
  else
  {
    v10 = *a1;
    a1[2] = 2;
    a1[4] = 20 * v10;
    a1[1] = 0;
    a1[3] = 18 * v10;
    return (uint32_t)memset(buf,0,3 * v10);
  }
  return result;
}

char * __sub_4081E0(char *p_i, char *Src, int32_t a3, char a4)
{
  ;
  char *Src_5, *v25;
  int32_t v4, n6_2, v8, n6_1, v10, Size, n4, Size_2, v15, Size_1, v17, v18, v20, v21, v23, n2,
          v26, v27, v29, v30, v31, v32, n4_1, v34, n6;
  uint32_t v6, Src_1, Src_2;
  uint8_t *Src_4, *Src_3;
  if ( (__byte_44339E[16 * a3] & 8) == 0 )
  {
    Size = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
    n4 = __n4_5;
    Src_1 = (uint32_t)&p_i[a3 + 16];
    if ( __n4_5 == 1 )
      return memcpy(&p_i[a3 + 16],Src,Size);
    p_i += a3;
    if ( Size <= 6
      || __n4_5 <= 0
      || (Src_1 <= (uint32_t)Src || Size > Src_1 - (uint32_t)Src)
      && (__n4_5 > 1 || (uint32_t)Src <= Src_1 || (uint32_t)&Src[-Src_1] < Size * __n4_5) )
    {
      Size_1 = 0;
      v17 = 0;
      do
      {
        p_i[v17 + 16] = Src[Size_1];
        v17 += n4;
        ++Size_1;
      }
      while ( Size_1 < Size );
    }
    else
    {
      Size_2 = 0;
      v15 = 0;
      do
      {
        p_i[v15 + 16] = Src[Size_2];
        v15 += n4;
        ++Size_2;
      }
      while ( Size_2 < Size );
    }
    return p_i;
  }
  n4_1 = __n4_5;
  n6 = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
  v29 = (uint8_t)__byte_44339D[0] - a3;
  v31 = (uint8_t)__byte_4433AD[0] - a3;
  n2 = (uint8_t)__byte_44339C[16 * a3];
  v34 = (uint8_t)__byte_44339F[16 * a3];
  Src_2 = (uint32_t)&p_i[a3 + 16];
  v4 = __dword_4433A4[4 * a3];
  v32 = __dword_4433A0[4 * a3];
  v30 = __dword_4433A8[4 * a3];
  if ( n2 != 2 || v32 + v4 != 128 )
    goto LABEL_3;
  if ( !v4 )
    goto LABEL_4;
  if ( v32 )
  {
LABEL_3:
    if ( __byte_44339C[16 * a3] != 1 )
    {
      if ( n2 == 2 )
      {
        v21 = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
        v27 = v4;
        Src_3 = (uint8_t *)Src_2;
        do
        {
          v23 = v34 + (uint8_t)*Src++;
          p_i = (char *)(((v32 * (uint8_t)Src_3[v29] + v27 * (uint32_t)(uint8_t)Src_3[v31] + 40) >> 7)
                       + v23);
          *Src_3 = (uint8_t)p_i;
          Src_3 += n4_1;
          --v21;
        }
        while ( v21 );
      }
      else if ( n2 == 3 )
      {
        v18 = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
        v26 = v4;
        Src_4 = (uint8_t *)Src_2;
        do
        {
          v20 = v34 + (uint8_t)*Src++;
          p_i = (char *)(((v26 * (uint8_t)*(Src_4 - 2)
                         + v32 * (uint8_t)*(Src_4 - 3)
                         + v30 * (uint32_t)(uint8_t)*(Src_4 - 1)
                         + 63) >> 7)
                       + v20);
          *Src_4 = (uint8_t)p_i;
          Src_4 += n4_1;
          --v18;
        }
        while ( v18 );
      }
      return p_i;
    }
    goto LABEL_4;
  }
  v29 = (uint8_t)__byte_4433AD[0] - a3;
LABEL_4:
  p_i += a3;
  v25 = &p_i[v29];
  if ( n6 <= 6 )
    goto LABEL_25;
  Src_5 = &p_i[v29 + 16];
  if ( __n4_5 <= 0 )
  {
    if ( __n4_5 >= 0 )
      goto LABEL_25;
    if ( (uint32_t)Src_5 >= Src_2 || (v6 = n6 * __n4_5, Src_2 - (uint32_t)Src_5 <= -(n6 * __n4_5)) )
    {
      if ( (uint32_t)Src_5 <= Src_2 )
        goto LABEL_25;
      v6 = n6 * __n4_5;
      if ( (uint32_t)&Src_5[-Src_2] <= -(n6 * __n4_5) )
        goto LABEL_25;
    }
  }
  else if ( (uint32_t)Src_5 >= Src_2 || (v6 = n6 * __n4_5, n6 * __n4_5 > Src_2 - (uint32_t)Src_5) )
  {
    if ( (uint32_t)Src_5 <= Src_2 )
      goto LABEL_25;
    v6 = n6 * __n4_5;
    if ( (uint32_t)&Src_5[-Src_2] < n6 * __n4_5 )
      goto LABEL_25;
  }
  if ( __n4_5 <= 0
    || ((uint32_t)Src >= Src_2 || Src_2 - (uint32_t)Src < n6)
    && (__n4_5 > 1 || (uint32_t)Src <= Src_2 || (uint32_t)&Src[-Src_2] < v6) )
  {
LABEL_25:
    n6_1 = 0;
    v10 = 0;
    do
    {
      p_i[v10 + 16] = v25[v10 + 16] + v34 + Src[n6_1];
      v10 += n4_1;
      ++n6_1;
    }
    while ( n6_1 < n6 );
    return p_i;
  }
  n6_2 = 0;
  v8 = 0;
  do
  {
    p_i[v8 + 16] = v25[v8 + 16] + v34 + Src[n6_2];
    v8 += n4_1;
    ++n6_2;
  }
  while ( n6_2 < n6 );
  return p_i;
}

char * __sub_407EF0(char *Blockb, char *Src, int32_t a3, char a4)
{
  ;
  char v8, v11, *v29;
  int32_t v4, n6_2, v7, n6_1, v10, Size, n4, Size_1, v16, Size_2, v18, v19, v21, v22, v23, v24,
          v26, n2, v30, v31, v33, v34, v35, v36, v37, n4_1, n6;
  uint32_t Src_2, Src_1, v27, Src_3;
  uint8_t *Src_5, *Src_4;
  if ( (__byte_44339E[16 * a3] & 8) == 0 )
  {
    Size = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    n4 = __n4_5;
    Src_1 = (uint32_t)&Blockb[a3 + 16];
    if ( __n4_5 == 1 )
      return memcpy(Src,&Blockb[a3 + 16],Size);
    Blockb += a3;
    if ( Size > 6 && __n4_5 > 0 )
    {
      if ( __n4_5 > 1 )
        goto LABEL_30;
      if ( (uint32_t)Src > Src_1 && (uint32_t)&Src[-Src_1] >= Size * __n4_5 )
        goto LABEL_31;
      if ( __n4_5 > 0 )
      {
LABEL_30:
        if ( Src_1 > (uint32_t)Src && Size <= Src_1 - (uint32_t)Src )
        {
LABEL_31:
          Size_1 = 0;
          v16 = 0;
          do
          {
            Src[Size_1] = Blockb[v16 + 16];
            v16 += n4;
            ++Size_1;
          }
          while ( Size_1 < Size );
          return Blockb;
        }
      }
    }
    Size_2 = 0;
    v18 = 0;
    do
    {
      Src[Size_2] = Blockb[v18 + 16];
      v18 += n4;
      ++Size_2;
    }
    while ( Size_2 < Size );
    return Blockb;
  }
  n4_1 = __n4_5;
  n6 = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
  v33 = (uint8_t)__byte_44339D[0] - a3;
  v35 = (uint8_t)__byte_4433AD[0] - a3;
  n2 = (uint8_t)__byte_44339C[16 * a3];
  v37 = (uint8_t)__byte_44339F[16 * a3];
  Src_3 = (uint32_t)&Blockb[a3 + 16];
  v4 = __dword_4433A4[4 * a3];
  v36 = __dword_4433A0[4 * a3];
  v34 = __dword_4433A8[4 * a3];
  if ( n2 == 2 && v36 + v4 == 128 )
  {
    if ( !v4 )
      goto LABEL_4;
    if ( !v36 )
    {
      v33 = (uint8_t)__byte_4433AD[0] - a3;
      goto LABEL_4;
    }
  }
  if ( __byte_44339C[16 * a3] == 1 )
  {
LABEL_4:
    Blockb += a3;
    v29 = &Blockb[v33];
    if ( n6 <= 6
      || (Src_2 = (uint32_t)&Blockb[v33 + 16], __n4_5 <= 0)
      || (__n4_5 > 1 || (uint32_t)Src <= Src_2 || (uint32_t)&Src[-Src_2] < n6 * __n4_5)
      && (Src_2 <= (uint32_t)Src || Src_2 - (uint32_t)Src < n6)
      || (__n4_5 > 1 || (uint32_t)Src <= Src_3 || (uint32_t)&Src[-Src_3] < n6 * __n4_5)
      && ((uint32_t)Src >= Src_3 || Src_3 - (uint32_t)Src < n6) )
    {
      n6_1 = 0;
      v10 = 0;
      do
      {
        v11 = Blockb[v10 + 16] - v37 - v29[v10 + 16];
        v10 += n4_1;
        Src[n6_1++] = v11;
      }
      while ( n6_1 < n6 );
    }
    else
    {
      n6_2 = 0;
      v7 = 0;
      do
      {
        v8 = Blockb[v7 + 16] - v37 - v29[v7 + 16];
        v7 += n4_1;
        Src[n6_2++] = v8;
      }
      while ( n6_2 < n6 );
    }
    return Blockb;
  }
  if ( n2 == 2 )
  {
    v24 = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    v31 = v4;
    Src_4 = (uint8_t *)Src_3;
    do
    {
      v26 = *Src_4 - v37;
      v27 = v36 * Src_4[v33] + v31 * Src_4[v35] + 40;
      Src_4 += n4_1;
      Blockb = (char *)(v26 - (v27 >> 7));
      *Src++ = (char)Blockb;
      --v24;
    }
    while ( v24 );
  }
  else if ( n2 == 3 )
  {
    v19 = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    v30 = v4;
    Src_5 = (uint8_t *)Src_3;
    do
    {
      v21 = *Src_5 - v37;
      v22 = v30 * *(Src_5 - 2) + v36 * *(Src_5 - 3);
      v23 = v34 * *(Src_5 - 1);
      Src_5 += n4_1;
      Blockb = (char *)(v21 - ((uint32_t)(v22 + v23 + 63) >> 7));
      *Src++ = (char)Blockb;
      --v19;
    }
    while ( v19 );
  }
  return Blockb;
}

__attribute__((noreturn)) void __exit_402E40(int32_t Code, ...)
{
  ;
  va_list ap;
  va_start(ap, Code);
  vprintf((&__off_441068)[Code], ap);
  va_end(ap);
  printf("\n");
  exit(Code);
  __builtin_unreachable();
}
BMF_SSE int32_t __sub_4149C0(char ArgList_1)
{
  ;
  __m128i si128, *v13;
  char *v11, *v12, ArgList;
  int32_t __sub_4149C0_n8, n151, result, v7, v8, v16, v17, v20;
  uint16_t *n256, *__sub_4149C0_n256_1;
  uint32_t v5, i;
  uint64_t *v9;
  uint8_t *v1, *v4;
  v1 = (uint8_t *)__Buffer_0;
  if ( __Buffer_0 != __Buffer_1 )
  {
    __sub_4149C0_n8 = ::__n8 - 8;
    if ( ::__n8 - 8 < 0 )
    {
      ::__n8 -= 8;
    }
    else
    {
      do
      {
        --v1;
        __sub_4149C0_n8 -= 8;
      }
      while ( __sub_4149C0_n8 >= 0 );
      __Buffer_0 = (int32_t)v1;
      ::__n8 = __sub_4149C0_n8;
    }
  }
  if ( __dword_443364 )
  {
    if ( __n2 == 2 )
    {
      __n8_0 = 8;
      __n8_1 = 8;
    }
    else
    {
      __n8_1 = 16;
      __n8_0 = 64;
    }
    __n2_0 = 2;
    __dword_445660[0] = 33685760;
    __byte_44571E = 0;
    __byte_44571D = 1;
    __n2_1 = 2;
    __n4 = 4;
    __byte_445722 = 1;
    __dword_445660[1] = 50529027;
    __n4_0 = 4;
    v7 = 2 * (uint8_t)__n2_1 + 4;
    __byte_445724 = 2 * __n2_1 + 4;
    __byte_445726 = 2 * __n2_1;
    *(uint64_t *)(v7 + BMF_BLOB(0x00445660)) = 0x404040404040404LL;
    __n8_2 = 8;
    v8 = v7 + 2 * (uint8_t)__n4_0;
    __byte_445728 = v8;
    __byte_44572A = v8 - 5;
    *(uint64_t *)(v8 + BMF_BLOB(0x00445660)) = 0x505050505050505LL;
    *(uint64_t *)(v8 + BMF_BLOB(0x00445668)) = 0x505050505050505LL;
    __n16 = 16;
    v9 = (uint64_t *)(v8 + 2 * (uint8_t)__n8_2);
    __byte_44572C = (char)v9;
    __byte_44572E = (uint8_t)v9 - 6;
    *(uint64_t *)((char *)v9 + BMF_BLOB(0x00445660)) = 0x606060606060606LL;
    *(uint64_t *)((char *)v9 + BMF_BLOB(0x00445668)) = 0x606060606060606LL;
    *(uint64_t *)((char *)v9 + BMF_BLOB(0x00445670)) = 0x606060606060606LL;
    *(uint64_t *)((char *)v9 + BMF_BLOB(0x00445678)) = 0x606060606060606LL;
    si128 = _mm_load_si128((const __m128i *)&__xmmword_4397F0);
    __n32 = 32;
    v11 = (char *)v9 + 2 * (uint8_t)__n16;
    __byte_445730 = (char)v11;
    __byte_445732[0] = (uint8_t)v11 - 7;
    v12 = (char *)__dword_445660 + (uint32_t)v11;
    *(uint64_t *)v12 = si128.m128i_i64[0];
    v13 = (__m128i *)(((int32_t)&__dword_445660[3] + (uint32_t)v11 + 3) & 0xFFFFFFF0);
    *((uint32_t *)v12 + 2) = 117901063;
    *((uint16_t *)v12 + 6) = 1799;
    v12[14] = 7;
    *((uint64_t *)v12 + 6) = si128.m128i_i64[0];
    *((uint64_t *)v12 + 7) = si128.m128i_i64[0];
    *v13 = si128;
    v13[1] = si128;
    v13[2] = si128;
    n256 = malloc(0x7F000u);
    if ( n256 )
    {
      ArgList = ArgList_1;
      __sub_4149C0_n256_1 = n256;
      v16 = 0;
      v17 = 0;
      do
      {
        v20 = v16;
        __sub_4149C0_n256_1[1] = 24 * __n8_1;
        n256[v17 + 2] = 205;
        n256[v17 + 6] = 48;
        n256[v17 + 3] = 124;
        n256[v17 + 7] = 16;
        n256[v17 + 4] = 147;
        n256[v17 + 5] = 83;
        n256[v17 + 8] = 8;
        n256[v17 + 9] = 4;
        *__sub_4149C0_n256_1 = 635;
        for ( i = 0; i < 0x7A; ++i )
        {
          __sub_4149C0_n256_1[2 * i + 10] = 60;
          __sub_4149C0_n256_1[2 * i + 11] = 36;
        }
        __sub_4149C0_n256_1 += 254;
        v17 += 254;
        ++v16;
      }
      while ( (uint32_t)(v20 + 1) < 0x400 );
    }
    else
    {
      n256 = nullptr;
    }
    ::__n256_1 = n256;
    v1 = (uint8_t *)__Buffer_0;
  }
  rc.dec_init();
  return __Buffer_0;
}
void **__sub_402E70(void **Blockb, char a2)
{
  ;
  free(*(Blockb + 24));
  free(*(Blockb + 19));
  free(*(Blockb + 20));
  if ( (a2 & 1) != 0 )
    free(Blockb);
  return Blockb;
}

 BMF_SSE uint32_t *__sub_40AF40(uint32_t *_this, int32_t Src, int32_t i, int32_t n0x20000, int32_t a5)
{
  ;
  __m128i v26, v27, v28;
  bool v13;
  char v48;
  int32_t v6, v8, n256_3, __sub_40AF40_n8, n256_5, n256_1, v17, v19, v29, v30, v33, v34,
          __sub_40AF40_n256, n256_2, v43, v46, n256_4;
  uint32_t *this_1, v7, v10, v15, v16, *v18, k_3, *v21, k_1, k_2, v24, k, j, v32, k_4, v38, v40,
           v41, *v42, v45;
  this_1 = _this;
  *_this = Src + i;
  *(_this + 4) = 0;
  *(_this + 1) = Src + 2 * i + 1;
  if ( __dword_443364 )
  {
    *(_this + 2) = -i;
    *(_this + 3) = -2 * i;
    v6 = __n2 == 2;
    v7 = 2 * v6 + 8;
    v8 = 2 * v6 + 6;
    if ( n0x20000 < 0x20000 )
      v7 = v8;
    if ( a5 )
    {
      v46 = v7 - 1;
      v40 = (n0x20000 - i + v7 - 1) / v7;
      v30 = v40;
      v43 = 0;
      for ( j = 0; j < 0x80; ++j )
      {
        v32 = 2 * j;
        v33 = 16 * v43;
        *(_this + 2 * j + 5) = 16 * v43;
        v34 = v30 - *(uint32_t *)&__buf[8 * j];
        if ( v34 <= 0 && v43 < v46 )
        {
          v34 += v40;
          ++v43;
          if ( ::__n8 < 8 )
          {
            *(uint32_t *)__Buffer_1 = ::__n256 | (2 * (v32 << ((31 - ::__n8) & 31)));
            __Buffer_1 = __Buffer_0;
            __Buffer_0 += 4;
            __sub_40AF40_n256 = v32 >> (::__n8 & 31);
            ::__n8 += 24;
            ::__n256 = __sub_40AF40_n256;
          }
          else
          {
            ::__n256 |= v32 << (-(char)::__n8 & 31);
            ::__n8 -= 8;
          }
          v33 = 16 * v43;
        }
        *(_this + 2 * j + 6) = v33;
        v30 = v34 - *(uint32_t *)&__buf[8 * j + 4];
        if ( v30 <= 0 && v43 < v46 )
        {
          v30 += v40;
          ++v43;
          if ( ::__n8 < 8 )
          {
            *(uint32_t *)__Buffer_1 = ::__n256 | (2 * ((2 * j + 1) << ((31 - ::__n8) & 31)));
            __Buffer_1 = __Buffer_0;
            ::__n256 = (2 * j + 1) >> (::__n8 & 31);
            __Buffer_0 += 4;
            ::__n8 += 24;
          }
          else
          {
            ::__n256 |= (2 * j + 1) << (-(char)::__n8 & 31);
            ::__n8 -= 8;
          }
        }
      }
      return this_1;
    }
    n256_3 = 0;
    v45 = v7 - 1;
    v10 = 0;
    do
    {
      __sub_40AF40_n8 = ::__n8;
      n256_4 = n256_3;
      n256_5 = n256_3;
      v13 = ::__n8 - 8 < 0;
      v48 = ::__n8 - 8;
      ::__n8 -= 8;
      if ( v13 )
      {
        v38 = *(uint32_t *)__Buffer_0;
        __Buffer_0 += 4;
        ::__n8 = __sub_40AF40_n8 + 24;
        n256_1 = ::__n256 | (uint8_t)(v38 << (__sub_40AF40_n8 & 31));
        ::__n256 = v38 >> (-v48 & 31);
      }
      else
      {
        n256_1 = (uint8_t)::__n256;
        ::__n256 = (uint32_t)::__n256 >> 8;
      }
      if ( n256_3 <= n256_1 )
      {
        if ( (n256_1 - n256_3 + 1) / 2 )
        {
          v41 = v10;
          n256_2 = n256_1;
          v15 = (n256_1 - n256_3 + 1) / 2;
          v16 = 0;
          v17 = 16 * v10;
          v18 = _this + n256_3;
          do
          {
            v18[2 * v16 + 5] = v17;
            v18[2 * v16 + 6] = v17;
            n256_3 = n256_5 + 2 * v16++ + 2;
          }
          while ( v16 < v15 );
          v10 = v41;
          n256_1 = n256_2;
          v19 = 2 * v16 + 1;
        }
        else
        {
          v19 = 1;
        }
        if ( n256_1 - n256_4 + 1 > (uint32_t)(v19 - 1) )
        {
          n256_3 = v19 + n256_5;
          *(_this + v19 + n256_5 + 4) = 16 * v10;
        }
      }
      ++v10;
    }
    while ( v10 < v45 );
    this_1 = _this;
    if ( n256_3 >= 256 )
      return this_1;
    k_3 = 256 - n256_3;
    v21 = _this + n256_3;
    k_1 = ((uint8_t)v21 + 4) & 0xF;
    v42 = v21;
    if ( (((uint8_t)v21 + 4) & 0xF) != 0 )
    {
      if ( (((uint8_t)v21 + 4) & 3) != 0 )
        goto LABEL_38;
      k_1 = (16 - k_1) >> 2;
    }
    if ( (int32_t)k_3 >= (int32_t)(k_1 + 4) )
    {
      k_2 = k_3 - (((uint8_t)k_3 - (uint8_t)k_1) & 3);
      v24 = 16 * v10;
      if ( k_1 )
      {
        k_4 = k_3;
        for ( k = 0; k < k_1; ++k )
          v42[k + 5] = v24;
        k_3 = k_4;
        this_1 = _this;
      }
      v26 = _mm_cvtsi32_si128(v24);
      v27 = _mm_unpacklo_epi32(v26, v26);
      v28 = _mm_unpacklo_epi64(v27, v27);
      do
      {
        *(__m128i *)&v42[k_1 + 5] = v28;
        k_1 += 4;
      }
      while ( k_1 < k_2 );
      goto LABEL_27;
    }
LABEL_38:
    k_2 = 0;
LABEL_27:
    if ( k_2 < k_3 )
    {
      v29 = 16 * v10;
      do
        v42[k_2++ + 5] = v29;
      while ( k_2 < k_3 );
    }
    return this_1;
  }
  *(_this + 2) = -1;
  *(_this + 3) = -i;
  return _this;
}

 int32_t *__sub_40E4D0()
{
  ;
  int32_t v0, n4, *v2, *v3, v4, *result, v6, v7, v8;
  v0 = 1;
  if ( *(int32_t *)(__dword_44342C + 4) < 0 )
  {
    do
    {
      ++v0;
      n4 = 4 * v0;
      v2 = (int32_t *)(__dword_44342C + 4 * v0);
    }
    while ( *v2 < 0 );
  }
  else
  {
    n4 = 4;
    v2 = (int32_t *)(__dword_44342C + 4);
  }
  v3 = (int32_t *)__dword_443434;
  v4 = __dword_443438;
  *(uint32_t *)(__dword_443434 + 4) = v0;
  *v3 = (v4 - (int32_t)v3) >> 3;
  __dword_443438 = v4 + 8 * (1 << (v0 & 31));
  result = (int32_t *)(__dword_443438 - 8);
  v6 = (1 << (v0 & 31)) - 1;
  if ( v6 >= 0 )
  {
    __dword_44342C = (int32_t)v2;
    do
    {
      v7 = *v2;
      result[1] = 0;
      *result = v7;
      v8 = __dword_442EE0[v7];
      *v2 = v8;
      if ( v8 < 0 )
      {
        result -= 2;
        if ( --v6 >= 0 )
        {
          do
          {
            __dword_443434 = (int32_t)result;
            __sub_40E4D0();
            result = (int32_t *)(__dword_443434 - 8);
            --v6;
          }
          while ( v6 >= 0 );
          v2 = (int32_t *)__dword_44342C;
        }
      }
      result -= 2;
      --v6;
    }
    while ( v6 >= 0 );
  }
  __dword_443434 = (int32_t)v3;
  __dword_44342C = (int32_t)&v2[n4 / 0xFFFFFFFC];
  return result;
}

 int32_t *__sub_40DEB0()
{
  alignas(16) uint8_t __hexrays_frame[112];
  char * &v61 = *(char * *)(__hexrays_frame + 0);
  int32_t * &v62 = *(int32_t * *)(__hexrays_frame + 4);
  int32_t * &v63 = *(int32_t * *)(__hexrays_frame + 4);
  int32_t &v64 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v65 = *(int32_t *)(__hexrays_frame + 8);
  uint32_t * &v66 = *(uint32_t * *)(__hexrays_frame + 8);
  int32_t &v67 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v68 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v69 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v70 = *(int32_t *)(__hexrays_frame + 12);
  uint32_t &v71 = *(uint32_t *)(__hexrays_frame + 16);
  uint32_t &v72 = *(uint32_t *)(__hexrays_frame + 16);
  uint32_t &v73 = *(uint32_t *)(__hexrays_frame + 16);
  int32_t &v74 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &n11_1 = *(int32_t *)(__hexrays_frame + 24);
  uint32_t &v76 = *(uint32_t *)(__hexrays_frame + 28);
  uint32_t &k_3 = *(uint32_t *)(__hexrays_frame + 32);
  uint32_t * &v78 = *(uint32_t * *)(__hexrays_frame + 36);
  int32_t &v79 = *(int32_t *)(__hexrays_frame + 40);
  int32_t &n8_2 = *(int32_t *)(__hexrays_frame + 44);
  int32_t &v81 = *(int32_t *)(__hexrays_frame + 48);
  char * &v82 = *(char * *)(__hexrays_frame + 52);
  int32_t &v83 = *(int32_t *)(__hexrays_frame + 56);
  int32_t &n256_1 = *(int32_t *)(__hexrays_frame + 60);
  int32_t &v85 = *(int32_t *)(__hexrays_frame + 64);
  char &v86 = *(char *)(__hexrays_frame + 68);
  int32_t &v87 = *(int32_t *)(__hexrays_frame + 72);
  int32_t &v88 = *(int32_t *)(__hexrays_frame + 72);
  ;
  __m128i v5;
  bool v2;
  char v3, *v23;
  int32_t *v0, __sub_40DEB0_n8, v4, n8_1, v9, v10, n4, *v12, v13, *v14, v15, v16, v17, n11, v20,
          v22, v24, v25, v26, v27, v28, v29, *v30, *v31, k_1, k, v34, v35, v36, v38, v40, n8_3,
          v43, k_2, v45, v46, v47, k_4, v49, v50, v51, v52, v53, v54, v55, n256_2;
  uint32_t __sub_40DEB0_n256, i, v19, *v21, *v39, j, v56, v57, v59, v60;
  v0 = (int32_t *)malloc(0x8ED0u);
  if ( v0 )
  {
    __sub_40DEB0_n8 = ::__n8;
    v2 = ::__n8 - 18 < 0;
    v3 = ::__n8 - 18;
    ::__n8 -= 18;
    if ( v2 )
    {
      v60 = *(uint32_t *)__Buffer_0;
      __Buffer_0 += 4;
      v4 = ::__n256 | (v60 << (__sub_40DEB0_n8 & 31)) & 0x3FFFF;
      ::__n256 = v60 >> (-v3 & 31);
      ::__n8 = __sub_40DEB0_n8 + 14;
    }
    else
    {
      v4 = ::__n256 & 0x3FFFF;
      ::__n256 = (uint32_t)::__n256 >> 18;
    }
    if ( v4 )
    {
      n8_1 = ::__n8;
      __sub_40DEB0_n256 = ::__n256;
      v67 = v4;
      v61 = (char *)(v0 + 2);
      v62 = v0;
      v5 = -1;
      __xmmword_442EA0 = -1;
      __xmmword_442EB0 = -1;
      __xmmword_442EC0 = -1;
      __xmmword_442ED0 = -1;
      for ( i = 0; i < 0x15; ++i )
      {
        n8_1 -= 4;
        if ( n8_1 < 0 )
        {
          ::__n8 = n8_1;
          v59 = *(uint32_t *)__Buffer_0;
          __Buffer_0 += 4;
          v9 = __sub_40DEB0_n256 | (v59 << ((n8_1 + 4) & 31)) & 0xF;
          __sub_40DEB0_n256 = v59 >> (-(char)n8_1 & 31);
          n8_1 += 32;
        }
        else
        {
          v9 = __sub_40DEB0_n256 & 0xF;
          __sub_40DEB0_n256 >>= 4;
        }
        ::__n256 = __sub_40DEB0_n256;
        __dword_442EE0[i] = *((uint32_t *)&__xmmword_442EA0 + v9);
        *((uint32_t *)&__xmmword_442EA0 + v9) = i;
      }
      v10 = 1;
      v4 = v67;
      v0 = v62;
      ::__n8 = n8_1;
      __dword_443348 = (int32_t)v62;
      if ( SDWORD1(__xmmword_442EA0) < 0 )
      {
        do
        {
          ++v10;
          n4 = 4 * v10;
        }
        while ( *((int32_t *)&__xmmword_442EA0 + v10) < 0 );
      }
      else
      {
        n4 = 4;
      }
      v12 = (int32_t *)((char *)&__xmmword_442EA0 + n4);
      v62[1] = v10;
      *v62 = (v61 - (char *)v62) >> 3;
      v13 = 1 << (v10 & 31);
      __dword_443438 = (int32_t)&v62[2 * (1 << (v10 & 31)) + 2];
      v14 = &v62[2 * (1 << (v10 & 31))];
      v15 = v13 - 1;
      if ( v15 >= 0 )
      {
        __dword_44342C = (int32_t)v12;
        do
        {
          v16 = *v12;
          v14[1] = 0;
          *v14 = v16;
          v17 = __dword_442EE0[v16];
          *v12 = v17;
          if ( v17 < 0 )
          {
            v14 -= 2;
            if ( --v15 >= 0 )
            {
              do
              {
                __dword_443434 = (int32_t)v14;
                __sub_40E4D0();
                v14 = (int32_t *)(__dword_443434 - 8);
                --v15;
              }
              while ( v15 >= 0 );
              v12 = (int32_t *)__dword_44342C;
              v5 = -1;
            }
          }
          v14 -= 2;
          --v15;
        }
        while ( v15 >= 0 );
      }
    }
    else
    {
      __dword_443438 = (int32_t)(v0 + 2);
      v5 = -1;
    }
    v63 = v0;
    if ( __n2 == 2 )
      v4 >>= 11;
    n11 = 11;
    if ( __n2 != 2 )
      n11 = 0;
    v19 = 0;
    v86 = 0;
    n11_1 = n11;
    v76 = 18 - n11;
    do
    {
      v20 = __dword_438B00[v19 + n11_1];
      __xmmword_442EA0 = v5;
      __xmmword_442EB0 = v5;
      __xmmword_442EC0 = v5;
      __xmmword_442ED0 = v5;
      if ( (v4 & 1) != 0 )
      {
        v21 = (uint32_t *)__dword_443438;
        if ( v20 > 0 )
        {
          n8_2 = ::__n8;
          n256_1 = ::__n256;
          v85 = __dword_443348;
          v83 = __dword_438B00[v19 + n11_1];
          v82 = (char *)(&__off_4410C0[v19])[n11_1];
          v81 = *(uint32_t *)(__dword_443348 + 4);
          v38 = 0;
          v65 = __dword_443438;
          v73 = v19;
          v70 = v4;
          do
          {
            v39 = (uint32_t *)v85;
            v40 = v81;
            n8_3 = n8_2;
            v79 = v38;
            for ( j = n256_1; ; ::__n256 = j )
            {
              n8_3 -= v40;
              if ( n8_3 < 0 )
              {
                v78 = v39;
                ::__n8 = n8_3;
                v56 = *(uint32_t *)__Buffer_0;
                __Buffer_0 += 4;
                v43 = j | __dword_439360[v40] & (v56 << ((v40 + n8_3) & 31));
                j = v56 >> (-(char)n8_3 & 31);
                n8_3 += 32;
                v39 = v78;
              }
              else
              {
                v43 = j & __dword_439360[v40];
                j >>= (v40 & 31);
              }
              v39 += 2 * *v39 + 2 * v43;
              v40 = v39[1];
              if ( !v40 )
                break;
            }
            v49 = *v39;
            n256_1 = j;
            n8_2 = n8_3;
            if ( (int32_t)*v39 < 16 )
            {
              ::__n256 = j;
              v86 = v49;
              v50 = 1;
            }
            else
            {
              v50 = *(uint32_t *)&__byte_43945C[4 * v49];
              v51 = *(uint32_t *)&__byte_439470[4 * v49];
              if ( v51 )
              {
                n8_2 = n8_3 - v51;
                if ( n8_3 - v51 < 0 )
                {
                  v88 = *(uint32_t *)&__byte_43945C[4 * v49];
                  ::__n256 = j;
                  ::__n8 = n8_3 - v51;
                  v57 = *(uint32_t *)__Buffer_0;
                  __Buffer_0 += 4;
                  v52 = j | __dword_439360[v51] & (v57 << (n8_3 & 31));
                  n256_2 = v57 >> (-(char)n8_2 & 31);
                  n8_2 += 32;
                  n256_1 = n256_2;
                  ::__n256 = n256_2;
                  v50 = v88;
                }
                else
                {
                  v52 = j & __dword_439360[v51];
                  n256_1 = j >> (v51 & 31);
                  ::__n256 = j >> (v51 & 31);
                }
                v50 += v52;
              }
              else
              {
                ::__n256 = j;
              }
            }
            v87 = v50;
            v53 = 0;
            v54 = v79;
            do
            {
              v55 = (v82[v79 + v53] - v86) & 0xF;
              __dword_442EE0[v79 + v53++] = *((uint32_t *)&__xmmword_442EA0 + v55);
              *((uint32_t *)&__xmmword_442EA0 + v55) = v54++;
            }
            while ( v53 < v87 );
            v38 = v53 + v79;
          }
          while ( v53 + v79 < v83 );
          v21 = (uint32_t *)v65;
          v19 = v73;
          v4 = v70;
          ::__n8 = n8_2;
        }
      }
      else
      {
        v21 = (uint32_t *)__dword_443438;
        if ( v20 > 0 )
        {
          v22 = __dword_438B00[v19 + n11_1];
          v23 = (char *)(&__off_4410C0[v19])[n11_1];
          v64 = __dword_443438;
          v24 = 0;
          v71 = v19;
          v25 = -1;
          v68 = v4;
          do
          {
            ++v25;
            v26 = (uint8_t)v23[v24++];
            *((uint32_t *)&__xmmword_442ED0 + v24 + 3) = *((uint32_t *)&__xmmword_442EA0 + v26);
            *((uint32_t *)&__xmmword_442EA0 + v26) = v25;
          }
          while ( v24 < v22 );
          v21 = (uint32_t *)v64;
          v19 = v71;
          v4 = v68;
        }
      }
      v2 = SDWORD1(__xmmword_442EA0) < 0;
      __dword_443300[v19 + n11_1] = (int32_t)v21;
      if ( v2 )
      {
        v28 = 0;
        do
          ++v28;
        while ( *((int32_t *)&__xmmword_442EA0 + v28 + 1) < 0 );
        v27 = v28 + 1;
      }
      else
      {
        v27 = 1;
      }
      v21[1] = v27;
      *v21 = 1;
      v29 = 1 << (v27 & 31);
      v74 = 4 * v27;
      v30 = (int32_t *)(4 * v27 + BMF_BLOB(0x00442EA0));
      __dword_443438 = (int32_t)&v21[2 * (1 << (v27 & 31)) + 2];
      v31 = &v21[2 * (1 << (v27 & 31))];
      k_1 = v29 - 1;
      if ( k_1 >= 0 )
      {
        __dword_44342C = (int32_t)v30;
        v72 = v19;
        v69 = v4;
        for ( k = k_1; k >= 0; --k )
        {
          v34 = *v30;
          v35 = __dword_442EE0[*v30];
          v31[1] = 0;
          *v31 = v34;
          *v30 = v35;
          if ( v35 < 0 )
          {
            v31 -= 2;
            k_2 = k;
            v45 = --k;
            if ( k >= 0 )
            {
              __dword_443434 = (int32_t)v31;
              k_3 = k_2;
              v66 = v21;
              v46 = 0;
              v47 = 0;
              while ( 1 )
              {
                __sub_40E4D0();
                v31 = (int32_t *)(__dword_443434 - 8);
                k_4 = v46 + v45 - 1;
                --v46;
                if ( ++v47 >= k_3 )
                  break;
                __dword_443434 -= 8;
              }
              v21 = v66;
              k = k_4;
              v30 = (int32_t *)__dword_44342C;
              v5 = -1;
            }
          }
          v31 -= 2;
        }
        v19 = v72;
        v4 = v69;
      }
      v36 = (int32_t)&v30[v74 / 0xFFFFFFFC];
      v4 >>= 1;
      ++v19;
    }
    while ( v19 < v76 );
    __dword_44342C = v36;
    __dword_443434 = (int32_t)v21;
    return v63;
  }
  else
  {
    __dword_443434 = 0;
    return nullptr;
  }
}
static inline uint32_t * __fwd_sub_40CF80_sub_40AF40(void *a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4) { return __sub_40AF40((uint32_t *)a0, a1, a2, a3, a4); }

 BMF_SSE void __sub_40CF80(int32_t i, uint8_t *Src, int32_t n0x20000)
{
  alignas(16) uint8_t __hexrays_frame[1172];
  int32_t &v144 = *(int32_t *)(__hexrays_frame + 0);
  void * &v145 = *(void * *)(__hexrays_frame + 4);
  uint32_t (&v146)[2] = *(uint32_t (*)[2])(__hexrays_frame + 8);
  int32_t &v147 = *(int32_t *)(__hexrays_frame + 16);
  int32_t &v148 = *(int32_t *)(__hexrays_frame + 20);
  uint32_t (&v149)[256] = *(uint32_t (*)[256])(__hexrays_frame + 28);
  int32_t &v150 = *(int32_t *)(__hexrays_frame + 1052);
  char * &buf_3 = *(char * *)(__hexrays_frame + 1056);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 1060);
  int32_t &__sub_40CF80_n4_2 = *(int32_t *)(__hexrays_frame + 1064);
  int32_t &n2_2 = *(int32_t *)(__hexrays_frame + 1068);
  uint32_t &v155 = *(uint32_t *)(__hexrays_frame + 1072);
  int32_t &n2_1 = *(int32_t *)(__hexrays_frame + 1076);
  int32_t &n256_3 = *(int32_t *)(__hexrays_frame + 1080);
  int32_t &v158 = *(int32_t *)(__hexrays_frame + 1084);
  int32_t &v159 = *(int32_t *)(__hexrays_frame + 1088);
  int32_t &v160 = *(int32_t *)(__hexrays_frame + 1092);
  int32_t &v161 = *(int32_t *)(__hexrays_frame + 1096);
  int32_t &v162 = *(int32_t *)(__hexrays_frame + 1100);
  int32_t &v163 = *(int32_t *)(__hexrays_frame + 1104);
  int32_t &v164 = *(int32_t *)(__hexrays_frame + 1108);
  int32_t &v165 = *(int32_t *)(__hexrays_frame + 1112);
  void * &Block = *(void * *)(__hexrays_frame + 1116);
  int32_t &v167 = *(int32_t *)(__hexrays_frame + 1120);
  char * &v168 = *(char * *)(__hexrays_frame + 1124);
  char * &buf = *(char * *)(__hexrays_frame + 1128);
  int32_t &v170 = *(int32_t *)(__hexrays_frame + 1132);
  int32_t * &v171 = *(int32_t * *)(__hexrays_frame + 1136);
  ;
  bool v36, v136;
  char *buf_1, v11, *v15, v16, v24, *v28, v29, v34, v37, n256_1, *buf_2, *v62, *v66, *v68, *v75,
       *buf_4, *buf_5, *v99, v100, *buf_6, *v103, *buf_7, *v131, *v135;
  int16_t v67;
  int32_t i_1, n64, v5, v6, v7, v8, v9, n0x4000_1, j, n0x4000, __sub_40CF80_n256,
          __sub_40CF80_n8, n1008, n15, v30, v31, __sub_40CF80_m, *v33, n8_1, v40, v41, v42, v43,
          v44, v45, v46, v47, v48, v49, *v50, n8_2, v53, n8_3, v55, v57, v58, v59, v60, n8_4,
          v63, v64, v69, v70, v71, v72, v73, v74, v76, v78, n8_5, v80, Size, n8_6, v84, v85,
          n2_4, __sub_40CF80_n4_1, n8_7, *v92, v93, v95, v96, v98, __sub_40CF80_n2, v104, v105,
          v107, v108, v109, *m_2, *m_1, v112, v113, *v114, *n, v116, *v117, v119, v120, n2_5,
          __sub_40CF80_n2_3, *v123, v124, v125, v127, v128, v129, v130, v132, v134,
          *__sub_40CF80_p_n2, n2_6;
  uint32_t v19, v20, k, v26, v27, __sub_40CF80_n256_2, v56, *v77, v86, v87, v88, n256_4, v106,
           n256_5, v133, v137, v140, v141, v143;
  uint8_t v65;
  void *v25, *v142;
  if ( ::__n2 )
  {
    ::__n4_1 = 4;
    __n0xFFFFFF = -1;
  }
  else
  {
    ::__n4_1 = 3;
    __n0xFFFFFF = 0xFFFFFF;
  }
  i_1 = i;
  ::__n2_3 = 2;
  n64 = 0;
  v5 = 1;
  __n3 = 3;
  __dword_442DC4[0] = 0;
  v6 = -1;
  ::__n4_2 = 4;
  v145 = nullptr;
  __dword_4433F8 = 0;
  __n5 = 5;
  ::__p_n2 = (int32_t)&::__n2_3;
  while ( 1 )
  {
    v146[v5 - 1] = i_1 + *(&v144 + v5);
    v7 = v5 + v6;
    v8 = v5;
    if ( v5 >= 0 )
      break;
LABEL_12:
    --v6;
    ++v5;
  }
  while ( 1 )
  {
    v9 = v146[v8 - 1];
    __dword_442BC4[2 * n64] = 0;
    ::__m[2 * n64] = v7 + v9;
    if ( ++n64 == 64 )
      break;
    if ( v8 != v5 )
    {
      if ( v8 )
      {
        __dword_442BC4[2 * n64] = 0;
        ::__m[2 * n64++] = v9 + v6 + v8;
        if ( n64 == 64 )
          break;
      }
    }
    ++v7;
    if ( --v8 < 0 )
    {
      i_1 = i;
      goto LABEL_12;
    }
  }
  buf_1 = (char *)malloc(0x4000u);
  memset(buf_1,0,0x4000);
  n0x4000_1 = 0;
  for ( j = 0; j < 64; ++j )
  {
    n0x4000 = ::__m[2 * j];
    if ( n0x4000 <= 0 || n0x4000 >= 0x4000 || buf_1[n0x4000] )
    {
      do
        ++n0x4000_1;
      while ( buf_1[n0x4000_1] );
      ::__m[2 * j] = n0x4000_1;
      n0x4000 = n0x4000_1;
    }
    buf_1[n0x4000] = 1;
  }
  free(buf_1);
  __fwd_sub_40CF80_sub_40AF40(v146, (int32_t)Src, i, n0x20000, 0);
  v15 = (char *)malloc(0x20800u);
  if ( v15 )
  {
    __sub_40CF80_n256 = 256;
    __sub_40CF80_n8 = ::__n8;
    if ( __dword_443364 )
      __sub_40CF80_n256 = 4096;
    ::__n256_2[0] = __sub_40CF80_n256;
    ::__n8 -= 30;
    if ( __sub_40CF80_n8 - 30 < 0 )
    {
      v56 = *(uint32_t *)__Buffer_0;
      __Buffer_0 += 4;
      v19 = ::__n256 | (v56 << (__sub_40CF80_n8 & 31)) & 0x3FFFFFFF;
      ::__n256 = v56 >> ((30 - __sub_40CF80_n8) & 31);
      ::__n8 = __sub_40CF80_n8 + 2;
    }
    else
    {
      v19 = ::__n256 & 0x3FFFFFFF;
      ::__n256 = (uint32_t)::__n256 >> 30;
    }
    __dword_44340C[0] = v19 & 0x3F;
    __n24 = (v19 >> 6) & 0x3F;
    __n14 = (v19 >> 12) & 0x3F;
    __n9 = (v19 >> 18) & 0x3F;
    __n7 = HIBYTE(v19) & 0x3F;
    __dword_443420 = 0;
    v20 = (uint32_t)(v15 + 15) & 0xFFFFFFF0;
    *(uint64_t *)v15 = 0;
    *((uint32_t *)v15 + 2) = 0;
    *((uint16_t *)v15 + 6) = 0;
    v15[14] = 0;
    *((uint64_t *)v15 + 126) = 0;
    *((uint64_t *)v15 + 127) = 0;
    n1008 = 1008;
    do
    {
      *(__m128i *)(v20 + n1008 - 16) = 0;
      *(__m128i *)(v20 + n1008 - 32) = 0;
      *(__m128i *)(v20 + n1008 - 48) = 0;
      *(__m128i *)(v20 + n1008 - 64) = 0;
      *(__m128i *)(v20 + n1008 - 80) = 0;
      *(__m128i *)(v20 + n1008 - 96) = 0;
      *(__m128i *)(v20 + n1008 - 112) = 0;
      n1008 -= 112;
    }
    while ( n1008 );
    *((uint32_t *)v15 + 256) = 0;
    if ( __dword_443364 )
    {
      *((uint32_t *)v15 + 256) = 30;
      for ( k = 0; k < 7; ++k )
      {
        v15[4 * k + 1028] = __byte_439408[2 * k];
        v24 = __byte_439409[2 * k];
        v15[4 * k + 1029] = 2 * k;
        v15[4 * k + 1030] = v24;
        v15[4 * k + 1031] = 2 * k + 1;
      }
      v15[1056] = 1;
      n15 = 15;
      v15[1057] = 14;
    }
    else
    {
      n15 = 0;
    }
    v25 = (void *)(256 - n15);
    v26 = (uint32_t)(256 - n15) >> 1;
    if ( v26 )
    {
      v145 = (void *)(256 - n15);
      v27 = 0;
      v28 = &v15[2 * n15];
      do
      {
        v28[4 * v27 + 1028] = 0;
        v28[4 * v27 + 1030] = 0;
        v29 = n15 + 2 * v27;
        v28[4 * v27 + 1029] = v29;
        v28[4 * v27++ + 1031] = v29 + 1;
      }
      while ( v27 < v26 );
      v25 = v145;
      v30 = 2 * v27 + 1;
    }
    else
    {
      v30 = 1;
    }
    if ( (uint32_t)v25 > v30 - 1 )
    {
      v31 = v30 + n15;
      v15[2 * v31 + 1026] = 0;
      v15[2 * v31 + 1027] = v30 + n15 - 1;
    }
    for ( __sub_40CF80_m = 1; __sub_40CF80_m < 256; ++__sub_40CF80_m )
      memcpy(&v15[516 * __sub_40CF80_m + 1024],v15 + 1024,516);
  }
  else
  {
    v15 = nullptr;
  }
  v33 = __sub_40DEB0();
  n8_1 = ::__n8;
  v36 = ::__n8 - 8 < 0;
  v37 = ::__n8 - 8;
  ::__n8 -= 8;
  if ( v36 )
  {
    v145 = *(void **)__Buffer_0;
    __Buffer_0 += 4;
    n256_1 = ::__n256 | ((uint32_t)v145 << (n8_1 & 31));
    ::__n256 = (uint32_t)v145 >> (-v37 & 31);
    ::__n8 = n8_1 + 24;
  }
  else
  {
    n256_1 = ::__n256;
    ::__n256 = (uint32_t)::__n256 >> 8;
  }
  n4 = ::__n4_1;
  *Src = n256_1;
  v150 = __dword_44340C[0];
  buf_3 = &Src[n0x20000];
  v145 = v33;
  Block = v15;
  buf_2 = Src + 1;
  while ( 1 )
  {
    while ( 1 )
    {
      if ( __dword_443364 )
      {
        if ( (uint32_t)buf_2 <= v146[1] )
        {
          v47 = (uint8_t)*(buf_2 - 1);
          if ( (uint32_t)buf_2 <= v146[0] )
          {
            v46 = v47 & 1 | v149[v47] | (2 * (v47 & 1));
          }
          else
          {
            v48 = (uint8_t)buf_2[v147 - 1];
            v49 = (uint8_t)buf_2[v147 + 1];
            v46 = buf_2[v147] & 1
                | *(uint32_t *)((char *)v149 + ((v49 + (uint8_t)buf_2[v147] + v47 + v48) & 0xFFFFFFFC))
                | (2 * (v47 & 1));
            if ( ::__n2 == 1 )
            {
              v58 = v46 < v149[v47] || v46 < v149[v49] || v46 < v149[v48];
              v46 |= (v58 << 7) | (8 * (v48 & 1)) | (4 * (v49 & 1));
            }
          }
        }
        else
        {
          v40 = (uint8_t)*(buf_2 - 1);
          v41 = (uint8_t)buf_2[v147 - 1];
          v42 = (uint8_t)buf_2[v147 + 1];
          v43 = (uint8_t)*(buf_2 - 2);
          v163 = (uint8_t)buf_2[v147];
          v162 = v40;
          v160 = v41;
          v161 = v42;
          v44 = (uint8_t)*(buf_2 - 3);
          v45 = (uint8_t)*(buf_2 - 4);
          v164 = (uint8_t)buf_2[v148]
               + (uint8_t)buf_2[v147 + 2]
               + (uint8_t)buf_2[v148 + 1]
               + v43;
          v165 = v163 + v40;
          v46 = v163 & 1
              | v149[((uint8_t)buf_2[v148 + 3]
                    + (uint8_t)buf_2[v148 + 2]
                    + (uint8_t)buf_2[v148 - 2]
                    + (uint8_t)buf_2[v147 - 3]
                    + (uint8_t)buf_2[v147 + 4]
                    + (uint8_t)buf_2[v148 - 1]
                    + (uint8_t)buf_2[v147 + 3]
                    + (uint8_t)buf_2[v147 - 2]
                    + 3 * (v42 + v160)
                    + v45
                    + v44
                    + 4 * (v163 + v40)
                    + 2 * v164) >> 5]
              | (2 * (v40 & 1));
          if ( ::__n2 == 1 )
          {
            v57 = v46 < v149[v162] || v46 < v149[v161] || v46 < v149[v160];
            v46 |= (v57 << 7) | (8 * (v160 & 1)) | (4 * (v161 & 1));
          }
        }
      }
      else
      {
        v46 = (uint8_t)*(buf_2 - 1);
      }
      v50 = (int32_t *)__dword_443330[*((uint32_t *)Block + v46)];
      __sub_40CF80_n256_2 = ::__n256;
      buf = buf_2;
      v167 = v46;
      n8_2 = ::__n8;
      v53 = v50[1];
      v168 = (char *)Block + 516 * v46 + 1024;
      _mm_prefetch(v168, 1);
      n8_3 = n8_2;
      while ( 1 )
      {
        n8_3 -= v53;
        if ( n8_3 < 0 )
        {
          ::__n8 = n8_3;
          v143 = *(uint32_t *)__Buffer_0;
          __Buffer_0 += 4;
          v55 = __sub_40CF80_n256_2 | __dword_439360[v53] & (v143 << ((v53 + n8_3) & 31));
          __sub_40CF80_n256_2 = v143 >> (-(char)n8_3 & 31);
          n8_3 += 32;
        }
        else
        {
          v55 = __sub_40CF80_n256_2 & __dword_439360[v53];
          __sub_40CF80_n256_2 >>= (v53 & 31);
        }
        v50 += 2 * *v50 + 2 * v55;
        v53 = v50[1];
        if ( !v53 )
          break;
        ::__n256 = __sub_40CF80_n256_2;
      }
      buf_2 = buf;
      v59 = v167;
      v60 = *v50;
      n8_4 = n8_3;
      v62 = v168;
      if ( !*v50 )
        break;
      ::__n256 = __sub_40CF80_n256_2;
      ::__n8 = n8_4;
      v63 = v60 - 1;
      v64 = 2 * v60 - 2;
      *buf = v168[v64 + 5];
      ++buf_2;
      v65 = v62[v64 + 4] + 1;
      v62[v64 + 4] = v65;
      if ( v63 && (uint8_t)v62[v64 + 2] <= (int32_t)v65 )
      {
        v66 = &v62[v64 + 4];
        v67 = *(uint16_t *)v66;
        *(uint16_t *)v66 = *(uint16_t *)&v62[v64 + 2];
        v68 = &v62[v64 + 2];
        if ( v68 != v62 + 4 )
        {
          v167 = v59;
          do
          {
            if ( (uint8_t)*(v68 - 2) > (int32_t)(uint8_t)v67 )
              break;
            *(uint16_t *)v68 = *((uint16_t *)v68 - 1);
            v68 -= 2;
          }
          while ( v68 != v62 + 4 );
          v59 = v167;
        }
        *(uint16_t *)v68 = v67;
      }
      v69 = *(uint32_t *)v62;
      v70 = *(uint32_t *)v62 + 1;
      *(uint32_t *)v62 = v70;
      if ( (v70 & 0x1F) == 0 && (uint8_t)v62[4] >= 8u )
      {
        v71 = (8 * v69 + 8 - v70) >> 3;
        v72 = 0;
        do
        {
          v73 = (uint8_t)v62[2 * v72++ + 4];
          v71 -= v73;
        }
        while ( v71 >= 0 );
        if ( v72 >= v150 )
        {
          *((uint32_t *)Block + v59) = 0;
        }
        else
        {
          v74 = 0;
          do
            ++v74;
          while ( v72 < __dword_44340C[v74] );
          *((uint32_t *)Block + v59) = v74;
        }
        if ( (uint8_t)v62[4] >= 0xE0u || *(uint32_t *)v62 >= ::__n256_2[0] )
        {
          *(uint32_t *)v62 = 0;
          v75 = v62 + 4;
          do
          {
            if ( !*v75 )
              break;
            v76 = (uint8_t)*v75 >> 1;
            *v75 = v76;
            *(uint32_t *)v62 += v76;
            v75 += 2;
          }
          while ( v75 != v62 + 516 );
        }
      }
    }
    if ( buf >= buf_3 )
      break;
    if ( ::__n2 == 2 )
    {
      v77 = (uint32_t *)__dword_44332C;
      v78 = *(uint32_t *)(__dword_44332C + 4);
      n8_5 = n8_4;
      do
      {
        n8_5 -= v78;
        if ( n8_5 < 0 )
        {
          ::__n256 = __sub_40CF80_n256_2;
          ::__n8 = n8_5;
          v87 = *(uint32_t *)__Buffer_0;
          __Buffer_0 += 4;
          v80 = __sub_40CF80_n256_2 | __dword_439360[v78] & (v87 << ((n8_5 + v78) & 31));
          __sub_40CF80_n256_2 = v87 >> (-(char)n8_5 & 31);
          n8_5 += 32;
        }
        else
        {
          v80 = __sub_40CF80_n256_2 & __dword_439360[v78];
          __sub_40CF80_n256_2 >>= (v78 & 31);
        }
        v77 += 2 * *v77 + 2 * v80;
        v78 = v77[1];
      }
      while ( v78 );
      buf_4 = buf;
      Size = __dword_438920[*v77];
      n8_6 = n8_5;
      v84 = __dword_439300[*v77];
      if ( v84 )
      {
        ::__n8 = n8_6 - v84;
        if ( n8_6 - v84 < 0 )
        {
          ::__n256 = __sub_40CF80_n256_2;
          v86 = *(uint32_t *)__Buffer_0;
          __Buffer_0 += 4;
          v85 = __sub_40CF80_n256_2 | __dword_439360[v84] & (v86 << (n8_6 & 31));
          ::__n256 = v86 >> ((v84 - n8_6) & 31);
          buf_4 = buf;
          ::__n8 = n8_6 - v84 + 32;
        }
        else
        {
          v85 = __sub_40CF80_n256_2 & __dword_439360[v84];
          ::__n256 = __sub_40CF80_n256_2 >> (v84 & 31);
        }
        Size += v85;
      }
      else
      {
        ::__n256 = __sub_40CF80_n256_2;
        ::__n8 = n8_6;
      }
      memset(buf_4,0,Size);
      buf_2 = &buf_4[Size];
    }
    else
    {
      ::__n256 = __sub_40CF80_n256_2;
      n256_3 = __sub_40CF80_n256_2;
      v88 = 0;
      n2_1 = 1;
      n2_4 = 1;
      __sub_40CF80_n4_1 = 4;
      n8_7 = n8_4;
      while ( 1 )
      {
        v92 = *(int32_t **)((char *)__dword_443310 + __sub_40CF80_n4_1);
        v93 = v92[1];
        buf = buf_2;
        n256_4 = n256_3;
        v155 = v88;
        __sub_40CF80_n4_2 = __sub_40CF80_n4_1;
        n2_2 = n2_4;
        v95 = v93;
        do
        {
          n8_7 -= v95;
          if ( n8_7 < 0 )
          {
            ::__n8 = n8_7;
            v141 = *(uint32_t *)__Buffer_0;
            __Buffer_0 += 4;
            v96 = n256_4 | __dword_439360[v95] & (v141 << ((n8_7 + v95) & 31));
            n256_4 = v141 >> (-(char)n8_7 & 31);
            ::__n256 = n256_4;
            n8_7 += 32;
          }
          else
          {
            v96 = n256_4 & __dword_439360[v95];
            n256_4 >>= (v95 & 31);
            ::__n256 = n256_4;
          }
          v92 += 2 * *v92 + 2 * v96;
          v95 = v92[1];
        }
        while ( v95 );
        buf_5 = buf;
        ::__n8 = n8_7;
        v98 = *v92;
        v159 = __dword_438980[*(int32_t *)((char *)__dword_439360 + __sub_40CF80_n4_2) & *v92];
        v155 |= v98 >> (n2_2 & 31);
        v99 = &buf[-::__m[2 * v155]];
        v100 = *v99;
        v158 = 8 * v155;
        __sub_40CF80_n2 = v159 - 4;
        *buf = v100;
        buf_5[1] = v99[1];
        buf_5[2] = v99[2];
        buf_5[3] = v99[3];
        buf_6 = buf_5 + 4;
        v103 = v99 + 4;
        while ( 1 )
        {
          *buf_6++ = *v103;
          if ( __sub_40CF80_n2 == 1 )
            break;
          *buf_6++ = v103[1];
          if ( __sub_40CF80_n2 == 2 )
            break;
          *buf_6++ = v103[2];
          if ( __sub_40CF80_n2 == 3 )
            break;
          *buf_6++ = v103[3];
          v103 += 4;
          __sub_40CF80_n2 -= 4;
          if ( !__sub_40CF80_n2 )
          {
            v104 = v159;
            v105 = v158;
            v106 = v155;
            goto LABEL_123;
          }
        }
        v104 = v159;
        v105 = v158;
        v106 = v155;
LABEL_123:
        if ( v104 >= __dword_439480[n2_1] && n2_1 < 5 )
        {
          n2_1 = *(uint32_t *)::__p_n2;
          ::__p_n2 += 4;
        }
        v107 = v104 + *(int32_t *)((char *)__dword_442BC4 + v105);
        *(int32_t *)((char *)__dword_442BC4 + v105) = v107;
        if ( v106 && v107 > *(int32_t *)((char *)&__dword_442BBC + v105) )
        {
          v108 = ::__m[2 * v106];
          v109 = ::__m[2 * v106 + 1];
          m_2 = &::__m[2 * v106 - 2];
          ::__m[2 * v106] = *m_2;
          ::__m[2 * v106 + 1] = ::__m[2 * v106 - 1];
          if ( m_2 != ::__m )
          {
            buf = buf_6;
            do
            {
              m_1 = m_2 - 2;
              v112 = *(m_2 - 1);
              if ( v112 >= v109 )
                break;
              *m_2 = *m_1;
              m_2[1] = v112;
              m_2 -= 2;
            }
            while ( m_1 != ::__m );
            buf_6 = buf;
          }
          *m_2 = v108;
          m_2[1] = v109;
          if ( __dword_442BFC[0] - __dword_442C04 > 20 )
          {
            v113 = __dword_442BC4[0];
            v114 = __dword_442BC8;
            for ( n = ::__m; v113; v114 += 2 )
            {
              n[1] = v113 >> 1;
              v113 = v114[1];
              n += 2;
            }
          }
        }
        n2_2 = n2_1;
        v116 = __dword_4432FC[n2_1];
        v170 = *(uint32_t *)(v116 + 4);
        v171 = (int32_t *)v116;
        __sub_40CF80_n4_2 = 4 * n2_1;
        while ( 1 )
        {
          v117 = v171;
          n8_7 = ::__n8;
          n256_5 = ::__n256;
          v119 = v170;
          buf = buf_6;
          while ( 1 )
          {
            n8_7 -= v119;
            if ( n8_7 < 0 )
            {
              ::__n8 = n8_7;
              v140 = *(uint32_t *)__Buffer_0;
              __Buffer_0 += 4;
              v120 = n256_5 | __dword_439360[v119] & (v140 << ((v119 + n8_7) & 31));
              n256_5 = v140 >> (-(char)n8_7 & 31);
              n8_7 += 32;
            }
            else
            {
              v120 = n256_5 & __dword_439360[v119];
              n256_5 >>= (v119 & 31);
            }
            v117 += 2 * *v117 + 2 * v120;
            v119 = v117[1];
            if ( !v119 )
              break;
            ::__n256 = n256_5;
          }
          buf_2 = buf;
          n2_5 = *v117;
          __sub_40CF80_n2_3 = *v117 - 1;
          if ( __sub_40CF80_n2_3 )
            break;
          v123 = (int32_t *)__dword_443328;
          v124 = *(uint32_t *)(__dword_443328 + 4);
          do
          {
            n8_7 -= v124;
            if ( n8_7 < 0 )
            {
              ::__n256 = n256_5;
              ::__n8 = n8_7;
              v137 = *(uint32_t *)__Buffer_0;
              __Buffer_0 += 4;
              v125 = n256_5 | __dword_439360[v124] & (v137 << ((n8_7 + v124) & 31));
              n256_5 = v137 >> (-(char)n8_7 & 31);
              n8_7 += 32;
            }
            else
            {
              v125 = n256_5 & __dword_439360[v124];
              n256_5 >>= (v124 & 31);
            }
            v123 += 2 * *v123 + 2 * v125;
            v124 = v123[1];
          }
          while ( v124 );
          buf_7 = buf;
          v127 = *v123;
          if ( *v123 < 24 )
          {
            v134 = ::__m[2 * v127];
            ::__n256 = n256_5;
            ::__n8 = n8_7;
            v135 = &buf[-v134];
            v136 = n4 == 4;
            *buf = *v135;
            buf_7[1] = v135[1];
            buf_7[2] = v135[2];
            if ( v136 )
              buf_7[3] = v135[3];
            buf_6 = &buf_7[n4];
          }
          else
          {
            ::__n8 = n8_7 - 11;
            if ( n8_7 - 11 < 0 )
            {
              ::__n256 = n256_5;
              v133 = *(uint32_t *)__Buffer_0;
              __Buffer_0 += 4;
              ::__n256 = v133 >> ((11 - n8_7) & 31);
              v128 = n256_5 | (v133 << (n8_7 & 31)) & 0x7FF;
              ::__n8 = n8_7 + 21;
            }
            else
            {
              v128 = n256_5 & 0x7FF;
              ::__n256 = n256_5 >> 11;
            }
            v129 = __dword_4433F8;
            v130 = (v127 << 11) + v128 - 49152;
            __dword_442DB8[-2 * (__dword_4433F8 & 3)] = v130;
            v131 = &buf_7[-v130];
            v132 = *(uint32_t *)v131;
            __dword_4433F8 = v129 + 1;
            *(uint32_t *)buf_7 = v132;
            *((uint32_t *)buf_7 + 1) = *((uint32_t *)v131 + 1);
            *((uint32_t *)buf_7 + 2) = *((uint32_t *)v131 + 2);
            buf_6 = buf_7 + 12;
          }
        }
        n256_3 = n256_5;
        __sub_40CF80_n4_1 = __sub_40CF80_n4_2;
        n2_1 = __sub_40CF80_n2_3;
        v36 = __sub_40CF80_n2_3 < 0;
        n2_4 = n2_2;
        ::__n256 = n256_3;
        if ( v36 )
          break;
        if ( n2_2 == n2_1 )
        {
          v88 = 0;
        }
        else
        {
          ::__n8 = n8_7;
          __sub_40CF80_p_n2 = (int32_t *)(::__p_n2 - 4);
          *__sub_40CF80_p_n2 = n2_2;
          ::__p_n2 = (int32_t)__sub_40CF80_p_n2;
          n8_7 = ::__n8;
          n2_4 = n2_5;
          n2_6 = n2_5;
          n256_3 = ::__n256;
          v88 = 0x80u >> (n2_5 & 31);
          __sub_40CF80_n4_1 = 4 * n2_6;
        }
      }
      ::__n8 = n8_7;
      if ( n2_2 != 1 )
      {
        ::__p_n2 -= 4;
        *(uint32_t *)::__p_n2 = n2_2;
      }
    }
  }
  v142 = v145;
  ::__n256 = __sub_40CF80_n256_2;
  ::__n8 = n8_4;
  free(Block);
  free(v142);
}

 BMF_SSE uint32_t *__sub_40E590(uint32_t *_this, int32_t i, char a3)
{
  ;
  __m128i si128, v21, v22, v23, v24, v28, v30, v31, v32, v33;
  int32_t v4, v5, v6, n3, v10, v11, v12, v15, v18, j, v36;
  uint32_t n0x200, *v9, *v13, n0x400_2, n0x400_4, n0x400, n0x400_3, n0x400_5, n0x400_1, *this_1;
  void *v14;
  v4 = 0;
  do
  {
    v36 = v4;
    this_1 = _this;
    v5 = 1040 * v4;
    *(uint32_t *)((char *)_this + v5 + 108) = __dword_4394E0[4 * v4];
    *(uint32_t *)((char *)_this + v5 + 112) = __dword_4394E4[4 * v4];
    v6 = __dword_4394EC[4 * v4];
    *(uint32_t *)((char *)_this + v5 + 116) = __dword_4394E8[4 * v4];
    *(uint32_t *)((char *)_this + v5 + 120) = v6;
    n3 = 0;
    n0x200 = 0;
    v9 = _this + 260 * v4;
    do
    {
      v10 = n3 <= 3 && 2 * n0x200 == v9[n3 + 27];
      v11 = n3 + v10;
      *((uint8_t *)v9 + 2 * n0x200 + 124) = n3 + v10 + 1;
      v12 = n3 + v10 <= 3 && 2 * n0x200 + 1 == v9[v11 + 27];
      n3 = v11 + v12;
      *((uint8_t *)v9 + 2 * n0x200++ + 125) = v11 + v12 + 1;
    }
    while ( n0x200 < 0x200 );
    _this = this_1;
    v4 = v36 + 1;
  }
  while ( (uint32_t)(v36 + 1) < 4 );
  v13 = malloc(8 * i + 8);
  this_1[24] = v13;
  v13 += 2;
  this_1[23] = v13;
  this_1[22] = 0;
  this_1[21] = 0;
  *(v13 - 2) = 0;
  *(v13 - 1) = this_1[22];
  this_1[19] = malloc(i + 2);
  v14 = malloc(i + 2);
  v15 = this_1[19];
  this_1[20] = v14;
  this_1[17] = v15 + 2;
  *(uint8_t *)(v15 + 1) = a3;
  *(uint8_t *)this_1[19] = a3;
  this_1[9] = -384;
  this_1[15] = -384;
  this_1[2] = -384;
  this_1[8] = -384;
  n0x400_2 = (uint8_t)this_1 & 0xF;
  if ( ((uint8_t)this_1 & 0xF) != 0 )
  {
    if ( ((uint8_t)this_1 & 3) != 0 )
    {
      n0x400 = 0;
      goto LABEL_17;
    }
    n0x400_2 = (16 - n0x400_2) >> 2;
    n0x400_4 = 0;
    v18 = 0;
    do
    {
      this_1[n0x400_4 + 1068] = v18 + 1023;
      --v18;
      ++n0x400_4;
    }
    while ( n0x400_4 < n0x400_2 );
  }
  si128 = _mm_load_si128((const __m128i *)&__xmmword_439520);
  n0x400 = 1024 - (-(char)n0x400_2 & 0xF);
  v21 = _mm_unpacklo_epi64(
          _mm_unpacklo_epi32(_mm_cvtsi32_si128(1023 - n0x400_2), _mm_cvtsi32_si128(1022 - n0x400_2)),
          _mm_unpacklo_epi32(_mm_cvtsi32_si128(1021 - n0x400_2), _mm_cvtsi32_si128(1020 - n0x400_2)));
  do
  {
    *(__m128i *)&this_1[n0x400_2 + 1068] = v21;
    v22 = _mm_add_epi32(v21, si128);
    *(__m128i *)&this_1[n0x400_2 + 1072] = v22;
    v23 = _mm_add_epi32(v22, si128);
    *(__m128i *)&this_1[n0x400_2 + 1076] = v23;
    v24 = _mm_add_epi32(v23, si128);
    *(__m128i *)&this_1[n0x400_2 + 1080] = v24;
    v21 = _mm_add_epi32(v24, si128);
    n0x400_2 += 16;
  }
  while ( n0x400_2 < n0x400 );
LABEL_17:
  for ( j = -n0x400; n0x400 < 0x400; ++n0x400 )
  {
    this_1[n0x400 + 1068] = j + 1023;
    --j;
  }
  n0x400_3 = ((uint8_t)this_1 + 12) & 0xF;
  if ( (((uint8_t)this_1 + 12) & 0xF) != 0 )
  {
    if ( (((uint8_t)this_1 + 12) & 3) != 0 )
    {
      n0x400_1 = 0;
      goto LABEL_25;
    }
    n0x400_3 = (16 - n0x400_3) >> 2;
    n0x400_5 = 0;
    do
    {
      this_1[n0x400_5 + 2091] = n0x400_5;
      ++n0x400_5;
    }
    while ( n0x400_5 < n0x400_3 );
  }
  v28 = _mm_load_si128((const __m128i *)&__xmmword_439530);
  n0x400_1 = 1024 - (-n0x400_3 & 0xF);
  v30 = _mm_unpacklo_epi64(
          _mm_unpacklo_epi32(_mm_cvtsi32_si128(n0x400_3), _mm_cvtsi32_si128(n0x400_3 + 1)),
          _mm_unpacklo_epi32(_mm_cvtsi32_si128(n0x400_3 + 2), _mm_cvtsi32_si128(n0x400_3 + 3)));
  do
  {
    *(__m128i *)&this_1[n0x400_3 + 2091] = v30;
    v31 = _mm_add_epi32(v30, v28);
    *(__m128i *)&this_1[n0x400_3 + 2095] = v31;
    v32 = _mm_add_epi32(v31, v28);
    *(__m128i *)&this_1[n0x400_3 + 2099] = v32;
    v33 = _mm_add_epi32(v32, v28);
    *(__m128i *)&this_1[n0x400_3 + 2103] = v33;
    v30 = _mm_add_epi32(v33, v28);
    n0x400_3 += 16;
  }
  while ( n0x400_3 < n0x400_1 );
LABEL_25:
  while ( n0x400_1 < 0x400 )
  {
    this_1[n0x400_1 + 2091] = n0x400_1;
    ++n0x400_1;
  }
  return this_1;
}

BMF_SSE char * __sub_40FAC0(int32_t *Blockb, char a2, int32_t Src, int32_t i, int32_t a5)
{
  ;
  __m128i si128, v10, v11, v12, v14, v15, v22, v23, v25, v26, v27, v28, v29, v30, v31;
  char v19, v20, v21, v34, v35, v39, v46, *result, v55, *v98, v119, *v127;
  int32_t Src_1, n256, n128_1, n128, v18, n256_1, v33, v40, v41, v42, v43, v45, v47, v48, v49,
          v50, v52, v53, v54, v56, v58, v59, v61, v62, v63, v65, v66, v67, v68, v70, v71, v72,
          v73, v74, v75, v76, v77, v79, v80, v81, v82, v83, v84, v85, v86, v87, v88, v89, v90,
          v91, v92, v93, v94, v95, v96, v97, v99, v100, v101, v102, v104, v105, v106, n6, v108,
          v109, v110, v111, v112, n128_2, v114, v115, v116, v117, *v120, *v121, *v122, *v123,
          v124, v125, Blockb_1, v128, v130, v132, v133, v134, v135;
  uint32_t n0x70, j, k, v36, m_1, m, *v57, *v69, *v78, *v131;
  uint8_t *v44, *v60, *v64, *v103, *v129;
  alignas(16) uint8_t v118[255];
  Src_1 = Src;
  n256 = __n256_0[0];
  v127 = (char *)(Src + 1);
  n128_1 = 2 * __n256_0[0] + 1;
  if ( __n2 )
  {
    si128 = _mm_load_si128((const __m128i *)&__xmmword_4395F0);
    v10 = _mm_load_si128((const __m128i *)&__xmmword_4395D0);
    v11 = _mm_load_si128((const __m128i *)&__xmmword_439600);
    v12 = _mm_load_si128((const __m128i *)&__xmmword_439610);
    n0x70 = 0;
    v118[0] = 0;
    v119 = 0x80;
    do
    {
      v14 = v11;
      v15 = v11;
      v11 = _mm_add_epi8(v11, si128);
      *(__m128i *)&v118[2 * n0x70 + 1] = _mm_unpacklo_epi8(v14, v12);
      *(__m128i *)&v118[2 * n0x70 + 17] = _mm_unpackhi_epi8(v15, v12);
      v12 = _mm_add_epi8(v12, v10);
      n0x70 += 16;
    }
    while ( n0x70 < 0x70 );
    Blockb_1 = (int32_t)Blockb;
    for ( j = 0; j < 7; ++j )
    {
      v118[4 * j + 225] = -2 * j - 113;
      v118[4 * j + 227] = -2 * j - 114;
      v118[4 * j + 226] = 2 * j + 113;
      v118[4 * j + 228] = 2 * j + 114;
    }
    Blockb = (int32_t *)Blockb_1;
    v118[253] = -127;
    v118[254] = 127;
    if ( __n256_0[0] )
    {
      n128 = n128_1;
      if ( n128_1 < 128 )
      {
        n128_2 = n128_1;
        v18 = 1;
        do
        {
          v19 = v118[2 * v18];
          v118[2 * v18] = v118[2 * n128];
          v20 = v118[2 * n128 - 1];
          v118[2 * n128] = v19;
          v21 = v118[2 * v18 - 1];
          v118[2 * v18 - 1] = v20;
          v118[2 * n128 - 1] = v21;
          n128 += n128_2;
          ++v18;
        }
        while ( n128 < 128 );
        Src_1 = Src;
        Blockb = (int32_t *)Blockb_1;
      }
    }
  }
  else
  {
    v22 = _mm_load_si128((const __m128i *)&__xmmword_4395D0);
    v23 = _mm_load_si128((const __m128i *)&__xmmword_4395E0);
    for ( k = 0; k < 0x100; k += 128 )
    {
      *(__m128i *)&v118[k] = v23;
      v25 = _mm_add_epi8(v23, v22);
      *(__m128i *)&v118[k + 16] = v25;
      v26 = _mm_add_epi8(v25, v22);
      *(__m128i *)&v118[k + 32] = v26;
      v27 = _mm_add_epi8(v26, v22);
      *(__m128i *)&v118[k + 48] = v27;
      v28 = _mm_add_epi8(v27, v22);
      *(__m128i *)&v118[k + 64] = v28;
      v29 = _mm_add_epi8(v28, v22);
      *(__m128i *)&v118[k + 80] = v29;
      v30 = _mm_add_epi8(v29, v22);
      *(__m128i *)&v118[k + 96] = v30;
      v31 = _mm_add_epi8(v30, v22);
      *(__m128i *)&v118[k + 112] = v31;
      v23 = _mm_add_epi8(v31, v22);
    }
    v114 = 0;
    n256_1 = n256;
    if ( n256 < 256 )
    {
      v33 = 0;
      Blockb_1 = (int32_t)Blockb;
      do
      {
        v34 = v118[v33];
        v118[v33++] = v118[n256];
        v118[n256] = v34;
        n256 += n128_1;
      }
      while ( n256 < 256 );
      Blockb = (int32_t *)Blockb_1;
      n256_1 = __n256_0[0];
      v114 = v33;
    }
    if ( n256 - n256_1 < 256 )
    {
      v35 = v118[v114];
      v118[v114] = v119;
      v119 = v35;
    }
  }
  if ( i <= 1 )
  {
    v44 = (uint8_t *)Blockb[17];
  }
  else
  {
    v36 = i - 1;
    m_1 = (i - 1) / 2;
    if ( m_1 )
    {
      for ( m = 0; m < m_1; ++m )
      {
        v39 = v118[*(uint8_t *)(Src_1 + 2 * m + 1)] + *(uint8_t *)(Src_1 + 2 * m);
        *(uint8_t *)(Src_1 + 2 * m + 1) = v39;
        *(uint8_t *)Blockb[17] = v39;
        v40 = Blockb[*(uint8_t *)Blockb[17] - *(uint8_t *)(Blockb[17] - 1) + 2091];
        *(uint32_t *)(Blockb[23] + 4) = v40;
        *(uint32_t *)Blockb[23] = v40;
        ++Blockb[17];
        Blockb[23] += 8;
        LOBYTE(v40) = v118[*(uint8_t *)(Src_1 + 2 * m + 2)] + *(uint8_t *)(Src_1 + 2 * m + 1);
        *(uint8_t *)(Src_1 + 2 * m + 2) = v40;
        *(uint8_t *)Blockb[17] = v40;
        v41 = Blockb[*(uint8_t *)Blockb[17] - *(uint8_t *)(Blockb[17] - 1) + 2091];
        *(uint32_t *)(Blockb[23] + 4) = v41;
        *(uint32_t *)Blockb[23] = v41;
        v42 = Blockb[17];
        Blockb[23] += 8;
        v43 = Src_1 + 2 * m + 3;
        v44 = (uint8_t *)(v42 + 1);
        Blockb[17] = (int32_t)v44;
      }
      v127 = (char *)v43;
      v36 = i - 1;
      v45 = 2 * m + 1;
    }
    else
    {
      v45 = 1;
      v44 = nullptr;
    }
    if ( v36 > v45 - 1 )
    {
      v46 = v118[*(uint8_t *)(v45 + Src_1)] + *(uint8_t *)(v45 + Src_1 - 1);
      *(uint8_t *)(v45 + Src_1) = v46;
      *(uint8_t *)Blockb[17] = v46;
      v47 = Blockb[*(uint8_t *)Blockb[17] - *(uint8_t *)(Blockb[17] - 1) + 2091];
      *(uint32_t *)(Blockb[23] + 4) = v47;
      *(uint32_t *)Blockb[23] = v47;
      v48 = Blockb[17];
      Blockb[23] += 8;
      v44 = (uint8_t *)(v48 + 1);
      Blockb[17] = (int32_t)v44;
      v127 = (char *)(Src_1 + v45 + 1);
    }
  }
  v49 = (uint8_t)*(v44 - 1);
  *v44 = v49;
  v50 = Blockb[23];
  Blockb[16] = v49;
  Blockb[21] = *(uint32_t *)(v50 - 8);
  Blockb[22] = *(uint32_t *)(v50 - 4);
  result = memcpy((char *)Blockb[20],(char *)Blockb[19],i + 2);
  v52 = a5 - 1;
  if ( a5 != 1 )
  {
    v120 = Blockb + 807;
    v121 = Blockb + 287;
    result = v127;
    v122 = Blockb + 547;
    v123 = Blockb + 27;
    do
    {
      while ( 1 )
      {
        v53 = (uint8_t)(result[-i] + v118[(uint8_t)*result]);
        *result = v53;
        v54 = Blockb[17];
        v127 = result + 1;
        v55 = *((uint8_t *)Blockb + 64);
        *(uint8_t *)(v54 - 1) = v55;
        *(uint8_t *)Blockb[17] = v55;
        v56 = Blockb[23];
        *(uint32_t *)(v56 - 8) = Blockb[21];
        *(uint32_t *)(v56 - 4) = Blockb[22];
        v57 = (uint32_t *)Blockb[23];
        *v57 = *(v57 - 2);
        v57[1] = *(v57 - 1);
        v58 = Blockb[19];
        v59 = Blockb[20];
        Blockb[19] = v59;
        Blockb[20] = v58;
        v59 += 2;
        Blockb[17] = v59;
        Blockb[18] = v58 + 2;
        Blockb[16] = v53;
        *(uint8_t *)(v59 - 2) = v53;
        Blockb[23] = Blockb[24] + 8;
        v60 = (uint8_t *)Blockb[18];
        result = v127;
        Blockb[21] = -Blockb[v53 - *(v60 - 1) + 2091];
        Blockb[22] = 0;
        if ( i != 1 )
          break;
        if ( !--v52 )
          return result;
      }
      v128 = i - 1;
      v115 = v52;
      v127 = result;
      while ( 1 )
      {
        v61 = Blockb[16];
        v62 = *v60;
        v63 = *(v60 - 1);
        Blockb_1 = (int32_t)Blockb;
        v64 = (uint8_t *)Blockb[17];
        v65 = *v64;
        v130 = v61;
        v66 = v63 + v65;
        v67 = v60[1];
        v133 = v62;
        v129 = v64;
        v68 = v61 + v62;
        v134 = (16 * (v68 + v67) + -3 * (v66 - *(v64 - 2)) + 8 * v68 + 4 * v61 - v133 + 31) >> 6;
        v69 = *(uint32_t **)(Blockb_1 + 92);
        v70 = *(v69 - 2) + *(uint32_t *)(Blockb_1 + 84);
        v71 = v69[2] + *v69;
        v131 = v69;
        Blockb = (int32_t *)Blockb_1;
        v72 = v71 + v70;
        v73 = v134;
        if ( v72 <= 0 )
        {
          v124 = -v72;
          *(uint32_t *)(Blockb_1 + 100) = v121;
          v75 = v124;
          v74 = v130 + ((*v60 - *(v60 - 1) + 2) >> 2);
        }
        else
        {
          v124 = v72;
          *(uint32_t *)(Blockb_1 + 100) = v123;
          v74 = *v60 + ((v68 - v66 + 4) >> 3);
          v75 = v124;
        }
        v135 = v68;
        Blockb[3] = v73;
        Blockb[7] = v74;
        v76 = (v73 + v74) >> 1;
        Blockb[5] = v76;
        Blockb[4] = (v76 + v73) >> 1;
        v77 = (v76 + v74) >> 1;
        v78 = v131;
        v79 = v135;
        Blockb[6] = v77;
        v80 = *(uint8_t *)(Blockb[25] + v75 + 16);
        *Blockb = v80;
        v81 = Blockb[v80 + 2];
        v82 = Blockb[22] + v78[3] + v78[1];
        if ( v82 < 0 )
        {
          v125 = -v82;
          Blockb[26] = (int32_t)v120;
          v91 = *(v60 - 1);
          v92 = *(v60 - 2);
          v132 = 2 * v81;
          v93 = 2 * v91 - *(v129 - 1) + v79 - 2 * v81 - v92 + 2;
          v88 = v125;
          v89 = v93 >> 2;
        }
        else
        {
          Blockb_1 = (int32_t)Blockb;
          v125 = v82;
          Blockb[26] = (int32_t)v122;
          v83 = *(v60 - 1);
          v84 = v60[1];
          v85 = *v60;
          v132 = 2 * v81;
          v86 = 2 * v84 - 2 * v81 - v129[1] - (v83 - v84);
          Blockb = (int32_t *)Blockb_1;
          v87 = v86 + v85 + 4;
          v88 = v125;
          v89 = v87 >> 3;
        }
        v90 = v81 + v89;
        Blockb[10] = v81;
        Blockb[14] = v90;
        v94 = v81 + v90 + 1;
        Blockb[11] = (v94 + v132) >> 2;
        Blockb[12] = v94 >> 1;
        v95 = v94 + 2 * v90;
        v96 = Blockb[26];
        Blockb[13] = v95 >> 2;
        v97 = *(uint8_t *)(v96 + v88 + 16);
        v98 = v127;
        Blockb[1] = v97;
        v99 = (uint8_t)(LOBYTE(Blockb[v97 + 9]) + v118[(uint8_t)*v98]);
        *v98 = v99;
        v100 = Blockb[23];
        v101 = Blockb[21];
        v127 = v98 + 1;
        *(uint32_t *)(v100 - 8) = v101;
        *(uint32_t *)(v100 - 4) = Blockb[22];
        v102 = Blockb[16];
        v103 = (uint8_t *)Blockb[18];
        Blockb[21] = Blockb[v99 - v102 + 2091] - Blockb[v99 - *v103 + 2091];
        v104 = Blockb[v99 - *(v103 - 1) + 2091] - Blockb[v99 - v103[1] + 2091];
        v105 = Blockb[17];
        Blockb[22] = v104;
        *(uint8_t *)(v105 - 1) = v102;
        v106 = Blockb[1];
        ++Blockb[17];
        Blockb[16] = v99;
        ++Blockb[18];
        Blockb[23] += 8;
        n6 = Blockb[v99 - Blockb[v106 + 9] + 2091];
        if ( n6 >= 6 )
        {
          v116 = Blockb[v99 - Blockb[10] + 2091] - 1;
          v117 = *Blockb;
          if ( v116 <= Blockb[v99 - Blockb[v117 + 1] + 2091] )
          {
            if ( v116 <= Blockb[v99 - Blockb[v117 + 3] + 2091] )
            {
              v110 = n6 - 1;
              if ( v110 <= Blockb[v99 - Blockb[v106 + 8] + 2091] )
              {
                if ( v110 > Blockb[v99 - Blockb[v106 + 10] + 2091] )
                {
                  v111 = Blockb[26];
                  v112 = *(uint32_t *)(v111 + 4 * v106 - 4) - 1;
                  *(uint32_t *)(v111 + 4 * v106 - 4) = v112;
                  *(uint8_t *)(Blockb[26] + v112 + 16) = v106 + 1;
                }
              }
              else
              {
                *(uint8_t *)(*(uint32_t *)(Blockb[26] + 4 * v106 - 8) + Blockb[26] + 16) = v106 - 1;
                ++*(uint32_t *)(Blockb[26] + 4 * Blockb[1] - 8);
              }
            }
            else
            {
              v108 = Blockb[25];
              v109 = *(uint32_t *)(v108 + 4 * v117 - 4) - 1;
              *(uint32_t *)(v108 + 4 * v117 - 4) = v109;
              *(uint8_t *)(Blockb[25] + v109 + 16) = v117 + 1;
            }
          }
          else
          {
            *(uint8_t *)(*(uint32_t *)(Blockb[25] + 4 * v117 - 8) + Blockb[25] + 16) = v117 - 1;
            ++*(uint32_t *)(Blockb[25] + 4 * *Blockb - 8);
          }
        }
        if ( !--v128 )
          break;
        v60 = (uint8_t *)Blockb[18];
      }
      result = v127;
      v52 = v115 - 1;
    }
    while ( v115 != 1 );
  }
  return result;
}

 BMF_SSE uint8_t *__sub_410310(int32_t Src, int32_t i, int32_t a3)
{
  ;
  __m128i si128, v8, v9, v10, v12, v13, v20, v21, v23, v24, v25, v26, v27, v28, v29;
  char v17, v18, v19, v33, v34, v39, v40, v53;
  int32_t Src_1, n256, n128_1, n128, v16, n256_1, v31, n128_2, i_1, v41, v42, v45, v46, v47, v48,
          n128_3;
  uint32_t n0x70, j, k, v36, m_1, m, v44, v50;
  uint8_t *result, *v43;
  alignas(16) uint8_t v52[255];
  Src_1 = Src;
  n256 = __n256_0[0];
  result = (uint8_t *)(Src + 1);
  n128_1 = 2 * __n256_0[0] + 1;
  if ( __n2 )
  {
    si128 = _mm_load_si128((const __m128i *)&__xmmword_439640);
    v8 = _mm_load_si128((const __m128i *)&__xmmword_439620);
    v9 = _mm_load_si128((const __m128i *)&__xmmword_439650);
    v10 = _mm_load_si128((const __m128i *)&__xmmword_439660);
    n0x70 = 0;
    v52[0] = 0;
    v53 = 0x80;
    do
    {
      v12 = v9;
      v13 = v9;
      v9 = _mm_add_epi8(v9, si128);
      *(__m128i *)&v52[2 * n0x70 + 1] = _mm_unpacklo_epi8(v12, v10);
      *(__m128i *)&v52[2 * n0x70 + 17] = _mm_unpackhi_epi8(v13, v10);
      v10 = _mm_add_epi8(v10, v8);
      n0x70 += 16;
    }
    while ( n0x70 < 0x70 );
    for ( j = 0; j < 7; ++j )
    {
      v52[4 * j + 225] = -2 * j - 113;
      v52[4 * j + 227] = -2 * j - 114;
      v52[4 * j + 226] = 2 * j + 113;
      v52[4 * j + 228] = 2 * j + 114;
    }
    Src_1 = Src;
    v52[253] = -127;
    v52[254] = 127;
    if ( __n256_0[0] )
    {
      n128 = n128_1;
      if ( n128_1 < 128 )
      {
        n128_3 = n128_1;
        v16 = 1;
        do
        {
          v17 = v52[2 * v16];
          v52[2 * v16] = v52[2 * n128];
          v18 = v52[2 * n128 - 1];
          v52[2 * n128] = v17;
          v19 = v52[2 * v16 - 1];
          v52[2 * v16 - 1] = v18;
          v52[2 * n128 - 1] = v19;
          n128 += n128_3;
          ++v16;
        }
        while ( n128 < 128 );
        result = (uint8_t *)(Src + 1);
        Src_1 = Src;
      }
    }
  }
  else
  {
    v20 = _mm_load_si128((const __m128i *)&__xmmword_439620);
    v21 = _mm_load_si128((const __m128i *)&__xmmword_439630);
    for ( k = 0; k < 0x100; k += 128 )
    {
      *(__m128i *)&v52[k] = v21;
      v23 = _mm_add_epi8(v21, v20);
      *(__m128i *)&v52[k + 16] = v23;
      v24 = _mm_add_epi8(v23, v20);
      *(__m128i *)&v52[k + 32] = v24;
      v25 = _mm_add_epi8(v24, v20);
      *(__m128i *)&v52[k + 48] = v25;
      v26 = _mm_add_epi8(v25, v20);
      *(__m128i *)&v52[k + 64] = v26;
      v27 = _mm_add_epi8(v26, v20);
      *(__m128i *)&v52[k + 80] = v27;
      v28 = _mm_add_epi8(v27, v20);
      *(__m128i *)&v52[k + 96] = v28;
      v29 = _mm_add_epi8(v28, v20);
      *(__m128i *)&v52[k + 112] = v29;
      v21 = _mm_add_epi8(v29, v20);
    }
    n256_1 = n256;
    v31 = 0;
    if ( n256 < 256 )
    {
      n128_2 = n128_1;
      do
      {
        v33 = v52[v31];
        v52[v31++] = v52[n256];
        v52[n256] = v33;
        n256 += n128_2;
      }
      while ( n256 < 256 );
      Src_1 = Src;
      n256_1 = __n256_0[0];
    }
    if ( n256 - n256_1 < 256 )
    {
      v34 = v52[v31];
      v52[v31] = v53;
      v53 = v34;
    }
  }
  i_1 = i;
  if ( i == 1 )
  {
    v42 = a3 - 1;
    if ( a3 == 1 )
      return result;
    v36 = 0;
    goto LABEL_29;
  }
  v36 = i - 1;
  m_1 = (i - 1) / 2;
  if ( m_1 )
  {
    for ( m = 0; m < m_1; ++m )
    {
      v39 = *(uint8_t *)(Src_1 + 2 * m) + v52[*(uint8_t *)(Src_1 + 2 * m + 1)];
      v40 = v52[*(uint8_t *)(Src_1 + 2 * m + 2)];
      *(uint8_t *)(Src_1 + 2 * m + 1) = v39;
      *(uint8_t *)(Src_1 + 2 * m + 2) = v39 + v40;
      result = (uint8_t *)(Src_1 + 2 * m + 3);
    }
    i_1 = i;
    v41 = 2 * m + 1;
  }
  else
  {
    v41 = 1;
  }
  if ( v36 > v41 - 1 )
  {
    *(uint8_t *)(v41 + Src_1) = *(uint8_t *)(v41 + Src_1 - 1) + v52[*(uint8_t *)(v41 + Src_1)];
    result = (uint8_t *)(Src_1 + v41 + 1);
    v42 = a3 - 1;
    if ( a3 == 1 )
      return result;
    goto LABEL_29;
  }
  v42 = a3 - 1;
  if ( a3 != 1 )
  {
LABEL_29:
    v50 = v36;
    v43 = &result[-i_1];
    do
    {
      while ( 1 )
      {
        ++v43;
        *result = v52[(uint8_t)*result] + result[-i_1];
        v44 = v50;
        ++result;
        if ( i_1 != 1 )
          break;
        if ( !--v42 )
          return result;
      }
      v48 = v42;
      do
      {
        v45 = (uint8_t)*(result - 1);
        v46 = (uint8_t)*v43;
        v47 = (uint8_t)result[-i - 1];
        if ( v45 < v46 )
        {
          if ( v47 < v45 )
          {
            LOBYTE(v45) = *v43;
            goto LABEL_41;
          }
          if ( v47 <= v46 )
LABEL_40:
            LOBYTE(v45) = v46 + v45 - v47;
        }
        else
        {
          if ( v47 > v45 )
          {
            LOBYTE(v45) = *v43;
            goto LABEL_41;
          }
          if ( v47 >= v46 )
            goto LABEL_40;
        }
LABEL_41:
        *result = v45 + v52[(uint8_t)*result];
        ++v43;
        ++result;
        --v44;
      }
      while ( v44 );
      i_1 = i;
      v42 = v48 - 1;
    }
    while ( v48 != 1 );
  }
  return result;
}

int32_t __sub_40AE10(uint32_t *_this, int32_t Src)
{
  ;
  int32_t v2, v3, result, v5, v6, v7, v8, v9;
  v2 = *(_this + 2);
  v3 = *(_this + 3);
  v8 = *(uint8_t *)(Src - 1);
  v9 = *(uint8_t *)(Src + v2);
  v6 = *(uint8_t *)(Src + v2 - 1);
  v7 = *(uint8_t *)(Src + v2 + 1);
  result = v9 & 1
         | *(_this
           + ((*(uint8_t *)(Src + v3 + 3)
             + *(uint8_t *)(Src + v3 + 2)
             + *(uint8_t *)(Src + v3 - 2)
             + *(uint8_t *)(Src + v3 - 1)
             + *(uint8_t *)(Src + v2 - 3)
             + *(uint8_t *)(Src + v2 + 4)
             + *(uint8_t *)(Src + v2 + 3)
             + *(uint8_t *)(Src + v2 - 2)
             + 3 * (v7 + v6)
             + *(uint8_t *)(Src - 4)
             + *(uint8_t *)(Src - 3)
             + 4 * (v9 + v8)
             + 2
             * (*(uint8_t *)(Src + v2 + 2)
              + *(uint8_t *)(Src + v3 + 1)
              + *(uint8_t *)(Src + v3)
              + *(uint8_t *)(Src - 2))) >> 5)
           + 5)
         | (2 * (v8 & 1));
  if ( __n2 == 1 )
  {
    v5 = result < *(_this + v8 + 5) || result < *(_this + v7 + 5) || result < *(_this + v6 + 5);
    return (v5 << 7) | (8 * (v6 & 1)) | (4 * (v7 & 1)) | result;
  }
  return result;
}

BMF_SSE __attribute__((visibility("hidden")))
__gnu_m128d __sub_436E10(const __m128d &a0__ref, const __m128d &a1__ref)
{
  ;
  __m128d a0 = a0__ref;
  __m128d a1 = a1__ref;
  __m128d x = _mm_or_pd(_mm_and_pd(a0, a1), _mm_andnot_pd(a1, __bmf_half_half));
  if ( _mm_movemask_pd(a1) )
  {
    // Intel's __svml_log2 was the natural log despite the name (see
    // override/sub_436E10.inc).  Lane 1 is computed too: the caller only reads
    // lane 0 through the mask, but leaving it undefined would let a signalling
    // value through.
    __m128d r;
    r.m128d_f64[0] = log(x.m128d_f64[0]);
    r.m128d_f64[1] = log(x.m128d_f64[1]);
    return r;
  }
  return x;
}

 BMF_SSE int32_t __sub_411700(int32_t a1, int32_t n2)
{
  ;
  __m128 v17, v18;
  __m128d v13, v15;
  __m128i v4, v5;
  double v7, v8, v14, v16;
  int32_t v11;
  uint32_t n2_1, v3, n2_4, n2_3, n2_2;
  n2_1 = n2;
  if ( n2 <= 0 )
  {
    v7 = 0;
    v8 = 0.0;
  }
  else
  {
    if ( n2 < 2 )
    {
      n2_4 = 0;
      v7 = 0;
      v8 = 0;
    }
    else
    {
      v13 = 0;
      v15 = 0;
      v3 = 0;
      do
      {
        v4 = _mm_loadl_epi64((const __m128i *)(a1 + 4 * v3));
        v17 = (__m128)_mm_cvtepi32_pd(v4);
        v5 = _mm_cmpeq_epi32((__m128i)0LL, v4);
        v18 = (__m128)_mm_unpacklo_epi32(v5, v5);
        v13 = _mm_add_pd(v13, (__m128d)_mm_andnot_ps(v18, v17));
        __m128d v19 = __sub_436E10((__m128d)v17, (__m128d)_mm_xor_si128((__m128i)__xmmword_4397A0, (__m128i)v18));
        v3 += 2;
        v15 = _mm_add_pd(v15, (__m128d)_mm_andnot_ps(v18, (__m128)_mm_mul_pd((__m128d)v17, v19)));
      }
      while ( v3 < n2 - (n2 & 1u) );
      n2_1 = n2;
      n2_4 = n2 - (n2 & 1);
      v7 = v15.m128d_f64[0] + M128D(_mm_unpackhi_pd(v15, v15)).m128d_f64[0];
      v8 = v13.m128d_f64[0] + M128D(_mm_unpackhi_pd(v13, v13)).m128d_f64[0];
    }
    if ( n2_4 < n2_1 )
    {
      v14 = v7;
      v16 = v8;
      n2_3 = n2_4;
      n2_2 = n2_1;
      do
      {
        v11 = *(uint32_t *)(a1 + 4 * n2_3);
        if ( v11 )
        {
          v16 = v16 + (double)v11;
          v14 = v14 + (double)v11 * log((double)v11);
        }
        ++n2_3;
      }
      while ( n2_3 < n2_2 );
      v7 = v14;
      v8 = v16;
    }
    if ( v8 != 0.0 )
      v8 = v8 * log(v8);
  }
  return (int32_t)((v8 - v7) * 1.442695040888963);
}

BMF_SSE int32_t __sub_40BBC0(int32_t n2, int32_t a2, int32_t a3, char a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
  alignas(16) uint8_t __hexrays_frame[2128];
  __m128i (&__Src)[64] = *(__m128i (*)[64])(__hexrays_frame + 0);
  int32_t &v38 = *(int32_t *)(__hexrays_frame + 1024);
  char (&v39)[16] = *(char (*)[16])(__hexrays_frame + 1028);
  __m128i (&v40)[63] = *(__m128i (*)[63])(__hexrays_frame + 1044);
  int32_t &v41 = *(int32_t *)(__hexrays_frame + 2052);
  int32_t &v42 = *(int32_t *)(__hexrays_frame + 2056);
  int32_t &v43 = *(int32_t *)(__hexrays_frame + 2060);
  int32_t &v44 = *(int32_t *)(__hexrays_frame + 2064);
  int32_t &n2_1 = *(int32_t *)(__hexrays_frame + 2068);
  int32_t &v46 = *(int32_t *)(__hexrays_frame + 2072);
  int32_t &v47 = *(int32_t *)(__hexrays_frame + 2076);
  int32_t &v48 = *(int32_t *)(__hexrays_frame + 2080);
  int32_t &v49 = *(int32_t *)(__hexrays_frame + 2084);
  int32_t &v50 = *(int32_t *)(__hexrays_frame + 2088);
  ;
  __m128i v19, v20, v21, v22, si128, v24, v25, v26, v27, v28, v29, v30;
  char v11, v33, v34;
  int32_t v9, v10, v12, v13, v14, v15, v18, v31, v32, v35, n2_2;
  uint32_t n0x100;
  v49 = a3;
  v48 = a2;
  v9 = *(&__n14 + n2);
  v50 = *(&__n24 + n2);
  v10 = v50 - 1;
  if ( v50 - 1 == v9 )
    return 0;
  memcpy((char *)__Src,(&::__Src)[n2],1028);
  memcpy(v39,(&__dword_442E74)[n2],1028);
  v46 = *(uint32_t *)(v48 + 4 * n2 + 4) + *(uint32_t *)(v48 + 4 * n2);
  v47 = 0;
  if ( v10 > v9 )
  {
    n2_1 = n2;
    v12 = a8 + 4 * v50 - 4;
    v49 += 257 * (4 * v50 - 4);
    v13 = v9;
    v14 = 0;
    do
    {
      ++v14;
      if ( *(uint32_t *)(-4 * v14 + v12 + 4) )
      {
        n0x100 = 0;
        v18 = v49 - 1028 * v14;
        do
        {
          v19 = _mm_loadu_si128((const __m128i *)(v18 + 4 * n0x100 + 1028));
          v20 = _mm_loadu_si128((const __m128i *)(v18 + 4 * n0x100 + 1044));
          v21 = _mm_loadu_si128((const __m128i *)&v40[n0x100 / 4]);
          v22 = _mm_loadu_si128((const __m128i *)(v18 + 4 * n0x100 + 1060));
          si128 = _mm_load_si128((const __m128i *)&__Src[n0x100 / 4]);
          v24 = _mm_load_si128((const __m128i *)&__Src[n0x100 / 4 + 1]);
          v25 = _mm_load_si128((const __m128i *)&__Src[n0x100 / 4 + 2]);
          *(__m128i *)&v39[4 * n0x100] = _mm_sub_epi32(_mm_loadu_si128((const __m128i *)&v39[4 * n0x100]), v19);
          v26 = _mm_loadu_si128((const __m128i *)(v18 + 4 * n0x100 + 1076));
          v27 = _mm_add_epi32(si128, v19);
          v28 = _mm_load_si128((const __m128i *)&__Src[n0x100 / 4 + 3]);
          __Src[n0x100 / 4] = v27;
          v29 = _mm_loadu_si128((const __m128i *)&v40[n0x100 / 4 + 1]);
          __Src[n0x100 / 4 + 1] = _mm_add_epi32(v24, v20);
          v30 = _mm_loadu_si128((const __m128i *)&v40[n0x100 / 4 + 2]);
          v40[n0x100 / 4] = _mm_sub_epi32(v21, v20);
          __Src[n0x100 / 4 + 2] = _mm_add_epi32(v25, v22);
          v40[n0x100 / 4 + 1] = _mm_sub_epi32(v29, v22);
          __Src[n0x100 / 4 + 3] = _mm_add_epi32(v28, v26);
          v40[n0x100 / 4 + 2] = _mm_sub_epi32(v30, v26);
          n0x100 += 16;
        }
        while ( n0x100 < 0x100 );
        v31 = *(uint32_t *)(v18 + 2052);
        v41 -= v31;
        v38 += v31;
        v44 = __sub_411700((int32_t)__Src, 257);
        v32 = __sub_411700((int32_t)v39, 257);
        v43 = v32;
        v42 = v44 + v32;
        if ( v44 + v32 > v46 + 512 )
          return v47;
        if ( v44 + v32 < v46 )
        {
          memcpy((&::__Src)[n2_1],(char *)__Src,1028);
          memcpy((&__dword_442E74)[n2_1],v39,1028);
          v35 = v48;
          n2_2 = n2_1;
          v47 = 1;
          *(uint32_t *)(v48 + 4 * n2_1) = v44;
          *(uint32_t *)(v35 + 4 * n2_2 + 4) = v43;
          v46 = v42;
          v15 = v50 - v14 - 1;
          *(&__n24 + n2_2) = v50 - v14;
          v13 = *(&__n14 + n2_2);
        }
        else
        {
          v13 = *(&__n14 + n2_1);
          v15 = v50 - v14 - 1;
        }
      }
      else
      {
        v15 = v50 - v14 - 1;
      }
    }
    while ( v15 > v13 );
  }
  return v47;
}

BMF_SSE uint32_t __sub_40B330(uint32_t *_this, uint32_t *buf)
{
  alignas(16) uint8_t __hexrays_frame[2176];
  int32_t &v79 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v80 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v81 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v82 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v83 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v84 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v85 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v86 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v87 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v88 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v89 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v90 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v91 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v92 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v93 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &__sub_40B330_Src = *(int32_t *)(__hexrays_frame + 16);
  int32_t &Src_4 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &Src_8 = *(int32_t *)(__hexrays_frame + 24);
  __m128i (&v97)[63] = *(__m128i (*)[63])(__hexrays_frame + 32);
  int32_t &v98 = *(int32_t *)(__hexrays_frame + 1040);
  char (&v99)[16] = *(char (*)[16])(__hexrays_frame + 1044);
  __m128i (&v100)[63] = *(__m128i (*)[63])(__hexrays_frame + 1060);
  int32_t &v101 = *(int32_t *)(__hexrays_frame + 2068);
  int32_t (&v102)[6] = *(int32_t (*)[6])(__hexrays_frame + 2072);
  int32_t &v103 = *(int32_t *)(__hexrays_frame + 2096);
  int32_t &v104 = *(int32_t *)(__hexrays_frame + 2100);
  int32_t &v105 = *(int32_t *)(__hexrays_frame + 2104);
  int32_t &v106 = *(int32_t *)(__hexrays_frame + 2108);
  int32_t &v107 = *(int32_t *)(__hexrays_frame + 2112);
  int32_t &v108 = *(int32_t *)(__hexrays_frame + 2116);
  int32_t &v109 = *(int32_t *)(__hexrays_frame + 2120);
  int32_t &v110 = *(int32_t *)(__hexrays_frame + 2124);
  int32_t &v111 = *(int32_t *)(__hexrays_frame + 2128);
  uint32_t &n5_2 = *(uint32_t *)(__hexrays_frame + 2132);
  int32_t &v113 = *(int32_t *)(__hexrays_frame + 2136);
  uint32_t * &this_1 = *(uint32_t * *)(__hexrays_frame + 2140);
  ;
  __m128i v12, v13, v68, v69, si128, v71, v72, v73, v74;
  char *v4, *v16, *v39, v46, v51, v57, v59, v61, v63, v75;
  int32_t v3, v5, v6, n28672, v14, Src_1, v17, v18, v20, Src_2, v22, Src_3, v24, v25, v27, v28,
          v29, __sub_40B330_n8, n5, Src_5, v36, v37, Src_6, v40, v41, v43, Src_7, v45, Src_9,
          v48, v50, v52, v53, v54, v55, v56, v58, v60, v62, v64, v65, v67, v77, v78;
  uint32_t i, *this_2, n0x40_1, n0x40_2, n0x40, n0x80, n0x20, v30, n6, n0x3F, n0x80_1, n5_1,
           n0x100, n5_3;
  this_1 = _this;
  for ( i = 0; i < 0xC; ++i )
  {
    if ( __dword_438B00[i] > 0 )
    {
      v3 = (int32_t)*(&__n633 + i);
      v4 = (char *)__off_4410C0[i];
      v5 = __dword_438B00[i];
      v6 = 0;
      Src_4 = (uint8_t)__byte_439418[i];
      __sub_40B330_Src = i;
      do
      {
        ++v6;
        *(uint32_t *)(v3 + 4 * v6 - 4) -= 1 << ((Src_4 - v4[v6 - 1]) & 31);
      }
      while ( v6 < v5 );
      i = __sub_40B330_Src;
    }
  }
  this_2 = this_1;
  v113 = (int32_t)(this_1 + 49664);
  n0x40_1 = (uint32_t)(this_1 + 49664) & 0xF;
  if ( n0x40_1 )
  {
    n28672 = 0;
    if ( (n0x40_1 & 3) != 0 )
    {
      n0x40 = 0;
      goto LABEL_16;
    }
    n0x40_1 = (16 - n0x40_1) >> 2;
    n0x40_2 = 0;
    do
      n28672 += this_1[n0x40_2++ + 49664];
    while ( n0x40_2 < n0x40_1 );
  }
  else
  {
    n28672 = 0;
  }
  n0x40 = 64 - (-n0x40_1 & 3);
  v12 = _mm_cvtsi32_si128(n28672);
  do
  {
    v12 = _mm_add_epi32(v12, *(__m128i *)&this_1[n0x40_1 + 49664]);
    n0x40_1 += 4;
  }
  while ( n0x40_1 < n0x40 );
  v13 = _mm_add_epi32(v12, _mm_srli_si128(v12, 8));
  n28672 = _mm_cvtsi128_si32(_mm_add_epi32(v13, _mm_srli_si128(v13, 4)));
LABEL_16:
  while ( n0x40 < 0x40 )
    n28672 += this_1[n0x40++ + 49664];
  __dword_44340C[0] = 64;
  __dword_443424 = 0;
  if ( n28672 < 28672 && !__dword_443388 )
  {
    __n24 = 24;
    __n14 = 14;
    __sub_40B330_Src = 0;
    __n9 = 9;
    __n7 = 7;
    __dword_443420 = 0;
    do
    {
      v14 = __dword_44340C[__sub_40B330_Src];
      Src_1 = v14 - 1;
      if ( v14 - 1 >= *(&__n24 + __sub_40B330_Src) )
      {
        v16 = (&::__Src)[__sub_40B330_Src];
        Src_8 = *(&__n24 + __sub_40B330_Src);
        v17 = 0;
        v18 = (int32_t)&this_1[257 * v14 + 49471];
        do
        {
          Src_4 = Src_1;
          ++v17;
          n0x80 = 0;
          v20 = v18 - 1028 * v17;
          do
          {
            *(uint32_t *)&v16[8 * n0x80] += *(uint32_t *)(v20 + 8 * n0x80 + 1028);
            *(uint32_t *)&v16[8 * n0x80 + 4] += *(uint32_t *)(v20 + 8 * n0x80 + 1032);
            ++n0x80;
          }
          while ( n0x80 < 0x80 );
          Src_2 = Src_4;
          *((uint32_t *)v16 + 256) += *(uint32_t *)(v20 + 2052);
          Src_1 = Src_2 - 1;
        }
        while ( Src_1 >= Src_8 );
      }
      v22 = __sub_411700((int32_t)(&::__Src)[__sub_40B330_Src], 257);
      Src_3 = __sub_40B330_Src;
      v102[__sub_40B330_Src] = v22;
      __sub_40B330_Src = Src_3 + 1;
    }
    while ( (uint32_t)(Src_3 + 1) < 6 );
    v24 = __dword_443388;
    goto LABEL_28;
  }
  n5 = 5;
  __sub_40B330_Src = (n28672 + 3) / 6;
  Src_5 = __sub_40B330_Src;
  n0x3F = 0;
  v36 = 0;
  do
  {
    Src_5 -= this_2[n0x3F + 49665];
    if ( Src_5 < 0 || n5 >= v36 + 62 )
    {
      Src_5 += __sub_40B330_Src;
      --v36;
      __dword_44340C[n5] = ++n0x3F;
      if ( !--n5 )
        break;
    }
    else
    {
      --v36;
      ++n0x3F;
    }
  }
  while ( n0x3F < 0x3F );
  v106 = (int32_t)(this_2 + 49728);
  this_1 = this_2;
  __sub_40B330_Src = 0;
  do
  {
    v37 = __dword_44340C[__sub_40B330_Src];
    Src_6 = v37 - 1;
    if ( v37 - 1 >= *(&__n24 + __sub_40B330_Src) )
    {
      v39 = (&::__Src)[__sub_40B330_Src];
      Src_8 = *(&__n24 + __sub_40B330_Src);
      v40 = 0;
      v41 = (int32_t)&this_1[257 * v37 + 49471];
      do
      {
        Src_4 = Src_6;
        ++v40;
        n0x80_1 = 0;
        v43 = v41 - 1028 * v40;
        do
        {
          *(uint32_t *)&v39[8 * n0x80_1] += *(uint32_t *)(v43 + 8 * n0x80_1 + 1028);
          *(uint32_t *)&v39[8 * n0x80_1 + 4] += *(uint32_t *)(v43 + 8 * n0x80_1 + 1032);
          ++n0x80_1;
        }
        while ( n0x80_1 < 0x80 );
        Src_7 = Src_4;
        *((uint32_t *)v39 + 256) += *(uint32_t *)(v43 + 2052);
        Src_6 = Src_7 - 1;
      }
      while ( Src_6 >= Src_8 );
    }
    v45 = __sub_411700((int32_t)(&::__Src)[__sub_40B330_Src], 257);
    Src_9 = __sub_40B330_Src;
    v102[__sub_40B330_Src] = v45;
    __sub_40B330_Src = Src_9 + 1;
  }
  while ( (uint32_t)(Src_9 + 1) < 6 );
  v48 = v106;
  while ( 2 )
  {
    v106 = v48;
    n5_1 = 0;
    v110 = 0;
    do
    {
      v50 = *(&__n24 + n5_1);
      v111 = __dword_44340C[n5_1];
      if ( v111 == v50 + 1 )
      {
        v52 = 0;
        goto LABEL_62;
      }
      memcpy((char *)&__sub_40B330_Src,(&::__Src)[n5_1],1028);
      memcpy(v99,(&__dword_442E74)[n5_1],1028);
      v52 = 0;
      v109 = v102[n5_1 + 1] + v102[n5_1];
      if ( v50 + 1 >= v111 )
        goto LABEL_62;
      v107 = 0;
      n5_2 = n5_1;
      v53 = 0;
      v108 = (int32_t)&this_1[257 * v50 + 49728];
      while ( 1 )
      {
        ++v53;
        if ( !*(uint32_t *)(v113 + 4 * v50) )
          goto LABEL_58;
        n0x100 = 0;
        v67 = v108 + 1028 * v53;
        do
        {
          v68 = _mm_loadu_si128((const __m128i *)&v99[4 * n0x100]);
          v69 = _mm_loadu_si128((const __m128i *)&v100[n0x100 / 4]);
          si128 = _mm_load_si128((const __m128i *)&v97[n0x100 / 4]);
          *(__m128i *)(&__sub_40B330_Src + n0x100) = _mm_sub_epi32(
                                          _mm_load_si128((const __m128i *)(&__sub_40B330_Src + n0x100)),
                                          _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 1028)));
          v71 = _mm_load_si128((const __m128i *)&v97[n0x100 / 4 + 1]);
          *(__m128i *)&v99[4 * n0x100] = _mm_add_epi32(v68, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 1028)));
          v72 = _mm_loadu_si128((const __m128i *)&v100[n0x100 / 4 + 1]);
          v97[n0x100 / 4] = _mm_sub_epi32(si128, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 1012)));
          v73 = _mm_load_si128((const __m128i *)&v97[n0x100 / 4 + 2]);
          v100[n0x100 / 4] = _mm_add_epi32(v69, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 1012)));
          v74 = _mm_loadu_si128((const __m128i *)&v100[n0x100 / 4 + 2]);
          v97[n0x100 / 4 + 1] = _mm_sub_epi32(v71, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 996)));
          v100[n0x100 / 4 + 1] = _mm_add_epi32(v72, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 996)));
          v97[n0x100 / 4 + 2] = _mm_sub_epi32(v73, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 980)));
          v100[n0x100 / 4 + 2] = _mm_add_epi32(v74, _mm_loadu_si128((const __m128i *)(v67 + 4 * n0x100 - 980)));
          n0x100 += 16;
        }
        while ( n0x100 < 0x100 );
        v98 -= *(uint32_t *)(v67 - 4);
        v101 += *(uint32_t *)(v67 - 4);
        v105 = __sub_411700((int32_t)&__sub_40B330_Src, 257);
        v104 = __sub_411700((int32_t)v99, 257);
        v103 = v105 + v104;
        if ( v105 + v104 > v109 + 4096 )
          break;
        if ( v105 + v104 < v109 )
        {
          memcpy((&::__Src)[n5_2],(char *)&__sub_40B330_Src,1028);
          memcpy((&__dword_442E74)[n5_2],v99,1028);
          n5_3 = n5_2;
          v77 = v104;
          v102[n5_2] = v105;
          v78 = v103;
          v102[n5_3 + 1] = v77;
          v109 = v78;
          v107 = 1;
          *(&__n24 + n5_3) = ++v50;
          goto LABEL_59;
        }
LABEL_58:
        ++v50;
LABEL_59:
        if ( v50 + 1 >= __dword_44340C[n5_2] )
        {
          v52 = v107;
          n5_1 = n5_2;
          goto LABEL_62;
        }
      }
      v52 = v107;
      n5_1 = n5_2;
LABEL_62:
      v110 |= v52;
      ++n5_1;
    }
    while ( n5_1 < 5 );
    v54 = v110;
    v48 = v106;
    v55 = v113;
    v56 = __sub_40BBC0(0, (int32_t)v102, v106, v46, v79, v84, v89, v113) | v54;
    v58 = __sub_40BBC0(1, (int32_t)v102, v48, v57, v80, v85, v90, v55) | v56;
    v60 = __sub_40BBC0(2, (int32_t)v102, v48, v59, v81, v86, v91, v55) | v58;
    v62 = __sub_40BBC0(3, (int32_t)v102, v48, v61, v82, v87, v92, v55) | v60;
    v64 = __sub_40BBC0(4, (int32_t)v102, v48, v63, v83, v88, v93, v55);
    v24 = __dword_443388;
    if ( v64 | v62 )
    {
      if ( __dword_443388 )
        goto LABEL_65;
      continue;
    }
    break;
  }
LABEL_28:
  if ( v24 )
  {
LABEL_65:
    v65 = 0;
    for ( n6 = 0; n6 < 6; ++n6 )
      v65 += v102[n6];
    __n256_2[0] = v65;
  }
  else
  {
    *buf = 0;
    v25 = 0;
    n0x20 = 0;
    buf[1] = 1;
    v27 = 0;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    buf[6] = 6;
    buf[7] = 7;
    buf[8] = 8;
    buf[9] = 9;
    buf[10] = 10;
    buf[11] = 11;
    do
    {
      v28 = v25 + 12;
      buf[v27 + 75] = v25 + 12;
      v29 = *(&__n24 + v25);
      if ( v29 == v27 + 63 )
      {
        ++v25;
        v29 = *(&__n24 + v25);
        v28 = v25 + 12;
      }
      buf[v27 + 74] = v28;
      if ( v29 == v27 + 62 )
        ++v25;
      v27 -= 2;
      ++n0x20;
    }
    while ( n0x20 < 0x20 );
    v30 = (__dword_443420 << 24) | (__n7 << 18) | (__n9 << 12) | __n24 | (__n14 << 6);
    if ( ::__n8 < 30 )
    {
      *(uint32_t *)__Buffer_1 = __n256 | (2 * (v30 << ((31 - ::__n8) & 31)));
      __Buffer_1 = __Buffer_0;
      n6 = v30 >> (::__n8 & 31);
      __Buffer_0 += 4;
      __n256 = n6;
      __sub_40B330_n8 = ::__n8 + 2;
    }
    else
    {
      n6 = v30 << (-(char)::__n8 & 31);
      __sub_40B330_n8 = ::__n8 - 30;
      __n256 |= n6;
    }
    ::__n8 = __sub_40B330_n8;
  }
  return n6;
}

 int32_t __sub_40CB70()
{
  ;
  char v9;
  int32_t v0, n4, *v2, v3, v4, v5, v6, v7, n4_1;
  v0 = 1;
  if ( *(int32_t *)(__dword_44342C + 4) < 0 )
  {
    do
    {
      ++v0;
      n4 = 4 * v0;
      v2 = (int32_t *)(__dword_44342C + 4 * v0);
    }
    while ( *v2 < 0 );
  }
  else
  {
    n4 = 4;
    v2 = (int32_t *)(__dword_44342C + 4);
  }
  v3 = (1 << (v0 & 31)) - 1;
  if ( v3 >= 0 )
  {
    v4 = __buf_1;
    v5 = __dword_443428;
    __dword_44342C = (int32_t)v2;
    v9 = v0;
    n4_1 = n4;
    do
    {
      v6 = *v2;
      *(uint16_t *)(v4 + 4 * v6) = v5 | ((uint16_t)v3 << ((((uint32_t)((char *)v2 - (char *)&__xmmword_442EA0) >> 2) - v9) & 31));
      v7 = __dword_442EE0[v6];
      *v2 = v7;
      if ( v7 < 0 )
      {
        if ( --v3 >= 0 )
        {
          do
          {
            __dword_443428 = v5 | (v3 << ((((uint32_t)(__dword_44342C - (uint32_t)&__xmmword_442EA0) >> 2) - v9) & 31));
            __sub_40CB70();
            --v3;
          }
          while ( v3 >= 0 );
          v2 = (int32_t *)__dword_44342C;
        }
        __dword_443428 = v5;
      }
      --v3;
    }
    while ( v3 >= 0 );
    n4 = n4_1;
  }
  __dword_44342C = (int32_t)v2 - n4;
  return n4;
}

int32_t __sub_40CC50(int32_t j, uint32_t *a2, char *buf, char a4)
{
  alignas(16) uint8_t __hexrays_frame[192];
  uint8_t (&v44)[128] = *(uint8_t (*)[128])(__hexrays_frame + 0);
  char * &bufa = *(char * *)(__hexrays_frame + 128);
  int32_t &k_2 = *(int32_t *)(__hexrays_frame + 132);
  uint32_t * &i_1 = *(uint32_t * *)(__hexrays_frame + 136);
  int32_t &v48 = *(int32_t *)(__hexrays_frame + 140);
  int32_t &i_7 = *(int32_t *)(__hexrays_frame + 144);
  uint32_t &j_1 = *(uint32_t *)(__hexrays_frame + 148);
  int32_t &n15_1 = *(int32_t *)(__hexrays_frame + 152);
  int32_t &v52 = *(int32_t *)(__hexrays_frame + 156);
  ;
  bool v17;
  char *bufa_2, v39, v40, *bufa_1;
  int32_t v5, v6, result, v9, v10, i_3, v19, v23, n15_2, n15, v29, v32, v33, v34, v36, v38, v41,
          v43;
  uint32_t j_2, k_4, *i_5, j_3, j_4, *i_9, *i, *i_2, *i_4, *i_6, *i_8, k_1, k, k_3;
  uint8_t *v24, *v28;
  bufa = buf;
  j_1 = j;
  v5 = 0;
  v6 = 0;
  if ( j <= 0 )
    goto LABEL_50;
  result = (int32_t)__dword_443408;
  j_2 = 0;
  do
  {
    v9 = a2[j_2];
    v10 = 3 * j_2;
    *(uint32_t *)(result + 8 * v10 + 4) = 0;
    *(uint32_t *)(result + 8 * v10) = v9;
    if ( v9 )
    {
      v6 = result + 24 * j_2;
      v9 = 1;
    }
    ++j_2;
    v5 += v9;
  }
  while ( j_2 < j_1 );
  k_4 = 0;
  if ( v5 <= 1 )
  {
LABEL_50:
    result = (int32_t)memset(bufa,0,4 * j_1);
    if ( v5 == 1 )
    {
      v41 = v6 - (uint32_t)__dword_443408;
      bufa_1 = bufa;
      v43 = *a2 != 0;
      bufa[4 * (v41 / 24) + 2] = 1;
      bufa_1[4 * v43 + 2] = 1;
      return 715827883 * v41;
    }
  }
  else
  {
    i_3 = result + 12336;
    k_2 = (int32_t)j_1 / 2;
    i_1 = (uint32_t *)(result + 12336);
    v52 = result + 24 * j_1;
    while ( 1 )
    {
      i_5 = i_1;
      j_3 = j_1;
      j_4 = 0;
      do
      {
        if ( *(uint32_t *)(result + 24 * j_4) )
        {
          i_9 = (uint32_t *)(result + 24 * j_4);
          i_5[5] = i_9;
          i_9[4] = i_5;
          i_5 = i_9;
          i_9[3] = 0;
          i_9[2] = 0;
        }
        ++j_4;
      }
      while ( j_4 < j_3 );
      i_5[5] = i_3;
      v17 = i_5 == *(uint32_t **)(i_3 + 20);
      *(uint32_t *)(result + 12336) = 0x7FFFFFFF;
      *(uint32_t *)(i_3 + 16) = i_5;
      if ( !v17 )
      {
        i = i_1;
        v48 = result;
        v19 = 0;
        do
        {
          ++v19;
          i_2 = i;
          i_4 = (uint32_t *)i_3;
          for ( i = (uint32_t *)i_3; i_5 != (uint32_t *)i_3; i_5 = (uint32_t *)i_5[4] )
          {
            if ( *i_5 < *i_2 )
            {
              i_2 = i_4;
              if ( *i_5 >= *i_4 )
                i_2 = i_5;
              else
                i_4 = i_5;
            }
          }
          *(uint32_t *)(i_4[5] + 16) = i_4[4];
          *(uint32_t *)(i_4[4] + 20) = i_4[5];
          *(uint32_t *)(i_2[5] + 16) = i_2[4];
          *(uint32_t *)(i_2[4] + 20) = i_2[5];
          i_6 = (uint32_t *)(v52 + 24 * v19);
          *(i_6 - 6) = *i_4 + *i_2;
          i_6 -= 6;
          i_6[2] = i_4;
          i_6[3] = i_2;
          v23 = *(uint32_t *)(i_3 + 20);
          i_6[5] = v23;
          i_6[4] = i_3;
          *(uint32_t *)(v23 + 16) = i_6;
          i_5 = *(uint32_t **)(i_3 + 16);
          *(uint32_t *)(i_3 + 20) = i_6;
        }
        while ( i_5 != i_6 );
        result = v48;
        i_1 = (uint32_t *)i_3;
        k_4 = 0;
      }
      v24 = v44;
      i_7 = i_3;
      n15_2 = 0;
      v48 = result;
      n15 = 0;
      while ( 1 )
      {
        i_8 = (uint32_t *)i_5[2];
        if ( i_8 )
        {
          n15_1 = n15;
          v28 = v24;
          v29 = 0;
          do
          {
            ++n15_2;
            ++v29;
            v24 = &v28[4 * v29];
            *((uint32_t *)v24 - 1) = i_5;
            i_5[1] = n15_2 - 1;
            i_5 = i_8;
            i_8 = (uint32_t *)i_8[2];
          }
          while ( i_8 );
          n15 = n15_1;
          k_4 = 0;
        }
        i_5[1] = n15_2;
        if ( n15_2 >= n15 )
          n15 = n15_2;
        v24 -= 4;
        if ( v24 < v44 )
          break;
        i_5 = *(uint32_t **)(*(uint32_t *)v24 + 12);
        n15_2 = *(uint32_t *)(*(uint32_t *)v24 + 4) + 1;
      }
      i_3 = i_7;
      result = v48;
      if ( n15 <= 15 )
        break;
      if ( k_2 )
      {
        k_1 = k_2;
        for ( k = 0; k < k_1; ++k )
        {
          v32 = 48 * k;
          if ( *(int32_t *)(result + 48 * k + 4) > 15 )
            ++*(uint32_t *)(result + 48 * k);
          if ( *(int32_t *)(result + v32 + 28) > 15 )
            ++*(uint32_t *)(result + v32 + 24);
        }
        k_2 = k_1;
        v33 = 2 * k + 1;
      }
      else
      {
        v33 = 1;
      }
      if ( v33 - 1 < j_1 )
      {
        v34 = 3 * v33;
        if ( *(int32_t *)(result + 8 * v34 - 20) > 15 )
          ++*(uint32_t *)(result + 8 * v34 - 24);
      }
    }
    k_3 = k_2;
    v36 = 1;
    if ( k_2 )
    {
      bufa_2 = bufa;
      do
      {
        v38 = 2 * k_4;
        v39 = *(uint8_t *)(result + 48 * k_4 + 4);
        v40 = *(uint8_t *)(result + 48 * k_4++ + 28);
        bufa_2[4 * v38 + 2] = v39;
        bufa_2[4 * v38 + 6] = v40;
      }
      while ( k_4 < k_3 );
      v36 = 2 * k_4 + 1;
    }
    if ( v36 - 1 < j_1 )
    {
      result = *(uint8_t *)(result + 24 * v36 - 20);
      bufa[4 * v36 - 2] = result;
    }
  }
  return result;
}

BMF_SSE char * __sub_40E860(int32_t Blocka, char a2, char *a3, int32_t a4, int32_t a5, uint8_t *a6)
{
  alignas(16) uint8_t __hexrays_frame[896];
  int32_t &n128_1 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v143 = *(int32_t *)(__hexrays_frame + 0);
  char &v144 = *(char *)(__hexrays_frame + 4);
  int32_t &v145 = *(int32_t *)(__hexrays_frame + 4);
  uint32_t &v146 = *(uint32_t *)(__hexrays_frame + 8);
  int32_t &v147 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v148 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n128_12 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v150 = *(int32_t *)(__hexrays_frame + 12);
  uint8_t (&v151)[512] = *(uint8_t (*)[512])(__hexrays_frame + 16);
  uint8_t * &v152 = *(uint8_t * *)(__hexrays_frame + 528);
  uint8_t * &v153 = *(uint8_t * *)(__hexrays_frame + 532);
  uint32_t &v154 = *(uint32_t *)(__hexrays_frame + 536);
  int32_t &v155 = *(int32_t *)(__hexrays_frame + 540);
  int32_t &n128_10 = *(int32_t *)(__hexrays_frame + 544);
  int32_t &n128_3 = *(int32_t *)(__hexrays_frame + 548);
  int32_t &v158 = *(int32_t *)(__hexrays_frame + 552);
  int32_t &v159 = *(int32_t *)(__hexrays_frame + 556);
  uint8_t (&v160)[255] = *(uint8_t (*)[255])(__hexrays_frame + 560);
  char &v161 = *(char *)(__hexrays_frame + 815);
  int32_t &Blocka_2 = *(int32_t *)(__hexrays_frame + 816);
  uint8_t * &v163 = *(uint8_t * *)(__hexrays_frame + 820);
  char * &v164 = *(char * *)(__hexrays_frame + 824);
  int32_t &v165 = *(int32_t *)(__hexrays_frame + 828);
  uint8_t * &v166 = *(uint8_t * *)(__hexrays_frame + 832);
  int32_t &v167 = *(int32_t *)(__hexrays_frame + 836);
  uint32_t * &v168 = *(uint32_t * *)(__hexrays_frame + 840);
  int32_t &v169 = *(int32_t *)(__hexrays_frame + 844);
  int32_t &v170 = *(int32_t *)(__hexrays_frame + 848);
  int32_t &v171 = *(int32_t *)(__hexrays_frame + 852);
  int32_t &v172 = *(int32_t *)(__hexrays_frame + 856);
  int32_t &v173 = *(int32_t *)(__hexrays_frame + 860);
  int32_t &v174 = *(int32_t *)(__hexrays_frame + 864);
  ;
  __m128i si128, v10, v11, v12, v14, v15, v23, v24, v26, v27, v28, v29, v30, v31, v32;
  bool v47;
  char v19, v20, v21, v35, v36, v54, v56, *v57, v59, v60, *result, v69, v70, v72, *v73, v75,
       v120, v121, v122, v135, *v141;
  int32_t Blocka_1, n256, n128_2, n128_4, v18, __sub_40E860_n2, n256_1, v34, v38, n128, v40, v41,
          v43, n128_11, n128_8, n128_9, v48, n128_7, v50, n128_6, n128_5, n16, v62, v63, v65,
          v66, n16_1, v74, v76, v78, v79, v81, v82, v83, v84, v86, v87, v88, v89, v91, v92, v93,
          v94, v95, v96, v97, v98, v100, v101, v102, v103, v104, v105, v106, v107, v108, v109,
          v110, v111, v112, v113, v114, v115, v116, v117, v118, v119, n16_2, v125, v126, v127,
          v129, v130, v131, n6, v134, v136, v137, v138, v139, v140;
  uint32_t n0x70, i, j, v37, v42, k, m, *v77, *v90, *v99;
  uint8_t *v58, *v64, *v68, *v80, *v85, *v124, *v128, *v133;
  Blocka_1 = Blocka;
  n256 = __n256_0[0];
  n128_2 = 2 * __n256_0[0] + 1;
  if ( ::__n2 )
  {
    si128 = _mm_load_si128((const __m128i *)&__xmmword_439560);
    v10 = _mm_load_si128((const __m128i *)&__xmmword_439540);
    v11 = _mm_load_si128((const __m128i *)&__xmmword_439570);
    v12 = _mm_load_si128((const __m128i *)&__xmmword_439580);
    n0x70 = 0;
    v160[0] = 0;
    v161 = 0x80;
    do
    {
      v14 = v11;
      v15 = v11;
      v11 = _mm_add_epi8(v11, si128);
      *(__m128i *)&v160[2 * n0x70 + 1] = _mm_unpacklo_epi8(v14, v12);
      *(__m128i *)&v160[2 * n0x70 + 17] = _mm_unpackhi_epi8(v15, v12);
      v12 = _mm_add_epi8(v12, v10);
      n0x70 += 16;
    }
    while ( n0x70 < 0x70 );
    Blocka_2 = Blocka;
    for ( i = 0; i < 7; ++i )
    {
      v160[4 * i + 225] = -2 * i - 113;
      v160[4 * i + 226] = 2 * i + 113;
      v160[4 * i + 227] = -2 * i - 114;
      v160[4 * i + 228] = 2 * i + 114;
    }
    Blocka_1 = Blocka_2;
    v160[253] = -127;
    v160[254] = 127;
    if ( !n256 || (n128_4 = n128_2, n128_2 >= 128) )
    {
LABEL_21:
      v151[0] = 0;
      v151[128] = -1;
      if ( n256 <= 0 )
      {
        n128 = 1;
      }
      else
      {
        if ( n256 / 2 )
        {
          n128_3 = n128_2;
          Blocka_2 = Blocka_1;
          v37 = 0;
          v38 = 0;
          do
          {
            v151[v38 + 255] = 0;
            v151[2 * v37 + 1] = 0;
            v151[v38 + 254] = 0;
            n128 = 2 * v37 + 3;
            v38 -= 2;
            v151[2 * v37++ + 2] = 0;
          }
          while ( v37 < n256 / 2 );
          n128_2 = n128_3;
          Blocka_1 = Blocka_2;
          v40 = 2 * v37 + 1;
        }
        else
        {
          n128 = 1;
          v40 = 1;
        }
        if ( n256 > (uint32_t)(v40 - 1) )
        {
          v151[-v40 + 256] = 0;
          n128 = v40 + 1;
          v151[v40] = 0;
        }
        if ( n128 >= 128 )
          goto LABEL_53;
      }
      n128_12 = 1;
      v146 = 128 - n128;
      v41 = 0;
      v154 = (128 - n128) / 2;
      if ( v154 )
      {
        n128_1 = n128;
        v42 = 0;
        n128_3 = n128_2;
        v43 = 0;
        Blocka_2 = Blocka_1;
        v153 = &v151[n128];
        v152 = &v151[-n128];
        n128_11 = 1;
        do
        {
          n128_8 = n128_3;
          n128_9 = n128_11 - 1;
          if ( n128_9 )
            n128_8 = n128_9;
          n128_10 = n128_8;
          if ( !n128_9 )
            ++v41;
          v144 = 2 * v41;
          v153[2 * v42] = 2 * v41;
          v155 = (uint8_t)(2 * v41 - 1);
          n128_11 = n128_10 - 1;
          v47 = n128_10 == 1;
          v152[v43 + 256] = v155;
          if ( v47 )
          {
            n128_11 = n128_3;
            v144 = 2 * ++v41;
            v155 = (uint8_t)(2 * v41 - 1);
          }
          v153[2 * v42++ + 1] = v144;
          v152[v43 + 255] = v155;
          v43 -= 2;
        }
        while ( v42 < v154 );
        n128 = n128_1;
        Blocka_1 = Blocka_2;
        n128_12 = n128_11;
        v48 = 2 * v42 + 1;
      }
      else
      {
        v48 = 1;
      }
      if ( v48 - 1 < v146 )
      {
        Blocka_2 = Blocka_1;
        if ( n128_12 == 1 )
          LOBYTE(v41) = v41 + 1;
        v151[n128 - 1 + v48] = 2 * v41;
        v151[-n128 - v48 + 257] = 2 * v41 - 1;
        Blocka_1 = Blocka_2;
      }
      goto LABEL_53;
    }
    n128_3 = n128_2;
    v18 = 1;
    do
    {
      v19 = v160[2 * v18];
      v160[2 * v18] = v160[2 * n128_4];
      v20 = v160[2 * n128_4 - 1];
      v160[2 * n128_4] = v19;
      v21 = v160[2 * v18 - 1];
      v160[2 * v18 - 1] = v20;
      v160[2 * n128_4 - 1] = v21;
      n128_4 += n128_3;
      ++v18;
    }
    while ( n128_4 < 128 );
    n128_2 = n128_3;
    Blocka_1 = Blocka_2;
    __sub_40E860_n2 = ::__n2;
  }
  else
  {
    v23 = _mm_load_si128((const __m128i *)&__xmmword_439540);
    v24 = _mm_load_si128((const __m128i *)&__xmmword_439550);
    for ( j = 0; j < 0x100; j += 128 )
    {
      *(__m128i *)&v160[j] = v24;
      v26 = _mm_add_epi8(v24, v23);
      *(__m128i *)&v160[j + 16] = v26;
      v27 = _mm_add_epi8(v26, v23);
      *(__m128i *)&v160[j + 32] = v27;
      v28 = _mm_add_epi8(v27, v23);
      *(__m128i *)&v160[j + 48] = v28;
      v29 = _mm_add_epi8(v28, v23);
      *(__m128i *)&v160[j + 64] = v29;
      v30 = _mm_add_epi8(v29, v23);
      *(__m128i *)&v160[j + 80] = v30;
      v31 = _mm_add_epi8(v30, v23);
      *(__m128i *)&v160[j + 96] = v31;
      v32 = _mm_add_epi8(v31, v23);
      *(__m128i *)&v160[j + 112] = v32;
      v24 = _mm_add_epi8(v32, v23);
    }
    n256_1 = n256;
    v34 = 0;
    if ( n256 < 256 )
    {
      do
      {
        v35 = v160[v34];
        v160[v34++] = v160[n256];
        v160[n256] = v35;
        n256 += n128_2;
      }
      while ( n256 < 256 );
      n256_1 = __n256_0[0];
    }
    if ( n256 - n256_1 < 256 )
    {
      v36 = v160[v34];
      v160[v34] = v161;
      v161 = v36;
    }
    __sub_40E860_n2 = ::__n2;
  }
  if ( __sub_40E860_n2 )
  {
    n256 = __n256_0[0];
    goto LABEL_21;
  }
  Blocka_2 = Blocka_1;
  n128_7 = n128_2 + 1;
  v50 = 0;
  for ( k = 0; k < 0x80; ++k )
  {
    n128_6 = n128_7 - 1;
    n128_5 = n128_2;
    if ( n128_6 )
      n128_5 = n128_6;
    else
      ++v50;
    v54 = v50;
    v151[2 * k] = v50;
    n128_7 = n128_5 - 1;
    if ( n128_5 == 1 )
    {
      n128_7 = n128_2;
      v54 = ++v50;
    }
    v151[2 * k + 1] = v54;
  }
  Blocka_1 = Blocka_2;
LABEL_53:
  for ( m = 0; m < 0x80; ++m )
  {
    v151[(uint8_t)v160[2 * m] + 256] = 2 * m;
    v151[(uint8_t)v160[2 * m + 1] + 256] = 2 * m + 1;
  }
  v56 = *a3;
  v163 = a6 + 1;
  *a6 = v56;
  v164 = a3 + 1;
  if ( a4 <= 1 )
  {
    v64 = *(uint8_t **)(Blocka_1 + 68);
  }
  else
  {
    v57 = a3 + 1;
    v58 = v163;
    v147 = 1;
    do
    {
      v59 = *(v58 - 1);
      v164 = v57;
      v145 = (uint8_t)(*v57 - v59);
      v150 = (uint8_t)v151[v145];
      v60 = v160[v150] + v59;
      n16 = (uint8_t)*v58 - (uint8_t)(v60 + *v58 - *v57);
      if ( n16 < -16 || n16 > 16 )
      {
        LOBYTE(v150) = v151[v145 + 256];
        *v58 = *v57;
      }
      else
      {
        *v58 = v60;
      }
      *v57 = v150;
      **(uint8_t **)(Blocka_1 + 68) = *v58;
      v62 = *(uint32_t *)(Blocka_1
                      + 4
                      * (**(uint8_t **)(Blocka_1 + 68) - *(uint8_t *)(*(uint32_t *)(Blocka_1 + 68) - 1))
                      + 8364);
      *(uint32_t *)(*(uint32_t *)(Blocka_1 + 92) + 4) = v62;
      **(uint32_t **)(Blocka_1 + 92) = v62;
      v63 = *(uint32_t *)(Blocka_1 + 68);
      ++v57;
      *(uint32_t *)(Blocka_1 + 92) += 8;
      ++v147;
      ++v58;
      v64 = (uint8_t *)(v63 + 1);
      *(uint32_t *)(Blocka_1 + 68) = v64;
    }
    while ( v147 < a4 );
    v164 = v57;
    v163 = v58;
  }
  v65 = (uint8_t)*(v64 - 1);
  *v64 = v65;
  v66 = *(uint32_t *)(Blocka_1 + 92);
  *(uint32_t *)(Blocka_1 + 64) = v65;
  *(uint32_t *)(Blocka_1 + 84) = *(uint32_t *)(v66 - 8);
  *(uint32_t *)(Blocka_1 + 88) = *(uint32_t *)(v66 - 4);
  result = memcpy(*(char **)(Blocka_1 + 80),*(char **)(Blocka_1 + 76),a4 + 2);
  if ( a5 != 1 )
  {
    v143 = a5 - 1;
    v68 = v163;
    v155 = Blocka_1 + 1148;
    n128_3 = Blocka_1 + 2188;
    v154 = Blocka_1 + 3228;
    n128_10 = Blocka_1 + 108;
    do
    {
      v69 = v68[-a4];
      v148 = (uint8_t)(*v164 - v69);
      v70 = v160[(uint8_t)v151[v148]] + v69;
      n16_1 = (uint8_t)*v68 - (uint8_t)(v70 + *v68 - *v164);
      v72 = v151[v148];
      if ( n16_1 < -16 || n16_1 > 16 )
      {
        *v68 = *v164;
        v72 = v151[v148 + 256];
      }
      else
      {
        *v68 = v70;
      }
      v73 = v164;
      *v164 = v72;
      v74 = (uint8_t)*v68;
      v75 = *(uint8_t *)(Blocka_1 + 64);
      *(uint8_t *)(*(uint32_t *)(Blocka_1 + 68) - 1) = v75;
      **(uint8_t **)(Blocka_1 + 68) = v75;
      v76 = *(uint32_t *)(Blocka_1 + 92);
      *(uint32_t *)(v76 - 8) = *(uint32_t *)(Blocka_1 + 84);
      *(uint32_t *)(v76 - 4) = *(uint32_t *)(Blocka_1 + 88);
      v77 = *(uint32_t **)(Blocka_1 + 92);
      *v77 = *(v77 - 2);
      v77[1] = *(v77 - 1);
      v78 = *(uint32_t *)(Blocka_1 + 76);
      v79 = *(uint32_t *)(Blocka_1 + 80);
      *(uint32_t *)(Blocka_1 + 76) = v79;
      *(uint32_t *)(Blocka_1 + 80) = v78;
      v79 += 2;
      *(uint32_t *)(Blocka_1 + 68) = v79;
      *(uint32_t *)(Blocka_1 + 72) = v78 + 2;
      *(uint32_t *)(Blocka_1 + 64) = v74;
      *(uint8_t *)(v79 - 2) = v74;
      v80 = *(uint8_t **)(Blocka_1 + 72);
      *(uint32_t *)(Blocka_1 + 92) = *(uint32_t *)(Blocka_1 + 96) + 8;
      v81 = -*(uint32_t *)(Blocka_1 + 4 * (v74 - *(v80 - 1)) + 8364);
      *(uint32_t *)(Blocka_1 + 88) = 0;
      *(uint32_t *)(Blocka_1 + 84) = v81;
      v164 = v73 + 1;
      ++v68;
      if ( a4 != 1 )
      {
        v165 = a4 - 1;
        v163 = v68;
        while ( 1 )
        {
          v82 = *(uint32_t *)(Blocka_1 + 64);
          v83 = *v80;
          v84 = *(v80 - 1);
          Blocka_2 = Blocka_1;
          v85 = *(uint8_t **)(Blocka_1 + 68);
          v86 = *v85;
          v167 = v82;
          v87 = v84 + v86;
          v88 = v80[1];
          v171 = v83;
          v166 = v85;
          v89 = v82 + v83;
          v172 = (16 * (v89 + v88) + -3 * (v87 - *(v85 - 2)) + 8 * v89 + 4 * v82 - v171 + 31) >> 6;
          v90 = *(uint32_t **)(Blocka_2 + 92);
          v91 = *(v90 - 2) + *(uint32_t *)(Blocka_2 + 84);
          v92 = v90[2] + *v90;
          v168 = v90;
          Blocka_1 = Blocka_2;
          v93 = v92 + v91;
          v94 = v172;
          if ( v93 <= 0 )
          {
            v158 = -v93;
            *(uint32_t *)(Blocka_2 + 100) = v155;
            v96 = v158;
            v95 = v167 + ((*v80 - *(v80 - 1) + 2) >> 2);
          }
          else
          {
            v158 = v93;
            *(uint32_t *)(Blocka_2 + 100) = n128_10;
            v95 = *v80 + ((v89 - v87 + 4) >> 3);
            v96 = v158;
          }
          v173 = v89;
          *(uint32_t *)(Blocka_1 + 12) = v94;
          *(uint32_t *)(Blocka_1 + 28) = v95;
          v97 = (v94 + v95) >> 1;
          *(uint32_t *)(Blocka_1 + 20) = v97;
          *(uint32_t *)(Blocka_1 + 16) = (v97 + v94) >> 1;
          v98 = (v97 + v95) >> 1;
          v99 = v168;
          v100 = v173;
          *(uint32_t *)(Blocka_1 + 24) = v98;
          v101 = *(uint8_t *)(*(uint32_t *)(Blocka_1 + 100) + v96 + 16);
          *(uint32_t *)Blocka_1 = v101;
          v102 = *(uint32_t *)(Blocka_1 + 4 * v101 + 8);
          v103 = *(uint32_t *)(Blocka_1 + 88) + v99[3] + v99[1];
          if ( v103 < 0 )
          {
            v159 = -v103;
            *(uint32_t *)(Blocka_1 + 104) = v154;
            v112 = *(v80 - 1);
            v113 = *(v80 - 2);
            v170 = 2 * v102;
            v114 = 2 * v112 - *(v166 - 1) + v100 - 2 * v102 - v113 + 2;
            v109 = v159;
            v110 = v114 >> 2;
          }
          else
          {
            Blocka_2 = Blocka_1;
            v159 = v103;
            *(uint32_t *)(Blocka_1 + 104) = n128_3;
            v104 = *(v80 - 1);
            v105 = v80[1];
            v106 = *v80;
            v170 = 2 * v102;
            v107 = 2 * v105 - 2 * v102 - v166[1] - (v104 - v105);
            Blocka_1 = Blocka_2;
            v108 = v107 + v106 + 4;
            v109 = v159;
            v110 = v108 >> 3;
          }
          v111 = v102 + v110;
          *(uint32_t *)(Blocka_1 + 40) = v102;
          *(uint32_t *)(Blocka_1 + 56) = v111;
          v115 = v102 + v111 + 1;
          *(uint32_t *)(Blocka_1 + 44) = (v115 + v170) >> 2;
          *(uint32_t *)(Blocka_1 + 48) = v115 >> 1;
          v116 = v115 + 2 * v111;
          v117 = *(uint32_t *)(Blocka_1 + 104);
          *(uint32_t *)(Blocka_1 + 52) = v116 >> 2;
          v118 = *(uint8_t *)(v117 + v109 + 16);
          *(uint32_t *)(Blocka_1 + 4) = v118;
          LOBYTE(v116) = *(uint8_t *)(Blocka_1 + 4 * v118 + 36);
          v169 = (uint8_t)*v164;
          v119 = (uint8_t)(v169 - v116);
          v120 = v160[(uint8_t)v151[v119]];
          v174 = (uint8_t)v151[v119];
          v121 = v120 + v116;
          v122 = v174;
          n16_2 = (uint8_t)*v163 - (uint8_t)(v121 + *v163 - v169);
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            *v163 = v169;
            v122 = v151[v119 + 256];
          }
          else
          {
            *v163 = v121;
          }
          v124 = v163;
          *v164 = v122;
          v125 = *v124;
          v126 = *(uint32_t *)(Blocka_1 + 92);
          *(uint32_t *)(v126 - 8) = *(uint32_t *)(Blocka_1 + 84);
          *(uint32_t *)(v126 - 4) = *(uint32_t *)(Blocka_1 + 88);
          v127 = *(uint32_t *)(Blocka_1 + 64);
          v128 = *(uint8_t **)(Blocka_1 + 72);
          *(uint32_t *)(Blocka_1 + 84) = *(uint32_t *)(Blocka_1 + 4 * (v125 - v127) + 8364)
                                     - *(uint32_t *)(Blocka_1 + 4 * (v125 - *v128) + 8364);
          v129 = *(uint32_t *)(Blocka_1 + 4 * (v125 - *(v128 - 1)) + 8364)
               - *(uint32_t *)(Blocka_1 + 4 * (v125 - v128[1]) + 8364);
          v130 = *(uint32_t *)(Blocka_1 + 68);
          *(uint32_t *)(Blocka_1 + 88) = v129;
          *(uint8_t *)(v130 - 1) = v127;
          v131 = *(uint32_t *)(Blocka_1 + 4);
          *(uint32_t *)(Blocka_1 + 64) = v125;
          ++*(uint32_t *)(Blocka_1 + 68);
          ++*(uint32_t *)(Blocka_1 + 72);
          *(uint32_t *)(Blocka_1 + 92) += 8;
          n6 = *(uint32_t *)(Blocka_1 + 4 * (v125 - *(uint32_t *)(Blocka_1 + 4 * v131 + 36)) + 8364);
          if ( n6 >= 6 )
          {
            v133 = *(uint8_t **)Blocka_1;
            v152 = (uint8_t *)(*(uint32_t *)(Blocka_1 + 4 * (v125 - *(uint32_t *)(Blocka_1 + 40)) + 8364) - 1);
            v153 = v133;
            if ( (int32_t)v152 <= *(uint32_t *)(Blocka_1 + 4 * (v125 - *(uint32_t *)(Blocka_1 + 4 * (uint32_t)v133 + 4)) + 8364) )
            {
              if ( (int32_t)v152 <= *(uint32_t *)(Blocka_1 + 4 * (v125 - *(uint32_t *)(Blocka_1 + 4 * (uint32_t)v153 + 12)) + 8364) )
              {
                v137 = n6 - 1;
                if ( v137 <= *(uint32_t *)(Blocka_1 + 4 * (v125 - *(uint32_t *)(Blocka_1 + 4 * v131 + 32)) + 8364) )
                {
                  if ( v137 > *(uint32_t *)(Blocka_1 + 4 * (v125 - *(uint32_t *)(Blocka_1 + 4 * v131 + 40)) + 8364) )
                  {
                    v138 = *(uint32_t *)(Blocka_1 + 104);
                    v139 = *(uint32_t *)(v138 + 4 * v131 - 4) - 1;
                    *(uint32_t *)(v138 + 4 * v131 - 4) = v139;
                    *(uint8_t *)(*(uint32_t *)(Blocka_1 + 104) + v139 + 16) = v131 + 1;
                  }
                }
                else
                {
                  *(uint8_t *)(*(uint32_t *)(*(uint32_t *)(Blocka_1 + 104) + 4 * v131 - 8) + *(uint32_t *)(Blocka_1 + 104) + 16) = v131 - 1;
                  ++*(uint32_t *)(*(uint32_t *)(Blocka_1 + 104) + 4 * *(uint32_t *)(Blocka_1 + 4) - 8);
                }
              }
              else
              {
                v134 = *(uint32_t *)(Blocka_1 + 100);
                v135 = (uint8_t)v153 + 1;
                v136 = *(uint32_t *)(v134 + 4 * (uint32_t)v153 - 4) - 1;
                *(uint32_t *)(v134 + 4 * (uint32_t)v153 - 4) = v136;
                *(uint8_t *)(*(uint32_t *)(Blocka_1 + 100) + v136 + 16) = v135;
              }
            }
            else
            {
              *(uint8_t *)(*(uint32_t *)(*(uint32_t *)(Blocka_1 + 100) + 4 * (uint32_t)v153 - 8)
                       + *(uint32_t *)(Blocka_1 + 100)
                       + 16) = (uint8_t)v153 - 1;
              ++*(uint32_t *)(*(uint32_t *)(Blocka_1 + 100) + 4 * *(uint32_t *)Blocka_1 - 8);
            }
          }
          v140 = v165;
          v141 = v164 + 1;
          ++*(uint32_t *)&__buf[4 * (uint8_t)*v164];
          v164 = v141;
          ++v163;
          v165 = v140 - 1;
          if ( v140 == 1 )
            break;
          v80 = *(uint8_t **)(Blocka_1 + 72);
        }
        v68 = v163;
      }
      result = (char *)--v143;
    }
    while ( v143 );
  }
  return result;
}

 BMF_SSE uint8_t *__sub_410AC0(uint8_t *Src, int32_t i, int32_t a3, uint8_t *a4)
{
  alignas(16) uint8_t __hexrays_frame[848];
  uint8_t * &v80 = *(uint8_t * *)(__hexrays_frame + 0);
  int32_t &v81 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v82 = *(int32_t *)(__hexrays_frame + 4);
  char &v83 = *(char *)(__hexrays_frame + 8);
  int32_t &v84 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v85 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n128_11 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v87 = *(int32_t *)(__hexrays_frame + 12);
  uint8_t (&v88)[512] = *(uint8_t (*)[512])(__hexrays_frame + 16);
  uint8_t * &v89 = *(uint8_t * *)(__hexrays_frame + 528);
  uint8_t * &v90 = *(uint8_t * *)(__hexrays_frame + 532);
  uint32_t &v91 = *(uint32_t *)(__hexrays_frame + 536);
  uint8_t * &v92 = *(uint8_t * *)(__hexrays_frame + 540);
  int32_t &n128_9 = *(int32_t *)(__hexrays_frame + 544);
  int32_t &n128_2 = *(int32_t *)(__hexrays_frame + 548);
  uint8_t (&v96)[255] = *(uint8_t (*)[255])(__hexrays_frame + 560);
  char &v97 = *(char *)(__hexrays_frame + 815);
  ;
  __m128i si128, v7, v8, v9, v11, v12, v20, v21, v23, v24, v25, v26, v27, v28, v29;
  bool v45, v70;
  char v16, v17, v18, v32, v33, v52, v57, v59, v62, v63, v64, v69, v71, v72, v75;
  int32_t n256, n128_1, n128_3, v15, __sub_410AC0_n2, n256_1, v31, v35, n128, v37, v39, v41,
          n128_10, n128_7, n128_8, v46, n128_6, v48, n128_5, n128_4, v58, n16, n16_1, v66, v67,
          v68, v74, n16_2;
  uint32_t n0x70, j, k, v34, v38, v40, m, n, v73, v78;
  uint8_t *v54, *result, *v56, *v61, *v77, *v79;
  n256 = __n256_0[0];
  n128_1 = 2 * __n256_0[0] + 1;
  if ( ::__n2 )
  {
    si128 = _mm_load_si128((const __m128i *)&__xmmword_439720);
    v7 = _mm_load_si128((const __m128i *)&__xmmword_439700);
    v8 = _mm_load_si128((const __m128i *)&__xmmword_439730);
    v9 = _mm_load_si128((const __m128i *)&__xmmword_439740);
    n0x70 = 0;
    v96[0] = 0;
    v97 = 0x80;
    do
    {
      v11 = v8;
      v12 = v8;
      v8 = _mm_add_epi8(v8, si128);
      *(__m128i *)&v96[2 * n0x70 + 1] = _mm_unpacklo_epi8(v11, v9);
      *(__m128i *)&v96[2 * n0x70 + 17] = _mm_unpackhi_epi8(v12, v9);
      v9 = _mm_add_epi8(v9, v7);
      n0x70 += 16;
    }
    while ( n0x70 < 0x70 );
    for ( j = 0; j < 7; ++j )
    {
      v96[4 * j + 225] = -2 * j - 113;
      v96[4 * j + 226] = 2 * j + 113;
      v96[4 * j + 227] = -2 * j - 114;
      v96[4 * j + 228] = 2 * j + 114;
    }
    v96[253] = -127;
    v96[254] = 127;
    if ( !n256 || (n128_3 = n128_1, n128_1 >= 128) )
    {
LABEL_21:
      v88[0] = 0;
      v88[128] = -1;
      if ( n256 <= 0 )
      {
        n128 = 1;
      }
      else
      {
        if ( n256 / 2 )
        {
          n128_2 = n128_1;
          v34 = 0;
          v35 = 0;
          do
          {
            v88[v35 + 255] = 0;
            v88[2 * v34 + 1] = 0;
            v88[v35 + 254] = 0;
            n128 = 2 * v34 + 3;
            v35 -= 2;
            v88[2 * v34++ + 2] = 0;
          }
          while ( v34 < n256 / 2 );
          n128_1 = n128_2;
          v37 = 2 * v34 + 1;
        }
        else
        {
          n128 = 1;
          v37 = 1;
        }
        if ( n256 > (uint32_t)(v37 - 1) )
        {
          v88[-v37 + 256] = 0;
          v88[v37] = 0;
          n128 = v37 + 1;
        }
        if ( n128 >= 128 )
          goto LABEL_52;
      }
      n128_11 = 1;
      v38 = 128 - n128;
      v91 = (128 - n128) / 2;
      v39 = 0;
      if ( v91 )
      {
        v40 = 0;
        v41 = 0;
        v90 = &v88[n128];
        n128_2 = n128_1;
        v89 = &v88[-n128];
        n128_10 = 1;
        do
        {
          n128_7 = n128_2;
          n128_8 = n128_10 - 1;
          if ( n128_8 )
            n128_7 = n128_8;
          n128_9 = n128_7;
          if ( !n128_8 )
            ++v39;
          v83 = 2 * v39;
          v90[2 * v40] = 2 * v39;
          v92 = (uint8_t *)(uint8_t)(2 * v39 - 1);
          n128_10 = n128_9 - 1;
          v45 = n128_9 == 1;
          v89[v41 + 256] = (uint8_t)v92;
          if ( v45 )
          {
            n128_10 = n128_2;
            v83 = 2 * ++v39;
            v92 = (uint8_t *)(uint8_t)(2 * v39 - 1);
          }
          v90[2 * v40++ + 1] = v83;
          v89[v41 + 255] = (uint8_t)v92;
          v41 -= 2;
        }
        while ( v40 < v91 );
        v38 = 128 - n128;
        n128_11 = n128_10;
        v46 = 2 * v40 + 1;
      }
      else
      {
        v46 = 1;
      }
      if ( v38 > v46 - 1 )
      {
        if ( n128_11 == 1 )
          LOBYTE(v39) = v39 + 1;
        v88[n128 - 1 + v46] = 2 * v39;
        v88[-n128 - v46 + 257] = 2 * v39 - 1;
      }
      goto LABEL_52;
    }
    n128_2 = n128_1;
    v15 = 1;
    do
    {
      v16 = v96[2 * v15];
      v96[2 * v15] = v96[2 * n128_3];
      v17 = v96[2 * n128_3 - 1];
      v96[2 * n128_3] = v16;
      v18 = v96[2 * v15 - 1];
      v96[2 * v15 - 1] = v17;
      v96[2 * n128_3 - 1] = v18;
      n128_3 += n128_2;
      ++v15;
    }
    while ( n128_3 < 128 );
    n128_1 = n128_2;
    __sub_410AC0_n2 = ::__n2;
  }
  else
  {
    v20 = _mm_load_si128((const __m128i *)&__xmmword_439700);
    v21 = _mm_load_si128((const __m128i *)&__xmmword_439710);
    for ( k = 0; k < 0x100; k += 128 )
    {
      *(__m128i *)&v96[k] = v21;
      v23 = _mm_add_epi8(v21, v20);
      *(__m128i *)&v96[k + 16] = v23;
      v24 = _mm_add_epi8(v23, v20);
      *(__m128i *)&v96[k + 32] = v24;
      v25 = _mm_add_epi8(v24, v20);
      *(__m128i *)&v96[k + 48] = v25;
      v26 = _mm_add_epi8(v25, v20);
      *(__m128i *)&v96[k + 64] = v26;
      v27 = _mm_add_epi8(v26, v20);
      *(__m128i *)&v96[k + 80] = v27;
      v28 = _mm_add_epi8(v27, v20);
      *(__m128i *)&v96[k + 96] = v28;
      v29 = _mm_add_epi8(v28, v20);
      *(__m128i *)&v96[k + 112] = v29;
      v21 = _mm_add_epi8(v29, v20);
    }
    n256_1 = n256;
    v31 = 0;
    if ( n256 < 256 )
    {
      do
      {
        v32 = v96[v31];
        v96[v31++] = v96[n256];
        v96[n256] = v32;
        n256 += n128_1;
      }
      while ( n256 < 256 );
      n256_1 = __n256_0[0];
    }
    if ( n256 - n256_1 < 256 )
    {
      v33 = v96[v31];
      v96[v31] = v97;
      __sub_410AC0_n2 = ::__n2;
      v97 = v33;
    }
    else
    {
      __sub_410AC0_n2 = ::__n2;
    }
  }
  if ( __sub_410AC0_n2 )
  {
    n256 = __n256_0[0];
    goto LABEL_21;
  }
  n128_6 = n128_1 + 1;
  v48 = 0;
  for ( m = 0; m < 0x80; ++m )
  {
    n128_5 = n128_6 - 1;
    n128_4 = n128_1;
    if ( n128_5 )
      n128_4 = n128_5;
    else
      ++v48;
    v52 = v48;
    v88[2 * m] = v48;
    n128_6 = n128_4 - 1;
    if ( n128_4 == 1 )
    {
      n128_6 = n128_1;
      v52 = ++v48;
    }
    v88[2 * m + 1] = v52;
  }
LABEL_52:
  for ( n = 0; n < 0x80; ++n )
  {
    v88[(uint8_t)v96[2 * n] + 256] = 2 * n;
    v88[(uint8_t)v96[2 * n + 1] + 256] = 2 * n + 1;
  }
  *a4 = *Src;
  v54 = a4 + 1;
  result = Src + 1;
  v56 = (uint8_t *)(i - 1);
  if ( i != 1 )
  {
    v84 = i - 1;
    do
    {
      v57 = *(v54 - 1);
      v58 = (uint8_t)v88[(uint8_t)(*result - v57)];
      v81 = (uint8_t)(*result - v57);
      v59 = v96[v58] + v57;
      n16 = (uint8_t)*v54 - (uint8_t)(v59 + *v54 - *result);
      if ( n16 < -16 || n16 > 16 )
      {
        LOBYTE(v58) = v88[v81 + 256];
        *v54 = *result;
      }
      else
      {
        *v54 = v59;
      }
      *result++ = v58;
      ++v54;
      --v84;
    }
    while ( v84 );
    v56 = (uint8_t *)(i - 1);
  }
  v61 = &v54[-i];
  if ( a3 != 1 )
  {
    v82 = a3 - 1;
    v80 = v56;
    do
    {
      v62 = *result;
      v92 = v61;
      v63 = v54[-i];
      v85 = (uint8_t)(v62 - v63);
      v87 = (uint8_t)v88[v85];
      v64 = v96[v87] + v63;
      n16_1 = (uint8_t)*v54 - (uint8_t)(v64 + *v54 - v62);
      if ( n16_1 < -16 || n16_1 > 16 )
      {
        v87 = (uint8_t)v88[v85 + 256];
        *v54 = v62;
      }
      else
      {
        *v54 = v64;
      }
      *result++ = v87;
      ++v61;
      ++v54;
      ++*(uint32_t *)&__buf[4 * v87];
      if ( i != 1 )
      {
        v89 = v80;
        do
        {
          v66 = (uint8_t)*(v54 - 1);
          v67 = *v61;
          v68 = (uint8_t)v54[-i - 1];
          if ( v66 < v67 )
          {
            if ( v68 >= v66 )
            {
              v92 = v61;
              v71 = v66 + v67 - v68;
              v70 = v68 <= v67;
              LOBYTE(v67) = v66;
              if ( v70 )
                LOBYTE(v67) = v71;
              v61 = v92;
            }
          }
          else if ( v68 <= v66 )
          {
            v92 = v61;
            v69 = v66 + v67 - v68;
            v70 = v68 < v67;
            LOBYTE(v67) = v66;
            if ( !v70 )
              LOBYTE(v67) = v69;
            v61 = v92;
          }
          v72 = *result;
          v92 = v61;
          v73 = (uint8_t)v88[(uint8_t)(v72 - v67)];
          v90 = (uint8_t *)(uint8_t)(v72 - v67);
          v74 = (uint8_t)*v54;
          v91 = v73;
          v75 = v96[v73] + v67;
          n16_2 = v74 - (uint8_t)(v75 + v74 - v72);
          v77 = v92;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            v91 = (uint8_t)v88[(uint32_t)v90 + 256];
            *v54 = v72;
          }
          else
          {
            *v54 = v75;
          }
          v78 = v91;
          v79 = v89;
          *result++ = v91;
          v61 = v77 + 1;
          ++*(uint32_t *)&__buf[4 * v78];
          ++v54;
          v89 = v79 - 1;
        }
        while ( v79 != (uint8_t *)1 );
      }
      --v82;
    }
    while ( v82 );
  }
  return result;
}

 BMF_SSE uint32_t __sub_4111B0(uint8_t *Src, int32_t i, int32_t a3, uint8_t *a4)
{
  alignas(16) uint8_t __hexrays_frame[848];
  char &v57 = *(char *)(__hexrays_frame + 0);
  char &v58 = *(char *)(__hexrays_frame + 4);
  uint8_t * &Src_2 = *(uint8_t * *)(__hexrays_frame + 8);
  uint8_t * &Src_3 = *(uint8_t * *)(__hexrays_frame + 8);
  uint8_t * &Src_4 = *(uint8_t * *)(__hexrays_frame + 8);
  int32_t &n128_11 = *(int32_t *)(__hexrays_frame + 16);
  uint8_t (&v63)[512] = *(uint8_t (*)[512])(__hexrays_frame + 24);
  uint8_t * &v64 = *(uint8_t * *)(__hexrays_frame + 536);
  uint8_t * &v65 = *(uint8_t * *)(__hexrays_frame + 540);
  uint32_t &v66 = *(uint32_t *)(__hexrays_frame + 544);
  int32_t &v67 = *(int32_t *)(__hexrays_frame + 548);
  int32_t &n128_9 = *(int32_t *)(__hexrays_frame + 552);
  int32_t &n128_2 = *(int32_t *)(__hexrays_frame + 556);
  uint8_t (&v70)[255] = *(uint8_t (*)[255])(__hexrays_frame + 560);
  char &v71 = *(char *)(__hexrays_frame + 815);
  uint8_t * &Src_5 = *(uint8_t * *)(__hexrays_frame + 816);
  ;
  __m128i si128, v8, v9, v10, v12, v13, v21, v22, v24, v25, v26, v27, v28, v29, v30;
  bool v46;
  char v17, v18, v19, v34, v35, v53;
  int32_t n256, n128_1, n128_3, v16, __sub_4111B0_n2, n256_1, v32, v33, v37, n128, v39, v40,
          n128_10, v43, n128_7, n128_8, v47, n128_6, v49, n128_5, n128_4, n64;
  uint32_t n0x70, j, k, v36, v41, m, n0x80;
  uint8_t *Src_1;
  Src_1 = Src;
  n256 = __n256_0[0];
  Src_5 = &Src[a3 * i];
  n128_1 = 2 * __n256_0[0] + 1;
  if ( ::__n2 )
  {
    si128 = _mm_load_si128((const __m128i *)&__xmmword_439770);
    v8 = _mm_load_si128((const __m128i *)&__xmmword_439750);
    v9 = _mm_load_si128((const __m128i *)&__xmmword_439780);
    v10 = _mm_load_si128((const __m128i *)&__xmmword_439790);
    n0x70 = 0;
    v70[0] = 0;
    v71 = 0x80;
    do
    {
      v12 = v9;
      v13 = v9;
      v9 = _mm_add_epi8(v9, si128);
      *(__m128i *)&v70[2 * n0x70 + 1] = _mm_unpacklo_epi8(v12, v10);
      *(__m128i *)&v70[2 * n0x70 + 17] = _mm_unpackhi_epi8(v13, v10);
      v10 = _mm_add_epi8(v10, v8);
      n0x70 += 16;
    }
    while ( n0x70 < 0x70 );
    for ( j = 0; j < 7; ++j )
    {
      v70[4 * j + 225] = -2 * j - 113;
      v70[4 * j + 227] = -2 * j - 114;
      v70[4 * j + 226] = 2 * j + 113;
      v70[4 * j + 228] = 2 * j + 114;
    }
    v70[253] = -127;
    v70[254] = 127;
    if ( !n256 || (n128_3 = n128_1, n128_1 >= 128) )
    {
LABEL_22:
      v63[0] = 0;
      v63[128] = -1;
      if ( n256 <= 0 )
      {
        n128 = 1;
      }
      else
      {
        if ( n256 / 2 )
        {
          n128_2 = n128_1;
          Src_2 = Src_1;
          v36 = 0;
          v37 = 0;
          do
          {
            v63[v37 + 255] = 0;
            v63[2 * v36 + 1] = 0;
            v63[v37 + 254] = 0;
            n128 = 2 * v36 + 3;
            v37 -= 2;
            v63[2 * v36++ + 2] = 0;
          }
          while ( v36 < n256 / 2 );
          n128_1 = n128_2;
          Src_1 = Src_2;
          v39 = 2 * v36 + 1;
        }
        else
        {
          n128 = 1;
          v39 = 1;
        }
        if ( n256 > (uint32_t)(v39 - 1) )
        {
          v63[-v39 + 256] = 0;
          v63[v39] = 0;
          n128 = v39 + 1;
        }
        if ( n128 >= 128 )
          goto LABEL_54;
      }
      n128_11 = 1;
      v40 = 0;
      v66 = (128 - n128) / 2;
      if ( v66 )
      {
        v41 = 0;
        n128_2 = n128_1;
        Src_3 = Src_1;
        n128_10 = 1;
        v65 = &v63[n128];
        v64 = &v63[-n128];
        v43 = 0;
        do
        {
          n128_7 = n128_2;
          n128_8 = n128_10 - 1;
          if ( n128_8 )
            n128_7 = n128_8;
          n128_9 = n128_7;
          if ( !n128_8 )
            ++v40;
          v58 = 2 * v40;
          v65[2 * v41] = 2 * v40;
          v67 = (uint8_t)(2 * v40 - 1);
          n128_10 = n128_9 - 1;
          v46 = n128_9 == 1;
          v64[v43 + 256] = v67;
          if ( v46 )
          {
            n128_10 = n128_2;
            v58 = 2 * ++v40;
            v67 = (uint8_t)(2 * v40 - 1);
          }
          v65[2 * v41++ + 1] = v58;
          v64[v43 + 255] = v67;
          v43 -= 2;
        }
        while ( v41 < v66 );
        n128_11 = n128_10;
        Src_1 = Src_3;
        v47 = 2 * v41 + 1;
      }
      else
      {
        v47 = 1;
      }
      if ( v47 - 1 < (uint32_t)(128 - n128) )
      {
        if ( n128_11 == 1 )
          LOBYTE(v40) = v40 + 1;
        v63[n128 - 1 + v47] = 2 * v40;
        v63[-n128 - v47 + 257] = 2 * v40 - 1;
      }
      goto LABEL_54;
    }
    n128_2 = n128_1;
    v16 = 1;
    do
    {
      v17 = v70[2 * v16];
      v70[2 * v16] = v70[2 * n128_3];
      v18 = v70[2 * n128_3 - 1];
      v70[2 * n128_3] = v17;
      v19 = v70[2 * v16 - 1];
      v70[2 * v16 - 1] = v18;
      v70[2 * n128_3 - 1] = v19;
      n128_3 += n128_2;
      ++v16;
    }
    while ( n128_3 < 128 );
    n128_1 = n128_2;
    Src_1 = Src;
    __sub_4111B0_n2 = ::__n2;
  }
  else
  {
    v21 = _mm_load_si128((const __m128i *)&__xmmword_439750);
    v22 = _mm_load_si128((const __m128i *)&__xmmword_439760);
    for ( k = 0; k < 0x100; k += 128 )
    {
      *(__m128i *)&v70[k] = v22;
      v24 = _mm_add_epi8(v22, v21);
      *(__m128i *)&v70[k + 16] = v24;
      v25 = _mm_add_epi8(v24, v21);
      *(__m128i *)&v70[k + 32] = v25;
      v26 = _mm_add_epi8(v25, v21);
      *(__m128i *)&v70[k + 48] = v26;
      v27 = _mm_add_epi8(v26, v21);
      *(__m128i *)&v70[k + 64] = v27;
      v28 = _mm_add_epi8(v27, v21);
      *(__m128i *)&v70[k + 80] = v28;
      v29 = _mm_add_epi8(v28, v21);
      *(__m128i *)&v70[k + 96] = v29;
      v30 = _mm_add_epi8(v29, v21);
      *(__m128i *)&v70[k + 112] = v30;
      v22 = _mm_add_epi8(v30, v21);
    }
    n256_1 = n256;
    v32 = 0;
    if ( n256 < 256 )
    {
      v33 = 0;
      do
      {
        v34 = v70[v33];
        v70[v33++] = v70[n256];
        v70[n256] = v34;
        n256 += n128_1;
      }
      while ( n256 < 256 );
      n256_1 = __n256_0[0];
      v32 = v33;
    }
    if ( n256 - n256_1 < 256 )
    {
      v35 = v70[v32];
      v70[v32] = v71;
      v71 = v35;
    }
    __sub_4111B0_n2 = ::__n2;
  }
  if ( __sub_4111B0_n2 )
  {
    n256 = __n256_0[0];
    goto LABEL_22;
  }
  Src_4 = Src_1;
  n128_6 = n128_1 + 1;
  v49 = 0;
  for ( m = 0; m < 0x80; ++m )
  {
    n128_5 = n128_6 - 1;
    n128_4 = n128_1;
    if ( n128_5 )
      n128_4 = n128_5;
    else
      ++v49;
    v53 = v49;
    v63[2 * m] = v49;
    n128_6 = n128_4 - 1;
    if ( n128_4 == 1 )
    {
      n128_6 = n128_1;
      v53 = ++v49;
    }
    v63[2 * m + 1] = v53;
  }
  Src_1 = Src_4;
LABEL_54:
  for ( n0x80 = 0; n0x80 < 0x80; ++n0x80 )
  {
    v63[(uint8_t)v70[2 * n0x80] + 256] = 2 * n0x80;
    v63[(uint8_t)v70[2 * n0x80 + 1] + 256] = 2 * n0x80 + 1;
  }
  for ( ; Src_1 < Src_5; ++a4 )
  {
    n0x80 = (uint8_t)v63[(uint8_t)*Src_1];
    v57 = v70[n0x80];
    n64 = (uint8_t)*a4 - (uint8_t)(*a4 + v57 - *Src_1);
    if ( n64 < -64 || n64 > 64 )
    {
      *a4 = *Src_1;
      n0x80 = (uint8_t)v63[(uint8_t)*Src_1 + 256];
    }
    else
    {
      *a4 = v57;
    }
    *Src_1++ = n0x80;
  }
  return n0x80;
}
static inline int32_t __fwd_sub_4273F0_sub_413E60(void *a0, int32_t a1, int32_t a2) { return __sub_413E60((uint16_t *)a0, a1, a2); }
static inline void ** __fwd_sub_4273F0_sub_4244A0(void *a0, char a1) { return __sub_4244A0((void **)a0, a1); }
static inline int32_t __fwd_sub_4273F0_sub_4248D0(void *a0, void *a1, int32_t a2) { return __sub_4248D0((uint8_t **)a0, (uint32_t *)a1, a2); }
static inline int32_t * __fwd_sub_4273F0_sub_4256F0(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_4256F0((int32_t *)a0, a1, a2, a3); }

void ** __sub_4273F0(char ArgList, uint8_t *Src, int32_t i, int32_t a4)
{
  ;
  bool v33;
  int32_t *v4, *v5, v6, v7, v8, v9, v10, v11, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23,
          v24, v25, v26, v27, v28, v29, v30, v31, v32, v35, v36, v39, v41;
  int64_t v37;
  uint8_t *v12, *v13, *Src_1;
  v4 = (int32_t *)malloc(0x99D4D8u);
  if ( v4 )
    v5 = __fwd_sub_4273F0_sub_4256F0(v4, i, a4, 0);
  else
    v5 = nullptr;
  __sub_4149C0(ArgList);
  if ( v5[1] > 0 )
  {
    v6 = 0;
    do
    {
      ++v6;
      *(uint16_t *)v5[49] = *(uint16_t *)(v5[49] - 2);
      *(uint16_t *)(v5[49] + 2) = *(uint16_t *)(v5[49] - 4);
      *(uint16_t *)(v5[49] + 4) = *(uint16_t *)(v5[49] - 6);
      *(uint16_t *)(v5[49] + 6) = *(uint16_t *)(v5[49] - 8);
      *(uint16_t *)(v5[49] + 8) = *(uint16_t *)(v5[49] - 10);
      *(uint16_t *)(v5[49] + 10) = *(uint16_t *)(v5[49] - 12);
      v7 = v5[48];
      v8 = v5[47];
      v9 = v5[46];
      v10 = v5[45];
      v11 = v5[44];
      v5[48] = v8;
      v5[47] = v9;
      v5[46] = v10;
      v5[45] = v11;
      v5[44] = v7;
      v7 += 8;
      v5[49] = v7;
      v11 += 8;
      v5[50] = v11;
      v5[51] = v10 + 8;
      v5[52] = v9 + 8;
      v5[53] = v8 + 8;
      *(uint16_t *)(v7 - 8) = *(uint16_t *)(v11 + 6);
      *(uint16_t *)(v5[49] - 6) = *(uint16_t *)(v5[50] + 4);
      *(uint16_t *)(v5[49] - 4) = *(uint16_t *)(v5[50] + 2);
      *(uint16_t *)(v5[49] - 2) = *(uint16_t *)v5[50];
      v12 = (uint8_t *)v5[51];
      v13 = (uint8_t *)v5[53];
      v5[5] = 0;
      v5[6] = 0;
      v5[7] = 0;
      v14 = *(v12 - 3);
      v5[6] = v14;
      v15 = *(v12 - 1);
      v5[7] = v15;
      v16 = *(v13 - 3) + v14;
      v5[6] = v16;
      v17 = *(v13 - 1) + v15;
      v5[7] = v17;
      v18 = v12[1] + v16;
      v5[6] = v18;
      v19 = v12[3] + v17;
      v5[7] = v19;
      v20 = v13[1] + v18;
      v5[6] = v20;
      v21 = v13[3] + v19;
      v5[7] = v21;
      v22 = v12[5] + v20;
      v5[6] = v22;
      v23 = v12[7] + v21;
      v5[7] = v23;
      v24 = v13[5] + v22;
      v5[6] = v24;
      v25 = v13[7] + v23;
      v5[7] = v25;
      v26 = v12[9] + v24;
      v5[6] = v26;
      v27 = v12[11] + v25;
      v5[7] = v27;
      v28 = v13[9] + v26;
      v29 = v5[49];
      v5[6] = v28;
      v30 = v13[11] + v27;
      v5[7] = v30;
      v31 = *(uint8_t *)(v29 - 7) + v28;
      v5[6] = v31;
      v32 = *(uint8_t *)(v29 - 5) + v30;
      v5[7] = v32;
      v5[6] = *(uint8_t *)(v29 - 3) + v31;
      v33 = *v5 <= 0;
      v5[7] = *(uint8_t *)(v29 - 1) + v32;
      if ( !v33 )
      {
        Src_1 = Src;
        v39 = v6;
        v41 = 0;
        do
        {
          ++v41;
          __fwd_sub_4273F0_sub_4248D0((uint32_t *)v5, nullptr, 0);
          v36 = (uint8_t)(*((uint8_t *)v5 + 8)
                                + *((uint8_t *)v5 + (uint8_t)__fwd_sub_4273F0_sub_413E60(&v5[4 * v5[3] + 950], v35, v5[4]) + 1496));
          *Src_1 = v36;
          v37 = v36 - v5[2];
          *(uint8_t *)v5[49] = v36;
          *(uint8_t *)(v5[49] + 1) = (BYTE4(v37) ^ v37) - BYTE4(v37);
          v5[v5[5] + 6] = v5[v5[5] + 6]
                        + *(uint8_t *)(v5[49] + 1)
                        - *(uint8_t *)(v5[49] - 7)
                        - (*(uint8_t *)(v5[53] - 3)
                         - *(uint8_t *)(v5[53] + 13)
                         + *(uint8_t *)(v5[51] - 3)
                         - *(uint8_t *)(v5[51] + 13));
          LODWORD(v37) = 16 * v5[3];
          v5[5] = v5[5] == 0;
          if ( *(uint16_t *)((char *)v5 + v37 + 3800) < 0x4000u )
            __sub_4259F0((int32_t)v5);
          v5[49] += 2;
          ++Src_1;
          v5[50] += 2;
          v5[51] += 2;
          v5[52] += 2;
          v5[53] += 2;
        }
        while ( v41 < *v5 );
        v6 = v39;
        Src = Src_1;
      }
    }
    while ( v6 < v5[1] );
  }
  __sub_414920();
  return __fwd_sub_4273F0_sub_4244A0((void **)v5, 1);
}

static inline int32_t __fwd_sub_427740_sub_413E60(void *a0, int32_t a1, int32_t a2) { return __sub_413E60((uint16_t *)a0, a1, a2); }
static inline void ** __fwd_sub_427740_sub_4244A0(void *a0, char a1) { return __sub_4244A0((void **)a0, a1); }
static inline int32_t __fwd_sub_427740_sub_4248D0(void *a0, void *a1, int32_t a2) { return __sub_4248D0((uint8_t **)a0, (uint32_t *)a1, a2); }
static inline int32_t * __fwd_sub_427740_sub_4256F0(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_4256F0((int32_t *)a0, a1, a2, a3); }

 int32_t __sub_427740(uint16_t *p_i, int32_t Src)
{
  alignas(16) uint8_t __hexrays_frame[116];
  uint32_t * &v83 = *(uint32_t * *)(__hexrays_frame + 0);
  void * &v84 = *(void * *)(__hexrays_frame + 4);
  int32_t &v85 = *(int32_t *)(__hexrays_frame + 8);
  uint32_t &v86 = *(uint32_t *)(__hexrays_frame + 12);
  uint32_t &v87 = *(uint32_t *)(__hexrays_frame + 12);
  int32_t &v88 = *(int32_t *)(__hexrays_frame + 16);
  int32_t &v89 = *(int32_t *)(__hexrays_frame + 16);
  int32_t &Src_1 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &v91 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v92 = *(int32_t *)(__hexrays_frame + 28);
  void * &Block = *(void * *)(__hexrays_frame + 32);
  uint32_t * &v94 = *(uint32_t * *)(__hexrays_frame + 36);
  uint8_t ** &v95 = *(uint8_t ** *)(__hexrays_frame + 40);
  uint8_t ** &v96 = *(uint8_t ** *)(__hexrays_frame + 44);
  int32_t &v97 = *(int32_t *)(__hexrays_frame + 48);
  int32_t &v98 = *(int32_t *)(__hexrays_frame + 52);
  uint32_t &i_1 = *(uint32_t *)(__hexrays_frame + 56);
  int32_t &ArgList_1 = *(int32_t *)(__hexrays_frame + 60);
  int32_t &v101 = *(int32_t *)(__hexrays_frame + 64);
  uint32_t &i_4 = *(uint32_t *)(__hexrays_frame + 68);
  int32_t &v103 = *(int32_t *)(__hexrays_frame + 72);
  int32_t &v104 = *(int32_t *)(__hexrays_frame + 76);
  int32_t &v105 = *(int32_t *)(__hexrays_frame + 80);
  ;
  char v11, v12, v13, v58;
  int32_t i, v3, i_2, n4, *v6, *v7, v8, v9, v10, v14, v15, v16, ArgList, v18, i_3, n4_1, n4_2,
          v24, v25, v26, v27, v28, v29, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42,
          v43, v44, v45, v46, v47, v48, v49, v50, v52, v53, v54, v56, v57, v59, v60, v61, v62,
          v64, v65, v67, v68, v71, v72, v74, v75, v78, v79, n4_3, n4_4;
  uint32_t v20, *v51;
  uint8_t *v30, *v31, *v55, *v63, **v66, *v69, *v70, **v73, *v76, *v77;
  void **v82;
  i = *p_i;
  v3 = p_i[1];
  i_1 = i;
  if ( __n4_5 > 0 )
  {
    i_2 = i;
    n4 = 0;
    do
    {
      v6 = (int32_t *)malloc(0x99D4D8u);
      if ( v6 )
        v7 = __fwd_sub_427740_sub_4256F0(v6, i_2, v3, n4);
      else
        v7 = nullptr;
      *(&Block + n4++) = v7;
    }
    while ( n4 < __n4_5 );
  }
  v8 = 16 * (uint8_t)__byte_4433AD[0];
  v9 = 16 * (uint8_t)__byte_4433BD[0];
  v10 = 16 * (uint8_t)__n3_0;
  v11 = __byte_44339E[v8];
  v12 = __byte_44339E[v9];
  v13 = __byte_44339E[v10];
  v14 = (uint8_t)__byte_44339F[v9];
  v15 = (uint8_t)__byte_44339F[v10];
  v97 = (uint8_t)__byte_44339F[v8];
  v98 = v14;
  v16 = v11 & 8;
  v85 = v15;
  ArgList = v12 & 8;
  v18 = v13 & 8;
  __sub_4149C0(ArgList);
  if ( v3 > 0 )
  {
    v91 = v18;
    v20 = 0;
    ArgList_1 = ArgList;
    i_3 = i_1;
    v101 = v16;
    n4_1 = __n4_5;
    do
    {
      if ( n4_1 > 0 )
      {
        v86 = v20;
        n4_2 = 0;
        v88 = v3;
        Src_1 = Src;
        do
        {
          ++n4_2;
          v24 = *(&v92 + n4_2);
          **(uint16_t **)(v24 + 196) = *(uint16_t *)(*(uint32_t *)(v24 + 196) - 2);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) + 2) = *(uint16_t *)(*(uint32_t *)(v24 + 196) - 4);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) + 4) = *(uint16_t *)(*(uint32_t *)(v24 + 196) - 6);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) + 6) = *(uint16_t *)(*(uint32_t *)(v24 + 196) - 8);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) + 8) = *(uint16_t *)(*(uint32_t *)(v24 + 196) - 10);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) + 10) = *(uint16_t *)(*(uint32_t *)(v24 + 196) - 12);
          v25 = *(uint32_t *)(v24 + 192);
          v26 = *(uint32_t *)(v24 + 188);
          v27 = *(uint32_t *)(v24 + 184);
          v28 = *(uint32_t *)(v24 + 180);
          v29 = *(uint32_t *)(v24 + 176);
          *(uint32_t *)(v24 + 192) = v26;
          *(uint32_t *)(v24 + 188) = v27;
          *(uint32_t *)(v24 + 184) = v28;
          *(uint32_t *)(v24 + 180) = v29;
          *(uint32_t *)(v24 + 176) = v25;
          v25 += 8;
          *(uint32_t *)(v24 + 196) = v25;
          v29 += 8;
          *(uint32_t *)(v24 + 200) = v29;
          *(uint32_t *)(v24 + 204) = v28 + 8;
          *(uint32_t *)(v24 + 208) = v27 + 8;
          *(uint32_t *)(v24 + 212) = v26 + 8;
          *(uint16_t *)(v25 - 8) = *(uint16_t *)(v29 + 6);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) - 6) = *(uint16_t *)(*(uint32_t *)(v24 + 200) + 4);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) - 4) = *(uint16_t *)(*(uint32_t *)(v24 + 200) + 2);
          *(uint16_t *)(*(uint32_t *)(v24 + 196) - 2) = **(uint16_t **)(v24 + 200);
          v30 = *(uint8_t **)(v24 + 204);
          v31 = *(uint8_t **)(v24 + 212);
          *(uint32_t *)(v24 + 20) = 0;
          *(uint32_t *)(v24 + 24) = 0;
          *(uint32_t *)(v24 + 28) = 0;
          v32 = *(v30 - 3);
          *(uint32_t *)(v24 + 24) = v32;
          v33 = *(v30 - 1);
          *(uint32_t *)(v24 + 28) = v33;
          v34 = *(v31 - 3) + v32;
          *(uint32_t *)(v24 + 24) = v34;
          v35 = *(v31 - 1) + v33;
          *(uint32_t *)(v24 + 28) = v35;
          v36 = v30[1] + v34;
          *(uint32_t *)(v24 + 24) = v36;
          v37 = v30[3] + v35;
          *(uint32_t *)(v24 + 28) = v37;
          v38 = v31[1] + v36;
          *(uint32_t *)(v24 + 24) = v38;
          v39 = v31[3] + v37;
          *(uint32_t *)(v24 + 28) = v39;
          v40 = v30[5] + v38;
          *(uint32_t *)(v24 + 24) = v40;
          v41 = v30[7] + v39;
          *(uint32_t *)(v24 + 28) = v41;
          v42 = v31[5] + v40;
          *(uint32_t *)(v24 + 24) = v42;
          v43 = v31[7] + v41;
          *(uint32_t *)(v24 + 28) = v43;
          v44 = v30[9] + v42;
          *(uint32_t *)(v24 + 24) = v44;
          v45 = v30[11] + v43;
          *(uint32_t *)(v24 + 28) = v45;
          v46 = v31[9] + v44;
          v47 = *(uint32_t *)(v24 + 196);
          *(uint32_t *)(v24 + 24) = v46;
          v48 = v31[11] + v45;
          *(uint32_t *)(v24 + 28) = v48;
          v49 = *(uint8_t *)(v47 - 7) + v46;
          *(uint32_t *)(v24 + 24) = v49;
          v50 = *(uint8_t *)(v47 - 5) + v48;
          *(uint32_t *)(v24 + 28) = v50;
          *(uint32_t *)(v24 + 24) = *(uint8_t *)(v47 - 3) + v49;
          *(uint32_t *)(v24 + 28) = *(uint8_t *)(v47 - 1) + v50;
          n4_1 = __n4_5;
        }
        while ( n4_2 < __n4_5 );
        v20 = v86;
        v3 = v88;
        i_3 = i_1;
        Src = Src_1;
      }
      if ( i_3 > 0 )
      {
        v87 = v20;
        i_4 = 0;
        v89 = v3;
        do
        {
          v51 = Block;
          __fwd_sub_427740_sub_4248D0((uint8_t **)Block, nullptr, 0);
          v53 = __fwd_sub_427740_sub_413E60(&v51[4 * v51[3] + 950], v52, v51[4]);
          v54 = v51[2];
          v55 = (uint8_t *)v51[49];
          v56 = (uint8_t)(v54 + *((uint8_t *)v51 + v53 + 1496));
          v103 = v56;
          *v55 = v56;
          *(uint8_t *)(v51[49] + 1) = abs32(v56 - v54);
          v51[v51[5] + 6] = v51[v51[5] + 6]
                          + *(uint8_t *)(v51[49] + 1)
                          - *(uint8_t *)(v51[49] - 7)
                          - (*(uint8_t *)(v51[53] - 3)
                           - *(uint8_t *)(v51[53] + 13)
                           + *(uint8_t *)(v51[51] - 3)
                           - *(uint8_t *)(v51[51] + 13));
          v57 = 4 * v51[3];
          v51[5] = v51[5] == 0;
          if ( LOWORD(v51[v57 + 950]) < 0x4000u )
            __sub_4259F0((int32_t)v51);
          v58 = v103;
          v51[49] += 2;
          v51[50] += 2;
          v51[51] += 2;
          v51[52] += 2;
          v51[53] += 2;
          v59 = (int32_t)v94;
          *(uint8_t *)((uint8_t)__byte_44339D[0] + Src) = v58;
          __fwd_sub_427740_sub_4248D0((uint8_t **)v59, Block, 0);
          v61 = __fwd_sub_427740_sub_413E60((uint16_t *)(v59 + 16 * *(uint32_t *)(v59 + 12) + 3800), v60, *(uint32_t *)(v59 + 16));
          v62 = *(uint32_t *)(v59 + 8);
          v63 = *(uint8_t **)(v59 + 196);
          v64 = (uint8_t)(v62 + *(uint8_t *)(v61 + v59 + 1496));
          v104 = v64;
          *v63 = v64;
          *(uint8_t *)(*(uint32_t *)(v59 + 196) + 1) = abs32(v64 - v62);
          *(uint32_t *)(v59 + 4 * *(uint32_t *)(v59 + 20) + 24) = *(uint32_t *)(v59 + 4 * *(uint32_t *)(v59 + 20) + 24)
                                                            + *(uint8_t *)(*(uint32_t *)(v59 + 196) + 1)
                                                            - *(uint8_t *)(*(uint32_t *)(v59 + 196) - 7)
                                                            - (*(uint8_t *)(*(uint32_t *)(v59 + 212) - 3)
                                                             - *(uint8_t *)(*(uint32_t *)(v59 + 212) + 13)
                                                             + *(uint8_t *)(*(uint32_t *)(v59 + 204) - 3)
                                                             - *(uint8_t *)(*(uint32_t *)(v59 + 204) + 13));
          v65 = 16 * *(uint32_t *)(v59 + 12);
          *(uint32_t *)(v59 + 20) = *(uint32_t *)(v59 + 20) == 0;
          if ( *(uint16_t *)(v65 + v59 + 3800) < 0x4000u )
            __sub_4259F0(v59);
          *(uint32_t *)(v59 + 196) += 2;
          *(uint32_t *)(v59 + 200) += 2;
          *(uint32_t *)(v59 + 204) += 2;
          *(uint32_t *)(v59 + 208) += 2;
          *(uint32_t *)(v59 + 212) += 2;
          if ( v101 )
            v104 += v97 + *(uint8_t *)((uint8_t)__byte_44339D[0] + Src);
          v66 = v95;
          v84 = Block;
          v83 = v94;
          *(uint8_t *)(Src + (uint8_t)__byte_4433AD[0]) = v104;
          __fwd_sub_427740_sub_4248D0(v66, v83, (int32_t)v84);
          v68 = __fwd_sub_427740_sub_413E60(&v66[4 * (uint32_t)v66[3] + 950], v67, (int32_t)v66[4]);
          v69 = v66[2];
          v70 = v66[49];
          v71 = (uint8_t)((uint8_t)v69 + *((uint8_t *)v66 + v68 + 1496));
          v105 = v71;
          *v70 = v71;
          v66[49][1] = abs32(v71 - (uint32_t)v69);
          v66[(uint32_t)v66[5] + 6] = &v66[(uint32_t)v66[5] + 6][v66[49][1]
                                                           - *(v66[49] - 7)
                                                           - (*(v66[53] - 3)
                                                            - v66[53][13])
                                                           - (*(v66[51] - 3)
                                                            - v66[51][13])];
          v72 = 4 * (uint32_t)v66[3];
          v66[5] = (uint8_t *)(v66[5] == nullptr);
          if ( LOWORD(v66[v72 + 950]) < 0x4000u )
            __sub_4259F0((int32_t)v66);
          v66[49] += 2;
          v66[50] += 2;
          v66[51] += 2;
          v66[52] += 2;
          v66[53] += 2;
          if ( ArgList_1 )
            *(uint8_t *)((uint8_t)__byte_4433BD[0] + Src) = ((__dword_4433A0[4 * (uint8_t)__byte_4433BD[0]]
                                                                * *(uint8_t *)((uint8_t)__byte_44339D[0]
                                                                                     + Src)
                                                                + __dword_4433A4[4 * (uint8_t)__byte_4433BD[0]]
                                                                * (uint32_t)*(uint8_t *)((uint8_t)__byte_4433AD[0] + Src)
                                                                + 40) >> 7)
                                                              + v98
                                                              + v105;
          else
            *(uint8_t *)((uint8_t)__byte_4433BD[0] + Src) = v105;
          n4_1 = __n4_5;
          if ( __n4_5 >= 4 )
          {
            v73 = v96;
            __fwd_sub_427740_sub_4248D0(v96, v95, (int32_t)v94);
            v75 = __fwd_sub_427740_sub_413E60(&v73[4 * (uint32_t)v73[3] + 950], v74, (int32_t)v73[4]);
            v76 = v73[2];
            v77 = v73[49];
            v78 = (uint8_t)((uint8_t)v76 + *((uint8_t *)v73 + v75 + 1496));
            v92 = v78;
            *v77 = v78;
            v73[49][1] = abs32(v78 - (uint32_t)v76);
            v73[(uint32_t)v73[5] + 6] = &v73[(uint32_t)v73[5] + 6][v73[49][1]
                                                             - *(v73[49] - 7)
                                                             - (*(v73[53] - 3)
                                                              - v73[53][13])
                                                             - (*(v73[51] - 3)
                                                              - v73[51][13])];
            v79 = 4 * (uint32_t)v73[3];
            v73[5] = (uint8_t *)(v73[5] == nullptr);
            if ( LOWORD(v73[v79 + 950]) < 0x4000u )
              __sub_4259F0((int32_t)v73);
            v73[49] += 2;
            v73[50] += 2;
            v73[51] += 2;
            v73[52] += 2;
            v73[53] += 2;
            if ( v91 )
              v92 += ((__dword_4433A4[4 * (uint8_t)__n3_0] * *(uint8_t *)((uint8_t)__n3_0 + Src - 2)
                     + __dword_4433A0[4 * (uint8_t)__n3_0] * *(uint8_t *)((uint8_t)__n3_0 + Src - 3)
                     + __dword_4433A8[4 * (uint8_t)__n3_0] * *(uint8_t *)((uint8_t)__n3_0 + Src - 1)
                     + 64) >> 7)
                   + v85;
            *(uint8_t *)((uint8_t)__n3_0 + Src) = v92;
            n4_1 = __n4_5;
          }
          Src += n4_1;
          ++i_4;
        }
        while ( i_4 < i_1 );
        v20 = v87;
        v3 = v89;
        i_3 = i_1;
      }
      ++v20;
    }
    while ( v20 < v3 );
  }
  __sub_414920();
  n4_3 = __n4_5;
  if ( __n4_5 > 0 )
  {
    n4_4 = 0;
    do
    {
      v82 = (void **)*(&Block + n4_4);
      if ( v82 )
      {
        __fwd_sub_427740_sub_4244A0(v82, 1);
        n4_3 = __n4_5;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline int32_t __fwd_sub_40BEE0_sub_40CC50(int32_t a0, void *a1, void *a2, char a3) { return __sub_40CC50(a0, (uint32_t *)a1, (char *)a2, a3); }

BMF_SSE int32_t __sub_40BEE0(char a1)
{
  alignas(16) uint8_t __hexrays_frame[2544];
  uint32_t &v101 = *(uint32_t *)(__hexrays_frame + 0);
  int32_t &n4_2 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &n4_3 = *(int32_t *)(__hexrays_frame + 0);
  uint32_t &v104 = *(uint32_t *)(__hexrays_frame + 4);
  uint32_t &n0x15_1 = *(uint32_t *)(__hexrays_frame + 4);
  int32_t &n256_1 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v107 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 8);
  uint8_t (&k_2)[2288] = *(uint8_t (*)[2288])(__hexrays_frame + 12);
  uint32_t (&v110)[3] = *(uint32_t (*)[3])(__hexrays_frame + 2300);
  char &v111 = *(char *)(__hexrays_frame + 2314);
  int32_t &v112 = *(int32_t *)(__hexrays_frame + 2316);
  int32_t (&v113)[4] = *(int32_t (*)[4])(__hexrays_frame + 2320);
  __m128i &v114 = *(__m128i *)(__hexrays_frame + 2336);
  __m128i &v115 = *(__m128i *)(__hexrays_frame + 2352);
  __m128i &v116 = *(__m128i *)(__hexrays_frame + 2368);
  __m128i &v117 = *(__m128i *)(__hexrays_frame + 2384);
  int32_t &v118 = *(int32_t *)(__hexrays_frame + 2400);
  int32_t &v119 = *(int32_t *)(__hexrays_frame + 2404);
  int32_t &v120 = *(int32_t *)(__hexrays_frame + 2408);
  int32_t &n11_1 = *(int32_t *)(__hexrays_frame + 2412);
  int32_t &n11_2 = *(int32_t *)(__hexrays_frame + 2416);
  int32_t &v123 = *(int32_t *)(__hexrays_frame + 2420);
  uint32_t &v124 = *(uint32_t *)(__hexrays_frame + 2424);
  int32_t &v125 = *(int32_t *)(__hexrays_frame + 2428);
  uint8_t * &k_4 = *(uint8_t * *)(__hexrays_frame + 2432);
  int32_t &v127 = *(int32_t *)(__hexrays_frame + 2436);
  uint32_t &v128 = *(uint32_t *)(__hexrays_frame + 2440);
  int32_t &v129 = *(int32_t *)(__hexrays_frame + 2444);
  int32_t &j_2 = *(int32_t *)(__hexrays_frame + 2448);
  int32_t &v131 = *(int32_t *)(__hexrays_frame + 2452);
  int32_t &v132 = *(int32_t *)(__hexrays_frame + 2456);
  uint32_t * &v133 = *(uint32_t * *)(__hexrays_frame + 2460);
  int32_t &v134 = *(int32_t *)(__hexrays_frame + 2464);
  int32_t &v135 = *(int32_t *)(__hexrays_frame + 2468);
  int32_t &n14_5 = *(int32_t *)(__hexrays_frame + 2472);
  uint32_t &v137 = *(uint32_t *)(__hexrays_frame + 2476);
  int32_t &v138 = *(int32_t *)(__hexrays_frame + 2480);
  int32_t &v139 = *(int32_t *)(__hexrays_frame + 2484);
  int32_t &v140 = *(int32_t *)(__hexrays_frame + 2488);
  int32_t &n14_2 = *(int32_t *)(__hexrays_frame + 2492);
  uint32_t * &v142 = *(uint32_t * *)(__hexrays_frame + 2496);
  int32_t &__sub_40BEE0_buf_1 = *(int32_t *)(__hexrays_frame + 2500);
  uint8_t * &k_1 = *(uint8_t * *)(__hexrays_frame + 2504);
  ;
  bool v5;
  char v6, *buf, v27, v40, *v51, *v77;
  int32_t v1, v3, v4, n11, n11_3, j_1, v13, n14_1, v15, n14, v18, v19, v20, v22, v23, v24, n14_3,
          n14_4, v31, v32, v33, v34, v35, v36, n0x15_2, v43, v44, v45, v46, v47, v48, v49, v50,
          v52, v53, v54, v55, v56, v57, v58, v60, j, k_9, n11_4, n4_1, __sub_40BEE0_n256, v66,
          buf_2, v70, v71, v72, v73, v74, v75, v76, buf_3, v79, v80, v81, v82, buf_4, v85,
          __sub_40BEE0_n8, v89, v92, n8_1, n256_2, k, v96, k_11, v98, n8_2, v100;
  uint32_t i, *v10, v17, v37, v38, n0x15, v42, v59, v67, v69, v84, v87, v88, v91;
  uint8_t *k_3, *k_6, *k_7, *k_5, *k_8, *k_10;
  v1 = 0;
  for ( i = 0; i < 7; ++i )
  {
    v3 = v1 + 1;
    v4 = 2 * i + 1;
    if ( 2 * i != __dword_4394A0[v1] )
      v3 = v1;
    v5 = v4 == __dword_4394A0[v3];
    *((uint8_t *)v110 + 2 * i) = v3;
    v1 = v3 + v5;
    *((uint8_t *)v110 + v4) = v3 + v5;
  }
  v6 = v3 + v5 + 1;
  if ( __dword_4394A0[v1] == 14 )
    LOBYTE(v1) = v6;
  n11 = 11;
  v111 = v1;
  k_1 = k_2;
  if ( __n2 != 2 )
    n11 = 0;
  v101 = 0;
  n11_1 = n11;
  n11_2 = n11;
  v120 = -(int32_t)k_2;
  v134 = 0;
  v124 = 4 * n11;
  n11_3 = n11;
  v123 = 0;
  while ( 1 )
  {
    ++n11_3;
    buf = (char *)__dword_442DDC[v123 + 1 + v124 / 4];
    v10 = (uint32_t *)__dword_442E3C[v123 + 1 + v124 / 4];
    j_1 = __dword_438AFC[++v123 + v124 / 4];
    v133 = v10;
    __fwd_sub_40BEE0_sub_40CC50(j_1, v10, buf, a1);
    if ( n11_3 == 19 )
      break;
    *(__m128i *)v113 = 0;
    v114 = 0;
    v115 = 0;
    v116 = 0;
    v117 = 0;
    v118 = _mm_cvtsi128_si32((__m128i)0LL);
    v125 = v120;
    k_3 = k_1;
    v131 = 0;
    if ( j_1 > 0 )
    {
      v132 = 0;
      v112 = 0;
      v13 = -1;
      v129 = __dword_4410BC[v123 + v124 / 4];
      n14_1 = 0;
      k_4 = k_1;
      v15 = 0;
      j_2 = j_1;
      __sub_40BEE0_buf_1 = (int32_t)buf;
      n11_1 = n11_3;
      do
      {
        n14_5 = n14_1;
        n14 = j_2 - v15;
        if ( j_2 - v15 >= 14 )
          n14 = 14;
        v140 = v15 + v129;
        if ( n14_1 < n14 )
        {
          v127 = 0;
          v138 = n14_1 + v15 + v129;
          v137 = n14 - n14_1;
          v17 = 0;
          v132 = v15;
          v18 = __sub_40BEE0_buf_1 + 4 * (v15 + n14_1);
          do
          {
            v19 = (*(uint8_t *)(v17 + v138) - *(uint8_t *)(v18 + 4 * v17 + 2)) & 0xF;
            if ( v19 != v13 )
              break;
            n14_1 = v17 + n14_5 + 1;
            ++v17;
          }
          while ( v17 < v137 );
          v134 = v19;
          v15 = v132;
        }
        if ( n14_1 <= 1 )
        {
          if ( n14_1 == 1 )
          {
            k_5 = k_4;
            --v125;
            *k_4 = v13;
            ++v113[(uint8_t)v13];
            k_4 = k_5 + 1;
          }
        }
        else
        {
          v20 = *((uint8_t *)v110 + n14_1);
          k_6 = k_4;
          --v125;
          v22 = (uint8_t)(v20 + 16);
          ++v113[v22];
          *k_6 = v22;
          k_4 = k_6 + 1;
          v23 = __dword_4394B0[v20];
          if ( v23 )
          {
            v131 -= v23;
            v24 = n14_1 - __dword_43949C[v20];
            k_7 = k_4;
            *k_4 = v24;
            --v125;
            k_4 = k_7 + 1;
          }
        }
        if ( v13 != v134 )
        {
          v27 = v134;
          k_8 = k_4;
          ++v113[v134];
          ++n14_1;
          --v125;
          *k_8 = v27;
          k_4 = k_8 + 1;
        }
        v132 = v15;
        n14_2 = n14_1;
        n14_3 = n14_1;
        n14_4 = 0;
        v142 = &v133[v15];
        v31 = __sub_40BEE0_buf_1 + 4 * v15;
        v32 = v131;
        v139 = v31;
        v33 = 0;
        do
        {
          v34 = v142[n14_4] * (*(uint8_t *)(n14_4 + v140) - *(uint8_t *)(v139 + 4 * n14_4 + 2));
          ++n14_4;
          v32 += v34;
          n14_1 = v33 + n14_3 - 1;
          --v33;
        }
        while ( n14_4 < n14_2 );
        v13 = v134;
        v131 = v32;
        v15 = n14_4 + v132;
      }
      while ( n14_4 + v132 < j_2 );
      k_3 = k_4;
      j_1 = j_2;
      buf = (char *)__sub_40BEE0_buf_1;
      n11_3 = n11_1;
    }
    k_4 = k_3;
    v35 = __sub_411700((int32_t)v113, 21);
    if ( v131 - v35 - ((int32_t)&k_4[v120 + 1] >> 1) <= 0 )
    {
      v40 = j_1 <= 0;
      if ( j_1 <= 0 )
        goto LABEL_49;
      v60 = __dword_4410BC[v123 + v124 / 4];
      for ( j = 0; j < j_1; buf[4 * j - 2] = *(uint8_t *)(j + v60 - 1) )
        ++j;
      goto LABEL_48;
    }
    k_1 = k_4;
    v36 = __dword_442E88;
    v120 = v125;
    v101 |= 1 << ((n11_3 + 31) & 31);
    v37 = __dword_442E88 & 0xF;
    if ( (__dword_442E88 & 0xF) != 0 )
    {
      if ( (__dword_442E88 & 3) != 0 )
      {
        n0x15_2 = 0;
        n0x15_1 = 0;
        goto LABEL_44;
      }
      n11_1 = n11_3;
      v37 = (16 - v37) >> 2;
      v38 = 0;
      do
      {
        *(uint32_t *)(v36 + 4 * v38) += v113[v38];
        ++v38;
      }
      while ( v38 < v37 );
      n11_3 = n11_1;
    }
    v104 = 21 - ((1 - (uint8_t)v37) & 3);
    if ( ((uint8_t)&v113[v37] & 0xF) != 0 )
    {
      n0x15_2 = 21 - ((1 - (uint8_t)v37) & 3);
      do
      {
        *(__m128i *)(v36 + 4 * v37) = _mm_add_epi32(
                                        _mm_loadu_si128((const __m128i *)&v113[v37]),
                                        *(__m128i *)(v36 + 4 * v37));
        v37 += 4;
      }
      while ( v37 < v104 );
    }
    else
    {
      n0x15_2 = 21 - ((1 - (uint8_t)v37) & 3);
      do
      {
        *(__m128i *)(v36 + 4 * v37) = _mm_add_epi32(
                                        _mm_load_si128((const __m128i *)(v36 + 4 * v37)),
                                        *(__m128i *)&v113[v37]);
        v37 += 4;
      }
      while ( v37 < v104 );
    }
    n0x15_1 = n0x15_2;
LABEL_44:
    if ( n0x15_1 < 0x15 )
    {
      n0x15 = n0x15_2;
      do
      {
        *(uint32_t *)(v36 + 4 * n0x15) += v113[n0x15];
        ++n0x15;
      }
      while ( n0x15 < 0x15 );
LABEL_48:
      v40 = j_1 <= 0;
      goto LABEL_49;
    }
    v40 = j_1 <= 0;
LABEL_49:
    ::__buf_1 = (int32_t)buf;
    __xmmword_442EA0 = -1;
    __xmmword_442EB0 = -1;
    __xmmword_442EC0 = -1;
    __xmmword_442ED0 = -1;
    if ( !v40 )
    {
      if ( j_1 / 2 )
      {
        j_2 = j_1;
        n11_1 = n11_3;
        v42 = 0;
        do
        {
          v43 = 2 * v42;
          v44 = (uint8_t)buf[8 * v42 + 2];
          __dword_442EE0[v43] = *((uint32_t *)&__xmmword_442EA0 + v44);
          *((uint32_t *)&__xmmword_442EA0 + v44) = 2 * v42;
          v45 = (uint8_t)buf[8 * v42 + 6];
          __dword_442EE4[v43] = *((uint32_t *)&__xmmword_442EA0 + v45);
          v46 = 2 * v42++ + 1;
          *((uint32_t *)&__xmmword_442EA0 + v45) = v46;
        }
        while ( v42 < j_1 / 2 );
        j_1 = j_2;
        n11_3 = n11_1;
        v47 = 2 * v42 + 1;
      }
      else
      {
        v47 = 1;
      }
      if ( j_1 > (uint32_t)(v47 - 1) )
      {
        v48 = (uint8_t)buf[4 * v47 - 2];
        *((uint32_t *)&__xmmword_442ED0 + v47 + 3) = *((uint32_t *)&__xmmword_442EA0 + v48);
        *((uint32_t *)&__xmmword_442EA0 + v48) = v47 - 1;
      }
    }
    if ( SDWORD1(__xmmword_442EA0) < 0 )
    {
      v49 = 1;
      do
        v50 = v49++;
      while ( *((int32_t *)&__xmmword_442EA0 + v50 + 1) < 0 );
    }
    else
    {
      v49 = 1;
    }
    v51 = (char *)(4 * v49 + BMF_BLOB(0x00442EA0));
    v52 = 4 * v49;
    v53 = (1 << (v49 & 31)) - 1;
    if ( v53 < 0 )
    {
      __dword_443428 = 0;
    }
    else
    {
      __dword_44342C = 4 * v49 + BMF_BLOB(0x00442EA0);
      v119 = 4 * v49;
      v135 = v49;
      n11_1 = n11_3;
      do
      {
        v54 = *(uint32_t *)v51;
        *(uint16_t *)&buf[4 * v54] = v53 << ((((uint32_t)(v51 - (char *)&__xmmword_442EA0) >> 2) - v135) & 31);
        v55 = __dword_442EE0[v54];
        *(uint32_t *)v51 = v55;
        if ( v55 < 0 )
        {
          v56 = v53;
          v57 = --v53;
          if ( v53 >= 0 )
          {
            v128 = v56;
            __sub_40BEE0_buf_1 = (int32_t)buf;
            v58 = 0;
            v59 = 0;
            do
            {
              __dword_443428 = (v57 - v59) << ((((uint32_t)(__dword_44342C - (uint32_t)&__xmmword_442EA0) >> 2) - v135) & 31);
              __sub_40CB70();
              v53 = v58 + v57 - 1;
              --v58;
              ++v59;
            }
            while ( v59 < v128 );
            buf = (char *)__sub_40BEE0_buf_1;
            v51 = (char *)__dword_44342C;
          }
        }
        --v53;
      }
      while ( v53 >= 0 );
      v52 = v119;
      n11_3 = n11_1;
      __dword_443428 = 0;
    }
    n11_2 = n11_3;
    __dword_44342C = (int32_t)&v51[-v52];
  }
  k_9 = (int32_t)k_1;
  n11_4 = n11_2;
  __sub_40BEE0_buf_1 = (int32_t)buf;
  if ( ::__n8 < 18 )
  {
    *(uint32_t *)__Buffer_1 = ::__n256 | (2 * (v101 << ((31 - ::__n8) & 31)));
    __Buffer_1 = __Buffer_0;
    __Buffer_0 += 4;
    __sub_40BEE0_n256 = v101 >> (::__n8 & 31);
    n4_1 = ::__n8 + 14;
    ::__n8 += 14;
  }
  else
  {
    n4_1 = ::__n8 - 18;
    __sub_40BEE0_n256 = ::__n256 | (v101 << (-(char)::__n8 & 31));
    ::__n8 -= 18;
  }
  if ( v101 )
  {
    v66 = __dword_438B00[n11_4];
    __xmmword_442EA0 = -1;
    __xmmword_442EB0 = -1;
    __xmmword_442EC0 = -1;
    __xmmword_442ED0 = -1;
    ::__buf_1 = __sub_40BEE0_buf_1;
    if ( v66 > 0 )
    {
      v67 = v66 / 2;
      if ( v66 / 2 )
      {
        k_1 = (uint8_t *)k_9;
        n4_2 = n4_1;
        buf_2 = __sub_40BEE0_buf_1;
        v107 = v66;
        v69 = 0;
        do
        {
          v70 = 2 * v69;
          v71 = *(uint8_t *)(buf_2 + 8 * v69 + 2);
          __dword_442EE0[v70] = *((uint32_t *)&__xmmword_442EA0 + v71);
          *((uint32_t *)&__xmmword_442EA0 + v71) = 2 * v69;
          v72 = *(uint8_t *)(buf_2 + 8 * v69 + 6);
          __dword_442EE4[v70] = *((uint32_t *)&__xmmword_442EA0 + v72);
          v73 = 2 * v69++ + 1;
          *((uint32_t *)&__xmmword_442EA0 + v72) = v73;
        }
        while ( v69 < v67 );
        k_9 = (int32_t)k_1;
        v66 = v107;
        __sub_40BEE0_buf_1 = buf_2;
        v74 = 2 * v69 + 1;
        n4_1 = n4_2;
      }
      else
      {
        v74 = 1;
      }
      if ( v66 > (uint32_t)(v74 - 1) )
      {
        k_1 = (uint8_t *)k_9;
        v75 = *(uint8_t *)(__sub_40BEE0_buf_1 + 4 * v74 - 2);
        *((uint32_t *)&__xmmword_442ED0 + v74 + 3) = *((uint32_t *)&__xmmword_442EA0 + v75);
        k_9 = (int32_t)k_1;
        *((uint32_t *)&__xmmword_442EA0 + v75) = v74 - 1;
      }
    }
    v76 = 1;
    if ( SDWORD1(__xmmword_442EA0) < 0 )
    {
      do
        ++v76;
      while ( *((int32_t *)&__xmmword_442EA0 + v76) < 0 );
      n4 = 4 * v76;
    }
    else
    {
      n4 = 4;
    }
    v77 = (char *)&__xmmword_442EA0 + n4;
    if ( (1 << (v76 & 31)) - 1 < 0 )
    {
      __dword_443428 = 0;
    }
    else
    {
      ::__n256 = __sub_40BEE0_n256;
      __dword_44342C = (int32_t)&__xmmword_442EA0 + n4;
      k_1 = (uint8_t *)k_9;
      buf_3 = __sub_40BEE0_buf_1;
      n4_3 = n4_1;
      n256_1 = __sub_40BEE0_n256;
      v79 = (1 << (v76 & 31)) - 1;
      v110[0] = v76;
      do
      {
        v80 = *(uint32_t *)v77;
        *(uint16_t *)(buf_3 + 4 * v80) = v79 << ((((uint32_t)(v77 - (char *)&__xmmword_442EA0) >> 2) - LOBYTE(v110[0])) & 31);
        v81 = __dword_442EE0[v80];
        *(uint32_t *)v77 = v81;
        if ( v81 < 0 && --v79 >= 0 )
        {
          v100 = v110[0];
          __sub_40BEE0_buf_1 = buf_3;
          do
          {
            __dword_443428 = v79 << ((((uint32_t)(__dword_44342C - (uint32_t)&__xmmword_442EA0) >> 2) - v100) & 31);
            __sub_40CB70();
            --v79;
          }
          while ( v79 >= 0 );
          buf_3 = __sub_40BEE0_buf_1;
          v77 = (char *)__dword_44342C;
          v110[0] = v100;
        }
        --v79;
      }
      while ( v79 >= 0 );
      n4_1 = n4_3;
      __sub_40BEE0_n256 = n256_1;
      __dword_443428 = 0;
      __sub_40BEE0_buf_1 = buf_3;
      k_9 = (int32_t)k_1;
    }
    __dword_44342C = (int32_t)&v77[-n4];
    v82 = 0;
    k_1 = (uint8_t *)k_9;
    buf_4 = __sub_40BEE0_buf_1;
    while ( 1 )
    {
      v84 = *(uint8_t *)(buf_4 + 8 * v82 + 2);
      if ( n4_1 < 4 )
      {
        *(uint32_t *)__Buffer_1 = (2 * (v84 << ((31 - n4_1) & 31))) | __sub_40BEE0_n256;
        __Buffer_1 = __Buffer_0;
        __Buffer_0 += 4;
        v87 = v84 >> (::__n8 & 31);
        __sub_40BEE0_n8 = ::__n8 + 28;
      }
      else
      {
        v85 = v84 << (-(char)n4_1 & 31);
        __sub_40BEE0_n8 = n4_1 - 4;
        v87 = v85 | __sub_40BEE0_n256;
      }
      v88 = *(uint8_t *)(buf_4 + 8 * v82 + 6);
      if ( __sub_40BEE0_n8 < 4 )
      {
        ::__n8 = __sub_40BEE0_n8;
        *(uint32_t *)__Buffer_1 = (2 * (v88 << ((31 - __sub_40BEE0_n8) & 31))) | v87;
        __Buffer_1 = __Buffer_0;
        __Buffer_0 += 4;
        __sub_40BEE0_n256 = v88 >> (::__n8 & 31);
        n4_1 = ::__n8 + 28;
      }
      else
      {
        v89 = v88 << (-(char)__sub_40BEE0_n8 & 31);
        n4_1 = __sub_40BEE0_n8 - 4;
        __sub_40BEE0_n256 = v89 | v87;
      }
      if ( (uint32_t)++v82 >= 0xA )
        break;
      ::__n8 = n4_1;
    }
    __sub_40BEE0_buf_1 = buf_4;
    k_10 = k_1;
    v91 = *(uint8_t *)(__sub_40BEE0_buf_1 + 82);
    if ( n4_1 < 4 )
    {
      ::__n8 = n4_1;
      *(uint32_t *)__Buffer_1 = (2 * (v91 << ((31 - n4_1) & 31))) | __sub_40BEE0_n256;
      __Buffer_1 = __Buffer_0;
      __Buffer_0 += 4;
      n256_2 = v91 >> (::__n8 & 31);
      n8_1 = ::__n8 + 28;
      ::__n8 += 28;
      ::__n256 = n256_2;
    }
    else
    {
      v92 = v91 << (-(char)n4_1 & 31);
      n8_1 = n4_1 - 4;
      n256_2 = v92 | __sub_40BEE0_n256;
      ::__n256 = n256_2;
      ::__n8 = n8_1;
    }
    k_9 = k_10 - k_2;
    if ( k_9 > 0 )
    {
      k_1 = (uint8_t *)k_9;
      for ( k = 0; k < (int32_t)k_1; ++k )
      {
        v96 = (uint8_t)k_2[k];
        k_11 = *(uint32_t *)(__sub_40BEE0_buf_1 + 4 * v96);
        k_9 = (uint16_t)k_11;
        if ( n8_1 < BYTE2(k_11) )
        {
          *(uint32_t *)__Buffer_1 = (2 * ((uint16_t)k_11 << ((31 - n8_1) & 31))) | n256_2;
          __Buffer_1 = __Buffer_0;
          __Buffer_0 += 4;
          n256_2 = (uint16_t)k_11 >> (::__n8 & 31);
          n8_1 = ::__n8 - BYTE2(k_11) + 32;
        }
        else
        {
          LOBYTE(k_11) = n8_1;
          n8_1 -= BYTE2(k_11);
          k_9 <<= (-(char)k_11 & 31);
          n256_2 |= k_9;
        }
        v40 = (v96 - 16 < 0) ^ __OFADD__(-16, v96);
        v98 = v96 - 16;
        if ( !v40 && (n8_2 = __dword_4394B0[v98]) != 0 )
        {
          k_9 = (uint8_t)k_2[++k];
          if ( n8_1 < n8_2 )
          {
            ::__n8 = n8_1;
            *(uint32_t *)__Buffer_1 = (2 * (k_9 << ((31 - n8_1) & 31))) | n256_2;
            __Buffer_1 = __Buffer_0;
            __Buffer_0 += 4;
            n256_2 = (uint32_t)k_9 >> (::__n8 & 31);
            ::__n256 = (uint32_t)k_9 >> (::__n8 & 31);
            n8_1 = ::__n8 - n8_2 + 32;
          }
          else
          {
            k_9 <<= (-(char)n8_1 & 31);
            n8_1 -= n8_2;
            n256_2 |= k_9;
            ::__n256 = n256_2;
          }
          ::__n8 = n8_1;
        }
        else
        {
          ::__n8 = n8_1;
          ::__n256 = n256_2;
        }
      }
    }
  }
  else
  {
    ::__n256 = __sub_40BEE0_n256;
  }
  return k_9;
}

int32_t __sub_40A8A0(uint32_t *_this, int32_t Src, uint32_t *p_Src, int32_t n6, int32_t n24, int32_t __n2)
{
  ;
  double v6, v9, v11, v15, v25, v26, v29, v32;
  float v7, n6_1, v10, v12, v13, v14, v16, v27, v30, v33;
  int32_t Src_1, v18, v19, v20, v21, v23, v28, v34, v35, v37, v38, v39, v40, v41, v42, v43, n2_1,
          v46, v47, n4, v50;
  uint32_t *v22, *v24, *v36, Src_2, v49;
  n4 = __n4_1;
  if ( n6 == __n4_1 )
  {
    if ( *(uint32_t *)(__dword_442E3C[__n2] + 4) )
      v9 = log((double)*(int32_t *)((char *)__buf_2 + 4 * __n2 + 0x342B8) / (double)*(uint32_t *)(__dword_442E3C[__n2] + 4)) * 1.442695040888963;
    else
      v9 = 0;
    v10 = v9;
    v11 = (double)__n2 + 1.1;
    v12 = v10;
    v13 = v11;
    v14 = v12 + 0.08;
    if ( v14 <= v11 )
      v13 = v12 + 0.08;
    if ( *(uint32_t *)(__dword_442E68 + 4 * n24) )
      v15 = log((double)*(int32_t *)((char *)__buf_2 + 0x342E4) / (double)*(uint32_t *)(__dword_442E68 + 4 * n24)) * 1.442695040888963;
    else
      v15 = 0;
    v16 = v15;
    n6_1 = v13 + v16;
  }
  else if ( n24 < 0 )
  {
    if ( *(uint32_t *)(__dword_442E6C + 4 * *((uint8_t *)__buf_2 + n6 + 213056)) )
      v32 = log((double)*(int32_t *)((char *)__buf_2 + 0x342E8) / (double)*(uint32_t *)(__dword_442E6C + 4 * *((uint8_t *)__buf_2 + n6 + 213056))) * 1.442695040888963;
    else
      v32 = 0;
    v33 = v32;
    n6_1 = v33 + 0.5;
  }
  else
  {
    if ( *(uint32_t *)(__dword_442E54[0] + 4 * ((2 * n24) | (n6 - 5))) )
      v6 = log((double)*(int32_t *)((char *)__buf_2 + 0x342D0) / (double)*(uint32_t *)(__dword_442E54[0] + 4 * ((2 * n24) | (n6 - 5)))) * 1.442695040888963;
    else
      v6 = 0;
    v7 = v6;
    n6_1 = v7 + 0.14;
  }
  if ( n6 <= 0 )
    return 0;
  Src_1 = Src;
  v50 = __dword_443364;
  v49 = p_Src[1];
  Src_2 = *p_Src;
  n2_1 = ::__n2;
  v46 = Src + p_Src[2];
  v18 = 0;
  v47 = Src + p_Src[3];
  while ( 1 )
  {
    if ( v50 )
    {
      if ( Src_1 + v18 <= v49 )
      {
        v21 = *(uint8_t *)(v18 + Src_1 - 1);
        if ( Src_1 + v18 <= Src_2 )
        {
          v20 = v21 & 1 | p_Src[v21 + 5] | (2 * (v21 & 1));
        }
        else
        {
          v37 = *(uint8_t *)(v18 + v46 - 1);
          v38 = *(uint8_t *)(v18 + v46 + 1);
          v20 = *(uint8_t *)(v18 + v46) & 1
              | *(uint32_t *)((char *)p_Src + ((*(uint8_t *)(v18 + v46) + v38 + v21 + v37) & 0xFFFFFFFC)
                                                + 20)
              | (2 * (v21 & 1));
          if ( n2_1 == 1 )
          {
            v35 = v20 < (int32_t)p_Src[v21 + 5] || v20 < (int32_t)p_Src[v38 + 5] || v20 < (int32_t)p_Src[v37 + 5];
            v20 |= (v35 << 7) | (8 * (v37 & 1)) | (4 * (v38 & 1));
          }
        }
      }
      else
      {
        v43 = *(uint8_t *)(v18 + v46);
        v42 = *(uint8_t *)(v18 + Src_1 - 1);
        v40 = *(uint8_t *)(v18 + v46 - 1);
        v41 = *(uint8_t *)(v18 + v46 + 1);
        v19 = *(uint8_t *)(v18 + v47)
            + *(uint8_t *)(v18 + v46 + 2)
            + *(uint8_t *)(v18 + v47 + 1)
            + *(uint8_t *)(v18 + Src_1 - 2);
        Src_1 = Src;
        v20 = v43 & 1
            | p_Src[((*(uint8_t *)(v18 + v47 + 3)
                    + *(uint8_t *)(v18 + v47 + 2)
                    + *(uint8_t *)(v18 + v47 - 2)
                    + *(uint8_t *)(v18 + v46 - 3)
                    + *(uint8_t *)(v18 + v46 + 4)
                    + *(uint8_t *)(v18 + v47 - 1)
                    + *(uint8_t *)(v18 + v46 + 3)
                    + *(uint8_t *)(v18 + v46 - 2)
                    + 3 * (v41 + v40)
                    + *(uint8_t *)(v18 + Src - 4)
                    + *(uint8_t *)(v18 + Src - 3)
                    + 4 * (v42 + v43)
                    + 2 * v19) >> 5)
                  + 5]
            | (2 * (v42 & 1));
        if ( n2_1 == 1 )
        {
          v34 = v20 < (int32_t)p_Src[v42 + 5] || v20 < (int32_t)p_Src[v41 + 5] || v20 < (int32_t)p_Src[v40 + 5];
          v20 |= (v34 << 7) | (8 * (v40 & 1)) | (4 * (v41 & 1));
        }
      }
    }
    else
    {
      v20 = *(uint8_t *)(v18 + Src_1 - 1);
    }
    v22 = _this + 194 * v20;
    v23 = *v22;
    if ( v18 )
    {
      v24 = _this + 257 * v23;
      goto LABEL_35;
    }
    v24 = _this + 257 * v23;
    if ( n6 != n4 )
    {
      v39 = v24[49728];
      if ( !v39 )
        return 1;
      if ( *(_this + v23 + 49664) <= 2 * v39 )
      {
        v26 = 1.0;
      }
      else
      {
        v36 = v22;
        v25 = log((double)*(_this + v23 + 49664) / (double)v39);
        v22 = v36;
        v24 = _this + 257 * v23;
        v26 = v25 * 1.442695040888963;
      }
      v27 = v26;
      n6_1 = n6_1 + v27;
      if ( (float)n6 >= n6_1 )
        return 1;
    }
LABEL_35:
    v28 = v24[*((uint8_t *)v22 + *(uint8_t *)(v18 + Src_1) + 520) + 49729];
    if ( !v28 )
      return 1;
    v29 = *(_this + v23 + 49664) <= 2 * v28 ? 1.0 : log((double)*(_this + v23 + 49664) / (double)v28) * 1.442695040888963;
    v30 = v29;
    n6_1 = n6_1 - v30;
    if ( n6_1 <= 0.0 )
      return 1;
    if ( ++v18 >= n6 )
      return 0;
  }
}
static inline int32_t __fwd_sub_408510_sub_40A8A0(void *a0, int32_t a1, void *a2, int32_t a3, int32_t a4, int32_t a5) { return __sub_40A8A0((uint32_t *)a0, a1, (uint32_t *)a2, a3, a4, a5); }
static inline int32_t __fwd_sub_408510_sub_40AE10(void *a0, int32_t a1) { return __sub_40AE10((uint32_t *)a0, a1); }
static inline uint32_t * __fwd_sub_408510_sub_40AF40(void *a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4) { return __sub_40AF40((uint32_t *)a0, a1, a2, a3, a4); }
static inline uint32_t __fwd_sub_408510_sub_40B330(void *a0, void *a1) { return __sub_40B330((uint32_t *)a0, (uint32_t *)a1); }

 BMF_SSE void __sub_408510(int32_t Src, int32_t n0x20000, int32_t i)
{
  alignas(16) uint8_t __hexrays_frame[1348];
  int32_t &n12360 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &n633_1 = *(int32_t *)(__hexrays_frame + 4);
  uint32_t &n11 = *(uint32_t *)(__hexrays_frame + 8);
  int32_t * &v306 = *(int32_t * *)(__hexrays_frame + 12);
  uint8_t * &Src_15 = *(uint8_t * *)(__hexrays_frame + 16);
  uint32_t &Src_16 = *(uint32_t *)(__hexrays_frame + 20);
  int32_t &v309 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v310 = *(int32_t *)(__hexrays_frame + 28);
  uint32_t (&v311)[8] = *(uint32_t (*)[8])(__hexrays_frame + 36);
  int32_t &n64_1 = *(int32_t *)(__hexrays_frame + 68);
  int32_t &v313 = *(int32_t *)(__hexrays_frame + 72);
  __m128i &v314 = *(__m128i *)(__hexrays_frame + 1060);
  int32_t &v315 = *(int32_t *)(__hexrays_frame + 1076);
  int32_t &v316 = *(int32_t *)(__hexrays_frame + 1080);
  int32_t &v317 = *(int32_t *)(__hexrays_frame + 1084);
  int32_t &v318 = *(int32_t *)(__hexrays_frame + 1088);
  int32_t &v319 = *(int32_t *)(__hexrays_frame + 1092);
  int32_t &v320 = *(int32_t *)(__hexrays_frame + 1096);
  int32_t &v321 = *(int32_t *)(__hexrays_frame + 1100);
  int32_t &v322 = *(int32_t *)(__hexrays_frame + 1104);
  int32_t * &m_5 = *(int32_t * *)(__hexrays_frame + 1108);
  int32_t &v324 = *(int32_t *)(__hexrays_frame + 1112);
  int32_t &v325 = *(int32_t *)(__hexrays_frame + 1116);
  uint32_t &v326 = *(uint32_t *)(__hexrays_frame + 1120);
  int32_t &v327 = *(int32_t *)(__hexrays_frame + 1124);
  int32_t &v328 = *(int32_t *)(__hexrays_frame + 1128);
  int32_t &v329 = *(int32_t *)(__hexrays_frame + 1132);
  int32_t &v330 = *(int32_t *)(__hexrays_frame + 1136);
  uint32_t &v331 = *(uint32_t *)(__hexrays_frame + 1140);
  int32_t &n24_8 = *(int32_t *)(__hexrays_frame + 1144);
  int32_t &n2_5 = *(int32_t *)(__hexrays_frame + 1148);
  uint8_t * &Src_2 = *(uint8_t * *)(__hexrays_frame + 1152);
  int32_t &v335 = *(int32_t *)(__hexrays_frame + 1156);
  int32_t &v336 = *(int32_t *)(__hexrays_frame + 1160);
  uint32_t &Src_18 = *(uint32_t *)(__hexrays_frame + 1164);
  uint32_t &Src_12 = *(uint32_t *)(__hexrays_frame + 1168);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 1172);
  int32_t &n0xFFFFFF_2 = *(int32_t *)(__hexrays_frame + 1176);
  void * &kk_2 = *(void * *)(__hexrays_frame + 1180);
  uint32_t &Src_8 = *(uint32_t *)(__hexrays_frame + 1184);
  uint8_t * &Src_7 = *(uint8_t * *)(__hexrays_frame + 1188);
  int32_t &v344 = *(int32_t *)(__hexrays_frame + 1192);
  int32_t &n2_7 = *(int32_t *)(__hexrays_frame + 1196);
  int32_t &n24 = *(int32_t *)(__hexrays_frame + 1200);
  int32_t &n2_8 = *(int32_t *)(__hexrays_frame + 1204);
  int32_t &v348 = *(int32_t *)(__hexrays_frame + 1208);
  int32_t &n224 = *(int32_t *)(__hexrays_frame + 1212);
  int32_t &n4_4 = *(int32_t *)(__hexrays_frame + 1216);
  int32_t * &m_1 = *(int32_t * *)(__hexrays_frame + 1220);
  int32_t &v352 = *(int32_t *)(__hexrays_frame + 1224);
  char * &v353 = *(char * *)(__hexrays_frame + 1228);
  int32_t &n6_1 = *(int32_t *)(__hexrays_frame + 1232);
  uint32_t &v355 = *(uint32_t *)(__hexrays_frame + 1236);
  int32_t &v356 = *(int32_t *)(__hexrays_frame + 1240);
  uint32_t &v357 = *(uint32_t *)(__hexrays_frame + 1244);
  int32_t &n2_2 = *(int32_t *)(__hexrays_frame + 1248);
  int32_t &kk_1 = *(int32_t *)(__hexrays_frame + 1252);
  void * &Block = *(void * *)(__hexrays_frame + 1256);
  uint32_t &n0x20000_1 = *(uint32_t *)(__hexrays_frame + 1260);
  int32_t &v362 = *(int32_t *)(__hexrays_frame + 1264);
  int32_t &v363 = *(int32_t *)(__hexrays_frame + 1268);
  int32_t &v364 = *(int32_t *)(__hexrays_frame + 1272);
  uint8_t * &Src_5 = *(uint8_t * *)(__hexrays_frame + 1276);
  int32_t &v366 = *(int32_t *)(__hexrays_frame + 1280);
  int32_t &n2_1 = *(int32_t *)(__hexrays_frame + 1284);
  int32_t &v368 = *(int32_t *)(__hexrays_frame + 1288);
  int32_t &v369 = *(int32_t *)(__hexrays_frame + 1292);
  int32_t &v370 = *(int32_t *)(__hexrays_frame + 1296);
  uint32_t * &buf_4 = *(uint32_t * *)(__hexrays_frame + 1300);
  int32_t &v372 = *(int32_t *)(__hexrays_frame + 1304);
  ;
  __m128i v25, v54, v55, v56, v57, *v58;
  bool v26, v146;
  char *v10, *buf, v13, *v18, v19, v21, v31, v33, *v37, *v38, v43, *v45, *v46, *buf_1, v48,
       *n633_8, *n633_10, v82, *v91, *v117, *v120, *v124, *n633_4, *v233, *jj_1, v258, n8_2;
  int16_t v96, v98, v138, v169, v211;
  int32_t __sub_408510_n0xFFFFFF, i_1, v5, v6, v7, v8, n64, v11, n0x4000_1, j, n0x4000,
          __sub_408510_n256, n240, n15, n7_1, v40, v42, n256_1, __sub_408510_buf_2,
          __sub_408510_n5, n11_2, v63, v64, n633_9, v67, n11_3, v69, v71, v73, *v74, *v76, v80,
          v81, n633_2, v86, v87, v88, v89, v92, n633_3, v94, v95, v100, v101, __sub_408510_n2,
          n0xFFFFFF_1, n6_2, n12, v107, v108, kk_10, v112, v113, v114, v115, v116, v118, v119,
          v121, v125, v126, v129, v131, kk_11, n64_2, v134, v135, v136, v139, v140, n4_7, n24_3,
          n2_4, n2_6, v147, __sub_408510_n4_1, v150, __sub_408510_n4_2, v152, v153, v155, v156,
          v157, v158, v159, v160, *v161, v162, v163, v165, n2_11, n2_12, kk_6, n24_4, v173, v174,
          v175, kk_7, v178, v180, *m_4, v182, v183, *m_6, v185, *m_3, v188, v189, *m_7, v191,
          v192, *v193, *ii, v195, v196, v197, v199, v201, v202, v203, v204, v205, v210, v213,
          n24_1, n4_9, n2_9, n4_8, v221, *m_2, n4_5, v224, v225, v226, n4_3, v229, n2_10, n4_6,
          v232, v234, jj, v237, __sub_408510_n2_3, v239, v242, kk_12, *v244, n633_6, v247, v248,
          v249, v251, n633_5, v253, v254, v256, __sub_408510_n8, __sub_408510_n256_2, kk, n24_2,
          v265, v266, v268, v269, v270, v271, v272, n8_1, v275, v276, v279, v280, v281, n6, n276,
          v285, v287, v288, v289, v290, v291, v292, v293, v294, v295, v297, kk_3, v300, v301,
          kk_4;
  uint16_t *kk_5, *kk_9, *kk_8, *kk_13;
  uint32_t *v22, v23, *v27, *v29, n7, n633_7, n11_1, n0x8000_1, n0x8000_2, v52, n0x8000, k,
           __sub_408510_m, __sub_408510_n633, n11_4, v78, v79, n9, n, *buf_3, Src_11, *buf_12,
           *v110, v122, v127, v128, *v130, *buf_6, *buf_7, *buf_8, *v164, *__sub_408510_p_n2,
           v177, v179, *buf_9, *buf_11, *n24_6, *n24_7, *buf_10, *n24_5, Src_13, n11_6, n11_5,
           *v255, v259, v263, v274, v277, n0x18, v286, *v296, *buf_5;
  uint8_t *__sub_408510_Src_1, *v34, v39, *v41, *Src_3, *Src_4, *v154, *Src_17, *Src_14, *Src_9,
          *Src_10, *Src_6;
  void *v83, *v123;
  if ( ::__n2 )
  {
    ::__n4_1 = 4;
    __sub_408510_n0xFFFFFF = -1;
  }
  else
  {
    ::__n4_1 = 3;
    __sub_408510_n0xFFFFFF = 0xFFFFFF;
  }
  i_1 = i;
  ::__n2_3 = 2;
  v5 = 1;
  v6 = -1;
  __n3 = 3;
  ::__n0xFFFFFF = __sub_408510_n0xFFFFFF;
  ::__n4_2 = 4;
  __dword_442DC4[0] = 0;
  n633_1 = 0;
  ::__n5 = 5;
  ::__p_n2 = (int32_t)&::__n2_3;
  __dword_4433F8 = 0;
  n64_1 = 0;
  while ( 1 )
  {
    *(&n633_1 + v5) = i_1 + *(&n12360 + v5);
    v7 = v5 + v6;
    v8 = v5;
    if ( v5 >= 0 )
      break;
LABEL_13:
    --v6;
    ++v5;
  }
  n64 = n64_1;
  v313 = v6;
  while ( 1 )
  {
    v10 = (char *)*(&n633_1 + v8);
    __dword_442BC4[2 * n64] = 0;
    ::__m[2 * n64] = (int32_t)&v10[v7];
    if ( ++n64 == 64 )
      break;
    if ( v8 != v5 )
    {
      if ( v8 )
      {
        v11 = v313;
        __dword_442BC4[2 * n64] = 0;
        ::__m[2 * n64++] = (int32_t)&v10[v8 + v11];
        if ( n64 == 64 )
          break;
      }
    }
    ++v7;
    if ( --v8 < 0 )
    {
      v6 = v313;
      n64_1 = n64;
      i_1 = i;
      goto LABEL_13;
    }
  }
  buf = (char *)malloc(0x4000u);
  memset(buf,0,0x4000);
  n0x4000_1 = 0;
  for ( j = 0; j < 64; ++j )
  {
    n0x4000 = ::__m[2 * j];
    if ( n0x4000 <= 0 || n0x4000 >= 0x4000 || buf[n0x4000] )
    {
      do
        ++n0x4000_1;
      while ( buf[n0x4000_1] );
      ::__m[2 * j] = n0x4000_1;
      n0x4000 = n0x4000_1;
    }
    buf[n0x4000] = 1;
  }
  free(buf);
  __sub_408510_Src_1 = (uint8_t *)Src;
  n0x20000_1 = n0x20000;
  __fwd_sub_408510_sub_40AF40(&Src_15, Src, i, n0x20000, 1);
  kk_2 = malloc(2 * n0x20000_1 + 8);
  kk_1 = (int32_t)kk_2;
  v18 = (char *)malloc(0x40A00u);
  Block = v18;
  if ( v18 )
  {
    __sub_408510_n256 = 256;
    if ( __dword_443364 )
      __sub_408510_n256 = 4096;
    ::__n256_2[0] = __sub_408510_n256;
    memset(v18 + 198912,0,65792);
    v22 = Block;
    v23 = ((uint32_t)Block + 198671) & 0xFFFFFFF0;
    v314 = 0;
    *((uint64_t *)Block + 24832) = 0;
    v22[49666] = 0;
    *((uint16_t *)v22 + 99334) = 0;
    *((uint8_t *)v22 + 198670) = 0;
    *((uint64_t *)v22 + 24862) = 0;
    *((uint64_t *)v22 + 24863) = 0;
    n240 = 240;
    do
    {
      v25 = _mm_load_si128(&v314);
      *(__m128i *)(v23 + n240 - 16) = v25;
      *(__m128i *)(v23 + n240 - 32) = v25;
      *(__m128i *)(v23 + n240 - 48) = v25;
      *(__m128i *)(v23 + n240 - 64) = v25;
      *(__m128i *)(v23 + n240 - 80) = v25;
      n240 -= 80;
    }
    while ( n240 );
    v26 = __dword_443364 == 0;
    v27 = Block;
    *(uint32_t *)Block = 63;
    if ( v26 )
    {
      v27[1] = 0;
      n15 = 0;
    }
    else
    {
      v29 = v27;
      v27[1] = 30;
      n7 = 0;
      Src_2 = (uint8_t *)Src;
      do
      {
        v31 = __byte_439409[2 * n7];
        n7_1 = n7;
        LOBYTE(v29[n7_1 + 2]) = __byte_439408[2 * n7];
        v33 = 2 * n7 + 1;
        *((uint8_t *)v29 + 2 * n7 + 520) = 2 * n7;
        BYTE1(v29[n7_1 + 2]) = n7_1 * 2;
        BYTE2(v29[n7_1 + 2]) = v31;
        *((uint8_t *)v29 + 2 * n7 + 521) = v33;
        HIBYTE(v29[n7_1 + 2]) = v33;
        ++n7;
      }
      while ( n7 < 7 );
      __sub_408510_Src_1 = Src_2;
      v34 = Block;
      *((uint8_t *)Block + 36) = 1;
      v34[534] = 14;
      v34[37] = 14;
      n15 = 15;
    }
    n633_7 = 256 - n15;
    n11 = (uint32_t)(256 - n15) >> 1;
    if ( n11 )
    {
      n633_1 = 256 - n15;
      Src_2 = __sub_408510_Src_1;
      n11_1 = 0;
      v37 = (char *)Block + n15;
      v38 = (char *)Block + 2 * n15;
      do
      {
        v38[4 * n11_1 + 8] = 0;
        v39 = n15 + 2 * n11_1;
        v37[2 * n11_1 + 520] = v39;
        *(uint16_t *)&v38[4 * n11_1 + 9] = v39++;
        v37[2 * n11_1 + 521] = v39;
        v38[4 * n11_1++ + 11] = v39;
      }
      while ( n11_1 < n11 );
      n633_7 = n633_1;
      __sub_408510_Src_1 = Src_2;
      v40 = 2 * n11_1 + 1;
    }
    else
    {
      v40 = 1;
    }
    if ( n633_7 > v40 - 1 )
    {
      v41 = Block;
      v42 = v40 + n15;
      *((uint8_t *)Block + 2 * v42 + 6) = 0;
      v43 = v40 + n15 - 1;
      v41[n15 + 519 + v40] = v43;
      v41[2 * v42 + 7] = v43;
    }
    Src_2 = __sub_408510_Src_1;
    n256_1 = 1;
    v45 = (char *)Block + 776;
    v46 = (char *)Block;
    do
    {
      memcpy(v45,v46,776);
      v45 += 776;
      ++n256_1;
    }
    while ( n256_1 < 256 );
    __sub_408510_Src_1 = Src_2;
  }
  else
  {
    Block = nullptr;
    v314 = 0;
  }
  buf_1 = (char *)malloc(0x342FCu);
  __sub_408510_buf_2 = (int32_t)buf_1;
  if ( buf_1 )
    memset(buf_1,0,213756);
  else
    __sub_408510_buf_2 = 0;
  ::__buf_2 = (void *)__sub_408510_buf_2;
  n0x8000_1 = __sub_408510_buf_2 & 0xF;
  if ( (__sub_408510_buf_2 & 0xF) != 0 )
  {
    if ( (__sub_408510_buf_2 & 3) != 0 )
    {
      n0x8000 = 0;
      goto LABEL_52;
    }
    n0x8000_1 = (16 - n0x8000_1) >> 2;
    n0x8000_2 = 0;
    v52 = (uint32_t)(__sub_408510_Src_1 - 0x4000);
    do
      *(uint32_t *)(__sub_408510_buf_2 + 4 * n0x8000_2++ + 0x10000) = v52;
    while ( n0x8000_2 < n0x8000_1 );
  }
  else
  {
    v52 = (uint32_t)(__sub_408510_Src_1 - 0x4000);
  }
  n0x8000 = 0x8000 - (-n0x8000_1 & 0xF);
  v54 = _mm_cvtsi32_si128(v52);
  v55 = _mm_unpacklo_epi32(v54, v54);
  v56 = _mm_unpacklo_epi64(v55, v55);
  do
  {
    *(__m128i *)(__sub_408510_buf_2 + 4 * n0x8000_1 + 0x10000) = v56;
    *(__m128i *)(__sub_408510_buf_2 + 4 * n0x8000_1 + 65552) = v56;
    *(__m128i *)(__sub_408510_buf_2 + 4 * n0x8000_1 + 65568) = v56;
    *(__m128i *)(__sub_408510_buf_2 + 4 * n0x8000_1 + 65584) = v56;
    n0x8000_1 += 16;
  }
  while ( n0x8000_1 < n0x8000 );
LABEL_52:
  while ( n0x8000 < 0x8000 )
    *(uint32_t *)(__sub_408510_buf_2 + 4 * n0x8000++ + 0x10000) = __sub_408510_Src_1 - 0x4000;
  memset((char *)(__sub_408510_buf_2 + 196608),255,0x4000);
  v57 = _mm_load_si128(&v314);
  *(uint64_t *)(__sub_408510_buf_2 + 213692) = v57.m128i_i64[0];
  v58 = (__m128i *)((__sub_408510_buf_2 + 213707) & 0xFFFFFFF0);
  *(uint32_t *)(__sub_408510_buf_2 + 213700) = 0;
  *(uint16_t *)(__sub_408510_buf_2 + 213704) = 0;
  *(uint8_t *)(__sub_408510_buf_2 + 213706) = 0;
  *(uint64_t *)(__sub_408510_buf_2 + 213740) = v57.m128i_i64[0];
  *(uint64_t *)(__sub_408510_buf_2 + 213748) = v57.m128i_i64[0];
  *v58 = v57;
  v58[1] = v57;
  v58[2] = v57;
  v314.m128i_i32[1] = __sub_408510_buf_2;
  Src_2 = __sub_408510_Src_1;
  n633_8 = nullptr;
  __sub_408510_n5 = 5;
  for ( k = 0; k < 0x20; ++k )
  {
    n633_1 = (int32_t)n633_8;
    n11_2 = ::__m[4 * k];
    *(uint8_t *)(n11_2 + v314.m128i_i32[1] + 196608) = 2 * k;
    n11 = n11_2;
    v63 = __sub_408510_n5 - ((int32_t)(2 * k) >= 1 << ((7 - __sub_408510_n5) & 31));
    v64 = v314.m128i_i32[1];
    n633_9 = n633_1;
    *(uint8_t *)(v314.m128i_i32[1] + 2 * k + 212992) = v63;
    if ( n11_2 < n633_9 )
      n11_2 = n633_9;
    n633_10 = (char *)__dword_442BC8[4 * k];
    n11 = n11_2;
    n633_1 = (int32_t)n633_10;
    v67 = 2 * k + 1;
    n633_10[v64 + 196608] = v67;
    n11_3 = n11;
    __sub_408510_n5 = v63 - (v67 >= 1 << ((7 - v63) & 31));
    *(uint8_t *)(v314.m128i_i32[1] + 2 * k + 212993) = __sub_408510_n5;
    n633_8 = (char *)n633_1;
    if ( n633_1 < n11_3 )
      n633_8 = (char *)n11_3;
  }
  v69 = v314.m128i_i32[1];
  Src_3 = Src_2;
  __Src_0 = (int32_t)&n633_8[(uint32_t)Src_2];
  v71 = 0;
  for ( __sub_408510_m = 0; __sub_408510_m < 0xC; ++__sub_408510_m )
  {
    v73 = ::__m[4 * __sub_408510_m];
    if ( v73 < v71 )
      v73 = v71;
    v71 = v73;
    if ( __dword_442BC8[4 * __sub_408510_m] >= v73 )
      v71 = __dword_442BC8[4 * __sub_408510_m];
  }
  n11 = 0;
  ::__Src_1 = (int32_t)&Src_2[v71];
  v74 = __dword_438980;
  if ( ::__n2 == 2 )
    v74 = __dword_438920;
  __sub_408510_n633 = 633;
  if ( ::__n2 == 2 )
    __sub_408510_n633 = 277;
  v306 = v74;
  if ( __sub_408510_n633 >> 1 )
  {
    v76 = v74;
    v314.m128i_i32[0] = __sub_408510_n633 >> 1;
    n11_4 = n11;
    v78 = 0;
    n633_1 = __sub_408510_n633;
    do
    {
      v79 = (2 * v78 == v76[n11_4 + 1]) + n11_4;
      *(uint8_t *)(v69 + 2 * v78 + 213056) = v79;
      n11_4 = (2 * v78 + 1 == v76[v79 + 1]) + v79;
      *(uint8_t *)(v69 + 2 * v78++ + 213057) = n11_4;
    }
    while ( v78 < v314.m128i_i32[0] );
    __sub_408510_n633 = n633_1;
    Src_3 = Src_2;
    n11 = n11_4;
    v80 = 2 * v78 + 1;
  }
  else
  {
    v80 = 1;
  }
  if ( __sub_408510_n633 > v80 - 1 )
    *(uint8_t *)(v80 + v69 + 213055) = (v80 - 1 == v306[n11 + 1]) + n11;
  n12360 = 12360;
  v81 = (uint8_t)__byte_438A00[Src_3[3]]
      ^ (8
       * ((uint8_t)__byte_438A00[Src_3[2]]
        ^ (8 * ((uint8_t)__byte_438A00[Src_3[1]] ^ (8 * (uint8_t)__byte_438A00[*Src_3])))));
  __dword_443408 = malloc(0x3048u);
  __dword_442DE0 = malloc(0x23B4u);
  n633_1 = (int32_t)malloc(0x23B4u);
  ::__n633 = (void *)n633_1;
  memset((char *)n633_1,0,9140);
  v83 = __dword_442DE0;
  v314.m128i_i32[1] = v69;
  n633_2 = n633_1;
  n9 = 0;
  Src_2 = Src_3;
  do
  {
    v86 = __dword_438B00[2 * n9];
    v87 = __dword_438B04[2 * n9];
    v88 = (int32_t)v83 + 4 * v86;
    v89 = n633_2 + 4 * v86;
    __dword_442DE4[2 * n9] = v88;
    __dword_442E44[2 * n9] = v89;
    v83 = (void *)(v88 + 4 * v87);
    __dword_442DE8[2 * n9] = (int32_t)v83;
    n633_2 = v89 + 4 * v87;
    __dword_442E48[2 * n9++] = n633_2;
  }
  while ( n9 < 9 );
  v368 = v81;
  for ( n = 0; n < 0xC; ++n )
  {
    if ( __dword_438B00[n] > 0 )
    {
      v91 = (char *)__off_4410C0[n];
      v92 = (int32_t)*(&::__n633 + n);
      n633_1 = __dword_438B00[n];
      n633_3 = 0;
      n11 = (uint8_t)__byte_439418[n];
      do
      {
        v94 = 1 << ((n11 - v91[n633_3++]) & 31);
        v95 = v314.m128i_i32[1];
        *(uint32_t *)(v92 + 4 * n633_3 - 4) = v94;
        *(uint32_t *)(v95 + 4 * n + 213692) += v94;
      }
      while ( n633_3 < n633_1 );
    }
  }
  v96 = (uint8_t)__byte_438A00[Src_2[4]];
  Src_4 = Src_2;
  n0x20000_1 += (uint32_t)Src_2;
  v98 = v96 ^ (8 * v368);
  buf_3 = ::__buf_2;
  buf_4 = ::__buf_2;
  Src_5 = Src_2;
  v100 = v98 & 0x7FFF;
  v101 = *((uint32_t *)::__buf_2 + v100 + 0x4000);
  v368 = v100;
  *((uint32_t *)::__buf_2 + ((uint16_t)Src_2 & 0x3FFF)) = v101;
  __sub_408510_n2 = ::__n2;
  n0xFFFFFF_1 = ::__n0xFFFFFF;
  buf_3[v100 + 0x4000] = Src_4;
  n2_1 = __sub_408510_n2;
  n4 = ::__n4_1;
  n0xFFFFFF_2 = n0xFFFFFF_1;
  Src_12 = __Src_0;
  Src_18 = ::__Src_1;
  v336 = __dword_442E68;
  v335 = __dword_442E6C;
  n6_1 = 1;
  n6_2 = 1;
  n2_7 = 0;
  n24 = 0;
  n2_2 = 1;
  v362 = 1;
LABEL_84:
  while ( 2 )
  {
    if ( n2_1 == 2 )
    {
      v280 = v309;
      v281 = v310;
      Src_8 = Src_16;
      Src_7 = Src_15;
      v344 = __dword_443364;
      n6 = n6_2;
      Src_6 = Src_5;
      while ( 1 )
      {
        Src_6 += n6;
        n276 = n0x20000_1 - (uint32_t)Src_6;
        if ( (int32_t)(n0x20000_1 - (uint32_t)Src_6) < 6 )
          break;
        if ( *(uint32_t *)Src_6 || *((uint16_t *)Src_6 + 2) )
        {
          v108 = v344;
          Src_5 = Src_6;
          goto LABEL_94;
        }
        if ( n276 > 276 )
          n276 = 276;
        n6 = 6;
        if ( n276 <= 6 )
          goto LABEL_356;
        do
        {
          if ( Src_6[n6] )
            break;
          ++n6;
        }
        while ( n6 < n276 );
        if ( n6 <= 8 )
        {
LABEL_356:
          n6_1 = n6;
          v285 = __fwd_sub_408510_sub_40A8A0(Block, (int32_t)Src_6, (uint32_t *)&Src_15, n6, -1, 0);
          n6 = n6_1;
          if ( !v285 )
          {
            v108 = v344;
            Src_5 = Src_6;
            goto LABEL_94;
          }
        }
        if ( v344 )
        {
          if ( (uint32_t)Src_6 <= Src_8 )
          {
            v293 = *(Src_6 - 1);
            v324 = v293;
            if ( Src_6 <= Src_7 )
            {
              v325 = v281;
              v324 = v311[v293] | (2 * (v293 & 1)) | v293 & 1;
            }
            else
            {
              v294 = v293;
              v295 = Src_6[v280];
              v314.m128i_i32[0] = v280;
              v325 = v281;
              v324 = v295 & 1
                   | *(uint32_t *)((char *)v311 + ((v295 + Src_6[v280 + 1] + v294 + Src_6[v280 - 1]) & 0xFFFFFFFC))
                   | (2 * (v294 & 1));
            }
          }
          else
          {
            v286 = Src_6[v280];
            v287 = *(Src_6 - 1);
            v314.m128i_i64[0] = __PAIR64__(v286, v280);
            n6_1 = n6;
            v288 = Src_6[v281 + 1];
            v324 = v287;
            v289 = Src_6[v281] + Src_6[v280 + 2] + v288 + *(Src_6 - 2);
            v290 = *(Src_6 - 4);
            v314.m128i_i32[2] = v289;
            v291 = *(Src_6 - 3);
            v314.m128i_i32[3] = v324 + v286;
            v292 = Src_6[v281 + 3]
                 + Src_6[v281 + 2]
                 + Src_6[v281 - 2]
                 + Src_6[v314.m128i_i32[0] - 3]
                 + Src_6[v314.m128i_i32[0] + 4]
                 + Src_6[v281 - 1]
                 + Src_6[v280 + 3]
                 + Src_6[v280 - 2]
                 + 3 * (Src_6[v280 + 1] + Src_6[v280 - 1])
                 + v290
                 + v291
                 + 4 * (v324 + v286)
                 + 2 * v314.m128i_i32[2];
            n6 = n6_1;
            v324 = v314.m128i_i8[4] & 1 | v311[v292 >> 5] | (2 * (v324 & 1));
          }
        }
        else
        {
          v324 = *(Src_6 - 1);
        }
        v325 = v281;
        v296 = Block;
        v297 = *((uint32_t *)Block + 194 * v324);
        ++*((uint32_t *)Block + v297 + 49664);
        ++v296[257 * v297 + 49728];
        kk_3 = kk_1;
        buf_5 = buf_4;
        *(uint16_t *)kk_1 = v297 + 12;
        v300 = *((uint8_t *)buf_5 + n6 + 213056);
        ++buf_5[53434];
        ++*(uint32_t *)(v335 + 4 * v300);
        *(uint16_t *)(kk_3 + 2) = ((uint16_t)v300 << 7) | 0xB;
        v281 = v325;
        v26 = __dword_439300[v300] == 0;
        kk_1 = kk_3 + 4;
        if ( !v26 )
        {
          v301 = n6 - __dword_438920[v300];
          kk_4 = kk_1;
          *(uint16_t *)kk_1 = v301;
          kk_1 = kk_4 + 2;
        }
      }
      v108 = v344;
      Src_5 = Src_6;
      if ( n276 )
        goto LABEL_94;
      __sub_408510_n2_3 = n2_2;
      v239 = v362;
      Src_9 = Src_6;
      Src_10 = Src_2;
      goto LABEL_271;
    }
    Src_11 = (uint32_t)Src_5;
    v357 = 4 * n2_2;
    v356 = n2_2 - 1;
    v355 = ((uint16_t)n2_2 - 1) | 0xFFFF0080;
    while ( 1 )
    {
      n12 = n0x20000_1 - Src_11 - n6_2;
      if ( n12 >= 12 )
      {
        v369 = 0;
        LOWORD(v136) = v368;
        goto LABEL_118;
      }
      if ( n12 <= 4 )
        break;
      v369 = -1;
      LOWORD(v136) = v368;
      do
      {
LABEL_118:
        buf_6 = buf_4;
        v138 = (uint8_t)__byte_438A00[*(uint8_t *)(++Src_11 + 4)] ^ (uint16_t)(8 * v136);
        v372 = *(uint8_t *)(Src_11 + 4);
        v136 = v138 & 0x7FFF;
        --n6_2;
        buf_4[Src_11 & 0x3FFF] = buf_4[v136 + 0x4000];
        buf_6[v136 + 0x4000] = Src_11;
      }
      while ( n6_2 );
      v139 = *(uint32_t *)Src_11;
      v368 = v136;
      LOWORD(v140) = Src_11;
      if ( v362 )
      {
        n6_1 = 0;
        buf_7 = buf_6;
        while ( 1 )
        {
          v140 = buf_7[v140 & 0x3FFF];
          n4_7 = Src_11 - v140;
          if ( Src_11 - v140 >= 0x4000 )
            break;
          if ( *((uint8_t *)buf_7 + n4_7 + 196608) < 0x40u && v139 == *(uint32_t *)v140 )
            goto LABEL_124;
        }
        v108 = __dword_443364;
        Src_5 = (uint8_t *)Src_11;
        goto LABEL_94;
      }
      buf_8 = buf_6;
      n6_1 = 0;
      while ( 1 )
      {
        v140 = buf_8[v140 & 0x3FFF];
        n4_7 = Src_11 - v140;
        if ( Src_11 - v140 >= 0x4000 )
          break;
        if ( v139 == *(uint32_t *)v140 )
        {
          if ( *((uint8_t *)buf_8 + n4_7 + 196608) < 0x40u )
          {
LABEL_124:
            v327 = v139;
            if ( Src_11 < Src_12 )
            {
              n24_3 = *((uint8_t *)buf_4 + n4_7 + 196608);
              n2_4 = *((uint8_t *)buf_4 + n24_3 + 212992);
              n2_6 = n2_4 + 1;
              v146 = n2_4 < n2_2;
              if ( n2_4 >= n2_2 )
                n2_4 = n2_2;
              n2_5 = n2_4;
              if ( !v146 )
                n2_6 = n2_2;
              v147 = n0x20000_1 - Src_11;
              if ( __dword_439480[n2_6] < (int32_t)(n0x20000_1 - Src_11) )
                v147 = __dword_439480[n2_6];
              __sub_408510_n4_1 = 4;
              if ( *(uint8_t *)(v140 + 4) == v372 )
              {
                n24_8 = n24_3;
                while ( ++__sub_408510_n4_1 < v147 )
                {
                  if ( *(uint8_t *)(__sub_408510_n4_1 + Src_11) != *(uint8_t *)(__sub_408510_n4_1 + v140) )
                    goto LABEL_137;
                  if ( ++__sub_408510_n4_1 >= v147 )
                    break;
                  if ( *(uint8_t *)(__sub_408510_n4_1 + Src_11) != *(uint8_t *)(__sub_408510_n4_1 + v140) )
                  {
LABEL_137:
                    n24_3 = n24_8;
                    goto LABEL_138;
                  }
                }
                n24_3 = n24_8;
              }
LABEL_138:
              if ( __sub_408510_n4_1 <= n6_1 && (__sub_408510_n4_1 != n6_1 || n24_3 >= n24) )
              {
                v139 = v327;
                buf_8 = buf_4;
                continue;
              }
              v26 = __sub_408510_n4_1 == __dword_439480[v357 / 4];
              n6_1 = __sub_408510_n4_1;
              n24 = n24_3;
              n2_7 = n2_5;
              if ( !v26 || n24_3 )
              {
                v139 = v327;
                buf_8 = buf_4;
                v369 = -1;
                continue;
              }
              __sub_408510_n4_2 = __sub_408510_n4_1;
              v152 = v356;
              Src_5 = (uint8_t *)Src_11;
              goto LABEL_155;
            }
            __sub_408510_n4_2 = n6_1;
            Src_5 = (uint8_t *)Src_11;
            n4_9 = __dword_439480[v357 / 4];
            Src_13 = Src_11;
            n2_8 = n2_2;
            n2_9 = n2_2;
            m_1 = ::__m;
            n4_8 = n0x20000_1 - Src_11;
            if ( n4_9 < n4_8 )
              n4_8 = n4_9;
            n4_4 = n4_8;
            v370 = *(uint32_t *)(Src_13 + 1);
            v352 = 1;
            v221 = 1 << ((7 - n2_2) & 31);
            m_2 = m_1;
            n4_5 = n4_4;
            while ( 2 )
            {
              v224 = ::__m[2 * v221];
              ::__m[2 * v221] = 0;
              m_2 -= 2;
              v329 = v224;
              v328 = v221;
              v225 = v327;
              n4_4 = n4_5;
              n2_8 = n2_9;
              n6_1 = __sub_408510_n4_2;
              v226 = v352;
              while ( 1 )
              {
                do
                {
                  do
                  {
LABEL_227:
                    m_2 += 2;
                    Src_14 = &Src_5[-*m_2];
                    if ( v225 == *(uint32_t *)Src_14 )
                      break;
                    m_2 += 2;
                    Src_14 = &Src_5[-*m_2];
                    if ( v225 == *(uint32_t *)Src_14 )
                      break;
                    m_2 += 2;
                    Src_14 = &Src_5[-*m_2];
                  }
                  while ( v225 != *(uint32_t *)Src_14 );
                }
                while ( v370 != *(uint32_t *)&Src_14[v226] );
                if ( Src_14 == Src_5 )
                  break;
                if ( Src_14[4] == v372 )
                {
                  m_1 = m_2;
                  v352 = v226;
                  n4_3 = 4;
                  do
                  {
                    if ( ++n4_3 >= n4_4 )
                      break;
                    if ( Src_5[n4_3] != Src_14[n4_3] )
                      break;
                    if ( ++n4_3 >= n4_4 )
                      break;
                    if ( Src_5[n4_3] != Src_14[n4_3] )
                      break;
                    if ( ++n4_3 >= n4_4 )
                      break;
                    if ( Src_5[n4_3] != Src_14[n4_3] )
                      break;
                    if ( ++n4_3 >= n4_4 )
                      break;
                  }
                  while ( Src_5[n4_3] == Src_14[n4_3] );
                  m_2 = m_1;
                  v226 = v352;
                  v225 = v327;
                  if ( n4_3 <= n6_1 )
                    goto LABEL_227;
                }
                else
                {
                  if ( n6_1 >= 4 )
                    goto LABEL_227;
                  n4_3 = 4;
                }
                n6_1 = n4_3;
                n2_7 = n2_8;
                n24 = ((char *)m_2 - (char *)::__m) >> 3;
                if ( n4_3 >= n4_4 )
                {
                  __sub_408510_n4_2 = n6_1;
                  v152 = v356;
                  ::__m[2 * v328] = v329;
                  goto LABEL_155;
                }
                v226 = n4_3 - 3;
                v370 = *(uint32_t *)&Src_5[n4_3 - 3];
              }
              v229 = v328;
              n4_5 = n4_4;
              n2_10 = n2_8;
              v352 = v226;
              __sub_408510_n4_2 = n6_1;
              *m_2 = v329;
              if ( n2_10 != 1 )
              {
                n4_6 = __dword_439480[n2_10];
                if ( n4_5 <= n4_6 || (n4_5 = __dword_439480[n2_10], __sub_408510_n4_2 < n4_6) )
                {
                  n2_9 = n2_10 - 1;
                  v221 = 2 * v229;
                  continue;
                }
              }
              break;
            }
            v152 = v356;
LABEL_155:
            if ( v362 )
            {
              v153 = ::__m[2 * n24];
              v326 = 8 * n24;
              v154 = &Src_5[-v153];
              if ( (__sub_408510_n4_2 != 6
                 || (uint32_t)(Src_5 + 11) > n0x20000_1
                 || *(uint32_t *)(Src_5 + 7) != *(uint32_t *)(v154 + 7)
                 || Src_5[6] != v154[6])
                && !__fwd_sub_408510_sub_40A8A0(Block, (int32_t)Src_5, (uint32_t *)&Src_15, __sub_408510_n4_2, n24, 0) )
              {
                v108 = __dword_443364;
                goto LABEL_94;
              }
              if ( __dword_443364 )
              {
                if ( (uint32_t)Src_5 <= Src_16 )
                {
                  v161 = (int32_t *)*(Src_5 - 1);
                  v306 = v161;
                  if ( Src_5 <= Src_15 )
                  {
                    v160 = (uint8_t)v161 & 1 | v311[(uint32_t)v161] | (2 * ((uint8_t)v161 & 1));
                  }
                  else
                  {
                    v162 = Src_5[v309];
                    v163 = Src_5[v309 + 1];
                    n11 = Src_5[v309 - 1];
                    v160 = v162 & 1
                         | *(uint32_t *)((char *)v311 + (((uint32_t)v306 + n11 + v163 + v162) & 0xFFFFFFFC))
                         | (2 * ((uint8_t)v306 & 1));
                    if ( n2_1 == 1 )
                    {
                      v195 = v160 < v311[(uint32_t)v306] || v160 < v311[v163] || v160 < v311[n11];
                      v160 |= (v195 << 7) | (8 * (n11 & 1)) | (4 * (v163 & 1));
                    }
                  }
                }
                else
                {
                  v155 = Src_5[v309];
                  v156 = *(Src_5 - 1);
                  v157 = Src_5[v309 + 1];
                  n6_1 = __sub_408510_n4_2;
                  v318 = v155;
                  v317 = v156;
                  v315 = Src_5[v309 - 1];
                  v158 = Src_5[v310 + 1];
                  v316 = v157;
                  v319 = Src_5[v310] + Src_5[v309 + 2] + v158 + *(Src_5 - 2);
                  v159 = *(Src_5 - 3);
                  v320 = v156 + v155;
                  __sub_408510_n4_2 = n6_1;
                  v160 = v155 & 1
                       | v311[(Src_5[v310 + 3]
                             + Src_5[v310 + 2]
                             + Src_5[v310 - 2]
                             + Src_5[v309 - 3]
                             + Src_5[v309 + 4]
                             + Src_5[v310 - 1]
                             + Src_5[v309 + 3]
                             + Src_5[v309 - 2]
                             + 3 * (v316 + v315)
                             + *(Src_5 - 4)
                             + v159
                             + 4 * (v156 + v155)
                             + 2 * v319) >> 5]
                       | (2 * (v156 & 1));
                  if ( n2_1 == 1 )
                  {
                    v196 = v160 < v311[v317] || v160 < v311[v316] || v160 < v311[v315];
                    v160 |= (v196 << 7) | (8 * (v315 & 1)) | (4 * (v316 & 1));
                  }
                }
              }
              else
              {
                v160 = *(Src_5 - 1);
              }
              v164 = Block;
              v165 = *((uint32_t *)Block + 194 * v160);
              ++*((uint32_t *)Block + v165 + 49664);
              kk_5 = (uint16_t *)kk_1;
              ++v164[257 * v165 + 49728];
              *kk_5 = v165 + 12;
              v362 = 0;
              kk_1 = (int32_t)(kk_5 + 1);
            }
            else
            {
              n2_11 = n2_7;
              ++buf_4[v152 + 53423];
              n2_12 = n2_11 + 1;
              ++*((uint32_t *)*(&::__n633 + v152) + n2_11 + 1);
              v169 = v152 | (((uint16_t)n2_11 + 1) << 7);
              kk_6 = kk_1;
              *(uint16_t *)kk_1 = v169;
              kk_1 = kk_6 + 2;
              if ( n2_11 == n2_2 )
              {
                v326 = 8 * n24;
              }
              else
              {
                n24_4 = n24;
                __sub_408510_p_n2 = (uint32_t *)(::__p_n2 - 4);
                *__sub_408510_p_n2 = n2_2;
                ::__p_n2 = (int32_t)__sub_408510_p_n2;
                n2_2 = n2_7;
                n2_7 = n2_12;
                n2_1 = ::__n2;
                v326 = 8 * n24_4;
                v357 = 4 * n2_2;
              }
            }
            v173 = *((uint8_t *)buf_4 + __sub_408510_n4_2 + 213056);
            n6_2 = __dword_438980[v173];
            v174 = n2_7 + 4;
            v175 = ((uint8_t)(n24 << (n2_7 & 31)) | (uint8_t)v173) & 0x7F;
            ++buf_4[v174 + 53423];
            ++*((uint32_t *)*(&::__n633 + v174) + v175);
            kk_7 = kk_1;
            *(uint16_t *)kk_1 = v174 | ((uint16_t)v175 << 7);
            kk_1 = kk_7 + 2;
            if ( n6_2 >= __dword_439480[v357 / 4] && n2_2 < 5 )
            {
              n2_2 = *(uint32_t *)::__p_n2;
              ::__p_n2 += 4;
            }
            v177 = v326;
            v178 = n6_2 + __dword_442BC4[v326 / 4];
            v26 = n24 == 0;
            __dword_442BC4[v326 / 4] = v178;
            if ( !v26 && v178 > *(int32_t *)((char *)&__dword_442BBC + v177) )
            {
              v179 = v177;
              v180 = *(int32_t *)((char *)::__m + v177);
              v322 = *(int32_t *)((char *)__dword_442BC4 + v177);
              m_4 = (int32_t *)((char *)&__dword_442BB8 + v177);
              v182 = *m_4;
              v183 = m_4[1];
              m_5 = m_4;
              *(int32_t *)((char *)::__m + v179) = v182;
              *(int32_t *)((char *)__dword_442BC4 + v179) = v183;
              ++*((uint8_t *)buf_4 + v182 + 196608);
              if ( m_4 != ::__m )
              {
                m_6 = m_4;
                v185 = v322;
                buf_9 = buf_4;
                v321 = v180;
                n6_1 = n6_2;
                do
                {
                  m_3 = m_6 - 2;
                  v188 = *(m_6 - 1);
                  if ( v188 >= v185 )
                    break;
                  v189 = *m_3;
                  *m_6 = *m_3;
                  m_6[1] = v188;
                  m_6 -= 2;
                  ++*((uint8_t *)buf_9 + v189 + 196608);
                }
                while ( m_3 != ::__m );
                v180 = v321;
                n6_2 = n6_1;
                m_5 = m_6;
              }
              m_7 = m_5;
              v191 = v322;
              *m_5 = v180;
              m_7[1] = v191;
              *((uint8_t *)buf_4 + v180 + 196608) = (uint32_t)((char *)m_7 - (char *)::__m) >> 3;
              if ( __dword_442BFC[0] - __dword_442C04 > 20 )
              {
                v192 = __dword_442BC4[0];
                v193 = __dword_442BC8;
                for ( ii = ::__m; v192; v193 += 2 )
                {
                  ii[1] = v192 >> 1;
                  v192 = v193[1];
                  ii += 2;
                }
              }
            }
            goto LABEL_84;
          }
          if ( !v369
            && *(uint32_t *)(v140 + 8) == *(uint32_t *)(Src_11 + 8)
            && *(uint32_t *)(v140 + 4) == *(uint32_t *)(Src_11 + 4) )
          {
            v150 = v369;
            if ( n4_7 >= 4 )
              v150 = Src_11 - v140;
            v369 = v150;
          }
        }
      }
      v197 = v369;
      __sub_408510_n4_2 = n6_1;
      if ( n6_1 )
      {
        v152 = v356;
        Src_5 = (uint8_t *)Src_11;
        goto LABEL_155;
      }
      if ( v369 <= 0 )
      {
        v205 = n0xFFFFFF_2 & v139;
        n633_4 = (char *)__dword_442C80[0];
        __dword_442C80[0] = 0;
        n24 = (int32_t)&__dword_442BB8;
        if ( Src_11 <= Src_18 )
        {
          n633_1 = (int32_t)n633_4;
          n24_5 = (uint32_t *)n24;
          do
          {
            do
            {
              n24_5 += 2;
              Src_17 = (uint8_t *)(Src_11 - *n24_5);
            }
            while ( Src_17 < Src_2 );
          }
          while ( v205 != (n0xFFFFFF_2 & *(uint32_t *)Src_17) );
          n24 = (int32_t)n24_5;
          n633_4 = (char *)n633_1;
        }
        else
        {
          n24_6 = (uint32_t *)n24;
          while ( 1 )
          {
            n24_7 = n24_6 + 2;
            if ( v205 == (n0xFFFFFF_2 & *(uint32_t *)(Src_11 - *n24_7)) )
              break;
            n24_7 += 2;
            if ( v205 == (n0xFFFFFF_2 & *(uint32_t *)(Src_11 - *n24_7)) )
              break;
            n24_6 = n24_7 + 2;
            if ( v205 == (n0xFFFFFF_2 & *(uint32_t *)(Src_11 - *n24_6)) )
            {
              n24 = (int32_t)n24_6;
              goto LABEL_212;
            }
          }
          n24 = (int32_t)n24_7;
        }
LABEL_212:
        __dword_442C80[0] = (int32_t)n633_4;
        n24 = (n24 - (int32_t)::__m) >> 3;
        if ( n24 == 24 || !__fwd_sub_408510_sub_40A8A0(Block, Src_11, (uint32_t *)&Src_15, n4, n24, n2_2) )
        {
          v107 = v356;
          Src_5 = (uint8_t *)Src_11;
          goto LABEL_91;
        }
        buf_10 = buf_4;
        v210 = (int32_t)*(&::__n633 + v356);
        v211 = v355;
        kk_8 = (uint16_t *)kk_1;
        ++buf_4[v356 + 53423];
        v213 = v336;
        *kk_8 = v211;
        ++*(uint32_t *)(v210 + 4);
        n24_1 = n24;
        ++buf_10[53433];
        n6_2 = n4;
        ++*(uint32_t *)(v213 + 4 * n24_1);
        kk_8[1] = ((uint16_t)n24_1 << 7) | 0xA;
        kk_1 = (int32_t)(kk_8 + 2);
      }
      else
      {
        buf_11 = buf_4;
        v199 = (int32_t)*(&::__n633 + v356);
        kk_9 = (uint16_t *)kk_1;
        ++buf_4[v356 + 53423];
        *kk_9 = v355;
        ++*(uint32_t *)(v199 + 4);
        v201 = v336;
        v202 = (v197 >> 11) + 24;
        ++buf_11[53433];
        ++*(uint32_t *)(v201 + 4 * v202);
        kk_9[1] = ((uint16_t)v202 << 7) | 0xA;
        kk_9[2] = v197 & 0x7FF;
        kk_1 = (int32_t)(kk_9 + 3);
        v203 = __dword_4433F8;
        v204 = 63 - (__dword_4433F8 & 3);
        *((uint8_t *)buf_11 + v197 + 196608) = v204;
        __dword_4433F8 = v203 + 1;
        n6_2 = 12;
        *((uint8_t *)buf_11 + ::__m[2 * v204] + 196608) = -1;
        ::__m[2 * v204] = v197;
      }
    }
    v107 = v356;
    Src_5 = (uint8_t *)(n6_2 + Src_11);
    if ( n6_2 + Src_11 != n0x20000_1 )
    {
      if ( v362 )
      {
        v108 = __dword_443364;
        goto LABEL_94;
      }
LABEL_91:
      buf_12 = buf_4;
      v110 = *(&::__n633 + v107);
      kk_10 = kk_1;
      *(uint16_t *)kk_1 = v107;
      ++buf_12[v107 + 53423];
      kk_1 = kk_10 + 2;
      ++*v110;
      if ( n2_2 == 1 )
      {
        v108 = __dword_443364;
        v362 = 1;
      }
      else
      {
        ::__p_n2 -= 4;
        *(uint32_t *)::__p_n2 = n2_2;
        v108 = __dword_443364;
        n2_1 = ::__n2;
        v362 = 1;
        n2_2 = 1;
      }
LABEL_94:
      if ( v108 )
      {
        if ( (uint32_t)Src_5 <= Src_16 )
        {
          v113 = *(Src_5 - 1);
          if ( Src_5 <= Src_15 )
          {
            v112 = v113 & 1 | v311[v113] | (2 * (v113 & 1));
          }
          else
          {
            v114 = Src_5[v309];
            v115 = Src_5[v309 - 1];
            v348 = Src_5[v309 + 1];
            v112 = v114 & 1 | *(uint32_t *)((char *)v311 + ((v348 + v114 + v113 + v115) & 0xFFFFFFFC)) | (2 * (v113 & 1));
            if ( n2_1 == 1 )
            {
              v232 = v112 < v311[v113] || v112 < v311[v348] || v112 < v311[v115];
              v112 |= (v232 << 7) | (8 * (v115 & 1)) | (4 * (v348 & 1));
            }
          }
        }
        else
        {
          v112 = __fwd_sub_408510_sub_40AE10(&Src_15, (int32_t)Src_5);
        }
      }
      else
      {
        v112 = *(Src_5 - 1);
      }
      v116 = *Src_5;
      v364 = 776 * v112;
      v117 = (char *)Block + 776 * v112;
      v118 = *(uint32_t *)v117;
      v119 = (uint8_t)v117[v116 + 520];
      v120 = &v117[2 * v119];
      v121 = (uint8_t)v120[8] + 1;
      v120[8] = v121;
      v366 = v121;
      if ( v119 )
      {
        v353 = &v117[2 * v119];
        if ( (uint8_t)v120[6] <= (int32_t)(uint8_t)v366 )
        {
          v122 = *((uint16_t *)v353 + 3);
          v331 = *((uint16_t *)v353 + 4);
          v123 = Block;
          *((uint16_t *)v353 + 4) = v122;
          ++v117[(v122 >> 8) + 520];
          v124 = &v117[2 * v119 + 6];
          v125 = (int32_t)v123 + v364 + 8;
          if ( v124 != (char *)v125 )
          {
            v126 = (uint8_t)v331;
            v330 = v118;
            do
            {
              if ( (uint8_t)*(v124 - 2) > v126 )
                break;
              v127 = *((uint16_t *)v124 - 1);
              *(uint16_t *)v124 = v127;
              v124 -= 2;
              ++v117[(v127 >> 8) + 520];
            }
            while ( v124 != (char *)v125 );
            v118 = v330;
          }
          v128 = v331;
          *(uint16_t *)v124 = v331;
          v117[(v128 >> 8) + 520] = (uint32_t)&v124[-v125] >> 1;
        }
      }
      v129 = *((uint32_t *)v117 + 1);
      v363 = v129 + 1;
      *((uint32_t *)v117 + 1) = v129 + 1;
      if ( (((uint8_t)v129 + 1) & 0x1F) == 0 )
      {
        n224 = (uint8_t)v117[8];
        if ( n224 >= 8 )
        {
          *(uint32_t *)v117 = 0;
          n64_2 = 0;
          v134 = (8 * v129 + 8 - v363) >> 3;
          while ( 1 )
          {
            v135 = (uint8_t)v117[2 * n64_2++ + 8];
            v134 -= v135;
            if ( v134 < 0 )
              break;
            *(uint32_t *)v117 = n64_2;
          }
          *(uint32_t *)v117 = n64_2 < 64 ? n64_2 : 63;
          if ( n224 >= 224 || v363 >= ::__n256_2[0] )
          {
            v233 = (char *)Block;
            v234 = v364;
            *((uint32_t *)v117 + 1) = 0;
            jj_1 = &v233[v234 + 8];
            for ( jj = (int32_t)&v233[v234 + 520]; jj_1 != (char *)jj; jj_1 += 2 )
            {
              if ( !*jj_1 )
                break;
              v237 = (uint8_t)*jj_1 >> 1;
              *jj_1 = v237;
              *((uint32_t *)v117 + 1) += v237;
            }
          }
        }
      }
      v130 = Block;
      ++*((uint32_t *)Block + v118 + 49664);
      v131 = v119 + 1;
      n6_2 = 1;
      ++v130[257 * v118 + 49728 + v131];
      LOWORD(v131) = (v118 + 12) | ((uint16_t)v131 << 7);
      kk_11 = kk_1;
      *(uint16_t *)kk_1 = v131;
      kk_1 = kk_11 + 2;
      continue;
    }
    break;
  }
  __sub_408510_n2_3 = n2_2;
  v239 = v362;
  Src_9 = Src_5;
  Src_10 = Src_2;
  v108 = __dword_443364;
LABEL_271:
  if ( !v239 )
  {
    v242 = __sub_408510_n2_3 - 1;
    ++buf_4[v242 + 53423];
    ++*(uint32_t *)*(&::__n633 + v242);
    kk_12 = kk_1;
    *(uint16_t *)kk_1 = v242;
    kk_1 = kk_12 + 2;
  }
  if ( v108 )
  {
    if ( (uint32_t)Src_9 <= Src_16 )
    {
      n11_5 = *(Src_9 - 1);
      n11 = n11_5;
      if ( Src_9 <= Src_15 )
      {
        v249 = n11_5 & 1 | v311[n11_5] | (2 * (n11_5 & 1));
      }
      else
      {
        v251 = Src_9[v309];
        n633_5 = Src_9[v309 - 1];
        v253 = Src_9[v309 + 1];
        n633_1 = n633_5;
        v249 = v251 & 1 | *(uint32_t *)((char *)v311 + ((v253 + v251 + n11 + n633_5) & 0xFFFFFFFC)) | (2 * (n11 & 1));
        if ( n2_1 == 1 )
        {
          v272 = v249 < v311[n11] || v249 < v311[v253] || v249 < v311[n633_1];
          v249 |= (v272 << 7) | (8 * (n633_1 & 1)) | (4 * (v253 & 1));
        }
      }
    }
    else
    {
      v244 = (int32_t *)*(Src_9 - 1);
      n633_6 = Src_9[v309 - 1];
      Src_2 = Src_10;
      v314.m128i_i32[0] = Src_9[v309];
      v306 = v244;
      n11_6 = Src_9[v309 + 1];
      n633_1 = n633_6;
      v247 = *(Src_9 - 2);
      n11 = n11_6;
      v314.m128i_i32[1] = Src_9[v310] + Src_9[v309 + 2] + Src_9[v310 + 1] + v247;
      v248 = *(Src_9 - 3);
      v314.m128i_i32[2] = (int32_t)v306 + v314.m128i_i32[0];
      v249 = v314.m128i_i8[0] & 1
           | v311[(int32_t)(Src_9[v310 + 3]
                      + Src_9[v310 + 2]
                      + Src_9[v310 - 2]
                      + Src_9[v309 - 3]
                      + Src_9[v309 + 4]
                      + Src_9[v310 - 1]
                      + Src_9[v309 + 3]
                      + Src_9[v309 - 2]
                      + 3 * (n11_6 + n633_1)
                      + *(Src_9 - 4)
                      + v248
                      + 4 * ((uint32_t)v306 + v314.m128i_i32[0])
                      + 2 * v314.m128i_i32[1]) >> 5]
           | (2 * ((uint8_t)v306 & 1));
      if ( n2_1 == 1 )
      {
        v271 = v249 < v311[(uint32_t)v306] || v249 < v311[n11] || v249 < v311[n633_1];
        v249 |= (v271 << 7) | (8 * (n633_1 & 1)) | (4 * (n11 & 1));
      }
    }
  }
  else
  {
    v249 = *(Src_9 - 1);
  }
  v254 = 776 * v249;
  v255 = Block;
  v256 = *(uint32_t *)((char *)Block + v254);
  kk_13 = (uint16_t *)kk_1;
  ++*((uint32_t *)Block + v256 + 49664);
  ++v255[257 * v256 + 49728];
  *kk_13 = v256 + 12;
  kk_1 = (int32_t)(kk_13 + 1);
  __fwd_sub_408510_sub_40B330(v255, buf_4);
  if ( __dword_443388 )
  {
    v268 = 0;
    while ( 1 )
    {
      v269 = __sub_411700((int32_t)*(&::__n633 + v268), __dword_438B00[v268]);
      v270 = v269 + ::__n256_2[0];
      if ( ++v268 >= 12 )
        break;
      ::__n256_2[0] += v269;
    }
    n0x18 = 0;
    if ( ::__n2 == 2 )
    {
      do
      {
        v279 = *(uint32_t *)(__dword_442E6C + 4 * n0x18) * __dword_439300[n0x18];
        ++n0x18;
        v270 += v279;
      }
      while ( n0x18 < 0x18 );
      ::__n256_2[0] = v270;
    }
    else
    {
      do
        v270 += 11 * *(uint32_t *)(__dword_442E68 + 4 * n0x18++ + 96);
      while ( n0x18 < 8 );
      ::__n256_2[0] = v270;
    }
  }
  else
  {
    __sub_40BEE0(v258);
    v259 = *Src_10;
    if ( ::__n8 < 8 )
    {
      *(uint32_t *)__Buffer_1 = ::__n256 | (2 * (v259 << ((31 - ::__n8) & 31)));
      __Buffer_1 = __Buffer_0;
      __sub_408510_n256_2 = v259 >> (::__n8 & 31);
      __Buffer_0 += 4;
      ::__n256 = __sub_408510_n256_2;
      __sub_408510_n8 = ::__n8 + 24;
      ::__n8 += 24;
    }
    else
    {
      __sub_408510_n8 = ::__n8 - 8;
      __sub_408510_n256_2 = ::__n256 | (v259 << (-(char)::__n8 & 31));
      ::__n256 = __sub_408510_n256_2;
      ::__n8 -= 8;
    }
    kk_1 = (kk_1 - (int32_t)kk_2) >> 1;
    if ( kk_1 > 0 )
    {
      for ( kk = 0; kk < kk_1; ++kk )
      {
        v263 = *((uint16_t *)kk_2 + kk);
        n11 = buf_4[*((uint16_t *)kk_2 + kk) & 0x7F];
        n24_2 = v263 >> 7;
        v265 = *((uint32_t *)*(&__dword_442DE0 + n11) + n24_2);
        v266 = (uint16_t)v265;
        n633_1 = BYTE2(v265);
        if ( __sub_408510_n8 < BYTE2(v265) )
        {
          *(uint32_t *)__Buffer_1 = (2 * ((uint16_t)v265 << ((31 - __sub_408510_n8) & 31))) | __sub_408510_n256_2;
          __Buffer_1 = __Buffer_0;
          __Buffer_0 += 4;
          __sub_408510_n256_2 = (uint16_t)v265 >> (::__n8 & 31);
          __sub_408510_n8 = ::__n8 - n633_1 + 32;
        }
        else
        {
          n8_2 = __sub_408510_n8;
          __sub_408510_n8 -= n633_1;
          __sub_408510_n256_2 |= v266 << (-n8_2 & 31);
        }
        if ( n11 == 10 )
        {
          if ( n24_2 >= 24 )
          {
            ++kk;
            if ( __sub_408510_n8 < 11 )
            {
              v277 = *((uint16_t *)kk_2 + kk);
              ::__n8 = __sub_408510_n8;
              *(uint32_t *)__Buffer_1 = (2 * (v277 << ((31 - __sub_408510_n8) & 31))) | __sub_408510_n256_2;
              __Buffer_1 = __Buffer_0;
              __Buffer_0 += 4;
              __sub_408510_n256_2 = v277 >> (::__n8 & 31);
              __sub_408510_n8 = ::__n8 + 21;
              ::__n8 += 21;
            }
            else
            {
              v276 = *((uint16_t *)kk_2 + kk) << (-(char)__sub_408510_n8 & 31);
              __sub_408510_n8 -= 11;
              ::__n8 = __sub_408510_n8;
              __sub_408510_n256_2 |= v276;
            }
            ::__n256 = __sub_408510_n256_2;
            continue;
          }
        }
        else if ( n11 == 11 )
        {
          n8_1 = __dword_439300[n24_2];
          if ( n8_1 )
          {
            ++kk;
            v274 = *((uint16_t *)kk_2 + kk);
            if ( __sub_408510_n8 < n8_1 )
            {
              ::__n8 = __sub_408510_n8;
              *(uint32_t *)__Buffer_1 = (2 * (v274 << ((31 - __sub_408510_n8) & 31))) | __sub_408510_n256_2;
              __Buffer_1 = __Buffer_0;
              __Buffer_0 += 4;
              __sub_408510_n256_2 = v274 >> (::__n8 & 31);
              ::__n256 = v274 >> (::__n8 & 31);
              __sub_408510_n8 = ::__n8 - n8_1 + 32;
              ::__n8 = __sub_408510_n8;
            }
            else
            {
              v275 = v274 << (-(char)__sub_408510_n8 & 31);
              __sub_408510_n8 -= n8_1;
              ::__n8 = __sub_408510_n8;
              __sub_408510_n256_2 |= v275;
              ::__n256 = __sub_408510_n256_2;
            }
            continue;
          }
        }
        ::__n8 = __sub_408510_n8;
        ::__n256 = __sub_408510_n256_2;
      }
    }
  }
  free(Block);
  free(kk_2);
  free(__dword_443408);
  free(__dword_442DE0);
  free(::__n633);
  free(::__buf_2);
}

BMF_SSE char * __sub_40F450(uint32_t *Blockb, char a2, int32_t a3, int32_t a4, int32_t a5)
{
  ;
  __m128i si128, v8, v13, v14;
  char v12, v19, *__sub_40F450_buf, v28, v85, v89, v92, *buf_3, *buf_1, *buf_2;
  int32_t v11, v18, v20, v21, v22, v23, v25, v27, v29, v31, v32, v33, v34, v36, v37, v39, v40,
          v41, v42, v44, v45, v46, v47, v48, v49, v50, v51, v53, v54, v55, v56, v57, v58, v59,
          v60, v61, v62, v63, v64, v65, v66, v67, v68, v69, v70, v71, v73, v74, v75, v77, v78,
          v79, v81, v82, v83, v84, n6, v87, v88, v90, v91, v93, v94, v97, v99, v101, v102, v107,
          v108, v111, v114, v115, v116, v118, v119, v120, v121;
  uint32_t n7, i, n7_1, j, *v30, *v43, *v52, *v76, *v103, *v104, *v105, *v106, *v112, *Blockb_1;
  uint8_t *v16, *v17, *v26, *v35, *v38, *v72, *v80, *v95, *v109, *v113;
  alignas(16) uint8_t v98[256];
  n7 = 0;
  v109 = (uint8_t *)(a3 + 1);
  v98[0] = 0;
  v98[128] = -1;
  do
  {
    v98[2 * n7 + 1] = 4 * n7 + 2;
    v98[2 * n7 + 2] = 4 * n7 + 4;
    ++n7;
  }
  while ( n7 < 7 );
  si128 = _mm_load_si128((const __m128i *)&__xmmword_439590);
  v8 = _mm_load_si128((const __m128i *)&__xmmword_4395A0);
  v98[15] = 30;
  for ( i = 15; i < 0x7F; i += 16 )
  {
    *(__m128i *)&v98[i + 1] = v8;
    v8 = _mm_add_epi8(v8, si128);
  }
  n7_1 = 0;
  v11 = 0;
  do
  {
    v98[2 * n7_1 + 129] = 2 * v11 - 3;
    v12 = 2 * v11 - 5;
    v11 -= 2;
    v98[2 * n7_1++ + 130] = v12;
  }
  while ( n7_1 < 7 );
  v13 = _mm_load_si128((const __m128i *)&__xmmword_4395B0);
  v14 = _mm_load_si128((const __m128i *)&__xmmword_4395C0);
  v98[143] = -31;
  for ( j = 15; j < 0x7F; j += 16 )
  {
    *(__m128i *)&v98[j + 129] = v14;
    v14 = _mm_add_epi8(v14, v13);
  }
  v16 = (uint8_t *)Blockb[17];
  if ( a4 > 1 )
  {
    v17 = v109;
    v18 = 1;
    do
    {
      v19 = *v17;
      *v16 = *v17;
      *v17 = v98[(uint8_t)(v19 - *(uint8_t *)(Blockb[17] - 1))];
      v20 = Blockb[*(uint8_t *)Blockb[17] - *(uint8_t *)(Blockb[17] - 1) + 2091];
      *(uint32_t *)(Blockb[23] + 4) = v20;
      *(uint32_t *)Blockb[23] = v20;
      v21 = Blockb[17];
      Blockb[23] += 8;
      ++v18;
      ++v17;
      v16 = (uint8_t *)(v21 + 1);
      Blockb[17] = v16;
    }
    while ( v18 < a4 );
    v109 = v17;
  }
  v22 = (uint8_t)*(v16 - 1);
  *v16 = v22;
  v23 = Blockb[23];
  Blockb[16] = v22;
  Blockb[21] = *(uint32_t *)(v23 - 8);
  Blockb[22] = *(uint32_t *)(v23 - 4);
  __sub_40F450_buf = memcpy((char *)Blockb[20],(char *)Blockb[19],a4 + 2);
  v25 = a5 - 1;
  if ( a5 != 1 )
  {
    buf_1 = (char *)(a4 - 1);
    v103 = Blockb + 807;
    v104 = Blockb + 287;
    v105 = Blockb + 27;
    v26 = v109;
    v106 = Blockb + 547;
    do
    {
      while ( 1 )
      {
        v27 = (uint8_t)*v26;
        v28 = *((uint8_t *)Blockb + 64);
        *(uint8_t *)(Blockb[17] - 1) = v28;
        *(uint8_t *)Blockb[17] = v28;
        v29 = Blockb[23];
        *(uint32_t *)(v29 - 8) = Blockb[21];
        *(uint32_t *)(v29 - 4) = Blockb[22];
        v30 = (uint32_t *)Blockb[23];
        *v30 = *(v30 - 2);
        v30[1] = *(v30 - 1);
        v31 = Blockb[19];
        v32 = Blockb[20];
        Blockb[19] = v32;
        Blockb[20] = v31;
        v32 += 2;
        Blockb[17] = v32;
        Blockb[18] = v31 + 2;
        Blockb[16] = v27;
        *(uint8_t *)(v32 - 2) = v27;
        Blockb[23] = Blockb[24] + 8;
        v33 = Blockb[18];
        Blockb[21] = -Blockb[v27 - *(uint8_t *)(v33 - 1) + 2091];
        Blockb[22] = 0;
        *v26 = v98[(uint8_t)(*v26 - *(uint8_t *)(v33 - 1))];
        __sub_40F450_buf = buf_1;
        ++v26;
        if ( a4 != 1 )
          break;
        if ( !--v25 )
          return __sub_40F450_buf;
      }
      buf_2 = buf_1;
      v99 = v25;
      v109 = v26;
      do
      {
        v34 = Blockb[16];
        v35 = (uint8_t *)Blockb[18];
        v36 = *v35;
        v37 = *(v35 - 1);
        Blockb_1 = Blockb;
        v38 = (uint8_t *)Blockb[17];
        v39 = *v38;
        v114 = v34;
        v40 = v37 + v39;
        v41 = v35[1];
        v118 = v36;
        v113 = v38;
        v42 = v34 + v36;
        v119 = (16 * (v42 + v41) + -3 * (v40 - *(v38 - 2)) + 8 * v42 + 4 * v34 - v118 + 31) >> 6;
        v43 = (uint32_t *)Blockb_1[23];
        v44 = Blockb_1[21];
        v45 = v43[2] + *v43;
        v112 = v43;
        v111 = v44;
        v46 = v45 + *(v43 - 2) + v44;
        Blockb = Blockb_1;
        v47 = v119;
        if ( v46 <= 0 )
        {
          v107 = -v46;
          Blockb_1[25] = v104;
          v49 = v107;
          v48 = v114 + ((*v35 - *(v35 - 1) + 2) >> 2);
        }
        else
        {
          v107 = v46;
          Blockb_1[25] = v105;
          v48 = *v35 + ((v42 - v40 + 4) >> 3);
          v49 = v107;
        }
        v120 = v42;
        Blockb[3] = v47;
        Blockb[7] = v48;
        v50 = (v47 + v48) >> 1;
        Blockb[5] = v50;
        Blockb[4] = (v50 + v47) >> 1;
        v51 = (v50 + v48) >> 1;
        v52 = v112;
        v53 = v120;
        Blockb[6] = v51;
        v54 = *(uint8_t *)(Blockb[25] + v49 + 16);
        *Blockb = v54;
        v55 = Blockb[v54 + 2];
        v56 = Blockb[22] + v52[3] + v52[1];
        if ( v56 < 0 )
        {
          v108 = -v56;
          Blockb[26] = v103;
          v65 = *(v35 - 1);
          v66 = *(v35 - 2);
          v115 = 2 * v55;
          v67 = 2 * v65 - *(v113 - 1) + v53 - 2 * v55 - v66 + 2;
          v62 = v108;
          v63 = v67 >> 2;
        }
        else
        {
          Blockb_1 = Blockb;
          v108 = v56;
          Blockb[26] = v106;
          v57 = *(v35 - 1);
          v58 = v35[1];
          v59 = *v35;
          v115 = 2 * v55;
          v60 = 2 * v58 - 2 * v55 - v113[1] - (v57 - v58);
          Blockb = Blockb_1;
          v61 = v60 + v59 + 4;
          v62 = v108;
          v63 = v61 >> 3;
        }
        v64 = v55 + v63;
        v68 = v115;
        Blockb[10] = v55;
        Blockb[14] = v64;
        v69 = v55 + v64 + 1;
        Blockb[11] = (v69 + v68) >> 2;
        Blockb[12] = v69 >> 1;
        v70 = v69 + 2 * v64;
        v71 = Blockb[26];
        Blockb[13] = v70 >> 2;
        v72 = v109;
        v73 = *(uint8_t *)(v71 + v62 + 16);
        v74 = v111;
        Blockb[1] = v73;
        v75 = Blockb[v73 + 9];
        v76 = v112;
        v77 = *v72;
        *(v112 - 2) = v74;
        v78 = Blockb[22];
        v121 = v75;
        *(v76 - 1) = v78;
        v79 = Blockb[16];
        v80 = (uint8_t *)Blockb[18];
        Blockb[21] = Blockb[v77 - v79 + 2091] - Blockb[v77 - *v80 + 2091];
        v81 = Blockb[v77 - *(v80 - 1) + 2091] - Blockb[v77 - v80[1] + 2091];
        v82 = Blockb[17];
        Blockb[22] = v81;
        *(uint8_t *)(v82 - 1) = v79;
        v83 = Blockb[1];
        Blockb[16] = v77;
        ++Blockb[17];
        v116 = v83;
        ++Blockb[18];
        Blockb[23] += 8;
        v84 = Blockb[v83 + 9];
        v85 = v121;
        n6 = Blockb[v77 - v84 + 2091];
        if ( n6 >= 6 )
        {
          v87 = *Blockb;
          v101 = Blockb[v77 - Blockb[10] + 2091] - 1;
          v102 = v87;
          if ( v101 <= Blockb[v77 - Blockb[v87 + 1] + 2091] )
          {
            if ( v101 <= Blockb[v77 - Blockb[v102 + 3] + 2091] )
            {
              v91 = n6 - 1;
              if ( v91 <= Blockb[v77 - Blockb[v116 + 8] + 2091] )
              {
                if ( v91 > Blockb[v77 - Blockb[v116 + 10] + 2091] )
                {
                  v92 = v116;
                  v93 = Blockb[26];
                  v94 = *(uint32_t *)(v93 + 4 * v116 - 4) - 1;
                  *(uint32_t *)(v93 + 4 * v116 - 4) = v94;
                  *(uint8_t *)(Blockb[26] + v94 + 16) = v92 + 1;
                }
              }
              else
              {
                *(uint8_t *)(*(uint32_t *)(Blockb[26] + 4 * v116 - 8) + Blockb[26] + 16) = v116 - 1;
                ++*(uint32_t *)(Blockb[26] + 4 * Blockb[1] - 8);
              }
            }
            else
            {
              v88 = Blockb[25];
              v89 = v102 + 1;
              v90 = *(uint32_t *)(v88 + 4 * v102 - 4) - 1;
              *(uint32_t *)(v88 + 4 * v102 - 4) = v90;
              *(uint8_t *)(Blockb[25] + v90 + 16) = v89;
            }
          }
          else
          {
            *(uint8_t *)(*(uint32_t *)(Blockb[25] + 4 * v102 - 8) + Blockb[25] + 16) = v102 - 1;
            ++*(uint32_t *)(Blockb[25] + 4 * *Blockb - 8);
          }
        }
        v95 = v109;
        buf_3 = buf_2;
        v97 = (uint8_t)v98[(uint8_t)(*v109 - v85)];
        *v109 = v97;
        __sub_40F450_buf = (char *)::__buf;
        v26 = v95 + 1;
        v109 = v26;
        ++*(uint32_t *)&::__buf[4 * v97];
        buf_2 = buf_3 - 1;
      }
      while ( buf_3 != (char *)1 );
      v25 = v99 - 1;
    }
    while ( v99 != 1 );
  }
  return __sub_40F450_buf;
}
BMF_SSE int32_t __sub_41C4B0(__m128 *_this, __m128 *a2, __m128 **a3, int32_t n2)
{
  ;
  __m128 v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v23,
         v24, v25, v26, v27, v29, v30, v31, v33, v34, v35, v36, v37, v38, v39, v40, v41;
  float v22, v32, v42;
  v4 = *a2;
  v5 = _mm_add_ps(
         _mm_add_ps(
           _mm_add_ps(
             _mm_add_ps(
               _mm_add_ps(
                 _mm_add_ps(_mm_mul_ps((__m128)__xmmword_441120, *a2), _mm_mul_ps((__m128)__xmmword_441130, a2[1])),
                 _mm_mul_ps((__m128)__xmmword_441140, a2[2])),
               _mm_mul_ps((__m128)__xmmword_441150, a2[3])),
             _mm_mul_ps((__m128)__xmmword_441160, a2[4])),
           _mm_mul_ps((__m128)__xmmword_441170, a2[5])),
         _mm_mul_ps((__m128)__xmmword_441180, a2[6]));
  v6 = _mm_add_ps(v5, _mm_movehl_ps(v5, v5));
  v6.m128_f32[0] = v6.m128_f32[0] + M128F(_mm_shuffle_ps(v6, v6, 1)).m128_f32[0];
  a2[7].m128_i32[0] = v6.m128_i32[0];
  v42 = v6.m128_f32[0];
  v7 = _mm_unpacklo_ps(v6, v6);
  v8 = _mm_movelh_ps(v7, v7);
  v9 = _mm_sub_ps(v4, v8);
  *a2 = v9;
  v40 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_mul_ps((__m128)__xmmword_445760[6 * n2], **a3),
                  _mm_mul_ps((__m128)__xmmword_445770[6 * n2], *a3[1])),
                _mm_mul_ps((__m128)__xmmword_445780[6 * n2], *a3[2])),
              _mm_mul_ps((__m128)__xmmword_445790[6 * n2], *a3[3])),
            _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], *a3[4])),
          _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], *a3[5]));
  v34 = _mm_sub_ps(a2[1], v8);
  a2[1] = v34;
  v10 = _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], a3[5][1]);
  v11 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_mul_ps((__m128)__xmmword_445760[6 * n2], (*a3)[1]),
                _mm_mul_ps((__m128)__xmmword_445770[6 * n2], a3[1][1])),
              _mm_mul_ps((__m128)__xmmword_445780[6 * n2], a3[2][1])),
            _mm_mul_ps((__m128)__xmmword_445790[6 * n2], a3[3][1])),
          _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], a3[4][1]));
  v33 = _mm_sub_ps(a2[2], v8);
  a2[2] = v33;
  v38 = _mm_add_ps(v11, v10);
  v12 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_mul_ps((__m128)__xmmword_445760[6 * n2], (*a3)[2]),
                  _mm_mul_ps((__m128)__xmmword_445770[6 * n2], a3[1][2])),
                _mm_mul_ps((__m128)__xmmword_445780[6 * n2], a3[2][2])),
              _mm_mul_ps((__m128)__xmmword_445790[6 * n2], a3[3][2])),
            _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], a3[4][2])),
          _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], a3[5][2]));
  v13 = _mm_sub_ps(a2[3], v8);
  a2[3] = v13;
  v35 = v13;
  v14 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_mul_ps((__m128)__xmmword_445760[6 * n2], (*a3)[3]),
                  _mm_mul_ps((__m128)__xmmword_445770[6 * n2], a3[1][3])),
                _mm_mul_ps((__m128)__xmmword_445780[6 * n2], a3[2][3])),
              _mm_mul_ps((__m128)__xmmword_445790[6 * n2], a3[3][3])),
            _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], a3[4][3])),
          _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], a3[5][3]));
  v15 = _mm_sub_ps(a2[4], v8);
  a2[4] = v15;
  v36 = v15;
  v41 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_mul_ps((__m128)__xmmword_445760[6 * n2], (*a3)[4]),
                  _mm_mul_ps((__m128)__xmmword_445770[6 * n2], a3[1][4])),
                _mm_mul_ps((__m128)__xmmword_445780[6 * n2], a3[2][4])),
              _mm_mul_ps((__m128)__xmmword_445790[6 * n2], a3[3][4])),
            _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], a3[4][4])),
          _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], a3[5][4]));
  v16 = _mm_mul_ps(v41, v15);
  v37 = _mm_sub_ps(a2[5], v8);
  a2[5] = v37;
  v17 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_mul_ps((__m128)__xmmword_445760[6 * n2], (*a3)[5]),
                  _mm_mul_ps((__m128)__xmmword_445770[6 * n2], a3[1][5])),
                _mm_mul_ps((__m128)__xmmword_445780[6 * n2], a3[2][5])),
              _mm_mul_ps((__m128)__xmmword_445790[6 * n2], a3[3][5])),
            _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], a3[4][5])),
          _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], a3[5][5]));
  v18 = _mm_sub_ps(a2[6], v8);
  a2[6] = v18;
  v39 = v18;
  v19 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_mul_ps((__m128)__xmmword_445760[6 * n2], (*a3)[6]),
                  _mm_mul_ps((__m128)__xmmword_445770[6 * n2], a3[1][6])),
                _mm_mul_ps((__m128)__xmmword_445780[6 * n2], a3[2][6])),
              _mm_mul_ps((__m128)__xmmword_445790[6 * n2], a3[3][6])),
            _mm_mul_ps((__m128)__xmmword_4457A0[6 * n2], a3[4][6])),
          _mm_mul_ps((__m128)__xmmword_4457B0[6 * n2], a3[5][6]));
  v20 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(_mm_add_ps(_mm_mul_ps(v40, v9), _mm_mul_ps(v38, v34)), _mm_mul_ps(v12, v33)),
                _mm_mul_ps(v14, v35)),
              v16),
            _mm_mul_ps(v17, v37)),
          _mm_mul_ps(v19, v39));
  v21 = _mm_add_ps(v20, _mm_movehl_ps(v20, v20));
  v22 = (float)(v21.m128_f32[0] + M128F(_mm_shuffle_ps(v21, v21, 1)).m128_f32[0]) + v42;
  a2[7].m128_f32[2] = v22;
  if ( *((uint32_t *)_this + 60) )
  {
    v30 = _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_add_ps(
                    _mm_add_ps(_mm_mul_ps(*_this, v9), _mm_mul_ps(*(_this + 1), v34)),
                    _mm_mul_ps(*(_this + 2), v33)),
                  _mm_mul_ps(*(_this + 3), v35)),
                _mm_mul_ps(*(_this + 4), v36)),
              _mm_mul_ps(*(_this + 5), v37)),
            _mm_mul_ps(*(_this + 6), v39));
    v31 = _mm_add_ps(v30, _mm_movehl_ps(v30, v30));
    v32 = v42 + (float)(v31.m128_f32[0] + M128F(_mm_shuffle_ps(v31, v31, 1)).m128_f32[0]);
    a2[7].m128_f32[1] = v32;
    return (int32_t)(float)(v22 + (float)((float)((float)(v32 - v22) * *((float *)_this + 56)) / *((float *)_this + 57)));
  }
  else
  {
    *_this = _mm_mul_ps(v40, (__m128)__xmmword_439B20);
    v23 = _mm_mul_ps((*a3)[7], (__m128)__xmmword_439B30);
    *(_this + 1) = _mm_mul_ps(v38, (__m128)__xmmword_439B20);
    *(_this + 7) = v23;
    v24 = _mm_mul_ps((*a3)[8], (__m128)__xmmword_439B30);
    *(_this + 2) = _mm_mul_ps(v12, (__m128)__xmmword_439B20);
    *(_this + 8) = v24;
    v25 = _mm_mul_ps((*a3)[9], (__m128)__xmmword_439B30);
    *(_this + 3) = _mm_mul_ps(v14, (__m128)__xmmword_439B20);
    *(_this + 9) = v25;
    v26 = _mm_mul_ps((*a3)[10], (__m128)__xmmword_439B30);
    *(_this + 4) = _mm_mul_ps(v41, (__m128)__xmmword_439B20);
    *(_this + 10) = v26;
    v27 = _mm_mul_ps((*a3)[11], (__m128)__xmmword_439B30);
    *(_this + 5) = _mm_mul_ps(v17, (__m128)__xmmword_439B20);
    *(_this + 11) = v27;
    v29 = _mm_mul_ps((*a3)[12], (__m128)__xmmword_439B30);
    *(_this + 6) = _mm_mul_ps(v19, (__m128)__xmmword_439B20);
    *(_this + 12) = v29;
    *(_this + 13) = _mm_mul_ps((__m128)__xmmword_439B30, (*a3)[13]);
    *((uint32_t *)_this + 56) = 1111228416;
    *((uint32_t *)_this + 57) = 1126773555;
    *((uint32_t *)_this + 58) = 1065353216;
    a2[7].m128_f32[1] = v22;
    return (int32_t)v22;
  }
}
static inline int32_t __fwd_sub_41A130_sub_41C4B0(void *a0, void *a1, void *a2, int32_t a3) { return __sub_41C4B0((__m128 *)a0, (__m128 *)a1, (__m128 **)a2, a3); }

BMF_SSE int32_t __sub_41A130(__m128 *a1, const __m128 &a2__ref, const __m128 &a3__ref, uint32_t *a4, uint32_t *a5)
{
  alignas(16) uint8_t __hexrays_frame[208];
  int32_t &v246 = *(int32_t *)(__hexrays_frame + 0);
  int16_t * &v247 = *(int16_t * *)(__hexrays_frame + 0);
  int32_t &v248 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v249 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v250 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v251 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &n15 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v253 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v254 = *(int32_t *)(__hexrays_frame + 0);
  int16_t * &v255 = *(int16_t * *)(__hexrays_frame + 0);
  int32_t &v256 = *(int32_t *)(__hexrays_frame + 4);
  char * &v257 = *(char * *)(__hexrays_frame + 4);
  int32_t &v258 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v259 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v260 = *(int32_t *)(__hexrays_frame + 4);
  uint32_t &v261 = *(uint32_t *)(__hexrays_frame + 4);
  int32_t &v262 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v263 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v264 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v265 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v266 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v267 = *(int32_t *)(__hexrays_frame + 4);
  int16_t * &v268 = *(int16_t * *)(__hexrays_frame + 8);
  int32_t &v269 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v270 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v271 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v272 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v273 = *(int32_t *)(__hexrays_frame + 8);
  int16_t * &v274 = *(int16_t * *)(__hexrays_frame + 8);
  __m128 * &v275 = *(__m128 * *)(__hexrays_frame + 12);
  int16_t * &v276 = *(int16_t * *)(__hexrays_frame + 16);
  int32_t &v277 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &v278 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v279 = *(int32_t *)(__hexrays_frame + 28);
  int32_t &v280 = *(int32_t *)(__hexrays_frame + 32);
  int16_t * &v281 = *(int16_t * *)(__hexrays_frame + 36);
  int16_t * &v282 = *(int16_t * *)(__hexrays_frame + 40);
  int16_t * &v283 = *(int16_t * *)(__hexrays_frame + 44);
  int16_t * &v284 = *(int16_t * *)(__hexrays_frame + 48);
  int16_t * &v285 = *(int16_t * *)(__hexrays_frame + 52);
  int16_t * &v286 = *(int16_t * *)(__hexrays_frame + 56);
  int32_t &n1840_2 = *(int32_t *)(__hexrays_frame + 60);
  int32_t &n1840_1 = *(int32_t *)(__hexrays_frame + 64);
  __m128 * &v289 = *(__m128 * *)(__hexrays_frame + 68);
  int32_t &v290 = *(int32_t *)(__hexrays_frame + 72);
  int32_t &n3536 = *(int32_t *)(__hexrays_frame + 76);
  int32_t &v292 = *(int32_t *)(__hexrays_frame + 80);
  __m128 * &v293 = *(__m128 * *)(__hexrays_frame + 84);
  int16_t * &v294 = *(int16_t * *)(__hexrays_frame + 88);
  int16_t * &v295 = *(int16_t * *)(__hexrays_frame + 92);
  int16_t * &v296 = *(int16_t * *)(__hexrays_frame + 96);
  uint32_t &v297 = *(uint32_t *)(__hexrays_frame + 100);
  uint32_t &v298 = *(uint32_t *)(__hexrays_frame + 104);
  int32_t &n960_1 = *(int32_t *)(__hexrays_frame + 108);
  int32_t &n1840 = *(int32_t *)(__hexrays_frame + 112);
  int32_t &v301 = *(int32_t *)(__hexrays_frame + 116);
  int32_t &v302 = *(int32_t *)(__hexrays_frame + 120);
  int32_t &v303 = *(int32_t *)(__hexrays_frame + 124);
  int32_t &v304 = *(int32_t *)(__hexrays_frame + 128);
  int32_t &v305 = *(int32_t *)(__hexrays_frame + 132);
  int32_t &v306 = *(int32_t *)(__hexrays_frame + 136);
  int32_t &v307 = *(int32_t *)(__hexrays_frame + 140);
  int32_t &v308 = *(int32_t *)(__hexrays_frame + 144);
  int32_t &v309 = *(int32_t *)(__hexrays_frame + 148);
  int32_t &v310 = *(int32_t *)(__hexrays_frame + 152);
  int32_t &v311 = *(int32_t *)(__hexrays_frame + 156);
  int32_t &v312 = *(int32_t *)(__hexrays_frame + 160);
  int32_t &v313 = *(int32_t *)(__hexrays_frame + 164);
  int32_t &v314 = *(int32_t *)(__hexrays_frame + 168);
  int32_t &v315 = *(int32_t *)(__hexrays_frame + 172);
  ;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  __m128 *v28, *v31, v32, v33, n0x45F20000, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44,
         *v50, *v53, *v59, v60, v61, v62, v63, v64, *v110, *v118, *v160, *v166, *v173, *v184,
         *v194, *v196, *v204, *v243;
  bool v26, v58;
  char v142;
  float v70, v77, v79, v89, v94, v101, v244;
  int16_t *v7, *v45, *v49, *v52, *v66, *v67, *v69, *v71, *v72, *v73, *v76, *v78, *v80, *v81,
          *v82, *v83, *v84, *v86, *v87, *v90, *v91, *v92, *v93, *v95, *v96, *v97, *v98, *v99,
          *v100, *v102, *v115, *v129, *v143, v150, *v158, *v170, *v205, *v217, *v223, *v230,
          *v245;
  int32_t v6, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24,
          v25, v27, v29, v30, v46, v47, v48, v51, v54, v55, v56, v57, v65, v68, v74, v75, v85,
          v88, *v103, *v104, v105, n2, v107, n3536_5, v109, v111, v112, v113, v114, v116, v117,
          n3536_1, v121, v122, v123, v124, v125, v126, v127, v128, v130, n2256, v132, v134, v137,
          v138, v140, v141, v144, n2576, n1840_13, v147, v148, v149, v151, v154, v155, v156,
          v157, n2896, v161, v162, v163, v164, v167, v168, v169, v171, v172, n3536_2, v175, v176,
          v177, v178, v179, v181, v182, n3536_3, v185, v186, v188, v189, v190, v191, v192, v193,
          n1840_14, n1840_15, v198, n1840_16, v200, v201, n1840_17, v203, v206, v207, v208, n960,
          n3536_4, v211, v212, v213, v214, v215, n1840_3, n1840_8, n1840_7, n1840_10, n1840_9,
          v222, v224, n1840_11, n1840_12, n255, v228, n1840_5, n1840_4, n1840_6, v234, v235,
          v236, v237, v238, v239, v240, v241, v242;
  int8_t v139;
  uint32_t v120, v133, v135, v136, v152, v153, v165, v180;
  uint8_t *v187;
  v6 = a1[17421].m128_i32[0];
  v7 = (int16_t *)a1[17421].m128_i32[1];
  v8 = *(int16_t *)(v6 - 24);
  v9 = 23 * *(int16_t *)(v6 - 6);
  v289 = a1;
  v246 = v6;
  v268 = v7;
  v275 = (__m128 *)a1[17421].m128_i32[2];
  v10 = v275[-2].m128_i16[5];
  n1840_1 = 21 * v275[-1].m128_i16[5]
          + 12 * v7[33]
          + 16 * v7[24]
          + 22 * v7[15]
          + v9
          + 20 * v7[6]
          + __dword_4458E0
          + 14 * v8;
  n1840_2 = 17 * v10
          + 21 * v7[25]
          + 15 * v7[16]
          + 25 * v7[7]
          + 9 * *(v7 - 2)
          + 22 * *(int16_t *)(v6 - 4)
          + __dword_4458E4
          + 19 * *(int16_t *)(v6 - 22);
  v11 = v6;
  v12 = 17 * v7[31]
      + 15 * v7[22]
      + 21 * v7[13]
      + 18 * v7[4]
      + 16 * *(v7 - 5)
      + 22 * *(int16_t *)(v6 - 10)
      + __dword_4458E8
      + 19 * *(int16_t *)(v6 - 28);
  v13 = v275->m128_i16[5];
  v14 = *(int16_t *)(v289[17421].m128_i32[3] + 10);
  v15 = *(int16_t *)(v11 - 26);
  v16 = *(int16_t *)(v11 - 8);
  v292 = v12;
  n3536 = 14 * v268[32] + 23 * v268[14] + 19 * v268[5] + 25 * v16 + __dword_4458EC + 17 * v15 + 15 * (v14 + v13);
  v17 = v275[-2].m128_i16[7];
  v18 = v275[2].m128_i16[2] + v275->m128_i16[0];
  v290 = v12 + n3536 + n1840_1 + n1840_2;
  v19 = v268[9];
  v20 = v18 + v19 + v17;
  v21 = *(int16_t *)(v246 - 18);
  v22 = 2 * *(int16_t *)(v246 - 36) + 2 * v21;
  v23 = *v268 + 2 * v21;
  v276 = (int16_t *)v22;
  v277 = v19 + v23;
  v279 = 16 * v20;
  v24 = 13
      * ((8 * v12 > 43 * n3536)
       + (8 * v12 > 17 * n3536)
       + (8 * v12 > 9 * n3536)
       + (8 * v12 > 5 * n3536)
       + (8 * v12 > 2 * n3536));
  v25 = v290 > __dword_4398E4[v24];
  v26 = v290 <= __dword_4398E8[v24];
  v278 = (8 * v12 > 43 * n3536)
       + (8 * v12 > 17 * n3536)
       + (8 * v12 > 9 * n3536)
       + (8 * v12 > 5 * n3536)
       + (8 * v12 > 2 * n3536);
  v256 = ((v290 > __dword_4398F0[v24]) + (v290 > __dword_4398EC[v24]) + !v26 + v25) << 6;
  v280 = (16 * n1840_2 > n1840_1 * __dword_4398D4[v24])
       + (16 * n1840_2 > n1840_1 * __dword_4398D0[v24])
       + (16 * n1840_2 > n1840_1 * __dword_4398CC[v24]);
  v27 = __dword_4398C0[v24];
  v281 = (int16_t *)(16 * ((v277 > __dword_4398E0[v24]) + (v277 > __dword_4398DC[v24]) + (v277 > __dword_4398D8[v24])));
  v28 = v289;
  v257 = (char *)&v281[160 * v278 + 2 * v280]
       + (v279 > __dword_4398C8[v24] * (int32_t)v276)
       + (v279 > (int32_t)v276 * __dword_4398C4[v24])
       + (v279 > (int32_t)v276 * v27)
       + v256;
  v29 = v289[17554].m128_i16[(uint32_t)v257 + 4];
  if ( v289[17554].m128_i16[(uint32_t)v257 + 4] )
  {
    v293 = v289 + 17408;
    v31 = &v289[16 * v29];
    v289[17416].m128_i32[0] = (int32_t)v31;
    v32 = _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_add_ps(
                    _mm_add_ps(_mm_mul_ps(*v31, v28[17408]), _mm_mul_ps(v31[1], v28[17409])),
                    _mm_mul_ps(v31[2], v28[17410])),
                  _mm_mul_ps(v31[3], v28[17411])),
                _mm_mul_ps(v31[4], v28[17412])),
              _mm_mul_ps(v31[5], v28[17413])),
            _mm_mul_ps(v31[6], v28[17414]));
    v33 = _mm_add_ps(v32, _mm_movehl_ps(v32, v32));
    a3.m128_f32[0] = (float)((float)*(int16_t *)(v246 - 18)
                           - (float)((float)(v33.m128_f32[0] + COERCE_FLOAT(_mm_shuffle_ps(v33, v33, 1)))
                                   + v28[17415].m128_f32[0]))
                   * 2.0999999f;
    n0x45F20000 = (__m128)0x45F20000u;
    n0x45F20000.m128_f32[0] = 7744.0f * v31[14].m128_f32[2];
    v35 = _mm_unpacklo_ps(a3, a3);
    v36 = _mm_movelh_ps(v35, v35);
    v37 = _mm_unpacklo_ps(n0x45F20000, n0x45F20000);
    a2 = _mm_movelh_ps(v37, v37);
    v38 = _mm_add_ps(v31[7], _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17408], v28[17408]), v31[7]), (__m128)__xmmword_439B10));
    v31[7] = v38;
    *v31 = _mm_add_ps(
             _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_441190, v36), v28[17408]), _mm_add_ps(v38, a2)),
             *v31);
    v39 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17409], v28[17409]), v31[8]), (__m128)__xmmword_439B10), v31[8]);
    v31[8] = v39;
    v31[1] = _mm_add_ps(
               _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411A0, v36), v28[17409]), _mm_add_ps(v39, a2)),
               v31[1]);
    v40 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17410], v28[17410]), v31[9]), (__m128)__xmmword_439B10), v31[9]);
    v31[9] = v40;
    v31[2] = _mm_add_ps(
               _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411B0, v36), v28[17410]), _mm_add_ps(v40, a2)),
               v31[2]);
    v41 = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17411], v28[17411]), v31[10]), (__m128)__xmmword_439B10),
            v31[10]);
    v31[10] = v41;
    v31[3] = _mm_add_ps(
               _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411C0, v36), v28[17411]), _mm_add_ps(v41, a2)),
               v31[3]);
    v42 = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17412], v28[17412]), v31[11]), (__m128)__xmmword_439B10),
            v31[11]);
    v31[11] = v42;
    v31[4] = _mm_add_ps(
               _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411D0, v36), v28[17412]), _mm_add_ps(v42, a2)),
               v31[4]);
    v43 = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17413], v28[17413]), v31[12]), (__m128)__xmmword_439B10),
            v31[12]);
    v31[12] = v43;
    v31[5] = _mm_add_ps(
               _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411E0, v36), v28[17413]), _mm_add_ps(v43, a2)),
               v31[5]);
    v44 = _mm_add_ps(
            _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v28[17414], v28[17414]), v31[13]), (__m128)__xmmword_439B10),
            v31[13]);
    v31[13] = v44;
    v31[6] = _mm_add_ps(
               _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411F0, v36), v28[17414]), _mm_add_ps(v44, a2)),
               v31[6]);
  }
  else
  {
    v30 = v289[17419].m128_i32[3];
    v293 = v289 + 17408;
    v289[17419].m128_i32[3] = ++v30;
    v28[17554].m128_i16[(uint32_t)v257 + 4] = v30;
    v28[17416].m128_i32[0] = (int32_t)&v28[16 * (int16_t)v30];
  }
  v45 = (int16_t *)v28[17421].m128_i32[1];
  v28[17408].m128_f32[0] = (float)v45[1];
  v46 = v28[17421].m128_i32[0];
  v28[17408].m128_f32[1] = (float)v45[10];
  v28[17408].m128_f32[2] = (float)(*(int16_t *)(v46 - 16) + v45[1] - *(v45 - 8));
  v47 = *(int16_t *)(v46 - 34);
  v48 = v45[1] - *(v45 - 17);
  v276 = (int16_t *)v28[17421].m128_i32[2];
  v49 = v276;
  v28[17408].m128_f32[3] = (float)(v47 + v48);
  v50 = (__m128 *)v28[17421].m128_i32[3];
  v28[17409].m128_f32[0] = (float)(*(v45 - 8) + v45[1] - *(v49 - 8));
  v51 = v50->m128_i16[1];
  v275 = v50;
  v28[17409].m128_f32[1] = (float)(-3 * (v49[1] - v45[1]) + v51);
  v28[17409].m128_f32[2] = (float)(*(int16_t *)(v46 - 16) + v45[19] - v45[10]);
  v28[17409].m128_f32[3] = (float)(*(int16_t *)(v46 - 34) + v45[10] - *(v45 - 8));
  v28[17410].m128_f32[0] = (float)(2 * *(int16_t *)(v46 - 16) - *(int16_t *)(v46 - 34));
  v52 = v276;
  v28[17410].m128_f32[1] = (float)(*(int16_t *)(v46 - 52) + v45[1] - *(v45 - 26));
  v28[17410].m128_f32[2] = (float)(v52[1] + v45[10] - v275[1].m128_i16[2]);
  v28[17410].m128_f32[3] = (float)*(int16_t *)(v46 - 52);
  v28[17411].m128_f32[0] = (float)(*(v45 - 17) + v45[1] - *(v52 - 17));
  v28[17411].m128_f32[1] = (float)(*(int16_t *)(v46 - 34) + *(v45 - 8) - *(v45 - 26));
  v53 = v275;
  v28[17411].m128_f32[2] = (float)(v45[10] + ((v45[19] + v45[1]) >> 1) - v52[19]);
  v28[17411].m128_f32[3] = (float)v53->m128_i16[1];
  if ( a4 )
  {
    v282 = (int16_t *)(a4[69684] - 18);
    v54 = a4[69686] - 18;
    v281 = (int16_t *)(a4[69685] - 18);
    v55 = a5[69684];
    v286 = (int16_t *)v54;
    v56 = a5[69685];
    v57 = a5[69686];
    v284 = (int16_t *)(v55 - 18);
    v58 = v28[17420].m128_i32[3] == 0;
    v283 = (int16_t *)(v56 - 18);
    v285 = (int16_t *)(v57 - 18);
    if ( !v58 )
    {
      v59 = v293;
      v60 = v293[1];
      v61 = v293[2];
      v62 = v293[3];
      a2.m128_f32[0] = (float)*(int16_t *)(v46 + 2);
      v63 = _mm_unpacklo_ps(a2, a2);
      v64 = _mm_movelh_ps(v63, v63);
      *v293 = _mm_add_ps(*v293, v64);
      v59[1] = _mm_add_ps(v60, v64);
      v59[2] = _mm_add_ps(v61, v64);
      v59[3] = _mm_add_ps(v62, v64);
      v46 = v28[17421].m128_i32[0];
      v45 = (int16_t *)v28[17421].m128_i32[1];
    }
    v65 = v28[17420].m128_i32[2];
    if ( v65 )
    {
      if ( v65 == 1 )
      {
        v81 = v281;
        v28[17412].m128_f32[0] = (float)(*(int16_t *)(v46 + 2) + v45[28]);
        v82 = v282;
        v28[17412].m128_f32[1] = (float)(*(int16_t *)(v46 - 36) + v81[18] - *v81);
        v28[17412].m128_f32[2] = (float)(v45[9] + *(v82 - 9) - *v81);
        v83 = v283;
        v84 = v285;
        v28[17412].m128_f32[3] = (float)(*(int16_t *)(v46 - 18) + *v81 - *(v81 - 9));
        v85 = v83[18] - v84[27];
        v86 = v284;
        v28[17413].m128_f32[0] = (float)(v45[9] + v85);
        v87 = v282;
        v28[17413].m128_f32[1] = (float)(*(int16_t *)(v46 - 36) + *v86 - *(v86 - 18));
        v28[17413].m128_f32[2] = (float)(*v45 + *v86 - *v83);
        v58 = v28[17420].m128_i32[3] == 0;
        v28[17413].m128_f32[3] = (float)(*(int16_t *)(v46 - 36) + v87[2]);
        if ( v58 )
        {
          v259 = v46;
          v89 = (float)(*(int16_t *)(v46 - 36) + *v87 - *(v87 - 18));
          v90 = (int16_t *)v28[17421].m128_i32[2];
          v91 = v286;
          v28[17414].m128_f32[0] = v89;
          v92 = v281;
          v28[17414].m128_f32[1] = (float)(*v90 + *v87 - *v91);
          v28[17414].m128_f32[2] = (float)(*v286 + *v87 - *v90 + 2 * (*v45 - *v92));
          v88 = *(int16_t *)(v259 - 18) + *(v45 - 9) + *(v281 - 18) + *v87 - *(v87 - 9) - *(v281 - 9) - *(v45 - 18);
        }
        else
        {
          v28[17414].m128_f32[0] = (float)*v45;
          v28[17414].m128_f32[1] = (float)*(int16_t *)(v46 - 54);
          v28[17414].m128_f32[2] = (float)(*(int16_t *)(v46 - 18) + *(v87 - 9) - *(v87 - 18));
          v88 = *(int16_t *)(v46 - 54) + *(int16_t *)(v46 - 18) - *(int16_t *)(v46 - 72);
        }
        v28[17414].m128_f32[3] = (float)v88;
      }
      else
      {
        v93 = v282;
        v28[17412].m128_f32[0] = (float)(*(int16_t *)(v46 - 54) + *(int16_t *)(v46 - 18) - *(int16_t *)(v46 - 72));
        v94 = (float)(*(int16_t *)(v46 - 18) + *(v93 - 9) - *(v93 - 18));
        v95 = v281;
        v96 = v286;
        v28[17412].m128_f32[1] = v94;
        v28[17412].m128_f32[2] = (float)(v45[9] + *v95 - v96[9]);
        v97 = v282;
        v28[17412].m128_f32[3] = (float)(*(int16_t *)(v46 - 36) + v95[18] - *v95);
        v28[17413].m128_f32[0] = (float)(*v45 + *(v97 - 18) - *(v95 - 18));
        v28[17413].m128_f32[1] = (float)(*v45 + *v97 - *v95);
        v98 = v284;
        v28[17413].m128_f32[2] = (float)(*(int16_t *)(v46 - 36) + *v97 - *(v97 - 18));
        v99 = (int16_t *)v28[17421].m128_i32[2];
        v100 = v285;
        v28[17413].m128_f32[3] = (float)(*(int16_t *)(v46 - 36) + *v98 - *(v98 - 18));
        v28[17414].m128_f32[0] = (float)(*v99 + *v98 - *v100);
        v28[17414].m128_f32[1] = (float)(*(v98 - 18)
                                       + *v98
                                       - *(int16_t *)(v46 - 36)
                                       + 2 * (*(int16_t *)(v46 - 18) - *(v98 - 9)));
        v101 = (float)(*(int16_t *)(v46 - 36) + v98[2]);
        v102 = v282;
        v28[17414].m128_f32[2] = v101;
        v28[17414].m128_f32[3] = (float)(*v99 + v102[2]);
      }
    }
    else
    {
      v28[17412].m128_f32[0] = (float)(*(int16_t *)(v46 - 54) + *(int16_t *)(v46 - 18) - *(int16_t *)(v46 - 72));
      v258 = v46;
      v66 = (int16_t *)v28[17421].m128_i32[3];
      v28[17412].m128_f32[1] = (float)(*(int16_t *)(v46 - 90) + *v45 - *(v45 - 45));
      v67 = (int16_t *)v28[17421].m128_i32[2];
      v28[17412].m128_f32[2] = (float)(*(int16_t *)(v46 - 72) + *v45 - *(v45 - 36));
      v247 = v67;
      v68 = *v67 + 3 * v45[9] - 4 * v67[9];
      v69 = v281;
      v70 = (float)(v68 - (((v45[18] - *v45 - (v66[18] - *v66)) >> 1) - v66[9]));
      v71 = v282;
      v28[17412].m128_f32[3] = v70;
      v72 = v286;
      v28[17413].m128_f32[0] = (float)(*(int16_t *)(v258 - 36) + *v71 - *(v71 - 18));
      v28[17413].m128_f32[1] = (float)(v45[9] + v69[9] - v72[18]);
      v73 = v282;
      v28[17413].m128_f32[2] = (float)(*(v45 - 18) + v69[18] - *v72);
      v74 = *(int16_t *)(v258 - 18) - *(v73 - 9);
      v75 = *(v73 - 18) + *v73 - *(int16_t *)(v258 - 36);
      v76 = v284;
      v77 = (float)(v75 + 2 * v74);
      v78 = v285;
      v28[17413].m128_f32[3] = v77;
      v28[17414].m128_f32[0] = (float)(v247[9] + *v76 - v78[9]);
      v79 = (float)(*(int16_t *)(v258 - 36) + *v76 - *(v76 - 18));
      v80 = v283;
      v28[17414].m128_f32[1] = v79;
      v28[17414].m128_f32[2] = (float)(*(int16_t *)(v258 - 18) + v80[9] - *v80);
      v28[17414].m128_f32[3] = (float)(v45[9] + v80[18] - v78[27]);
    }
  }
  else
  {
    v274 = v45;
    v28[17412].m128_f32[0] = (float)(v45[27] + *v45 - v52[27]);
    v239 = *v45;
    v240 = *(v45 - 36);
    v276 = v52;
    v28[17412].m128_f32[1] = (float)(*(int16_t *)(v46 - 72) + v239 - v240);
    v241 = *v52 + 3 * v45[9] - 4 * v52[9];
    v242 = v45[18] - *v45;
    v243 = v275;
    v244 = (float)(v241 - (((v242 - (v275[2].m128_i16[2] - v275->m128_i16[0])) >> 1) - v275[1].m128_i16[1]));
    v245 = v276;
    v28[17412].m128_f32[2] = v244;
    v255 = (int16_t *)v28[17422].m128_i32[0];
    v28[17412].m128_f32[3] = (float)(*(v245 - 9) + v274[9] - v243->m128_i16[0]);
    v28[17413].m128_f32[0] = (float)(v243[1].m128_i16[1] + *v274 - v255[9]);
    v28[17413].m128_f32[1] = (float)v274[27];
    v28[17413].m128_f32[2] = (float)(*(int16_t *)(v46 - 54) + *(int16_t *)(v46 - 18) - *(int16_t *)(v46 - 72));
    v28[17413].m128_f32[3] = (float)(*(int16_t *)(v46 - 18) + v243->m128_i16[0] - v243[-2].m128_i16[7]);
    v28[17414].m128_f32[0] = (float)(*(int16_t *)(v46 - 90) + *v274 - *(v274 - 45));
    v28[17414].m128_f32[1] = (float)*v255;
    v28[17414].m128_f32[2] = (float)(*(int16_t *)(v46 - 90) + *(int16_t *)(v46 - 18) - *(int16_t *)(v46 - 108));
    v28[17414].m128_f32[3] = (float)*(v274 - 54);
    v285 = nullptr;
    v283 = nullptr;
    v284 = nullptr;
    v286 = nullptr;
    v281 = nullptr;
    v282 = nullptr;
  }
  v103 = (int32_t *)v28[17416].m128_i32[3];
  v275 = (__m128 *)*(v103 - 1);
  v104 = (int32_t *)v28[17417].m128_i32[0];
  v276 = (int16_t *)v104[1];
  v277 = v104[2];
  v278 = *(v103 - 2);
  v279 = *v104;
  v280 = *v103;
  v105 = 14 * n3536;
  n2 = 1;
  v107 = 13 * n1840_2;
  if ( 16 * v292 <= 14 * n3536 )
  {
    v105 = 16 * v292;
    n2 = 0;
  }
  if ( v105 > v107 )
    n2 = 2;
  else
    v107 = v105;
  if ( v107 > 11 * n1840_1 )
    n2 = 3;
  n3536_5 = __fwd_sub_41A130_sub_41C4B0((__m128 *)v28[17416].m128_i32[0], v293, &v275, n2);
  v109 = v28[17421].m128_i32[0];
  v110 = (__m128 *)v28[17421].m128_i32[1];
  v28[17418].m128_i32[3] = n3536_5;
  v111 = v110->m128_i16[3];
  v292 = v109;
  v112 = *(int16_t *)(v109 - 12);
  v293 = v110;
  v269 = v112 + v111;
  if ( a4 )
    v269 += (v284[3] + v282[3]) >> 1;
  v113 = *(int16_t *)(v292 - 30);
  v114 = *(int16_t *)(v292 - 48);
  v289 = v28;
  v115 = (int16_t *)v28[17421].m128_i32[3];
  n3536 = n3536_5;
  v116 = *(int16_t *)(v292 - 66);
  v294 = v115;
  v117 = v293[3].m128_i16[6] + v293[-1].m128_i16[2] + v112 + v114 + v116 + v113;
  v118 = v289;
  n3536_1 = n3536;
  v260 = *(int16_t *)(v292 - 14);
  v120 = (v293[1].m128_i16[3] + v293->m128_i16[2] + v293[-1].m128_i16[1] + v260) & 0x80000
       | (v294[2] + *(int16_t *)(v292 - 32) + 2 * *(int16_t *)(v292 - 68)) & 0x40000
       | (v293[-4].m128_i16[7] + *(int16_t *)(v292 - 50) + *(int16_t *)(v292 - 86) + *(int16_t *)(v292 - 122)) & 0x20000
       | v260 & 0x10000
       | *(uint16_t *)(v292 - 50) & 0x8000
       | (((n3536 > 3536) + (n3536 > 720) + (n3536 > 288)) << 13)
       | ((((uint32_t)(752 - (v269 + v117)) >> 31)
         + ((uint32_t)(400 - (v269 + v117)) >> 31)
         + ((uint32_t)(240 - (v269 + v117)) >> 31)) << 11);
  v289[17417].m128_i32[1] = v120;
  if ( a4 )
  {
    v261 = v120;
    v121 = *(v282 - 16);
    v289 = v118;
    v122 = *(int16_t *)(v292 - 32);
    v123 = v282[2];
    v124 = *(v282 - 7);
    n3536 = n3536_1;
    v125 = *(v284 - 7) & 0x2000000
         | v284[2] & 0x1000000
         | v124 & 0x800000
         | (v284[2] + *(v284 - 16)) & 0x400000
         | (v121 + v123) & 0x200000
         | v122 & 0x100000
         | v261;
    v118 = v289;
    v295 = (int16_t *)v289[17421].m128_i32[2];
  }
  else
  {
    v236 = *(int16_t *)(v292 - 68);
    v237 = *(int16_t *)(v292 - 86);
    v238 = *(int16_t *)(v292 - 122);
    v289 = v118;
    n3536 = n3536_1;
    v295 = (int16_t *)v118[17421].m128_i32[2];
    v125 = (v293[3].m128_i16[5] + *(int16_t *)(v292 - 50) + v238 + v237) & 0x2000000
         | (v295[11] + v295[2] + v236 + *(int16_t *)(v292 - 140)) & 0x1000000
         | (v236 + *(int16_t *)(v292 - 104)) & 0x800000
         | v236 & 0x400000
         | v237 & 0x200000
         | v238 & 0x100000
         | v120;
  }
  v126 = v125 >> 11;
  v118[17417].m128_i32[1] = v126;
  v127 = ((1 << ((v118[17794].m128_i8[4 * v126 + 8] + 31) & 31)) + v118[17794].m128_i16[2 * v126 + 5]) >> (v118[17794].m128_i8[4 * v126 + 8] & 31);
  v128 = v290;
  v129 = (int16_t *)v118[17422].m128_i32[0];
  v130 = v292;
  v118[17431].m128_i32[3] = v127;
  n2256 = v127 + n3536_1;
  v118[17431].m128_i32[2] = n2256;
  v132 = *(int16_t *)(v130 - 90);
  v296 = v129;
  n1840 = v129[36];
  v290 = ((v269 << 9) + v128) >> 13;
  n960_1 = v132 - n2256;
  v133 = ((uint32_t)(24 - v290) >> 20) & 0xFFFFF800;
  n1840_1 = n1840 - n2256;
  v301 = v293[5].m128_i16[5];
  n1840_2 = v301 - n2256;
  v297 = ((uint32_t)(39 - v290) >> 20) & 0xFFFFF800;
  if ( a4 )
  {
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v289 = v118;
    v134 = v282[1];
    v248 = *(int16_t *)(v292 + 2);
    v270 = v293[2].m128_i16[3];
    v135 = (v248 + *(int16_t *)(v292 - 52) - n2256 - (v270 - v293[5].m128_i16[6])) & 0x800000
         | (v248 + *(int16_t *)(v292 - 88) - n2256) & 0x400000
         | (v248 + v293->m128_i16[1] + v283[19] - v285[19] - n2256) & 0x200000
         | (v248 + *(int16_t *)(v292 - 16) + v284[1] - *(v284 - 8) - n2256) & 0x100000
         | (*(v295 - 8) + v248 + v134 - *(v286 - 8) - n2256) & 0x80000
         | (v248 + v270 + v134 - v281[19] - n2256) & 0x40000
         | n1840_2 & 0x20000
         | n1840_1 & 0x10000
         | n960_1 & 0x8000
         | (((n2256 > 2256) + (n2256 > 1056) + (n2256 > 144)) << 13)
         | ((((uint32_t)(55 - v290) >> 20) & 0xFFFFF800) + v298 + v133);
    v118 = v289;
    v136 = v298;
    v137 = (*(int16_t *)(v292 - 18) + *v282 - *(v282 - 9) - n2256) & 0x2000000
         | (v286[1] + v281[1] - 2 * v282[1]) & 0x1000000
         | v135;
  }
  else
  {
    v289 = v118;
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v254 = *v294;
    v267 = n2256 - *(int16_t *)(v292 - 54);
    v273 = v293[2].m128_i16[2];
    v137 = (v293[-6].m128_i16[3] - v293[-3].m128_i16[6] + v267) & 0x1000000
         | (v267 + v273 - v301) & 0x800000
         | (v296[27] - v295[18] + n2256 - v295[9]) & 0x400000
         | (*(v296 - 9) - v254 + n2256 - v293[-2].m128_i16[7]) & 0x200000
         | (v293->m128_i16[0] - v273 + n2256 - *(int16_t *)(v292 - 36)) & 0x100000
         | -n1840_2 & 0x80000
         | -n1840_1 & 0x40000
         | (n2256 - v296[18]) & 0x20000
         | (n2256 - *(v296 - 27)) & 0x10000
         | -n960_1 & 0x8000
         | (((n2256 > 2400) + (n2256 > 1024) + (n2256 > 240)) << 13)
         | (v297 + v133 + (((uint32_t)(11 - v290) >> 20) & 0xFFFFF800))
         | (n1840 - v254 + n2256 - v293[4].m128_i16[4]) & 0x2000000;
    v136 = v298;
  }
  v298 = v136;
  v138 = v137 >> 11;
  v118[17417].m128_i32[2] = v138;
  v139 = v118[25986].m128_i8[4 * v138 + 8];
  v140 = v118[25986].m128_i16[2 * v138 + 5];
  v141 = 1 << ((v139 + 31) & 31);
  v142 = v139;
  v143 = v295;
  v144 = (v141 + v140) >> (v142 & 31);
  n2576 = v144 + n2256;
  v118[17432].m128_i32[0] = n2576;
  v118[17432].m128_i32[1] = v144;
  n1840_13 = *v143;
  n960_1 = v293->m128_i16[0];
  v147 = *(int16_t *)(v292 - 36);
  n1840 = n1840_13;
  v302 = v147 - n2576;
  v301 = v147 + n2576 - 2 * *(int16_t *)(v292 - 18);
  v303 = n1840_13 - n2576;
  if ( a4 )
  {
    v148 = *(int16_t *)(v292 + 2);
    v149 = *(int16_t *)(v292 - 16);
    v289 = v118;
    v150 = *v284;
    v249 = v148;
    v151 = *v282;
    v262 = *v284;
    n1840_2 = v293[1].m128_i16[2];
    n1840_1 = v151 - n2576;
    v152 = (v249 - *(int16_t *)(v292 - 34) + 2 * v149 - n2576) & 0x100000
         | (2 * n960_1 - n2576 - n1840) & 0x80000
         | -v301 & 0x40000
         | v303 & 0x20000
         | v302 & 0x10000
         | (208 - v150) & 0x8000
         | (((n2576 > 2576) + (n2576 > 1280) + (n2576 > 640)) << 13)
         | ((((uint32_t)(33 - v290) >> 31) + ((uint32_t)(12 - v290) >> 31) + ((uint32_t)(4 - v290) >> 31)) << 11);
    v153 = v298;
    v154 = (v249 + v293[-1].m128_i16[0] - n2576 - (v295[1] - n1840_2)) & 0x2000000
         | (v249 + v149 - n2576 - (v293->m128_i16[1] - n1840_2)) & 0x1000000
         | (v262 - n2576 + n1840 - *v285) & 0x800000
         | (n1840_1 + n1840 - *v286) & 0x400000
         | (n1840_1 + v293[-2].m128_i16[7] - *(v281 - 9)) & 0x200000
         | v152;
  }
  else
  {
    v235 = *v294;
    v289 = v118;
    v153 = v298;
    v266 = *v296;
    v272 = v294[9];
    v154 = (n2576 + 3 * (n1840 - n960_1) - v235) & 0x2000000
         | (n2576 + v266 - (v295[18] + *(v295 - 18))) & 0x1000000
         | (v294[18] - *(v295 - 9) + n2576 - v293[3].m128_i16[3]) & 0x800000
         | (v272 - v293[1].m128_i16[1] - v303) & 0x400000
         | (v235 - n960_1 - v303) & 0x200000
         | v301 & 0x100000
         | (n2576 - v296[27]) & 0x80000
         | (v272 - n2576) & 0x40000
         | (n2576 - v266) & 0x20000
         | (n2576 - *(v294 - 18)) & 0x10000
         | -v302 & 0x8000
         | (((n2576 > 2464) + (n2576 > 1216) + (n2576 > 688)) << 13)
         | ((((uint32_t)(58 - v290) >> 31) + ((uint32_t)(25 - v290) >> 31) + ((uint32_t)(13 - v290) >> 31)) << 11);
  }
  v289 = v118;
  v298 = v153;
  v155 = v154 >> 11;
  v118[17417].m128_i32[3] = v155;
  v156 = ((1 << ((v118[34178].m128_i8[4 * v155 + 8] + 31) & 31)) + v118[34178].m128_i16[2 * v155 + 5]) >> (v118[34178].m128_i8[4 * v155 + 8] & 31);
  v157 = v292;
  v158 = v295;
  v118[17432].m128_i32[3] = v156;
  n2896 = v156 + n2576;
  v160 = v293;
  v118[17432].m128_i32[2] = n2896;
  v161 = v160->m128_i16[1];
  v162 = *(int16_t *)(v157 - 36);
  v250 = *(int16_t *)(v157 + 2);
  v263 = v161;
  v163 = *(int16_t *)(v157 - 54);
  v164 = v158[10];
  v304 = v162;
  v305 = v164;
  v303 = n2896 - v163;
  v306 = n2896 - v250;
  v165 = 9 - v290;
  v290 = -v290;
  v166 = v289;
  v167 = (int32_t)((3 * (v304 - *(int16_t *)(v292 - 18)) + n2896 - v163) & 0x2000000
             | (v305
              - ((uint32_t)(v293[1].m128_i16[2] + v293[2].m128_i16[3] + v293[-1].m128_i16[0] + v263) >> 1)
              + v306)
             & 0x1000000
             | (*(v295 - 27) - v293[-3].m128_i16[6] + n2896 - v293[-2].m128_i16[7]) & 0x800000
             | -(v295[18] + n2896 - 2 * v293[1].m128_i16[1]) & 0x400000
             | (v293[-3].m128_i16[7] - v304 + n2896 - v263) & 0x200000
             | (n2896 - v293[3].m128_i16[3]) & 0x100000
             | (v306 - v305) & 0x80000
             | (n2896 - *v294) & 0x40000
             | (2 * n2896 - v263 - (v293->m128_i16[0] + v250)) & 0x20000
             | (n2896 - *(int16_t *)(v292 - 16) - v250) & 0x10000
             | (n2896 - v163) & 0x8000
             | (((n2896 > 2896) + (n2896 > 1568) + (n2896 > 592)) << 13)
             | ((((uint32_t)(v290 + 37) >> 31) + ((uint32_t)(v290 + 19) >> 31) + (v165 >> 31)) << 11)) >> 11;
  v289[17418].m128_i32[0] = v167;
  LOBYTE(v163) = v166[42370].m128_i8[4 * v167 + 8];
  v168 = v166[42370].m128_i16[2 * v167 + 5];
  v169 = 1 << ((v163 + 31) & 31);
  LOBYTE(v164) = v163;
  v170 = v294;
  v171 = (v169 + v168) >> (v164 & 31);
  v172 = v292;
  v173 = v293;
  v166[17433].m128_i32[1] = v171;
  n3536_2 = v171 + n2896;
  n3536 = n3536_2;
  v166[17433].m128_i32[0] = n3536_2;
  v175 = v173->m128_i16[0];
  v176 = *v170;
  v307 = *(int16_t *)(v172 + 2);
  v308 = v175;
  v177 = *(int16_t *)(v172 - 70);
  v178 = v173[1].m128_i16[1];
  v179 = v173[-2].m128_i16[7];
  v309 = v177;
  v310 = v178;
  v311 = v179;
  v312 = n3536_2 - v307;
  v313 = n3536_2 + v176;
  v180 = (((n3536_2 > 3056) + (n3536_2 > 1952) + (n3536_2 > 368)) << 13)
       | (v297 + (((uint32_t)(v290 + 21) >> 20) & 0xFFFFF800) + v298);
  v181 = ((uint16_t)n3536_2 - (uint16_t)v177 - (uint16_t)v307) & 0x8000;
  v182 = n3536_2 - *(int16_t *)(v292 - 18);
  n3536_3 = n3536;
  v184 = v289;
  v185 = (int32_t)((n3536 - ((uint32_t)(v310 + v311 + 2 * v308) >> 2)) & 0x2000000
             | (v313 - *(int16_t *)(v292 - 36) - (v294[19] + v307)) & 0x1000000
             | (n3536 - 2 * *(int16_t *)(v292 - 34) - (v307 - v309)) & 0x800000
             | (n3536 - *(int16_t *)(v292 - 52) - v307 - (v308 - v293[-4].m128_i16[5])) & 0x400000
             | (v313 - v307 - (v310 + *(v295 - 8))) & 0x200000
             | (*(v295 - 18) + n3536 - 2 * v311) & 0x100000
             | (v295[1] - 2 * v293->m128_i16[1] + v312) & 0x80000
             | (v182 - (v308 - v311)) & 0x40000
             | (v312 - v293[4].m128_i16[5]) & 0x20000
             | (v312 - v293[-3].m128_i16[7]) & 0x10000
             | v181
             | v180) >> 11;
  v289[17418].m128_i32[1] = v185;
  v186 = ((1 << ((v184[50562].m128_i8[4 * v185 + 8] + 31) & 31)) + v184[50562].m128_i16[2 * v185 + 5]) >> (v184[50562].m128_i8[4 * v185 + 8] & 31);
  v187 = (uint8_t *)v295;
  v271 = v186;
  v184[17433].m128_i32[3] = v186;
  n1840 = n3536_3 + v186;
  v184[17433].m128_i32[2] = n3536_3 + v186;
  v188 = v293[-1].m128_u8[15];
  v189 = v187[17];
  v314 = v293[-2].m128_u8[13]
       + v293[4].m128_u8[7]
       + *(uint8_t *)(v292 - 37)
       + *((uint8_t *)v294 + 17)
       + *(v187 - 1)
       + v187[53];
  v190 = v187[89];
  v191 = v188 + *(uint8_t *)(v292 - 19) + v189;
  v192 = *(v187 - 19);
  v315 = v191;
  v193 = *((uint8_t *)v294 + 53)
       + *((uint8_t *)v294 + 35)
       + *((uint8_t *)v294 - 1)
       + *((uint8_t *)v294 - 19)
       + *((uint8_t *)v296 - 19)
       + v187[107]
       + v190
       + v187[71]
       + v192
       + *(v187 - 37);
  v194 = v293;
  n1840_14 = n1840;
  n960_1 = v293[6].m128_u8[11]
         + v293[5].m128_u8[9]
         + v293[-3].m128_u8[11]
         + v293[-4].m128_u8[9]
         + 3 * (v293[3].m128_u8[5] + *((uint8_t *)v295 + 35))
         + 7 * v293[1].m128_u8[1]
         + 6 * v293[2].m128_u8[3]
         + *(uint8_t *)(v292 - 91)
         + *(uint8_t *)(v292 - 109)
         + *(uint8_t *)(v292 - 127)
         + 8 * *(uint8_t *)(v292 - 1)
         + *((uint8_t *)v296 + 53)
         + *((uint8_t *)v296 + 35)
         + *((uint8_t *)v296 + 17)
         + *((uint8_t *)v296 - 1)
         + *(uint8_t *)(v292 - 55)
         + *(uint8_t *)(v292 - 73)
         + v193
         + 4 * v315
         + 2 * v314;
  v196 = v289;
  n1840_15 = v289[17420].m128_i32[0];
  v289[17423].m128_i32[2] = (n1840 < 1840) + (n1840 < 272);
  v198 = v194->m128_i16[0];
  n1840_16 = v196[17420].m128_i32[1];
  n1840_1 = n1840_15;
  n1840_2 = n1840_16;
  v200 = (n1840_14 - v198 <= n1840_16) + (n1840_14 - v198 < n1840_15);
  v201 = v292;
  v196[17424].m128_i32[2] = v200;
  n1840_17 = n1840_14 - *(int16_t *)(v201 - 18);
  v203 = n1840_17 < n1840_1;
  v26 = n1840_17 <= n1840_16;
  v204 = v293;
  v205 = v295;
  v196[17425].m128_i32[2] = v26 + v203;
  v196[17426].m128_i32[2] = v204[1].m128_u8[0];
  v196[17427].m128_i32[2] = *(uint8_t *)(v201 - 2);
  v206 = *(uint8_t *)(v201 - 19);
  v207 = *(uint8_t *)(v201 - 1);
  v208 = *((uint8_t *)v205 + 17) + v204[1].m128_u8[1];
  n960 = n960_1;
  v302 = v208;
  n3536_4 = n3536;
  v301 = v207 + v206;
  if ( a4 )
  {
    v211 = *((uint8_t *)v284 + 17);
    v212 = *((uint8_t *)v284 - 1);
    v289 = v196;
    v213 = *((uint8_t *)v282 - 1);
    v251 = *((uint8_t *)v283 + 17) + *((uint8_t *)v281 + 17);
    v264 = v211 + *((uint8_t *)v282 + 17);
    n960 = v251 + n960_1 + 4 * v264 + 2 * (v213 + v212);
    v214 = v264 + v301 + *((uint8_t *)v284 - 19) + v212 + *((uint8_t *)v282 - 19) + v213;
    n3536_4 = n3536;
    if ( v196[17420].m128_i32[3] )
    {
      n1840_3 = n1840 - v293->m128_i16[1] - *(int16_t *)(v292 + 2);
      if ( n1840_3 < n1840_1 || n1840_3 > n1840_2 )
      {
        n1840 = n1840 - *(int16_t *)(v292 - 16) - *(int16_t *)(v292 + 2);
        v215 = n1840 >= n1840_1 && n1840_2 >= n1840;
      }
      else
      {
        v215 = 1;
      }
    }
    else
    {
      v215 = v251 + v264 + v302 + *((uint8_t *)v285 + 17) + *((uint8_t *)v286 + 17);
    }
    if ( v289[17420].m128_i32[2] == 1 )
    {
      v230 = v282;
      n960_1 = n960;
      v253 = v214;
      n1840_4 = n1840_2;
      n1840_5 = *v282 - *v281;
      v289[17426].m128_i32[2] = (n1840_5 <= n1840_2) + (n1840_5 < n1840_1);
      n1840_6 = *v230 - *(v230 - 9);
      v234 = (n1840_6 <= n1840_4) + (n1840_6 < n1840_1);
      v214 = v253;
      n960 = n960_1;
      n3536_4 = n3536;
      v196[17427].m128_i32[2] = v234;
    }
    else if ( v196[17420].m128_i32[2] > 1 )
    {
      v217 = v284;
      n960_1 = n960;
      v265 = v215;
      n1840_7 = n1840_2;
      n1840_8 = *v284 - *v283;
      v289[17426].m128_i32[2] = (n1840_8 <= n1840_2) + (n1840_8 < n1840_1);
      n1840_10 = *v217 - *(v217 - 9);
      n1840_9 = n1840_1;
      v222 = n1840_10 < n1840_1;
      v26 = n1840_10 <= n1840_7;
      v215 = v265;
      v223 = v281;
      n960 = n960_1;
      v196[17427].m128_i32[2] = v26 + v222;
      v224 = *v282;
      n1840_11 = v224 - *v223;
      v26 = n1840_9 <= n1840_11;
      n3536_4 = n3536;
      if ( v26 && n1840_11 <= n1840_2 )
      {
        v214 = 1;
      }
      else
      {
        n1840_12 = v224 - *(v282 - 9);
        v214 = n1840_12 >= n1840_1 && n1840_12 <= n1840_2;
      }
    }
  }
  else
  {
    v214 = v301 + *(uint8_t *)(v292 - 37) + *(uint8_t *)(v292 - 55) + *(uint8_t *)(v292 - 73);
    v215 = *((uint8_t *)v296 + 17) + *((uint8_t *)v294 + 17) + v302 + *(uint8_t *)(v292 + 17);
  }
  if ( n960 >= 960 )
  {
    n15 = 15;
    v196[17419].m128_i32[0] = 15;
  }
  else
  {
    n15 = v196[17547].m128_u8[n960 >> 3];
    v196[17419].m128_i32[0] = n15;
  }
  n255 = (n3536_4 + v271 + 7) >> 4;
  if ( n255 >= 255 )
    n255 = 255;
  if ( n255 < 0 )
    n255 = 0;
  v196[17419].m128_i32[1] = v196[17435].m128_i32[n255] + n15;
  v228 = v196[17423].m128_i32[2];
  v196[17419].m128_i32[2] = n15 + v196[17434].m128_i32[n255];
  v196[17419].m128_i32[0] = n15
                          + 32 * (v215 == 0)
                          + 16 * (v214 == 0)
                          + v196[17427].m128_i32[v196[17427].m128_i32[2] + 3]
                          + v196[17426].m128_i32[v196[17426].m128_i32[2] + 3]
                          + v196[17425].m128_i32[v196[17425].m128_i32[2] + 3]
                          + v196[17424].m128_i32[v196[17424].m128_i32[2] + 3]
                          + v196[17423].m128_i32[v228 + 3];
  return n255;
}

static inline int32_t __fwd_sub_417200_sub_412B10(void *a0, int32_t a1) { return __sub_412B10((uint32_t *)a0, a1); }
static inline uint32_t __fwd_sub_417200_sub_414860(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_414860((int32_t *)a0, a1, a2, a3); }

void __sub_417200(int32_t Blocka, char a2, uint8_t *a3)
{
  alignas(16) uint8_t __hexrays_frame[66064];
  uint32_t (&v78)[15] = *(uint32_t (*)[15])(__hexrays_frame + 0);
  void * &v79 = *(void * *)(__hexrays_frame + 60);
  int32_t &n0x2000_5 = *(int32_t *)(__hexrays_frame + 64);
  char (&buf)[4] = *(char (*)[4])(__hexrays_frame + 68);
  uint64_t (&v82)[127] = *(uint64_t (*)[127])(__hexrays_frame + 72);
  int32_t &v83 = *(int32_t *)(__hexrays_frame + 1088);
  int32_t &v84 = *(int32_t *)(__hexrays_frame + 1092);
  int32_t &v85 = *(int32_t *)(__hexrays_frame + 1096);
  uint64_t (&v86)[2] = *(uint64_t (*)[2])(__hexrays_frame + 65604);
  int32_t &v87 = *(int32_t *)(__hexrays_frame + 65620);
  uint32_t (&v88)[91] = *(uint32_t (*)[91])(__hexrays_frame + 65624);
  int32_t &n4_1 = *(int32_t *)(__hexrays_frame + 65988);
  void * &Block = *(void * *)(__hexrays_frame + 65992);
  uint8_t * &v91 = *(uint8_t * *)(__hexrays_frame + 65996);
  uint32_t &v92 = *(uint32_t *)(__hexrays_frame + 66000);
  uint32_t &v93 = *(uint32_t *)(__hexrays_frame + 66004);
  uint32_t &v94 = *(uint32_t *)(__hexrays_frame + 66012);
  int32_t &Blockaa = *(int32_t *)(__hexrays_frame + 66016);
  uint8_t * &v96 = *(uint8_t * *)(__hexrays_frame + 66020);
  uint32_t &k_1 = *(uint32_t *)(__hexrays_frame + 66024);
  int32_t &Blocka_1 = *(int32_t *)(__hexrays_frame + 66032);
  ;
  bool v46, v48, v59;
  char *v28, v35;
  int32_t n8, v8, *Blockaa_1, v11, n4, n0x2000_2, n0x2000_1, v20, Blockaa_4, v26, n4_2, v30, v31,
          v32, *p_n4, n16_2, v39, v44, n256, v49, v50, v51, v52, v54, v55, v56, v57, v58, v62,
          v63, *p_n4_2, n16_1, Blockaa_2, v68, n0x2000, v71, v72, v74, *p_n4_1, n16;
  uint32_t k_2, i, v12, *Blockaa_3, v19, n0x2000_4, n0x2000_3, v24, k, v29, k_3, j_1, j, v53,
           v61, v64, v70, v73, v75;
  uint64_t *n0x2000_6;
  uint8_t *v4, *v10, *v33, *v42, *v43, *v45, *v60;
  void *v3, *v13, *v34;
  Blocka_1 = Blocka;
  v3 = alloca(65968);
  v4 = a3;
  n8 = *(uint32_t *)(Blocka + 8);
  v91 = a3;
  Blockaa = Blocka;
  v93 = 0xFFFFFFFF >> (-(char)n8 & 31);
  k_2 = (n8 + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    v8 = 12 * i;
    v88[v8] = 0;
    v88[v8 + 6] = 0;
  }
  Blockaa_1 = (int32_t *)Blockaa;
  if ( n8 <= 8 )
  {
    n256 = 256;
    do
    {
      *(__m128i *)&v78[n256 + 12] = 0;
      *(__m128i *)&v78[n256 + 8] = 0;
      *(__m128i *)&v78[n256 + 4] = 0;
      *(__m128i *)&v78[n256] = 0;
      n256 -= 16;
    }
    while ( n256 * 4 );
    v48 = Blockaa_1[2] < 8;
    v49 = Blockaa_1[1];
    Blockaa_1[4] = 0;
    if ( v48 )
    {
      v50 = 0;
      if ( v49 )
      {
        v51 = *Blockaa_1;
        v96 = a3 - 1;
        v84 = 0;
        v52 = 0;
        do
        {
          if ( !v51 )
            break;
          v83 = v52;
          v53 = 0;
          v54 = v84;
          v55 = 0;
          do
          {
            v56 = Blockaa_1[2];
            v57 = v55 - v56;
            if ( v57 < 0 )
            {
              ++v96;
              v57 = 8 - v56;
            }
            v58 = v93 & (*v96 >> (v57 & 31));
            v59 = *(uint32_t *)&buf[4 * v58 - 4] == 0;
            v85 = v57;
            ++v53;
            *(uint32_t *)&buf[4 * v58 - 4] = 1;
            v55 = v85;
            Blockaa_1[4] += v59;
            *(uint16_t *)(Blockaa_1[269559] + 2 * v54) = v58;
            v51 = *Blockaa_1;
            ++v54;
          }
          while ( v53 < *Blockaa_1 );
          v50 = Blockaa_1[4];
          v84 = v54;
          v52 = v83 + 1;
        }
        while ( v83 + 1 < (uint32_t)Blockaa_1[1] );
      }
    }
    else if ( v49 * *Blockaa_1 )
    {
      v60 = v91;
      v61 = 0;
      do
      {
        Blockaa_1[4] += *(uint32_t *)&buf[4 * *v60 - 4] == 0;
        v62 = Blockaa_1[269559];
        v63 = *v60;
        *(uint32_t *)&buf[4 * v63 - 4] = 1;
        ++v60;
        *(uint16_t *)(v62 + 2 * v61++) = v63;
      }
      while ( v61 < Blockaa_1[1] * *Blockaa_1 );
      v50 = Blockaa_1[4];
    }
    else
    {
      v50 = 0;
    }
    rc.encode(v50 - 1, v50, v93 + 1);
    v64 = Blockaa_1[4];
    if ( v64 <= v93 )
    {
      __fwd_sub_417200_sub_414860((int32_t *)v86, (int32_t)Blockaa_1, v93 - v64 + 2, 1);
      v87 = 19 * LODWORD(v86[0]);
      v70 = Blockaa_1[4];
      if ( v70 )
      {
        v71 = 0;
        v72 = 0;
        v73 = 0;
        do
        {
          if ( *(uint32_t *)&buf[4 * v72 - 4] )
          {
            __fwd_sub_417200_sub_412B10((uint32_t *)v86, v72 - v71);
            v70 = Blockaa_1[4];
            *(uint32_t *)&buf[4 * v72 - 4] = v73;
            v74 = v72 + 1;
            v71 = v72 + 1;
            ++v73;
          }
          else
          {
            v74 = v72 + 1;
          }
          v72 = v74;
        }
        while ( v73 < v70 );
      }
      if ( Blockaa_1[1] * *Blockaa_1 )
      {
        v75 = 0;
        do
        {
          *(uint16_t *)(Blockaa_1[269559] + 2 * v75) = *(uint32_t *)&buf[4
                                                                  * *(uint16_t *)(Blockaa_1[269559] + 2 * v75)
                                                                  - 4];
          ++v75;
        }
        while ( v75 < Blockaa_1[1] * *Blockaa_1 );
      }
      p_n4_1 = &n4_1;
      n16 = 16;
      do
      {
        p_n4_1 -= 6;
        free((void *)p_n4_1[5]);
        --n16;
      }
      while ( n16 );
    }
    else
    {
      p_n4_2 = &n4_1;
      n16_1 = 16;
      do
      {
        p_n4_2 -= 6;
        free((void *)p_n4_2[5]);
        --n16_1;
      }
      while ( n16_1 );
    }
  }
  else
  {
    memset(buf,0,0x10000);
    Blockaa_1[4] = 1;
    *(uint32_t *)buf = v93 & *(uint32_t *)a3;
    *(uint16_t *)Blockaa_1[269559] = 0;
    if ( (uint32_t)(Blockaa_1[1] * *Blockaa_1) > 1 )
    {
      v96 = a3;
      k_1 = k_2;
      Blockaa = (int32_t)Blockaa_1;
      v10 = v91;
      v11 = 0;
      v12 = 1;
      while ( 1 )
      {
        v10 += k_1;
        v13 = (void *)(v93 & *(uint32_t *)v10);
        if ( v13 != *(void **)&buf[8 * v11] )
        {
          v11 = 0;
          if ( v13 != *(void **)buf )
          {
            v92 = v12;
            v91 = v10;
            while ( 1 )
            {
              n4 = *(uint32_t *)&buf[8 * v11] < (uint32_t)v13;
              n0x2000_6 = &v82[v11];
              v11 = *((uint16_t *)n0x2000_6 + n4);
              if ( !*((uint16_t *)n0x2000_6 + n4) )
                break;
              if ( v13 == *(void **)&buf[8 * v11] )
              {
                v12 = v92;
                v10 = v91;
                __n4_4 = n4;
                goto LABEL_12;
              }
            }
            n0x2000_5 = (int32_t)n0x2000_6;
            v10 = v91;
            n4_1 = n4;
            Block = v13;
            Blockaa_2 = Blockaa;
            v68 = *(uint32_t *)(Blockaa + 16);
            __n4_4 = n4;
            v11 = (uint16_t)v68;
            n0x2000 = v68 + 1;
            *(uint16_t *)(n0x2000_5 + 2 * n4) = v11;
            v12 = v92;
            *(uint32_t *)(Blockaa_2 + 16) = n0x2000;
            if ( n0x2000 > 0x2000 )
            {
              v4 = v96;
              n0x2000_2 = n0x2000;
              k_2 = k_1;
              Blockaa_1 = (int32_t *)Blockaa;
              goto LABEL_14;
            }
            *(uint32_t *)&buf[8 * v11] = Block;
          }
        }
LABEL_12:
        Blockaa_3 = (uint32_t *)Blockaa;
        *(uint16_t *)(*(uint32_t *)(Blockaa + 1078236) + 2 * v12++) = v11;
        if ( v12 >= Blockaa_3[1] * *Blockaa_3 )
        {
          v4 = v96;
          k_2 = k_1;
          Blockaa_1 = (int32_t *)Blockaa;
          n0x2000_2 = *(uint32_t *)(Blockaa + 16);
          goto LABEL_14;
        }
      }
    }
    n0x2000_2 = Blockaa_1[4];
LABEL_14:
    rc.encode(n0x2000_2 - 1, n0x2000_2, 0x2001u);
    n0x2000_1 = Blockaa_1[4];
    if ( n0x2000_1 > 0x2000 )
    {
      Block = malloc(Blockaa_1[1] * k_2 * *Blockaa_1);
      v26 = *Blockaa_1;
      n4_2 = Blockaa_1[1];
      n0x2000_5 = *Blockaa_1;
      n4_1 = n4_2;
      if ( k_2 )
      {
        v94 = n4_1 * v26;
        if ( k_2 >> 1 )
        {
          v28 = (char *)Block + n4_1 * n0x2000_5;
          v96 = v4;
          k_1 = k_2;
          Blockaa = (int32_t)Blockaa_1;
          v29 = 0;
          do
          {
            v30 = 2 * v29;
            v31 = 2 * v29++ * v94;
            (&v91)[v30] = (uint8_t *)Block + v31;
            *(&v92 + v30) = (uint32_t)&v28[v31];
          }
          while ( v29 < k_2 >> 1 );
          v4 = v96;
          k_2 = k_1;
          Blockaa_1 = (int32_t *)Blockaa;
          v32 = 2 * v29 + 1;
        }
        else
        {
          v32 = 1;
        }
        if ( k_2 > v32 - 1 )
          *(&Block + v32) = (char *)Block + n4_1 * -n0x2000_5 + v94 * v32;
      }
      else
      {
        v94 = n4_1 * v26;
      }
      if ( v94 )
      {
        v33 = a3;
        if ( k_2 )
        {
          k_1 = k_2;
          Blockaa = (int32_t)Blockaa_1;
          n0x2000_5 = 0;
          k_3 = k_2;
          v39 = 0;
          j_1 = k_3 >> 1;
          while ( 1 )
          {
            while ( 1 )
            {
              if ( j_1 )
              {
                for ( j = 0; j < j_1; ++j )
                {
                  v42 = (&v91)[2 * j];
                  *v42 = v33[2 * j];
                  v43 = (uint8_t *)*(&v92 + 2 * j);
                  (&v91)[2 * j] = v42 + 1;
                  *v43 = v33[2 * j + 1];
                  *(&v92 + 2 * j) = (uint32_t)(v43 + 1);
                }
                v44 = 2 * j + 1;
                v4 = &v33[2 * j];
              }
              else
              {
                v44 = 1;
              }
              if ( v44 - 1 >= k_1 )
                break;
              v45 = *(&Block + v44);
              v4 = &v33[v44];
              *v45 = v33[v44 - 1];
              v46 = ++v39 < v94;
              *(&Block + v44) = v45 + 1;
              if ( !v46 )
                goto LABEL_71;
              v33 += v44;
            }
            if ( ++v39 >= v94 )
              break;
            v33 = v4;
          }
LABEL_71:
          k_2 = k_1;
          Blockaa_1 = (int32_t *)Blockaa;
        }
      }
      v79 = (void *)Blockaa_1[269559];
      Blockaa_1[1] = k_2 * n4_1;
      Blockaa_1[2] = 8;
      free(v79);
      v34 = malloc(2 * Blockaa_1[1] * *Blockaa_1);
      v79 = Block;
      Blockaa_1[269559] = (int32_t)v34;
      __sub_417200((int32_t)Blockaa_1, v35, (uint8_t *)v79);
      free(Block);
    }
    else
    {
      if ( 4 * k_2 )
      {
        Blockaa = (int32_t)Blockaa_1;
        v19 = 0;
        do
        {
          __fwd_sub_417200_sub_414860((int32_t *)&v86[3 * v19], v19, 256, 1);
          ++v19;
        }
        while ( v19 < 4 * k_2 );
        Blockaa_1 = (int32_t *)Blockaa;
        n0x2000_1 = *(uint32_t *)(Blockaa + 16);
      }
      if ( n0x2000_1 )
      {
        v20 = 0;
        n0x2000_4 = 0;
        n0x2000_3 = n0x2000_1;
        Blockaa_4 = (int32_t)Blockaa_1;
        do
        {
          v24 = *(uint32_t *)&buf[8 * n0x2000_4];
          if ( k_2 )
          {
            n0x2000_5 = n0x2000_4;
            k_1 = k_2;
            Blockaa = Blockaa_4;
            for ( k = 0; k < k_1; ++k )
            {
              __fwd_sub_417200_sub_412B10((uint32_t *)&v86[12 * k + 3 * v20], (uint8_t)v24);
              v20 = (uint8_t)v24 >> 6;
              v24 >>= 8;
            }
            n0x2000_4 = n0x2000_5;
            k_2 = k_1;
            Blockaa_4 = Blockaa;
            v24 = *(uint32_t *)&buf[8 * n0x2000_5];
            n0x2000_3 = *(uint32_t *)(Blockaa + 16);
          }
          v20 = (uint8_t)v24 >> 7;
          ++n0x2000_4;
        }
        while ( n0x2000_4 < n0x2000_3 );
      }
    }
    p_n4 = &n4_1;
    n16_2 = 16;
    do
    {
      p_n4 -= 6;
      free((void *)p_n4[5]);
      --n16_2;
    }
    while ( n16_2 );
  }
}

int32_t __sub_407460(uint8_t *a1, uint8_t *n2, int32_t a3, char a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
  alignas(16) uint8_t __hexrays_frame[26712];
  char (&buf)[4096] = *(char (*)[4096])(__hexrays_frame + 0);
  int32_t (&v72)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 4096);
  int32_t (&v73)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 8192);
  int32_t (&v74)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 12288);
  int32_t (&v75)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 16384);
  int32_t (&v76)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 20480);
  char (&buf_1)[4] = *(char (*)[4])(__hexrays_frame + 24576);
  uint32_t &v78 = *(uint32_t *)(__hexrays_frame + 24580);
  int32_t &v79 = *(int32_t *)(__hexrays_frame + 24584);
  int32_t &v80 = *(int32_t *)(__hexrays_frame + 24588);
  int32_t &v81 = *(int32_t *)(__hexrays_frame + 24592);
  int32_t &v82 = *(int32_t *)(__hexrays_frame + 24596);
  int32_t &v83 = *(int32_t *)(__hexrays_frame + 24600);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 24604);
  int32_t &v85 = *(int32_t *)(__hexrays_frame + 24608);
  int32_t &v86 = *(int32_t *)(__hexrays_frame + 24612);
  uint8_t * &n2_2 = *(uint8_t * *)(__hexrays_frame + 26624);
  int32_t &v88 = *(int32_t *)(__hexrays_frame + 26628);
  uint8_t * &v89 = *(uint8_t * *)(__hexrays_frame + 26632);
  int32_t &v90 = *(int32_t *)(__hexrays_frame + 26636);
  int32_t &v91 = *(int32_t *)(__hexrays_frame + 26640);
  int32_t &v92 = *(int32_t *)(__hexrays_frame + 26644);
  int32_t &v93 = *(int32_t *)(__hexrays_frame + 26648);
  int32_t &v94 = *(int32_t *)(__hexrays_frame + 26652);
  int32_t &v95 = *(int32_t *)(__hexrays_frame + 26656);
  int32_t &n191_5 = *(int32_t *)(__hexrays_frame + 26660);
  int32_t &n191_2 = *(int32_t *)(__hexrays_frame + 26664);
  uint8_t * &v98 = *(uint8_t * *)(__hexrays_frame + 26668);
  uint8_t * &v99 = *(uint8_t * *)(__hexrays_frame + 26672);
  uint8_t * &n2_1 = *(uint8_t * *)(__hexrays_frame + 26676);
  int32_t &v101 = *(int32_t *)(__hexrays_frame + 26680);
  ;
  bool v57, v67;
  char v11;
  double v16, v17, v18, v19, v20, v32, n191_1, n191_4;
  int32_t v9, v10, v12, v14, v22, v23, v24, v25, v26, v27, v28, v30, v31, n191, n191_3, v37, v38,
          v39, v40, v41, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v58, v59,
          v60, n191_6, n191_7, v63, v64;
  uint32_t v69, v70;
  uint8_t *n2_3, *v15, *v21, *v29, *v42, *n2_4, *v44, *v65, *v66;
  void *v8;
  v101 = a3;
  n2_1 = n2;
  v99 = a1;
  v8 = alloca(26672);
  v88 = a3;
  n4 = __n4_5;
  v9 = *((uint16_t *)a1 + 2);
  n2_2 = n2;
  v89 = a1;
  v92 = (int32_t)(n2_1 + 1) % 3 - (uint32_t)n2_1;
  v95 = (int32_t)(n2_1 + 2) % 3 - (uint32_t)n2_1;
  v78 = (uint32_t)&v99[*((uint32_t *)v99 + 3) + 16];
  v10 = v92;
  *(uint32_t *)buf_1 = v9;
  memset(buf,0,24576);
  v12 = *(uint32_t *)buf_1;
  n2_3 = n2_2;
  v14 = v88;
  v15 = v89;
  v16 = 0;
  v17 = 0.0;
  v18 = 0.0;
  v19 = 0.0;
  v20 = 0.0;
  v90 = 16 * (uint32_t)n2_2;
  *(uint8_t *)(16 * (uint32_t)n2_2 + v88) = 2;
  *(uint8_t *)(v14 + 33) = (uint8_t)n2_3;
  v91 = (int32_t)&n2_3[(uint32_t)v15];
  v21 = &n2_3[(uint32_t)v15 + 16 + v12 + n4];
  if ( (uint32_t)v21 < v78 )
  {
    v92 = v10;
    *(uint32_t *)buf_1 = v12;
    v85 = v10 - v12;
    v83 = v10 - v12 - n4;
    v82 = v10 - n4;
    v86 = v95 - v12;
    v81 = v95 - v12 - n4;
    v80 = v95 - n4;
    v79 = v12 + n4;
    do
    {
      v22 = v95;
      v23 = v21[v83] + v21[v92] - (v21[v85] + v21[v82]);
      v24 = v81;
      ++*(uint32_t *)&buf[4 * v23 + 2048];
      v25 = v21[v24] + v21[v22] - (v21[v86] + v21[v80]);
      v26 = v79;
      v20 = v20 + (double)v23 * (double)v23;
      ++v72[v25 + 512];
      v17 = v17 + (double)v25 * (double)v25;
      v19 = v19 + (double)v23 * (double)v25;
      ++v73[((uint16_t)v25 - (uint16_t)v23 - 512) & 0x3FF];
      v27 = v21[-v26] + *v21;
      v28 = v21[-n4];
      v29 = &v21[-*(uint32_t *)buf_1];
      v21 += n4;
      v30 = v27 - (*v29 + v28);
      v18 = v18 + (double)v23 * (double)v30;
      ++v74[((uint16_t)v30 - (uint16_t)v23 - 512) & 0x3FF];
      v16 = v16 + (double)v25 * (double)v30;
      ++v75[((uint16_t)v30 - (uint16_t)v25 - 512) & 0x3FF];
      v31 = ((uint16_t)v30 - (uint16_t)((uint32_t)(((v23 + v25) << 6) + 40) >> 7) - 512) & 0x3FF;
      ++v76[v31];
    }
    while ( (uint32_t)v21 < v78 );
    v10 = v92;
  }
  v32 = 128.0 / (0.1 - v19 * v19 + v20 * v17);
  n191_1 = (v17 * v18 - v19 * v16) * v32;
  n191_4 = v32 * (v20 * v16 - v19 * v18);
  n191 = (int32_t)n191_1;
  if ( (int32_t)n191_1 >= 191 )
    n191 = 191;
  if ( n191 < -64 )
    n191 = -64;
  n191_2 = n191;
  n191_3 = (int32_t)n191_4;
  if ( (int32_t)n191_4 >= 191 )
    n191_3 = 191;
  if ( n191_3 < -64 )
    n191_3 = -64;
  n191_5 = n191_3;
  memset(buf_1,0,2048);
  v37 = *((uint16_t *)v89 + 2);
  v38 = (*((uint16_t *)v89 + 1) - 1) * *(uint16_t *)v89;
  v92 = v10;
  v94 = v38 - 1;
  v93 = -v37;
  v39 = -__n4_5;
  v40 = v91 - (-v37 - __n4_5);
  v101 = -v37 - __n4_5;
  v99 = (uint8_t *)(v40 + 16);
  v41 = v10 + v40 + 16;
  v42 = (uint8_t *)(v40 + 16);
  v98 = (uint8_t *)v41;
  n2_1 = (uint8_t *)(v95 + v40 + 16);
  n2_4 = n2_1;
  v44 = (uint8_t *)v41;
  do
  {
    v45 = v42[v101];
    v98 = v44;
    v99 = v42;
    n2_1 = n2_4;
    v46 = v45 + *v42 - v42[v93] - v42[v39];
    v47 = *v44;
    v91 = v46;
    v48 = v94;
    n2_4 = &n2_1[-v39];
    v49 = ((uint16_t)v91
         - (uint16_t)((n191_2 * (v44[v101] + v47 - v44[v93] - v44[v39])
                             + n191_5 * (n2_1[v101] + *n2_1 - n2_1[v93] - (uint32_t)n2_1[v39])
                             + 40) >> 7)
         - 256)
        & 0x1FF;
    v44 -= v39;
    ++*(uint32_t *)&buf_1[4 * v49];
    v42 = &v99[-v39];
    v94 = v48 - 1;
  }
  while ( v48 != 1 );
  v50 = v92;
  v51 = __sub_411700((int32_t)buf_1, 512);
  v52 = 16 * (uint32_t)n2_2;
  *(uint32_t *)(a8 + 16 * (uint32_t)n2_2) = v51;
  *(uint32_t *)(a8 + v52 + 4) = __sub_411700((int32_t)v74, 1024);
  *(uint32_t *)(a8 + v52 + 8) = __sub_411700((int32_t)v75, 1024);
  *(uint32_t *)(a8 + v52 + 12) = __sub_411700((int32_t)v76, 1024);
  v91 = __sub_411700((int32_t)buf, 1024);
  v92 = __sub_411700((int32_t)v72, 1024);
  v53 = __sub_411700((int32_t)v73, 1024);
  v94 = v53;
  v54 = v53;
  if ( v91 < v53 )
    v54 = v91;
  if ( v92 < v53 )
    v53 = v92;
  v55 = v92 + v54;
  v56 = v91 + v53;
  v57 = v55 < v56;
  v93 = v56;
  v58 = v94;
  if ( v57 )
  {
    v93 = v55;
    v59 = v50;
    v50 = v95;
    v91 = v92;
    v95 = v59;
    v60 = *(uint32_t *)(a8 + v52 + 4);
    *(uint32_t *)(a8 + v52 + 4) = *(uint32_t *)(a8 + v52 + 8);
    n191_6 = n191_5;
    *(uint32_t *)(a8 + v52 + 8) = v60;
    n191_7 = n191_2;
    n191_2 = n191_6;
    n191_5 = n191_7;
  }
  v63 = v90;
  v64 = v88;
  *(uint32_t *)(v90 + v88 + 4) = n191_2;
  *(uint32_t *)(v63 + v64 + 8) = n191_5;
  v65 = &n2_2[v50];
  v66 = &n2_2[v95];
  *(uint8_t *)(16 * (uint32_t)v65 + v64) = 0;
  *(uint8_t *)(v64 + 1) = (uint8_t)v65;
  v67 = __dword_443384 == 0;
  *(uint8_t *)(16 * (uint32_t)v66 + v64) = 1;
  *(uint8_t *)(v64 + 17) = (uint8_t)v66;
  if ( !v67 && *((uint32_t *)v89 + 3) > 0x1000000u )
    return v91 + v58 + *(uint32_t *)(a8 + v52);
  v69 = *(uint32_t *)(a8 + v52);
  v70 = *(uint32_t *)(a8 + v52 + 8);
  if ( v69 >= *(uint32_t *)(a8 + v52 + 4) )
    v69 = *(uint32_t *)(a8 + v52 + 4);
  if ( v70 >= *(uint32_t *)(a8 + v52 + 12) )
    v70 = *(uint32_t *)(a8 + v52 + 12);
  if ( v69 < v70 )
    v70 = v69;
  return v93 + v70;
}

static inline int32_t __fwd_sub_405CF0_sub_407460(void *a0, void *a1, int32_t a2, char a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7) { return __sub_407460((uint8_t *)a0, (uint8_t *)a1, a2, a3, a4, a5, a6, a7); }

BMF_SSE int32_t __sub_405CF0(int32_t a1, int32_t n3, char a3)
{
  alignas(16) uint8_t __hexrays_frame[41456];
  int32_t &v174 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v175 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &v176 = *(int32_t *)(__hexrays_frame + 8);
  char (&buf)[4] = *(char (*)[4])(__hexrays_frame + 16);
  int32_t (&v178)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 4112);
  int32_t (&v179)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 8208);
  int32_t (&v180)[5120] = *(int32_t (*)[5120])(__hexrays_frame + 12304);
  char (&buf_3)[4] = *(char (*)[4])(__hexrays_frame + 32784);
  char (&buf_1)[4] = *(char (*)[4])(__hexrays_frame + 34832);
  int32_t &v183 = *(int32_t *)(__hexrays_frame + 34836);
  int32_t &v184 = *(int32_t *)(__hexrays_frame + 34840);
  int32_t &v185 = *(int32_t *)(__hexrays_frame + 34844);
  uint8_t * &v186 = *(uint8_t * *)(__hexrays_frame + 34848);
  uint8_t * &v187 = *(uint8_t * *)(__hexrays_frame + 34852);
  int32_t &v188 = *(int32_t *)(__hexrays_frame + 34856);
  int32_t &v189 = *(int32_t *)(__hexrays_frame + 34860);
  uint8_t * &v190 = *(uint8_t * *)(__hexrays_frame + 34864);
  int32_t &v191 = *(int32_t *)(__hexrays_frame + 34868);
  int32_t &v192 = *(int32_t *)(__hexrays_frame + 34872);
  int32_t &v193 = *(int32_t *)(__hexrays_frame + 34876);
  uint8_t * &v194 = *(uint8_t * *)(__hexrays_frame + 34880);
  uint8_t * &v195 = *(uint8_t * *)(__hexrays_frame + 34884);
  int32_t &v196 = *(int32_t *)(__hexrays_frame + 34888);
  int32_t &v197 = *(int32_t *)(__hexrays_frame + 34892);
  uint8_t * &v198 = *(uint8_t * *)(__hexrays_frame + 34896);
  int32_t &v199 = *(int32_t *)(__hexrays_frame + 34900);
  char (&buf_4)[4] = *(char (*)[4])(__hexrays_frame + 36880);
  char (&buf_2)[4] = *(char (*)[4])(__hexrays_frame + 38928);
  __m128i &v202 = *(__m128i *)(__hexrays_frame + 40976);
  __m128i &v203 = *(__m128i *)(__hexrays_frame + 40992);
  __m128i &v204 = *(__m128i *)(__hexrays_frame + 41008);
  __m128i &v205 = *(__m128i *)(__hexrays_frame + 41024);
  __m128i &v206 = *(__m128i *)(__hexrays_frame + 41040);
  __m128i &v207 = *(__m128i *)(__hexrays_frame + 41056);
  int64_t &v208 = *(int64_t *)(__hexrays_frame + 41072);
  int64_t &v209 = *(int64_t *)(__hexrays_frame + 41080);
  double &v210 = *(double *)(__hexrays_frame + 41088);
  double &v211 = *(double *)(__hexrays_frame + 41096);
  double &v212 = *(double *)(__hexrays_frame + 41104);
  double &v213 = *(double *)(__hexrays_frame + 41112);
  int32_t (&v214)[4] = *(int32_t (*)[4])(__hexrays_frame + 41120);
  uint64_t (&v215)[5] = *(uint64_t (*)[5])(__hexrays_frame + 41136);
  double &v216 = *(double *)(__hexrays_frame + 41176);
  int32_t (&v217)[16] = *(int32_t (*)[16])(__hexrays_frame + 41184);
  char (&v218)[64] = *(char (*)[64])(__hexrays_frame + 41248);
  char (&v219)[64] = *(char (*)[64])(__hexrays_frame + 41312);
  int64_t &v220 = *(int64_t *)(__hexrays_frame + 41376);
  double &v221 = *(double *)(__hexrays_frame + 41384);
  double &v222 = *(double *)(__hexrays_frame + 41392);
  uint8_t * &v223 = *(uint8_t * *)(__hexrays_frame + 41400);
  uint32_t &v224 = *(uint32_t *)(__hexrays_frame + 41404);
  int32_t &__sub_405CF0_n191_1 = *(int32_t *)(__hexrays_frame + 41408);
  uint8_t * &v226 = *(uint8_t * *)(__hexrays_frame + 41412);
  uint32_t &v227 = *(uint32_t *)(__hexrays_frame + 41416);
  int32_t &v228 = *(int32_t *)(__hexrays_frame + 41424);
  ;
  __m128i v21, v46, v47, v51, v54, v113, v114;
  bool v19, n2_4, v42, v106;
  char v7, v10, v12, v16, v18, v22, *v44, n0x100_1, k;
  double v65, v66, v68, v69, v70, v72, v73, v74, v75, v76;
  int16_t v92;
  int32_t n4, v5, v6, n192, v11, v13, v14, n2_3, n2, n16, *v25, n128_1, n128, v30, v31, v32, v33,
          v34, v35, n0x4000, n2_1, v43, n0x100, v48, v49, i, n255, j_1, j, v57, n128_2, v59,
          n128_3, v61, v62, v63, v71, __sub_405CF0_n191, n191_2, n191_3, v81, v82, v83, v84, v85,
          v86, v87, v88, v89, v90, v91, v93, v94, v95, v96, v98, v99, v100, v101, v102, n191_4,
          n2_2, __sub_405CF0_n3_1, *v112, v115, v116, v118, v119, v120, v124, v125, v126, v127,
          v128, v130, v131, v132, v133, v137, v138, v139, v140, v141, v143, n192_1, n192_2,
          n192_4, n192_3, v148, v149, v150, v152, v154, v155, v156, v157, v158, n192_5, v161,
          v162, v163, v165, v167, v168, v169, v170, v171, n192_6;
  uint32_t v9, v15, v17, v26, v36, v38, v39, v67, v80, v103, v104, v105, v108, v110, v129, v142,
           v159, v172;
  uint8_t *v29, *v64, *v97, *v121, *v122, *v123, *v134, *v135, *v136, *v151, *v153, *v164, *v166;
  void *v3;
  v228 = a1;
  v3 = alloca(41424);
  n4 = __n4_5;
  v5 = *(uint16_t *)(a1 + 4);
  v6 = *(uint32_t *)(a1 + 12);
  v226 = (uint8_t *)a1;
  __dword_443388 = 1;
  LODWORD(v208) = v5;
  v227 = a1 + v6 + 16;
  memset(buf,0,0x8000);
  n192 = 192;
  do
  {
    *(__m128i *)((char *)&v215[4] + n192) = 0;
    *(__m128i *)((char *)&v215[2] + n192) = 0;
    *(__m128i *)((char *)v215 + n192) = 0;
    *(__m128i *)((char *)v214 + n192) = 0;
    n192 -= 64;
  }
  while ( n192 );
  if ( n4 > 0 )
  {
    if ( n4 / 2 )
    {
      v9 = 0;
      do
      {
        v10 = 2 * v9;
        v11 = 32 * v9;
        __byte_44339D[v11] = 2 * v9;
        v12 = 2 * v9++ + 1;
        __byte_44339C[v11] = v10;
        __byte_4433AD[v11] = v12;
        __byte_4433AC[v11] = v12;
      }
      while ( v9 < n4 / 2 );
      v13 = 2 * v9 + 1;
    }
    else
    {
      v13 = 1;
    }
    n192 = v13 - 1;
    if ( n4 > (uint32_t)(v13 - 1) )
    {
      v14 = 4 * v13;
      BYTE1(__n256_2[v14]) = n192;
      LOBYTE(__n256_2[v14]) = n192;
    }
    if ( n4 >= 3 )
    {
      v15 = __fwd_sub_405CF0_sub_407460(v226, nullptr, (int32_t)v217, v7, v174, v175, v176, (int32_t)&v214[2]);
      v17 = __fwd_sub_405CF0_sub_407460(v226, (uint8_t *)1, (int32_t)v218, v16, v174, v175, v176, (int32_t)&v214[2]);
      v19 = v17 >= v15;
      if ( v17 >= v15 )
        v17 = v15;
      n2_3 = !v19;
      v205.m128i_i32[2] = n2_3;
      v21 = 0;
      n2_4 = __fwd_sub_405CF0_sub_407460(v226, (uint8_t *)2, (int32_t)v219, v18, v174, v175, v176, (int32_t)&v214[2]) < v17;
      n2 = n2_3;
      if ( n2_4 )
        n2 = 2;
      v205.m128i_i32[2] = n2;
      n16 = 16;
      v25 = &v217[16 * n2];
      do
      {
        *(uint64_t *)(n16 * 4 + BMF_BLOB(0x00443394)) = *(uint64_t *)&v25[n16 - 2];
        *(uint64_t *)(n16 * 4 + BMF_BLOB(0x0044338C)) = *(uint64_t *)&v25[n16 - 4];
        *(uint64_t *)(n16 * 4 + BMF_BLOB(0x00443384)) = *(uint64_t *)&v25[n16 - 6];
        *(uint64_t *)(n16 * 4 + BMF_BLOB(0x0044337C)) = *(uint64_t *)&v25[n16 - 8];
        n16 -= 8;
      }
      while ( n16 * 4 );
      v26 = v214[4 * v205.m128i_i32[2] + 2];
      v205.m128i_i32[1] = 16 * v205.m128i_i32[2];
      LODWORD(v209) = (uint8_t)__byte_44339D[0] - v205.m128i_i32[2];
      n128_1 = __dword_4433A4[4 * v205.m128i_i32[2]];
      HIDWORD(v208) = (uint8_t)__byte_4433AD[0] - v205.m128i_i32[2];
      n128 = __dword_4433A0[4 * v205.m128i_i32[2]];
      if ( __dword_441090 || (uint32_t)__n7_0 > 7 )
      {
        LODWORD(v210) = n128_1;
        v207.m128i_i32[3] = n128;
        HIDWORD(v209) = n4;
        v202.m128i_i32[1] = (__n7_0 + 5) / 3u;
        v184 = n128 - 1;
        v57 = n128 - 1;
        v192 = n128_1 - 1;
        n128_2 = n128;
        v59 = n128_1 - 1;
        v183 = n128_2 - v202.m128i_i32[1];
        n128_3 = n128_1;
        v61 = v183;
        *(uint32_t *)buf_1 = n128_3 - v202.m128i_i32[1];
        v202.m128i_i32[0] = (int32_t)&v226[v205.m128i_i32[2]];
        v62 = n128_3 - v202.m128i_i32[1];
        while ( 1 )
        {
          if ( v57 <= v61 )
          {
            if ( v59 <= v62 )
            {
              v203.m128i_i32[0] = v207.m128i_i32[3] + 1;
              n192_1 = v207.m128i_i32[3] + 1;
              v205.m128i_i32[3] = LODWORD(v210) + 1;
              n192_2 = LODWORD(v210) + 1;
              v202.m128i_i32[3] = v207.m128i_i32[3] + v202.m128i_i32[1];
              n192_4 = v207.m128i_i32[3] + v202.m128i_i32[1];
              v202.m128i_i32[2] = LODWORD(v210) + v202.m128i_i32[1];
              n192_3 = LODWORD(v210) + v202.m128i_i32[1];
              while ( 1 )
              {
                if ( n192_1 >= n192_4 )
                {
                  if ( n192_2 >= n192_3 )
                  {
                    n128_1 = LODWORD(v210);
                    n128 = v207.m128i_i32[3];
                    n4 = HIDWORD(v209);
                    v21 = 0;
                    goto LABEL_19;
                  }
                  if ( n192_1 >= n192_4 )
                    goto LABEL_109;
                }
                if ( n192_1 < 192 )
                {
                  v202.m128i_i32[2] = n192_3;
                  memset(buf_1,0,2048);
                  v148 = *(uint16_t *)v226 * (*((uint16_t *)v226 + 1) - 1);
                  v149 = *((uint16_t *)v226 + 2);
                  v205.m128i_i32[3] = n192_2;
                  v203.m128i_i32[0] = n192_1;
                  v203.m128i_i32[1] = v148 - 1;
                  v205.m128i_i32[0] = v26;
                  v204.m128i_i32[1] = -v149;
                  v150 = -__n4_5;
                  v204.m128i_i32[0] = -v149 - __n4_5;
                  v203.m128i_i32[3] = v202.m128i_i32[0] - v204.m128i_i32[0] + 16;
                  v203.m128i_i32[2] = v209 + v202.m128i_i32[0] - v204.m128i_i32[0] + 16;
                  v151 = (uint8_t *)v203.m128i_i32[2];
                  v152 = HIDWORD(v208) + v202.m128i_i32[0] - v204.m128i_i32[0] + 16;
                  v153 = (uint8_t *)v203.m128i_i32[3];
                  do
                  {
                    v154 = v153[v204.m128i_i32[0]];
                    v203.m128i_i64[1] = __PAIR64__((uint32_t)v153, (uint32_t)v151);
                    v204.m128i_i32[2] = v152;
                    v155 = v154 + *v153 - v153[v204.m128i_i32[1]] - v153[v150];
                    v156 = *v151;
                    v204.m128i_i32[3] = v155;
                    v152 = v204.m128i_i32[2] - v150;
                    v157 = v203.m128i_i32[1];
                    v158 = (v204.m128i_i16[6]
                          - (uint16_t)((v203.m128i_i32[0]
                                              * (v151[v204.m128i_i32[0]] + v156 - v151[v204.m128i_i32[1]] - v151[v150])
                                              + LODWORD(v210)
                                              * (*(uint8_t *)(v204.m128i_i32[2] + v204.m128i_i32[0])
                                               + *(uint8_t *)v204.m128i_i32[2]
                                               - *(uint8_t *)(v204.m128i_i32[2] + v204.m128i_i32[1])
                                               - (uint32_t)*(uint8_t *)(v204.m128i_i32[2] + v150))
                                              + 40) >> 7)
                          - 256)
                         & 0x1FF;
                    ++*(uint32_t *)&buf_1[4 * v158];
                    v151 -= v150;
                    v153 = (uint8_t *)(v203.m128i_i32[3] - v150);
                    v203.m128i_i32[1] = v157 - 1;
                  }
                  while ( v157 != 1 );
                  n192_2 = v205.m128i_i32[3];
                  n192_1 = v203.m128i_i32[0];
                  v26 = v205.m128i_i32[0];
                  v159 = __sub_411700((int32_t)buf_1, 512);
                  n192_3 = v202.m128i_i32[2];
                  n192_5 = v207.m128i_i32[3];
                  if ( v159 < v26 )
                  {
                    v26 = v159;
                    n192_5 = n192_1;
                  }
                  v207.m128i_i32[3] = n192_5;
                  n192_4 = v202.m128i_i32[1] + n192_5;
                }
                if ( n192_2 < n192_3 )
                {
LABEL_109:
                  if ( n192_2 < 192 )
                  {
                    v202.m128i_i32[3] = n192_4;
                    memset(buf_2,0,2048);
                    v161 = *(uint16_t *)v226 * (*((uint16_t *)v226 + 1) - 1);
                    v162 = *((uint16_t *)v226 + 2);
                    v205.m128i_i32[3] = n192_2;
                    v203.m128i_i32[0] = n192_1;
                    v206.m128i_i32[0] = v161 - 1;
                    v205.m128i_i32[0] = v26;
                    v207.m128i_i32[0] = -v162;
                    v163 = -__n4_5;
                    v206.m128i_i32[3] = -v162 - __n4_5;
                    v206.m128i_i32[2] = v202.m128i_i32[0] - v206.m128i_i32[3] + 16;
                    v164 = (uint8_t *)(v209 + v202.m128i_i32[0] - v206.m128i_i32[3] + 16);
                    v206.m128i_i32[1] = (int32_t)v164;
                    v165 = HIDWORD(v208) + v202.m128i_i32[0] - v206.m128i_i32[3] + 16;
                    v166 = (uint8_t *)v206.m128i_i32[2];
                    do
                    {
                      v167 = v166[v206.m128i_i32[3]];
                      *(int64_t *)((char *)v206.m128i_i64 + 4) = __PAIR64__((uint32_t)v166, (uint32_t)v164);
                      v207.m128i_i32[1] = v165;
                      v168 = v167 + *v166 - v166[v207.m128i_i32[0]] - v166[v163];
                      v169 = *v164;
                      v207.m128i_i32[2] = v168;
                      v165 = v207.m128i_i32[1] - v163;
                      v170 = v206.m128i_i32[0];
                      v171 = (v207.m128i_i16[4]
                            - (uint16_t)((v207.m128i_i32[3]
                                                * (v164[v206.m128i_i32[3]] + v169 - v164[v207.m128i_i32[0]] - v164[v163])
                                                + v205.m128i_i32[3]
                                                * (*(uint8_t *)(v207.m128i_i32[1] + v206.m128i_i32[3])
                                                 + *(uint8_t *)v207.m128i_i32[1]
                                                 - *(uint8_t *)(v207.m128i_i32[1] + v207.m128i_i32[0])
                                                 - (uint32_t)*(uint8_t *)(v207.m128i_i32[1] + v163))
                                                + 40) >> 7)
                            - 256)
                           & 0x1FF;
                      ++*(uint32_t *)&buf_2[4 * v171];
                      v164 -= v163;
                      v166 = (uint8_t *)(v206.m128i_i32[2] - v163);
                      v206.m128i_i32[0] = v170 - 1;
                    }
                    while ( v170 != 1 );
                    n192_2 = v205.m128i_i32[3];
                    n192_1 = v203.m128i_i32[0];
                    v26 = v205.m128i_i32[0];
                    v172 = __sub_411700((int32_t)buf_2, 512);
                    n192_4 = v202.m128i_i32[3];
                    n192_6 = LODWORD(v210);
                    if ( v172 < v26 )
                    {
                      v26 = v172;
                      n192_6 = n192_2;
                    }
                    LODWORD(v210) = n192_6;
                    n192_3 = v202.m128i_i32[1] + n192_6;
                  }
                }
                ++n192_1;
                ++n192_2;
              }
            }
            if ( v57 <= v61 )
              goto LABEL_55;
          }
          if ( v57 >= -64 )
          {
            *(uint32_t *)buf_1 = v62;
            memset(buf_3,0,2048);
            v118 = *(uint16_t *)v226 * (*((uint16_t *)v226 + 1) - 1);
            v119 = *((uint16_t *)v226 + 2);
            v192 = v59;
            v184 = v57;
            v185 = v118 - 1;
            v205.m128i_i32[0] = v26;
            v189 = -v119;
            v120 = -__n4_5;
            v188 = -v119 - __n4_5;
            v187 = (uint8_t *)(v202.m128i_i32[0] - v188 + 16);
            v186 = (uint8_t *)(v209 + v202.m128i_i32[0] - v188 + 16);
            v121 = v186;
            v122 = (uint8_t *)(HIDWORD(v208) + v202.m128i_i32[0] - v188 + 16);
            v123 = v187;
            do
            {
              v124 = v123[v188];
              v186 = v121;
              v187 = v123;
              v190 = v122;
              v125 = v124 + *v123 - v123[v189] - v123[v120];
              v126 = *v121;
              v191 = v125;
              v122 = &v190[-v120];
              v127 = v185;
              v128 = ((uint16_t)v191
                    - (uint16_t)((v184 * (v121[v188] + v126 - v121[v189] - v121[v120])
                                        + LODWORD(v210) * (v190[v188] + *v190 - v190[v189] - (uint32_t)v190[v120])
                                        + 40) >> 7)
                    - 256)
                   & 0x1FF;
              ++*(uint32_t *)&buf_3[4 * v128];
              v121 -= v120;
              v123 = &v187[-v120];
              v185 = v127 - 1;
            }
            while ( v127 != 1 );
            v59 = v192;
            v57 = v184;
            v26 = v205.m128i_i32[0];
            v129 = __sub_411700((int32_t)buf_3, 512);
            v62 = *(uint32_t *)buf_1;
            v130 = v207.m128i_i32[3];
            if ( v129 < v26 )
            {
              v26 = v129;
              v130 = v57;
            }
            v207.m128i_i32[3] = v130;
            v61 = v130 - v202.m128i_i32[1];
          }
          if ( v59 > v62 )
          {
LABEL_55:
            if ( v59 >= -64 )
            {
              v183 = v61;
              memset(buf_4,0,2048);
              v131 = *(uint16_t *)v226 * (*((uint16_t *)v226 + 1) - 1);
              v132 = *((uint16_t *)v226 + 2);
              v192 = v59;
              v184 = v57;
              v193 = v131 - 1;
              v205.m128i_i32[0] = v26;
              v197 = -v132;
              v133 = -__n4_5;
              v196 = -v132 - __n4_5;
              v195 = (uint8_t *)(v202.m128i_i32[0] - v196 + 16);
              v134 = (uint8_t *)(v209 + v202.m128i_i32[0] - v196 + 16);
              v194 = v134;
              v135 = (uint8_t *)(HIDWORD(v208) + v202.m128i_i32[0] - v196 + 16);
              v136 = v195;
              do
              {
                v137 = v136[v196];
                v194 = v134;
                v195 = v136;
                v198 = v135;
                v138 = v137 + *v136 - v136[v197] - v136[v133];
                v139 = *v134;
                v199 = v138;
                v135 = &v198[-v133];
                v140 = v193;
                v141 = ((uint16_t)v199
                      - (uint16_t)((v207.m128i_i32[3] * (v134[v196] + v139 - v134[v197] - v134[v133])
                                          + v192 * (v198[v196] + *v198 - v198[v197] - (uint32_t)v198[v133])
                                          + 40) >> 7)
                      - 256)
                     & 0x1FF;
                ++*(uint32_t *)&buf_4[4 * v141];
                v134 -= v133;
                v136 = &v195[-v133];
                v193 = v140 - 1;
              }
              while ( v140 != 1 );
              v59 = v192;
              v57 = v184;
              v26 = v205.m128i_i32[0];
              v142 = __sub_411700((int32_t)buf_4, 512);
              v61 = v183;
              v143 = LODWORD(v210);
              if ( v142 < v26 )
              {
                v26 = v142;
                v143 = v59;
              }
              LODWORD(v210) = v143;
              v62 = v143 - v202.m128i_i32[1];
            }
          }
          --v57;
          --v59;
        }
      }
LABEL_19:
      v29 = &v226[v208 + 16 + n4 + v205.m128i_i32[2]];
      if ( (uint32_t)v29 < v227 )
      {
        LODWORD(v210) = n128_1;
        v207.m128i_i32[3] = n128;
        v205.m128i_i32[0] = v26;
        HIDWORD(v209) = n4;
        do
        {
          v30 = v29[HIDWORD(v208)];
          v31 = v30 * LODWORD(v210);
          v32 = v29[v209];
          v33 = v32 * v207.m128i_i32[3];
          ++v180[v30 - v32 + 1280];
          v34 = *v29 + 512;
          v29 += HIDWORD(v209);
          v35 = ((uint16_t)v34 - (uint16_t)((uint32_t)(v31 + v33 + 40) >> 7)) & 0x3FF;
          ++*(uint32_t *)&buf[4 * v35];
          ++v178[v34 - v32];
          ++v179[v34 - v30];
          v36 = v34 - ((uint32_t)(((v32 + v30) << 6) + 40) >> 7);
          ++v180[v36];
        }
        while ( (uint32_t)v29 < v227 );
        n128_1 = LODWORD(v210);
        n128 = v207.m128i_i32[3];
        v26 = v205.m128i_i32[0];
        n4 = HIDWORD(v209);
      }
      n0x4000 = v26 >> 7;
      if ( v26 >> 7 >= 0x4000 )
        n0x4000 = 0x4000;
      v38 = n0x4000 + v26;
      v39 = *(int32_t *)((char *)&v214[3] + v205.m128i_i32[1]);
      n2_4 = v39 < v38;
      if ( v39 < v38 )
      {
        v38 = *(int32_t *)((char *)&v214[3] + v205.m128i_i32[1]);
        n128 = 128;
        n128_1 = 0;
      }
      n2_1 = n2_4;
      if ( *(uint32_t *)((char *)v215 + v205.m128i_i32[1]) < v38 )
      {
        v38 = *(uint32_t *)((char *)v215 + v205.m128i_i32[1]);
        n2_1 = 2;
        n128 = 0;
        n128_1 = 128;
      }
      v42 = v38 <= *(uint32_t *)((char *)v215 + v205.m128i_i32[1] + 4);
      if ( v38 > *(uint32_t *)((char *)v215 + v205.m128i_i32[1] + 4) )
        n2_1 = 3;
      v43 = v205.m128i_i32[1];
      if ( !v42 )
      {
        n128 = 64;
        n128_1 = 64;
      }
      *(int32_t *)((char *)__dword_4433A0 + v205.m128i_i32[1]) = n128;
      *(int32_t *)((char *)__dword_4433A4 + v43) = n128_1;
      v44 = &buf[4096 * n2_1];
      n0x100 = (uint8_t)v44 & 0xF;
      v46 = _mm_cvtsi32_si128(0);
      do
      {
        v46 = _mm_add_epi32(
                _mm_add_epi32(
                  _mm_add_epi32(_mm_add_epi32(v46, *(__m128i *)&v44[4 * n0x100]), *(__m128i *)&v44[4 * n0x100 + 16]),
                  *(__m128i *)&v44[4 * n0x100 + 32]),
                *(__m128i *)&v44[4 * n0x100 + 48]);
        n0x100 += 16;
      }
      while ( (uint32_t)n0x100 < 0x100 );
      v47 = _mm_add_epi32(v46, _mm_srli_si128(v46, 8));
      v48 = _mm_cvtsi128_si32(_mm_add_epi32(v47, _mm_srli_si128(v47, 4)));
      v49 = v48;
      n0x100_1 = -1;
      if ( n0x100 < 1024 )
      {
        HIDWORD(v209) = n4;
        do
        {
          v48 = *(uint32_t *)&v44[4 * n0x100] + v48 - *(uint32_t *)&v44[4 * n0x100 - 1024];
          if ( v48 >= v49 )
          {
            n0x100_1 = n0x100;
            v49 = v48;
          }
          ++n0x100;
        }
        while ( n0x100 < 1024 );
        n4 = HIDWORD(v209);
      }
      v51 = v21;
      __byte_44339F[v205.m128i_i32[1]] = n0x100_1 + 1;
      for ( i = 0; (uint32_t)i < 0x100; i += 16 )
        v51 = _mm_add_epi32(
                _mm_add_epi32(
                  _mm_add_epi32(_mm_add_epi32(v51, *(__m128i *)&v180[i + 1024]), *(__m128i *)&v180[i + 1028]),
                  *(__m128i *)&v180[i + 1032]),
                *(__m128i *)&v180[i + 1036]);
      n255 = 255;
      v54 = _mm_add_epi32(v51, _mm_srli_si128(v51, 8));
      j_1 = _mm_cvtsi128_si32(_mm_add_epi32(v54, _mm_srli_si128(v54, 4)));
      for ( j = j_1; i < 512; ++i )
      {
        j = v180[i + 1024] + j - v180[i + 768];
        if ( j >= j_1 )
        {
          n255 = i;
          j_1 = j;
        }
      }
      n192 = n255 + 1;
      __byte_44339F[16 * HIDWORD(v208) + v205.m128i_i32[1]] = n192;
      if ( n4 >= 4 )
      {
        v202 = v21;
        v203 = v21;
        v204 = v21;
        v205 = v21;
        v206 = v21;
        v207 = v21;
        memset(buf,0,0x8000);
        v223 = &v226[v208];
        v63 = (int32_t)(v227 - 17 - (uint32_t)&v226[v208]) / 4;
        v64 = &v226[v208 + 20];
        if ( v227 <= (uint32_t)v64 )
        {
          v72 = *(double *)&v202.m128i_i64[1];
          v73 = *(double *)v203.m128i_i64;
          v74 = *(double *)&v204.m128i_i64[1];
          v209 = v202.m128i_i64[1];
          v208 = v204.m128i_i64[1];
          v216 = *(double *)v202.m128i_i64;
          v220 = v206.m128i_i64[1];
          v75 = *(double *)v203.m128i_i64;
        }
        else
        {
          v65 = *(double *)v206.m128i_i64;
          v66 = *(double *)v204.m128i_i64;
          v220 = v206.m128i_i64[1];
          v216 = *(double *)v202.m128i_i64;
          v221 = *(double *)&v202.m128i_i64[1];
          v212 = *(double *)v203.m128i_i64;
          v222 = *(double *)&v204.m128i_i64[1];
          v213 = *(double *)v207.m128i_i64;
          v211 = *(double *)&v207.m128i_i64[1];
          LODWORD(v210) = &v226[v208 + 20];
          v67 = 0;
          v224 = (int32_t)(v227 - 17 - (uint32_t)&v226[v208]) / 4;
          do
          {
            v68 = (double)(v226[4 * v67 + 16] + v223[4 * v67 + 20] - (v226[4 * v67 + 20] + v223[4 * v67 + 16]));
            v69 = (double)(v226[4 * v67 + 17] + v223[4 * v67 + 21] - (v226[4 * v67 + 21] + v223[4 * v67 + 17]));
            v70 = (double)(v226[4 * v67 + 18] + v223[4 * v67 + 22] - (v226[4 * v67 + 22] + v223[4 * v67 + 18]));
            v71 = v226[4 * v67 + 19] + v223[4 * v67 + 23] - (v226[4 * v67 + 23] + v223[4 * v67 + 19]);
            v216 = v216 + v68 * v68;
            ++v67;
            v221 = v221 + v68 * v69;
            v212 = v212 + v68 * v70;
            v66 = v66 + v69 * v69;
            v222 = v222 + v69 * v70;
            v65 = v65 + v70 * v70;
            *(double *)&v220 = *(double *)&v220 + v68 * (double)v71;
            v213 = v213 + v69 * (double)v71;
            v211 = v211 + v70 * (double)v71;
          }
          while ( v67 < v224 );
          v72 = v221;
          v73 = v212;
          v74 = v222;
          v64 = (uint8_t *)LODWORD(v210);
          v63 = v224;
          *(double *)&v207.m128i_i64[1] = v211;
          *(double *)v207.m128i_i64 = v213;
          v206.m128i_i64[1] = v220;
          *(double *)v206.m128i_i64 = v65;
          *(double *)&v204.m128i_i64[1] = v222;
          *(double *)v204.m128i_i64 = v66;
          *(double *)v203.m128i_i64 = v212;
          *(double *)&v202.m128i_i64[1] = v221;
          *(double *)v202.m128i_i64 = v216;
          v75 = v212;
          *(double *)&v208 = v222;
          *(double *)&v209 = v221;
        }
        v221 = v72;
        v222 = v74;
        v210 = v75;
        *(double *)&v203.m128i_i64[1] = v72;
        *(double *)v205.m128i_i64 = v73;
        *(double *)&v205.m128i_i64[1] = v74;
        v211 = v216 * *(double *)v206.m128i_i64 - v75 * v73;
        v212 = 0.0 - v216 * v74 + v73 * *(double *)&v209;
        v213 = v75 * v74 - *(double *)&v209 * *(double *)v206.m128i_i64;
        v76 = 128.0 / (*(double *)v204.m128i_i64 * v211 + *(double *)&v208 * v212 + v72 * v213 + 0.1);
        *(double *)v214 = v76;
        __sub_405CF0_n191 = (int32_t)(((v73 * *(double *)&v208 - v72 * *(double *)v206.m128i_i64) * *(double *)v207.m128i_i64
                    + (0.0 - v73 * *(double *)v204.m128i_i64 + v72 * v222) * *(double *)&v207.m128i_i64[1]
                    + (*(double *)v206.m128i_i64 * *(double *)v204.m128i_i64 - v222 * *(double *)&v208)
                    * *(double *)&v220)
                   * v76);
        if ( __sub_405CF0_n191 >= 191 )
          __sub_405CF0_n191 = 191;
        if ( __sub_405CF0_n191 < -64 )
          __sub_405CF0_n191 = -64;
        __sub_405CF0_n191_1 = __sub_405CF0_n191;
        n191_2 = (int32_t)((v211 * *(double *)v207.m128i_i64 + v212 * *(double *)&v207.m128i_i64[1]
                                                         + v213 * *(double *)&v220)
                     * v76);
        if ( n191_2 >= 191 )
          n191_2 = 191;
        if ( n191_2 < -64 )
          n191_2 = -64;
        n191_3 = (int32_t)(*(double *)v214
                     * ((v216 * *(double *)&v207.m128i_i64[1] - v210 * *(double *)&v220) * *(double *)v204.m128i_i64
                      + (0.0 - v216 * *(double *)v207.m128i_i64 + *(double *)&v220 * *(double *)&v209)
                      * *(double *)&v208
                      + (v210 * *(double *)v207.m128i_i64 - *(double *)&v209 * *(double *)&v207.m128i_i64[1]) * v221));
        if ( n191_3 >= 191 )
          n191_3 = 191;
        if ( n191_3 < -64 )
          n191_3 = -64;
        if ( (uint32_t)v64 < v227 )
        {
          v202.m128i_i64[0] = __PAIR64__(n191_3, n191_2);
          v224 = v63;
          v80 = 0;
          do
          {
            v81 = v226[4 * v80 + 20];
            v82 = v226[4 * v80 + 16] + v223[4 * v80 + 20];
            v83 = v223[4 * v80 + 16];
            v202.m128i_i32[2] = v80;
            v84 = v81 + v83;
            v85 = v223[4 * v80 + 21];
            v86 = v82 - v84;
            v87 = v223[4 * v80 + 17];
            v202.m128i_i32[3] = v86;
            v88 = v226[4 * v80 + 22];
            v89 = v226[4 * v80 + 17] + v85 - (v226[4 * v80 + 21] + v87);
            v90 = v223[4 * v80 + 22];
            v203.m128i_i32[0] = v89;
            v91 = v226[4 * v80 + 18] + v90 - (v88 + v223[4 * v80 + 18]);
            LOWORD(v89) = v226[4 * v80 + 19] + v223[4 * v80 + 23] - v223[4 * v80 + 19] - v226[4 * v80 + 23];
            v92 = v203.m128i_i16[0];
            LOWORD(v89) = v89 - 512;
            v93 = v203.m128i_i32[0] * v202.m128i_i32[0] + v202.m128i_i32[3] * __sub_405CF0_n191_1;
            v94 = v91 * v202.m128i_i32[1];
            ++v178[((uint16_t)v89 - v202.m128i_i16[6]) & 0x3FF];
            v95 = ((uint16_t)v89 - (uint16_t)((uint32_t)(v93 + v94 + 63) >> 7)) & 0x3FF;
            ++*(uint32_t *)&buf[4 * v95];
            ++v179[((uint16_t)v89 - v92) & 0x3FF];
            LOWORD(v89) = v89 - v91;
            v96 = v202.m128i_i32[2];
            v97 = v223;
            ++v180[v89 & 0x3FF];
            v98 = v97[4 * v96 + 23] + 256;
            v99 = ((uint16_t)v98
                 - (uint16_t)((v202.m128i_i32[0] * v97[4 * v96 + 21]
                                     + __sub_405CF0_n191_1 * v97[4 * v96 + 20]
                                     + v202.m128i_i32[1] * (uint32_t)v97[4 * v96 + 22]
                                     + 63) >> 7)
                 + 256)
                & 0x3FF;
            ++v180[v99 + 1024];
            v100 = v98 - v97[4 * v96 + 20];
            ++v180[v100 + 2048];
            v101 = v98 - v97[4 * v96 + 21];
            ++v180[v101 + 3072];
            v102 = v98 - v97[4 * v96 + 22];
            ++v180[v102 + 4096];
            v80 = v96 + 1;
          }
          while ( v80 < v224 );
          n191_3 = v202.m128i_i32[1];
          n191_2 = v202.m128i_i32[0];
        }
        v103 = __sub_411700((int32_t)buf, 1024);
        v104 = (v103 >> 7) + v103;
        v105 = __sub_411700((int32_t)v178, 1024);
        v106 = v105 < v104;
        if ( v105 < v104 )
          v104 = v105;
        n191_4 = __sub_405CF0_n191_1;
        if ( v106 )
        {
          n191_4 = 128;
          n191_3 = 0;
          n191_2 = 0;
        }
        __sub_405CF0_n191_1 = n191_4;
        v202.m128i_i32[0] = v106;
        v108 = __sub_411700((int32_t)v179, 1024);
        n2_2 = v202.m128i_i32[0];
        if ( v108 < v104 )
        {
          v104 = v108;
          n2_2 = 2;
          n191_2 = 128;
          n191_3 = 0;
          __sub_405CF0_n191_1 = 0;
        }
        v202.m128i_i32[0] = n2_2;
        v110 = __sub_411700((int32_t)v180, 1024);
        __sub_405CF0_n3_1 = v202.m128i_i32[0];
        if ( v110 < v104 )
        {
          __sub_405CF0_n3_1 = 3;
          n191_3 = 128;
          n191_2 = 0;
          __sub_405CF0_n191_1 = 0;
        }
        ::__n191 = __sub_405CF0_n191_1;
        __n191_0 = n191_2;
        ::__n191_1 = n191_3;
        __n3_0 = 3;
        ::__n3_1 = 3;
        v112 = &v180[1024 * __sub_405CF0_n3_1 + 1024];
        n192 = (uint8_t)v112 & 0xF;
        v113 = _mm_cvtsi32_si128(0);
        do
        {
          v113 = _mm_add_epi32(
                   _mm_add_epi32(
                     _mm_add_epi32(_mm_add_epi32(v113, *(__m128i *)&v112[n192]), *(__m128i *)&v112[n192 + 4]),
                     *(__m128i *)&v112[n192 + 8]),
                   *(__m128i *)&v112[n192 + 12]);
          n192 += 16;
        }
        while ( (uint32_t)n192 < 0x100 );
        v114 = _mm_add_epi32(v113, _mm_srli_si128(v113, 8));
        v115 = _mm_cvtsi128_si32(_mm_add_epi32(v114, _mm_srli_si128(v114, 4)));
        v116 = v115;
        for ( k = -1; n192 < 1024; ++n192 )
        {
          v115 = v112[n192] + v115 - v112[n192 - 256];
          if ( v115 >= v116 )
          {
            k = n192;
            v116 = v115;
          }
        }
        __byte_4433CF = k + 1;
      }
    }
  }
  return n192;
}

 BMF_SSE int32_t *__sub_42AB20(char *FileName)
{
  alignas(16) uint8_t __hexrays_frame[128];
  uint32_t &Size_4 = *(uint32_t *)(__hexrays_frame + 0);
  int32_t &v46 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v47 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &Offset_1 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &Size = *(int32_t *)(__hexrays_frame + 4);
  uint32_t &Sizea = *(uint32_t *)(__hexrays_frame + 4);
  char &Sizeb = *(char *)(__hexrays_frame + 4);
  int32_t * &v52 = *(int32_t * *)(__hexrays_frame + 8);
  int32_t &Src_2 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v54 = *(int32_t *)(__hexrays_frame + 12);
  void * &Buffer_3 = *(void * *)(__hexrays_frame + 16);
  char * &Src = *(char * *)(__hexrays_frame + 24);
  uint8_t (&Buffer_2)[4] = *(uint8_t (*)[4])(__hexrays_frame + 32);
  uint32_t (&Buffer_1)[2] = *(uint32_t (*)[2])(__hexrays_frame + 36);
  int32_t &v59 = *(int32_t *)(__hexrays_frame + 44);
  int16_t &v60 = *(int16_t *)(__hexrays_frame + 48);
  uint16_t &n5 = *(uint16_t *)(__hexrays_frame + 50);
  int32_t &n2 = *(int32_t *)(__hexrays_frame + 52);
  int32_t &Size_3 = *(int32_t *)(__hexrays_frame + 68);
  int16_t (&Buffer)[5] = *(int16_t (*)[5])(__hexrays_frame + 76);
  int32_t &Offset = *(int32_t *)(__hexrays_frame + 86);
  ;
  FILE *Stream_v;
  __m128i v17, v18, v19, v20, v21;
  char v10, v25, *Src_4, v28, v30, *Src_3, *Src_6, v39, *Buffer_4, *Src_5;
  int32_t *v3, Size_1, i, v7, v8, v9, Src_1, j_3, Sizea_1, v22, n2_1, v26, v31, Offset_2, v35,
          v38, v40, v41;
  uint32_t Size_2, j_1, j_2, j, n2_2, v29, ElementCount, ElementCount_1, v44;
  Stream_v = fopen(FileName, "rb");
  if ( !Stream_v
    || fread(Buffer, 0xEu, 1u, Stream_v) != 1
    || Buffer[0] != 19778
    || fread(Buffer_1, 0x28u, 1u, Stream_v) != 1
    || Buffer_1[0] != 40
    || v60 != 1 )
  {
    return nullptr;
  }
  v3 = __sub_42B830(Buffer_1[1], v59, n5, n5 <= 8u, 1);
  Size_2 = (*((uint16_t *)v3 + 2) + 3) & 0xFFFFFFFC;
  if ( n5 <= 8u )
  {
    Size_1 = 1 << (n5 & 31);
    if ( Size_3 )
      Size_1 = Size_3;
    if ( Size_1 > 0 )
    {
      Size_4 = (*((uint16_t *)v3 + 2) + 3) & 0xFFFFFFFC;
      Size = Size_1;
      for ( i = 0; i < Size; ++i )
      {
        fread(Buffer_2, 4u, 1u, Stream_v);
        if ( (*((uint8_t *)v3 + 10) & 0x80) != 0 )
          v7 = (int32_t)v3 + v3[3] + 16;
        else
          v7 = 0;
        *(uint8_t *)(v7 + 3 * i + 2) = Buffer_2[2];
        if ( (*((uint8_t *)v3 + 10) & 0x80) != 0 )
          v8 = (int32_t)v3 + v3[3] + 16;
        else
          v8 = 0;
        *(uint8_t *)(v8 + 3 * i + 1) = Buffer_2[1];
        if ( (*((uint8_t *)v3 + 10) & 0x80) != 0 )
          v9 = (int32_t)v3 + v3[3] + 16;
        else
          v9 = 0;
        *(uint8_t *)(v9 + 3 * i) = Buffer_2[0];
      }
      Size_2 = Size_4;
    }
  }
  Buffer_3 = malloc(Size_2);
  Src = (char *)v3 + v3[3] - *((uint16_t *)v3 + 2) + 16;
  fseek(Stream_v, Offset, 0);
  if ( n2 )
  {
    if ( n2 == 1 )
    {
      memset((char *)v3 + 16,0,v3[3]);
      Src_1 = (int32_t)Src;
      v52 = v3;
      v46 = *((uint16_t *)v3 + 1) - 1;
      while ( 1 )
      {
        Src_2 = Src_1;
        if ( ferror(Stream_v) )
          return nullptr;
        j_3 = fgetc(Stream_v);
        Sizea_1 = fgetc(Stream_v);
        Sizea = Sizea_1;
        if ( j_3 )
        {
          j_1 = Src_1 & 0xF;
          if ( (Src_1 & 0xF) != 0 )
            j_1 = 16 - (Src_1 & 0xF);
          if ( j_3 < (int32_t)(j_1 + 16) )
          {
            j_2 = 0;
          }
          else
          {
            j_2 = j_3 - (((uint8_t)j_3 - (uint8_t)j_1) & 0xF);
            if ( j_1 )
            {
              for ( j = 0; j < j_1; ++j )
                *(uint8_t *)(j + Src_1) = Sizea;
            }
            v17 = _mm_cvtsi32_si128((char)Sizea);
            v18 = _mm_unpacklo_epi8(v17, v17);
            v19 = _mm_unpacklo_epi16(v18, v18);
            v20 = _mm_unpacklo_epi32(v19, v19);
            v21 = _mm_unpacklo_epi64(v20, v20);
            do
            {
              *(__m128i *)(Src_1 + j_1) = v21;
              j_1 += 16;
            }
            while ( j_1 < j_2 );
            Src_1 += j_1;
          }
          for ( ; j_2 < j_3; ++j_2 )
          {
            *(uint8_t *)(Src_2 + j_2) = Sizea;
            Src_1 = j_2 + Src_2 + 1;
          }
        }
        else if ( Sizea_1 )
        {
          if ( Sizea_1 == 1 )
            goto LABEL_61;
          if ( Sizea_1 == 2 )
          {
            v38 = fgetc(Stream_v);
            Src_1 = v38 + Src_1 - fgetc(Stream_v) * *((uint16_t *)v52 + 2);
          }
          else
          {
            fread(Buffer_3, (Sizea_1 + 1) & 0xFFFFFFFE, 1u, Stream_v);
            memcpy((char *)Src_1,(char *)Buffer_3,Sizea);
            Src_1 += Sizea;
          }
        }
        else
        {
          if ( --v46 < 0 )
            goto LABEL_61;
          Src_1 = (int32_t)v52 + v46 * *((uint16_t *)v52 + 2) + 16;
        }
      }
    }
    if ( n2 != 2 )
      return nullptr;
    memset((char *)v3 + 16,0,v3[3]);
    v52 = v3;
    v22 = 1;
    v47 = *((uint16_t *)v3 + 1) - 1;
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
LABEL_44:
          if ( ferror(Stream_v) )
            return nullptr;
          v54 = fgetc(Stream_v);
          n2_1 = fgetc(Stream_v);
          n2_2 = n2_1;
          if ( !v54 )
            break;
          v25 = n2_1 & 0xF;
          if ( v22 )
          {
            v31 = v54;
            Src_3 = Src;
            while ( v31 != 1 )
            {
              *Src_3++ = n2_2;
              v31 -= 2;
              if ( !v31 )
              {
                Src = Src_3;
                v22 = 1;
                goto LABEL_44;
              }
            }
            Src = Src_3;
            *Src_3 = n2_2 & 0xF0;
            v22 = 0;
          }
          else
          {
            v26 = v54;
            Src_4 = Src;
            v28 = *Src;
            v29 = n2_2 >> 4;
            v30 = 16 * v25;
            while ( 1 )
            {
              *Src_4++ = v29 | v28;
              if ( v26 == 1 )
                break;
              v28 = v30;
              v26 -= 2;
              if ( !v26 )
              {
                Src = Src_4;
                *Src_4 = v30;
                v22 = 0;
                goto LABEL_44;
              }
            }
            Src = Src_4;
            v22 = 1;
          }
        }
        if ( n2_1 )
          break;
        if ( --v47 < 0 )
          goto LABEL_61;
        Src = (char *)v52 + v47 * *((uint16_t *)v52 + 2) + 16;
      }
      if ( n2_1 == 1 )
        goto LABEL_61;
      if ( n2_1 != 2 )
        break;
      v40 = fgetc(Stream_v);
      v41 = (v40 >> 1) - fgetc(Stream_v) * *((uint16_t *)v52 + 2);
      if ( (v40 & 1) == 1 )
      {
        if ( !v22 )
          ++v41;
        v22 = !v22;
      }
      Src += v41;
    }
    fread(Buffer_3, (((n2_1 + 1) >> 1) + 1) & 0xFFFFFFFE, 1u, Stream_v);
    Buffer_4 = (char *)Buffer_3;
    Src_5 = Src;
    while ( 1 )
    {
      Sizeb = *Buffer_4;
      if ( v22 )
      {
        v44 = n2_2 - 1;
        if ( !v44 )
        {
          Src = Src_5;
          *Src_5 = *Buffer_4 & 0xF0;
          v22 = 0;
          goto LABEL_44;
        }
        *Src_5++ = Sizeb;
        v22 = 1;
      }
      else
      {
        *Src_5++ |= (uint8_t)*Buffer_4 >> 4;
        v44 = n2_2 - 1;
        if ( !v44 )
        {
          Src = Src_5;
          v22 = 1;
          goto LABEL_44;
        }
        *Src_5 = 16 * (Sizeb & 0xF);
        v22 = 0;
      }
      ++Buffer_4;
      n2_2 = v44 - 1;
      if ( !n2_2 )
      {
        Src = Src_5;
        goto LABEL_44;
      }
    }
  }
  ElementCount = *((uint16_t *)v3 + 2);
  Offset_2 = Size_2 - ElementCount;
  if ( v59 - 1 >= 0 )
  {
    Offset_1 = Offset_2;
    v35 = v59 - 1;
    v52 = v3;
    Src_6 = Src;
    while ( 1 )
    {
      ElementCount_1 = fread(Src_6, 1u, ElementCount, Stream_v);
      ElementCount = *((uint16_t *)v52 + 2);
      if ( ElementCount_1 != ElementCount )
        return nullptr;
      if ( Offset_1 )
      {
        fseek(Stream_v, Offset_1, 1);
        ElementCount = *((uint16_t *)v52 + 2);
      }
      Src_6 -= ElementCount;
      if ( --v35 < 0 )
      {
LABEL_61:
        v3 = v52;
        break;
      }
    }
  }
  fclose(Stream_v);
  free(Buffer_3);
  return v3;
} int32_t __sub_412E60(uint32_t *a1)
{
  alignas(16) uint8_t __hexrays_frame[32824];
  uint16_t * &v55 = *(uint16_t * *)(__hexrays_frame + 0);
  uint16_t * &v56 = *(uint16_t * *)(__hexrays_frame + 4);
  int32_t &v57 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v58 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v59 = *(int32_t *)(__hexrays_frame + 16);
  uint32_t &n0x800000_1 = *(uint32_t *)(__hexrays_frame + 20);
  int32_t &v61 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v62 = *(int32_t *)(__hexrays_frame + 28);
  uint32_t &n0x7F800000_1 = *(uint32_t *)(__hexrays_frame + 32768);
  int32_t &__sub_412E60_n0x2000 = *(int32_t *)(__hexrays_frame + 32772);
  int32_t &v65 = *(int32_t *)(__hexrays_frame + 32776);
  uint16_t * &v66 = *(uint16_t * *)(__hexrays_frame + 32780);
  uint32_t * &v67 = *(uint32_t * *)(__hexrays_frame + 32784);
  uint32_t * &v68 = *(uint32_t * *)(__hexrays_frame + 32788);
  ;
  char v23, v34, *v36, *v38, v40;
  int16_t v39;
  int32_t sym_cum, sym_high, v2, v3, v5, v6, v7, v8, __sub_412E60_n0x7F800000, n0x7F800000_2,
          v16, v17, n0x2000_5, n0x2000_2, v28, n0x800000_3, n0x800000_2, n251, v46, v49, v51,
          v53;
  uint16_t **v4, *v20, **v21, *v25, **v26, *v33, v35, *v43, *v44, *v45, *v47, *v48, *v52;
  uint32_t *v9, __sub_412E60_n0x800000, n0x2000_6, v13, n0x2000_4, n0x2000_3,
           __sub_412E60_n0x2000_1, *v32, v41, v42, v50, v54;
  uint8_t *Buffer;
  void *v1;
  v68 = a1;
  v1 = alloca(32788);
  v2 = a1[1];
  v3 = a1[5];
  v4 = &v55;
  v68 = (uint32_t *)(uint8_t)__byte_445700;
  v67 = a1;
  v5 = 0;
  v6 = 0;
  do
  {
    if ( (uint32_t *)(uint8_t)__buf_0[*(uint16_t *)(v3 + 3 * v6)] == v68 )
    {
      v8 = 0;
    }
    else
    {
      v7 = v3 + 3 * v6;
      v8 = *(uint8_t *)(v7 + 2);
      *v4++ = (uint16_t *)v7;
    }
    v5 += v8;
    ++v6;
  }
  while ( v6 < v2 );
  v9 = v67;
  if ( !v5 )
    return -1;
  *v4 = nullptr;
  v65 = v9[2];
  n0x2000_6 = v5 + v65;
  n0x2000_4 = rc.get_freq(n0x2000_6);
  __sub_412E60_n0x2000 = v5 + v65;
  v67 = v9;
  n0x2000_5 = v5 + v65 - 1;
  v20 = v55;
  v21 = &v56;
  v66 = v55;
  n0x2000_2 = 0;
  while ( 1 )
  {
    n0x2000_2 += *((uint8_t *)v20 + 2);
    if ( n0x2000_2 > (int32_t)n0x2000_4 )
      break;
    v20 = *v21++;
    if ( !v20 )
    {
      v23 = (char)v68;
      sym_cum = n0x2000_2;
      n0x2000_3 = __sub_412E60_n0x2000;
      sym_high = __sub_412E60_n0x2000;
      v25 = v66;
      v26 = &v56;
      do
      {
        __buf_0[*v25] = v23;
        v25 = *v26++;
      }
      while ( v25 );
      __sub_412E60_n0x2000_1 = __sub_412E60_n0x2000;
      v62 = -1;
      goto LABEL_19;
    }
  }
  v32 = v67;
  sym_high = n0x2000_2;
  sym_cum = n0x2000_2 - *((uint8_t *)v20 + 2);
  v62 = *v20;
  *((uint8_t *)v20 + 2) += 4;
  v33 = (uint16_t *)v32[5];
  v32[3] += 4;
  if ( v20 == v33 )
  {
    n251 = *((uint8_t *)v20 + 2);
  }
  else
  {
    v34 = *((uint8_t *)v20 + 2);
    v35 = *v20;
    v36 = (char *)v20 - 3;
    *v20 = *(uint16_t *)((char *)v20 - 3);
    *((uint8_t *)v20 + 2) = *((uint8_t *)v20 - 1);
    *(uint16_t *)v36 = v35;
    v36[2] = v34;
    v33 = (uint16_t *)v32[5];
    if ( (uint16_t *)((char *)v20 - 3) == v33 )
    {
      n251 = *((uint8_t *)v20 - 1);
    }
    else
    {
      n0x800000_1 = __sub_412E60_n0x800000;
      while ( 1 )
      {
        n251 = (uint8_t)v36[2];
        v38 = v36 - 3;
        if ( n251 <= (uint8_t)*(v36 - 1) )
          break;
        v39 = *(uint16_t *)v36;
        v40 = v36[2];
        *(uint16_t *)v36 = *(uint16_t *)v38;
        v36[2] = *(v36 - 1);
        *(uint16_t *)v38 = v39;
        *(v36 - 1) = v40;
        v33 = (uint16_t *)v32[5];
        v36 -= 3;
        if ( v38 == (char *)v33 )
        {
          __sub_412E60_n0x800000 = n0x800000_1;
          n251 = (uint8_t)v38[2];
          goto LABEL_30;
        }
      }
      __sub_412E60_n0x800000 = n0x800000_1;
    }
  }
LABEL_30:
  v41 = v32[4];
  if ( n251 > 251 || v41 < v32[3] )
  {
    v55 = (uint16_t *)v32[1];
    v42 = 20 * *v32;
    v43 = v55;
    n0x800000_1 = __sub_412E60_n0x800000;
    v59 = v41 < v42;
    v44 = (uint16_t *)((char *)v33 - 3);
    do
    {
      v45 = v44;
      v44 = (uint16_t *)((char *)v44 + 3);
      v46 = (v59 + (uint32_t)*((uint8_t *)v44 + 2)) >> 1;
      *((uint8_t *)v44 + 2) = v46;
      if ( v44 != (uint16_t *)v32[5] )
      {
        v47 = (uint16_t *)((char *)v44 - 3);
        v58 = *((uint8_t *)v44 - 1);
        if ( v46 > v58 )
        {
          v57 = *v44;
          *v44 = *v47;
          *((uint8_t *)v44 + 2) = v58;
          if ( v47 != (uint16_t *)v32[5] )
          {
            v56 = v44;
            v55 = v43;
            do
            {
              v48 = (uint16_t *)((char *)v47 - 3);
              v49 = *((uint8_t *)v47 - 1);
              if ( v46 <= v49 )
                break;
              *v47 = *v48;
              *((uint8_t *)v47 + 2) = v49;
              v47 = (uint16_t *)((char *)v47 - 3);
            }
            while ( v48 != (uint16_t *)v32[5] );
            v44 = v56;
            v43 = v55;
          }
          *v47 = v57;
          *((uint8_t *)v47 + 2) = v46;
        }
      }
      v43 = (uint16_t *)((uintptr_t)v43 - (1));
    }
    while ( v43 );
    __sub_412E60_n0x800000 = n0x800000_1;
    v50 = v32[2];
    v51 = *((uint8_t *)v44 + 2);
    v55 = nullptr;
    if ( !v51 )
    {
      v52 = v55;
      do
      {
        v52 = (uint16_t *)((char *)v52 + 1);
        v32[2] = ++v50;
        v53 = *((uint8_t *)v45 + 2);
        v45 = (uint16_t *)((char *)v45 - 3);
      }
      while ( !v53 );
      v55 = v52;
      v32[1] -= (uint32_t)v52;
    }
    v54 = v32[3];
    v32[2] = v50 - (v50 >> 1);
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
    __sub_412E60_n0x2000_1 = n0x2000_6;
    v32[3] = v54 - (v54 >> 1);
  }
  else
  {
    __sub_412E60_n0x2000_1 = n0x2000_6;
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
  }
LABEL_19:
  rc.decode(n0x2000_2, n0x2000_3, __sub_412E60_n0x2000_1);
  return v62;
}
static inline int32_t __fwd_sub_418650_sub_412850(void *a0, void *a1) { return __sub_412850((uint16_t *)a0, (uint16_t *)a1); }
static inline int32_t __fwd_sub_418650_sub_412E60(void *a0) { return __sub_412E60((uint32_t *)a0); }
static inline int32_t __fwd_sub_418650_sub_4148F0(void *a0) { return __sub_4148F0((uint16_t *)a0); }
static inline int32_t __fwd_sub_418650_sub_416860(void *a0, void *a1) { return __sub_416860((int32_t *)a0, (uint32_t *)a1); }

int32_t __sub_418650(uint32_t *_this, int32_t a2)
{
  alignas(16) uint8_t __hexrays_frame[164];
  int32_t &n15_8 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &n4_1 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &__sub_418650_n4_4 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n15_3 = *(int32_t *)(__hexrays_frame + 12);
  uint16_t * &v184 = *(uint16_t * *)(__hexrays_frame + 16);
  uint32_t * &this_1 = *(uint32_t * *)(__hexrays_frame + 20);
  int32_t &v186 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v187 = *(int32_t *)(__hexrays_frame + 28);
  int32_t &v188 = *(int32_t *)(__hexrays_frame + 32);
  int32_t &v189 = *(int32_t *)(__hexrays_frame + 36);
  int32_t &v190 = *(int32_t *)(__hexrays_frame + 40);
  int32_t &v191 = *(int32_t *)(__hexrays_frame + 44);
  int32_t &v192 = *(int32_t *)(__hexrays_frame + 48);
  int32_t &v193 = *(int32_t *)(__hexrays_frame + 52);
  int32_t &v194 = *(int32_t *)(__hexrays_frame + 56);
  int32_t &v195 = *(int32_t *)(__hexrays_frame + 60);
  int32_t &v196 = *(int32_t *)(__hexrays_frame + 64);
  int32_t &v197 = *(int32_t *)(__hexrays_frame + 68);
  int32_t &v198 = *(int32_t *)(__hexrays_frame + 72);
  int32_t &v199 = *(int32_t *)(__hexrays_frame + 76);
  int32_t &v200 = *(int32_t *)(__hexrays_frame + 80);
  int32_t &v201 = *(int32_t *)(__hexrays_frame + 84);
  int32_t &v202 = *(int32_t *)(__hexrays_frame + 88);
  int32_t &v203 = *(int32_t *)(__hexrays_frame + 92);
  int32_t &v204 = *(int32_t *)(__hexrays_frame + 96);
  int32_t &v205 = *(int32_t *)(__hexrays_frame + 100);
  int32_t &v206 = *(int32_t *)(__hexrays_frame + 104);
  int32_t &v207 = *(int32_t *)(__hexrays_frame + 108);
  int32_t &v208 = *(int32_t *)(__hexrays_frame + 112);
  int32_t &v209 = *(int32_t *)(__hexrays_frame + 116);
  int32_t &v210 = *(int32_t *)(__hexrays_frame + 120);
  int32_t &v211 = *(int32_t *)(__hexrays_frame + 124);
  int32_t &n15_24 = *(int32_t *)(__hexrays_frame + 128);
  ;
  bool v19;
  char v70, v71, v72, v73, v74, v75, v91, *v157;
  int16_t v14, n4_14, v146, v160, n15_4;
  int32_t arg_cum, arg_high, arg_tot, n4_8, n4_7, n15_6, n15_7, v8, v9, v10, v12, v13, v15, n4_9,
          __sub_418650_n15, v21, v22, v23, n4_11, v25, v26, v27, v29, n15_10, v31, n0xFFFF, v33,
          n0xFFFF_1, n53248, v36, n4_12, v40, n15_11, n8, n15_12, v44, n4_22, v46, n15_14,
          n15_18, v49, v50, v51, n4_13, v53, v54, v55, v56, v57, *v59, v60, v61, n15_13, v66,
          v67, n15_15, n15_17, n4_17, v80, v81, v83, v84, v85, n4, n256_2, n15_1, n256_1, n15_23,
          n4_19, n4_20, n4_5, n4_6, n15_5, this_4, v102, v103, v104, v105, v107, v108, v109,
          v111, v112, v113, v114, v115, v116, v117, v118, v119, v120, v122, v123, v124, v125,
          v126, v127, v128, n32, n15_25, n4_21, v132, v133, v135, n256, v143, n4_18, n15_19,
          n15_20, v148, v149, v150, v151, v152, v153, v155, n15_21, v158, v159, v161, v163, v164,
          n4_2, n15_22, n256_4, n256_5, n256_3, n15_2;
  uint16_t *n15_9, *v4, *v16, *n4_10, *v58, *__sub_418650_n4_3, *v90, *v95, *v97, *v100, *v106,
           *v110, *v121, v154, v162, *v170, *v171, v174;
  uint32_t bin_tot, *this_2, *this_3, n4_16, n4_15, **v134, v136, v137, v138, v139, v140, v172,
           v173, v175, v176, v177;
  uint64_t *v142;
  uint8_t *v38, *v39, *n15_16, *v76;
  n15_9 = (uint16_t *)*(_this + 20);
  n4_8 = *n15_9;
  v4 = (uint16_t *)*(_this + 19);
  n4_7 = *(v4 - 4);
  n15_6 = n15_9[4];
  this_1 = _this;
  n15_7 = *(n15_9 - 4);
  n15_8 = n4_8;
  ::__n4_4 = n4_8;
  n4_1 = n4_7;
  ::__n4_3 = n4_7;
  n15_3 = n15_6;
  ::__n15 = n15_6;
  v8 = *((uint8_t *)n15_9 + 3) + 4 * (n15_6 == n15_7);
  v9 = *((uint8_t *)n15_9 + 11);
  __sub_418650_n4_4 = n15_7;
  __n15_0 = n15_7;
  v10 = 2 * *((uint8_t *)v4 - 6) + 8 * v9 + v8;
  this_2 = this_1;
  v12 = 32 * *((uint8_t *)v4 - 4) + 16 * *((uint8_t *)v4 - 2) + v10;
  if ( n4_8 == n4_7 )
  {
    if ( n15_3 == n15_8 )
    {
      v14 = *((uint16_t *)this_1 + n4_8 + 3029720);
      if ( n4_8 == __sub_418650_n4_4 )
        v13 = (uint16_t)(v14 - *(v4 - 8));
      else
        v13 = (uint16_t)(v14 - __sub_418650_n4_4);
    }
    else
    {
      v13 = (uint16_t)(*((uint16_t *)this_1 + n4_8 + 3029720) - n15_3);
    }
  }
  else
  {
    v13 = (uint16_t)(*((uint16_t *)this_1 + n4_8 + 3029720) - n4_1);
  }
  this_1[1514858] = &this_1[4 * v13 + 269674];
  v15 = *((uint8_t *)this_2 + v12 + 1078244);
  this_2[9] = v15;
  v16 = (uint16_t *)&this_2[0x10000 * v15 + 531818 + v13];
  this_2[1514859] = v16;
  n4_9 = *v16;
  if ( n4_9 == n15_8 )
  {
    __sub_418650_n15 = 15;
  }
  else if ( n4_9 == n4_1 )
  {
    __sub_418650_n15 = 30;
  }
  else if ( n4_9 == n15_3 )
  {
    __sub_418650_n15 = 45;
  }
  else
  {
    v19 = n4_9 == __sub_418650_n4_4;
    __sub_418650_n15 = 60;
    if ( !v19 )
      __sub_418650_n15 = 0;
  }
  n4_10 = (uint16_t *)v16[1];
  if ( n4_10 == (uint16_t *)n15_8 )
  {
    __sub_418650_n15 += 75;
  }
  else if ( n4_10 == (uint16_t *)n4_1 )
  {
    __sub_418650_n15 += 150;
  }
  else if ( n4_10 == (uint16_t *)n15_3 )
  {
    __sub_418650_n15 += 225;
  }
  else if ( n4_10 == (uint16_t *)__sub_418650_n4_4 )
  {
    __sub_418650_n15 += 300;
  }
  v21 = this_2[22];
  n15_3 = (int32_t)n15_9;
  v22 = *((uint8_t *)this_2 + v15 + __sub_418650_n15 + 1078308);
  v23 = this_2[21];
  this_2[10] = v22;
  n4_11 = *((uint8_t *)n15_9 + 2);
  v184 = v4;
  this_1 = this_2;
  __sub_418650_n4_4 = n4_11;
  v25 = this_2[23];
  v26 = *((uint8_t *)v4 - 5);
  v186 = v25;
  v27 = 8 * *((uint8_t *)v4 - 12) + 4 * *((uint8_t *)v4 - 9) + v26 + 2 * *((uint8_t *)v4 - 10);
  this_3 = this_1;
  v29 = ((uint8_t)(*(uint8_t *)(v186 + 2) & *(uint8_t *)(v21 + 2) & __sub_418650_n4_4 & *(uint8_t *)(v23 + 2)) << 9)
      + ((uint8_t)(*(uint8_t *)(v186 + 3) & *(uint8_t *)(v21 + 3) & *(uint8_t *)(v23 + 3) & *((uint8_t *)n15_9 + 3)) << 8)
      + (v22 << 10)
      + v27;
  n15_10 = n15_3;
  v31 = ((*((uint8_t *)this_1 + 1078695) == 0) << 7)
      + ((*((uint8_t *)this_1 + 1078694) == 0) << 6)
      + 32 * (*((uint8_t *)this_1 + 1078693) == 0)
      + 16 * (*((uint8_t *)this_1 + 1078692) == 0)
      + v29;
  n0xFFFF = *((uint16_t *)this_1 + v31 + 3037912);
  if ( n0xFFFF == 0xFFFF )
  {
    *((uint16_t *)this_1 + v31 + 3037912) = this_1[5];
    n15_10 = this_3[20];
    v4 = (uint16_t *)this_3[19];
    ++this_3[5];
    n0xFFFF = *((uint16_t *)this_3 + v31 + 3037912);
    __sub_418650_n4_4 = *(uint8_t *)(n15_10 + 2);
  }
  v33 = *((uint8_t *)v4 - 1) + 4 * *(uint8_t *)(n15_10 + 13) + 2 * __sub_418650_n4_4 + 8 * n0xFFFF;
  n0xFFFF_1 = *((uint16_t *)this_3 + v33 + 3230424);
  if ( n0xFFFF_1 == 0xFFFF )
  {
    *((uint16_t *)this_3 + v33 + 3230424) = this_3[6]++;
    n0xFFFF_1 = *((uint16_t *)this_3 + v33 + 3230424);
  }
  if ( (int32_t)this_3[4] < 32 )
  {
    n53248 = this_3[7];
    n4_1 = 16 * n0xFFFF_1 + (n4_1 & 0xF);
    v36 = (int32_t)this_3 + 2 * n4_1;
    n0xFFFF_1 = *(uint16_t *)(v36 + 6678448);
    if ( n0xFFFF_1 == 0xFFFF )
    {
      n4_12 = n4_1;
      if ( n53248 > 53248 )
        n4_12 = n4_1 | 0xF;
      v36 = (int32_t)this_3 + 2 * n4_12;
      n0xFFFF_1 = *(uint16_t *)(v36 + 6678448);
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *(uint16_t *)(v36 + 6678448) = n53248;
      ++this_3[7];
      n0xFFFF_1 = *(uint16_t *)(v36 + 6678448);
    }
  }
  if ( (*(uint8_t *)(this_3[19] - 5) & *(uint8_t *)(this_3[19] - 6)) != 0 )
  {
    v38 = (uint8_t *)this_3[20];
    v39 = (uint8_t *)this_3[21];
    if ( ((uint8_t)(v39[19] & v39[11] & v39[3] & v38[27] & v38[19] & v38[11] & v38[3] & v38[2] & *(v38 - 5))
        & v39[27]) != 0 )
    {
      v40 = v39[2];
      n15_11 = 1;
      if ( *this_3 - a2 <= 1 )
      {
        n8 = 8;
      }
      else
      {
        n4_1 = *this_3 - a2;
        this_1 = this_3;
        while ( 1 )
        {
          n8 = 8 * n15_11;
          if ( (v38[8 * n15_11 + 19] & v38[8 * n15_11 + 18]) == 0 )
            break;
          v40 = (uint8_t)(v39[n8 + 2] & v40);
          if ( ++n15_11 >= n4_1 )
          {
            this_3 = this_1;
            n8 = 8 * n15_11;
            goto LABEL_42;
          }
        }
        this_3 = this_1;
      }
LABEL_42:
      n15_12 = *(uint8_t *)(this_3[269671] + n15_11);
      v44 = __fwd_sub_418650_sub_412850(
              (uint16_t *)&this_3[12 * n15_12
                                        + 269089
                                        + 6 * (uint8_t)(v39[n8 + 27] & v39[n8 + 19])
                                        + 3 * v40]
            + 3 * *(uint8_t *)(this_3[269672] + n15_8)
            + 1,
              (uint16_t *)this_3 + 538176);
      n4_22 = ::__n4_4;
      v46 = this_3[269672];
      this_3[8] = v44;
      *(uint8_t *)(v46 + n4_22) = v44;
      n15_14 = this_3[8];
      if ( n15_14 )
      {
        n15_18 = n15_11;
      }
      else
      {
        n15_18 = 0;
        if ( n15_11 == 1 )
          goto LABEL_57;
        n15_3 = n15_11;
        n15_8 = n15_12;
        this_1 = this_3;
        n15_18 = 0;
        v49 = 1 << (n15_12 & 31);
        v50 = 0;
        do
        {
          if ( (v49 | v50) < n15_3 )
          {
            v51 = *((uint16_t *)&this_1[24 * (v50 == 0) + 269473 + 24 * (n15_12 == n15_8)] + 3 * n15_12 + 1);
            __sub_418650_n4_4 = (int32_t)&this_1[24 * (v50 == 0) + 269473 + 24 * (n15_12 == n15_8)] + 6 * n15_12 + 2;
            bin_tot = v51 + *((uint16_t *)&this_1[24 * (v50 == 0) + 269474 + 24 * (n15_12 == n15_8)] + 3 * n15_12);
            n4_1 = rc.decode_bit(
                     v51,
                     *((uint16_t *)&this_1[24 * (v50 == 0) + 269474 + 24 * (n15_12 == n15_8)] + 3 * n15_12));
            if ( *((uint16_t *)&this_1[24 * (v50 == 0) + 269474 + 24 * (n15_12 == n15_8)] + 3 * n15_12 + 1) < (uint32_t)bin_tot )
              __fwd_sub_418650_sub_4148F0((uint16_t *)__sub_418650_n4_4);
            n4_13 = n4_1;
            *(uint16_t *)(__sub_418650_n4_4 + 2 * n4_1) += 8;
            if ( n4_13 )
              n15_18 |= v49;
            v50 |= n15_18 & v49;
          }
          --n15_12;
          v49 >>= 1;
        }
        while ( v49 );
        this_3 = this_1;
      }
      if ( n15_18 )
        *(uint16_t *)(this_3[19] + 8 * n15_18 - 8) = *(uint16_t *)(this_3[19] - 8);
      n15_14 = this_3[8];
LABEL_57:
      if ( n15_18 > n15_14 )
      {
        v53 = this_3[21];
        this_3[20] = this_3[20] + 8 * n15_18 - 8 * n15_14;
        v54 = this_3[22];
        v55 = v53 + 8 * n15_18;
        v56 = this_3[23];
        this_3[21] = v55 - 8 * n15_14;
        this_3[22] = v54 + 8 * n15_18 - 8 * n15_14;
        v57 = this_3[19];
        this_3[23] = v56 + 8 * n15_18 - 8 * n15_14;
        *(uint32_t *)(v57 + 4) = 16843009;
        *(uint32_t *)this_3[19] = 16843009;
        v58 = (uint16_t *)this_3[1514859];
        LOWORD(v55) = ::__n4_4;
        LOWORD(v54) = *v58;
        n4_1 = ::__n4_4;
        v58[1] = v54;
        *(uint16_t *)this_3[19] = v55;
        *(uint16_t *)this_3[1514859] = v55;
        v59 = (int32_t *)this_3[19];
        v60 = v59[1];
        n15_3 = *v59;
        v61 = (int32_t)(v59 + 2);
        this_3[19] = v61;
        if ( n15_18 - n15_14 != 1 )
        {
          __sub_418650_n4_4 = (n15_18 - n15_14 - 1) / 2;
          if ( __sub_418650_n4_4 )
          {
            n15_13 = n15_3;
            n15_8 = n15_14;
            n4_14 = n4_1;
            n4_16 = 0;
            n15_24 = n15_18;
            n4_15 = __sub_418650_n4_4;
            do
            {
              *(uint16_t *)(this_3[1514859] + 2) = n4_14;
              *(uint32_t *)this_3[19] = n15_13;
              *(uint32_t *)(this_3[19] + 4) = v60;
              v66 = this_3[1514859];
              this_3[19] += 8;
              *(uint16_t *)(v66 + 2) = n4_14;
              *(uint32_t *)this_3[19] = n15_13;
              *(uint32_t *)(this_3[19] + 4) = v60;
              v61 = this_3[19] + 8;
              this_3[19] = v61;
              ++n4_16;
            }
            while ( n4_16 < n4_15 );
            n15_14 = n15_8;
            n15_18 = n15_24;
            v67 = 2 * n4_16 + 1;
          }
          else
          {
            v67 = 1;
          }
          if ( n15_18 - n15_14 - 1 > (uint32_t)(v67 - 1) )
          {
            n15_15 = n15_3;
            *(uint16_t *)(this_3[1514859] + 2) = n4_1;
            *(uint32_t *)this_3[19] = n15_15;
            *(uint32_t *)(this_3[19] + 4) = v60;
            v61 = this_3[19] + 8;
            this_3[19] = v61;
          }
        }
        n15_24 = n15_18;
        n15_16 = (uint8_t *)this_3[20];
        v70 = *(n15_16 - 22);
        v71 = *(n15_16 - 14);
        v72 = n15_16[18];
        v73 = n15_16[26];
        n15_8 = (int32_t)n15_16;
        v74 = v71 + v70;
        v75 = n15_16[34];
        v76 = (uint8_t *)this_3[21];
        *((uint8_t *)this_3 + 1078692) = v73 + v72 + v74 + v75 - 5;
        *((uint8_t *)this_3 + 1078693) = v76[26]
                                     + v76[18]
                                     + v76[10]
                                     + v76[2]
                                     + *(v76 - 6)
                                     + *(v76 - 14)
                                     + *(v76 - 22)
                                     + v76[34]
                                     - 8;
        n15_17 = n15_8;
        *((uint8_t *)this_3 + 1078694) = *(uint8_t *)(v61 - 29) + *(uint8_t *)(v61 - 21) - 2;
        n4_17 = n4_1;
        *((uint8_t *)this_3 + 1078695) = *(uint8_t *)(v61 - 38)
                                     + *(uint8_t *)(v61 - 46)
                                     + *(uint8_t *)(v61 - 54)
                                     + *(uint8_t *)(v61 - 30)
                                     - 4;
        *(uint8_t *)(v61 - 2) = n4_17 == *(uint16_t *)(n15_17 + 8);
        n15_18 = n15_24;
        *(uint8_t *)(this_3[19] - 1) = n4_17 == *(uint16_t *)(this_3[20] + 16);
        n15_14 = this_3[8];
      }
      if ( n15_14 )
        return n15_18;
      goto LABEL_86;
    }
  }
  v80 = (int32_t)&this_3[4 * this_3[10] + 24];
  v184 = (uint16_t *)v80;
  v81 = 4 * n0xFFFF_1;
  __sub_418650_n4_3 = (uint16_t *)&this_3[v81 + 776];
  v83 = HIWORD(this_3[v81 + 778]);
  if ( HIWORD(this_3[v81 + 778]) )
  {
    if ( v83 == 1 )
    {
      v142 = (uint64_t *)v80;
      v143 = *(uint8_t *)(v80 + 15);
      n4_18 = v143 * LOWORD(this_3[v81 + 777]);
      n15_19 = v143 * __sub_418650_n4_3[3];
      n4_1 = v143 * *__sub_418650_n4_3;
      v146 = v143 * __sub_418650_n4_3[1];
      __sub_418650_n4_4 = n4_18;
      n15_20 = v143 * __sub_418650_n4_3[4];
      n15_8 = n15_19;
      n15_3 = n15_20;
      *(uint64_t *)__sub_418650_n4_3 = *v142;
      *((uint64_t *)__sub_418650_n4_3 + 1) = v142[1];
      v148 = __sub_418650_n4_3[5];
      v149 = *__sub_418650_n4_3;
      *((uint8_t *)__sub_418650_n4_3 + 14) *= 8;
      this_1 = this_3;
      v150 = 21 * __sub_418650_n4_3[1];
      n4_1 += (21 * v149 + v148 - 1) / v148;
      *__sub_418650_n4_3 = n4_1;
      v151 = (v150 + v148 - 1) / v148;
      v152 = 21 * __sub_418650_n4_3[2];
      v153 = __sub_418650_n4_3[3];
      v154 = v151 + v146;
      __sub_418650_n4_3[1] = v154;
      v155 = 21 * v153;
      n15_21 = n15_8;
      v157 = (char *)((v152 + v148 - 1) / v148 + __sub_418650_n4_4);
      __sub_418650_n4_3[2] = (uint16_t)v157;
      v158 = (v155 + v148 - 1) / v148 + n15_21;
      v159 = 21 * __sub_418650_n4_3[4];
      __sub_418650_n4_3[3] = v158;
      v160 = v158 + (uint16_t)v157 + v154;
      this_3 = this_1;
      v161 = (v159 + v148 - 1) / v148 + n15_3;
      __sub_418650_n4_3[4] = v161;
      v162 = n4_1 + v161 + v160;
      v83 = v162;
      __sub_418650_n4_3[5] = v162;
    }
    arg_tot = v83;
    v84 = rc.get_freq(arg_tot);
    v85 = *__sub_418650_n4_3;
    if ( v85 <= v84 )
    {
      v85 += __sub_418650_n4_3[1];
      if ( v85 <= v84 )
      {
        v85 += __sub_418650_n4_3[2];
        if ( v85 <= v84 )
        {
          v85 += __sub_418650_n4_3[3];
          if ( v85 <= v84 )
          {
            v85 += __sub_418650_n4_3[4];
            n4 = 4;
          }
          else
          {
            n4 = 3;
          }
        }
        else
        {
          n4 = 2;
        }
      }
      else
      {
        n4 = 1;
      }
    }
    else
    {
      n4 = 0;
    }
    n256_2 = __sub_418650_n4_3[6];
    arg_high = v85;
    n15_1 = *((uint8_t *)__sub_418650_n4_3 + 15);
    arg_cum = v85 - __sub_418650_n4_3[n4];
    n256_1 = __sub_418650_n4_3[5];
    if ( n256_1 > n256_2 && (__sub_418650_n4_3[n4] + n15_1 + 8 < n256_1 || __sub_418650_n4_3[5] > 0x4000u) )
    {
      v136 = __sub_418650_n4_3[2];
      n15_8 = n256_2;
      v137 = __sub_418650_n4_3[1];
      this_1 = this_3;
      v138 = *__sub_418650_n4_3;
      n4_1 = n4;
      __sub_418650_n4_4 = n15_1;
      LOWORD(v138) = v138 - (v138 >> 1);
      *__sub_418650_n4_3 = v138;
      LOWORD(v137) = v137 - (v137 >> 1);
      __sub_418650_n4_3[1] = v137;
      LOWORD(v136) = v136 - (v136 >> 1);
      v139 = __sub_418650_n4_3[3];
      __sub_418650_n4_3[2] = v136;
      LOWORD(v139) = v139 - (v139 >> 1);
      v140 = __sub_418650_n4_3[4];
      __sub_418650_n4_3[3] = v139;
      LOWORD(v140) = v140 - (v140 >> 1);
      __sub_418650_n4_3[4] = v140;
      LOWORD(v137) = v139 + v136 + v137;
      n15_1 = __sub_418650_n4_4;
      LOWORD(v140) = v138 + v140;
      this_3 = this_1;
      n256_1 = (uint16_t)(v140 + v137);
      n256 = n15_8;
      n4 = n4_1;
      __sub_418650_n4_3[5] = n256_1;
      if ( n256 < 256 && !*((uint8_t *)__sub_418650_n4_3 + 14) )
      {
        n256 = 256;
        __sub_418650_n4_3[6] = 256;
      }
      if ( n256_1 > n256 )
      {
        if ( n15_1 < 15 )
          LOWORD(n15_1) = 15;
        *((uint8_t *)__sub_418650_n4_3 + 15) = n15_1;
      }
    }
    __sub_418650_n4_3[5] = n15_1 + n256_1;
    __sub_418650_n4_3[n4] += n15_1;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3[8] = n4;
    if ( *((uint8_t *)__sub_418650_n4_3 + 14) )
    {
      --*((uint8_t *)__sub_418650_n4_3 + 14);
      v90 = v184;
      ++v184[5];
      ++v90[n4];
      n4 = this_3[8];
    }
  }
  else
  {
    arg_tot = *(uint16_t *)(v80 + 10);
    v163 = rc.get_freq(arg_tot);
    v164 = *v184;
    if ( v164 <= v163 )
    {
      v164 += v184[1];
      if ( v164 <= v163 )
      {
        v164 += v184[2];
        if ( v164 <= v163 )
        {
          v164 += v184[3];
          if ( v164 <= v163 )
          {
            v164 += v184[4];
            n4_2 = 4;
          }
          else
          {
            n4_2 = 3;
          }
        }
        else
        {
          n4_2 = 2;
        }
      }
      else
      {
        n4_2 = 1;
      }
    }
    else
    {
      n4_2 = 0;
    }
    n15_22 = *((uint8_t *)v184 + 15);
    arg_high = v164;
    arg_cum = v164 - v184[n4_2];
    n256_4 = v184[5];
    n256_5 = v184[6];
    n15_3 = n15_22;
    if ( n256_4 > n256_5 && (v184[n4_2] + n15_3 + 8 < n256_4 || n256_4 > 0x4000) )
    {
      n15_8 = n256_5;
      this_1 = this_3;
      n4_1 = (int32_t)__sub_418650_n4_3;
      __sub_418650_n4_4 = n4_2;
      v171 = v184;
      v172 = v184[1];
      v173 = v184[2];
      v174 = *v184 - (*v184 >> 1);
      *v184 = v174;
      LOWORD(v172) = v172 - (v172 >> 1);
      v171[1] = v172;
      v175 = v173 - (v173 >> 1);
      v176 = (uint16_t)v171[3];
      v171[2] = v175;
      LOWORD(v176) = v176 - (v176 >> 1);
      v177 = (uint16_t)v171[4];
      v171[3] = v176;
      LOWORD(v177) = v177 - (v177 >> 1);
      v171[4] = v177;
      LOWORD(v177) = v174 + v177;
      this_3 = this_1;
      n256_4 = (uint16_t)(v177 + v176 + v175 + v172);
      n256_3 = n15_8;
      __sub_418650_n4_3 = (uint16_t *)n4_1;
      v171[5] = n256_4;
      n4_2 = __sub_418650_n4_4;
      if ( n256_3 < 256 && !*((uint8_t *)v184 + 14) )
      {
        n256_3 = 256;
        v184[6] = 256;
      }
      if ( n256_4 > n256_3 )
      {
        n15_2 = n15_3;
        if ( n15_3 < 15 )
          n15_2 = 15;
        n15_3 = n15_2;
        *((uint8_t *)v184 + 15) = n15_2;
      }
    }
    n15_4 = n15_3;
    v170 = v184;
    v184[5] = n15_3 + n256_4;
    v170[n4_2] += n15_4;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3[8] = n4_2;
    __sub_418650_n4_3[5] = __sub_418650_n4_3[n4_2]++ != 0;
    n4 = this_3[8];
  }
  if ( n4 )
  {
    *(uint16_t *)this_3[19] = __dword_44573C[n4];
    return 1;
  }
  n15_18 = 0;
LABEL_86:
  v91 = __byte_445700;
  n15_23 = ::__n15;
  n4_19 = ::__n4_4;
  n15_24 = n15_18;
  n4_20 = ::__n4_3;
  __buf_0[__n15_0] = __byte_445700;
  v95 = (uint16_t *)this_3[1514859];
  __buf_0[n15_23] = v91;
  __buf_0[n4_20] = v91;
  __buf_0[n4_19] = v91;
  __byte_445440[0] = v91;
  this_3[269554] = 0;
  n4_5 = v95[1];
  n15_8 = *v95;
  v97 = (uint16_t *)this_3[1514858];
  n4_6 = *v97;
  n15_5 = v97[1];
  v100 = (uint16_t *)v97[2];
  n4_1 = n4_5;
  this_4 = v97[3];
  __sub_418650_n4_4 = n4_6;
  v102 = v97[4];
  n15_3 = n15_5;
  v103 = v97[5];
  v184 = v100;
  v104 = v97[6];
  v105 = v97[7];
  this_1 = (uint32_t *)this_4;
  v106 = (uint16_t *)this_3[20];
  v186 = v102;
  v107 = v106[8];
  v187 = v103;
  v108 = this_3[19];
  v188 = v104;
  v109 = *(uint16_t *)(v108 - 16);
  v189 = v105;
  v190 = v109;
  v110 = (uint16_t *)this_3[21];
  v111 = v110[4];
  v191 = v107;
  v112 = *v110;
  v192 = v111;
  v113 = *(v106 - 8);
  v193 = v112;
  v114 = *(v110 - 4);
  v194 = v113;
  v115 = *(uint16_t *)(v108 - 24);
  v195 = v114;
  v116 = v106[12];
  v196 = v115;
  v117 = v106[16];
  v197 = v116;
  v118 = *(uint16_t *)(v108 - 32);
  v198 = v117;
  v119 = *(v106 - 12);
  v199 = v118;
  v120 = v110[8];
  v200 = v119;
  v201 = v120;
  v121 = (uint16_t *)this_3[22];
  v202 = *v121;
  v203 = *(v110 - 8);
  v122 = *(uint16_t *)(v108 - 40);
  v123 = *(uint16_t *)(v108 - 56);
  v204 = v122;
  v205 = v121[4];
  v124 = v106[20];
  v125 = v106[28];
  v206 = v124;
  v207 = *(uint16_t *)this_3[23];
  v208 = v123;
  v209 = *(v121 - 4);
  v126 = v110[12];
  this_3[11] = 0;
  v210 = v125;
  v211 = v126;
  do
  {
    v127 = __fwd_sub_418650_sub_416860(this_3, &n15_8);
    if ( v127 >= 0 )
    {
      v128 = __fwd_sub_418650_sub_412850(
               (uint16_t *)this_3 + 3 * this_3[13] + 525888,
               (uint16_t *)this_3 + 3 * this_3[12] + 525840);
      *(uint16_t *)this_3[19] = v127;
      if ( v128 )
        return n15_24 + 1;
      __buf_0[v127] = __byte_445700;
    }
    n32 = this_3[11] + 1;
    this_3[11] = n32;
  }
  while ( n32 < 32 );
  n15_25 = n15_24;
  n4_21 = ::__n4_4;
  v132 = this_3[269552];
  this_3[269554] = this_3[269553] + 24 * ::__n4_3;
  v133 = v132 + 24 * n4_21;
  v134 = (uint32_t **)this_3[269558];
  this_3[269555] = v133;
  while ( 1 )
  {
    if ( !(*v134)[1] )
      goto LABEL_95;
    v135 = __fwd_sub_418650_sub_412E60(*v134);
    *(uint16_t *)this_3[19] = v135;
    if ( v135 >= 0 )
      return n15_25 + 1;
    v134 = (uint32_t **)this_3[269558];
LABEL_95:
    this_3[269558] = ++v134;
  }
}
static inline int32_t __fwd_sub_4159E0_sub_412490(void *a0, void *a1, int32_t a2) { return __sub_412490((uint16_t *)a0, (uint16_t *)a1, a2); }
static inline int32_t __fwd_sub_4159E0_sub_412B10(void *a0, int32_t a1) { return __sub_412B10((uint32_t *)a0, a1); }
static inline int32_t __fwd_sub_4159E0_sub_4148F0(void *a0) { return __sub_4148F0((uint16_t *)a0); }
static inline int32_t __fwd_sub_4159E0_sub_416860(void *a0, void *a1) { return __sub_416860((int32_t *)a0, (uint32_t *)a1); }

int32_t __sub_4159E0(int32_t *_this, int32_t a2)
{
  alignas(16) uint8_t __hexrays_frame[164];
  int32_t &p_n15 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &n2_1 = *(int32_t *)(__hexrays_frame + 4);
  uint16_t * &n2_4 = *(uint16_t * *)(__hexrays_frame + 8);
  int32_t &n15_20 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &n15_31 = *(int32_t *)(__hexrays_frame + 16);
  int32_t &n15_2 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &n15_4 = *(int32_t *)(__hexrays_frame + 24);
  int32_t * &this_1 = *(int32_t * *)(__hexrays_frame + 28);
  int32_t &n15_3 = *(int32_t *)(__hexrays_frame + 32);
  uint16_t * &v185 = *(uint16_t * *)(__hexrays_frame + 36);
  int32_t &n15_7 = *(int32_t *)(__hexrays_frame + 40);
  int32_t &v187 = *(int32_t *)(__hexrays_frame + 44);
  int32_t &v188 = *(int32_t *)(__hexrays_frame + 48);
  int32_t &v189 = *(int32_t *)(__hexrays_frame + 52);
  int32_t &v190 = *(int32_t *)(__hexrays_frame + 56);
  int32_t &v191 = *(int32_t *)(__hexrays_frame + 60);
  int32_t &v192 = *(int32_t *)(__hexrays_frame + 64);
  int32_t &v193 = *(int32_t *)(__hexrays_frame + 68);
  int32_t &v194 = *(int32_t *)(__hexrays_frame + 72);
  int32_t &v195 = *(int32_t *)(__hexrays_frame + 76);
  int32_t &v196 = *(int32_t *)(__hexrays_frame + 80);
  int32_t &v197 = *(int32_t *)(__hexrays_frame + 84);
  int32_t &v198 = *(int32_t *)(__hexrays_frame + 88);
  int32_t &v199 = *(int32_t *)(__hexrays_frame + 92);
  int32_t &v200 = *(int32_t *)(__hexrays_frame + 96);
  int32_t &v201 = *(int32_t *)(__hexrays_frame + 100);
  int32_t &v202 = *(int32_t *)(__hexrays_frame + 104);
  int32_t &v203 = *(int32_t *)(__hexrays_frame + 108);
  int32_t &v204 = *(int32_t *)(__hexrays_frame + 112);
  int32_t &v205 = *(int32_t *)(__hexrays_frame + 116);
  int32_t &v206 = *(int32_t *)(__hexrays_frame + 120);
  int32_t &v207 = *(int32_t *)(__hexrays_frame + 124);
  int32_t &n15_14 = *(int32_t *)(__hexrays_frame + 128);
  ;
  bool v11;
  char v24, *v36, v64, v65, v66, v67, v68, v93, *v156;
  int16_t v14, n15_10, v147, v158, n15_21, v170;
  int32_t arg_cum, arg_high, arg_tot, n4, n4_1, __sub_4159E0_n15, n15_1, v8, v9, v10, *this_2,
          v13, v15, *v16, n15_6, n15_5, n15_8, v20, v21, v22, p_n15_1, n15_30, v26, v27, *this_3,
          v29, n0xFFFF, n2_9, v32, n0xFFFF_1, n53248, v35, v38, v39, v42, p_n15_2, n15_42, n8,
          v46, n15_32, n15_12, n15_9, v50, v52, v53, p_n15_4, n2_10, v59, v61, v63, n15_11,
          n15_13, n4_2, v74, n15_33, n15_35, n15_34, n15_15, n2_15, v82, n15_36, *n2_3,
          __sub_4159E0_n0x2000, n15_16, v87, n2, p_n15_5, p_n15_7, n15_17, n15_38, n15_22,
          __sub_4159E0_n4_3, __sub_4159E0_n4_4, p_n15_11, n2_5, n15_41, n15_23, n15_24, n15_25,
          *this_4, n15_26, v109, n15_27, v112, v114, v115, v116, v117, v118, v119, v120, v121,
          v122, v123, v125, v126, v127, v128, v129, n15_28, n32, n15_29, n4_5, v134, v135,
          p_n15_6, *v143, v144, n2_16, n2_17, n15_37, v149, v150, v151, v152, v154, v155, v157,
          n15_18, v161, n2_2, p_n15_10, p_n15_8, n15_40, p_n15_9, n15_19;
  uint16_t *n2_7, *n2_8, *v37, *v51, *n2_11, *n2_12, *n2_13, *n2_14, *v97, *v98, *n2_6, *v108,
           *v111, *v113, *v124, v153, *v159, *n15_39;
  uint32_t bin_tot, v55, v57, **v136, v137, p_n15_12, v139, v140, v141, v168, v169, v171, v172,
           v173;
  uint8_t *p_n15_3, *v41, *v69;
  n2_7 = (uint16_t *)*(_this + 20);
  n4 = *n2_7;
  n2_8 = (uint16_t *)*(_this + 19);
  n4_1 = (uint16_t)*(n2_8 - 4);
  __sub_4159E0_n15 = n2_7[4];
  this_1 = _this;
  n15_1 = *(n2_7 - 4);
  n15_3 = n4;
  ::__n4_4 = n4;
  n15_7 = n4_1;
  ::__n4_3 = n4_1;
  n15_4 = __sub_4159E0_n15;
  ::__n15 = __sub_4159E0_n15;
  v8 = *((uint8_t *)n2_7 + 3) + 4 * (__sub_4159E0_n15 == n15_1);
  v9 = *((uint8_t *)n2_7 + 11);
  n15_2 = n15_1;
  __n15_0 = n15_1;
  v10 = 32 * *((uint8_t *)n2_8 - 4)
      + 16 * *((uint8_t *)n2_8 - 2)
      + 2 * *((uint8_t *)n2_8 - 6)
      + 8 * v9
      + v8;
  v11 = n4 == n4_1;
  this_2 = this_1;
  if ( v11 )
  {
    if ( n15_3 == n15_4 )
    {
      v14 = *((uint16_t *)this_1 + n15_3 + 3029720);
      if ( n15_3 == n15_2 )
        v13 = (uint16_t)(v14 - *(n2_8 - 8));
      else
        v13 = (uint16_t)(v14 - n15_2);
    }
    else
    {
      v13 = (uint16_t)(*((uint16_t *)this_1 + n15_3 + 3029720) - n15_4);
    }
  }
  else
  {
    v13 = (uint16_t)(*((uint16_t *)this_1 + n15_3 + 3029720) - n15_7);
  }
  this_1[1514858] = (int32_t)&this_1[4 * v13 + 269674];
  v15 = *((uint8_t *)this_2 + v10 + 1078244);
  this_2[9] = v15;
  v16 = &this_2[0x10000 * v15 + 531818 + v13];
  this_2[1514859] = (int32_t)v16;
  n15_6 = *(uint16_t *)v16;
  if ( n15_6 == n15_3 )
  {
    n15_5 = 15;
  }
  else if ( n15_6 == n15_7 )
  {
    n15_5 = 30;
  }
  else if ( n15_6 == n15_4 )
  {
    n15_5 = 45;
  }
  else
  {
    v11 = n15_6 == n15_2;
    n15_5 = 60;
    if ( !v11 )
      n15_5 = 0;
  }
  n15_8 = *((uint16_t *)v16 + 1);
  if ( n15_8 == n15_3 )
  {
    n15_5 += 75;
  }
  else if ( n15_8 == n15_7 )
  {
    n15_5 += 150;
  }
  else if ( n15_8 == n15_4 )
  {
    n15_5 += 225;
  }
  else if ( n15_8 == n15_2 )
  {
    n15_5 += 300;
  }
  v20 = this_2[22];
  n2_1 = (int32_t)n2_7;
  v21 = *((uint8_t *)this_2 + v15 + n15_5 + 1078308);
  v22 = this_2[21];
  this_2[10] = v21;
  p_n15_1 = *((uint8_t *)n2_7 + 2);
  v24 = *((uint8_t *)n2_7 + 3);
  n2_4 = n2_8;
  this_1 = this_2;
  p_n15 = p_n15_1;
  n15_30 = this_2[23];
  v26 = *((uint8_t *)n2_8 - 5);
  n15_20 = n15_30;
  v27 = 8 * *((uint8_t *)n2_8 - 12)
      + 4 * *((uint8_t *)n2_8 - 9)
      + v26
      + 2 * *((uint8_t *)n2_8 - 10);
  this_3 = this_1;
  v29 = ((*((uint8_t *)this_1 + 1078695) == 0) << 7)
      + ((*((uint8_t *)this_1 + 1078694) == 0) << 6)
      + 32 * (*((uint8_t *)this_1 + 1078693) == 0)
      + 16 * (*((uint8_t *)this_1 + 1078692) == 0)
      + ((uint8_t)(*(uint8_t *)(n15_20 + 2) & *(uint8_t *)(v20 + 2) & p_n15 & *(uint8_t *)(v22 + 2)) << 9)
      + ((uint8_t)(*(uint8_t *)(n15_20 + 3) & *(uint8_t *)(v20 + 3) & *(uint8_t *)(v22 + 3) & v24) << 8)
      + (v21 << 10)
      + v27;
  n0xFFFF = *((uint16_t *)this_1 + v29 + 3037912);
  n2_9 = n2_1;
  if ( n0xFFFF == 0xFFFF )
  {
    *((uint16_t *)this_1 + v29 + 3037912) = this_1[5];
    n2_9 = this_3[20];
    n2_8 = (uint16_t *)this_3[19];
    ++this_3[5];
    n0xFFFF = *((uint16_t *)this_3 + v29 + 3037912);
    p_n15 = *(uint8_t *)(n2_9 + 2);
  }
  v32 = *((uint8_t *)n2_8 - 1) + 4 * *(uint8_t *)(n2_9 + 13) + 2 * p_n15 + 8 * n0xFFFF;
  n0xFFFF_1 = *((uint16_t *)this_3 + v32 + 3230424);
  if ( n0xFFFF_1 == 0xFFFF )
  {
    *((uint16_t *)this_3 + v32 + 3230424) = this_3[6]++;
    n0xFFFF_1 = *((uint16_t *)this_3 + v32 + 3230424);
  }
  if ( this_3[4] < 32 )
  {
    n53248 = this_3[7];
    v35 = 16 * n0xFFFF_1 + (n15_7 & 0xF);
    v36 = (char *)this_3 + 2 * v35;
    n0xFFFF_1 = *((uint16_t *)v36 + 3339224);
    if ( n0xFFFF_1 == 0xFFFF )
    {
      if ( n53248 > 53248 )
        v35 |= 0xFu;
      v36 = (char *)this_3 + 2 * v35;
      n0xFFFF_1 = *((uint16_t *)v36 + 3339224);
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *((uint16_t *)v36 + 3339224) = n53248;
      ++this_3[7];
      n0xFFFF_1 = *((uint16_t *)v36 + 3339224);
    }
  }
  v37 = (uint16_t *)this_3[19];
  v38 = *((uint8_t *)v37 - 5);
  v39 = *((uint8_t *)v37 - 6);
  v185 = v37;
  if ( (v38 & v39) != 0
    && (p_n15_3 = (uint8_t *)this_3[20],
        v41 = (uint8_t *)this_3[21],
        ((uint8_t)(v41[19]
                         & v41[11]
                         & v41[3]
                         & p_n15_3[27]
                         & p_n15_3[19]
                         & p_n15_3[11]
                         & p_n15_3[3]
                         & p_n15_3[2]
                         & *(p_n15_3 - 5))
       & v41[27]) != 0) )
  {
    v42 = v41[2];
    p_n15_2 = *this_3 - a2;
    n15_31 = 1;
    if ( p_n15_2 <= 1 )
    {
      n8 = 8;
    }
    else
    {
      p_n15 = p_n15_2;
      n15_42 = 1;
      this_1 = this_3;
      while ( 1 )
      {
        n8 = 8 * n15_42;
        if ( (p_n15_3[8 * n15_42 + 19] & p_n15_3[8 * n15_42 + 18]) == 0 )
          break;
        v42 = (uint8_t)(v41[n8 + 2] & v42);
        if ( ++n15_42 >= p_n15 )
        {
          this_3 = this_1;
          n15_31 = n15_42;
          n8 = 8 * n15_42;
          goto LABEL_42;
        }
      }
      this_3 = this_1;
      n15_31 = n15_42;
    }
LABEL_42:
    v46 = (uint8_t)(v41[n8 + 27] & v41[n8 + 19]);
    n15_2 = *(uint8_t *)(this_3[269671] + n15_31);
    n15_32 = *(uint8_t *)(this_3[269672] + n15_3) + 8 * n15_2 + 4 * v46 + 2 * v42;
    n15_12 = 0;
    if ( *v185 == n15_3 )
    {
      p_n15 = (int32_t)p_n15_3;
      this_1 = this_3;
      do
        ++n15_12;
      while ( n15_12 < n15_31 && v185[4 * n15_12] == n15_3 );
      this_3 = this_1;
      p_n15_3 = (uint8_t *)p_n15;
    }
    n15_9 = n15_12 == n15_31;
    n15_4 = n15_9;
    if ( n15_12 > n15_9 )
    {
      this_3[20] = (int32_t)&p_n15_3[8 * n15_12 + -8 * n15_9];
      this_3[21] = (int32_t)&v41[8 * n15_12 + -8 * n15_9];
      v50 = this_3[23];
      this_3[22] = this_3[22] + 8 * n15_12 - 8 * n15_9;
      v51 = v185;
      this_3[23] = v50 + 8 * n15_12 - 8 * n15_9;
      *((uint32_t *)v51 + 1) = 16843009;
      *(uint32_t *)this_3[19] = 16843009;
      *(uint16_t *)(this_3[1514859] + 2) = *(uint16_t *)this_3[1514859];
      LOWORD(v51) = n15_3;
      *(uint16_t *)this_3[19] = n15_3;
      *(uint16_t *)this_3[1514859] = (uint16_t)v51;
      v52 = this_3[19];
      v53 = *(uint32_t *)v52;
      n2_1 = *(uint32_t *)(v52 + 4);
      n2_4 = (uint16_t *)(v52 + 8);
      this_3[19] = v52 + 8;
      if ( n15_12 - n15_9 != 1 )
      {
        p_n15_4 = n15_12 - n15_9 - 1;
        v55 = p_n15_4 / 2;
        if ( p_n15_4 / 2 )
        {
          p_n15 = p_n15_4;
          n2_10 = n2_1;
          n15_14 = n15_12;
          v57 = 0;
          n15_20 = n15_32;
          n15_10 = n15_3;
          do
          {
            *(uint16_t *)(this_3[1514859] + 2) = n15_10;
            *(uint32_t *)this_3[19] = v53;
            *(uint32_t *)(this_3[19] + 4) = n2_10;
            v59 = this_3[1514859];
            this_3[19] += 8;
            *(uint16_t *)(v59 + 2) = n15_10;
            *(uint32_t *)this_3[19] = v53;
            *(uint32_t *)(this_3[19] + 4) = n2_10;
            n2_11 = (uint16_t *)(this_3[19] + 8);
            this_3[19] = (int32_t)n2_11;
            ++v57;
          }
          while ( v57 < v55 );
          p_n15_4 = p_n15;
          n15_32 = n15_20;
          n2_4 = n2_11;
          n15_12 = n15_14;
          v61 = 2 * v57 + 1;
        }
        else
        {
          v61 = 1;
        }
        if ( p_n15_4 > (uint32_t)(v61 - 1) )
        {
          *(uint16_t *)(this_3[1514859] + 2) = n15_3;
          *(uint32_t *)this_3[19] = v53;
          *(uint32_t *)(this_3[19] + 4) = n2_1;
          n2_12 = (uint16_t *)(this_3[19] + 8);
          this_3[19] = (int32_t)n2_12;
          n2_4 = n2_12;
        }
      }
      v63 = this_3[20];
      v64 = *(uint8_t *)(v63 - 14);
      v65 = *(uint8_t *)(v63 + 18);
      v66 = *(uint8_t *)(v63 + 34);
      n15_20 = n15_32;
      v67 = *(uint8_t *)(v63 - 22);
      n15_14 = n15_12;
      v68 = *(uint8_t *)(v63 + 26) + v65 + v64 + v67 + v66 - 5;
      v69 = (uint8_t *)this_3[21];
      *((uint8_t *)this_3 + 1078692) = v68;
      n2_13 = n2_4;
      *((uint8_t *)this_3 + 1078693) = v69[26]
                                   + v69[18]
                                   + v69[10]
                                   + v69[2]
                                   + *(v69 - 6)
                                   + *(v69 - 14)
                                   + *(v69 - 22)
                                   + v69[34]
                                   - 8;
      *((uint8_t *)this_3 + 1078694) = *((uint8_t *)n2_13 - 29) + *((uint8_t *)n2_13 - 21) - 2;
      n15_11 = n15_3;
      *((uint8_t *)this_3 + 1078695) = *((uint8_t *)n2_13 - 38)
                                   + *((uint8_t *)n2_13 - 46)
                                   + *((uint8_t *)n2_13 - 54)
                                   + *((uint8_t *)n2_13 - 30)
                                   - 4;
      *((uint8_t *)n2_13 - 2) = n15_11 == *(uint16_t *)(v63 + 8);
      n15_32 = n15_20;
      *(uint8_t *)(this_3[19] - 1) = n15_11 == *(uint16_t *)(this_3[20] + 16);
      n15_12 = n15_14;
    }
    __fwd_sub_4159E0_sub_412490((uint16_t *)this_3 + 3 * n15_32 + 538179, (uint16_t *)this_3 + 538176, n15_4);
    n15_13 = n15_4;
    n4_2 = ::__n4_4;
    v74 = this_3[269672];
    this_3[8] = n15_4;
    *(uint8_t *)(v74 + n4_2) = n15_13;
    if ( !n15_13 && n15_31 != 1 )
    {
      n15_14 = n15_12;
      p_n15 = n15_2;
      this_1 = this_3;
      n15_33 = n15_31;
      n15_35 = 1 << (n15_2 & 31);
      n15_34 = 0;
      n15_15 = n15_2;
      do
      {
        if ( n15_33 > (n15_35 | n15_34) )
        {
          n15_20 = n15_34;
          n2_14 = (uint16_t *)&this_1[24 * (n15_34 == 0) + 269473 + 24 * (n15_15 == p_n15)] + 3 * n15_15 + 1;
          n2_4 = n2_14;
          n2_1 = n15_14 & n15_35;
          bin_tot = *n2_14 + n2_14[1];
          rc.encode_bit(*n2_14, n2_14[1], (n15_14 & n15_35) != 0);
          if ( *((uint16_t *)&this_1[24 * (n15_34 == 0) + 269474 + 24 * (n15_15 == p_n15)] + 3 * n15_15 + 1) < (uint32_t)bin_tot )
            __fwd_sub_4159E0_sub_4148F0(n2_4);
          n2_15 = n2_1;
          n15_31 = n15_35;
          n2_4[n2_1 != 0] += 8;
          n15_34 |= n2_15;
          n15_35 = n15_31;
        }
        --n15_15;
        n15_35 >>= 1;
      }
      while ( n15_35 );
      n15_12 = n15_14;
      this_3 = this_1;
    }
    if ( this_3[8] )
      return n15_12;
  }
  else
  {
    v82 = 4 * this_3[10];
    n15_36 = (int32_t)&this_3[v82 + 24];
    n15_31 = n15_36;
    n2_3 = &this_3[4 * n0xFFFF_1 + 776];
    __sub_4159E0_n0x2000 = HIWORD(this_3[4 * n0xFFFF_1 + 778]);
    if ( __sub_4159E0_n0x2000 )
    {
      if ( __sub_4159E0_n0x2000 == 1 )
      {
        v143 = &this_3[v82 + 24];
        v144 = *(uint8_t *)(n15_36 + 15);
        n2_16 = v144 * *((uint16_t *)n2_3 + 2);
        n2_17 = v144 * *((uint16_t *)n2_3 + 3);
        p_n15 = v144 * *(uint16_t *)n2_3;
        v147 = v144 * *((uint16_t *)n2_3 + 1);
        n2_1 = n2_16;
        n15_37 = v144 * *((uint16_t *)n2_3 + 4);
        n2_4 = (uint16_t *)n2_17;
        n15_20 = n15_37;
        *(uint64_t *)n2_3 = *(uint64_t *)v143;
        *((uint64_t *)n2_3 + 1) = *((uint64_t *)v143 + 1);
        v149 = *((uint16_t *)n2_3 + 5);
        *((uint8_t *)n2_3 + 14) *= 8;
        this_1 = this_3;
        v150 = 21 * *((uint16_t *)n2_3 + 1);
        p_n15 += (21 * *(uint16_t *)n2_3 + v149 - 1) / v149;
        *(uint16_t *)n2_3 = p_n15;
        v151 = (v150 + v149 - 1) / v149;
        LOWORD(v150) = n2_1;
        v152 = 21 * *((uint16_t *)n2_3 + 2);
        v153 = v151 + v147;
        *((uint16_t *)n2_3 + 1) = v153;
        v154 = (v152 + v149 - 1) / v149;
        v155 = 21 * *((uint16_t *)n2_3 + 3);
        LOWORD(v150) = v154 + v150;
        *((uint16_t *)n2_3 + 2) = v150;
        v156 = (char *)n2_4 + (v155 + v149 - 1) / v149;
        v157 = 21 * *((uint16_t *)n2_3 + 4);
        *((uint16_t *)n2_3 + 3) = (uint16_t)v156;
        v158 = (uint16_t)v156 + v150 + v153;
        this_3 = this_1;
        LOWORD(v149) = (v157 + v149 - 1) / v149 + n15_20;
        *((uint16_t *)n2_3 + 4) = v149;
        __sub_4159E0_n0x2000 = (uint16_t)(p_n15 + v149 + v158);
        *((uint16_t *)n2_3 + 5) = __sub_4159E0_n0x2000;
        v185 = (uint16_t *)this_3[19];
      }
      n15_16 = *v185;
      arg_tot = __sub_4159E0_n0x2000;
      if ( n15_16 == n15_3 )
      {
        v87 = *(uint16_t *)n2_3;
        n2 = 1;
      }
      else if ( n15_16 == n15_7 )
      {
        v87 = *(uint16_t *)n2_3 + *((uint16_t *)n2_3 + 1);
        n2 = 2;
      }
      else if ( n15_16 == n15_4 )
      {
        v87 = *(uint16_t *)n2_3 + *((uint16_t *)n2_3 + 2) + *((uint16_t *)n2_3 + 1);
        n2 = 3;
      }
      else if ( n15_16 == n15_2 )
      {
        v87 = *((uint16_t *)n2_3 + 5) - *((uint16_t *)n2_3 + 4);
        n2 = 4;
      }
      else
      {
        v87 = 0;
        n2 = 0;
      }
      p_n15_5 = *((uint16_t *)n2_3 + 6);
      arg_cum = v87;
      arg_high = *((uint16_t *)n2_3 + n2) + v87;
      p_n15_7 = *((uint16_t *)n2_3 + 5);
      n15_17 = *((uint8_t *)n2_3 + 15);
      if ( p_n15_7 > p_n15_5
        && (*((uint16_t *)n2_3 + n2) + n15_17 + 8 < p_n15_7 || *((uint16_t *)n2_3 + 5) > 0x4000u) )
      {
        v137 = *((uint16_t *)n2_3 + 2);
        p_n15 = p_n15_5;
        p_n15_12 = *((uint16_t *)n2_3 + 1);
        this_1 = this_3;
        n2_1 = n2;
        v139 = *(uint16_t *)n2_3;
        n2_4 = (uint16_t *)n15_17;
        LOWORD(v139) = v139 - (v139 >> 1);
        *(uint16_t *)n2_3 = v139;
        LOWORD(p_n15_12) = p_n15_12 - (p_n15_12 >> 1);
        *((uint16_t *)n2_3 + 1) = p_n15_12;
        LOWORD(v137) = v137 - (v137 >> 1);
        v140 = *((uint16_t *)n2_3 + 3);
        *((uint16_t *)n2_3 + 2) = v137;
        LOWORD(v140) = v140 - (v140 >> 1);
        v141 = *((uint16_t *)n2_3 + 4);
        *((uint16_t *)n2_3 + 3) = v140;
        LOWORD(v141) = v141 - (v141 >> 1);
        *((uint16_t *)n2_3 + 4) = v141;
        LOWORD(p_n15_12) = v140 + v137 + p_n15_12;
        n15_17 = (int32_t)n2_4;
        LOWORD(p_n15_12) = v139 + v141 + p_n15_12;
        n2 = n2_1;
        this_3 = this_1;
        p_n15_7 = (uint16_t)p_n15_12;
        p_n15_6 = p_n15;
        *((uint16_t *)n2_3 + 5) = p_n15_7;
        if ( p_n15_6 < 256 && !*((uint8_t *)n2_3 + 14) )
        {
          p_n15_6 = 256;
          *((uint16_t *)n2_3 + 6) = 256;
        }
        if ( p_n15_7 > p_n15_6 )
        {
          if ( n15_17 < 15 )
            LOWORD(n15_17) = 15;
          *((uint8_t *)n2_3 + 15) = n15_17;
        }
      }
      *((uint16_t *)n2_3 + 5) = n15_17 + p_n15_7;
      *((uint16_t *)n2_3 + n2) += n15_17;
      rc.encode(arg_cum, arg_high, arg_tot);
      this_3[8] = n2;
      if ( *((uint8_t *)n2_3 + 14) )
      {
        --*((uint8_t *)n2_3 + 14);
        n15_38 = n15_31;
        ++*(uint16_t *)(n15_31 + 10);
        ++*(uint16_t *)(n15_38 + 2 * n2);
        n2 = this_3[8];
      }
    }
    else
    {
      v159 = (uint16_t *)&this_3[v82 + 24];
      n15_18 = *v185;
      arg_tot = *(uint16_t *)(n15_36 + 10);
      if ( n15_18 == n15_3 )
      {
        v161 = *v159;
        n2_2 = 1;
      }
      else if ( n15_18 == n15_7 )
      {
        v161 = *v159 + v159[1];
        n2_2 = 2;
      }
      else if ( n15_18 == n15_4 )
      {
        v161 = *v159 + v159[2] + v159[1];
        n2_2 = 3;
      }
      else if ( n15_18 == n15_2 )
      {
        v161 = v159[5] - v159[4];
        n2_2 = 4;
      }
      else
      {
        v161 = 0;
        n2_2 = 0;
      }
      arg_cum = v161;
      arg_high = *(uint16_t *)(n15_31 + 2 * n2_2) + v161;
      p_n15_10 = *(uint16_t *)(n15_31 + 10);
      p_n15_8 = *(uint16_t *)(n15_31 + 12);
      n15_20 = *(uint8_t *)(n15_31 + 15);
      if ( p_n15_10 > p_n15_8 && (*(uint16_t *)(n15_31 + 2 * n2_2) + n15_20 + 8 < p_n15_10 || p_n15_10 > 0x4000) )
      {
        p_n15 = p_n15_8;
        this_1 = this_3;
        n15_39 = (uint16_t *)n15_31;
        v168 = *(uint16_t *)(n15_31 + 2);
        v169 = *(uint16_t *)(n15_31 + 4);
        n2_1 = (int32_t)n2_3;
        n2_4 = (uint16_t *)n2_2;
        v170 = *(uint16_t *)n15_31 - (*(uint16_t *)n15_31 >> 1);
        *(uint16_t *)n15_31 = v170;
        LOWORD(v168) = v168 - (v168 >> 1);
        n15_39[1] = v168;
        v171 = v169 - (v169 >> 1);
        v172 = (uint16_t)n15_39[3];
        n15_39[2] = v171;
        LOWORD(v172) = v172 - (v172 >> 1);
        v173 = (uint16_t)n15_39[4];
        n15_39[3] = v172;
        LOWORD(v173) = v173 - (v173 >> 1);
        n15_39[4] = v173;
        LOWORD(v173) = v170 + v173;
        n2_2 = (int32_t)n2_4;
        p_n15_10 = (uint16_t)(v173 + v172 + v171 + v168);
        p_n15_9 = p_n15;
        n2_3 = (int32_t *)n2_1;
        n15_39[5] = p_n15_10;
        this_3 = this_1;
        if ( p_n15_9 < 256 && !*(uint8_t *)(n15_31 + 14) )
        {
          p_n15_9 = 256;
          *(uint16_t *)(n15_31 + 12) = 256;
        }
        if ( p_n15_10 > p_n15_9 )
        {
          n15_19 = n15_20;
          if ( n15_20 < 15 )
            n15_19 = 15;
          n15_20 = n15_19;
          *(uint8_t *)(n15_31 + 15) = n15_19;
        }
      }
      n15_21 = n15_20;
      n15_40 = n15_31;
      *(uint16_t *)(n15_31 + 10) = n15_20 + p_n15_10;
      *(uint16_t *)(n15_40 + 2 * n2_2) += n15_21;
      rc.encode(arg_cum, arg_high, arg_tot);
      this_3[8] = n2_2;
      *((uint16_t *)n2_3 + 5) = (*((uint16_t *)n2_3 + n2_2))++ != 0;
      n2 = this_3[8];
    }
    if ( n2 )
      return 1;
    n15_12 = 0;
  }
  v93 = __byte_445700;
  n15_22 = ::__n15;
  __sub_4159E0_n4_3 = ::__n4_3;
  __sub_4159E0_n4_4 = ::__n4_4;
  __buf_0[__n15_0] = __byte_445700;
  __buf_0[n15_22] = v93;
  v97 = (uint16_t *)this_3[1514859];
  __buf_0[__sub_4159E0_n4_3] = v93;
  __buf_0[__sub_4159E0_n4_4] = v93;
  v98 = (uint16_t *)this_3[1514858];
  __byte_445440[0] = v93;
  n15_14 = n15_12;
  this_3[269554] = 0;
  p_n15_11 = *v97;
  n2_5 = v97[1];
  n2_6 = (uint16_t *)*v98;
  n15_41 = v98[2];
  p_n15 = p_n15_11;
  n15_23 = v98[1];
  n2_1 = n2_5;
  n15_24 = v98[3];
  n2_4 = n2_6;
  n15_25 = v98[4];
  n15_20 = n15_23;
  this_4 = (int32_t *)v98[5];
  n15_31 = n15_41;
  n15_26 = v98[6];
  v108 = (uint16_t *)v98[7];
  n15_2 = n15_24;
  v109 = this_3[19];
  n15_4 = n15_25;
  n15_27 = *(uint16_t *)(v109 - 16);
  this_1 = this_4;
  v111 = (uint16_t *)this_3[20];
  n15_3 = n15_26;
  v112 = v111[8];
  v185 = v108;
  n15_7 = n15_27;
  v113 = (uint16_t *)this_3[21];
  v114 = v113[4];
  v187 = v112;
  v115 = *v113;
  v188 = v114;
  v116 = *(v111 - 8);
  v189 = v115;
  v117 = *(v113 - 4);
  v190 = v116;
  v118 = *(uint16_t *)(v109 - 24);
  v191 = v117;
  v119 = v111[12];
  v192 = v118;
  v120 = v111[16];
  v193 = v119;
  v121 = *(uint16_t *)(v109 - 32);
  v194 = v120;
  v122 = *(v111 - 12);
  v195 = v121;
  v123 = v113[8];
  v196 = v122;
  v197 = v123;
  v124 = (uint16_t *)this_3[22];
  v198 = *v124;
  v199 = *(v113 - 8);
  v125 = *(uint16_t *)(v109 - 40);
  v126 = *(uint16_t *)(v109 - 56);
  v200 = v125;
  v201 = v124[4];
  v127 = v111[20];
  v128 = v111[28];
  v202 = v127;
  v203 = *(uint16_t *)this_3[23];
  v204 = v126;
  v205 = *(v124 - 4);
  v206 = v128;
  v207 = v113[12];
  this_3[11] = 0;
  do
  {
    v129 = __fwd_sub_4159E0_sub_416860(this_3, &p_n15);
    if ( v129 >= 0 )
    {
      n15_28 = v129 == *(uint16_t *)this_3[19];
      __fwd_sub_4159E0_sub_412490(
        (uint16_t *)this_3 + 3 * this_3[13] + 525888,
        (uint16_t *)this_3 + 3 * this_3[12] + 525840,
        n15_28);
      if ( n15_28 )
        return n15_14 + 1;
      __buf_0[v129] = __byte_445700;
    }
    n32 = this_3[11] + 1;
    this_3[11] = n32;
  }
  while ( n32 < 32 );
  n15_29 = n15_14;
  n4_5 = ::__n4_4;
  v134 = this_3[269552];
  this_3[269554] = this_3[269553] + 24 * ::__n4_3;
  v135 = v134 + 24 * n4_5;
  v136 = (uint32_t **)this_3[269558];
  this_3[269555] = v135;
  while ( 1 )
  {
    if ( !(*v136)[1] )
      goto LABEL_94;
    if ( __fwd_sub_4159E0_sub_412B10(*v136, *(uint16_t *)this_3[19]) )
      return n15_29 + 1;
    v136 = (uint32_t **)this_3[269558];
LABEL_94:
    this_3[269558] = (int32_t)++v136;
  }
}
static inline int32_t __fwd_sub_419430_sub_412E60(void *a0) { return __sub_412E60((uint32_t *)a0); }
static inline uint32_t __fwd_sub_419430_sub_414860(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_414860((int32_t *)a0, a1, a2, a3); }

void __sub_419430(uint32_t *_this)
{
  alignas(16) uint8_t __hexrays_frame[420];
  uint64_t (&v28)[2] = *(uint64_t (*)[2])(__hexrays_frame + 0);
  int32_t &v29 = *(int32_t *)(__hexrays_frame + 16);
  uint32_t (&v30)[91] = *(uint32_t (*)[91])(__hexrays_frame + 20);
  uint32_t (&v31)[5] = *(uint32_t (*)[5])(__hexrays_frame + 384);
  ;
  bool v24;
  int32_t n8, v4, v6, v9, n16_1, v16, n16, v25, v27;
  uint32_t j_2, i, n8193, v8, *v10, j_1, j, v15, v17, v18, v19, v20, v21, *v22, v26;
  void *v12;
  n8 = *(_this + 2);
  j_2 = 0xFFFFFFFF >> (-*((uint8_t *)_this + 8) & 31);
  v4 = (n8 + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    v6 = 12 * i;
    v30[v6] = 0;
    v30[v6 + 6] = 0;
  }
  n8193 = j_2 + 1;
  if ( n8 > 8 )
    n8193 = 8193;
  v8 = __sub_4123E0(n8193);
  *(_this + 4) = v8 + 1;
  if ( (int32_t)(v8 + 1) <= 0x2000 )
  {
    v12 = malloc(4 * v8 + 4);
    j_1 = *(_this + 4);
    *(_this + 269560) = v12;
    if ( j_1 )
    {
      for ( j = 0; j < j_1; ++j )
      {
        *(uint32_t *)(*(_this + 269560) + 4 * j) = j;
        j_1 = *(_this + 4);
      }
    }
    if ( (int32_t)*(_this + 2) > 8 )
    {
      if ( 4 * v4 )
      {
        v15 = 0;
        do
          __fwd_sub_419430_sub_414860((int32_t *)&v28[3 * v15++], (int32_t)_this, 256, 1);
        while ( v15 < 4 * v4 );
        j_1 = *(_this + 4);
      }
      if ( j_1 )
      {
        v16 = *(_this + 269560);
        v17 = 0;
        v18 = 0;
        do
        {
          *(uint32_t *)(v16 + 4 * v18) = 0;
          if ( v4 )
          {
            v31[0] = v4;
            v19 = 0;
            do
            {
              v20 = __fwd_sub_419430_sub_412E60((uint32_t *)&v28[12 * v19 + 3 * v17]);
              v21 = v20 << ((8 * v19) & 31);
              v17 = v20 >> 6;
              *(uint32_t *)(*(_this + 269560) + 4 * v18) += v21;
              ++v19;
            }
            while ( v19 < v31[0] );
            v4 = v31[0];
          }
          v16 = *(_this + 269560);
          v17 = *(uint8_t *)(v16 + 4 * v18++) >> 7;
        }
        while ( v18 < *(_this + 4) );
      }
    }
    else if ( j_1 <= j_2 )
    {
      __fwd_sub_419430_sub_414860((int32_t *)v28, (int32_t)_this, j_2 - j_1 + 2, 1);
      v24 = *(_this + 4) == 0;
      v29 = 19 * LODWORD(v28[0]);
      if ( !v24 )
      {
        v25 = 0;
        v26 = 0;
        do
        {
          v27 = __fwd_sub_419430_sub_412E60(v28);
          *(uint32_t *)(*(_this + 269560) + 4 * v26) = v27 + v25;
          v25 += v27 + 1;
          ++v26;
        }
        while ( v26 < *(_this + 4) );
      }
    }
    v22 = v31;
    n16 = 16;
    do
    {
      v22 -= 6;
      free((void *)v22[5]);
      --n16;
    }
    while ( n16 );
  }
  else
  {
    v9 = *(_this + 1) * v4;
    *(_this + 2) = 8;
    *(_this + 1) = v9;
    __sub_419430(_this);
    v10 = v31;
    n16_1 = 16;
    do
    {
      v10 -= 6;
      free((void *)v10[5]);
      --n16_1;
    }
    while ( n16_1 );
  }
}

 BMF_SSE int32_t __sub_417980(int32_t a1, int32_t a2, int32_t i, int32_t a4, int32_t a5)
{
  ;
  __m128i si128, v26, v27;
  char *v8, v11, v12, v28, v29, v30, v31, v32, v37;
  int16_t v19;
  int32_t i_1, j, v9, v13, n0x2000_4, k_1, v35, v38;
  uint32_t n0x2000_2, n0x2000_5, n0x2000, k, m, n0x2000_3, n0x2000_6, n0x2000_1, n, n8, n0x18;
  uint8_t *v10;
  i_1 = i;
  __byte_445700 = 1;
  *(uint32_t *)a1 = i;
  *(uint32_t *)(a1 + 4) = a4;
  *(uint32_t *)(a1 + 8) = a5;
  *(uint32_t *)(a1 + 12) = a5;
  *(uint32_t *)(a1 + 1078204) = 0;
  *(uint32_t *)(a1 + 1078240) = 0;
  for ( j = 0; j < 5; ++j )
  {
    v8 = (char *)malloc(8 * i_1 + 128);
    *(uint32_t *)(a1 + 4 * j + 56) = v8;
    *(uint32_t *)(a1 + 4 * j + 76) = v8 + 64;
    i_1 = *(uint32_t *)a1;
    if ( *(int32_t *)a1 > -16 )
    {
      v9 = 0;
      do
      {
        *(uint16_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9) = 0;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 7) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 6) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 5) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 4) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 3) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 2) = 1;
        i_1 = *(uint32_t *)a1;
        ++v9;
      }
      while ( v9 < *(uint32_t *)a1 + 16 );
    }
  }
  v10 = malloc(i_1 + 1);
  *(uint32_t *)(a1 + 1078684) = v10;
  *v10 = 0;
  if ( *(int32_t *)a1 > 0 )
  {
    v12 = 0;
    v13 = 0;
    do
    {
      v12 += v13 == 2 << (v12 & 31);
      *(uint8_t *)(*(uint32_t *)(a1 + 1078684) + v13++ + 1) = v12;
    }
    while ( v13 < *(uint32_t *)a1 );
  }
  n0x2000_4 = a1 & 0xF;
  n0x2000_2 = n0x2000_4;
  if ( (a1 & 0xF) != 0 )
  {
    if ( (n0x2000_4 & 1) != 0 )
    {
      n0x2000 = 0;
      goto LABEL_15;
    }
    n0x2000_2 = (uint32_t)(16 - n0x2000_4) >> 1;
    n0x2000_5 = 0;
    do
      *(uint16_t *)(a1 + 2 * n0x2000_5++ + 6059440) = 0;
    while ( n0x2000_5 < n0x2000_2 );
  }
  n0x2000 = 0x2000 - (-n0x2000_2 & 0x1F);
  do
  {
    *(__m128i *)(a1 + 2 * n0x2000_2 + 6059440) = 0;
    *(__m128i *)(a1 + 2 * n0x2000_2 + 6059456) = 0;
    *(__m128i *)(a1 + 2 * n0x2000_2 + 6059472) = 0;
    *(__m128i *)(a1 + 2 * n0x2000_2 + 6059488) = 0;
    n0x2000_2 += 32;
  }
  while ( n0x2000_2 < n0x2000 );
LABEL_15:
  while ( n0x2000 < 0x2000 )
    *(uint16_t *)(a1 + 2 * n0x2000++ + 6059440) = 0;
  for ( k = 0; k < 0x2000; ++k )
  {
    v19 = *(uint16_t *)(a1 + 2 * k + 6059440);
    k_1 = k;
    for ( m = 0; m < 0xD; ++m )
    {
      v19 += v19 + (k_1 & 1);
      k_1 >>= 1;
    }
    *(uint16_t *)(a1 + 2 * k + 6059440) = v19;
  }
  n0x2000_3 = a1 & 0xF;
  if ( (a1 & 0xF) != 0 )
  {
    if ( (a1 & 1) != 0 )
    {
      n0x2000_1 = 0;
      goto LABEL_26;
    }
    n0x2000_3 = (16 - (a1 & 0xFu)) >> 1;
    n0x2000_6 = 0;
    do
      *(uint16_t *)(a1 + 2 * n0x2000_6++ + 6059440) *= 8;
    while ( n0x2000_6 < n0x2000_3 );
  }
  n0x2000_1 = 0x2000 - (-n0x2000_3 & 0x1F);
  do
  {
    si128 = _mm_load_si128((const __m128i *)(a1 + 2 * n0x2000_3 + 6059456));
    v26 = _mm_load_si128((const __m128i *)(a1 + 2 * n0x2000_3 + 6059472));
    v27 = _mm_load_si128((const __m128i *)(a1 + 2 * n0x2000_3 + 6059488));
    *(__m128i *)(a1 + 2 * n0x2000_3 + 6059440) = _mm_slli_epi16(
                                                   _mm_load_si128((const __m128i *)(a1 + 2 * n0x2000_3 + 6059440)),
                                                   3u);
    *(__m128i *)(a1 + 2 * n0x2000_3 + 6059456) = _mm_slli_epi16(si128, 3u);
    *(__m128i *)(a1 + 2 * n0x2000_3 + 6059472) = _mm_slli_epi16(v26, 3u);
    *(__m128i *)(a1 + 2 * n0x2000_3 + 6059488) = _mm_slli_epi16(v27, 3u);
    n0x2000_3 += 32;
  }
  while ( n0x2000_3 < n0x2000_1 );
LABEL_26:
  while ( n0x2000_1 < 0x2000 )
    *(uint16_t *)(a1 + 2 * n0x2000_1++ + 6059440) *= 8;
  memset((char *)(a1 + 3104),0,0x100000);
  *(uint32_t *)(a1 + 28) = 0;
  *(uint32_t *)(a1 + 24) = 0;
  *(uint32_t *)(a1 + 20) = 0;
  memset((char *)(a1 + 6075824),255,385024);
  memset((char *)(a1 + 6460848),255,217600);
  memset((char *)(a1 + 6678448),255,1424000);
  memset(__buf_0,0,8193);
  *(uint64_t *)(a1 + 1078216) = 0;
  *(uint64_t *)(a1 + 1078224) = 0;
  for ( n = 0; n < 0x40000; ++n )
  {
    *(uint16_t *)(a1 + 4 * n + 1078696) = 0x2000;
    *(uint16_t *)(a1 + 4 * n + 1078698) = 0x2000;
  }
  n8 = 0;
  do
  {
    v35 = 6 * n8;
    *(uint16_t *)(a1 + 2 * v35 + 1051680) = 40;
    ++n8;
    *(uint16_t *)(a1 + 2 * v35 + 1051682) = 16;
    *(uint16_t *)(a1 + 2 * v35 + 1051684) = 512;
    *(uint16_t *)(a1 + 2 * v35 + 1051686) = 40;
    *(uint16_t *)(a1 + 2 * v35 + 1051688) = 16;
    *(uint16_t *)(a1 + 2 * v35 + 1051690) = 512;
  }
  while ( n8 < 8 );
  n0x18 = 0;
  memset((char *)(a1 + 1051776),0,24576);
  *(uint32_t *)(a1 + 1078236) = malloc(2 * *(uint32_t *)(a1 + 4) * *(uint32_t *)a1);
  *(uint16_t *)(a1 + 1076352) = 4;
  *(uint16_t *)(a1 + 1076354) = 4;
  *(uint16_t *)(a1 + 1076356) = 72;
  memset((char *)(a1 + 1076358),0,1536);
  do
  {
    v38 = 6 * n0x18;
    *(uint16_t *)(a1 + 2 * v38 + 1077894) = 4;
    ++n0x18;
    *(uint16_t *)(a1 + 2 * v38 + 1077896) = 4;
    *(uint16_t *)(a1 + 2 * v38 + 1077898) = 72;
    *(uint16_t *)(a1 + 2 * v38 + 1077900) = 4;
    *(uint16_t *)(a1 + 2 * v38 + 1077902) = 4;
    *(uint16_t *)(a1 + 2 * v38 + 1077904) = 72;
  }
  while ( n0x18 < 0x18 );
  return a1;
}
static inline uint32_t __fwd_sub_417E80_sub_414860(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_414860((int32_t *)a0, a1, a2, a3); }
static inline int32_t __fwd_sub_417E80_sub_418650(void *a0, int32_t a1) { return __sub_418650((uint32_t *)a0, a1); }
static inline void __fwd_sub_417E80_sub_419430(void *a0) { __sub_419430((uint32_t *)a0); }

void __sub_417E80(uint32_t *_this, char *Src)
{
  alignas(16) uint8_t __hexrays_frame[100];
  int32_t &Size = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v82 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v84 = *(int32_t *)(__hexrays_frame + 8);
  char * &v85 = *(char * *)(__hexrays_frame + 12);
  int32_t &v86 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v87 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v88 = *(int32_t *)(__hexrays_frame + 16);
  char * &ArgList_1 = *(char * *)(__hexrays_frame + 20);
  char * &ArgList_5 = *(char * *)(__hexrays_frame + 20);
  int32_t &n6_3 = *(int32_t *)(__hexrays_frame + 20);
  char * &v92 = *(char * *)(__hexrays_frame + 24);
  int32_t &v93 = *(int32_t *)(__hexrays_frame + 28);
  int32_t &v94 = *(int32_t *)(__hexrays_frame + 32);
  int32_t &v95 = *(int32_t *)(__hexrays_frame + 36);
  int32_t &ArgList_4 = *(int32_t *)(__hexrays_frame + 40);
  char * &Src_1 = *(char * *)(__hexrays_frame + 44);
  uint32_t * &this_1 = *(uint32_t * *)(__hexrays_frame + 48);
  int32_t &v99 = *(int32_t *)(__hexrays_frame + 52);
  int32_t &v100 = *(int32_t *)(__hexrays_frame + 56);
  int32_t &v101 = *(int32_t *)(__hexrays_frame + 60);
  int32_t &v102 = *(int32_t *)(__hexrays_frame + 64);
  ;
  bool v38;
  char *ArgList, *v9, *ArgList_2, *buf, v26, *ArgList_3, *ArgList_9, *ArgList_10, *Src_2, *v77,
       *ArgList_8;
  int16_t v20;
  int32_t v3, v5, v6, v8, n5, v11, v14, v15, v16, v17, v18, v19, n0x10000, this_4, v27, v28, v29,
          v30, v34, v39, v40, n4_1, v43, v44, v45, v46, v47, v48, v51, v52, v53, v54, v56, v57,
          v58, v60, v61, v62, v64, v65, v66, n6, v68, v69, n6_4, n6_1, v73, n6_2, v76, v78, v80;
  uint32_t *this_2, *this_3, *v13, *v22, *v31, *i_1, *i, *v35, *j_1, *j, *ArgList_7, *ArgList_6;
  uint8_t *v49, *v50;
  v3 = *(_this + 2) < 8;
  Src_1 = Src;
  ArgList = &Src[-v3];
  __sub_4149C0((char)ArgList);
  __fwd_sub_417E80_sub_419430(_this);
  ArgList_1 = ArgList;
  this_1 = _this;
  v102 = 0;
  v5 = 0;
  do
  {
    v6 = (uint8_t)__byte_439860[v5];
    this_2 = this_1;
    *((uint8_t *)this_1 + v6 + 1078244) = v5;
    v100 = 0;
    v82 = v5;
    v92 = (char *)this_2 + v5;
    v88 = v6 & 4;
    v99 = v6 & 2;
    v95 = v6 & 0x10;
    v8 = 0;
    v93 = v6 & 1;
    v9 = (char *)this_2 + v5;
    ArgList_4 = v6 & 0x20;
    v94 = v6 & 8;
    do
    {
      v100 = v8;
      n5 = 0;
      v85 = &v9[15 * v8];
      do
      {
        v11 = v102;
        this_3 = this_1;
        v85[75 * n5 + 1078308] = v102;
        v101 = this_3[4];
        v13 = &this_3[4 * v11];
        *((uint16_t *)v13 + 49) = 2;
        *((uint16_t *)v13 + 50) = 2;
        *((uint16_t *)v13 + 51) = 2;
        *((uint16_t *)v13 + 52) = 2;
        if ( v88 )
        {
          v14 = (uint16_t)(*((uint16_t *)v13 + 52) + *((uint16_t *)v13 + 51));
          *((uint16_t *)v13 + 51) = v14;
          v15 = 0;
          *((uint16_t *)v13 + 52) = 0;
        }
        else
        {
          v14 = *((uint16_t *)v13 + 51);
          v15 = *((uint16_t *)v13 + 52);
        }
        if ( v99 )
        {
          v16 = (uint16_t)(v15 + *((uint16_t *)v13 + 50));
          *((uint16_t *)v13 + 50) = v16;
          v15 = 0;
          *((uint16_t *)v13 + 52) = 0;
        }
        else
        {
          v16 = *((uint16_t *)v13 + 50);
        }
        if ( v95 )
        {
          v16 = (uint16_t)(v14 + v16);
          *((uint16_t *)v13 + 50) = v16;
          v14 = 0;
          *((uint16_t *)v13 + 51) = 0;
        }
        if ( v93 )
        {
          *((uint16_t *)v13 + 49) += v15;
          v15 = 0;
          *((uint16_t *)v13 + 52) = 0;
        }
        if ( v94 )
        {
          *((uint16_t *)v13 + 49) += v14;
          v14 = 0;
          *((uint16_t *)v13 + 51) = 0;
        }
        if ( ArgList_4 )
        {
          *((uint16_t *)v13 + 49) += v16;
          v16 = 0;
          *((uint16_t *)v13 + 50) = 0;
        }
        v17 = (v14 != 0) + (v16 != 0) + (v15 != 0) + 2;
        if ( v17 <= v101 )
        {
          *((uint8_t *)v13 + 110) = v17;
          *((uint16_t *)v13 + 48) = 2;
        }
        else
        {
          LOBYTE(v17) = v17 - 1;
          *((uint8_t *)v13 + 110) = v17;
          *((uint16_t *)v13 + 48) = 0;
        }
        if ( *((uint16_t *)v13 + v100 + 48) && *((uint16_t *)v13 + n5 + 48) && (uint8_t)v17 <= v101 )
        {
          v19 = v100;
          v18 = 1;
          v20 = (uint8_t)(1 << ((5 - v17) & 31));
          *((uint8_t *)v13 + 111) = v20;
          *((uint16_t *)v13 + 54) = v20 << 6;
          *((uint16_t *)v13 + v19 + 48) += v20;
          *((uint16_t *)v13 + n5 + 48) += *((uint8_t *)v13 + 111);
          *((uint16_t *)v13 + 53) = *((uint16_t *)v13 + 48)
                               + *((uint16_t *)v13 + 52)
                               + *((uint16_t *)v13 + 51)
                               + *((uint16_t *)v13 + 50)
                               + *((uint16_t *)v13 + 49);
        }
        else
        {
          v18 = 0;
        }
        v102 += v18;
        ++n5;
      }
      while ( n5 < 5 );
      v9 = v92;
      v8 = v100 + 1;
    }
    while ( v100 + 1 < 5 );
    n0x10000 = 0;
    v22 = &this_1[0x10000 * v82];
    do
    {
      LOWORD(v22[n0x10000 + 531818]) = 0x2000;
      HIWORD(v22[n0x10000++ + 531818]) = 0x2000;
    }
    while ( n0x10000 < 0x10000 );
    v5 = v82 + 1;
  }
  while ( v82 + 1 < 15 );
  ArgList_2 = ArgList_1;
  this_4 = (int32_t)this_1;
  buf = (char *)malloc(this_1[4]);
  Size = this_1[4];
  this_1[269672] = buf;
  memset(buf,1,Size);
  v27 = *(uint32_t *)(this_4 + 100);
  v28 = *(uint32_t *)(this_4 + 104);
  *(uint32_t *)(this_4 + 1051664) = *(uint32_t *)(this_4 + 96);
  v29 = *(uint32_t *)(this_4 + 108);
  *(uint32_t *)(this_4 + 1051668) = v27;
  *(uint32_t *)(this_4 + 1051672) = v28;
  *(uint32_t *)(this_4 + 1051676) = v29;
  *(uint32_t *)(this_4 + 1078224) = this_4 + 1078184;
  __fwd_sub_417E80_sub_414860((int32_t *)(this_4 + 1078184), this_4, *(uint32_t *)(this_4 + 16), 1);
  *(uint32_t *)(this_4 + 1078232) = this_4 + 1078216;
  v30 = *(uint32_t *)(this_4 + 16);
  v31 = malloc(24 * v30 + 4);
  if ( v31 )
  {
    *v31 = v30;
    i_1 = v31 + 1;
    for ( i = i_1; v30; --v30 )
    {
      i_1[5] = 0;
      i_1 += 6;
    }
  }
  else
  {
    i = nullptr;
  }
  v34 = *(uint32_t *)(this_4 + 16);
  *(uint32_t *)(this_4 + 1078208) = i;
  v35 = malloc(24 * v34 + 4);
  if ( v35 )
  {
    *v35 = v34;
    j_1 = v35 + 1;
    for ( j = j_1; v34; --v34 )
    {
      j_1[5] = 0;
      j_1 += 6;
    }
  }
  else
  {
    j = nullptr;
  }
  v38 = *(uint32_t *)(this_4 + 16) <= 0;
  *(uint32_t *)(this_4 + 1078212) = j;
  if ( !v38 )
  {
    v39 = 0;
    do
    {
      __fwd_sub_417E80_sub_414860((int32_t *)(*(uint32_t *)(this_4 + 1078208) + 24 * v39), this_4, 99, 0);
      __fwd_sub_417E80_sub_414860((int32_t *)(*(uint32_t *)(this_4 + 1078212) + 24 * v39++), this_4, 33, 0);
    }
    while ( v39 < *(uint32_t *)(this_4 + 16) );
  }
  v40 = *(uint32_t *)(this_4 + 8);
  if ( v40 == *(uint32_t *)(this_4 + 12) )
  {
    ArgList_3 = nullptr;
  }
  else
  {
    ArgList_2 = (char *)malloc(*(uint32_t *)(this_4 + 4) * *(uint32_t *)this_4 + 3);
    v40 = *(uint32_t *)(this_4 + 8);
    ArgList_3 = ArgList_2;
  }
  n4_1 = (v40 + 7) >> 3;
  if ( *(int32_t *)(this_4 + 4) > 0 )
  {
    n4 = n4_1;
    ArgList_4 = (int32_t)ArgList_3;
    ArgList_5 = ArgList_2;
    v43 = 0;
    while ( 1 )
    {
      v86 = v43;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) + 3) = *(uint16_t *)(*(uint32_t *)(this_4 + 76) - 8) == 0;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) + 5) = *(uint16_t *)(*(uint32_t *)(this_4 + 80) - 8) == 0;
      v44 = *(uint32_t *)(this_4 + 72);
      v45 = *(uint32_t *)(this_4 + 68);
      v46 = *(uint32_t *)(this_4 + 64);
      v47 = *(uint32_t *)(this_4 + 60);
      v48 = *(uint32_t *)(this_4 + 56);
      *(uint32_t *)(this_4 + 72) = v45;
      *(uint32_t *)(this_4 + 68) = v46;
      *(uint32_t *)(this_4 + 64) = v47;
      *(uint32_t *)(this_4 + 60) = v48;
      *(uint32_t *)(this_4 + 56) = v44;
      v44 += 56;
      *(uint32_t *)(this_4 + 76) = v44;
      v48 += 56;
      *(uint32_t *)(this_4 + 80) = v48;
      *(uint32_t *)(this_4 + 84) = v47 + 56;
      *(uint32_t *)(this_4 + 88) = v46 + 56;
      *(uint32_t *)(this_4 + 92) = v45 + 56;
      LOBYTE(v48) = *(uint16_t *)(v48 + 8) == 0;
      *(uint8_t *)(v44 + 4) = v48;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) - 2) = v48;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) - 9) = v48;
      LOBYTE(v46) = *(uint16_t *)(*(uint32_t *)(this_4 + 80) + 16) == 0;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) + 6) = v46;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) - 1) = v46;
      *(uint8_t *)(*(uint32_t *)(this_4 + 76) + 7) = *(uint16_t *)(*(uint32_t *)(this_4 + 80) + 24) == 0;
      v49 = *(uint8_t **)(this_4 + 80);
      v50 = *(uint8_t **)(this_4 + 84);
      *(uint32_t *)(this_4 + 76) += 8;
      v49 += 8;
      *(uint32_t *)(this_4 + 88) += 8;
      *(uint32_t *)(this_4 + 80) = v49;
      v50 += 8;
      *(uint32_t *)(this_4 + 84) = v50;
      *(uint32_t *)(this_4 + 92) += 8;
      *(uint8_t *)(this_4 + 1078692) = v49[26] + v49[18] + v49[10] + v49[2] + v49[34] - 5;
      LOBYTE(v47) = v50[2];
      LOBYTE(v44) = v50[10];
      LOBYTE(v48) = v50[18];
      LOBYTE(v49) = v50[26];
      LOBYTE(v50) = v50[34];
      *(uint8_t *)(this_4 + 1078695) = 0;
      *(uint8_t *)(this_4 + 1078694) = 0;
      v51 = *(uint32_t *)this_4;
      *(uint8_t *)(this_4 + 1078693) = (uint8_t)v49 + v48 + v44 + v47 + (uint8_t)v50 - 5;
      v52 = v86;
      if ( v51 <= 0 )
        break;
      v53 = 0;
      do
      {
        v54 = __fwd_sub_417E80_sub_418650((uint32_t *)this_4, v53);
        __sub_416C90(this_4);
        v51 = *(uint32_t *)this_4;
        v53 += v54;
      }
      while ( v53 < *(uint32_t *)this_4 );
      v52 = v86;
      if ( n4 != 4 )
        goto LABEL_53;
      if ( v51 > 0 )
      {
        ArgList_6 = (uint32_t *)ArgList_5;
        v65 = 0;
        do
          *ArgList_6++ = *(uint32_t *)(*(uint32_t *)(this_4 + 1078240)
                                   + 4 * *(uint16_t *)(*(uint32_t *)(this_4 + 56) + 8 * v65++ + 64));
        while ( v65 < *(uint32_t *)this_4 );
        goto LABEL_73;
      }
LABEL_74:
      v43 = v52 + 1;
      if ( v43 >= *(uint32_t *)(this_4 + 4) )
      {
        ArgList_3 = (char *)ArgList_4;
        goto LABEL_76;
      }
    }
    if ( n4 == 4 )
      goto LABEL_74;
LABEL_53:
    if ( n4 == 3 )
    {
      if ( v51 > 0 )
      {
        ArgList_7 = (uint32_t *)ArgList_5;
        v56 = 0;
        do
        {
          v57 = *(uint32_t *)(this_4 + 1078240);
          v58 = *(uint16_t *)(*(uint32_t *)(this_4 + 56) + 8 * v56 + 64);
          *(uint16_t *)ArgList_7 = *(uint16_t *)(v57 + 4 * v58);
          *((uint8_t *)ArgList_7 + 2) = *(uint8_t *)(v57 + 4 * v58 + 2);
          ++v56;
          ArgList_7 = (uint32_t *)((char *)ArgList_7 + 3);
        }
        while ( v56 < *(uint32_t *)this_4 );
        ArgList_5 = (char *)ArgList_7;
      }
      goto LABEL_74;
    }
    if ( n4 != 2 )
    {
      if ( *(uint32_t *)(this_4 + 8) == 8 )
      {
        if ( v51 > 0 )
        {
          ArgList_8 = ArgList_5;
          v80 = 0;
          do
            *ArgList_8++ = *(uint8_t *)(*(uint32_t *)(this_4 + 1078240)
                                    + 4 * *(uint16_t *)(*(uint32_t *)(this_4 + 56) + 8 * v80++ + 64));
          while ( v80 < *(uint32_t *)this_4 );
          ArgList_5 = ArgList_8;
        }
      }
      else if ( v51 > 0 )
      {
        v87 = v52;
        v61 = 0;
        v62 = 0;
        ArgList_9 = ArgList_5;
        do
        {
          v64 = *(uint32_t *)(this_4 + 8);
          v62 -= v64;
          if ( v62 < 0 )
          {
            v62 = 8 - v64;
            *++ArgList_9 = *(uint32_t *)(*(uint32_t *)(this_4 + 1078240)
                                     + 4 * *(uint16_t *)(*(uint32_t *)(this_4 + 56) + 8 * v61 + 64)) << ((8 - v64) & 31);
          }
          else
          {
            *ArgList_9 |= *(uint32_t *)(*(uint32_t *)(this_4 + 1078240)
                                    + 4 * *(uint16_t *)(*(uint32_t *)(this_4 + 56) + 8 * v61 + 64)) << (v62 & 31);
          }
          ++v61;
        }
        while ( v61 < *(uint32_t *)this_4 );
        v52 = v87;
        ArgList_5 = ArgList_9;
      }
      goto LABEL_74;
    }
    if ( v51 > 0 )
    {
      ArgList_6 = (uint32_t *)ArgList_5;
      v60 = 0;
      do
      {
        *(uint16_t *)ArgList_6 = *(uint32_t *)(*(uint32_t *)(this_4 + 1078240)
                                        + 4 * *(uint16_t *)(*(uint32_t *)(this_4 + 56) + 8 * v60++ + 64));
        ArgList_6 = (uint32_t *)((char *)ArgList_6 + 2);
      }
      while ( v60 < *(uint32_t *)this_4 );
LABEL_73:
      ArgList_5 = (char *)ArgList_6;
      goto LABEL_74;
    }
    goto LABEL_74;
  }
LABEL_76:
  __sub_414920();
  v66 = *(uint32_t *)(this_4 + 12);
  if ( v66 != *(uint32_t *)(this_4 + 8) )
  {
    n6 = (v66 + 7) >> 3;
    if ( n6 <= 0 )
    {
      v68 = *(uint32_t *)(this_4 + 4) * *(uint32_t *)this_4;
    }
    else
    {
      n6_3 = 0;
      v68 = *(uint32_t *)(this_4 + 4) * *(uint32_t *)this_4;
      v69 = v68 / n6;
      if ( n6 >= 6 )
      {
        v84 = *(uint32_t *)(this_4 + 4) * *(uint32_t *)this_4;
        n6_4 = 0;
        this_1 = (uint32_t *)this_4;
        ArgList_10 = ArgList_3;
        do
        {
          (&v92)[n6_4] = ArgList_10;
          ArgList_10 += 5 * v69;
          *(&v93 + n6_4) = (int32_t)&ArgList_3[v69 * (n6_4 + 1)];
          *(&v94 + n6_4) = (int32_t)&ArgList_3[v69 * (n6_4 + 2)];
          *(&v95 + n6_4) = (int32_t)&ArgList_3[v69 * (n6_4 + 3)];
          *(&ArgList_4 + n6_4) = (int32_t)&ArgList_3[v69 * (n6_4 + 4)];
          n6_4 += 5;
        }
        while ( n6_4 <= n6 - 6 );
        this_4 = (int32_t)this_1;
        n6_3 = n6_4;
        v68 = v84;
      }
      n6_1 = n6_3;
      v73 = v69 * n6_3;
      this_1 = (uint32_t *)this_4;
      do
      {
        (&v92)[n6_1] = &ArgList_3[v73];
        v73 += v69;
        ++n6_1;
      }
      while ( n6_1 < n6 );
      this_4 = (int32_t)this_1;
    }
    if ( v68 > 0 )
    {
      Src_2 = Src_1;
      ArgList_4 = (int32_t)ArgList_3;
      n6_2 = 0;
      v76 = 0;
      do
      {
        v77 = (&v92)[n6_2];
        *Src_2 = *v77;
        v78 = *(uint32_t *)(this_4 + 4) * *(uint32_t *)this_4;
        ++Src_2;
        (&v92)[n6_2++] = v77 + 1;
        if ( n6_2 == n6 )
          n6_2 = 0;
        ++v76;
      }
      while ( v76 < v78 );
      ArgList_3 = (char *)ArgList_4;
    }
    free(ArgList_3);
  }
}
static inline int32_t __fwd_sub_424D90_sub_4135A0(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_4135A0((uint16_t *)a0, a1, a2, a3); }
static inline void ** __fwd_sub_424D90_sub_4244A0(void *a0, char a1) { return __sub_4244A0((void **)a0, a1); }
static inline int32_t __fwd_sub_424D90_sub_4248D0(void *a0, void *a1, int32_t a2) { return __sub_4248D0((uint8_t **)a0, (uint32_t *)a1, a2); }
static inline int32_t * __fwd_sub_424D90_sub_4256F0(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_4256F0((int32_t *)a0, a1, a2, a3); }

 int32_t __sub_424D90(uint16_t *p_i, int32_t a2)
{
  alignas(16) uint8_t __hexrays_frame[144];
  char &v90 = *(char *)(__hexrays_frame + 0);
  uint32_t &v91 = *(uint32_t *)(__hexrays_frame + 4);
  uint32_t &v92 = *(uint32_t *)(__hexrays_frame + 4);
  int32_t &v93 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v94 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v95 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v96 = *(int32_t *)(__hexrays_frame + 16);
  char &v97 = *(char *)(__hexrays_frame + 20);
  int32_t &v98 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v99 = *(int32_t *)(__hexrays_frame + 28);
  int32_t &n5_8 = *(int32_t *)(__hexrays_frame + 32);
  void * &Block = *(void * *)(__hexrays_frame + 36);
  uint8_t ** &v102 = *(uint8_t ** *)(__hexrays_frame + 40);
  uint8_t ** &v103 = *(uint8_t ** *)(__hexrays_frame + 44);
  uint8_t ** &v104 = *(uint8_t ** *)(__hexrays_frame + 48);
  int32_t &v105 = *(int32_t *)(__hexrays_frame + 52);
  int32_t &v106 = *(int32_t *)(__hexrays_frame + 56);
  uint32_t &i_1 = *(uint32_t *)(__hexrays_frame + 60);
  int32_t &v108 = *(int32_t *)(__hexrays_frame + 64);
  int32_t &v109 = *(int32_t *)(__hexrays_frame + 68);
  uint32_t &i_4 = *(uint32_t *)(__hexrays_frame + 72);
  int32_t &v111 = *(int32_t *)(__hexrays_frame + 76);
  int32_t &v112 = *(int32_t *)(__hexrays_frame + 80);
  int32_t &v113 = *(int32_t *)(__hexrays_frame + 84);
  int32_t &v114 = *(int32_t *)(__hexrays_frame + 88);
  int32_t &v115 = *(int32_t *)(__hexrays_frame + 92);
  int32_t &n5_6 = *(int32_t *)(__hexrays_frame + 96);
  int32_t &v117 = *(int32_t *)(__hexrays_frame + 100);
  int32_t &v118 = *(int32_t *)(__hexrays_frame + 104);
  int32_t &n5 = *(int32_t *)(__hexrays_frame + 108);
  ;
  char v11, v12, v13, v62, v71, v81, v83;
  int32_t i, v3, i_2, n4, *v6, *v7, v8, v9, v10, v14, v15, v16, v17, i_3, n4_1, n4_2, v23, v24,
          v25, v26, v27, v28, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43,
          v44, v45, v46, v47, v48, v49, n5_9, n5_7, n5_2, n5_1, v56, n16, v58, v59, v63, v64,
          v65, n16_1, n5_3, v69, n5_4, v73, v74, n16_2, v77, n3, n5_5, v84, n16_3, n4_3, n4_4;
  int64_t v68, v76, v86;
  uint32_t v19;
  uint8_t *v29, *v30, *v50, v53, v60, **v61, **v70, v79, **v80;
  void **v89;
  i = *p_i;
  v3 = p_i[1];
  i_1 = i;
  if ( __n4_5 > 0 )
  {
    i_2 = i;
    n4 = 0;
    do
    {
      v6 = (int32_t *)malloc(0x99D4D8u);
      if ( v6 )
        v7 = __fwd_sub_424D90_sub_4256F0(v6, i_2, v3, n4);
      else
        v7 = nullptr;
      *(&Block + n4++) = v7;
    }
    while ( n4 < __n4_5 );
  }
  v8 = 16 * (uint8_t)__byte_4433AD[0];
  v9 = 16 * (uint8_t)__byte_4433BD[0];
  v10 = 16 * (uint8_t)__n3_0;
  v11 = __byte_44339E[v8];
  v12 = __byte_44339E[v9];
  v13 = __byte_44339E[v10];
  v14 = (uint8_t)__byte_44339F[v9];
  LOBYTE(v10) = __byte_44339F[v10];
  v105 = (uint8_t)__byte_44339F[v8];
  v106 = v14;
  v15 = v11 & 8;
  v90 = v10;
  v16 = v12 & 8;
  v17 = v13 & 8;
  __sub_414F60();
  if ( v3 > 0 )
  {
    v96 = v17;
    v19 = 0;
    v108 = v16;
    i_3 = i_1;
    v109 = v15;
    n4_1 = __n4_5;
    do
    {
      if ( n4_1 > 0 )
      {
        v91 = v19;
        n4_2 = 0;
        v93 = v3;
        v95 = a2;
        do
        {
          ++n4_2;
          v23 = *(&n5_8 + n4_2);
          **(uint16_t **)(v23 + 196) = *(uint16_t *)(*(uint32_t *)(v23 + 196) - 2);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) + 2) = *(uint16_t *)(*(uint32_t *)(v23 + 196) - 4);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) + 4) = *(uint16_t *)(*(uint32_t *)(v23 + 196) - 6);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) + 6) = *(uint16_t *)(*(uint32_t *)(v23 + 196) - 8);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) + 8) = *(uint16_t *)(*(uint32_t *)(v23 + 196) - 10);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) + 10) = *(uint16_t *)(*(uint32_t *)(v23 + 196) - 12);
          v24 = *(uint32_t *)(v23 + 192);
          v25 = *(uint32_t *)(v23 + 188);
          v26 = *(uint32_t *)(v23 + 184);
          v27 = *(uint32_t *)(v23 + 180);
          v28 = *(uint32_t *)(v23 + 176);
          *(uint32_t *)(v23 + 192) = v25;
          *(uint32_t *)(v23 + 188) = v26;
          *(uint32_t *)(v23 + 184) = v27;
          *(uint32_t *)(v23 + 180) = v28;
          *(uint32_t *)(v23 + 176) = v24;
          v24 += 8;
          *(uint32_t *)(v23 + 196) = v24;
          v28 += 8;
          *(uint32_t *)(v23 + 200) = v28;
          *(uint32_t *)(v23 + 204) = v27 + 8;
          *(uint32_t *)(v23 + 208) = v26 + 8;
          *(uint32_t *)(v23 + 212) = v25 + 8;
          *(uint16_t *)(v24 - 8) = *(uint16_t *)(v28 + 6);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) - 6) = *(uint16_t *)(*(uint32_t *)(v23 + 200) + 4);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) - 4) = *(uint16_t *)(*(uint32_t *)(v23 + 200) + 2);
          *(uint16_t *)(*(uint32_t *)(v23 + 196) - 2) = **(uint16_t **)(v23 + 200);
          v29 = *(uint8_t **)(v23 + 204);
          v30 = *(uint8_t **)(v23 + 212);
          *(uint32_t *)(v23 + 20) = 0;
          *(uint32_t *)(v23 + 24) = 0;
          *(uint32_t *)(v23 + 28) = 0;
          v31 = *(v29 - 3);
          *(uint32_t *)(v23 + 24) = v31;
          v32 = *(v29 - 1);
          *(uint32_t *)(v23 + 28) = v32;
          v33 = *(v30 - 3) + v31;
          *(uint32_t *)(v23 + 24) = v33;
          v34 = *(v30 - 1) + v32;
          *(uint32_t *)(v23 + 28) = v34;
          v35 = v29[1] + v33;
          *(uint32_t *)(v23 + 24) = v35;
          v36 = v29[3] + v34;
          *(uint32_t *)(v23 + 28) = v36;
          v37 = v30[1] + v35;
          *(uint32_t *)(v23 + 24) = v37;
          v38 = v30[3] + v36;
          *(uint32_t *)(v23 + 28) = v38;
          v39 = v29[5] + v37;
          *(uint32_t *)(v23 + 24) = v39;
          v40 = v29[7] + v38;
          *(uint32_t *)(v23 + 28) = v40;
          v41 = v30[5] + v39;
          *(uint32_t *)(v23 + 24) = v41;
          v42 = v30[7] + v40;
          *(uint32_t *)(v23 + 28) = v42;
          v43 = v29[9] + v41;
          *(uint32_t *)(v23 + 24) = v43;
          v44 = v29[11] + v42;
          *(uint32_t *)(v23 + 28) = v44;
          v45 = v30[9] + v43;
          v46 = *(uint32_t *)(v23 + 196);
          *(uint32_t *)(v23 + 24) = v45;
          v47 = v30[11] + v44;
          *(uint32_t *)(v23 + 28) = v47;
          v48 = *(uint8_t *)(v46 - 7) + v45;
          *(uint32_t *)(v23 + 24) = v48;
          v49 = *(uint8_t *)(v46 - 5) + v47;
          *(uint32_t *)(v23 + 28) = v49;
          *(uint32_t *)(v23 + 24) = *(uint8_t *)(v46 - 3) + v48;
          *(uint32_t *)(v23 + 28) = *(uint8_t *)(v46 - 1) + v49;
          n4_1 = __n4_5;
        }
        while ( n4_2 < __n4_5 );
        v19 = v91;
        v3 = v93;
        i_3 = i_1;
        a2 = v95;
      }
      if ( i_3 > 0 )
      {
        v92 = v19;
        i_4 = 0;
        v94 = v3;
        do
        {
          v50 = (uint8_t *)Block;
          n5_9 = *(uint8_t *)(a2 + (uint8_t)__byte_44339D[0]);
          v114 = (uint8_t)__byte_44339D[0];
          n5_6 = n5_9;
          __fwd_sub_424D90_sub_4248D0((uint8_t **)Block, nullptr, 0);
          n5_7 = n5_6;
          v53 = v50[8];
          v112 = (uint8_t)(n5_6 - v53);
          n5_2 = v50[v112 + 984];
          n5_1 = (uint8_t)(v50[n5_2 + 1496] + v53);
          v56 = (uint8_t)(n5_1 + *(uint8_t *)(a2 + v114) - n5_6);
          n16 = *(uint8_t *)(a2 + v114) - v56;
          if ( n16 < -16 || n16 > 16 )
          {
            n5_2 = v50[v112 + 1240];
          }
          else
          {
            n5_6 = n5_1;
            *(uint8_t *)(a2 + v114) = v56;
            n5_7 = n5_1;
          }
          __fwd_sub_424D90_sub_4135A0((uint16_t *)&v50[16 * *((uint32_t *)v50 + 3) + 3800], n5_1, *((uint32_t *)v50 + 4), n5_2);
          v58 = n5_7 - *((uint32_t *)v50 + 2);
          **((uint8_t **)v50 + 49) = n5_6;
          *(uint8_t *)(*((uint32_t *)v50 + 49) + 1) = abs32(v58);
          *(uint32_t *)&v50[4 * *((uint32_t *)v50 + 5) + 24] = *(uint32_t *)&v50[4 * *((uint32_t *)v50 + 5) + 24]
                                                         + *(uint8_t *)(*((uint32_t *)v50 + 49) + 1)
                                                         - *(uint8_t *)(*((uint32_t *)v50 + 49) - 7)
                                                         - (*(uint8_t *)(*((uint32_t *)v50 + 53) - 3)
                                                          - *(uint8_t *)(*((uint32_t *)v50 + 53) + 13)
                                                          + *(uint8_t *)(*((uint32_t *)v50 + 51) - 3)
                                                          - *(uint8_t *)(*((uint32_t *)v50 + 51) + 13));
          v59 = 16 * *((uint32_t *)v50 + 3);
          *((uint32_t *)v50 + 5) = *((uint32_t *)v50 + 5) == 0;
          if ( *(uint16_t *)&v50[v59 + 3800] < 0x4000u )
            __sub_4259F0((int32_t)v50);
          *((uint32_t *)v50 + 49) += 2;
          *((uint32_t *)v50 + 50) += 2;
          *((uint32_t *)v50 + 51) += 2;
          *((uint32_t *)v50 + 52) += 2;
          *((uint32_t *)v50 + 53) += 2;
          v60 = *(uint8_t *)(a2 + (uint8_t)__byte_4433AD[0]);
          v113 = (uint8_t)__byte_4433AD[0];
          if ( v109 )
            v60 = v60 - v105 - *(uint8_t *)((uint8_t)__byte_44339D[0] + a2);
          v61 = v102;
          v118 = v60;
          __fwd_sub_424D90_sub_4248D0(v102, Block, 0);
          v62 = *((uint8_t *)v61 + 8);
          v63 = (uint8_t)(v118 - v62);
          n5 = *((uint8_t *)v61 + v63 + 984);
          v64 = *(uint8_t *)(a2 + v113);
          v65 = (uint8_t)(*((uint8_t *)v61 + n5 + 1496) + v62);
          v111 = (uint8_t)(v65 + *(uint8_t *)(a2 + v113) - v118);
          n16_1 = v64 - v111;
          n5_3 = n5;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            n5_3 = *((uint8_t *)v61 + v63 + 1240);
          }
          else
          {
            *(uint8_t *)(a2 + v113) = v111;
            v118 = v65;
          }
          __fwd_sub_424D90_sub_4135A0((uint16_t *)&v61[4 * (uint32_t)v61[3] + 950], 16 * (uint32_t)v61[3], (int32_t)v61[4], n5_3);
          v68 = v118 - (int32_t)v61[2];
          *v61[49] = v118;
          v61[49][1] = (BYTE4(v68) ^ v68) - BYTE4(v68);
          v61[(uint32_t)v61[5] + 6] = &v61[(uint32_t)v61[5] + 6][v61[49][1]
                                                           - *(v61[49] - 7)
                                                           - (*(v61[53] - 3)
                                                            - v61[53][13])
                                                           - (*(v61[51] - 3)
                                                            - v61[51][13])];
          LODWORD(v68) = 16 * (uint32_t)v61[3];
          v61[5] = (uint8_t *)(v61[5] == nullptr);
          if ( *(uint16_t *)((char *)v61 + v68 + 3800) < 0x4000u )
            __sub_4259F0((int32_t)v61);
          v61[49] += 2;
          v61[50] += 2;
          v61[51] += 2;
          v61[52] += 2;
          v61[53] += 2;
          v69 = *(uint8_t *)((uint8_t)__byte_4433BD[0] + a2);
          v117 = (uint8_t)__byte_4433BD[0];
          if ( v108 )
            v69 = (uint8_t)(v69
                                  - v106
                                  - ((__dword_4433A0[4 * (uint8_t)__byte_4433BD[0]]
                                    * *(uint8_t *)((uint8_t)__byte_44339D[0] + a2)
                                    + __dword_4433A4[4 * (uint8_t)__byte_4433BD[0]]
                                    * (uint32_t)*(uint8_t *)((uint8_t)__byte_4433AD[0] + a2)
                                    + 40) >> 7));
          v70 = v103;
          __fwd_sub_424D90_sub_4248D0(v103, (uint32_t *)v102, (int32_t)Block);
          v71 = *((uint8_t *)v70 + 8);
          v115 = (uint8_t)(v69 - v71);
          n5_4 = *((uint8_t *)v70 + v115 + 984);
          v73 = (uint8_t)(*((uint8_t *)v70 + n5_4 + 1496) + v71);
          v74 = (uint8_t)(v73 + *(uint8_t *)(v117 + a2) - v69);
          n16_2 = *(uint8_t *)(v117 + a2) - v74;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            n5_4 = *((uint8_t *)v70 + v115 + 1240);
          }
          else
          {
            v69 = v73;
            *(uint8_t *)(v117 + a2) = v74;
          }
          __fwd_sub_424D90_sub_4135A0((uint16_t *)&v70[4 * (uint32_t)v70[3] + 950], n5_4, (int32_t)v70[4], n5_4);
          v76 = v69 - (int32_t)v70[2];
          *v70[49] = v69;
          v70[49][1] = (BYTE4(v76) ^ v76) - BYTE4(v76);
          v70[(uint32_t)v70[5] + 6] = &v70[(uint32_t)v70[5] + 6][v70[49][1]
                                                           - *(v70[49] - 7)
                                                           - (*(v70[53] - 3)
                                                            - v70[53][13])
                                                           - (*(v70[51] - 3)
                                                            - v70[51][13])];
          v77 = 4 * (uint32_t)v70[3];
          v70[5] = (uint8_t *)(v70[5] == nullptr);
          if ( LOWORD(v70[v77 + 950]) < 0x4000u )
            __sub_4259F0((int32_t)v70);
          v70[49] += 2;
          v70[50] += 2;
          v70[51] += 2;
          v70[52] += 2;
          v70[53] += 2;
          n4_1 = __n4_5;
          if ( __n4_5 >= 4 )
          {
            n3 = (uint8_t)__n3_0;
            if ( v96 )
              v79 = *(uint8_t *)((uint8_t)__n3_0 + a2)
                  - v90
                  - ((__dword_4433A4[4 * (uint8_t)__n3_0] * *(uint8_t *)((uint8_t)__n3_0 + a2 - 2)
                    + __dword_4433A0[4 * (uint8_t)__n3_0] * *(uint8_t *)((uint8_t)__n3_0 + a2 - 3)
                    + __dword_4433A8[4 * (uint8_t)__n3_0] * *(uint8_t *)((uint8_t)__n3_0 + a2 - 1)
                    + 64) >> 7);
            else
              v79 = *(uint8_t *)((uint8_t)__n3_0 + a2);
            v80 = v104;
            v99 = v79;
            __fwd_sub_424D90_sub_4248D0(v104, (uint32_t *)v103, (int32_t)v102);
            v81 = *((uint8_t *)v80 + 8);
            n5_5 = *((uint8_t *)v80 + (uint8_t)(v99 - v81) + 984);
            v98 = (uint8_t)(v99 - v81);
            v83 = *((uint8_t *)v80 + n5_5 + 1496);
            n5_8 = n5_5;
            v84 = (uint8_t)(v83 + v81);
            n16_3 = *(uint8_t *)(n3 + a2) - (uint8_t)(v84 + *(uint8_t *)(n3 + a2) - v99);
            v97 = v84 + *(uint8_t *)(n3 + a2) - v99;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              n5_5 = *((uint8_t *)v80 + v98 + 1240);
            }
            else
            {
              v99 = v84;
              *(uint8_t *)(n3 + a2) = v97;
            }
            __fwd_sub_424D90_sub_4135A0((uint16_t *)&v80[4 * (uint32_t)v80[3] + 950], n5_5, (int32_t)v80[4], n5_5);
            v86 = v99 - (int32_t)v80[2];
            *v80[49] = v99;
            v80[49][1] = (BYTE4(v86) ^ v86) - BYTE4(v86);
            v80[(uint32_t)v80[5] + 6] = &v80[(uint32_t)v80[5] + 6][v80[49][1]
                                                             - *(v80[49] - 7)
                                                             - (*(v80[53] - 3)
                                                              - v80[53][13])
                                                             - (*(v80[51] - 3)
                                                              - v80[51][13])];
            LODWORD(v86) = 16 * (uint32_t)v80[3];
            v80[5] = (uint8_t *)(v80[5] == nullptr);
            if ( *(uint16_t *)((char *)v80 + v86 + 3800) < 0x4000u )
              __sub_4259F0((int32_t)v80);
            v80[49] += 2;
            v80[50] += 2;
            v80[51] += 2;
            v80[52] += 2;
            v80[53] += 2;
            n4_1 = __n4_5;
          }
          a2 += n4_1;
          ++i_4;
        }
        while ( i_4 < i_1 );
        v19 = v92;
        v3 = v94;
        i_3 = i_1;
      }
      ++v19;
    }
    while ( v19 < v3 );
  }
  __sub_414CE0();
  n4_3 = __n4_5;
  if ( __n4_5 > 0 )
  {
    n4_4 = 0;
    do
    {
      v89 = (void **)*(&Block + n4_4);
      if ( v89 )
      {
        __fwd_sub_424D90_sub_4244A0(v89, 1);
        n4_3 = __n4_5;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline int32_t __fwd_sub_41CAB0_sub_413430(void *a0, int32_t a1) { return __sub_413430((uint16_t *)a0, a1); }
static inline uint32_t __fwd_sub_41CAB0_sub_414800(void *a0) { return __sub_414800((uint16_t *)a0); }

BMF_SSE uint32_t __sub_41CAB0(int32_t a1, const __m128 &a2__ref, int32_t a3, uint8_t a4, int32_t a5)
{
  alignas(16) uint8_t __hexrays_frame[416];
  int32_t &n2 = *(int32_t *)(__hexrays_frame + 0);
  uint16_t * &n0xF0 = *(uint16_t * *)(__hexrays_frame + 4);
  uint32_t &n0x10_2 = *(uint32_t *)(__hexrays_frame + 8);
  uint16_t * &v508 = *(uint16_t * *)(__hexrays_frame + 12);
  uint32_t &n0x10_1 = *(uint32_t *)(__hexrays_frame + 16);
  int32_t &v510 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &v511 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v512 = *(int32_t *)(__hexrays_frame + 64);
  int32_t &v513 = *(int32_t *)(__hexrays_frame + 68);
  int32_t &v514 = *(int32_t *)(__hexrays_frame + 72);
  const char * &v515 = *(const char * *)(__hexrays_frame + 80);
  uint32_t &v516 = *(uint32_t *)(__hexrays_frame + 84);
  const char * &v517 = *(const char * *)(__hexrays_frame + 88);
  const char * &v518 = *(const char * *)(__hexrays_frame + 96);
  const char * &v519 = *(const char * *)(__hexrays_frame + 100);
  const char * &v520 = *(const char * *)(__hexrays_frame + 104);
  const char * &v521 = *(const char * *)(__hexrays_frame + 112);
  const char * &v522 = *(const char * *)(__hexrays_frame + 116);
  const char * &v523 = *(const char * *)(__hexrays_frame + 120);
  const char * &v524 = *(const char * *)(__hexrays_frame + 128);
  int32_t &v525 = *(int32_t *)(__hexrays_frame + 132);
  const char * &v526 = *(const char * *)(__hexrays_frame + 136);
  const char * &v527 = *(const char * *)(__hexrays_frame + 144);
  const char * &v528 = *(const char * *)(__hexrays_frame + 148);
  const char * &v529 = *(const char * *)(__hexrays_frame + 152);
  const char * &v530 = *(const char * *)(__hexrays_frame + 160);
  const char * &v531 = *(const char * *)(__hexrays_frame + 164);
  const char * &v532 = *(const char * *)(__hexrays_frame + 168);
  const char * &v533 = *(const char * *)(__hexrays_frame + 176);
  const char * &v534 = *(const char * *)(__hexrays_frame + 180);
  const char * &v535 = *(const char * *)(__hexrays_frame + 184);
  int32_t &v536 = *(int32_t *)(__hexrays_frame + 192);
  const char * &v537 = *(const char * *)(__hexrays_frame + 196);
  int32_t &v538 = *(int32_t *)(__hexrays_frame + 200);
  int32_t &v539 = *(int32_t *)(__hexrays_frame + 208);
  const char * &v540 = *(const char * *)(__hexrays_frame + 212);
  int32_t &v541 = *(int32_t *)(__hexrays_frame + 216);
  int32_t &v542 = *(int32_t *)(__hexrays_frame + 224);
  const char * &v543 = *(const char * *)(__hexrays_frame + 228);
  const char * &v544 = *(const char * *)(__hexrays_frame + 232);
  uint32_t &v545 = *(uint32_t *)(__hexrays_frame + 240);
  int32_t &n3 = *(int32_t *)(__hexrays_frame + 244);
  int32_t &v547 = *(int32_t *)(__hexrays_frame + 248);
  int32_t &v548 = *(int32_t *)(__hexrays_frame + 252);
  int32_t &v549 = *(int32_t *)(__hexrays_frame + 256);
  int32_t &v550 = *(int32_t *)(__hexrays_frame + 260);
  const char * &v551 = *(const char * *)(__hexrays_frame + 264);
  const char * &v552 = *(const char * *)(__hexrays_frame + 268);
  const char * &v553 = *(const char * *)(__hexrays_frame + 272);
  const char * &v554 = *(const char * *)(__hexrays_frame + 276);
  const char * &v555 = *(const char * *)(__hexrays_frame + 280);
  const char * &v556 = *(const char * *)(__hexrays_frame + 284);
  const char * &v557 = *(const char * *)(__hexrays_frame + 288);
  int32_t &v558 = *(int32_t *)(__hexrays_frame + 292);
  const char * &v559 = *(const char * *)(__hexrays_frame + 296);
  const char * &v560 = *(const char * *)(__hexrays_frame + 300);
  const char * &v561 = *(const char * *)(__hexrays_frame + 304);
  const char * &v562 = *(const char * *)(__hexrays_frame + 308);
  const char * &v563 = *(const char * *)(__hexrays_frame + 312);
  const char * &v564 = *(const char * *)(__hexrays_frame + 316);
  const char * &v565 = *(const char * *)(__hexrays_frame + 320);
  uint32_t &v566 = *(uint32_t *)(__hexrays_frame + 324);
  int32_t &v567 = *(int32_t *)(__hexrays_frame + 328);
  int32_t &v568 = *(int32_t *)(__hexrays_frame + 332);
  const char * &v569 = *(const char * *)(__hexrays_frame + 336);
  int32_t &v570 = *(int32_t *)(__hexrays_frame + 340);
  int32_t &v571 = *(int32_t *)(__hexrays_frame + 344);
  int32_t &v572 = *(int32_t *)(__hexrays_frame + 348);
  int32_t &v573 = *(int32_t *)(__hexrays_frame + 352);
  int32_t &v574 = *(int32_t *)(__hexrays_frame + 356);
  int32_t &v575 = *(int32_t *)(__hexrays_frame + 360);
  int32_t &v576 = *(int32_t *)(__hexrays_frame + 364);
  int32_t &v577 = *(int32_t *)(__hexrays_frame + 368);
  uint32_t * &v578 = *(uint32_t * *)(__hexrays_frame + 372);
  uint32_t &n5 = *(uint32_t *)(__hexrays_frame + 376);
  int32_t &v580 = *(int32_t *)(__hexrays_frame + 380);
  uint32_t * &v581 = *(uint32_t * *)(__hexrays_frame + 384);
  ;
  __m128 a2 = a2__ref;
  __m128 *v15, *v17, v19, n0x46D22000, v27, n0x459D8800, n0x3C54FDF4, v30, v31, v32, v33, v34,
         v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,
         v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63, v64, v65, v66, v67, v68,
         v69, v70, v71, v72, v73, v74;
  bool v87, v103, v104, v105;
  char *v90, v114, v116, v312;
  const char *v91, *v93, *v94, *v107, *v110, *v111, *v112;
  float v16, v18, v20, v21, v22, v23, v24, v26;
  int32_t v6, v8, v9, v75, v77, v79, v80, v81, v84, v85, v86, v88, v89, v95, v96, v98, n2_1,
          v100, v101, v102, v106, v108, v113, v115, v117, v118, v119, v120, v121, v122, v123,
          v124, v125, v126, v127, v128, v129, v130, v131, v132, v133, v134, v135, v136, v137,
          v138, v139, v140, v141, v142, v143, v144, v145, v146, v147, v148, v149, v150, v151,
          v152, v153, v154, v155, v156, v157, v158, v159, v160, v161, v162, v163, v164, v165,
          v166, v167, v168, v169, v170, v171, v172, v173, v174, v175, v176, v177, v178, v179,
          v180, v181, v182, v183, v184, v185, v186, v187, v188, v189, v190, v191, v192, v193,
          v194, v195, v196, v197, v198, v199, v200, v201, v202, v203, v204, v205, v206, v207,
          v208, v209, v210, v211, v212, v213, v214, v215, v216, v217, v218, v219, v220, v221,
          v222, v223, v224, v225, v226, v227, v228, v229, v230, v231, v232, v233, v234, v235,
          v236, v237, v238, v239, v240, v241, v242, v243, v244, v245, v246, v247, v248, v249,
          v250, v251, v252, v253, v254, v255, v256, v257, v258, v259, v260, v261, v262, v263,
          v264, v265, v266, v267, v268, v269, v270, v271, v272, v273, v274, v275, v276, v277,
          v278, v279, v280, v281, v282, v283, v284, v285, v286, v287, v288, v289, v290, v291,
          v292, v293, v294, v295, v296, v297, v298, v299, v300, v301, v302, v303, v304, v305,
          v306, v307, v308, v309, v310, v311, v313, v314, v315, v316, v317, v318, v319, v320,
          v321, v322, v323, v324, v325, v326, v327, v328, v329, v330, v331, v332, v333, v334,
          v335, v336, v337, v338, v339, v340, v341, v342, v343, v344, v345, v346, v347, v348,
          v349, v350, v351, v352, v353, v354, v355, v356, v357, v358, v359, v360, v361, v362,
          v363, v364, v365, v366, v367, v368, v369, v370, v371, v372, v373, v374, v375, v376,
          v377, v378, v379, v380, v381, v382, v383, v384, v386, n15, v391, v392, v394, v395,
          v397, v398, v399, v400, v403, v404, v406, v407, v409, v410, v412, v413, v415, v416,
          v417, v418, v419, v421, v422, v423, v424, v425, v426, v427, v429, v430, v431, v432,
          v433, v434, v435, v437, v438, v439, v440, v441, v442, v443, v447, v448, v449, v450,
          v453, v454, v459, v460, v461, v464, v465, v467, v468, v470, v471, v472, v475, v476,
          v478, v479, v481, v482, v483, v486, v487, v489, v490, v492, v493, v494, v497, v498,
          v500, v501, v503, v504;
  int64_t v10, v11, v12, v13, v14;
  uint16_t *v7, *v387, *v389, *n0x10_3, *n2_2, *v408, *v411, *v420, *v428, *v436, *v444, *v445,
           *n2_7, *n0x10_4, *v452, *n0xF0_5, *v456, *v457, *n2_6, *n0xF0_4, *v463, *v466, *n2_5,
           *n0xF0_3, *v474, *v477, *n2_4, *n0xF0_2, *v485, *v488, *v491, *n0xF0_1, *n2_3, *v502;
  uint32_t *v76, v78, v92, v97, v109, *v385, n0x10, v393, v396, v405, v414, v499;
  uint8_t *v82, v83;
  v6 = *(uint32_t *)(a1 + 278704) & 0xF;
  v7 = *(uint16_t **)(a1 + 278736);
  v577 = 16 * a3;
  *v7 = 16 * a3;
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 2) = **(uint16_t **)(a1 + 278736) - *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 2);
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 20) = 0;
  *(uint8_t *)(*(uint32_t *)(a1 + 278736) + 16) = (a5 <= (int32_t)(((uint32_t)(6 - v6) >> 31)
                                                         + ((uint32_t)(4 - v6) >> 31)
                                                         + 2 * ((uint32_t)(9 - v6) >> 31)))
                                            + (a5 < (int32_t)-(((uint32_t)(6 - v6) >> 31)
                                                                + ((uint32_t)(4 - v6) >> 31)
                                                                + 2 * ((uint32_t)(9 - v6) >> 31)));
  *(uint8_t *)(*(uint32_t *)(a1 + 278736) + 17) = abs32(a5);
  v8 = *(uint32_t *)(a1 + 278736);
  v9 = v577;
  a2.m128_f32[0] = (float)v577;
  v19 = a2;
  v10 = v577 - *(int16_t *)(v8 - 18);
  *(uint16_t *)(v8 + 8) = (WORD2(v10) ^ v10) - WORD2(v10);
  v11 = v9 - **(int16_t **)(a1 + 278740);
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 10) = (WORD2(v11) ^ v11) - WORD2(v11);
  v12 = v9 - *(int16_t *)(*(uint32_t *)(a1 + 278740) - 18);
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 12) = (WORD2(v12) ^ v12) - WORD2(v12);
  v13 = v9 - *(int16_t *)(*(uint32_t *)(a1 + 278740) + 18);
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 14) = (WORD2(v13) ^ v13) - WORD2(v13);
  v14 = (int16_t)(v9 - *(uint16_t *)(a1 + 278700));
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 4) = v14;
  *(uint16_t *)(*(uint32_t *)(a1 + 278736) + 6) = (WORD2(v14) ^ v14) - WORD2(v14);
  v15 = *(__m128 **)(a1 + 278656);
  v16 = v15[14].m128_f32[1] + 0.000099999997f;
  v17 = *(__m128 **)(*(uint32_t *)(a1 + 278668) - 4);
  v18 = *(float *)(a1 + 278648);
  n2 = *(int32_t *)(a1 + 278640);
  v19.m128_f32[0] = a2.m128_f32[0] - v18;
  v20 = *(float *)(a1 + 278644);
  v21 = v20 - v18;
  v22 = (float)((float)((float)((float)(a2.m128_f32[0] - v18) * (float)(v20 - v18)) - v15[14].m128_f32[0]) * 0.001f)
      + v15[14].m128_f32[0];
  v23 = v16 + (float)((float)((float)(v21 * v21) - v15[14].m128_f32[1]) * 0.001f);
  v15[14].m128_f32[1] = v23;
  v24 = 0.1f * v23;
  if ( (float)(0.1f * v23) <= v22 )
    v24 = fminf(v23, v22);
  v15[14].m128_f32[0] = v24;
  n0x46D22000 = (__m128)0x46D22000u;
  n0x46D22000.m128_f32[0] = 26896.0f * v15[14].m128_f32[2];
  v26 = (float)(1.0f - (float)(v24 / (float)(v23 + 576.0f))) * 2.0f;
  v27 = a2;
  v19.m128_f32[0] = v19.m128_f32[0] * v26;
  n0x459D8800 = (__m128)0x459D8800u;
  n0x459D8800.m128_f32[0] = 5041.0f * v17[14].m128_f32[2];
  v27.m128_f32[0] = (float)(a2.m128_f32[0] - v20) * 2.5999999f;
  n0x3C54FDF4 = (__m128)0x3C54FDF4u;
  n0x3C54FDF4.m128_f32[0] = 0.013f * v26;
  v30 = _mm_unpacklo_ps(v27, v27);
  v31 = _mm_movelh_ps(v30, v30);
  v32 = _mm_unpacklo_ps(n0x46D22000, n0x46D22000);
  v33 = _mm_movelh_ps(v32, v32);
  v34 = _mm_unpacklo_ps(n0x3C54FDF4, n0x3C54FDF4);
  v35 = _mm_movelh_ps(v34, v34);
  v36 = _mm_unpacklo_ps(v19, v19);
  v37 = _mm_movelh_ps(v36, v36);
  v38 = _mm_unpacklo_ps(n0x459D8800, n0x459D8800);
  v39 = _mm_movelh_ps(v38, v38);
  v40 = *(__m128 *)(a1 + 278528);
  n0xF0 = (uint16_t *)a2.m128_i32[0];
  v41 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v40, v40), v15[7]), (__m128)__xmmword_439B40), v15[7]);
  v15[7] = v41;
  *v15 = _mm_add_ps(
           _mm_div_ps(
             _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_441190, v31), *(__m128 *)(a1 + 278528)),
             _mm_add_ps(v41, v33)),
           *v15);
  v42 = *(__m128 *)(a1 + 278528);
  n5 = 0;
  v43 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v42, v42), v17[7]), v35), v17[7]);
  v577 = v9;
  v17[7] = v43;
  *v17 = _mm_add_ps(
           _mm_div_ps(
             _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_441190, v37), *(__m128 *)(a1 + 278528)),
             _mm_add_ps(v43, v39)),
           *v17);
  v44 = *(__m128 *)(a1 + 278544);
  v578 = (uint32_t *)a1;
  v45 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_mul_ps(v44, v44), v15[8]), (__m128)__xmmword_439B40), v15[8]);
  v15[8] = v45;
  v15[1] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411A0, v31), *(__m128 *)(a1 + 278544)),
               _mm_add_ps(v45, v33)),
             v15[1]);
  v46 = _mm_add_ps(
          _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278544), *(__m128 *)(a1 + 278544)), v17[8]), v35),
          v17[8]);
  v17[8] = v46;
  v17[1] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411A0, v37), *(__m128 *)(a1 + 278544)),
               _mm_add_ps(v46, v39)),
             v17[1]);
  v47 = _mm_add_ps(
          _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278560), *(__m128 *)(a1 + 278560)), v15[9]),
            (__m128)__xmmword_439B40),
          v15[9]);
  v15[9] = v47;
  v15[2] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411B0, v31), *(__m128 *)(a1 + 278560)),
               _mm_add_ps(v47, v33)),
             v15[2]);
  v48 = _mm_add_ps(
          _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278560), *(__m128 *)(a1 + 278560)), v17[9]), v35),
          v17[9]);
  v17[9] = v48;
  v17[2] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411B0, v37), *(__m128 *)(a1 + 278560)),
               _mm_add_ps(v48, v39)),
             v17[2]);
  v49 = _mm_add_ps(
          _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278576), *(__m128 *)(a1 + 278576)), v15[10]),
            (__m128)__xmmword_439B40),
          v15[10]);
  v15[10] = v49;
  v15[3] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411C0, v31), *(__m128 *)(a1 + 278576)),
               _mm_add_ps(v49, v33)),
             v15[3]);
  v50 = _mm_add_ps(
          _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278576), *(__m128 *)(a1 + 278576)), v17[10]), v35),
          v17[10]);
  v17[10] = v50;
  v17[3] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411C0, v37), *(__m128 *)(a1 + 278576)),
               _mm_add_ps(v50, v39)),
             v17[3]);
  v51 = _mm_add_ps(
          _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278592), *(__m128 *)(a1 + 278592)), v15[11]),
            (__m128)__xmmword_439B40),
          v15[11]);
  v15[11] = v51;
  v15[4] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411D0, v31), *(__m128 *)(a1 + 278592)),
               _mm_add_ps(v51, v33)),
             v15[4]);
  v52 = _mm_add_ps(
          _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278592), *(__m128 *)(a1 + 278592)), v17[11]), v35),
          v17[11]);
  v17[11] = v52;
  v17[4] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411D0, v37), *(__m128 *)(a1 + 278592)),
               _mm_add_ps(v52, v39)),
             v17[4]);
  v53 = _mm_add_ps(
          _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278608), *(__m128 *)(a1 + 278608)), v15[12]),
            (__m128)__xmmword_439B40),
          v15[12]);
  v15[12] = v53;
  v15[5] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411E0, v31), *(__m128 *)(a1 + 278608)),
               _mm_add_ps(v53, v33)),
             v15[5]);
  v54 = _mm_add_ps(
          _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278608), *(__m128 *)(a1 + 278608)), v17[12]), v35),
          v17[12]);
  v17[12] = v54;
  v17[5] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411E0, v37), *(__m128 *)(a1 + 278608)),
               _mm_add_ps(v54, v39)),
             v17[5]);
  v55 = _mm_add_ps(
          _mm_mul_ps(
            _mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278624), *(__m128 *)(a1 + 278624)), v15[13]),
            (__m128)__xmmword_439B40),
          v15[13]);
  v15[13] = v55;
  v15[6] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411F0, v31), *(__m128 *)(a1 + 278624)),
               _mm_add_ps(v55, v33)),
             v15[6]);
  v56 = _mm_add_ps(
          _mm_mul_ps(v35, _mm_sub_ps(_mm_mul_ps(*(__m128 *)(a1 + 278624), *(__m128 *)(a1 + 278624)), v17[13])),
          v17[13]);
  v17[13] = v56;
  v17[6] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411F0, v37), *(__m128 *)(a1 + 278624)),
               _mm_add_ps(v56, v39)),
             v17[6]);
  v57 = (__m128)(uint32_t)n0xF0;
  v58 = _mm_add_ps(
          _mm_add_ps(
            _mm_add_ps(
              _mm_add_ps(
                _mm_add_ps(
                  _mm_add_ps(_mm_mul_ps(*v15, *(__m128 *)(a1 + 278528)), _mm_mul_ps(v15[1], *(__m128 *)(a1 + 278544))),
                  _mm_mul_ps(v15[2], *(__m128 *)(a1 + 278560))),
                _mm_mul_ps(v15[3], *(__m128 *)(a1 + 278576))),
              _mm_mul_ps(v15[4], *(__m128 *)(a1 + 278592))),
            _mm_mul_ps(v15[5], *(__m128 *)(a1 + 278608))),
          _mm_mul_ps(v15[6], *(__m128 *)(a1 + 278624)));
  v59 = _mm_add_ps(v58, _mm_movehl_ps(v58, v58));
  v31.m128_f32[0] = *(float *)&n2 + (float)(v59.m128_f32[0] + COERCE_FLOAT(_mm_shuffle_ps(v59, v59, 1)));
  v60 = v15[9];
  v57.m128_f32[0] = *(float *)&n0xF0 - v31.m128_f32[0];
  v31.m128_i32[0] = v15[14].m128_i32[2];
  v61 = (__m128)v31.m128_u32[0];
  v61.m128_f32[0] = v31.m128_f32[0] * 529.0f;
  v62 = _mm_unpacklo_ps(v57, v57);
  v63 = _mm_movelh_ps(v62, v62);
  v64 = _mm_unpacklo_ps(v61, v61);
  v65 = _mm_movelh_ps(v64, v64);
  v66 = _mm_add_ps(v15[8], v65);
  *v15 = _mm_add_ps(
           _mm_div_ps(
             _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_441190, v63), *(__m128 *)(a1 + 278528)),
             _mm_add_ps(v15[7], v65)),
           *v15);
  v67 = v15[10];
  v15[1] = _mm_add_ps(
             _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411A0, v63), *(__m128 *)(a1 + 278544)), v66),
             v15[1]);
  v68 = _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411B0, v63), *(__m128 *)(a1 + 278560)), _mm_add_ps(v60, v65));
  v69 = v15[11];
  v15[2] = _mm_add_ps(v68, v15[2]);
  v70 = _mm_div_ps(_mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411C0, v63), *(__m128 *)(a1 + 278576)), _mm_add_ps(v67, v65));
  v71 = v15[12];
  v15[3] = _mm_add_ps(v70, v15[3]);
  v15[4] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411D0, v63), *(__m128 *)(a1 + 278592)),
               _mm_add_ps(v69, v65)),
             v15[4]);
  v15[5] = _mm_add_ps(
             _mm_div_ps(
               _mm_mul_ps(_mm_mul_ps((__m128)__xmmword_4411E0, v63), *(__m128 *)(a1 + 278608)),
               _mm_add_ps(v71, v65)),
             v15[5]);
  v72 = _mm_mul_ps((__m128)__xmmword_4411F0, v63);
  v73 = v15[13];
  v74 = _mm_mul_ps(v72, *(__m128 *)(a1 + 278624));
  ++v15[15].m128_i32[0];
  v15[6] = _mm_add_ps(_mm_div_ps(v74, _mm_add_ps(v73, v65)), v15[6]);
  v15[14].m128_f32[2] = v31.m128_f32[0] + (float)((float)(10.0f - v31.m128_f32[0]) * 0.00019999999f);
  **(uint32_t **)(a1 + 278668) = *(uint32_t *)(a1 + 278656);
  *(uint32_t *)(a1 + 278668) += 4;
  *(uint32_t *)(a1 + 278672) += 4;
  do
  {
    v75 = v578[n5 + 69669];
    v76 = &v578[v75];
    v77 = v577 - v578[2 * n5 + 69726];
    v581 = v76;
    v78 = n5 << 17;
    v79 = v77 + HIWORD(v76[0x8000 * n5 + 71178]);
    *(uint16_t *)((char *)v76 + v78 + 284714) = v79;
    v580 = *((uint8_t *)v76 + v78 + 284713);
    if ( v580 )
    {
      v576 = v75;
      v80 = v79 + 4 * ((v77 > __dword_4458F0) - (v77 < __dword_4458F4));
      *(uint16_t *)((char *)v581 + v78 + 284714) = v80;
      v575 = v80;
      v81 = v576;
      if ( (int32_t)abs32(v77) < 38 )
      {
        if ( (uint8_t)--v580 )
        {
          *((uint8_t *)v581 + v78 + 284713) = v580;
        }
        else
        {
          v82 = (uint8_t *)v581;
          if ( *((uint8_t *)v581 + v78 + 284712) < 8u )
          {
            v83 = *((uint8_t *)v581 + v78 + 284712) + 1;
            *((uint8_t *)v581 + v78 + 284712) = v83;
            v82[v78 + 284713] = *((uint8_t *)&__dword_439B7C + v83 + 3);
            *(uint16_t *)&v82[v78 + 284714] = 2 * v575;
          }
          else
          {
            *((uint8_t *)v581 + v78 + 284713) = v580;
          }
        }
      }
      if ( !__dword_443388 )
      {
        _mm_prefetch((const char *)&n2, 1);
        n2 = (int32_t)&v578[(v81 ^ 0x7FF0) + 71178] + v78;
        v84 = v578[2 * n5 + 69727] + v77;
        n3 = v81 & 3;
        if ( (uint32_t)n3 >= 3
          || (v545 = v78,
              v576 = v81,
              v85 = *(int16_t *)((char *)v581 + v78 + 284718),
              v86 = v84 - ((v85 + (1 << ((*((uint8_t *)v581 + v78 + 284716) + 31) & 31))) >> (*((uint8_t *)v581 + v78 + 284716) & 31)),
              v87 = n3 <= 0,
              *(uint16_t *)((char *)v581 + v78 + 284718) = v85
                                                      + ((32
                                                        * ((v86 > __dword_4458F0) - (uint32_t)(v86 < __dword_4458F4))
                                                        + v86
                                                        + 1) >> 1),
              v81 = v576,
              !v87) )
        {
          v545 = v78;
          v576 = v81;
          v88 = *(int16_t *)((char *)v581 + v78 + 284710);
          v89 = v84 - ((v88 + (1 << ((*((uint8_t *)v581 + v78 + 284708) + 31) & 31))) >> (*((uint8_t *)v581 + v78 + 284708) & 31));
          *(uint16_t *)((char *)v581 + v78 + 284710) = v88
                                                  + ((32 * ((v89 > __dword_4458F0) - (uint32_t)(v89 < __dword_4458F4))
                                                    + v89
                                                    + 2) >> 2);
          v81 = v576;
        }
        v90 = (char *)v578 + v78;
        v550 = v84;
        _mm_prefetch(&v90[4 * (v81 ^ 0x4000) + 284712], 1);
        v542 = (int32_t)&v90[4 * (v81 ^ 0x4000) + 284712];
        v544 = &v90[4 * (v81 ^ 0x3FF0) + 284712];
        _mm_prefetch(v544, 1);
        v543 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x4000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x4000) & 0xFFFFFFF3)];
        _mm_prefetch(v543, 1);
        _mm_prefetch(&v90[4 * (v81 ^ 0x2000) + 284712], 1);
        v539 = (int32_t)&v90[4 * (v81 ^ 0x2000) + 284712];
        _mm_prefetch(&v90[4 * (v81 ^ 0x5FF0) + 284712], 1);
        v541 = (int32_t)&v90[4 * (v81 ^ 0x5FF0) + 284712];
        v540 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x2000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x2000) & 0xFFFFFFF3)];
        _mm_prefetch(v540, 1);
        _mm_prefetch(&v90[4 * (v81 ^ 0x1000) + 284712], 1);
        v536 = (int32_t)&v90[4 * (v81 ^ 0x1000) + 284712];
        _mm_prefetch(&v90[4 * (v81 ^ 0x6FF0) + 284712], 1);
        v538 = (int32_t)&v90[4 * (v81 ^ 0x6FF0) + 284712];
        v537 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x1000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x1000) & 0xFFFFFFF3)];
        _mm_prefetch(v537, 1);
        v551 = &v90[4 * (v81 ^ 0x800) + 284712];
        v533 = v551;
        v535 = &v90[4 * (v81 ^ 0x77F0) + 284712];
        v534 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x800) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x800) & 0xFFFFFFF3)];
        v552 = &v90[4 * (v81 ^ 0x400) + 284712];
        v530 = v552;
        _mm_prefetch(v551, 1);
        _mm_prefetch(v535, 1);
        _mm_prefetch(v534, 1);
        v553 = &v90[4 * (v81 ^ 0x7BF0) + 284712];
        v532 = v553;
        v531 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x400) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x400) & 0xFFFFFFF3)];
        v554 = &v90[4 * (v81 ^ 0x200) + 284712];
        v527 = v554;
        _mm_prefetch(v552, 1);
        v555 = &v90[4 * (v81 ^ 0x7DF0) + 284712];
        v529 = v555;
        v91 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x200) & 0xC))
                 + 284712
                 + 4 * ((v81 ^ 0x200) & 0xFFFFFFF3)];
        _mm_prefetch(v553, 1);
        _mm_prefetch(v531, 1);
        v528 = v91;
        v556 = &v90[4 * (v81 ^ 0x100) + 284712];
        v524 = v556;
        v92 = *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x100) & 0xC)) + ((v81 ^ 0x100) & 0xFFFFFFF3);
        v557 = &v90[4 * (v81 ^ 0x7EF0) + 284712];
        v526 = v557;
        _mm_prefetch(v554, 1);
        v558 = (int32_t)&v90[4 * v92 + 284712];
        v525 = v558;
        v559 = &v90[4 * (v81 ^ 0x80) + 284712];
        v521 = v559;
        _mm_prefetch(v555, 1);
        _mm_prefetch(v91, 1);
        v523 = &v90[4 * (v81 ^ 0x7F70) + 284712];
        v560 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x80) & 0xC)) + 284712 + 4 * ((v81 ^ 0x80) & 0xFFFFFFF3)];
        v522 = v560;
        v561 = &v90[4 * (v81 ^ 0x40) + 284712];
        v518 = v561;
        _mm_prefetch(v556, 1);
        v562 = &v90[4 * (v81 ^ 0x7FB0) + 284712];
        v520 = v562;
        v93 = &v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x40) & 0xC)) + 284712 + 4 * ((v81 ^ 0x40) & 0xFFFFFFF3)];
        v94 = (const char *)v558;
        _mm_prefetch(v557, 1);
        _mm_prefetch(v94, 1);
        v563 = v93;
        v519 = v93;
        v95 = v81 ^ 0x20;
        v96 = v81 ^ 0x10;
        v564 = &v90[4 * v95 + 284712];
        v515 = v564;
        v565 = &v90[4 * (v95 ^ 0x7FF0) + 284712];
        v517 = v565;
        _mm_prefetch(v559, 1);
        _mm_prefetch(v523, 1);
        v566 = (uint32_t)&v90[4 * *(int32_t *)((char *)__dword_439880 + (v95 & 0xC)) + 284712 + 4 * (v95 & 0xFFFFFFF3)];
        v516 = v566;
        v567 = (int32_t)&v90[4 * v96 + 284712];
        v512 = v567;
        v97 = *(int32_t *)((char *)__dword_439880 + (v96 & 0xC)) + (v96 & 0xFFFFFFF3);
        v568 = (int32_t)&v90[4 * (v96 ^ 0x7FF0) + 284712];
        v514 = v568;
        _mm_prefetch(v560, 1);
        v98 = (int32_t)&v90[4 * v97 + 284712];
        n2_1 = n2;
        v569 = (const char *)v98;
        v100 = -v550;
        v513 = v98;
        LOBYTE(v97) = *(uint8_t *)n2;
        _mm_prefetch(v561, 1);
        v101 = *(int16_t *)(n2_1 + 2);
        _mm_prefetch(v562, 1);
        v102 = v100 - ((v101 + (1 << ((v97 + 31) & 31))) >> (v97 & 31));
        v105 = __OFSUB__(v102, __dword_4458F0);
        v103 = v102 == __dword_4458F0;
        v104 = v102 - __dword_4458F0 < 0;
        _mm_prefetch(v563, 1);
        v106 = 32 * (!(v104 ^ v105 | v103) - (v102 < __dword_4458F4));
        _mm_prefetch(v564, 1);
        _mm_prefetch(v565, 1);
        v107 = v569;
        v108 = v550;
        v109 = v106 + v102 + 2;
        v110 = (const char *)v567;
        LOWORD(v109) = v101 + (v109 >> 2);
        v87 = n3 < 3;
        v111 = (const char *)v566;
        *(uint16_t *)(n2 + 2) = v109;
        v112 = (const char *)v568;
        _mm_prefetch(v111, 1);
        _mm_prefetch(v110, 1);
        _mm_prefetch(v112, 1);
        _mm_prefetch(v107, 1);
        if ( v87
          && (v113 = *(int16_t *)(n2 + 6),
              v114 = *(uint8_t *)(n2 + 4),
              v550 = v108,
              v87 = n3 <= 0,
              *(uint16_t *)(n2 + 6) = ((uint32_t)(-v108 - ((v113 + (1 << ((v114 + 31) & 31))) >> (v114 & 31)) + 2) >> 2) + v113,
              v87) )
        {
          v311 = *(int16_t *)(v542 + 2);
          v312 = *(uint8_t *)v542;
          v550 = v108;
          v313 = v108 - ((v311 + (1 << ((v312 + 31) & 31))) >> (v312 & 31));
          *(uint16_t *)(v542 + 2) = v311
                               + ((32 * ((v313 > __dword_4458F0) - (uint32_t)(v313 < __dword_4458F4)) + v313 + 2) >> 2);
          v314 = *((int16_t *)v543 + 1);
          v315 = v108 - ((v314 + (1 << ((*v543 + 31) & 31))) >> (*v543 & 31));
          *((uint16_t *)v543 + 1) = v314
                               + ((32 * ((v315 > __dword_4458F0) - (uint32_t)(v315 < __dword_4458F4)) + v315 + 4) >> 3);
          v316 = *(int16_t *)(v542 + 6);
          v317 = (v316 + (1 << ((*(uint8_t *)(v542 + 4) + 31) & 31))) >> (*(uint8_t *)(v542 + 4) & 31);
          v549 = -v108;
          *(uint16_t *)(v542 + 6) = ((uint32_t)(v108 - v317 + 2) >> 2) + v316;
          v318 = *((int16_t *)v544 + 1);
          v319 = -v108 - ((v318 + (1 << ((*v544 + 31) & 31))) >> (*v544 & 31));
          *((uint16_t *)v544 + 1) = v318
                               + ((32 * ((v319 > __dword_4458F0) - (uint32_t)(v319 < __dword_4458F4)) + v319 + 4) >> 3);
          v320 = *(int16_t *)(v539 + 2);
          v321 = v550;
          v322 = v550 - ((v320 + (1 << ((*(uint8_t *)v539 + 31) & 31))) >> (*(uint8_t *)v539 & 31));
          *(uint16_t *)(v539 + 2) = v320
                               + ((32 * ((v322 > __dword_4458F0) - (uint32_t)(v322 < __dword_4458F4)) + v322 + 2) >> 2);
          v323 = *((int16_t *)v540 + 1);
          v324 = v321 - ((v323 + (1 << ((*v540 + 31) & 31))) >> (*v540 & 31));
          *((uint16_t *)v540 + 1) = v323
                               + ((32 * ((v324 > __dword_4458F0) - (uint32_t)(v324 < __dword_4458F4)) + v324 + 4) >> 3);
          *(uint16_t *)(v539 + 6) += (uint32_t)(v321
                                               - ((*(int16_t *)(v539 + 6) + (1 << ((*(uint8_t *)(v539 + 4) + 31) & 31))) >> (*(uint8_t *)(v539 + 4) & 31))
                                               + 2) >> 2;
          v325 = *(int16_t *)(v541 + 2);
          v326 = v549 - ((v325 + (1 << ((*(uint8_t *)v541 + 31) & 31))) >> (*(uint8_t *)v541 & 31));
          *(uint16_t *)(v541 + 2) = v325
                               + ((32 * ((v326 > __dword_4458F0) - (uint32_t)(v326 < __dword_4458F4)) + v326 + 4) >> 3);
          v327 = *(int16_t *)(v536 + 2);
          v328 = v550 - ((v327 + (1 << ((*(uint8_t *)v536 + 31) & 31))) >> (*(uint8_t *)v536 & 31));
          *(uint16_t *)(v536 + 2) = v327
                               + ((32 * ((v328 > __dword_4458F0) - (uint32_t)(v328 < __dword_4458F4)) + v328 + 2) >> 2);
          v329 = *((int16_t *)v537 + 1);
          v330 = v550 - ((v329 + (1 << ((*v537 + 31) & 31))) >> (*v537 & 31));
          *((uint16_t *)v537 + 1) = v329
                               + ((32 * ((v330 > __dword_4458F0) - (uint32_t)(v330 < __dword_4458F4)) + v330 + 4) >> 3);
          *(uint16_t *)(v536 + 6) += (uint32_t)(v550
                                               - ((*(int16_t *)(v536 + 6) + (1 << ((*(uint8_t *)(v536 + 4) + 31) & 31))) >> (*(uint8_t *)(v536 + 4) & 31))
                                               + 2) >> 2;
          v331 = *(int16_t *)(v538 + 2);
          v332 = v549 - ((v331 + (1 << ((*(uint8_t *)v538 + 31) & 31))) >> (*(uint8_t *)v538 & 31));
          *(uint16_t *)(v538 + 2) = v331
                               + ((32 * ((v332 > __dword_4458F0) - (uint32_t)(v332 < __dword_4458F4)) + v332 + 4) >> 3);
          v333 = *((int16_t *)v533 + 1);
          v334 = v550 - ((v333 + (1 << ((*v533 + 31) & 31))) >> (*v533 & 31));
          *((uint16_t *)v533 + 1) = v333
                               + ((32 * ((v334 > __dword_4458F0) - (uint32_t)(v334 < __dword_4458F4)) + v334 + 2) >> 2);
          v335 = *((int16_t *)v534 + 1);
          v336 = v550 - ((v335 + (1 << ((*v534 + 31) & 31))) >> (*v534 & 31));
          *((uint16_t *)v534 + 1) = v335
                               + ((32 * ((v336 > __dword_4458F0) - (uint32_t)(v336 < __dword_4458F4)) + v336 + 4) >> 3);
          *((uint16_t *)v533 + 3) += (uint32_t)(v550 - ((*((int16_t *)v533 + 3) + (1 << ((v533[4] + 31) & 31))) >> (v533[4] & 31)) + 2) >> 2;
          v337 = *((int16_t *)v535 + 1);
          v338 = v549 - ((v337 + (1 << ((*v535 + 31) & 31))) >> (*v535 & 31));
          *((uint16_t *)v535 + 1) = v337
                               + ((32 * ((v338 > __dword_4458F0) - (uint32_t)(v338 < __dword_4458F4)) + v338 + 4) >> 3);
          v339 = *((int16_t *)v530 + 1);
          v340 = v550 - ((v339 + (1 << ((*v530 + 31) & 31))) >> (*v530 & 31));
          *((uint16_t *)v530 + 1) = v339
                               + ((32 * ((v340 > __dword_4458F0) - (uint32_t)(v340 < __dword_4458F4)) + v340 + 2) >> 2);
          v341 = *((int16_t *)v531 + 1);
          v342 = v550 - ((v341 + (1 << ((*v531 + 31) & 31))) >> (*v531 & 31));
          *((uint16_t *)v531 + 1) = v341
                               + ((32 * ((v342 > __dword_4458F0) - (uint32_t)(v342 < __dword_4458F4)) + v342 + 4) >> 3);
          v343 = v550;
          *((uint16_t *)v530 + 3) += (uint32_t)(v550 - ((*((int16_t *)v530 + 3) + (1 << ((v530[4] + 31) & 31))) >> (v530[4] & 31)) + 2) >> 2;
          v344 = *((int16_t *)v532 + 1);
          v549 -= (v344 + (1 << ((*v532 + 31) & 31))) >> (*v532 & 31);
          v550 = v343;
          *((uint16_t *)v532 + 1) = v344
                               + ((32 * ((v549 > __dword_4458F0) - (uint32_t)(v549 < __dword_4458F4)) + v549 + 4) >> 3);
          v345 = *((int16_t *)v527 + 1);
          v346 = v550 - ((v345 + (1 << ((*v527 + 31) & 31))) >> (*v527 & 31));
          *((uint16_t *)v527 + 1) = v345
                               + ((32 * ((v346 > __dword_4458F0) - (uint32_t)(v346 < __dword_4458F4)) + v346 + 2) >> 2);
          v347 = *((int16_t *)v528 + 1);
          v348 = v550 - ((v347 + (1 << ((*v528 + 31) & 31))) >> (*v528 & 31));
          *((uint16_t *)v528 + 1) = v347
                               + ((32 * ((v348 > __dword_4458F0) - (uint32_t)(v348 < __dword_4458F4)) + v348 + 4) >> 3);
          v349 = v550;
          *((uint16_t *)v527 + 3) += (uint32_t)(v550 - ((*((int16_t *)v527 + 3) + (1 << ((v527[4] + 31) & 31))) >> (v527[4] & 31)) + 2) >> 2;
          v350 = *((int16_t *)v529 + 1);
          LOBYTE(v345) = *v529;
          v548 = -v349;
          v351 = -v349 - ((v350 + (1 << ((v345 + 31) & 31))) >> (v345 & 31));
          *((uint16_t *)v529 + 1) = v350
                               + ((32 * ((v351 > __dword_4458F0) - (uint32_t)(v351 < __dword_4458F4)) + v351 + 4) >> 3);
          v352 = *((int16_t *)v524 + 1);
          v353 = v550;
          v354 = v550 - ((v352 + (1 << ((*v524 + 31) & 31))) >> (*v524 & 31));
          *((uint16_t *)v524 + 1) = v352
                               + ((32 * ((v354 > __dword_4458F0) - (uint32_t)(v354 < __dword_4458F4)) + v354 + 2) >> 2);
          v355 = *(int16_t *)(v525 + 2);
          v356 = v353 - ((v355 + (1 << ((*(uint8_t *)v525 + 31) & 31))) >> (*(uint8_t *)v525 & 31));
          *(uint16_t *)(v525 + 2) = v355
                               + ((32 * ((v356 > __dword_4458F0) - (uint32_t)(v356 < __dword_4458F4)) + v356 + 4) >> 3);
          *((uint16_t *)v524 + 3) += (uint32_t)(v353 - ((*((int16_t *)v524 + 3) + (1 << ((v524[4] + 31) & 31))) >> (v524[4] & 31)) + 2) >> 2;
          v357 = *((int16_t *)v526 + 1);
          v358 = v548 - ((v357 + (1 << ((*v526 + 31) & 31))) >> (*v526 & 31));
          *((uint16_t *)v526 + 1) = v357
                               + ((32 * ((v358 > __dword_4458F0) - (uint32_t)(v358 < __dword_4458F4)) + v358 + 4) >> 3);
          v359 = *((int16_t *)v521 + 1);
          v360 = v550 - ((v359 + (1 << ((*v521 + 31) & 31))) >> (*v521 & 31));
          *((uint16_t *)v521 + 1) = v359
                               + ((32 * ((v360 > __dword_4458F0) - (uint32_t)(v360 < __dword_4458F4)) + v360 + 2) >> 2);
          v361 = *((int16_t *)v522 + 1);
          v362 = v550 - ((v361 + (1 << ((*v522 + 31) & 31))) >> (*v522 & 31));
          *((uint16_t *)v522 + 1) = v361
                               + ((32 * ((v362 > __dword_4458F0) - (uint32_t)(v362 < __dword_4458F4)) + v362 + 4) >> 3);
          *((uint16_t *)v521 + 3) += (uint32_t)(v550 - ((*((int16_t *)v521 + 3) + (1 << ((v521[4] + 31) & 31))) >> (v521[4] & 31)) + 2) >> 2;
          v363 = *((int16_t *)v523 + 1);
          v364 = v548 - ((v363 + (1 << ((*v523 + 31) & 31))) >> (*v523 & 31));
          *((uint16_t *)v523 + 1) = v363
                               + ((32 * ((v364 > __dword_4458F0) - (uint32_t)(v364 < __dword_4458F4)) + v364 + 4) >> 3);
          v365 = *((int16_t *)v518 + 1);
          v366 = v550 - ((v365 + (1 << ((*v518 + 31) & 31))) >> (*v518 & 31));
          *((uint16_t *)v518 + 1) = v365
                               + ((32 * ((v366 > __dword_4458F0) - (uint32_t)(v366 < __dword_4458F4)) + v366 + 2) >> 2);
          v367 = *((int16_t *)v519 + 1);
          v368 = v550 - ((v367 + (1 << ((*v519 + 31) & 31))) >> (*v519 & 31));
          *((uint16_t *)v519 + 1) = v367
                               + ((32 * ((v368 > __dword_4458F0) - (uint32_t)(v368 < __dword_4458F4)) + v368 + 4) >> 3);
          *((uint16_t *)v518 + 3) += (uint32_t)(v550 - ((*((int16_t *)v518 + 3) + (1 << ((v518[4] + 31) & 31))) >> (v518[4] & 31)) + 2) >> 2;
          v369 = *((int16_t *)v520 + 1);
          v370 = v548 - ((v369 + (1 << ((*v520 + 31) & 31))) >> (*v520 & 31));
          *((uint16_t *)v520 + 1) = v369
                               + ((32 * ((v370 > __dword_4458F0) - (uint32_t)(v370 < __dword_4458F4)) + v370 + 4) >> 3);
          v371 = *((int16_t *)v515 + 1);
          v372 = v550 - ((v371 + (1 << ((*v515 + 31) & 31))) >> (*v515 & 31));
          *((uint16_t *)v515 + 1) = v371
                               + ((32 * ((v372 > __dword_4458F0) - (uint32_t)(v372 < __dword_4458F4)) + v372 + 2) >> 2);
          v373 = *(int16_t *)(v516 + 2);
          v374 = v550 - ((v373 + (1 << ((*(uint8_t *)v516 + 31) & 31))) >> (*(uint8_t *)v516 & 31));
          *(uint16_t *)(v516 + 2) = v373
                               + ((32 * ((v374 > __dword_4458F0) - (uint32_t)(v374 < __dword_4458F4)) + v374 + 4) >> 3);
          v375 = v550;
          *((uint16_t *)v515 + 3) += (uint32_t)(v550 - ((*((int16_t *)v515 + 3) + (1 << ((v515[4] + 31) & 31))) >> (v515[4] & 31)) + 2) >> 2;
          v376 = *((int16_t *)v517 + 1);
          v377 = (v376 + (1 << ((*v517 + 31) & 31))) >> (*v517 & 31);
          v550 = v375;
          *((uint16_t *)v517 + 1) = v376
                               + ((32 * ((v548 - v377 > __dword_4458F0) - (uint32_t)(v548 - v377 < __dword_4458F4))
                                 + v548
                                 - v377
                                 + 4) >> 3);
          v378 = *(int16_t *)(v512 + 2);
          v379 = v550 - ((v378 + (1 << ((*(uint8_t *)v512 + 31) & 31))) >> (*(uint8_t *)v512 & 31));
          *(uint16_t *)(v512 + 2) = v378
                               + ((32 * ((v379 > __dword_4458F0) - (uint32_t)(v379 < __dword_4458F4)) + v379 + 2) >> 2);
          v380 = *(int16_t *)(v513 + 2);
          v381 = v550 - ((v380 + (1 << ((*(uint8_t *)v513 + 31) & 31))) >> (*(uint8_t *)v513 & 31));
          *(uint16_t *)(v513 + 2) = v380
                               + ((32 * ((v381 > __dword_4458F0) - (uint32_t)(v381 < __dword_4458F4)) + v381 + 4) >> 3);
          v382 = v550;
          *(uint16_t *)(v512 + 6) += (uint32_t)(v550
                                               - ((*(int16_t *)(v512 + 6) + (1 << ((*(uint8_t *)(v512 + 4) + 31) & 31))) >> (*(uint8_t *)(v512 + 4) & 31))
                                               + 2) >> 2;
          v383 = *(int16_t *)(v514 + 2);
          v384 = -v382 - ((v383 + (1 << ((*(uint8_t *)v514 + 31) & 31))) >> (*(uint8_t *)v514 & 31));
          *(uint16_t *)(v514 + 2) = v383
                               + ((32 * ((v384 > __dword_4458F0) - (uint32_t)(v384 < __dword_4458F4)) + v384 + 4) >> 3);
        }
        else
        {
          v115 = *(int16_t *)(n2 - 2);
          v116 = *(uint8_t *)(n2 - 4);
          v550 = v108;
          *(uint16_t *)(n2 - 2) = ((uint32_t)(-v108 - ((v115 + (1 << ((v116 + 31) & 31))) >> (v116 & 31)) + 4) >> 3) + v115;
          v117 = *(int16_t *)(v542 + 2);
          v118 = v550 - ((v117 + (1 << ((*(uint8_t *)v542 + 31) & 31))) >> (*(uint8_t *)v542 & 31));
          *(uint16_t *)(v542 + 2) = v117
                               + ((32 * ((v118 > __dword_4458F0) - (uint32_t)(v118 < __dword_4458F4)) + v118 + 2) >> 2);
          v119 = *((int16_t *)v543 + 1);
          v87 = n3 < 3;
          v120 = v550 - ((v119 + (1 << ((*v543 + 31) & 31))) >> (*v543 & 31));
          *((uint16_t *)v543 + 1) = v119
                               + ((32 * ((v120 > __dword_4458F0) - (uint32_t)(v120 < __dword_4458F4)) + v120 + 4) >> 3);
          v121 = v550;
          if ( v87 )
          {
            *(uint16_t *)(v542 + 6) += (uint32_t)(v550
                                                 - ((*(int16_t *)(v542 + 6) + (1 << ((*(uint8_t *)(v542 + 4) + 31) & 31))) >> (*(uint8_t *)(v542 + 4) & 31))
                                                 + 2) >> 2;
            v216 = *(int16_t *)(v542 - 2);
            v217 = v121 - ((v216 + (1 << ((*(uint8_t *)(v542 - 4) + 31) & 31))) >> (*(uint8_t *)(v542 - 4) & 31));
            v218 = -v121;
            v572 = v218;
            *(uint16_t *)(v542 - 2) = v216
                                 + ((32 * ((v217 > __dword_4458F0) - (uint32_t)(v217 < __dword_4458F4)) + v217 + 4) >> 3);
            v219 = *((int16_t *)v544 + 1);
            v220 = v218 - ((v219 + (1 << ((*v544 + 31) & 31))) >> (*v544 & 31));
            *((uint16_t *)v544 + 1) = v219
                                 + ((32 * ((v220 > __dword_4458F0) - (uint32_t)(v220 < __dword_4458F4)) + v220 + 4) >> 3);
            v221 = *(int16_t *)(v539 + 2);
            v222 = v550;
            v223 = v550 - ((v221 + (1 << ((*(uint8_t *)v539 + 31) & 31))) >> (*(uint8_t *)v539 & 31));
            *(uint16_t *)(v539 + 2) = v221
                                 + ((32 * ((v223 > __dword_4458F0) - (uint32_t)(v223 < __dword_4458F4)) + v223 + 2) >> 2);
            v224 = *((int16_t *)v540 + 1);
            v225 = v222 - ((v224 + (1 << ((*v540 + 31) & 31))) >> (*v540 & 31));
            *((uint16_t *)v540 + 1) = v224
                                 + ((32 * ((v225 > __dword_4458F0) - (uint32_t)(v225 < __dword_4458F4)) + v225 + 4) >> 3);
            *(uint16_t *)(v539 + 6) += (uint32_t)(v222
                                                 - ((*(int16_t *)(v539 + 6) + (1 << ((*(uint8_t *)(v539 + 4) + 31) & 31))) >> (*(uint8_t *)(v539 + 4) & 31))
                                                 + 2) >> 2;
            v226 = *(int16_t *)(v539 - 2);
            v227 = v222 - ((v226 + (1 << ((*(uint8_t *)(v539 - 4) + 31) & 31))) >> (*(uint8_t *)(v539 - 4) & 31));
            *(uint16_t *)(v539 - 2) = v226
                                 + ((32 * ((v227 > __dword_4458F0) - (uint32_t)(v227 < __dword_4458F4)) + v227 + 4) >> 3);
            v228 = *(int16_t *)(v541 + 2);
            v229 = v572 - ((v228 + (1 << ((*(uint8_t *)v541 + 31) & 31))) >> (*(uint8_t *)v541 & 31));
            *(uint16_t *)(v541 + 2) = v228
                                 + ((32 * ((v229 > __dword_4458F0) - (uint32_t)(v229 < __dword_4458F4)) + v229 + 4) >> 3);
            v230 = *(int16_t *)(v536 + 2);
            v231 = v550;
            v232 = v550 - ((v230 + (1 << ((*(uint8_t *)v536 + 31) & 31))) >> (*(uint8_t *)v536 & 31));
            *(uint16_t *)(v536 + 2) = v230
                                 + ((32 * ((v232 > __dword_4458F0) - (uint32_t)(v232 < __dword_4458F4)) + v232 + 2) >> 2);
            v233 = *((int16_t *)v537 + 1);
            v234 = v231 - ((v233 + (1 << ((*v537 + 31) & 31))) >> (*v537 & 31));
            *((uint16_t *)v537 + 1) = v233
                                 + ((32 * ((v234 > __dword_4458F0) - (uint32_t)(v234 < __dword_4458F4)) + v234 + 4) >> 3);
            *(uint16_t *)(v536 + 6) += (uint32_t)(v231
                                                 - ((*(int16_t *)(v536 + 6) + (1 << ((*(uint8_t *)(v536 + 4) + 31) & 31))) >> (*(uint8_t *)(v536 + 4) & 31))
                                                 + 2) >> 2;
            v235 = *(int16_t *)(v536 - 2);
            v236 = v231 - ((v235 + (1 << ((*(uint8_t *)(v536 - 4) + 31) & 31))) >> (*(uint8_t *)(v536 - 4) & 31));
            *(uint16_t *)(v536 - 2) = v235
                                 + ((32 * ((v236 > __dword_4458F0) - (uint32_t)(v236 < __dword_4458F4)) + v236 + 4) >> 3);
            v237 = *(int16_t *)(v538 + 2);
            v238 = v572 - ((v237 + (1 << ((*(uint8_t *)v538 + 31) & 31))) >> (*(uint8_t *)v538 & 31));
            *(uint16_t *)(v538 + 2) = v237
                                 + ((32 * ((v238 > __dword_4458F0) - (uint32_t)(v238 < __dword_4458F4)) + v238 + 4) >> 3);
            v239 = *((int16_t *)v533 + 1);
            v240 = v550;
            v241 = v550 - ((v239 + (1 << ((*v533 + 31) & 31))) >> (*v533 & 31));
            *((uint16_t *)v533 + 1) = v239
                                 + ((32 * ((v241 > __dword_4458F0) - (uint32_t)(v241 < __dword_4458F4)) + v241 + 2) >> 2);
            v242 = *((int16_t *)v534 + 1);
            v243 = v240 - ((v242 + (1 << ((*v534 + 31) & 31))) >> (*v534 & 31));
            *((uint16_t *)v534 + 1) = v242
                                 + ((32 * ((v243 > __dword_4458F0) - (uint32_t)(v243 < __dword_4458F4)) + v243 + 4) >> 3);
            *((uint16_t *)v533 + 3) += (uint32_t)(v240
                                                 - ((*((int16_t *)v533 + 3) + (1 << ((v533[4] + 31) & 31))) >> (v533[4] & 31))
                                                 + 2) >> 2;
            v244 = *((int16_t *)v533 - 1);
            v245 = v240 - ((v244 + (1 << ((*(v533 - 4) + 31) & 31))) >> (*(v533 - 4) & 31));
            *((uint16_t *)v533 - 1) = v244
                                 + ((32 * ((v245 > __dword_4458F0) - (uint32_t)(v245 < __dword_4458F4)) + v245 + 4) >> 3);
            v246 = *((int16_t *)v535 + 1);
            v247 = v572 - ((v246 + (1 << ((*v535 + 31) & 31))) >> (*v535 & 31));
            *((uint16_t *)v535 + 1) = v246
                                 + ((32 * ((v247 > __dword_4458F0) - (uint32_t)(v247 < __dword_4458F4)) + v247 + 4) >> 3);
            v248 = *((int16_t *)v530 + 1);
            v249 = v550;
            v250 = v550 - ((v248 + (1 << ((*v530 + 31) & 31))) >> (*v530 & 31));
            *((uint16_t *)v530 + 1) = v248
                                 + ((32 * ((v250 > __dword_4458F0) - (uint32_t)(v250 < __dword_4458F4)) + v250 + 2) >> 2);
            v251 = *((int16_t *)v531 + 1);
            LOBYTE(v248) = *v531;
            v252 = v251 + (1 << ((*v531 + 31) & 31));
            v550 = v249;
            *((uint16_t *)v531 + 1) = v251
                                 + ((32
                                   * ((v249 - (v252 >> (v248 & 31)) > __dword_4458F0)
                                    - (uint32_t)(v249 - (v252 >> (v248 & 31)) < __dword_4458F4))
                                   + v249
                                   - (v252 >> (v248 & 31))
                                   + 4) >> 3);
            v253 = v550;
            *((uint16_t *)v530 + 3) += (uint32_t)(v550
                                                 - ((*((int16_t *)v530 + 3) + (1 << ((v530[4] + 31) & 31))) >> (v530[4] & 31))
                                                 + 2) >> 2;
            v254 = *((int16_t *)v530 - 1);
            v255 = v253 - ((v254 + (1 << ((*(v530 - 4) + 31) & 31))) >> (*(v530 - 4) & 31));
            *((uint16_t *)v530 - 1) = v254
                                 + ((32 * ((v255 > __dword_4458F0) - (uint32_t)(v255 < __dword_4458F4)) + v255 + 4) >> 3);
            v256 = *((int16_t *)v532 + 1);
            LOBYTE(v254) = *v532;
            v573 = -v253;
            v257 = -v253 - ((v256 + (1 << ((v254 + 31) & 31))) >> (v254 & 31));
            *((uint16_t *)v532 + 1) = v256
                                 + ((32 * ((v257 > __dword_4458F0) - (uint32_t)(v257 < __dword_4458F4)) + v257 + 4) >> 3);
            v258 = *((int16_t *)v527 + 1);
            v259 = v550;
            v260 = v550 - ((v258 + (1 << ((*v527 + 31) & 31))) >> (*v527 & 31));
            *((uint16_t *)v527 + 1) = v258
                                 + ((32 * ((v260 > __dword_4458F0) - (uint32_t)(v260 < __dword_4458F4)) + v260 + 2) >> 2);
            v261 = *((int16_t *)v528 + 1);
            v262 = v259 - ((v261 + (1 << ((*v528 + 31) & 31))) >> (*v528 & 31));
            *((uint16_t *)v528 + 1) = v261
                                 + ((32 * ((v262 > __dword_4458F0) - (uint32_t)(v262 < __dword_4458F4)) + v262 + 4) >> 3);
            *((uint16_t *)v527 + 3) += (uint32_t)(v259
                                                 - ((*((int16_t *)v527 + 3) + (1 << ((v527[4] + 31) & 31))) >> (v527[4] & 31))
                                                 + 2) >> 2;
            v263 = *((int16_t *)v527 - 1);
            v264 = v259 - ((v263 + (1 << ((*(v527 - 4) + 31) & 31))) >> (*(v527 - 4) & 31));
            *((uint16_t *)v527 - 1) = v263
                                 + ((32 * ((v264 > __dword_4458F0) - (uint32_t)(v264 < __dword_4458F4)) + v264 + 4) >> 3);
            v265 = *((int16_t *)v529 + 1);
            v266 = v573 - ((v265 + (1 << ((*v529 + 31) & 31))) >> (*v529 & 31));
            *((uint16_t *)v529 + 1) = v265
                                 + ((32 * ((v266 > __dword_4458F0) - (uint32_t)(v266 < __dword_4458F4)) + v266 + 4) >> 3);
            v267 = *((int16_t *)v524 + 1);
            v268 = v550;
            v269 = v550 - ((v267 + (1 << ((*v524 + 31) & 31))) >> (*v524 & 31));
            *((uint16_t *)v524 + 1) = v267
                                 + ((32 * ((v269 > __dword_4458F0) - (uint32_t)(v269 < __dword_4458F4)) + v269 + 2) >> 2);
            v270 = *(int16_t *)(v525 + 2);
            v271 = v268 - ((v270 + (1 << ((*(uint8_t *)v525 + 31) & 31))) >> (*(uint8_t *)v525 & 31));
            *(uint16_t *)(v525 + 2) = v270
                                 + ((32 * ((v271 > __dword_4458F0) - (uint32_t)(v271 < __dword_4458F4)) + v271 + 4) >> 3);
            *((uint16_t *)v524 + 3) += (uint32_t)(v268
                                                 - ((*((int16_t *)v524 + 3) + (1 << ((v524[4] + 31) & 31))) >> (v524[4] & 31))
                                                 + 2) >> 2;
            v272 = *((int16_t *)v524 - 1);
            v273 = v268 - ((v272 + (1 << ((*(v524 - 4) + 31) & 31))) >> (*(v524 - 4) & 31));
            *((uint16_t *)v524 - 1) = v272
                                 + ((32 * ((v273 > __dword_4458F0) - (uint32_t)(v273 < __dword_4458F4)) + v273 + 4) >> 3);
            v274 = *((int16_t *)v526 + 1);
            v275 = v573 - ((v274 + (1 << ((*v526 + 31) & 31))) >> (*v526 & 31));
            *((uint16_t *)v526 + 1) = v274
                                 + ((32 * ((v275 > __dword_4458F0) - (uint32_t)(v275 < __dword_4458F4)) + v275 + 4) >> 3);
            v276 = *((int16_t *)v521 + 1);
            v277 = v550;
            v278 = v550 - ((v276 + (1 << ((*v521 + 31) & 31))) >> (*v521 & 31));
            *((uint16_t *)v521 + 1) = v276
                                 + ((32 * ((v278 > __dword_4458F0) - (uint32_t)(v278 < __dword_4458F4)) + v278 + 2) >> 2);
            v279 = *((int16_t *)v522 + 1);
            v280 = v277 - ((v279 + (1 << ((*v522 + 31) & 31))) >> (*v522 & 31));
            *((uint16_t *)v522 + 1) = v279
                                 + ((32 * ((v280 > __dword_4458F0) - (uint32_t)(v280 < __dword_4458F4)) + v280 + 4) >> 3);
            *((uint16_t *)v521 + 3) += (uint32_t)(v277
                                                 - ((*((int16_t *)v521 + 3) + (1 << ((v521[4] + 31) & 31))) >> (v521[4] & 31))
                                                 + 2) >> 2;
            v281 = *((int16_t *)v521 - 1);
            v282 = v277 - ((v281 + (1 << ((*(v521 - 4) + 31) & 31))) >> (*(v521 - 4) & 31));
            *((uint16_t *)v521 - 1) = v281
                                 + ((32 * ((v282 > __dword_4458F0) - (uint32_t)(v282 < __dword_4458F4)) + v282 + 4) >> 3);
            v283 = *((int16_t *)v523 + 1);
            v284 = v573 - ((v283 + (1 << ((*v523 + 31) & 31))) >> (*v523 & 31));
            *((uint16_t *)v523 + 1) = v283
                                 + ((32 * ((v284 > __dword_4458F0) - (uint32_t)(v284 < __dword_4458F4)) + v284 + 4) >> 3);
            v285 = *((int16_t *)v518 + 1);
            v286 = v550;
            v287 = v550 - ((v285 + (1 << ((*v518 + 31) & 31))) >> (*v518 & 31));
            *((uint16_t *)v518 + 1) = v285
                                 + ((32 * ((v287 > __dword_4458F0) - (uint32_t)(v287 < __dword_4458F4)) + v287 + 2) >> 2);
            v288 = *((int16_t *)v519 + 1);
            v289 = v286 - ((v288 + (1 << ((*v519 + 31) & 31))) >> (*v519 & 31));
            *((uint16_t *)v519 + 1) = v288
                                 + ((32 * ((v289 > __dword_4458F0) - (uint32_t)(v289 < __dword_4458F4)) + v289 + 4) >> 3);
            *((uint16_t *)v518 + 3) += (uint32_t)(v286
                                                 - ((*((int16_t *)v518 + 3) + (1 << ((v518[4] + 31) & 31))) >> (v518[4] & 31))
                                                 + 2) >> 2;
            v290 = *((int16_t *)v518 - 1);
            v291 = v286 - ((v290 + (1 << ((*(v518 - 4) + 31) & 31))) >> (*(v518 - 4) & 31));
            v292 = -v286;
            v574 = v292;
            *((uint16_t *)v518 - 1) = v290
                                 + ((32 * ((v291 > __dword_4458F0) - (uint32_t)(v291 < __dword_4458F4)) + v291 + 4) >> 3);
            v293 = *((int16_t *)v520 + 1);
            v294 = v292 - ((v293 + (1 << ((*v520 + 31) & 31))) >> (*v520 & 31));
            *((uint16_t *)v520 + 1) = v293
                                 + ((32 * ((v294 > __dword_4458F0) - (uint32_t)(v294 < __dword_4458F4)) + v294 + 4) >> 3);
            v295 = *((int16_t *)v515 + 1);
            v296 = v550;
            v297 = v550 - ((v295 + (1 << ((*v515 + 31) & 31))) >> (*v515 & 31));
            *((uint16_t *)v515 + 1) = v295
                                 + ((32 * ((v297 > __dword_4458F0) - (uint32_t)(v297 < __dword_4458F4)) + v297 + 2) >> 2);
            v298 = *(int16_t *)(v516 + 2);
            v299 = v296 - ((v298 + (1 << ((*(uint8_t *)v516 + 31) & 31))) >> (*(uint8_t *)v516 & 31));
            *(uint16_t *)(v516 + 2) = v298
                                 + ((32 * ((v299 > __dword_4458F0) - (uint32_t)(v299 < __dword_4458F4)) + v299 + 4) >> 3);
            *((uint16_t *)v515 + 3) += (uint32_t)(v296
                                                 - ((*((int16_t *)v515 + 3) + (1 << ((v515[4] + 31) & 31))) >> (v515[4] & 31))
                                                 + 2) >> 2;
            v300 = *((int16_t *)v515 - 1);
            v301 = v296 - ((v300 + (1 << ((*(v515 - 4) + 31) & 31))) >> (*(v515 - 4) & 31));
            *((uint16_t *)v515 - 1) = v300
                                 + ((32 * ((v301 > __dword_4458F0) - (uint32_t)(v301 < __dword_4458F4)) + v301 + 4) >> 3);
            v302 = *((int16_t *)v517 + 1);
            v303 = v574 - ((v302 + (1 << ((*v517 + 31) & 31))) >> (*v517 & 31));
            *((uint16_t *)v517 + 1) = v302
                                 + ((32 * ((v303 > __dword_4458F0) - (uint32_t)(v303 < __dword_4458F4)) + v303 + 4) >> 3);
            v304 = *(int16_t *)(v512 + 2);
            v305 = v550;
            v306 = v550 - ((v304 + (1 << ((*(uint8_t *)v512 + 31) & 31))) >> (*(uint8_t *)v512 & 31));
            *(uint16_t *)(v512 + 2) = v304
                                 + ((32 * ((v306 > __dword_4458F0) - (uint32_t)(v306 < __dword_4458F4)) + v306 + 2) >> 2);
            v307 = *(int16_t *)(v513 + 2);
            v308 = v305 - ((v307 + (1 << ((*(uint8_t *)v513 + 31) & 31))) >> (*(uint8_t *)v513 & 31));
            *(uint16_t *)(v513 + 2) = v307
                                 + ((32 * ((v308 > __dword_4458F0) - (uint32_t)(v308 < __dword_4458F4)) + v308 + 4) >> 3);
            *(uint16_t *)(v512 + 6) += (uint32_t)(v305
                                                 - ((*(int16_t *)(v512 + 6) + (1 << ((*(uint8_t *)(v512 + 4) + 31) & 31))) >> (*(uint8_t *)(v512 + 4) & 31))
                                                 + 2) >> 2;
            v309 = *(int16_t *)(v512 - 2);
            v310 = v305 - ((v309 + (1 << ((*(uint8_t *)(v512 - 4) + 31) & 31))) >> (*(uint8_t *)(v512 - 4) & 31));
            *(uint16_t *)(v512 - 2) = v309
                                 + ((32 * ((v310 > __dword_4458F0) - (uint32_t)(v310 < __dword_4458F4)) + v310 + 4) >> 3);
            v214 = *(int16_t *)(v514 + 2);
            v215 = v574 - ((v214 + (1 << ((*(uint8_t *)v514 + 31) & 31))) >> (*(uint8_t *)v514 & 31));
          }
          else
          {
            v122 = *(int16_t *)(v542 - 2);
            v123 = v550 - ((v122 + (1 << ((*(uint8_t *)(v542 - 4) + 31) & 31))) >> (*(uint8_t *)(v542 - 4) & 31));
            *(uint16_t *)(v542 - 2) = v122
                                 + ((32 * ((v123 > __dword_4458F0) - (uint32_t)(v123 < __dword_4458F4)) + v123 + 4) >> 3);
            v124 = *((int16_t *)v544 + 1);
            LOBYTE(v122) = *v544;
            v547 = -v121;
            v125 = -v121 - ((v124 + (1 << ((v122 + 31) & 31))) >> (v122 & 31));
            *((uint16_t *)v544 + 1) = v124
                                 + ((32 * ((v125 > __dword_4458F0) - (uint32_t)(v125 < __dword_4458F4)) + v125 + 4) >> 3);
            v126 = *(int16_t *)(v539 + 2);
            v127 = v550;
            v128 = v550 - ((v126 + (1 << ((*(uint8_t *)v539 + 31) & 31))) >> (*(uint8_t *)v539 & 31));
            *(uint16_t *)(v539 + 2) = v126
                                 + ((32 * ((v128 > __dword_4458F0) - (uint32_t)(v128 < __dword_4458F4)) + v128 + 2) >> 2);
            v129 = *((int16_t *)v540 + 1);
            v130 = v127 - ((v129 + (1 << ((*v540 + 31) & 31))) >> (*v540 & 31));
            *((uint16_t *)v540 + 1) = v129
                                 + ((32 * ((v130 > __dword_4458F0) - (uint32_t)(v130 < __dword_4458F4)) + v130 + 4) >> 3);
            v131 = *(int16_t *)(v539 - 2);
            v132 = v127 - ((v131 + (1 << ((*(uint8_t *)(v539 - 4) + 31) & 31))) >> (*(uint8_t *)(v539 - 4) & 31));
            *(uint16_t *)(v539 - 2) = v131
                                 + ((32 * ((v132 > __dword_4458F0) - (uint32_t)(v132 < __dword_4458F4)) + v132 + 4) >> 3);
            v133 = *(int16_t *)(v541 + 2);
            v134 = v547 - ((v133 + (1 << ((*(uint8_t *)v541 + 31) & 31))) >> (*(uint8_t *)v541 & 31));
            *(uint16_t *)(v541 + 2) = v133
                                 + ((32 * ((v134 > __dword_4458F0) - (uint32_t)(v134 < __dword_4458F4)) + v134 + 4) >> 3);
            v135 = *(int16_t *)(v536 + 2);
            v136 = v550;
            v137 = v550 - ((v135 + (1 << ((*(uint8_t *)v536 + 31) & 31))) >> (*(uint8_t *)v536 & 31));
            *(uint16_t *)(v536 + 2) = v135
                                 + ((32 * ((v137 > __dword_4458F0) - (uint32_t)(v137 < __dword_4458F4)) + v137 + 2) >> 2);
            v138 = *((int16_t *)v537 + 1);
            v139 = v136 - ((v138 + (1 << ((*v537 + 31) & 31))) >> (*v537 & 31));
            *((uint16_t *)v537 + 1) = v138
                                 + ((32 * ((v139 > __dword_4458F0) - (uint32_t)(v139 < __dword_4458F4)) + v139 + 4) >> 3);
            v140 = *(int16_t *)(v536 - 2);
            v141 = v136 - ((v140 + (1 << ((*(uint8_t *)(v536 - 4) + 31) & 31))) >> (*(uint8_t *)(v536 - 4) & 31));
            *(uint16_t *)(v536 - 2) = v140
                                 + ((32 * ((v141 > __dword_4458F0) - (uint32_t)(v141 < __dword_4458F4)) + v141 + 4) >> 3);
            v142 = *(int16_t *)(v538 + 2);
            v143 = v547 - ((v142 + (1 << ((*(uint8_t *)v538 + 31) & 31))) >> (*(uint8_t *)v538 & 31));
            *(uint16_t *)(v538 + 2) = v142
                                 + ((32 * ((v143 > __dword_4458F0) - (uint32_t)(v143 < __dword_4458F4)) + v143 + 4) >> 3);
            v144 = *((int16_t *)v533 + 1);
            v145 = v550;
            v146 = v550 - ((v144 + (1 << ((*v533 + 31) & 31))) >> (*v533 & 31));
            *((uint16_t *)v533 + 1) = v144
                                 + ((32 * ((v146 > __dword_4458F0) - (uint32_t)(v146 < __dword_4458F4)) + v146 + 2) >> 2);
            v147 = *((int16_t *)v534 + 1);
            v148 = v145 - ((v147 + (1 << ((*v534 + 31) & 31))) >> (*v534 & 31));
            *((uint16_t *)v534 + 1) = v147
                                 + ((32 * ((v148 > __dword_4458F0) - (uint32_t)(v148 < __dword_4458F4)) + v148 + 4) >> 3);
            v149 = *((int16_t *)v533 - 1);
            v150 = v145 - ((v149 + (1 << ((*(v533 - 4) + 31) & 31))) >> (*(v533 - 4) & 31));
            *((uint16_t *)v533 - 1) = v149
                                 + ((32 * ((v150 > __dword_4458F0) - (uint32_t)(v150 < __dword_4458F4)) + v150 + 4) >> 3);
            v151 = *((int16_t *)v535 + 1);
            v152 = v547 - ((v151 + (1 << ((*v535 + 31) & 31))) >> (*v535 & 31));
            *((uint16_t *)v535 + 1) = v151
                                 + ((32 * ((v152 > __dword_4458F0) - (uint32_t)(v152 < __dword_4458F4)) + v152 + 4) >> 3);
            v153 = *((int16_t *)v530 + 1);
            v154 = v550;
            v155 = v550 - ((v153 + (1 << ((*v530 + 31) & 31))) >> (*v530 & 31));
            *((uint16_t *)v530 + 1) = v153
                                 + ((32 * ((v155 > __dword_4458F0) - (uint32_t)(v155 < __dword_4458F4)) + v155 + 2) >> 2);
            v156 = *((int16_t *)v531 + 1);
            v157 = v154 - ((v156 + (1 << ((*v531 + 31) & 31))) >> (*v531 & 31));
            *((uint16_t *)v531 + 1) = v156
                                 + ((32 * ((v157 > __dword_4458F0) - (uint32_t)(v157 < __dword_4458F4)) + v157 + 4) >> 3);
            v158 = *((int16_t *)v530 - 1);
            v159 = v154 - ((v158 + (1 << ((*(v530 - 4) + 31) & 31))) >> (*(v530 - 4) & 31));
            *((uint16_t *)v530 - 1) = v158
                                 + ((32 * ((v159 > __dword_4458F0) - (uint32_t)(v159 < __dword_4458F4)) + v159 + 4) >> 3);
            v160 = *((int16_t *)v532 + 1);
            v547 -= (v160 + (1 << ((*v532 + 31) & 31))) >> (*v532 & 31);
            v161 = v550;
            *((uint16_t *)v532 + 1) = v160
                                 + ((32 * ((v547 > __dword_4458F0) - (uint32_t)(v547 < __dword_4458F4)) + v547 + 4) >> 3);
            v162 = *((int16_t *)v527 + 1);
            v163 = v161 - ((v162 + (1 << ((*v527 + 31) & 31))) >> (*v527 & 31));
            *((uint16_t *)v527 + 1) = v162
                                 + ((32 * ((v163 > __dword_4458F0) - (uint32_t)(v163 < __dword_4458F4)) + v163 + 2) >> 2);
            v164 = *((int16_t *)v528 + 1);
            v165 = v161 - ((v164 + (1 << ((*v528 + 31) & 31))) >> (*v528 & 31));
            *((uint16_t *)v528 + 1) = v164
                                 + ((32 * ((v165 > __dword_4458F0) - (uint32_t)(v165 < __dword_4458F4)) + v165 + 4) >> 3);
            v166 = *((int16_t *)v527 - 1);
            v167 = v161 - ((v166 + (1 << ((*(v527 - 4) + 31) & 31))) >> (*(v527 - 4) & 31));
            v168 = -v161;
            v570 = v168;
            *((uint16_t *)v527 - 1) = v166
                                 + ((32 * ((v167 > __dword_4458F0) - (uint32_t)(v167 < __dword_4458F4)) + v167 + 4) >> 3);
            v169 = *((int16_t *)v529 + 1);
            v170 = v168 - ((v169 + (1 << ((*v529 + 31) & 31))) >> (*v529 & 31));
            *((uint16_t *)v529 + 1) = v169
                                 + ((32 * ((v170 > __dword_4458F0) - (uint32_t)(v170 < __dword_4458F4)) + v170 + 4) >> 3);
            v171 = *((int16_t *)v524 + 1);
            v172 = v550;
            v173 = v550 - ((v171 + (1 << ((*v524 + 31) & 31))) >> (*v524 & 31));
            *((uint16_t *)v524 + 1) = v171
                                 + ((32 * ((v173 > __dword_4458F0) - (uint32_t)(v173 < __dword_4458F4)) + v173 + 2) >> 2);
            v174 = *(int16_t *)(v525 + 2);
            v175 = v172 - ((v174 + (1 << ((*(uint8_t *)v525 + 31) & 31))) >> (*(uint8_t *)v525 & 31));
            *(uint16_t *)(v525 + 2) = v174
                                 + ((32 * ((v175 > __dword_4458F0) - (uint32_t)(v175 < __dword_4458F4)) + v175 + 4) >> 3);
            v176 = *((int16_t *)v524 - 1);
            v177 = v172 - ((v176 + (1 << ((*(v524 - 4) + 31) & 31))) >> (*(v524 - 4) & 31));
            *((uint16_t *)v524 - 1) = v176
                                 + ((32 * ((v177 > __dword_4458F0) - (uint32_t)(v177 < __dword_4458F4)) + v177 + 4) >> 3);
            v178 = *((int16_t *)v526 + 1);
            v179 = v570 - ((v178 + (1 << ((*v526 + 31) & 31))) >> (*v526 & 31));
            *((uint16_t *)v526 + 1) = v178
                                 + ((32 * ((v179 > __dword_4458F0) - (uint32_t)(v179 < __dword_4458F4)) + v179 + 4) >> 3);
            v180 = *((int16_t *)v521 + 1);
            v181 = v550;
            v182 = v550 - ((v180 + (1 << ((*v521 + 31) & 31))) >> (*v521 & 31));
            *((uint16_t *)v521 + 1) = v180
                                 + ((32 * ((v182 > __dword_4458F0) - (uint32_t)(v182 < __dword_4458F4)) + v182 + 2) >> 2);
            v183 = *((int16_t *)v522 + 1);
            v184 = v181 - ((v183 + (1 << ((*v522 + 31) & 31))) >> (*v522 & 31));
            *((uint16_t *)v522 + 1) = v183
                                 + ((32 * ((v184 > __dword_4458F0) - (uint32_t)(v184 < __dword_4458F4)) + v184 + 4) >> 3);
            v185 = *((int16_t *)v521 - 1);
            v186 = v181 - ((v185 + (1 << ((*(v521 - 4) + 31) & 31))) >> (*(v521 - 4) & 31));
            *((uint16_t *)v521 - 1) = v185
                                 + ((32 * ((v186 > __dword_4458F0) - (uint32_t)(v186 < __dword_4458F4)) + v186 + 4) >> 3);
            v187 = *((int16_t *)v523 + 1);
            v188 = v570 - ((v187 + (1 << ((*v523 + 31) & 31))) >> (*v523 & 31));
            *((uint16_t *)v523 + 1) = v187
                                 + ((32 * ((v188 > __dword_4458F0) - (uint32_t)(v188 < __dword_4458F4)) + v188 + 4) >> 3);
            v189 = *((int16_t *)v518 + 1);
            v190 = v550;
            v191 = v550 - ((v189 + (1 << ((*v518 + 31) & 31))) >> (*v518 & 31));
            *((uint16_t *)v518 + 1) = v189
                                 + ((32 * ((v191 > __dword_4458F0) - (uint32_t)(v191 < __dword_4458F4)) + v191 + 2) >> 2);
            v192 = *((int16_t *)v519 + 1);
            v193 = v190 - ((v192 + (1 << ((*v519 + 31) & 31))) >> (*v519 & 31));
            *((uint16_t *)v519 + 1) = v192
                                 + ((32 * ((v193 > __dword_4458F0) - (uint32_t)(v193 < __dword_4458F4)) + v193 + 4) >> 3);
            v194 = *((int16_t *)v518 - 1);
            v195 = v190 - ((v194 + (1 << ((*(v518 - 4) + 31) & 31))) >> (*(v518 - 4) & 31));
            *((uint16_t *)v518 - 1) = v194
                                 + ((32 * ((v195 > __dword_4458F0) - (uint32_t)(v195 < __dword_4458F4)) + v195 + 4) >> 3);
            v196 = *((int16_t *)v520 + 1);
            v197 = v570 - ((v196 + (1 << ((*v520 + 31) & 31))) >> (*v520 & 31));
            *((uint16_t *)v520 + 1) = v196
                                 + ((32 * ((v197 > __dword_4458F0) - (uint32_t)(v197 < __dword_4458F4)) + v197 + 4) >> 3);
            v198 = *((int16_t *)v515 + 1);
            v199 = v550;
            v200 = v550 - ((v198 + (1 << ((*v515 + 31) & 31))) >> (*v515 & 31));
            *((uint16_t *)v515 + 1) = v198
                                 + ((32 * ((v200 > __dword_4458F0) - (uint32_t)(v200 < __dword_4458F4)) + v200 + 2) >> 2);
            v201 = *(int16_t *)(v516 + 2);
            v202 = v199 - ((v201 + (1 << ((*(uint8_t *)v516 + 31) & 31))) >> (*(uint8_t *)v516 & 31));
            v550 = v199;
            *(uint16_t *)(v516 + 2) = v201
                                 + ((32 * ((v202 > __dword_4458F0) - (uint32_t)(v202 < __dword_4458F4)) + v202 + 4) >> 3);
            v203 = *((int16_t *)v515 - 1);
            v204 = v199 - ((v203 + (1 << ((*(v515 - 4) + 31) & 31))) >> (*(v515 - 4) & 31));
            *((uint16_t *)v515 - 1) = v203
                                 + ((32 * ((v204 > __dword_4458F0) - (uint32_t)(v204 < __dword_4458F4)) + v204 + 4) >> 3);
            v205 = *((int16_t *)v517 + 1);
            LOBYTE(v203) = *v517;
            v571 = -v199;
            v206 = -v199 - ((v205 + (1 << ((v203 + 31) & 31))) >> (v203 & 31));
            *((uint16_t *)v517 + 1) = v205
                                 + ((32 * ((v206 > __dword_4458F0) - (uint32_t)(v206 < __dword_4458F4)) + v206 + 4) >> 3);
            v207 = *(int16_t *)(v512 + 2);
            v208 = v550;
            v209 = v550 - ((v207 + (1 << ((*(uint8_t *)v512 + 31) & 31))) >> (*(uint8_t *)v512 & 31));
            *(uint16_t *)(v512 + 2) = v207
                                 + ((32 * ((v209 > __dword_4458F0) - (uint32_t)(v209 < __dword_4458F4)) + v209 + 2) >> 2);
            v210 = *(int16_t *)(v513 + 2);
            v211 = v208 - ((v210 + (1 << ((*(uint8_t *)v513 + 31) & 31))) >> (*(uint8_t *)v513 & 31));
            *(uint16_t *)(v513 + 2) = v210
                                 + ((32 * ((v211 > __dword_4458F0) - (uint32_t)(v211 < __dword_4458F4)) + v211 + 4) >> 3);
            v212 = *(int16_t *)(v512 - 2);
            v213 = v208 - ((v212 + (1 << ((*(uint8_t *)(v512 - 4) + 31) & 31))) >> (*(uint8_t *)(v512 - 4) & 31));
            *(uint16_t *)(v512 - 2) = v212
                                 + ((32 * ((v213 > __dword_4458F0) - (uint32_t)(v213 < __dword_4458F4)) + v213 + 4) >> 3);
            v214 = *(int16_t *)(v514 + 2);
            v215 = v571 - ((v214 + (1 << ((*(uint8_t *)v514 + 31) & 31))) >> (*(uint8_t *)v514 & 31));
          }
          *(uint16_t *)(v514 + 2) = v214
                               + ((32 * ((v215 > __dword_4458F0) - (uint32_t)(v215 < __dword_4458F4)) + v215 + 4) >> 3);
        }
      }
    }
    ++n5;
  }
  while ( n5 < 5 );
  v385 = v578;
  v386 = v578[69676];
  v578[69684] += 18;
  v387 = (uint16_t *)&v385[2 * v386];
  v385[69685] += 18;
  v385[69686] += 18;
  v385[69687] += 18;
  v385[69688] += 18;
  n0x10 = v387[470036];
  if ( n0x10 > 0x10 )
  {
    v389 = (uint16_t *)v385[(a4 & 1) + 69677];
    v511 = a4 & 1;
    n15 = v386 & 0xF;
    v508 = v389;
    if ( n15 < 15 )
    {
      v391 = v387[470041];
      v392 = v387[470042];
      n2 = (int32_t)(v387 + 470040);
      if ( v387[470043] + v392 + v391 > 29696 )
        __fwd_sub_41CAB0_sub_414800(v387 + 470040);
      v393 = (10 * (uint32_t)v387[470040]) >> 4;
      if ( a4 )
      {
        *(uint16_t *)(n2 - 2 * v511 + 6) += v393;
        __fwd_sub_41CAB0_sub_413430((uint16_t *)__n256_1 + 254 * (uint32_t)v508 + 254, (a4 - 1) >> 1);
      }
      else
      {
        *(uint16_t *)(n2 + 2) += v393;
      }
      if ( n15 <= 0 )
      {
LABEL_37:
        n0x10 = v385[69676];
        if ( LOWORD(v385[2 * n0x10 + 235018]) <= 0x1Au )
          return n0x10;
        v397 = v385[69698];
        v398 = 2 - (*((uint8_t *)v385 + (uint8_t)-(char)a5 + 279984) & 1);
        if ( !*((uint8_t *)v385 + (uint8_t)-(char)a5 + 279984) )
          v398 = *((uint8_t *)v385 + (uint8_t)-(char)a5 + 279984);
        v399 = v385[69702];
        v400 = v385[69696] + (v385[69676] & 0x3F);
        v510 = v398;
        n0x10 = v385[-v385[69710] + 69713]
              + v385[-v385[69706] + 69709]
              + v385[-v399 + 69705]
              + v385[-v397 + 69701]
              + v400;
        n0x10_3 = (uint16_t *)&v385[2 * n0x10];
        n0x10_2 = (uint32_t)n0x10_3;
        if ( n15 < 15 )
        {
          n2_2 = n0x10_3 + 470040;
          v403 = n0x10_3[470041];
          v404 = n0x10_3[470042];
          n2 = (int32_t)n2_2;
          if ( n2_2[3] + v404 + v403 > 29696 )
          {
            n0x10_1 = n0x10;
            __fwd_sub_41CAB0_sub_414800(n2_2);
            n0x10 = n0x10_1;
          }
          v405 = *(uint16_t *)(n0x10_2 + 940080) & 0xFFFC;
          v510 *= 2;
          *(uint16_t *)(n2 + v510 + 2) += v405 >> 2;
          if ( n15 <= 0 )
            goto LABEL_48;
        }
        else
        {
          v510 *= 2;
        }
        v406 = *(uint16_t *)(n0x10_2 + 940066);
        v407 = *(uint16_t *)(n0x10_2 + 940068);
        n2 = n0x10_2 + 940064;
        if ( *(uint16_t *)(n0x10_2 + 940070) + v407 + v406 > 29696 )
        {
          n0x10_1 = n0x10;
          __fwd_sub_41CAB0_sub_414800((uint16_t *)(n0x10_2 + 940064));
          n0x10 = n0x10_1;
        }
        *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)(n0x10_2 + 940064) & 0xFFFC) >> 2;
LABEL_48:
        if ( a4 )
        {
          n2 = (a4 - 1) >> 1;
          n0xF0 = (uint16_t *)((uint8_t)v508 & 0xF0);
          if ( (uint32_t)n0xF0 >= 0xF0
            || (n0x10_1 = n0x10,
                __fwd_sub_41CAB0_sub_413430((uint16_t *)__n256_1 + 254 * (uint32_t)v508 + 4064, n2),
                n0x10 = n0x10_1,
                (int32_t)n0xF0 > 0) )
          {
            n0x10_1 = n0x10;
            __fwd_sub_41CAB0_sub_413430((uint16_t *)__n256_1 + 254 * (uint32_t)v508 - 4064, n2);
            n0x10 = n0x10_1;
          }
        }
        v408 = (uint16_t *)(n0x10_2 + 940072);
        if ( *(uint16_t *)(n0x10_2 + 940078)
           + *(uint16_t *)(n0x10_2 + 940076)
           + *(uint16_t *)(n0x10_2 + 940074) > 29696 )
        {
          n0x10_1 = n0x10;
          __fwd_sub_41CAB0_sub_414800((uint16_t *)(n0x10_2 + 940072));
          n0x10 = n0x10_1;
        }
        *(uint16_t *)((char *)v408 + v510 + 2) += (6 * (uint32_t)*(uint16_t *)(n0x10_2 + 940072)) >> 4;
        if ( !v385[69682] || LOWORD(v385[2 * v385[69676] + 235018]) > 0x100u )
        {
          v409 = 2 - v511;
          if ( !a4 )
            v409 = 0;
          v410 = v385[69694];
          v511 = v409;
          if ( v410 == 1 )
          {
            v491 = (uint16_t *)&v385[2 * v385[69695] + 235018 + 2 * (n0x10 - v385[69696])];
            v492 = HIWORD(v385[2 * v385[69695] + 235018 + 2 * (n0x10 - v385[69696])]);
            v493 = LOWORD(v385[2 * v385[69695] + 235019 + 2 * (n0x10 - v385[69696])]);
            v508 = v491;
            if ( v491[3] + v493 + v492 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v491);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v508 + v510 + 2) += (uint16_t)(*v508 & 0xFFFC) >> 2;
            v494 = v385[69676] - v385[69696];
            n0xF0_1 = (uint16_t *)&v385[2 * v494 + 2 * v385[69695]];
            n0x10_2 = v385[69697] + v494;
            n0xF0 = n0xF0_1;
            n2_3 = n0xF0_1 + 470036;
            v497 = n2_3[2] + n2_3[1];
            v498 = n2_3[3];
            n2 = (int32_t)n2_3;
            if ( v498 + v497 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n2_3);
              n0x10 = n0x10_1;
            }
            v499 = 3 * n0xF0[470036];
            v511 *= 2;
            *(uint16_t *)(n2 + v511 + 2) += v499 >> 4;
            if ( n15 >= 15 )
              goto LABEL_180;
            v500 = n0xF0[470041];
            v501 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v501 + v500 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v502 = (uint16_t *)&v385[2 * n0x10_2 + 235020];
            if ( HIWORD(v385[2 * n0x10_2 + 235021])
               + LOWORD(v385[2 * n0x10_2 + 235021])
               + HIWORD(v385[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800((uint16_t *)&v385[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v502 + v511 + 2) += (uint16_t)(*v502 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_180:
              v503 = n0xF0[470033];
              v504 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v504 + v503 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v411 = (uint16_t *)&v385[2 * n0x10 + 235018 + 2 * (v385[69696] - v385[-v410 + 69697])];
            if ( HIWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69696] - v385[-v410 + 69697])])
               + LOWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69696] - v385[-v410 + 69697])])
               + HIWORD(v385[2 * n0x10 + 235018 + 2 * (v385[69696] - v385[-v410 + 69697])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v411);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v411 + v510 + 2) += (7 * (uint32_t)*v411) >> 4;
            n0xF0 = (uint16_t *)&v385[2 * v385[69696] + 2 * (v385[69676] - v385[v385[69694] + 69695])];
            v412 = n0xF0[470037];
            v413 = n0xF0[470038];
            n2 = (int32_t)(n0xF0 + 470036);
            if ( n0xF0[470039] + v413 + v412 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            v414 = 7 * n0xF0[470036];
            v511 *= 2;
            *(uint16_t *)(n2 + v511 + 2) += v414 >> 4;
            if ( n15 >= 15 )
              goto LABEL_67;
            v415 = n0xF0[470041];
            v416 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v416 + v415 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n0x10_1 = n0x10;
            *(uint16_t *)(n2 + v511 + 2) = *(uint16_t *)(v511 + n2 + 2) + ((5 * (uint32_t)n0xF0[470040]) >> 4);
            n0x10 = n0x10_1;
            if ( n15 > 0 )
            {
LABEL_67:
              v417 = n0xF0[470033];
              v418 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v418 + v417 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v419 = v385[69698];
          if ( v419 == 1 )
          {
            n2_4 = (uint16_t *)&v385[2 * v385[69699] + 235018 + 2 * (n0x10 - v385[69700])];
            v481 = HIWORD(v385[2 * v385[69699] + 235018 + 2 * (n0x10 - v385[69700])]);
            v482 = LOWORD(v385[2 * v385[69699] + 235019 + 2 * (n0x10 - v385[69700])]);
            n2 = (int32_t)n2_4;
            if ( n2_4[3] + v482 + v481 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n2_4);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v483 = v385[69676] - v385[69700];
            n0xF0_2 = (uint16_t *)&v385[2 * v483 + 2 * v385[69699]];
            n0x10_2 = v385[69701] + v483;
            n0xF0 = n0xF0_2;
            v485 = n0xF0_2 + 470036;
            if ( v485[3] + v485[2] + v485[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v485);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v485 + v511 + 2) += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_167;
            v486 = n0xF0[470041];
            v487 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v487 + v486 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v488 = (uint16_t *)&v385[2 * n0x10_2 + 235020];
            if ( HIWORD(v385[2 * n0x10_2 + 235021])
               + LOWORD(v385[2 * n0x10_2 + 235021])
               + HIWORD(v385[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800((uint16_t *)&v385[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v488 + v511 + 2) += (uint16_t)(*v488 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_167:
              v489 = n0xF0[470033];
              v490 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v490 + v489 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v420 = (uint16_t *)&v385[2 * n0x10 + 235018 + 2 * (v385[69700] - v385[-v419 + 69701])];
            if ( HIWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69700] - v385[-v419 + 69701])])
               + LOWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69700] - v385[-v419 + 69701])])
               + HIWORD(v385[2 * n0x10 + 235018 + 2 * (v385[69700] - v385[-v419 + 69701])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v420);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v420 + v510 + 2) += (7 * (uint32_t)*v420) >> 4;
            n0xF0 = (uint16_t *)&v385[2 * v385[69700] + 2 * (v385[69676] - v385[v385[69698] + 69699])];
            v421 = n0xF0[470037];
            v422 = n0xF0[470038];
            n2 = (int32_t)(n0xF0 + 470036);
            if ( n0xF0[470039] + v422 + v421 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_79;
            v423 = n0xF0[470041];
            v424 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v424 + v423 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(v511 + n2 + 2) += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_79:
              v425 = n0xF0[470033];
              v426 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v426 + v425 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v427 = v385[69702];
          if ( v427 == 1 )
          {
            n2_5 = (uint16_t *)&v385[2 * v385[69703] + 235018 + 2 * (n0x10 - v385[69704])];
            v470 = HIWORD(v385[2 * v385[69703] + 235018 + 2 * (n0x10 - v385[69704])]);
            v471 = LOWORD(v385[2 * v385[69703] + 235019 + 2 * (n0x10 - v385[69704])]);
            n2 = (int32_t)n2_5;
            if ( n2_5[3] + v471 + v470 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n2_5);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v472 = v385[69676] - v385[69704];
            n0xF0_3 = (uint16_t *)&v385[2 * v472 + 2 * v385[69703]];
            n0x10_2 = v385[69705] + v472;
            n0xF0 = n0xF0_3;
            v474 = n0xF0_3 + 470036;
            if ( v474[3] + v474[2] + v474[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v474);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v474 + v511 + 2) += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_154;
            v475 = n0xF0[470041];
            v476 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v476 + v475 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v477 = (uint16_t *)&v385[2 * n0x10_2 + 235020];
            if ( HIWORD(v385[2 * n0x10_2 + 235021])
               + LOWORD(v385[2 * n0x10_2 + 235021])
               + HIWORD(v385[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800((uint16_t *)&v385[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v477 + v511 + 2) += (uint16_t)(*v477 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_154:
              v478 = n0xF0[470033];
              v479 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v479 + v478 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v428 = (uint16_t *)&v385[2 * n0x10 + 235018 + 2 * (v385[69704] - v385[-v427 + 69705])];
            if ( HIWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69704] - v385[-v427 + 69705])])
               + LOWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69704] - v385[-v427 + 69705])])
               + HIWORD(v385[2 * n0x10 + 235018 + 2 * (v385[69704] - v385[-v427 + 69705])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v428);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v428 + v510 + 2) += (7 * (uint32_t)*v428) >> 4;
            n0xF0 = (uint16_t *)&v385[2 * v385[69704] + 2 * (v385[69676] - v385[v385[69702] + 69703])];
            v429 = n0xF0[470037];
            v430 = n0xF0[470038];
            n2 = (int32_t)(n0xF0 + 470036);
            if ( n0xF0[470039] + v430 + v429 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_91;
            v431 = n0xF0[470041];
            v432 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v432 + v431 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(v511 + n2 + 2) += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_91:
              v433 = n0xF0[470033];
              v434 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v434 + v433 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v435 = v385[69706];
          if ( v435 == 1 )
          {
            n2_6 = (uint16_t *)&v385[2 * v385[69707] + 235018 + 2 * (n0x10 - v385[69708])];
            v459 = HIWORD(v385[2 * v385[69707] + 235018 + 2 * (n0x10 - v385[69708])]);
            v460 = LOWORD(v385[2 * v385[69707] + 235019 + 2 * (n0x10 - v385[69708])]);
            n2 = (int32_t)n2_6;
            if ( n2_6[3] + v460 + v459 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n2_6);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v461 = v385[69676] - v385[69708];
            n0xF0_4 = (uint16_t *)&v385[2 * v461 + 2 * v385[69707]];
            n0x10_2 = v385[69709] + v461;
            n0xF0 = n0xF0_4;
            v463 = n0xF0_4 + 470036;
            if ( v463[3] + v463[2] + v463[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v463);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v463 + v511 + 2) += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_141;
            v464 = n0xF0[470041];
            v465 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v465 + v464 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v466 = (uint16_t *)&v385[2 * n0x10_2 + 235020];
            if ( HIWORD(v385[2 * n0x10_2 + 235021])
               + LOWORD(v385[2 * n0x10_2 + 235021])
               + HIWORD(v385[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800((uint16_t *)&v385[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v466 + v511 + 2) += (uint16_t)(*v466 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_141:
              v467 = n0xF0[470033];
              v468 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v468 + v467 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v436 = (uint16_t *)&v385[2 * n0x10 + 235018 + 2 * (v385[69708] - v385[-v435 + 69709])];
            if ( HIWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69708] - v385[-v435 + 69709])])
               + LOWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69708] - v385[-v435 + 69709])])
               + HIWORD(v385[2 * n0x10 + 235018 + 2 * (v385[69708] - v385[-v435 + 69709])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(v436);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v436 + v510 + 2) += (7 * (uint32_t)*v436) >> 4;
            n0xF0 = (uint16_t *)&v385[2 * v385[69708] + 2 * (v385[69676] - v385[v385[69706] + 69707])];
            v437 = n0xF0[470037];
            v438 = n0xF0[470038];
            n2 = (int32_t)(n0xF0 + 470036);
            if ( n0xF0[470039] + v438 + v437 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_103;
            v439 = n0xF0[470041];
            v440 = n0xF0[470042];
            n2 = (int32_t)(n0xF0 + 470040);
            if ( n0xF0[470043] + v440 + v439 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_sub_41CAB0_sub_414800(n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(v511 + n2 + 2) += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_103:
              v441 = n0xF0[470033];
              v442 = n0xF0[470034];
              n2 = (int32_t)(n0xF0 + 470032);
              if ( n0xF0[470035] + v442 + v441 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_sub_41CAB0_sub_414800(n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v443 = v385[69710];
          if ( v443 == 1 )
          {
            n2_7 = (uint16_t *)&v385[2 * v385[69711] + 235018 + 2 * (n0x10 - v385[69712])];
            v447 = HIWORD(v385[2 * v385[69711] + 235018 + 2 * (n0x10 - v385[69712])]);
            v448 = LOWORD(v385[2 * v385[69711] + 235019 + 2 * (n0x10 - v385[69712])]);
            v449 = n2_7[3];
            n2 = (int32_t)n2_7;
            if ( v449 + v448 + v447 > 29696 )
              __fwd_sub_41CAB0_sub_414800(n2_7);
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v450 = v385[69676] - v385[69712];
            n0x10_4 = (uint16_t *)&v385[2 * v450 + 2 * v385[69711]];
            n0xF0 = (uint16_t *)(v385[69713] + v450);
            n0x10_2 = (uint32_t)n0x10_4;
            v452 = n0x10_4 + 470036;
            if ( n0x10_4[470039] + n0x10_4[470038] + n0x10_4[470037] > 29696 )
              __fwd_sub_41CAB0_sub_414800(v452);
            *(uint16_t *)((char *)v452 + v511 + 2) += (3 * (uint32_t)*(uint16_t *)(n0x10_2 + 940072)) >> 4;
            if ( n15 >= 15 )
              goto LABEL_128;
            v453 = *(uint16_t *)(n0x10_2 + 940086);
            v454 = *(uint16_t *)(n0x10_2 + 940084) + *(uint16_t *)(n0x10_2 + 940082);
            n2 = n0x10_2 + 940080;
            if ( v453 + v454 > 29696 )
              __fwd_sub_41CAB0_sub_414800((uint16_t *)(n0x10_2 + 940080));
            n0xF0_5 = n0xF0;
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(*(uint16_t *)(n0x10_2 + 940080) & 0xFFFC) >> 2;
            v456 = (uint16_t *)&v385[2 * (uint32_t)n0xF0_5 + 235020];
            if ( v456[3] + v456[2] + v456[1] > 29696 )
              __fwd_sub_41CAB0_sub_414800(v456);
            n0x10 = (uint16_t)(*v456 & 0xFFF8) >> 3;
            *(uint16_t *)((char *)v456 + v511 + 2) += n0x10;
            if ( n15 > 2 )
            {
LABEL_128:
              v457 = (uint16_t *)(n0x10_2 + 940064);
              if ( *(uint16_t *)(n0x10_2 + 940070)
                 + *(uint16_t *)(n0x10_2 + 940068)
                 + *(uint16_t *)(n0x10_2 + 940066) > 29696 )
                __fwd_sub_41CAB0_sub_414800((uint16_t *)(n0x10_2 + 940064));
              n0x10 = n0x10_2;
              *(uint16_t *)((char *)v457 + v511 + 2) += (6
                                                               * (uint32_t)*(uint16_t *)(n0x10_2 + 940064)) >> 4;
            }
          }
          else
          {
            v444 = (uint16_t *)&v385[2 * n0x10 + 235018 + 2 * (v385[69712] - v385[-v443 + 69713])];
            if ( HIWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69712] - v385[-v443 + 69713])])
               + LOWORD(v385[2 * n0x10 + 235019 + 2 * (v385[69712] - v385[-v443 + 69713])])
               + HIWORD(v385[2 * n0x10 + 235018 + 2 * (v385[69712] - v385[-v443 + 69713])]) > 29696 )
              __fwd_sub_41CAB0_sub_414800(v444);
            *(uint16_t *)((char *)v444 + v510 + 2) += (7 * (uint32_t)*v444) >> 4;
            v445 = (uint16_t *)&v385[2 * v385[69712] + 2 * (v385[69676] - v385[v385[69710] + 69711])];
            if ( v445[470039] + v445[470038] + v445[470037] > 29696 )
              __fwd_sub_41CAB0_sub_414800(v445 + 470036);
            *(uint16_t *)((char *)v445 + v511 + 940074) += (7 * (uint32_t)v445[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_115;
            if ( v445[470043] + v445[470042] + v445[470041] > 29696 )
              __fwd_sub_41CAB0_sub_414800(v445 + 470040);
            n0x10 = v445[470040];
            *(uint16_t *)((char *)v445 + v511 + 940082) += (5 * n0x10) >> 4;
            if ( n15 > 0 )
            {
LABEL_115:
              if ( v445[470035] + v445[470034] + v445[470033] > 29696 )
                __fwd_sub_41CAB0_sub_414800(v445 + 470032);
              n0x10 = v445[470032];
              *(uint16_t *)((char *)v445 + v511 + 940066) += (6 * n0x10) >> 4;
            }
          }
        }
        return n0x10;
      }
      v387 = (uint16_t *)&v385[2 * v385[69676]];
    }
    v394 = v387[470033];
    v395 = v387[470034];
    n2 = (int32_t)(v387 + 470032);
    if ( v387[470035] + v395 + v394 > 29696 )
      __fwd_sub_41CAB0_sub_414800(v387 + 470032);
    v396 = (13 * (uint32_t)v387[470032]) >> 4;
    if ( a4 )
    {
      *(uint16_t *)(n2 - 2 * v511 + 6) += v396;
      __fwd_sub_41CAB0_sub_413430((uint16_t *)__n256_1 + 254 * (uint32_t)v508 - 254, (a4 - 1) >> 1);
    }
    else
    {
      *(uint16_t *)(n2 + 2) += v396;
    }
    goto LABEL_37;
  }
  return n0x10;
}

static inline int32_t __fwd_sub_422DB0_sub_414620(void *a0, int32_t a1) { return __sub_414620((uint16_t *)a0, a1); }
static inline int32_t __fwd_sub_422DB0_sub_41A130(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __sub_41A130((__m128 *)a0, a1, a2, (uint32_t *)a3, (uint32_t *)a4); }

BMF_SSE void __sub_422DB0(int32_t lpAddress, char ArgList, const __m128 &a3__ref, const __m128 &a4__ref, uint8_t *a5, int32_t i, int32_t a7)
{
  alignas(16) uint8_t __hexrays_frame[44];
  int32_t &v95 = *(int32_t *)(__hexrays_frame + 0);
  uint32_t &v96 = *(uint32_t *)(__hexrays_frame + 0);
  uint8_t * &v97 = *(uint8_t * *)(__hexrays_frame + 8);
  ;
  __m128 a3 = a3__ref;
  __m128 a4 = a4__ref;
  __m128 v47;
  __m128i *v53;
  bool v17;
  char v9, v45, v46;
  int16_t v14, v24;
  int32_t i_1, v11, v12, v13, v18, v19, v20, v21, v22, v23, v25, v26, v27, v28, v29, v30, v31,
          v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v48, v49, *v50, v51,
          v52, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63, v64, v65, v66, v67, v68, v69,
          v70, v71, v72, v73, v74, v75, v76, v77, v78, v79, v80, v81, v82, v83, v84, v85, v86,
          v87, v88, v89, v90, v92, v93, v94;
  int64_t v16;
  uint16_t *v15;
  uint32_t j;
  __sub_4149C0(ArgList);
  i_1 = i;
  v11 = *(uint32_t *)(lpAddress + 278736);
  v12 = v11;
  if ( i > 0 )
  {
    v95 = 0;
    while ( 1 )
    {
      v13 = *(int16_t *)(v11 - 18) >> 4;
      *(uint32_t *)(lpAddress + 278708) = *(uint32_t *)(lpAddress + 278704) + *(uint32_t *)(lpAddress + 4 * v13 + 278960);
      *(uint32_t *)(lpAddress + 278712) = *(uint32_t *)(lpAddress + 278704) + *(uint32_t *)(lpAddress + 4 * v13 + 278944);
      v14 = (uint8_t)((*(uint16_t *)(*(uint32_t *)(lpAddress + 278736) - 18) >> 4)
                            + *(uint8_t *)(__fwd_sub_422DB0_sub_414620(
                                           (uint16_t *)(lpAddress
                                                              + 8
                                                              * (*(uint32_t *)(lpAddress + 278704)
                                                               + *(uint32_t *)(lpAddress
                                                                           + 4
                                                                           * ((*(int16_t *)(v12 - 18) <= *(int16_t *)(v12 - 36))
                                                                            + (*(int16_t *)(v12 - 18) < *(int16_t *)(v12 - 36)))
                                                                           + 278828)
                                                               + *(uint32_t *)(lpAddress
                                                                           + 4 * *(uint8_t *)(v12 - 20)
                                                                           + 278812)
                                                               + *(uint32_t *)(lpAddress
                                                                           + 4 * *(uint8_t *)(v12 - 2)
                                                                           + 278796)
                                                               + *(uint32_t *)(lpAddress
                                                                           + 4
                                                                           * (((uint32_t)(v13 - 115) >> 31)
                                                                            + ((uint32_t)(v13 - 17) >> 31))
                                                                           + 278780)
                                                               + *(uint32_t *)(lpAddress + 278848))
                                                              + 940072),
                                           lpAddress + 278708)
                                       + lpAddress
                                       + 280496));
      *a5 = v14;
      v14 *= 16;
      **(uint16_t **)(lpAddress + 278736) = v14;
      ++a5;
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 2) = v14;
      v15 = *(uint16_t **)(lpAddress + 278736);
      v16 = (int16_t)(*v15 - *(v15 - 9));
      v15[2] = v16;
      LOWORD(v16) = (WORD2(v16) ^ v16) - WORD2(v16);
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 6) = v16;
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 14) = v16;
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 12) = v16;
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 10) = v16;
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 8) = (uint32_t)*(int16_t *)(*(uint32_t *)(lpAddress + 278736) + 10) >> 1;
      *(uint8_t *)(*(uint32_t *)(lpAddress + 278736) + 17) = 2;
      *(uint8_t *)(*(uint32_t *)(lpAddress + 278736) + 16) = (*(int16_t *)(*(uint32_t *)(lpAddress + 278736) + 4) <= 0)
                                                       + (*(int16_t *)(*(uint32_t *)(lpAddress + 278736) + 4) < 0);
      v11 = *(uint32_t *)(lpAddress + 278736) + 18;
      v17 = v95 + 1 < i;
      *(uint32_t *)(lpAddress + 278736) = v11;
      ++v95;
      if ( !v17 )
        break;
      v12 = *(uint32_t *)(lpAddress + 278736);
    }
    i_1 = i;
  }
  v18 = *(uint32_t *)(v11 - 14);
  v19 = *(uint32_t *)(v11 - 10);
  *(uint32_t *)v11 = *(uint32_t *)(v11 - 18);
  v20 = *(uint32_t *)(v11 - 6);
  *(uint32_t *)(v11 + 4) = v18;
  LOWORD(v18) = *(uint16_t *)(v11 - 2);
  *(uint32_t *)(v11 + 8) = v19;
  *(uint32_t *)(v11 + 12) = v20;
  *(uint16_t *)(v11 + 16) = v18;
  v21 = *(uint32_t *)(lpAddress + 278736);
  v22 = *(uint32_t *)(v21 - 10);
  v23 = *(uint32_t *)(v21 - 6);
  *(uint32_t *)(v21 + 18) = *(uint32_t *)(v21 - 18);
  *(uint32_t *)(v21 + 22) = *(uint32_t *)(v21 - 14);
  v24 = *(uint16_t *)(v21 - 2);
  *(uint32_t *)(v21 + 26) = v22;
  *(uint32_t *)(v21 + 30) = v23;
  *(uint16_t *)(v21 + 34) = v24;
  v25 = *(uint32_t *)(lpAddress + 278736);
  v26 = *(uint32_t *)(v25 - 10);
  v27 = *(uint32_t *)(v25 - 6);
  *(uint32_t *)(v25 + 36) = *(uint32_t *)(v25 - 18);
  *(uint32_t *)(v25 + 40) = *(uint32_t *)(v25 - 14);
  LOWORD(v21) = *(uint16_t *)(v25 - 2);
  *(uint32_t *)(v25 + 44) = v26;
  *(uint32_t *)(v25 + 48) = v27;
  *(uint16_t *)(v25 + 52) = v21;
  v28 = *(uint32_t *)(lpAddress + 278736);
  v29 = *(uint32_t *)(v28 - 14);
  v30 = *(uint32_t *)(v28 - 10);
  *(uint32_t *)(v28 + 54) = *(uint32_t *)(v28 - 18);
  v31 = *(uint32_t *)(v28 - 6);
  *(uint32_t *)(v28 + 58) = v29;
  LOWORD(v29) = *(uint16_t *)(v28 - 2);
  *(uint32_t *)(v28 + 62) = v30;
  *(uint32_t *)(v28 + 66) = v31;
  *(uint16_t *)(v28 + 70) = v29;
  v32 = *(uint32_t *)(lpAddress + 278736);
  v33 = *(uint32_t *)(v32 - 14);
  v34 = *(uint32_t *)(v32 - 6);
  *(uint32_t *)(v32 + 72) = *(uint32_t *)(v32 - 18);
  v35 = *(uint32_t *)(v32 - 10);
  *(uint32_t *)(v32 + 76) = v33;
  LOWORD(v33) = *(uint16_t *)(v32 - 2);
  *(uint32_t *)(v32 + 80) = v35;
  *(uint32_t *)(v32 + 84) = v34;
  *(uint16_t *)(v32 + 88) = v33;
  v36 = *(uint32_t *)(lpAddress + 278736);
  v37 = -18 * i_1;
  *(uint32_t *)(v36 + v37 - 18) = *(uint32_t *)(v36 - 18 * i_1);
  *(uint32_t *)(v36 + v37 - 14) = *(uint32_t *)(v36 - 18 * i_1 + 4);
  *(uint32_t *)(v36 + v37 - 10) = *(uint32_t *)(v36 - 18 * i_1 + 8);
  *(uint32_t *)(v36 + v37 - 6) = *(uint32_t *)(v36 - 18 * i_1 + 12);
  *(uint16_t *)(v36 + v37 - 2) = *(uint16_t *)(v36 - 18 * i_1 + 16);
  v38 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v38 + v37 - 36) = *(uint32_t *)(v38 - 18 * i_1 + 18);
  *(uint32_t *)(v38 + v37 - 32) = *(uint32_t *)(v38 - 18 * i_1 + 22);
  *(uint32_t *)(v38 + v37 - 28) = *(uint32_t *)(v38 - 18 * i_1 + 26);
  *(uint32_t *)(v38 + v37 - 24) = *(uint32_t *)(v38 - 18 * i_1 + 30);
  *(uint16_t *)(v38 + v37 - 20) = *(uint16_t *)(v38 - 18 * i_1 + 34);
  v39 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v39 + v37 - 54) = *(uint32_t *)(v39 - 18 * i_1 + 36);
  *(uint32_t *)(v39 + v37 - 50) = *(uint32_t *)(v39 - 18 * i_1 + 40);
  *(uint32_t *)(v39 + v37 - 46) = *(uint32_t *)(v39 - 18 * i_1 + 44);
  *(uint32_t *)(v39 + v37 - 42) = *(uint32_t *)(v39 - 18 * i_1 + 48);
  *(uint16_t *)(v39 + v37 - 38) = *(uint16_t *)(v39 - 18 * i_1 + 52);
  v40 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v40 + v37 - 72) = *(uint32_t *)(v40 - 18 * i_1 + 54);
  *(uint32_t *)(v40 + v37 - 68) = *(uint32_t *)(v40 - 18 * i_1 + 58);
  *(uint32_t *)(v40 + v37 - 64) = *(uint32_t *)(v40 - 18 * i_1 + 62);
  *(uint32_t *)(v40 + v37 - 60) = *(uint32_t *)(v40 - 18 * i_1 + 66);
  *(uint16_t *)(v40 + v37 - 56) = *(uint16_t *)(v40 - 18 * i_1 + 70);
  v41 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v41 + v37 - 90) = *(uint32_t *)(v41 - 18 * i_1 + 72);
  *(uint32_t *)(v41 + v37 - 86) = *(uint32_t *)(v41 - 18 * i_1 + 76);
  *(uint32_t *)(v41 + v37 - 82) = *(uint32_t *)(v41 - 18 * i_1 + 80);
  *(uint32_t *)(v41 + v37 - 78) = *(uint32_t *)(v41 - 18 * i_1 + 84);
  *(uint16_t *)(v41 + v37 - 74) = *(uint16_t *)(v41 - 18 * i_1 + 88);
  v42 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v42 + v37 - 108) = *(uint32_t *)(v42 - 18 * i_1 + 90);
  *(uint32_t *)(v42 + v37 - 104) = *(uint32_t *)(v42 - 18 * i_1 + 94);
  *(uint32_t *)(v42 + v37 - 100) = *(uint32_t *)(v42 - 18 * i_1 + 98);
  *(uint32_t *)(v42 + v37 - 96) = *(uint32_t *)(v42 - 18 * i_1 + 102);
  *(uint16_t *)(v42 + v37 - 92) = *(uint16_t *)(v42 - 18 * i_1 + 106);
  v43 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v43 + v37 - 126) = *(uint32_t *)(v43 - 18 * i_1 + 108);
  *(uint32_t *)(v43 + v37 - 122) = *(uint32_t *)(v43 - 18 * i_1 + 112);
  *(uint32_t *)(v43 + v37 - 118) = *(uint32_t *)(v43 - 18 * i_1 + 116);
  *(uint32_t *)(v43 + v37 - 114) = *(uint32_t *)(v43 - 18 * i_1 + 120);
  *(uint16_t *)(v43 + v37 - 110) = *(uint16_t *)(v43 - 18 * i_1 + 124);
  v44 = *(uint32_t *)(lpAddress + 278736);
  *(uint32_t *)(v44 + v37 - 144) = *(uint32_t *)(v44 - 18 * i_1 + 126);
  *(uint32_t *)(v44 + v37 - 140) = *(uint32_t *)(v44 - 18 * i_1 + 130);
  *(uint32_t *)(v44 + v37 - 136) = *(uint32_t *)(v44 - 18 * i_1 + 134);
  *(uint32_t *)(v44 + v37 - 132) = *(uint32_t *)(v44 - 18 * i_1 + 138);
  *(uint16_t *)(v44 + v37 - 128) = *(uint16_t *)(v44 - 18 * i_1 + 142);
  memcpy(*(char **)(lpAddress + 278760),*(char **)(lpAddress + 278756),18 * i_1 + 234);
  memcpy(*(char **)(lpAddress + 278764),*(char **)(lpAddress + 278756),18 * i_1 + 234);
  memcpy(*(char **)(lpAddress + 278768),*(char **)(lpAddress + 278756),18 * i_1 + 234);
  if ( a7 > 1 )
  {
    v96 = 0;
    v47 = 0;
    do
    {
      v48 = *(uint32_t *)(lpAddress + 278668);
      v49 = *(uint32_t *)(v48 - 4);
      v97 = a5;
      *(uint32_t *)(v48 + 4) = v49;
      **(uint32_t **)(lpAddress + 278668) = v49;
      v50 = *(int32_t **)(lpAddress + 278660);
      v51 = *(uint32_t *)(lpAddress + 278664);
      *(uint32_t *)(lpAddress + 278660) = v51;
      *(uint32_t *)(lpAddress + 278664) = v50;
      v51 += 8;
      v50 += 2;
      *(uint32_t *)(lpAddress + 278668) = v51;
      *(uint32_t *)(lpAddress + 278672) = v50;
      v52 = *v50;
      *(uint32_t *)(v51 - 4) = *v50;
      *(uint32_t *)(*(uint32_t *)(lpAddress + 278668) - 8) = v52;
      *(uint64_t *)(lpAddress + 278528) = v47.m128_u64[0];
      *(uint32_t *)(lpAddress + 278536) = 0;
      *(uint16_t *)(lpAddress + 278540) = 0;
      v53 = (__m128i *)((lpAddress + 278543) & 0xFFFFFFF0);
      *(uint8_t *)(lpAddress + 278542) = 0;
      *(uint64_t *)(lpAddress + 278640) = v47.m128_u64[0];
      *(uint64_t *)(lpAddress + 278648) = v47.m128_u64[0];
      *v53 = v47;
      v53[1] = v47;
      v53[2] = v47;
      v53[3] = v47;
      v53[4] = v47;
      v53[5] = v47;
      v53[6] = v47;
      v54 = *(uint32_t *)(lpAddress + 278736);
      v55 = *(uint32_t *)(v54 - 14);
      v56 = *(uint32_t *)(v54 - 10);
      v57 = *(uint32_t *)(v54 - 6);
      *(uint32_t *)v54 = *(uint32_t *)(v54 - 18);
      LOWORD(v51) = *(uint16_t *)(v54 - 2);
      *(uint32_t *)(v54 + 4) = v55;
      *(uint32_t *)(v54 + 8) = v56;
      *(uint32_t *)(v54 + 12) = v57;
      *(uint16_t *)(v54 + 16) = v51;
      v58 = *(uint32_t *)(lpAddress + 278736);
      v59 = *(uint32_t *)(v58 - 32);
      v60 = *(uint32_t *)(v58 - 28);
      LOWORD(v51) = *(uint16_t *)(v58 - 20);
      *(uint32_t *)(v58 + 18) = *(uint32_t *)(v58 - 36);
      v61 = *(uint32_t *)(v58 - 24);
      *(uint32_t *)(v58 + 22) = v59;
      *(uint32_t *)(v58 + 26) = v60;
      *(uint32_t *)(v58 + 30) = v61;
      *(uint16_t *)(v58 + 34) = v51;
      v62 = *(uint32_t *)(lpAddress + 278736);
      v63 = *(uint32_t *)(v62 - 50);
      v64 = *(uint32_t *)(v62 - 42);
      LOWORD(v51) = *(uint16_t *)(v62 - 38);
      *(uint32_t *)(v62 + 36) = *(uint32_t *)(v62 - 54);
      v65 = *(uint32_t *)(v62 - 46);
      *(uint32_t *)(v62 + 40) = v63;
      *(uint32_t *)(v62 + 44) = v65;
      *(uint32_t *)(v62 + 48) = v64;
      *(uint16_t *)(v62 + 52) = v51;
      v66 = *(uint32_t *)(lpAddress + 278736);
      v67 = *(uint32_t *)(v66 - 64);
      v68 = *(uint32_t *)(v66 - 60);
      LOWORD(v51) = *(uint16_t *)(v66 - 56);
      *(uint32_t *)(v66 + 54) = *(uint32_t *)(v66 - 72);
      *(uint32_t *)(v66 + 58) = *(uint32_t *)(v66 - 68);
      *(uint32_t *)(v66 + 62) = v67;
      *(uint32_t *)(v66 + 66) = v68;
      *(uint16_t *)(v66 + 70) = v51;
      v69 = *(uint32_t *)(lpAddress + 278736);
      v70 = *(uint32_t *)(v69 - 82);
      v71 = *(uint32_t *)(v69 - 78);
      LOWORD(v68) = *(uint16_t *)(v69 - 74);
      *(uint32_t *)(v69 + 72) = *(uint32_t *)(v69 - 90);
      *(uint32_t *)(v69 + 76) = *(uint32_t *)(v69 - 86);
      *(uint32_t *)(v69 + 80) = v70;
      *(uint32_t *)(v69 + 84) = v71;
      *(uint16_t *)(v69 + 88) = v68;
      v72 = *(uint32_t *)(lpAddress + 278768);
      v73 = *(uint32_t *)(lpAddress + 278772);
      v74 = *(uint32_t *)(lpAddress + 278764);
      v75 = *(uint32_t *)(lpAddress + 278760);
      v76 = *(uint32_t *)(lpAddress + 278756);
      *(uint32_t *)(lpAddress + 278772) = v72;
      *(uint32_t *)(lpAddress + 278768) = v74;
      *(uint32_t *)(lpAddress + 278764) = v75;
      *(uint32_t *)(lpAddress + 278756) = v73;
      *(uint32_t *)(lpAddress + 278760) = v76;
      v73 += 144;
      *(uint32_t *)(lpAddress + 278736) = v73;
      v76 += 144;
      *(uint32_t *)(lpAddress + 278740) = v76;
      *(uint32_t *)(lpAddress + 278744) = v75 + 144;
      *(uint32_t *)(lpAddress + 278748) = v74 + 144;
      *(uint32_t *)(lpAddress + 278752) = v72 + 144;
      *(uint32_t *)(v73 - 18) = *(uint32_t *)v76;
      *(uint32_t *)(v73 - 14) = *(uint32_t *)(v76 + 4);
      *(uint32_t *)(v73 - 10) = *(uint32_t *)(v76 + 8);
      *(uint32_t *)(v73 - 6) = *(uint32_t *)(v76 + 12);
      *(uint16_t *)(v73 - 2) = *(uint16_t *)(v76 + 16);
      v77 = *(uint32_t *)(lpAddress + 278736);
      v78 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v77 - 36) = *(uint32_t *)(v78 + 18);
      *(uint32_t *)(v77 - 32) = *(uint32_t *)(v78 + 22);
      *(uint32_t *)(v77 - 28) = *(uint32_t *)(v78 + 26);
      *(uint32_t *)(v77 - 24) = *(uint32_t *)(v78 + 30);
      *(uint16_t *)(v77 - 20) = *(uint16_t *)(v78 + 34);
      v79 = *(uint32_t *)(lpAddress + 278736);
      v80 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v79 - 54) = *(uint32_t *)(v80 + 36);
      *(uint32_t *)(v79 - 50) = *(uint32_t *)(v80 + 40);
      *(uint32_t *)(v79 - 46) = *(uint32_t *)(v80 + 44);
      *(uint32_t *)(v79 - 42) = *(uint32_t *)(v80 + 48);
      *(uint16_t *)(v79 - 38) = *(uint16_t *)(v80 + 52);
      v81 = *(uint32_t *)(lpAddress + 278736);
      v82 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v81 - 72) = *(uint32_t *)(v82 + 54);
      *(uint32_t *)(v81 - 68) = *(uint32_t *)(v82 + 58);
      *(uint32_t *)(v81 - 64) = *(uint32_t *)(v82 + 62);
      *(uint32_t *)(v81 - 60) = *(uint32_t *)(v82 + 66);
      *(uint16_t *)(v81 - 56) = *(uint16_t *)(v82 + 70);
      v83 = *(uint32_t *)(lpAddress + 278736);
      v84 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v83 - 90) = *(uint32_t *)(v84 + 72);
      *(uint32_t *)(v83 - 86) = *(uint32_t *)(v84 + 76);
      *(uint32_t *)(v83 - 82) = *(uint32_t *)(v84 + 80);
      *(uint32_t *)(v83 - 78) = *(uint32_t *)(v84 + 84);
      *(uint16_t *)(v83 - 74) = *(uint16_t *)(v84 + 88);
      v85 = *(uint32_t *)(lpAddress + 278736);
      v86 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v85 - 108) = *(uint32_t *)(v86 + 90);
      *(uint32_t *)(v85 - 104) = *(uint32_t *)(v86 + 94);
      *(uint32_t *)(v85 - 100) = *(uint32_t *)(v86 + 98);
      *(uint32_t *)(v85 - 96) = *(uint32_t *)(v86 + 102);
      *(uint16_t *)(v85 - 92) = *(uint16_t *)(v86 + 106);
      v87 = *(uint32_t *)(lpAddress + 278736);
      v88 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v87 - 126) = *(uint32_t *)(v88 + 108);
      *(uint32_t *)(v87 - 122) = *(uint32_t *)(v88 + 112);
      *(uint32_t *)(v87 - 118) = *(uint32_t *)(v88 + 116);
      *(uint32_t *)(v87 - 114) = *(uint32_t *)(v88 + 120);
      *(uint16_t *)(v87 - 110) = *(uint16_t *)(v88 + 124);
      v89 = *(uint32_t *)(lpAddress + 278736);
      v90 = *(uint32_t *)(lpAddress + 278740);
      *(uint32_t *)(v89 - 144) = *(uint32_t *)(v90 + 126);
      *(uint32_t *)(v89 - 140) = *(uint32_t *)(v90 + 130);
      *(uint32_t *)(v89 - 136) = *(uint32_t *)(v90 + 134);
      *(uint32_t *)(v89 - 132) = *(uint32_t *)(v90 + 138);
      *(uint16_t *)(v89 - 128) = *(uint16_t *)(v90 + 142);
      *(uint16_t *)(*(uint32_t *)(lpAddress + 278736) + 2) = 0;
      if ( i > 0 )
      {
        for ( j = 0; j < i; ++j )
        {
          v92 = __fwd_sub_422DB0_sub_41A130((__m128 *)lpAddress, v47, a3, nullptr, nullptr);
          v93 = __fwd_sub_422DB0_sub_414620(
                  (uint16_t *)(lpAddress + 8 * *(uint32_t *)(lpAddress + 278704) + 940072),
                  lpAddress + 278708);
          v94 = (uint8_t)(v92 + *(uint8_t *)(v93 + lpAddress + 280496));
          v97[j] = v94;
          __sub_41CAB0(lpAddress, a4, v94, v93, v94 - v92);
          a5 = &v97[j + 1];
        }
        v47 = 0;
      }
      ++v96;
    }
    while ( v96 < a7 - 1 );
  }
  __sub_414920();
}
static inline void ** __fwd_sub_422D60_sub_419610(void *a0, char a1) { return __sub_419610((void **)a0, a1); }
static inline void __fwd_sub_422D60_sub_422DB0(int32_t a0, char a1, const __m128 &a2, const __m128 &a3, void *a4, int32_t a5, int32_t a6) { __sub_422DB0(a0, a1, a2, a3, (uint8_t *)a4, a5, a6); }

BMF_SSE void __sub_422D60(const __m128 &a1__ref, const __m128 &a2__ref, uint8_t *Src, int32_t i, int32_t a5)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  void *v5, **lpAddress;
  v5 = bmf_page_alloc(0x103E30u);
  if ( v5 )
    lpAddress = (void **)__sub_4229E0((int32_t)v5, i, 0);
  else
    lpAddress = nullptr;
  __fwd_sub_422D60_sub_422DB0((int32_t)lpAddress, i, a1, a2, Src, i, a5);
  if ( lpAddress )
    __fwd_sub_422D60_sub_419610(lpAddress, 1);
}
static inline int32_t __fwd_sub_423600_sub_414620(void *a0, int32_t a1) { return __sub_414620((uint16_t *)a0, a1); }
static inline void ** __fwd_sub_423600_sub_419610(void *a0, char a1) { return __sub_419610((void **)a0, a1); }
static inline int32_t __fwd_sub_423600_sub_41A130(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __sub_41A130((__m128 *)a0, a1, a2, (uint32_t *)a3, (uint32_t *)a4); }

 BMF_SSE int32_t __sub_423600(uint16_t *p_i, uint8_t *Src)
{
  alignas(16) uint8_t __hexrays_frame[276];
  uint32_t &Size_1 = *(uint32_t *)(__hexrays_frame + 0);
  __m128i &v140 = *(__m128i *)(__hexrays_frame + 4);
  __m128i &v141 = *(__m128i *)(__hexrays_frame + 20);
  __m128i &v142 = *(__m128i *)(__hexrays_frame + 36);
  __m128i &v143 = *(__m128i *)(__hexrays_frame + 52);
  __m128i &v144 = *(__m128i *)(__hexrays_frame + 68);
  __m128i &v145 = *(__m128i *)(__hexrays_frame + 84);
  __m128i &v146 = *(__m128i *)(__hexrays_frame + 100);
  __m128i &v147 = *(__m128i *)(__hexrays_frame + 116);
  __m128i &v148 = *(__m128i *)(__hexrays_frame + 132);
  int32_t &v149 = *(int32_t *)(__hexrays_frame + 148);
  int32_t &v150 = *(int32_t *)(__hexrays_frame + 152);
  int32_t &v151 = *(int32_t *)(__hexrays_frame + 152);
  uint8_t * &Src_1 = *(uint8_t * *)(__hexrays_frame + 156);
  int32_t &v153 = *(int32_t *)(__hexrays_frame + 160);
  int32_t &v154 = *(int32_t *)(__hexrays_frame + 164);
  void * &lpAddress = *(void * *)(__hexrays_frame + 168);
  __m128 * &v156 = *(__m128 * *)(__hexrays_frame + 172);
  __m128 * &v157 = *(__m128 * *)(__hexrays_frame + 176);
  __m128 * &v158 = *(__m128 * *)(__hexrays_frame + 180);
  int32_t &v159 = *(int32_t *)(__hexrays_frame + 184);
  int32_t &v160 = *(int32_t *)(__hexrays_frame + 188);
  uint32_t &Size = *(uint32_t *)(__hexrays_frame + 192);
  int32_t &ArgList = *(int32_t *)(__hexrays_frame + 196);
  int32_t &v163 = *(int32_t *)(__hexrays_frame + 200);
  uint32_t &i_1 = *(uint32_t *)(__hexrays_frame + 204);
  uint32_t &v165 = *(uint32_t *)(__hexrays_frame + 208);
  int32_t &n4_2 = *(int32_t *)(__hexrays_frame + 212);
  uint32_t &i_2 = *(uint32_t *)(__hexrays_frame + 216);
  int32_t &v168 = *(int32_t *)(__hexrays_frame + 220);
  int32_t &v169 = *(int32_t *)(__hexrays_frame + 224);
  int32_t &v170 = *(int32_t *)(__hexrays_frame + 228);
  uint32_t &v171 = *(uint32_t *)(__hexrays_frame + 232);
  ;
  __m128 v2, v3, *v111, *v120, *v129;
  __m128i *v62;
  bool v17, v109;
  char v9, v10, v19, v20, v54, v55;
  int16_t v110;
  int32_t i, v5, n4, n4_1, v14, v15, v16, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31,
          v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48,
          v49, v50, v51, v52, v53, v56, v57, v58, *v59, v60, v61, v63, v64, v65, v66, v67, v68,
          v69, v70, v71, v72, v73, v74, v75, v76, v77, v78, v79, v80, v81, v82, v83, v84, v85,
          v86, v87, v88, v89, v90, v91, v92, v93, v94, v95, v96, v97, v98, v99, v101, v103, v104,
          v105, v106, v107, v108, v112, v113, v114, v115, v116, v117, v118, v119, v121, v122,
          v123, v124, v125, v126, v127, v128, v130, v131, v132, v133, v134, v135, n4_3, n4_4;
  uint16_t *v102;
  uint32_t v11, *lpAddress_1;
  void *v7, *v8, **lpAddress_2;
  void * *v18;
  v140 = __xmmword_441120;
  v141 = __xmmword_441130;
  v142 = __xmmword_441140;
  v143 = __xmmword_441160;
  v144 = __xmmword_441170;
  v145 = __xmmword_441180;
  v146 = __xmmword_4411D0;
  v147 = __xmmword_4411E0;
  v148 = __xmmword_4411F0;
  v2 = _mm_add_ps((__m128)__xmmword_441120, (__m128)__xmmword_441160);
  v3 = _mm_add_ps((__m128)__xmmword_441130, (__m128)__xmmword_441170);
  __xmmword_441120 = (__m128i)v2;
  __xmmword_441130 = (__m128i)v3;
  __xmmword_441140 = (__m128i)_mm_add_ps((__m128)__xmmword_441140, (__m128)__xmmword_441180);
  __xmmword_4411F0 = __xmmword_439B60;
  __xmmword_4411E0 = __xmmword_439B60;
  __xmmword_4411D0 = __xmmword_439B60;
  __xmmword_441160 = 0;
  __xmmword_441170 = 0;
  __xmmword_441180 = 0;
  i = *p_i;
  v5 = p_i[1];
  if ( __n4_5 > 0 )
  {
    n4 = 0;
    do
    {
      v7 = bmf_page_alloc(0x103E30u);
      if ( v7 )
        v8 = (void *)__sub_4229E0((int32_t)v7, i, n4);
      else
        v8 = nullptr;
      *(&lpAddress + n4++) = v8;
    }
    while ( n4 < __n4_5 );
  }
  v163 = __byte_44339E[16 * (uint8_t)__byte_4433AD[0]] & 8;
  v9 = __byte_44339E[16 * (uint8_t)__n3_0];
  ArgList = __byte_44339E[16 * (uint8_t)__byte_4433BD[0]] & 8;
  v153 = v9 & 8;
  __sub_4149C0(ArgList);
  if ( v5 > 0 )
  {
    v11 = 0;
    v149 = 9 * i;
    v160 = -18 * i;
    n4_1 = __n4_5;
    Size = 18 * i + 234;
    v171 = i + 13;
    do
    {
      if ( n4_1 > 0 )
      {
        v165 = v11;
        n4_2 = 0;
        v150 = v5;
        i_1 = i;
        Src_1 = Src;
        do
        {
          ++n4_2;
          if ( v165 )
          {
            v18 = &lpAddress + n4_2;
            if ( v165 == 1 )
            {
              v25 = (int32_t)*(v18 - 1);
              v26 = *(uint32_t *)(v25 + 278736);
              v27 = *(uint32_t *)(v26 - 14);
              v28 = *(uint32_t *)(v26 - 10);
              *(uint32_t *)v26 = *(uint32_t *)(v26 - 18);
              v29 = *(uint32_t *)(v26 - 6);
              *(uint32_t *)(v26 + 4) = v27;
              LOWORD(v27) = *(uint16_t *)(v26 - 2);
              *(uint32_t *)(v26 + 8) = v28;
              *(uint32_t *)(v26 + 12) = v29;
              *(uint16_t *)(v26 + 16) = v27;
              v30 = *(uint32_t *)(v25 + 278736);
              v31 = *(uint32_t *)(v30 - 14);
              v32 = *(uint32_t *)(v30 - 10);
              *(uint32_t *)(v30 + 18) = *(uint32_t *)(v30 - 18);
              v33 = *(uint32_t *)(v30 - 6);
              *(uint32_t *)(v30 + 22) = v31;
              LOWORD(v31) = *(uint16_t *)(v30 - 2);
              *(uint32_t *)(v30 + 26) = v32;
              *(uint32_t *)(v30 + 30) = v33;
              *(uint16_t *)(v30 + 34) = v31;
              v34 = *(uint32_t *)(v25 + 278736);
              v35 = *(uint32_t *)(v34 - 14);
              v36 = *(uint32_t *)(v34 - 6);
              *(uint32_t *)(v34 + 36) = *(uint32_t *)(v34 - 18);
              v37 = *(uint32_t *)(v34 - 10);
              *(uint32_t *)(v34 + 40) = v35;
              LOWORD(v35) = *(uint16_t *)(v34 - 2);
              *(uint32_t *)(v34 + 44) = v37;
              *(uint32_t *)(v34 + 48) = v36;
              *(uint16_t *)(v34 + 52) = v35;
              v38 = *(uint32_t *)(v25 + 278736);
              v39 = *(uint32_t *)(v38 - 10);
              v40 = *(uint32_t *)(v38 - 6);
              *(uint32_t *)(v38 + 54) = *(uint32_t *)(v38 - 18);
              *(uint32_t *)(v38 + 58) = *(uint32_t *)(v38 - 14);
              LOWORD(v34) = *(uint16_t *)(v38 - 2);
              *(uint32_t *)(v38 + 62) = v39;
              *(uint32_t *)(v38 + 66) = v40;
              *(uint16_t *)(v38 + 70) = v34;
              v41 = *(uint32_t *)(v25 + 278736);
              v42 = *(uint32_t *)(v41 - 14);
              v43 = *(uint32_t *)(v41 - 10);
              *(uint32_t *)(v41 + 72) = *(uint32_t *)(v41 - 18);
              v44 = *(uint32_t *)(v41 - 6);
              *(uint32_t *)(v41 + 76) = v42;
              LOWORD(v42) = *(uint16_t *)(v41 - 2);
              *(uint32_t *)(v41 + 80) = v43;
              *(uint32_t *)(v41 + 84) = v44;
              *(uint16_t *)(v41 + 88) = v42;
              v45 = *(uint32_t *)(v25 + 278736);
              v46 = v160;
              Size_1 = Size;
              *(uint32_t *)(v45 + v160 - 18) = *(uint32_t *)(v160 + v45);
              *(uint32_t *)(v45 + v46 - 14) = *(uint32_t *)(v46 + v45 + 4);
              *(uint32_t *)(v45 + v46 - 10) = *(uint32_t *)(v46 + v45 + 8);
              *(uint32_t *)(v45 + v46 - 6) = *(uint32_t *)(v46 + v45 + 12);
              *(uint16_t *)(v45 + v46 - 2) = *(uint16_t *)(v46 + v45 + 16);
              v47 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v47 - 36) = *(uint32_t *)(v46 + v47 + 18);
              *(uint32_t *)(v46 + v47 - 32) = *(uint32_t *)(v46 + v47 + 22);
              *(uint32_t *)(v46 + v47 - 28) = *(uint32_t *)(v46 + v47 + 26);
              *(uint32_t *)(v46 + v47 - 24) = *(uint32_t *)(v46 + v47 + 30);
              *(uint16_t *)(v46 + v47 - 20) = *(uint16_t *)(v46 + v47 + 34);
              v48 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v48 - 54) = *(uint32_t *)(v46 + v48 + 36);
              *(uint32_t *)(v46 + v48 - 50) = *(uint32_t *)(v46 + v48 + 40);
              *(uint32_t *)(v46 + v48 - 46) = *(uint32_t *)(v46 + v48 + 44);
              *(uint32_t *)(v46 + v48 - 42) = *(uint32_t *)(v46 + v48 + 48);
              *(uint16_t *)(v46 + v48 - 38) = *(uint16_t *)(v46 + v48 + 52);
              v49 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v49 - 72) = *(uint32_t *)(v46 + v49 + 54);
              *(uint32_t *)(v46 + v49 - 68) = *(uint32_t *)(v46 + v49 + 58);
              *(uint32_t *)(v46 + v49 - 64) = *(uint32_t *)(v46 + v49 + 62);
              *(uint32_t *)(v46 + v49 - 60) = *(uint32_t *)(v46 + v49 + 66);
              *(uint16_t *)(v46 + v49 - 56) = *(uint16_t *)(v46 + v49 + 70);
              v50 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v50 - 90) = *(uint32_t *)(v46 + v50 + 72);
              *(uint32_t *)(v46 + v50 - 86) = *(uint32_t *)(v46 + v50 + 76);
              *(uint32_t *)(v46 + v50 - 82) = *(uint32_t *)(v46 + v50 + 80);
              *(uint32_t *)(v46 + v50 - 78) = *(uint32_t *)(v46 + v50 + 84);
              *(uint16_t *)(v46 + v50 - 74) = *(uint16_t *)(v46 + v50 + 88);
              v51 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v51 - 108) = *(uint32_t *)(v46 + v51 + 90);
              *(uint32_t *)(v46 + v51 - 104) = *(uint32_t *)(v46 + v51 + 94);
              *(uint32_t *)(v46 + v51 - 100) = *(uint32_t *)(v46 + v51 + 98);
              *(uint32_t *)(v46 + v51 - 96) = *(uint32_t *)(v46 + v51 + 102);
              *(uint16_t *)(v46 + v51 - 92) = *(uint16_t *)(v46 + v51 + 106);
              v52 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v52 - 126) = *(uint32_t *)(v46 + v52 + 108);
              *(uint32_t *)(v46 + v52 - 122) = *(uint32_t *)(v46 + v52 + 112);
              *(uint32_t *)(v46 + v52 - 118) = *(uint32_t *)(v46 + v52 + 116);
              *(uint32_t *)(v46 + v52 - 114) = *(uint32_t *)(v46 + v52 + 120);
              *(uint16_t *)(v46 + v52 - 110) = *(uint16_t *)(v46 + v52 + 124);
              v53 = *(uint32_t *)(v25 + 278736);
              *(uint32_t *)(v46 + v53 - 144) = *(uint32_t *)(v46 + v53 + 126);
              *(uint32_t *)(v46 + v53 - 140) = *(uint32_t *)(v46 + v53 + 130);
              *(uint32_t *)(v46 + v53 - 136) = *(uint32_t *)(v46 + v53 + 134);
              *(uint32_t *)(v46 + v53 - 132) = *(uint32_t *)(v46 + v53 + 138);
              *(uint16_t *)(v46 + v53 - 128) = *(uint16_t *)(v46 + v53 + 142);
              memcpy(*(char **)(v25 + 278760),*(char **)(v25 + 278756),Size_1);
              memcpy(*(char **)(v25 + 278764),*(char **)(v25 + 278756),Size);
              memcpy(*(char **)(v25 + 278768),*(char **)(v25 + 278756),Size);
            }
          }
          else
          {
            v14 = (int32_t)*(&lpAddress + n4_2 - 1);
            v159 = (int32_t)(&lpAddress + n4_2);
            v15 = 0;
            do
            {
              v16 = 9 * v15;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16) = 256;
              v17 = ++v15 < v171;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 2) = 256;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 4) = -16;
              *(uint8_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 16) = 1;
              *(uint8_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 17) = 3;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 6) = 512;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 14) = 512;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 12) = 512;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 10) = 1024;
              *(uint16_t *)(*(uint32_t *)(v14 + 278756) + 2 * v16 + 8) = 256;
            }
            while ( v17 );
            v18 = (void * *)v159;
            memcpy(*(char **)(v14 + 278760),*(char **)(v14 + 278756),Size);
            memcpy(*(char **)(v14 + 278764),*(char **)(v14 + 278756),Size);
            memcpy(*(char **)(v14 + 278768),*(char **)(v14 + 278756),Size);
            v21 = *(uint32_t *)(v14 + 278760) + 2 * v149 + 144;
            *(uint32_t *)(v14 + 278736) = *(uint32_t *)(v14 + 278756) + 2 * v149 + 144;
            v22 = *(uint32_t *)(v14 + 278764);
            *(uint32_t *)(v14 + 278740) = v21;
            v23 = *(uint32_t *)(v14 + 278768);
            *(uint32_t *)(v14 + 278744) = v22 + 2 * v149 + 144;
            v24 = *(uint32_t *)(v14 + 278772) + 2 * v149 + 144;
            *(uint32_t *)(v14 + 278748) = v23 + 2 * v149 + 144;
            *(uint32_t *)(v14 + 278752) = v24;
          }
          v56 = (int32_t)*(v18 - 1);
          v57 = *(uint32_t *)(v56 + 278668);
          v58 = *(uint32_t *)(v57 - 4);
          *(uint32_t *)(v57 + 4) = v58;
          **(uint32_t **)(v56 + 278668) = v58;
          v59 = *(int32_t **)(v56 + 278660);
          v60 = *(uint32_t *)(v56 + 278664);
          *(uint32_t *)(v56 + 278660) = v60;
          *(uint32_t *)(v56 + 278664) = v59;
          v60 += 8;
          *(uint32_t *)(v56 + 278668) = v60;
          v59 += 2;
          v2 = 0;
          *(uint32_t *)(v56 + 278672) = v59;
          v61 = *v59;
          *(uint32_t *)(v60 - 4) = *v59;
          v62 = (__m128i *)((v56 + 278543) & 0xFFFFFFF0);
          *(uint32_t *)(*(uint32_t *)(v56 + 278668) - 8) = v61;
          *(uint64_t *)(v56 + 278528) = 0;
          *(uint32_t *)(v56 + 278536) = 0;
          *(uint16_t *)(v56 + 278540) = 0;
          *(uint8_t *)(v56 + 278542) = 0;
          *(uint64_t *)(v56 + 278640) = 0;
          *(uint64_t *)(v56 + 278648) = 0;
          *v62 = 0;
          v62[1] = 0;
          v62[2] = 0;
          v62[3] = 0;
          v62[4] = 0;
          v62[5] = 0;
          v62[6] = 0;
          v63 = *(uint32_t *)(v56 + 278736);
          v64 = *(uint32_t *)(v63 - 14);
          v65 = *(uint32_t *)(v63 - 10);
          v66 = *(uint32_t *)(v63 - 6);
          *(uint32_t *)v63 = *(uint32_t *)(v63 - 18);
          LOWORD(v60) = *(uint16_t *)(v63 - 2);
          *(uint32_t *)(v63 + 4) = v64;
          *(uint32_t *)(v63 + 8) = v65;
          *(uint32_t *)(v63 + 12) = v66;
          *(uint16_t *)(v63 + 16) = v60;
          v67 = *(uint32_t *)(v56 + 278736);
          v68 = *(uint32_t *)(v67 - 32);
          v69 = *(uint32_t *)(v67 - 28);
          LOWORD(v60) = *(uint16_t *)(v67 - 20);
          *(uint32_t *)(v67 + 18) = *(uint32_t *)(v67 - 36);
          v70 = *(uint32_t *)(v67 - 24);
          *(uint32_t *)(v67 + 22) = v68;
          *(uint32_t *)(v67 + 26) = v69;
          *(uint32_t *)(v67 + 30) = v70;
          *(uint16_t *)(v67 + 34) = v60;
          v71 = *(uint32_t *)(v56 + 278736);
          v72 = *(uint32_t *)(v71 - 50);
          v73 = *(uint32_t *)(v71 - 42);
          LOWORD(v60) = *(uint16_t *)(v71 - 38);
          *(uint32_t *)(v71 + 36) = *(uint32_t *)(v71 - 54);
          v74 = *(uint32_t *)(v71 - 46);
          *(uint32_t *)(v71 + 40) = v72;
          *(uint32_t *)(v71 + 44) = v74;
          *(uint32_t *)(v71 + 48) = v73;
          *(uint16_t *)(v71 + 52) = v60;
          v75 = *(uint32_t *)(v56 + 278736);
          v76 = *(uint32_t *)(v75 - 64);
          v77 = *(uint32_t *)(v75 - 60);
          LOWORD(v60) = *(uint16_t *)(v75 - 56);
          *(uint32_t *)(v75 + 54) = *(uint32_t *)(v75 - 72);
          *(uint32_t *)(v75 + 58) = *(uint32_t *)(v75 - 68);
          *(uint32_t *)(v75 + 62) = v76;
          *(uint32_t *)(v75 + 66) = v77;
          *(uint16_t *)(v75 + 70) = v60;
          v78 = *(uint32_t *)(v56 + 278736);
          v79 = *(uint32_t *)(v78 - 82);
          v80 = *(uint32_t *)(v78 - 78);
          LOWORD(v77) = *(uint16_t *)(v78 - 74);
          *(uint32_t *)(v78 + 72) = *(uint32_t *)(v78 - 90);
          *(uint32_t *)(v78 + 76) = *(uint32_t *)(v78 - 86);
          *(uint32_t *)(v78 + 80) = v79;
          *(uint32_t *)(v78 + 84) = v80;
          *(uint16_t *)(v78 + 88) = v77;
          v81 = *(uint32_t *)(v56 + 278768);
          v82 = *(uint32_t *)(v56 + 278772);
          v83 = *(uint32_t *)(v56 + 278764);
          v84 = *(uint32_t *)(v56 + 278760);
          v85 = *(uint32_t *)(v56 + 278756);
          *(uint32_t *)(v56 + 278772) = v81;
          *(uint32_t *)(v56 + 278768) = v83;
          *(uint32_t *)(v56 + 278764) = v84;
          *(uint32_t *)(v56 + 278756) = v82;
          *(uint32_t *)(v56 + 278760) = v85;
          v82 += 144;
          *(uint32_t *)(v56 + 278736) = v82;
          v85 += 144;
          *(uint32_t *)(v56 + 278740) = v85;
          *(uint32_t *)(v56 + 278744) = v84 + 144;
          *(uint32_t *)(v56 + 278748) = v83 + 144;
          *(uint32_t *)(v56 + 278752) = v81 + 144;
          *(uint32_t *)(v82 - 18) = *(uint32_t *)v85;
          *(uint32_t *)(v82 - 14) = *(uint32_t *)(v85 + 4);
          *(uint32_t *)(v82 - 10) = *(uint32_t *)(v85 + 8);
          *(uint32_t *)(v82 - 6) = *(uint32_t *)(v85 + 12);
          *(uint16_t *)(v82 - 2) = *(uint16_t *)(v85 + 16);
          v86 = *(uint32_t *)(v56 + 278736);
          v87 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v86 - 36) = *(uint32_t *)(v87 + 18);
          *(uint32_t *)(v86 - 32) = *(uint32_t *)(v87 + 22);
          *(uint32_t *)(v86 - 28) = *(uint32_t *)(v87 + 26);
          *(uint32_t *)(v86 - 24) = *(uint32_t *)(v87 + 30);
          *(uint16_t *)(v86 - 20) = *(uint16_t *)(v87 + 34);
          v88 = *(uint32_t *)(v56 + 278736);
          v89 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v88 - 54) = *(uint32_t *)(v89 + 36);
          *(uint32_t *)(v88 - 50) = *(uint32_t *)(v89 + 40);
          *(uint32_t *)(v88 - 46) = *(uint32_t *)(v89 + 44);
          *(uint32_t *)(v88 - 42) = *(uint32_t *)(v89 + 48);
          *(uint16_t *)(v88 - 38) = *(uint16_t *)(v89 + 52);
          v90 = *(uint32_t *)(v56 + 278736);
          v91 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v90 - 72) = *(uint32_t *)(v91 + 54);
          *(uint32_t *)(v90 - 68) = *(uint32_t *)(v91 + 58);
          *(uint32_t *)(v90 - 64) = *(uint32_t *)(v91 + 62);
          *(uint32_t *)(v90 - 60) = *(uint32_t *)(v91 + 66);
          *(uint16_t *)(v90 - 56) = *(uint16_t *)(v91 + 70);
          v92 = *(uint32_t *)(v56 + 278736);
          v93 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v92 - 90) = *(uint32_t *)(v93 + 72);
          *(uint32_t *)(v92 - 86) = *(uint32_t *)(v93 + 76);
          *(uint32_t *)(v92 - 82) = *(uint32_t *)(v93 + 80);
          *(uint32_t *)(v92 - 78) = *(uint32_t *)(v93 + 84);
          *(uint16_t *)(v92 - 74) = *(uint16_t *)(v93 + 88);
          v94 = *(uint32_t *)(v56 + 278736);
          v95 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v94 - 108) = *(uint32_t *)(v95 + 90);
          *(uint32_t *)(v94 - 104) = *(uint32_t *)(v95 + 94);
          *(uint32_t *)(v94 - 100) = *(uint32_t *)(v95 + 98);
          *(uint32_t *)(v94 - 96) = *(uint32_t *)(v95 + 102);
          *(uint16_t *)(v94 - 92) = *(uint16_t *)(v95 + 106);
          v96 = *(uint32_t *)(v56 + 278736);
          v97 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v96 - 126) = *(uint32_t *)(v97 + 108);
          *(uint32_t *)(v96 - 122) = *(uint32_t *)(v97 + 112);
          *(uint32_t *)(v96 - 118) = *(uint32_t *)(v97 + 116);
          *(uint32_t *)(v96 - 114) = *(uint32_t *)(v97 + 120);
          *(uint16_t *)(v96 - 110) = *(uint16_t *)(v97 + 124);
          v98 = *(uint32_t *)(v56 + 278736);
          v99 = *(uint32_t *)(v56 + 278740);
          *(uint32_t *)(v98 - 144) = *(uint32_t *)(v99 + 126);
          *(uint32_t *)(v98 - 140) = *(uint32_t *)(v99 + 130);
          *(uint32_t *)(v98 - 136) = *(uint32_t *)(v99 + 134);
          *(uint32_t *)(v98 - 132) = *(uint32_t *)(v99 + 138);
          *(uint16_t *)(v98 - 128) = *(uint16_t *)(v99 + 142);
          *(uint16_t *)(*(uint32_t *)(v56 + 278736) + 2) = 0;
          n4_1 = __n4_5;
        }
        while ( __n4_5 > n4_2 );
        v11 = v165;
        v5 = v150;
        i = i_1;
        Src = Src_1;
      }
      __dword_4458EC = 0;
      __dword_4458E8 = 0;
      __dword_4458E4 = 0;
      __dword_4458E0 = 0;
      if ( i > 0 )
      {
        v165 = v11;
        i_2 = 0;
        v151 = v5;
        i_1 = i;
        do
        {
          __dword_4458E0 >>= 3;
          __dword_4458E4 >>= 3;
          __dword_4458E8 >>= 3;
          __dword_4458EC >>= 3;
          lpAddress_1 = lpAddress;
          v101 = __fwd_sub_423600_sub_41A130((__m128 *)lpAddress, v2, v3, v157, v156);
          v102 = (uint16_t *)&lpAddress_1[2 * lpAddress_1[69676] + 235018];
          v168 = v101;
          v103 = __fwd_sub_423600_sub_414620(v102, (int32_t)(lpAddress_1 + 69677));
          v104 = (uint8_t)(v168 + *((uint8_t *)lpAddress_1 + v103 + 280496));
          __sub_41CAB0((int32_t)lpAddress_1, (__m128)__xmmword_439B60, v104, v103, v104 - v168);
          v105 = lpAddress_1[69684];
          v106 = *(int16_t *)(v105 - 10);
          v107 = 32 * *(int16_t *)(v105 - 4);
          __dword_4458E0 += 32 * *(int16_t *)(v105 - 6);
          v108 = *(int16_t *)(v105 - 8);
          __dword_4458E4 += v107;
          __dword_4458E8 += 32 * v106;
          __dword_4458EC += 32 * v108;
          v109 = v163 == 0;
          Src[(uint8_t)__byte_44339D[0]] = v104;
          if ( v109 )
            v110 = 0;
          else
            v110 = 16 * Src[(uint8_t)__byte_44339D[0]];
          v111 = v156;
          *(uint16_t *)(v156[17421].m128_i32[0] + 2) = v110;
          v112 = __fwd_sub_423600_sub_41A130(v111, v2, v3, lpAddress, v157);
          v113 = __fwd_sub_423600_sub_414620(&v111[58754].m128_u16[4 * v111[17419].m128_i32[0] + 4], (int32_t)&v111[17419].m128_i32[1]);
          v114 = (uint8_t)(v112 + v111[17531].m128_i8[v113]);
          v169 = v114;
          __sub_41CAB0((int32_t)v111, (__m128)__xmmword_439B60, v114, v113, v114 - v112);
          v115 = v111[17421].m128_i32[0];
          v116 = *(int16_t *)(v115 - 4);
          v117 = *(int16_t *)(v115 - 10);
          __dword_4458E0 += 32 * *(int16_t *)(v115 - 6);
          v118 = *(int16_t *)(v115 - 8);
          __dword_4458E4 += 32 * v116;
          __dword_4458E8 += 32 * v117;
          __dword_4458EC += 32 * v118;
          v109 = ArgList == 0;
          Src[(uint8_t)__byte_4433AD[0]] = v169;
          if ( v109 )
            LOWORD(v119) = 0;
          else
            v119 = (__dword_4433A4[4 * (uint8_t)__byte_4433BD[0]] * Src[(uint8_t)__byte_4433AD[0]]
                  + __dword_4433A0[4 * (uint8_t)__byte_4433BD[0]] * Src[(uint8_t)__byte_44339D[0]]) >> 3;
          v120 = v157;
          *(uint16_t *)(v157[17421].m128_i32[0] + 2) = v119;
          v121 = __fwd_sub_423600_sub_41A130(v120, v2, v3, lpAddress, v156);
          v122 = __fwd_sub_423600_sub_414620(&v120[58754].m128_u16[4 * v120[17419].m128_i32[0] + 4], (int32_t)&v120[17419].m128_i32[1]);
          v123 = (uint8_t)(v121 + v120[17531].m128_i8[v122]);
          v170 = v123;
          __sub_41CAB0((int32_t)v120, (__m128)__xmmword_439B60, v123, v122, v123 - v121);
          v124 = v120[17421].m128_i32[0];
          v125 = *(int16_t *)(v124 - 4);
          v126 = *(int16_t *)(v124 - 10);
          __dword_4458E0 += 32 * *(int16_t *)(v124 - 6);
          v127 = *(int16_t *)(v124 - 8);
          __dword_4458E4 += 32 * v125;
          __dword_4458E8 += 32 * v126;
          __dword_4458EC += 32 * v127;
          Src[(uint8_t)__byte_4433BD[0]] = v170;
          n4_1 = __n4_5;
          if ( __n4_5 >= 4 )
          {
            if ( v153 )
              v128 = (__dword_4433A8[4 * (uint8_t)__n3_0] * Src[2]
                    + __dword_4433A4[4 * (uint8_t)__n3_0] * Src[1]
                    + __dword_4433A0[4 * (uint8_t)__n3_0] * *Src) >> 3;
            else
              LOWORD(v128) = 0;
            v129 = v158;
            *(uint16_t *)(v158[17421].m128_i32[0] + 2) = v128;
            v130 = __fwd_sub_423600_sub_41A130(v129, v2, v3, v157, lpAddress);
            v131 = __fwd_sub_423600_sub_414620(&v129[58754].m128_u16[4 * v129[17419].m128_i32[0] + 4], (int32_t)&v129[17419].m128_i32[1]);
            v154 = (uint8_t)(v130 + v129[17531].m128_i8[v131]);
            __sub_41CAB0((int32_t)v129, (__m128)__xmmword_439B60, v154, v131, v154 - v130);
            v132 = v129[17421].m128_i32[0];
            v133 = *(int16_t *)(v132 - 4);
            v134 = *(int16_t *)(v132 - 10);
            v135 = *(int16_t *)(v132 - 8);
            __dword_4458E0 += 32 * *(int16_t *)(v132 - 6);
            __dword_4458E4 += 32 * v133;
            __dword_4458E8 += 32 * v134;
            __dword_4458EC += 32 * v135;
            Src[3] = v154;
            n4_1 = __n4_5;
          }
          Src += n4_1;
          ++i_2;
        }
        while ( i_2 < i_1 );
        v11 = v165;
        v5 = v151;
        i = i_1;
      }
      ++v11;
    }
    while ( v11 < v5 );
  }
  __sub_414920();
  __xmmword_441120 = v140;
  __xmmword_441130 = v141;
  __xmmword_441140 = v142;
  __xmmword_441160 = v143;
  __xmmword_441170 = v144;
  __xmmword_441180 = v145;
  __xmmword_4411D0 = v146;
  __xmmword_4411E0 = v147;
  __xmmword_4411F0 = v148;
  n4_3 = __n4_5;
  if ( __n4_5 > 0 )
  {
    n4_4 = 0;
    do
    {
      lpAddress_2 = (void **)*(&lpAddress + n4_4);
      if ( lpAddress_2 )
      {
        __fwd_sub_423600_sub_419610(lpAddress_2, 1);
        n4_3 = __n4_5;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline void ** __fwd_sub_417DB0_sub_415270(void *a0, char a1) { return __sub_415270((void **)a0, a1); }
static inline void __fwd_sub_417DB0_sub_417E80(void *a0, void *a1) { __sub_417E80((uint32_t *)a0, (char *)a1); }
static inline void __fwd_sub_417DB0_sub_422D60(const __m128 &a0, const __m128 &a1, void *a2, int32_t a3, int32_t a4) { __sub_422D60(a0, a1, (uint8_t *)a2, a3, a4); }
static inline int32_t __fwd_sub_417DB0_sub_423600(void *a0, void *a1) { return __sub_423600((uint16_t *)a0, (uint8_t *)a1); }
static inline void ** __fwd_sub_417DB0_sub_4273F0(char a0, void *a1, int32_t a2, int32_t a3) { return __sub_4273F0(a0, (uint8_t *)a1, a2, a3); }
static inline int32_t __fwd_sub_417DB0_sub_427740(void *a0, int32_t a1) { return __sub_427740((uint16_t *)a0, a1); }

BMF_SSE void __sub_417DB0(char ArgList, const __m128 &a2__ref, const __m128 &a3__ref, uint16_t *p_i, uint8_t *Src)
{
  ;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  void *v5, **v6;
  if ( __dword_443364 )
  {
    if ( __n2 == 1 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __fwd_sub_417DB0_sub_4273F0(ArgList, Src, *p_i, p_i[1]);
      else
        __fwd_sub_417DB0_sub_427740(p_i, (int32_t)Src);
    }
    else if ( __n2 == 2 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __fwd_sub_417DB0_sub_422D60(a2, a3, Src, *p_i, p_i[1]);
      else
        __fwd_sub_417DB0_sub_423600(p_i, Src);
    }
  }
  else
  {
    v5 = malloc(0x7BA230u);
    if ( v5 )
      v6 = (void **)__sub_417980((int32_t)v5, p_i[1], *p_i, p_i[1], p_i[5] & 0x3F);
    else
      v6 = nullptr;
    __fwd_sub_417DB0_sub_417E80(v6, Src);
    if ( v6 )
      __fwd_sub_417DB0_sub_415270(v6, 1);
  }
}
static inline int32_t __fwd_sub_419800_sub_414390(void *a0, int32_t a1, int32_t a2) { return __sub_414390((uint16_t *)a0, a1, a2); }
static inline int32_t __fwd_sub_419800_sub_41A130(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __sub_41A130((__m128 *)a0, a1, a2, (uint32_t *)a3, (uint32_t *)a4); }

BMF_SSE void __sub_419800(__m128 *lpAddress, const __m128 &a2__ref, const __m128 &a3__ref, uint8_t *a4, int32_t i, int32_t a6, uint8_t *a7)
{
  alignas(16) uint8_t __hexrays_frame[68];
  int32_t &j = *(int32_t *)(__hexrays_frame + 0);
  uint32_t &v102 = *(uint32_t *)(__hexrays_frame + 0);
  uint8_t * &v103 = *(uint8_t * *)(__hexrays_frame + 4);
  int32_t &v104 = *(int32_t *)(__hexrays_frame + 8);
  uint8_t * &v105 = *(uint8_t * *)(__hexrays_frame + 8);
  int8_t * &v106 = *(int8_t * *)(__hexrays_frame + 12);
  char &v107 = *(char *)(__hexrays_frame + 16);
  __m128 * &lpAddress_1 = *(__m128 * *)(__hexrays_frame + 20);
  uint8_t * &v109 = *(uint8_t * *)(__hexrays_frame + 24);
  uint8_t &v110 = *(uint8_t *)(__hexrays_frame + 32);
  ;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  __m128 v50;
  __m128i *v56;
  char v10, v14, v48, v49, v97;
  int16_t v26;
  int32_t v11, v12, v13, n16, v16, v17, v20, v21, v22, v23, v24, v25, v27, v28, v29, v30, v31,
          v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, Size, v51,
          v52, *v53, v54, v55, v57, v58, v59, v60, v61, v62, v63, v64, v65, v66, v67, v68, v69,
          v70, v71, v72, v73, v74, v75, v76, v77, v78, v79, v80, v81, v82, v83, v84, v85, v86,
          v87, v88, v89, v90, v91, v92, v93, v95, v96, n16_1, v99, v100;
  int64_t v19;
  uint16_t *v18;
  uint32_t k;
  uint8_t *v8;
  v8 = a4;
  __sub_414F60();
  v11 = lpAddress[17421].m128_i32[0];
  if ( i > 0 )
  {
    v103 = a4;
    for ( j = 0; j < i; ++j )
    {
      v12 = *(uint16_t *)(v11 - 18) >> 4;
      v13 = (uint8_t)(*v103 - v12);
      v104 = lpAddress[17499].m128_u8[v13];
      v14 = lpAddress[17531].m128_i8[v104] + v12;
      n16 = (uint8_t)*a7 - (uint8_t)(v14 + *a7 - *v103);
      if ( n16 < -16 || n16 > 16 )
      {
        *a7 = *v103;
        v104 = lpAddress[17515].m128_u8[v13];
      }
      else
      {
        *a7 = v14;
      }
      v16 = lpAddress[17421].m128_i32[0];
      v17 = *(int16_t *)(v16 - 18) >> 4;
      lpAddress[17419].m128_i32[1] = lpAddress[17419].m128_i32[0] + lpAddress[17435].m128_i32[v17];
      lpAddress[17419].m128_i32[2] = lpAddress[17419].m128_i32[0] + lpAddress[17434].m128_i32[v17];
      __fwd_sub_419800_sub_414390(
        &lpAddress[58754].m128_u16[4 * lpAddress[17419].m128_i32[0]
                                 + 4
                                 + 4
                                 * lpAddress[17426].m128_i32[(*(int16_t *)(v16 - 18) <= *(int16_t *)(v16 - 36))
                                                           + 3
                                                           + (*(int16_t *)(v16 - 18) < *(int16_t *)(v16 - 36))]
                                 + 4 * lpAddress[17425].m128_i32[*(uint8_t *)(v16 - 20) + 3]
                                 + 4 * lpAddress[17424].m128_i32[*(uint8_t *)(v16 - 2) + 3]
                                 + 4
                                 * lpAddress[17423].m128_i32[((uint32_t)(v17 - 115) >> 31)
                                                           + 3
                                                           + ((uint32_t)(v17 - 17) >> 31)]
                                 + 4 * lpAddress[17428].m128_i32[0]],
        (int32_t)&lpAddress[17419].m128_i32[1],
        v104);
      ++v103;
      LOWORD(v16) = 16 * (uint8_t)*a7;
      *(uint16_t *)lpAddress[17421].m128_i32[0] = v16;
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 2) = v16;
      ++a7;
      v18 = (uint16_t *)lpAddress[17421].m128_i32[0];
      v19 = (int16_t)(*v18 - *(v18 - 9));
      v18[2] = v19;
      LOWORD(v19) = (WORD2(v19) ^ v19) - WORD2(v19);
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 6) = v19;
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 14) = v19;
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 12) = v19;
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 10) = v19;
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 8) = (uint32_t)*(int16_t *)(lpAddress[17421].m128_i32[0] + 10) >> 1;
      *(uint8_t *)(lpAddress[17421].m128_i32[0] + 17) = 2;
      *(uint8_t *)(lpAddress[17421].m128_i32[0] + 16) = (*(int16_t *)(lpAddress[17421].m128_i32[0] + 4) <= 0)
                                                    + (*(int16_t *)(lpAddress[17421].m128_i32[0] + 4) < 0);
      v11 = lpAddress[17421].m128_i32[0] + 18;
      lpAddress[17421].m128_i32[0] = v11;
    }
    v8 = v103;
  }
  v20 = *(uint32_t *)(v11 - 14);
  v21 = *(uint32_t *)(v11 - 10);
  *(uint32_t *)v11 = *(uint32_t *)(v11 - 18);
  v22 = *(uint32_t *)(v11 - 6);
  *(uint32_t *)(v11 + 4) = v20;
  LOWORD(v20) = *(uint16_t *)(v11 - 2);
  *(uint32_t *)(v11 + 8) = v21;
  *(uint32_t *)(v11 + 12) = v22;
  *(uint16_t *)(v11 + 16) = v20;
  v23 = lpAddress[17421].m128_i32[0];
  v24 = *(uint32_t *)(v23 - 10);
  v25 = *(uint32_t *)(v23 - 6);
  *(uint32_t *)(v23 + 18) = *(uint32_t *)(v23 - 18);
  *(uint32_t *)(v23 + 22) = *(uint32_t *)(v23 - 14);
  v26 = *(uint16_t *)(v23 - 2);
  *(uint32_t *)(v23 + 26) = v24;
  *(uint32_t *)(v23 + 30) = v25;
  *(uint16_t *)(v23 + 34) = v26;
  v27 = lpAddress[17421].m128_i32[0];
  v28 = *(uint32_t *)(v27 - 10);
  v29 = *(uint32_t *)(v27 - 6);
  *(uint32_t *)(v27 + 36) = *(uint32_t *)(v27 - 18);
  *(uint32_t *)(v27 + 40) = *(uint32_t *)(v27 - 14);
  LOWORD(v23) = *(uint16_t *)(v27 - 2);
  *(uint32_t *)(v27 + 44) = v28;
  *(uint32_t *)(v27 + 48) = v29;
  *(uint16_t *)(v27 + 52) = v23;
  v30 = lpAddress[17421].m128_i32[0];
  v31 = *(uint32_t *)(v30 - 14);
  v32 = *(uint32_t *)(v30 - 10);
  *(uint32_t *)(v30 + 54) = *(uint32_t *)(v30 - 18);
  v33 = *(uint32_t *)(v30 - 6);
  *(uint32_t *)(v30 + 58) = v31;
  LOWORD(v31) = *(uint16_t *)(v30 - 2);
  *(uint32_t *)(v30 + 62) = v32;
  *(uint32_t *)(v30 + 66) = v33;
  *(uint16_t *)(v30 + 70) = v31;
  v34 = lpAddress[17421].m128_i32[0];
  v35 = *(uint32_t *)(v34 - 14);
  v36 = *(uint32_t *)(v34 - 6);
  *(uint32_t *)(v34 + 72) = *(uint32_t *)(v34 - 18);
  v37 = *(uint32_t *)(v34 - 10);
  *(uint32_t *)(v34 + 76) = v35;
  LOWORD(v35) = *(uint16_t *)(v34 - 2);
  *(uint32_t *)(v34 + 80) = v37;
  *(uint32_t *)(v34 + 84) = v36;
  *(uint16_t *)(v34 + 88) = v35;
  v38 = lpAddress[17421].m128_i32[0];
  v39 = -18 * i;
  *(uint32_t *)(v38 + v39 - 18) = *(uint32_t *)(v38 - 18 * i);
  *(uint32_t *)(v38 + v39 - 14) = *(uint32_t *)(v38 + v39 + 4);
  *(uint32_t *)(v38 + v39 - 10) = *(uint32_t *)(v38 + v39 + 8);
  *(uint32_t *)(v38 + v39 - 6) = *(uint32_t *)(v38 + v39 + 12);
  *(uint16_t *)(v38 + v39 - 2) = *(uint16_t *)(v38 + v39 + 16);
  v40 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v40 + v39 - 36) = *(uint32_t *)(v40 + v39 + 18);
  *(uint32_t *)(v40 + v39 - 32) = *(uint32_t *)(v40 + v39 + 22);
  *(uint32_t *)(v40 + v39 - 28) = *(uint32_t *)(v40 + v39 + 26);
  *(uint32_t *)(v40 + v39 - 24) = *(uint32_t *)(v40 + v39 + 30);
  *(uint16_t *)(v40 + v39 - 20) = *(uint16_t *)(v40 + v39 + 34);
  v41 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v41 + v39 - 54) = *(uint32_t *)(v41 + v39 + 36);
  *(uint32_t *)(v41 + v39 - 50) = *(uint32_t *)(v41 + v39 + 40);
  *(uint32_t *)(v41 + v39 - 46) = *(uint32_t *)(v41 + v39 + 44);
  *(uint32_t *)(v41 + v39 - 42) = *(uint32_t *)(v41 + v39 + 48);
  *(uint16_t *)(v41 + v39 - 38) = *(uint16_t *)(v41 + v39 + 52);
  v42 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v42 + v39 - 72) = *(uint32_t *)(v42 + v39 + 54);
  *(uint32_t *)(v42 + v39 - 68) = *(uint32_t *)(v42 + v39 + 58);
  *(uint32_t *)(v42 + v39 - 64) = *(uint32_t *)(v42 + v39 + 62);
  *(uint32_t *)(v42 + v39 - 60) = *(uint32_t *)(v42 + v39 + 66);
  *(uint16_t *)(v42 + v39 - 56) = *(uint16_t *)(v42 + v39 + 70);
  v43 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v43 + v39 - 90) = *(uint32_t *)(v43 + v39 + 72);
  *(uint32_t *)(v43 + v39 - 86) = *(uint32_t *)(v43 + v39 + 76);
  *(uint32_t *)(v43 + v39 - 82) = *(uint32_t *)(v43 + v39 + 80);
  *(uint32_t *)(v43 + v39 - 78) = *(uint32_t *)(v43 + v39 + 84);
  *(uint16_t *)(v43 + v39 - 74) = *(uint16_t *)(v43 + v39 + 88);
  v44 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v44 + v39 - 108) = *(uint32_t *)(v44 + v39 + 90);
  *(uint32_t *)(v44 + v39 - 104) = *(uint32_t *)(v44 + v39 + 94);
  *(uint32_t *)(v44 + v39 - 100) = *(uint32_t *)(v44 + v39 + 98);
  *(uint32_t *)(v44 + v39 - 96) = *(uint32_t *)(v44 + v39 + 102);
  *(uint16_t *)(v44 + v39 - 92) = *(uint16_t *)(v44 + v39 + 106);
  v45 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v45 + v39 - 126) = *(uint32_t *)(v45 + v39 + 108);
  *(uint32_t *)(v45 + v39 - 122) = *(uint32_t *)(v45 + v39 + 112);
  *(uint32_t *)(v45 + v39 - 118) = *(uint32_t *)(v45 + v39 + 116);
  *(uint32_t *)(v45 + v39 - 114) = *(uint32_t *)(v45 + v39 + 120);
  *(uint16_t *)(v45 + v39 - 110) = *(uint16_t *)(v45 + v39 + 124);
  v46 = lpAddress[17421].m128_i32[0];
  *(uint32_t *)(v46 + v39 - 144) = *(uint32_t *)(v46 + v39 + 126);
  *(uint32_t *)(v46 + v39 - 140) = *(uint32_t *)(v46 + v39 + 130);
  *(uint32_t *)(v46 + v39 - 136) = *(uint32_t *)(v46 + v39 + 134);
  *(uint32_t *)(v46 + v39 - 132) = *(uint32_t *)(v46 + v39 + 138);
  *(uint16_t *)(v46 + v39 - 128) = *(uint16_t *)(v46 + v39 + 142);
  Size = 18 * i + 234;
  memcpy((char *)lpAddress[17422].m128_i32[2],(char *)lpAddress[17422].m128_i32[1],Size);
  memcpy((char *)lpAddress[17422].m128_i32[3],(char *)lpAddress[17422].m128_i32[1],Size);
  memcpy((char *)lpAddress[17423].m128_i32[0],(char *)lpAddress[17422].m128_i32[1],Size);
  if ( a6 > 1 )
  {
    v106 = &lpAddress[17419].m128_i8[4];
    v105 = a7;
    v50 = 0;
    v102 = 0;
    do
    {
      v51 = lpAddress[17416].m128_i32[3];
      v52 = *(uint32_t *)(v51 - 4);
      v109 = v8;
      *(uint32_t *)(v51 + 4) = v52;
      *(uint32_t *)lpAddress[17416].m128_i32[3] = v52;
      v53 = (int32_t *)lpAddress[17416].m128_i32[1];
      v54 = lpAddress[17416].m128_i32[2];
      lpAddress[17416].m128_i32[1] = v54;
      lpAddress[17416].m128_i32[2] = (int32_t)v53;
      v54 += 8;
      lpAddress[17416].m128_i32[3] = v54;
      v53 += 2;
      lpAddress[17417].m128_i32[0] = (int32_t)v53;
      v55 = *v53;
      *(uint32_t *)(v54 - 4) = *v53;
      *(uint32_t *)(lpAddress[17416].m128_i32[3] - 8) = v55;
      lpAddress[17408].m128_u64[0] = v50.m128_u64[0];
      lpAddress[17408].m128_i32[2] = 0;
      lpAddress[17408].m128_i16[6] = 0;
      lpAddress[17408].m128_i8[14] = 0;
      lpAddress[17415].m128_u64[0] = v50.m128_u64[0];
      lpAddress[17415].m128_u64[1] = v50.m128_u64[0];
      v56 = (__m128i *)(((uint32_t)&lpAddress[17408].m128_u32[3] + 3) & 0xFFFFFFF0);
      *v56 = v50;
      v56[1] = v50;
      v56[2] = v50;
      v56[3] = v50;
      v56[4] = v50;
      v56[5] = v50;
      v56[6] = v50;
      v57 = lpAddress[17421].m128_i32[0];
      v58 = *(uint32_t *)(v57 - 14);
      v59 = *(uint32_t *)(v57 - 10);
      v60 = *(uint32_t *)(v57 - 6);
      *(uint32_t *)v57 = *(uint32_t *)(v57 - 18);
      LOWORD(v53) = *(uint16_t *)(v57 - 2);
      *(uint32_t *)(v57 + 4) = v58;
      *(uint32_t *)(v57 + 8) = v59;
      *(uint32_t *)(v57 + 12) = v60;
      *(uint16_t *)(v57 + 16) = (uint16_t)v53;
      v61 = lpAddress[17421].m128_i32[0];
      v62 = *(uint32_t *)(v61 - 32);
      v63 = *(uint32_t *)(v61 - 28);
      LOWORD(v53) = *(uint16_t *)(v61 - 20);
      *(uint32_t *)(v61 + 18) = *(uint32_t *)(v61 - 36);
      v64 = *(uint32_t *)(v61 - 24);
      *(uint32_t *)(v61 + 22) = v62;
      *(uint32_t *)(v61 + 26) = v63;
      *(uint32_t *)(v61 + 30) = v64;
      *(uint16_t *)(v61 + 34) = (uint16_t)v53;
      v65 = lpAddress[17421].m128_i32[0];
      v66 = *(uint32_t *)(v65 - 50);
      v67 = *(uint32_t *)(v65 - 42);
      LOWORD(v53) = *(uint16_t *)(v65 - 38);
      *(uint32_t *)(v65 + 36) = *(uint32_t *)(v65 - 54);
      v68 = *(uint32_t *)(v65 - 46);
      *(uint32_t *)(v65 + 40) = v66;
      *(uint32_t *)(v65 + 44) = v68;
      *(uint32_t *)(v65 + 48) = v67;
      *(uint16_t *)(v65 + 52) = (uint16_t)v53;
      v69 = lpAddress[17421].m128_i32[0];
      v70 = *(uint32_t *)(v69 - 64);
      v71 = *(uint32_t *)(v69 - 60);
      LOWORD(v53) = *(uint16_t *)(v69 - 56);
      *(uint32_t *)(v69 + 54) = *(uint32_t *)(v69 - 72);
      *(uint32_t *)(v69 + 58) = *(uint32_t *)(v69 - 68);
      *(uint32_t *)(v69 + 62) = v70;
      *(uint32_t *)(v69 + 66) = v71;
      *(uint16_t *)(v69 + 70) = (uint16_t)v53;
      v72 = lpAddress[17421].m128_i32[0];
      v73 = *(uint32_t *)(v72 - 82);
      v74 = *(uint32_t *)(v72 - 78);
      LOWORD(v71) = *(uint16_t *)(v72 - 74);
      *(uint32_t *)(v72 + 72) = *(uint32_t *)(v72 - 90);
      *(uint32_t *)(v72 + 76) = *(uint32_t *)(v72 - 86);
      *(uint32_t *)(v72 + 80) = v73;
      *(uint32_t *)(v72 + 84) = v74;
      *(uint16_t *)(v72 + 88) = v71;
      v75 = lpAddress[17423].m128_i32[0];
      v76 = lpAddress[17423].m128_i32[1];
      v77 = lpAddress[17422].m128_i32[3];
      v78 = lpAddress[17422].m128_i32[2];
      v79 = lpAddress[17422].m128_i32[1];
      lpAddress[17423].m128_i32[1] = v75;
      lpAddress[17423].m128_i32[0] = v77;
      lpAddress[17422].m128_i32[3] = v78;
      lpAddress[17422].m128_i32[1] = v76;
      lpAddress[17422].m128_i32[2] = v79;
      v76 += 144;
      lpAddress[17421].m128_i32[0] = v76;
      v79 += 144;
      lpAddress[17421].m128_i32[1] = v79;
      lpAddress[17421].m128_i32[2] = v78 + 144;
      lpAddress[17421].m128_i32[3] = v77 + 144;
      lpAddress[17422].m128_i32[0] = v75 + 144;
      *(uint32_t *)(v76 - 18) = *(uint32_t *)v79;
      *(uint32_t *)(v76 - 14) = *(uint32_t *)(v79 + 4);
      *(uint32_t *)(v76 - 10) = *(uint32_t *)(v79 + 8);
      *(uint32_t *)(v76 - 6) = *(uint32_t *)(v79 + 12);
      *(uint16_t *)(v76 - 2) = *(uint16_t *)(v79 + 16);
      v80 = lpAddress[17421].m128_i32[0];
      v81 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v80 - 36) = *(uint32_t *)(v81 + 18);
      *(uint32_t *)(v80 - 32) = *(uint32_t *)(v81 + 22);
      *(uint32_t *)(v80 - 28) = *(uint32_t *)(v81 + 26);
      *(uint32_t *)(v80 - 24) = *(uint32_t *)(v81 + 30);
      *(uint16_t *)(v80 - 20) = *(uint16_t *)(v81 + 34);
      v82 = lpAddress[17421].m128_i32[0];
      v83 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v82 - 54) = *(uint32_t *)(v83 + 36);
      *(uint32_t *)(v82 - 50) = *(uint32_t *)(v83 + 40);
      *(uint32_t *)(v82 - 46) = *(uint32_t *)(v83 + 44);
      *(uint32_t *)(v82 - 42) = *(uint32_t *)(v83 + 48);
      *(uint16_t *)(v82 - 38) = *(uint16_t *)(v83 + 52);
      v84 = lpAddress[17421].m128_i32[0];
      v85 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v84 - 72) = *(uint32_t *)(v85 + 54);
      *(uint32_t *)(v84 - 68) = *(uint32_t *)(v85 + 58);
      *(uint32_t *)(v84 - 64) = *(uint32_t *)(v85 + 62);
      *(uint32_t *)(v84 - 60) = *(uint32_t *)(v85 + 66);
      *(uint16_t *)(v84 - 56) = *(uint16_t *)(v85 + 70);
      v86 = lpAddress[17421].m128_i32[0];
      v87 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v86 - 90) = *(uint32_t *)(v87 + 72);
      *(uint32_t *)(v86 - 86) = *(uint32_t *)(v87 + 76);
      *(uint32_t *)(v86 - 82) = *(uint32_t *)(v87 + 80);
      *(uint32_t *)(v86 - 78) = *(uint32_t *)(v87 + 84);
      *(uint16_t *)(v86 - 74) = *(uint16_t *)(v87 + 88);
      v88 = lpAddress[17421].m128_i32[0];
      v89 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v88 - 108) = *(uint32_t *)(v89 + 90);
      *(uint32_t *)(v88 - 104) = *(uint32_t *)(v89 + 94);
      *(uint32_t *)(v88 - 100) = *(uint32_t *)(v89 + 98);
      *(uint32_t *)(v88 - 96) = *(uint32_t *)(v89 + 102);
      *(uint16_t *)(v88 - 92) = *(uint16_t *)(v89 + 106);
      v90 = lpAddress[17421].m128_i32[0];
      v91 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v90 - 126) = *(uint32_t *)(v91 + 108);
      *(uint32_t *)(v90 - 122) = *(uint32_t *)(v91 + 112);
      *(uint32_t *)(v90 - 118) = *(uint32_t *)(v91 + 116);
      *(uint32_t *)(v90 - 114) = *(uint32_t *)(v91 + 120);
      *(uint16_t *)(v90 - 110) = *(uint16_t *)(v91 + 124);
      v92 = lpAddress[17421].m128_i32[0];
      v93 = lpAddress[17421].m128_i32[1];
      *(uint32_t *)(v92 - 144) = *(uint32_t *)(v93 + 126);
      *(uint32_t *)(v92 - 140) = *(uint32_t *)(v93 + 130);
      *(uint32_t *)(v92 - 136) = *(uint32_t *)(v93 + 134);
      *(uint32_t *)(v92 - 132) = *(uint32_t *)(v93 + 138);
      *(uint16_t *)(v92 - 128) = *(uint16_t *)(v93 + 142);
      *(uint16_t *)(lpAddress[17421].m128_i32[0] + 2) = 0;
      if ( i > 0 )
      {
        lpAddress_1 = lpAddress;
        for ( k = 0; k < i; ++k )
        {
          v95 = __fwd_sub_419800_sub_41A130(lpAddress_1, v50, a2, nullptr, nullptr);
          v107 = v109[k];
          v96 = (uint8_t)(v107 - v95);
          v97 = v95 + lpAddress_1[17531].m128_i8[lpAddress_1[17499].m128_u8[v96]];
          n16_1 = (uint8_t)v105[k] - (uint8_t)(v97 + v105[k] - v107);
          v99 = lpAddress_1[17499].m128_u8[v96];
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v105[k] = v107;
            v99 = lpAddress_1[17515].m128_u8[(uint8_t)(v107 - v95)];
          }
          else
          {
            v105[k] = v97;
          }
          v110 = v99;
          __fwd_sub_419800_sub_414390(&lpAddress_1[58754].m128_u16[4 * lpAddress_1[17419].m128_i32[0] + 4], (int32_t)v106, v99);
          __sub_41CAB0((int32_t)lpAddress_1, a3, (uint8_t)v105[k], v110, (uint8_t)v105[k] - v95);
          v8 = &v109[k + 1];
          v100 = (int32_t)&v105[k + 1];
        }
        lpAddress = lpAddress_1;
        v105 = (uint8_t *)v100;
        v50 = 0;
      }
      ++v102;
    }
    while ( v102 < a6 - 1 );
  }
  __sub_414CE0();
}
static inline void ** __fwd_sub_4197A0_sub_419610(void *a0, char a1) { return __sub_419610((void **)a0, a1); }
static inline void __fwd_sub_4197A0_sub_419800(void *a0, const __m128 &a1, const __m128 &a2, void *a3, int32_t a4, int32_t a5, void *a6) { __sub_419800((__m128 *)a0, a1, a2, (uint8_t *)a3, a4, a5, (uint8_t *)a6); }

BMF_SSE void __sub_4197A0(const __m128 &a1__ref, const __m128 &a2__ref, uint8_t *a3, int32_t i, int32_t a5, uint8_t *a6)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  void * v6;
  __m128 *lpAddress;
  v6 = bmf_page_alloc(0x103E30u);
  if ( v6 )
    lpAddress = (__m128 *)__sub_4229E0((int32_t)v6, i, 0);
  else
    lpAddress = nullptr;
  __fwd_sub_4197A0_sub_419800(lpAddress, a1, a2, a3, i, a5, a6);
  if ( lpAddress )
    __fwd_sub_4197A0_sub_419610((void **)lpAddress, 1);
}
static inline int32_t __fwd_sub_421930_sub_414390(void *a0, int32_t a1, int32_t a2) { return __sub_414390((uint16_t *)a0, a1, a2); }
static inline void ** __fwd_sub_421930_sub_419610(void *a0, char a1) { return __sub_419610((void **)a0, a1); }
static inline int32_t __fwd_sub_421930_sub_41A130(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __sub_41A130((__m128 *)a0, a1, a2, (uint32_t *)a3, (uint32_t *)a4); }

 BMF_SSE int32_t __sub_421930(uint16_t *p_i, uint8_t *a2)
{
  alignas(16) uint8_t __hexrays_frame[324];
  uint32_t &Size_1 = *(uint32_t *)(__hexrays_frame + 0);
  __m128i &v144 = *(__m128i *)(__hexrays_frame + 4);
  __m128i &v145 = *(__m128i *)(__hexrays_frame + 20);
  __m128i &v146 = *(__m128i *)(__hexrays_frame + 36);
  __m128i &v147 = *(__m128i *)(__hexrays_frame + 52);
  __m128i &v148 = *(__m128i *)(__hexrays_frame + 68);
  __m128i &v149 = *(__m128i *)(__hexrays_frame + 84);
  __m128i &v150 = *(__m128i *)(__hexrays_frame + 100);
  __m128i &v151 = *(__m128i *)(__hexrays_frame + 116);
  __m128i &v152 = *(__m128i *)(__hexrays_frame + 132);
  int32_t &v153 = *(int32_t *)(__hexrays_frame + 148);
  uint32_t &v154 = *(uint32_t *)(__hexrays_frame + 156);
  int32_t &v155 = *(int32_t *)(__hexrays_frame + 160);
  int32_t &n3 = *(int32_t *)(__hexrays_frame + 164);
  int32_t &v157 = *(int32_t *)(__hexrays_frame + 168);
  int32_t &v158 = *(int32_t *)(__hexrays_frame + 172);
  void * &lpAddress = *(void * *)(__hexrays_frame + 176);
  __m128 * &v160 = *(__m128 * *)(__hexrays_frame + 180);
  __m128 * &v161 = *(__m128 * *)(__hexrays_frame + 184);
  __m128 * &v162 = *(__m128 * *)(__hexrays_frame + 188);
  int32_t &v163 = *(int32_t *)(__hexrays_frame + 192);
  int32_t &v164 = *(int32_t *)(__hexrays_frame + 196);
  uint32_t &Size = *(uint32_t *)(__hexrays_frame + 200);
  uint8_t * &v166 = *(uint8_t * *)(__hexrays_frame + 204);
  int32_t &i_2 = *(int32_t *)(__hexrays_frame + 208);
  int32_t &v168 = *(int32_t *)(__hexrays_frame + 212);
  int32_t &v169 = *(int32_t *)(__hexrays_frame + 216);
  uint32_t &v170 = *(uint32_t *)(__hexrays_frame + 220);
  int32_t &n4_2 = *(int32_t *)(__hexrays_frame + 224);
  uint32_t &i = *(uint32_t *)(__hexrays_frame + 228);
  int32_t &v173 = *(int32_t *)(__hexrays_frame + 232);
  int32_t &v174 = *(int32_t *)(__hexrays_frame + 236);
  int32_t &v175 = *(int32_t *)(__hexrays_frame + 240);
  int32_t &v176 = *(int32_t *)(__hexrays_frame + 244);
  int32_t &v177 = *(int32_t *)(__hexrays_frame + 248);
  int32_t &v178 = *(int32_t *)(__hexrays_frame + 252);
  int32_t &v179 = *(int32_t *)(__hexrays_frame + 256);
  int32_t &v180 = *(int32_t *)(__hexrays_frame + 260);
  int32_t &v181 = *(int32_t *)(__hexrays_frame + 264);
  int32_t &v182 = *(int32_t *)(__hexrays_frame + 268);
  int32_t &v183 = *(int32_t *)(__hexrays_frame + 272);
  int32_t &v184 = *(int32_t *)(__hexrays_frame + 276);
  uint32_t &v185 = *(uint32_t *)(__hexrays_frame + 280);
  ;
  __m128 v2, v3, *v114, *v125, *v133;
  __m128i *v61;
  bool v16;
  char v9, v10, v18, v19, v53, v54;
  int16_t v113;
  int32_t i_1, v5, n4, n4_1, v13, v14, v15, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29,
          v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46,
          v47, v48, v49, v50, v51, v52, v55, v56, v57, *v58, v59, v60, v62, v63, v64, v65, v66,
          v67, v68, v69, v70, v71, v72, v73, v74, v75, v76, v77, v78, v79, v80, v81, v82, v83,
          v84, v85, v86, v87, v88, v89, v90, v91, v92, v93, v94, v95, v96, v97, v98, v99, v100,
          v101, v102, v104, v105, v106, v107, n16, v109, v110, v111, v112, v115, v116, v117,
          v118, n16_1, v120, v121, v122, v123, v124, v126, v127, v128, v129, n16_2, v131, v132,
          v134, v135, v136, v137, n16_3, v139, n4_3, n4_4;
  uint32_t v11;
  uint8_t *lpAddress_1;
  void *v7, *v8, **lpAddress_2;
  void * *v17;
  v166 = a2;
  v144 = __xmmword_441120;
  v145 = __xmmword_441130;
  v146 = __xmmword_441140;
  v147 = __xmmword_441160;
  v148 = __xmmword_441170;
  v149 = __xmmword_441180;
  v150 = __xmmword_4411D0;
  v151 = __xmmword_4411E0;
  v152 = __xmmword_4411F0;
  v2 = _mm_add_ps((__m128)__xmmword_441120, (__m128)__xmmword_441160);
  v3 = _mm_add_ps((__m128)__xmmword_441130, (__m128)__xmmword_441170);
  __xmmword_441120 = (__m128i)v2;
  __xmmword_441130 = (__m128i)v3;
  __xmmword_441140 = (__m128i)_mm_add_ps((__m128)__xmmword_441140, (__m128)__xmmword_441180);
  __xmmword_4411F0 = __xmmword_439B50;
  __xmmword_4411E0 = __xmmword_439B50;
  __xmmword_4411D0 = __xmmword_439B50;
  __xmmword_441160 = 0;
  __xmmword_441170 = 0;
  __xmmword_441180 = 0;
  i_1 = *p_i;
  v5 = p_i[1];
  if ( __n4_5 > 0 )
  {
    n4 = 0;
    do
    {
      v7 = bmf_page_alloc(0x103E30u);
      if ( v7 )
        v8 = (void *)__sub_4229E0((int32_t)v7, i_1, n4);
      else
        v8 = nullptr;
      *(&lpAddress + n4++) = v8;
    }
    while ( n4 < __n4_5 );
  }
  v169 = __byte_44339E[16 * (uint8_t)__byte_4433AD[0]] & 8;
  v9 = __byte_44339E[16 * (uint8_t)__n3_0];
  v168 = __byte_44339E[16 * (uint8_t)__byte_4433BD[0]] & 8;
  v155 = v9 & 8;
  __sub_414F60();
  if ( v5 > 0 )
  {
    v154 = v5;
    v11 = 0;
    i_2 = i_1;
    v153 = 9 * i_1;
    v164 = -18 * i_1;
    Size = 18 * i_1 + 234;
    v185 = i_1 + 13;
    n4_1 = __n4_5;
    do
    {
      if ( n4_1 > 0 )
      {
        v170 = v11;
        n4_2 = 0;
        do
        {
          ++n4_2;
          if ( v170 )
          {
            v17 = &lpAddress + n4_2;
            if ( v170 == 1 )
            {
              v24 = (int32_t)*(v17 - 1);
              v25 = *(uint32_t *)(v24 + 278736);
              v26 = *(uint32_t *)(v25 - 14);
              v27 = *(uint32_t *)(v25 - 10);
              *(uint32_t *)v25 = *(uint32_t *)(v25 - 18);
              v28 = *(uint32_t *)(v25 - 6);
              *(uint32_t *)(v25 + 4) = v26;
              LOWORD(v26) = *(uint16_t *)(v25 - 2);
              *(uint32_t *)(v25 + 8) = v27;
              *(uint32_t *)(v25 + 12) = v28;
              *(uint16_t *)(v25 + 16) = v26;
              v29 = *(uint32_t *)(v24 + 278736);
              v30 = *(uint32_t *)(v29 - 14);
              v31 = *(uint32_t *)(v29 - 10);
              *(uint32_t *)(v29 + 18) = *(uint32_t *)(v29 - 18);
              v32 = *(uint32_t *)(v29 - 6);
              *(uint32_t *)(v29 + 22) = v30;
              LOWORD(v30) = *(uint16_t *)(v29 - 2);
              *(uint32_t *)(v29 + 26) = v31;
              *(uint32_t *)(v29 + 30) = v32;
              *(uint16_t *)(v29 + 34) = v30;
              v33 = *(uint32_t *)(v24 + 278736);
              v34 = *(uint32_t *)(v33 - 14);
              v35 = *(uint32_t *)(v33 - 6);
              *(uint32_t *)(v33 + 36) = *(uint32_t *)(v33 - 18);
              v36 = *(uint32_t *)(v33 - 10);
              *(uint32_t *)(v33 + 40) = v34;
              LOWORD(v34) = *(uint16_t *)(v33 - 2);
              *(uint32_t *)(v33 + 44) = v36;
              *(uint32_t *)(v33 + 48) = v35;
              *(uint16_t *)(v33 + 52) = v34;
              v37 = *(uint32_t *)(v24 + 278736);
              v38 = *(uint32_t *)(v37 - 10);
              v39 = *(uint32_t *)(v37 - 6);
              *(uint32_t *)(v37 + 54) = *(uint32_t *)(v37 - 18);
              *(uint32_t *)(v37 + 58) = *(uint32_t *)(v37 - 14);
              LOWORD(v33) = *(uint16_t *)(v37 - 2);
              *(uint32_t *)(v37 + 62) = v38;
              *(uint32_t *)(v37 + 66) = v39;
              *(uint16_t *)(v37 + 70) = v33;
              v40 = *(uint32_t *)(v24 + 278736);
              v41 = *(uint32_t *)(v40 - 14);
              v42 = *(uint32_t *)(v40 - 10);
              *(uint32_t *)(v40 + 72) = *(uint32_t *)(v40 - 18);
              v43 = *(uint32_t *)(v40 - 6);
              *(uint32_t *)(v40 + 76) = v41;
              LOWORD(v41) = *(uint16_t *)(v40 - 2);
              *(uint32_t *)(v40 + 80) = v42;
              *(uint32_t *)(v40 + 84) = v43;
              *(uint16_t *)(v40 + 88) = v41;
              v44 = *(uint32_t *)(v24 + 278736);
              v45 = v164;
              Size_1 = Size;
              *(uint32_t *)(v44 + v164 - 18) = *(uint32_t *)(v164 + v44);
              *(uint32_t *)(v44 + v45 - 14) = *(uint32_t *)(v45 + v44 + 4);
              *(uint32_t *)(v44 + v45 - 10) = *(uint32_t *)(v45 + v44 + 8);
              *(uint32_t *)(v44 + v45 - 6) = *(uint32_t *)(v45 + v44 + 12);
              *(uint16_t *)(v44 + v45 - 2) = *(uint16_t *)(v45 + v44 + 16);
              v46 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v46 - 36) = *(uint32_t *)(v45 + v46 + 18);
              *(uint32_t *)(v45 + v46 - 32) = *(uint32_t *)(v45 + v46 + 22);
              *(uint32_t *)(v45 + v46 - 28) = *(uint32_t *)(v45 + v46 + 26);
              *(uint32_t *)(v45 + v46 - 24) = *(uint32_t *)(v45 + v46 + 30);
              *(uint16_t *)(v45 + v46 - 20) = *(uint16_t *)(v45 + v46 + 34);
              v47 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v47 - 54) = *(uint32_t *)(v45 + v47 + 36);
              *(uint32_t *)(v45 + v47 - 50) = *(uint32_t *)(v45 + v47 + 40);
              *(uint32_t *)(v45 + v47 - 46) = *(uint32_t *)(v45 + v47 + 44);
              *(uint32_t *)(v45 + v47 - 42) = *(uint32_t *)(v45 + v47 + 48);
              *(uint16_t *)(v45 + v47 - 38) = *(uint16_t *)(v45 + v47 + 52);
              v48 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v48 - 72) = *(uint32_t *)(v45 + v48 + 54);
              *(uint32_t *)(v45 + v48 - 68) = *(uint32_t *)(v45 + v48 + 58);
              *(uint32_t *)(v45 + v48 - 64) = *(uint32_t *)(v45 + v48 + 62);
              *(uint32_t *)(v45 + v48 - 60) = *(uint32_t *)(v45 + v48 + 66);
              *(uint16_t *)(v45 + v48 - 56) = *(uint16_t *)(v45 + v48 + 70);
              v49 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v49 - 90) = *(uint32_t *)(v45 + v49 + 72);
              *(uint32_t *)(v45 + v49 - 86) = *(uint32_t *)(v45 + v49 + 76);
              *(uint32_t *)(v45 + v49 - 82) = *(uint32_t *)(v45 + v49 + 80);
              *(uint32_t *)(v45 + v49 - 78) = *(uint32_t *)(v45 + v49 + 84);
              *(uint16_t *)(v45 + v49 - 74) = *(uint16_t *)(v45 + v49 + 88);
              v50 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v50 - 108) = *(uint32_t *)(v45 + v50 + 90);
              *(uint32_t *)(v45 + v50 - 104) = *(uint32_t *)(v45 + v50 + 94);
              *(uint32_t *)(v45 + v50 - 100) = *(uint32_t *)(v45 + v50 + 98);
              *(uint32_t *)(v45 + v50 - 96) = *(uint32_t *)(v45 + v50 + 102);
              *(uint16_t *)(v45 + v50 - 92) = *(uint16_t *)(v45 + v50 + 106);
              v51 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v51 - 126) = *(uint32_t *)(v45 + v51 + 108);
              *(uint32_t *)(v45 + v51 - 122) = *(uint32_t *)(v45 + v51 + 112);
              *(uint32_t *)(v45 + v51 - 118) = *(uint32_t *)(v45 + v51 + 116);
              *(uint32_t *)(v45 + v51 - 114) = *(uint32_t *)(v45 + v51 + 120);
              *(uint16_t *)(v45 + v51 - 110) = *(uint16_t *)(v45 + v51 + 124);
              v52 = *(uint32_t *)(v24 + 278736);
              *(uint32_t *)(v45 + v52 - 144) = *(uint32_t *)(v45 + v52 + 126);
              *(uint32_t *)(v45 + v52 - 140) = *(uint32_t *)(v45 + v52 + 130);
              *(uint32_t *)(v45 + v52 - 136) = *(uint32_t *)(v45 + v52 + 134);
              *(uint32_t *)(v45 + v52 - 132) = *(uint32_t *)(v45 + v52 + 138);
              *(uint16_t *)(v45 + v52 - 128) = *(uint16_t *)(v45 + v52 + 142);
              memcpy(*(char **)(v24 + 278760),*(char **)(v24 + 278756),Size_1);
              memcpy(*(char **)(v24 + 278764),*(char **)(v24 + 278756),Size);
              memcpy(*(char **)(v24 + 278768),*(char **)(v24 + 278756),Size);
            }
          }
          else
          {
            v13 = (int32_t)*(&lpAddress + n4_2 - 1);
            v163 = (int32_t)(&lpAddress + n4_2);
            v14 = 0;
            do
            {
              v15 = 9 * v14;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15) = 256;
              v16 = ++v14 < v185;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 2) = 256;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 4) = -16;
              *(uint8_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 16) = 1;
              *(uint8_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 17) = 3;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 6) = 512;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 14) = 512;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 12) = 512;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 10) = 1024;
              *(uint16_t *)(*(uint32_t *)(v13 + 278756) + 2 * v15 + 8) = 256;
            }
            while ( v16 );
            v17 = (void * *)v163;
            memcpy(*(char **)(v13 + 278760),*(char **)(v13 + 278756),Size);
            memcpy(*(char **)(v13 + 278764),*(char **)(v13 + 278756),Size);
            memcpy(*(char **)(v13 + 278768),*(char **)(v13 + 278756),Size);
            v20 = *(uint32_t *)(v13 + 278760) + 2 * v153 + 144;
            *(uint32_t *)(v13 + 278736) = *(uint32_t *)(v13 + 278756) + 2 * v153 + 144;
            v21 = *(uint32_t *)(v13 + 278764);
            *(uint32_t *)(v13 + 278740) = v20;
            v22 = *(uint32_t *)(v13 + 278768);
            *(uint32_t *)(v13 + 278744) = v21 + 2 * v153 + 144;
            v23 = *(uint32_t *)(v13 + 278772) + 2 * v153 + 144;
            *(uint32_t *)(v13 + 278748) = v22 + 2 * v153 + 144;
            *(uint32_t *)(v13 + 278752) = v23;
          }
          v55 = (int32_t)*(v17 - 1);
          v56 = *(uint32_t *)(v55 + 278668);
          v57 = *(uint32_t *)(v56 - 4);
          *(uint32_t *)(v56 + 4) = v57;
          **(uint32_t **)(v55 + 278668) = v57;
          v58 = *(int32_t **)(v55 + 278660);
          v59 = *(uint32_t *)(v55 + 278664);
          *(uint32_t *)(v55 + 278660) = v59;
          *(uint32_t *)(v55 + 278664) = v58;
          v59 += 8;
          *(uint32_t *)(v55 + 278668) = v59;
          v58 += 2;
          v2 = 0;
          *(uint32_t *)(v55 + 278672) = v58;
          v60 = *v58;
          *(uint32_t *)(v59 - 4) = *v58;
          v61 = (__m128i *)((v55 + 278543) & 0xFFFFFFF0);
          *(uint32_t *)(*(uint32_t *)(v55 + 278668) - 8) = v60;
          *(uint64_t *)(v55 + 278528) = 0;
          *(uint32_t *)(v55 + 278536) = 0;
          *(uint16_t *)(v55 + 278540) = 0;
          *(uint8_t *)(v55 + 278542) = 0;
          *(uint64_t *)(v55 + 278640) = 0;
          *(uint64_t *)(v55 + 278648) = 0;
          *v61 = 0;
          v61[1] = 0;
          v61[2] = 0;
          v61[3] = 0;
          v61[4] = 0;
          v61[5] = 0;
          v61[6] = 0;
          v62 = *(uint32_t *)(v55 + 278736);
          v63 = *(uint32_t *)(v62 - 14);
          v64 = *(uint32_t *)(v62 - 10);
          v65 = *(uint32_t *)(v62 - 6);
          *(uint32_t *)v62 = *(uint32_t *)(v62 - 18);
          LOWORD(v59) = *(uint16_t *)(v62 - 2);
          *(uint32_t *)(v62 + 4) = v63;
          *(uint32_t *)(v62 + 8) = v64;
          *(uint32_t *)(v62 + 12) = v65;
          *(uint16_t *)(v62 + 16) = v59;
          v66 = *(uint32_t *)(v55 + 278736);
          v67 = *(uint32_t *)(v66 - 32);
          v68 = *(uint32_t *)(v66 - 28);
          LOWORD(v59) = *(uint16_t *)(v66 - 20);
          *(uint32_t *)(v66 + 18) = *(uint32_t *)(v66 - 36);
          v69 = *(uint32_t *)(v66 - 24);
          *(uint32_t *)(v66 + 22) = v67;
          *(uint32_t *)(v66 + 26) = v68;
          *(uint32_t *)(v66 + 30) = v69;
          *(uint16_t *)(v66 + 34) = v59;
          v70 = *(uint32_t *)(v55 + 278736);
          v71 = *(uint32_t *)(v70 - 50);
          v72 = *(uint32_t *)(v70 - 42);
          LOWORD(v59) = *(uint16_t *)(v70 - 38);
          *(uint32_t *)(v70 + 36) = *(uint32_t *)(v70 - 54);
          v73 = *(uint32_t *)(v70 - 46);
          *(uint32_t *)(v70 + 40) = v71;
          *(uint32_t *)(v70 + 44) = v73;
          *(uint32_t *)(v70 + 48) = v72;
          *(uint16_t *)(v70 + 52) = v59;
          v74 = *(uint32_t *)(v55 + 278736);
          v75 = *(uint32_t *)(v74 - 64);
          v76 = *(uint32_t *)(v74 - 60);
          LOWORD(v59) = *(uint16_t *)(v74 - 56);
          *(uint32_t *)(v74 + 54) = *(uint32_t *)(v74 - 72);
          *(uint32_t *)(v74 + 58) = *(uint32_t *)(v74 - 68);
          *(uint32_t *)(v74 + 62) = v75;
          *(uint32_t *)(v74 + 66) = v76;
          *(uint16_t *)(v74 + 70) = v59;
          v77 = *(uint32_t *)(v55 + 278736);
          v78 = *(uint32_t *)(v77 - 82);
          v79 = *(uint32_t *)(v77 - 78);
          LOWORD(v76) = *(uint16_t *)(v77 - 74);
          *(uint32_t *)(v77 + 72) = *(uint32_t *)(v77 - 90);
          *(uint32_t *)(v77 + 76) = *(uint32_t *)(v77 - 86);
          *(uint32_t *)(v77 + 80) = v78;
          *(uint32_t *)(v77 + 84) = v79;
          *(uint16_t *)(v77 + 88) = v76;
          v80 = *(uint32_t *)(v55 + 278768);
          v81 = *(uint32_t *)(v55 + 278772);
          v82 = *(uint32_t *)(v55 + 278764);
          v83 = *(uint32_t *)(v55 + 278760);
          v84 = *(uint32_t *)(v55 + 278756);
          *(uint32_t *)(v55 + 278772) = v80;
          *(uint32_t *)(v55 + 278768) = v82;
          *(uint32_t *)(v55 + 278764) = v83;
          *(uint32_t *)(v55 + 278756) = v81;
          *(uint32_t *)(v55 + 278760) = v84;
          v81 += 144;
          *(uint32_t *)(v55 + 278736) = v81;
          v84 += 144;
          *(uint32_t *)(v55 + 278740) = v84;
          *(uint32_t *)(v55 + 278744) = v83 + 144;
          *(uint32_t *)(v55 + 278748) = v82 + 144;
          *(uint32_t *)(v55 + 278752) = v80 + 144;
          *(uint32_t *)(v81 - 18) = *(uint32_t *)v84;
          *(uint32_t *)(v81 - 14) = *(uint32_t *)(v84 + 4);
          *(uint32_t *)(v81 - 10) = *(uint32_t *)(v84 + 8);
          *(uint32_t *)(v81 - 6) = *(uint32_t *)(v84 + 12);
          *(uint16_t *)(v81 - 2) = *(uint16_t *)(v84 + 16);
          v85 = *(uint32_t *)(v55 + 278736);
          v86 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v85 - 36) = *(uint32_t *)(v86 + 18);
          *(uint32_t *)(v85 - 32) = *(uint32_t *)(v86 + 22);
          *(uint32_t *)(v85 - 28) = *(uint32_t *)(v86 + 26);
          *(uint32_t *)(v85 - 24) = *(uint32_t *)(v86 + 30);
          *(uint16_t *)(v85 - 20) = *(uint16_t *)(v86 + 34);
          v87 = *(uint32_t *)(v55 + 278736);
          v88 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v87 - 54) = *(uint32_t *)(v88 + 36);
          *(uint32_t *)(v87 - 50) = *(uint32_t *)(v88 + 40);
          *(uint32_t *)(v87 - 46) = *(uint32_t *)(v88 + 44);
          *(uint32_t *)(v87 - 42) = *(uint32_t *)(v88 + 48);
          *(uint16_t *)(v87 - 38) = *(uint16_t *)(v88 + 52);
          v89 = *(uint32_t *)(v55 + 278736);
          v90 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v89 - 72) = *(uint32_t *)(v90 + 54);
          *(uint32_t *)(v89 - 68) = *(uint32_t *)(v90 + 58);
          *(uint32_t *)(v89 - 64) = *(uint32_t *)(v90 + 62);
          *(uint32_t *)(v89 - 60) = *(uint32_t *)(v90 + 66);
          *(uint16_t *)(v89 - 56) = *(uint16_t *)(v90 + 70);
          v91 = *(uint32_t *)(v55 + 278736);
          v92 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v91 - 90) = *(uint32_t *)(v92 + 72);
          *(uint32_t *)(v91 - 86) = *(uint32_t *)(v92 + 76);
          *(uint32_t *)(v91 - 82) = *(uint32_t *)(v92 + 80);
          *(uint32_t *)(v91 - 78) = *(uint32_t *)(v92 + 84);
          *(uint16_t *)(v91 - 74) = *(uint16_t *)(v92 + 88);
          v93 = *(uint32_t *)(v55 + 278736);
          v94 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v93 - 108) = *(uint32_t *)(v94 + 90);
          *(uint32_t *)(v93 - 104) = *(uint32_t *)(v94 + 94);
          *(uint32_t *)(v93 - 100) = *(uint32_t *)(v94 + 98);
          *(uint32_t *)(v93 - 96) = *(uint32_t *)(v94 + 102);
          *(uint16_t *)(v93 - 92) = *(uint16_t *)(v94 + 106);
          v95 = *(uint32_t *)(v55 + 278736);
          v96 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v95 - 126) = *(uint32_t *)(v96 + 108);
          *(uint32_t *)(v95 - 122) = *(uint32_t *)(v96 + 112);
          *(uint32_t *)(v95 - 118) = *(uint32_t *)(v96 + 116);
          *(uint32_t *)(v95 - 114) = *(uint32_t *)(v96 + 120);
          *(uint16_t *)(v95 - 110) = *(uint16_t *)(v96 + 124);
          v97 = *(uint32_t *)(v55 + 278736);
          v98 = *(uint32_t *)(v55 + 278740);
          *(uint32_t *)(v97 - 144) = *(uint32_t *)(v98 + 126);
          *(uint32_t *)(v97 - 140) = *(uint32_t *)(v98 + 130);
          *(uint32_t *)(v97 - 136) = *(uint32_t *)(v98 + 134);
          *(uint32_t *)(v97 - 132) = *(uint32_t *)(v98 + 138);
          *(uint16_t *)(v97 - 128) = *(uint16_t *)(v98 + 142);
          *(uint16_t *)(*(uint32_t *)(v55 + 278736) + 2) = 0;
          n4_1 = __n4_5;
        }
        while ( __n4_5 > n4_2 );
        v11 = v170;
      }
      if ( i_2 <= 0 )
      {
        __dword_4458E0 = 0;
        __dword_4458E4 = 0;
        __dword_4458E8 = 0;
        __dword_4458EC = 0;
      }
      else
      {
        v170 = v11;
        v99 = 0;
        v100 = 0;
        v101 = 0;
        v102 = 0;
        for ( i = 0; i < i_2; ++i )
        {
          v184 = (uint8_t)__byte_44339D[0];
          __dword_4458E0 = v102 >> 3;
          __dword_4458E4 = v101 >> 3;
          __dword_4458E8 = v100 >> 3;
          __dword_4458EC = v99 >> 3;
          lpAddress_1 = lpAddress;
          v104 = v166[(uint8_t)__byte_44339D[0]];
          v178 = __fwd_sub_421930_sub_41A130((__m128 *)lpAddress, v2, v3, v161, v160);
          v181 = (uint8_t)(v104 - v178);
          v105 = (uint8_t)lpAddress_1[v181 + 279984];
          v106 = v166[v184];
          v173 = (uint8_t)(v178 + lpAddress_1[v105 + 280496]);
          v107 = (uint8_t)(v173 + v106 - v104);
          n16 = v106 - v107;
          if ( n16 < -16 || n16 > 16 )
          {
            v105 = (uint8_t)lpAddress_1[v181 + 280240];
          }
          else
          {
            v104 = v173;
            v166[v184] = v107;
          }
          __fwd_sub_421930_sub_414390(
            (uint16_t *)&lpAddress_1[8 * *((uint32_t *)lpAddress_1 + 69676) + 940072],
            (int32_t)(lpAddress_1 + 278708),
            v105);
          __sub_41CAB0((int32_t)lpAddress_1, (__m128)__xmmword_439B50, v104, v105, v104 - v178);
          v109 = *((uint32_t *)lpAddress_1 + 69684);
          v110 = *(int16_t *)(v109 - 4);
          v111 = *(int16_t *)(v109 - 10);
          v112 = *(int16_t *)(v109 - 8);
          __dword_4458E0 += 32 * *(int16_t *)(v109 - 6);
          __dword_4458E4 += 32 * v110;
          __dword_4458E8 += 32 * v111;
          __dword_4458EC += 32 * v112;
          if ( v169 )
            v113 = 16 * v166[(uint8_t)__byte_44339D[0]];
          else
            v113 = 0;
          v114 = v160;
          *(uint16_t *)(v160[17421].m128_i32[0] + 2) = v113;
          v176 = (uint8_t)__byte_4433AD[0];
          v115 = v166[(uint8_t)__byte_4433AD[0]];
          v179 = __fwd_sub_421930_sub_41A130(v114, v2, v3, lpAddress, v161);
          v183 = (uint8_t)(v115 - v179);
          v116 = v114[17499].m128_u8[v183];
          v117 = v166[v176];
          v175 = (uint8_t)(v179 + v114[17531].m128_i8[v116]);
          v118 = (uint8_t)(v175 + v117 - v115);
          n16_1 = v117 - v118;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v116 = v114[17515].m128_u8[v183];
          }
          else
          {
            v115 = v175;
            v166[v176] = v118;
          }
          __fwd_sub_421930_sub_414390(&v114[58754].m128_u16[4 * v114[17419].m128_i32[0] + 4], (int32_t)&v114[17419].m128_i32[1], v116);
          __sub_41CAB0((int32_t)v114, (__m128)__xmmword_439B50, v115, v116, v115 - v179);
          v120 = v114[17421].m128_i32[0];
          v121 = *(int16_t *)(v120 - 4);
          v122 = *(int16_t *)(v120 - 10);
          v123 = *(int16_t *)(v120 - 8);
          __dword_4458E0 += 32 * *(int16_t *)(v120 - 6);
          __dword_4458E4 += 32 * v121;
          __dword_4458E8 += 32 * v122;
          __dword_4458EC += 32 * v123;
          if ( v168 )
            v124 = (__dword_4433A4[4 * (uint8_t)__byte_4433BD[0]] * v166[(uint8_t)__byte_4433AD[0]]
                  + __dword_4433A0[4 * (uint8_t)__byte_4433BD[0]] * v166[(uint8_t)__byte_44339D[0]]) >> 3;
          else
            LOWORD(v124) = 0;
          v125 = v161;
          *(uint16_t *)(v161[17421].m128_i32[0] + 2) = v124;
          v177 = (uint8_t)__byte_4433BD[0];
          v126 = v166[(uint8_t)__byte_4433BD[0]];
          v180 = __fwd_sub_421930_sub_41A130(v125, v2, v3, lpAddress, v160);
          v182 = (uint8_t)(v126 - v180);
          v127 = v125[17499].m128_u8[v182];
          v128 = v166[v177];
          v174 = (uint8_t)(v180 + v125[17531].m128_i8[v127]);
          v129 = (uint8_t)(v174 + v128 - v126);
          n16_2 = v128 - v129;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            v127 = v125[17515].m128_u8[v182];
          }
          else
          {
            v126 = v174;
            v166[v177] = v129;
          }
          __fwd_sub_421930_sub_414390(&v125[58754].m128_u16[4 * v125[17419].m128_i32[0] + 4], (int32_t)&v125[17419].m128_i32[1], v127);
          __sub_41CAB0((int32_t)v125, (__m128)__xmmword_439B50, v126, v127, v126 - v180);
          v131 = v125[17421].m128_i32[0];
          n4_1 = __n4_5;
          v102 = __dword_4458E0 + 32 * *(int16_t *)(v131 - 6);
          v101 = __dword_4458E4 + 32 * *(int16_t *)(v131 - 4);
          v100 = __dword_4458E8 + 32 * *(int16_t *)(v131 - 10);
          v99 = __dword_4458EC + 32 * *(int16_t *)(v131 - 8);
          if ( __n4_5 >= 4 )
          {
            __dword_4458E0 += 32 * *(int16_t *)(v131 - 6);
            __dword_4458E4 = v101;
            __dword_4458E8 = v100;
            __dword_4458EC = v99;
            if ( v155 )
              v132 = (__dword_4433A8[4 * (uint8_t)__n3_0] * v166[2]
                    + __dword_4433A4[4 * (uint8_t)__n3_0] * v166[1]
                    + __dword_4433A0[4 * (uint8_t)__n3_0] * *v166) >> 3;
            else
              LOWORD(v132) = 0;
            v133 = v162;
            *(uint16_t *)(v162[17421].m128_i32[0] + 2) = v132;
            n3 = (uint8_t)__n3_0;
            v134 = v166[(uint8_t)__n3_0];
            v157 = __fwd_sub_421930_sub_41A130(v133, v2, v3, v161, lpAddress);
            v158 = (uint8_t)(v134 - v157);
            v135 = v133[17499].m128_u8[v158];
            v136 = v166[n3];
            v137 = (uint8_t)(v157 + v133[17531].m128_i8[v135] + v136 - v134);
            n16_3 = v136 - v137;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              v135 = v133[17515].m128_u8[v158];
            }
            else
            {
              v134 = (uint8_t)(v157 + v133[17531].m128_i8[v135]);
              v166[n3] = v137;
            }
            __fwd_sub_421930_sub_414390(&v133[58754].m128_u16[4 * v133[17419].m128_i32[0] + 4], (int32_t)&v133[17419].m128_i32[1], v135);
            __sub_41CAB0((int32_t)v133, (__m128)__xmmword_439B50, v134, v135, v134 - v157);
            v139 = v133[17421].m128_i32[0];
            n4_1 = __n4_5;
            v102 = __dword_4458E0 + 32 * *(int16_t *)(v139 - 6);
            v101 = __dword_4458E4 + 32 * *(int16_t *)(v139 - 4);
            v100 = __dword_4458E8 + 32 * *(int16_t *)(v139 - 10);
            v99 = __dword_4458EC + 32 * *(int16_t *)(v139 - 8);
          }
          v166 += n4_1;
        }
        v11 = v170;
        __dword_4458E0 = v102;
        __dword_4458E4 = v101;
        __dword_4458E8 = v100;
        __dword_4458EC = v99;
      }
      ++v11;
    }
    while ( v11 < v154 );
  }
  __sub_414CE0();
  __xmmword_441120 = v144;
  __xmmword_441130 = v145;
  __xmmword_441140 = v146;
  __xmmword_441160 = v147;
  __xmmword_441170 = v148;
  __xmmword_441180 = v149;
  __xmmword_4411D0 = v150;
  __xmmword_4411E0 = v151;
  __xmmword_4411F0 = v152;
  n4_3 = __n4_5;
  if ( __n4_5 > 0 )
  {
    n4_4 = 0;
    do
    {
      lpAddress_2 = (void **)*(&lpAddress + n4_4);
      if ( lpAddress_2 )
      {
        __fwd_sub_421930_sub_419610(lpAddress_2, 1);
        n4_3 = __n4_5;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline uint32_t __fwd_sub_415380_sub_414860(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __sub_414860((int32_t *)a0, a1, a2, a3); }
static inline void ** __fwd_sub_415380_sub_415270(void *a0, char a1) { return __sub_415270((void **)a0, a1); }
static inline int32_t __fwd_sub_415380_sub_4159E0(void *a0, int32_t a1) { return __sub_4159E0((int32_t *)a0, a1); }
static inline void __fwd_sub_415380_sub_417200(int32_t a0, char a1, void *a2) { __sub_417200(a0, a1, (uint8_t *)a2); }
static inline void __fwd_sub_415380_sub_4197A0(const __m128 &a0, const __m128 &a1, void *a2, int32_t a3, int32_t a4, void *a5) { __sub_4197A0(a0, a1, (uint8_t *)a2, a3, a4, (uint8_t *)a5); }
static inline int32_t __fwd_sub_415380_sub_421930(void *a0, void *a1) { return __sub_421930((uint16_t *)a0, (uint8_t *)a1); }
static inline void __fwd_sub_415380_sub_424500(void *a0, int32_t a1, int32_t a2, void *a3) { __sub_424500((uint8_t *)a0, a1, a2, (uint8_t *)a3); }
static inline int32_t __fwd_sub_415380_sub_424D90(void *a0, int32_t a1) { return __sub_424D90((uint16_t *)a0, a1); }

BMF_SSE void __sub_415380(const __m128 &a1__ref, const __m128 &a2__ref, uint16_t *p_i, uint8_t *a4, uint8_t *a5)
{
  alignas(16) uint8_t __hexrays_frame[96];
  int32_t &Size = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v58 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &v59 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &Blocka_5 = *(int32_t *)(__hexrays_frame + 12);
  int32_t &v61 = *(int32_t *)(__hexrays_frame + 12);
  uint32_t * &Blocka_2 = *(uint32_t * *)(__hexrays_frame + 16);
  void * &Blocka = *(void * *)(__hexrays_frame + 16);
  int32_t &v64 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &v65 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v66 = *(int32_t *)(__hexrays_frame + 28);
  int32_t &v67 = *(int32_t *)(__hexrays_frame + 32);
  int32_t &v68 = *(int32_t *)(__hexrays_frame + 36);
  int32_t &v69 = *(int32_t *)(__hexrays_frame + 40);
  int32_t &v70 = *(int32_t *)(__hexrays_frame + 44);
  int32_t &v71 = *(int32_t *)(__hexrays_frame + 48);
  int32_t &v72 = *(int32_t *)(__hexrays_frame + 52);
  int32_t &v73 = *(int32_t *)(__hexrays_frame + 56);
  uint32_t &n5 = *(uint32_t *)(__hexrays_frame + 60);
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  bool v43;
  char v7, *buf, v27;
  int16_t __sub_415380_n2, v22;
  int32_t Blocka_3, v8, Blocka_4, v10, v11, v14, n2_1, n2_2, v17, v18, v19, v20, v21, Blocka_1,
          v35, v36, v41, v42, v44, v45, v46, v47, v48, v49, v50, v53, v54, v55, v56;
  uint32_t *v12, n0x10000, *v24, v28, v29, v30, v31, *v32, *v33, v34, v37, *v38, *v39, v40;
  uint8_t *v51, *v52;
  void *v5;
  if ( __dword_443364 )
  {
    if ( ::__n2 == 1 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __fwd_sub_415380_sub_424500(a4, *p_i, p_i[1], a5);
      else
        __fwd_sub_415380_sub_424D90(p_i, (int32_t)a4);
    }
    else if ( ::__n2 == 2 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __fwd_sub_415380_sub_4197A0(a1, a2, a4, *p_i, p_i[1], a5);
      else
        __fwd_sub_415380_sub_421930(p_i, a4);
    }
  }
  else
  {
    v5 = malloc(0x7BA230u);
    if ( v5 )
      Blocka_3 = __sub_417980((int32_t)v5, p_i[1], *p_i, p_i[1], p_i[5] & 0x3F);
    else
      Blocka_3 = 0;
    __sub_414F60();
    __fwd_sub_415380_sub_417200(Blocka_3, v7, a4);
    v64 = 0;
    v8 = 0;
    Blocka_2 = (uint32_t *)Blocka_3;
    Blocka_4 = Blocka_3;
    do
    {
      v10 = (uint8_t)__byte_439860[v8];
      *((uint8_t *)Blocka_2 + v10 + 1078244) = v8;
      v11 = 0;
      v58 = v8;
      v69 = v10 & 4;
      v68 = v10 & 2;
      v67 = v10 & 0x10;
      v66 = v10 & 1;
      v65 = v10 & 8;
      v70 = v10 & 0x20;
      do
      {
        v72 = v11;
        n5 = 0;
        Blocka_5 = Blocka_4;
        v71 = Blocka_4 + 15 * v11;
        do
        {
          *(uint8_t *)(v71 + 75 * n5 + 1078308) = v64;
          v73 = Blocka_2[4];
          v12 = &Blocka_2[4 * v64];
          __sub_415380_n2 = 2;
          *((uint16_t *)v12 + 48) = 2;
          LOWORD(v14) = 2;
          n2_1 = 2;
          n2_2 = 2;
          if ( v69 )
          {
            n2_1 = 4;
            n2_2 = 0;
          }
          if ( v68 )
          {
            LOWORD(v14) = n2_2 + 2;
            n2_2 = 0;
          }
          if ( v67 )
          {
            LOWORD(v14) = n2_1 + v14;
            n2_1 = 0;
          }
          if ( v66 )
          {
            __sub_415380_n2 = n2_2 + 2;
            n2_2 = 0;
            *((uint16_t *)v12 + 52) = 0;
          }
          else
          {
            *((uint16_t *)v12 + 52) = n2_2;
          }
          if ( v65 )
          {
            __sub_415380_n2 += n2_1;
            n2_1 = 0;
            *((uint16_t *)v12 + 51) = 0;
          }
          else
          {
            *((uint16_t *)v12 + 51) = n2_1;
          }
          if ( v70 )
          {
            *((uint16_t *)v12 + 49) = v14 + __sub_415380_n2;
            LOWORD(v14) = 0;
            *((uint16_t *)v12 + 50) = 0;
          }
          else
          {
            *((uint16_t *)v12 + 50) = v14;
            *((uint16_t *)v12 + 49) = __sub_415380_n2;
          }
          v17 = n2_1 != 0;
          v18 = n2_2 != 0;
          v14 = (uint16_t)v14;
          if ( (uint16_t)v14 )
            v14 = 1;
          v19 = v18 + v17 + v14 + 2;
          if ( v19 <= v73 )
          {
            *((uint8_t *)v12 + 110) = v19;
          }
          else
          {
            *((uint8_t *)v12 + 110) = v18 + v17 + v14 + 1;
            *((uint16_t *)v12 + 48) = 0;
          }
          if ( *((uint16_t *)v12 + v72 + 48)
            && *((uint16_t *)v12 + n5 + 48)
            && (v20 = *((uint8_t *)v12 + 110), v20 <= v73) )
          {
            v21 = 1;
            v22 = (uint8_t)(1 << ((5 - v20) & 31));
            *((uint8_t *)v12 + 111) = v22;
            *((uint16_t *)v12 + 54) = v22 << 6;
            *((uint16_t *)v12 + v72 + 48) += v22;
            *((uint16_t *)v12 + n5 + 48) += *((uint8_t *)v12 + 111);
            *((uint16_t *)v12 + 53) = *((uint16_t *)v12 + 48)
                                 + *((uint16_t *)v12 + 52)
                                 + *((uint16_t *)v12 + 51)
                                 + *((uint16_t *)v12 + 50)
                                 + *((uint16_t *)v12 + 49);
          }
          else
          {
            v21 = 0;
          }
          v64 += v21;
          ++n5;
        }
        while ( n5 < 5 );
        Blocka_4 = Blocka_5;
        v11 = v72 + 1;
      }
      while ( (uint32_t)(v72 + 1) < 5 );
      n0x10000 = 0;
      v24 = &Blocka_2[0x10000 * v58];
      do
      {
        LOWORD(v24[n0x10000 + 531818]) = 0x2000;
        HIWORD(v24[n0x10000++ + 531818]) = 0x2000;
      }
      while ( n0x10000 < 0x10000 );
      Blocka_4 = Blocka_5 + 1;
      v8 = v58 + 1;
    }
    while ( (uint32_t)(v58 + 1) < 0xF );
    Blocka_1 = (int32_t)Blocka_2;
    buf = (char *)malloc(Blocka_2[4]);
    Size = Blocka_2[4];
    Blocka_2[269672] = (uint32_t)buf;
    memset(buf,1,Size);
    v28 = Blocka_2[25];
    v29 = Blocka_2[26];
    Blocka_2[262916] = Blocka_2[24];
    v30 = Blocka_2[27];
    Blocka_2[262917] = v28;
    Blocka_2[262918] = v29;
    Blocka_2[262919] = v30;
    Blocka_2[269556] = (uint32_t)(Blocka_2 + 269546);
    __fwd_sub_415380_sub_414860((int32_t *)(Blocka_1 + 1078184), 0, *(uint32_t *)(Blocka_1 + 16), 1);
    Blocka_2[269558] = (uint32_t)(Blocka_2 + 269554);
    v31 = Blocka_2[4];
    v32 = (uint32_t *)malloc(24 * v31 + 4);
    if ( v32 )
    {
      *v32 = v31;
      v33 = v32 + 1;
      if ( v31 )
      {
        if ( v31 >> 1 )
        {
          v34 = 0;
          do
          {
            v35 = 12 * v34;
            v32[v35 + 6] = 0;
            ++v34;
            v32[v35 + 12] = 0;
          }
          while ( v34 < v31 >> 1 );
          Blocka_1 = (int32_t)Blocka_2;
          v36 = 2 * v34 + 1;
        }
        else
        {
          v36 = 1;
        }
        if ( v31 > v36 - 1 )
          v32[6 * v36] = 0;
      }
    }
    else
    {
      v33 = nullptr;
    }
    v37 = *(uint32_t *)(Blocka_1 + 16);
    *(uint32_t *)(Blocka_1 + 1078208) = v33;
    v38 = (uint32_t *)malloc(24 * v37 + 4);
    if ( v38 )
    {
      *v38 = v37;
      v39 = v38 + 1;
      if ( v37 )
      {
        if ( v37 >> 1 )
        {
          Blocka = (void *)Blocka_1;
          v40 = 0;
          do
          {
            v41 = 12 * v40;
            v38[v41 + 6] = 0;
            ++v40;
            v38[v41 + 12] = 0;
          }
          while ( v40 < v37 >> 1 );
          Blocka_1 = (int32_t)Blocka;
          v42 = 2 * v40 + 1;
        }
        else
        {
          v42 = 1;
        }
        if ( v37 > v42 - 1 )
          v38[6 * v42] = 0;
      }
    }
    else
    {
      v39 = nullptr;
    }
    v43 = *(uint32_t *)(Blocka_1 + 16) <= 0;
    *(uint32_t *)(Blocka_1 + 1078212) = v39;
    if ( !v43 )
    {
      v44 = 0;
      do
      {
        __fwd_sub_415380_sub_414860((int32_t *)(*(uint32_t *)(Blocka_1 + 1078208) + 24 * v44), 0, 99, 0);
        __fwd_sub_415380_sub_414860((int32_t *)(*(uint32_t *)(Blocka_1 + 1078212) + 24 * v44++), 0, 33, 0);
      }
      while ( v44 < *(uint32_t *)(Blocka_1 + 16) );
    }
    if ( *(int32_t *)(Blocka_1 + 4) > 0 )
    {
      v59 = *(uint32_t *)(Blocka_1 + 1078236);
      v45 = 0;
      do
      {
        v61 = v45 + 1;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) + 3) = *(uint16_t *)(*(uint32_t *)(Blocka_1 + 76) - 8) == 0;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) + 5) = *(uint16_t *)(*(uint32_t *)(Blocka_1 + 80) - 8) == 0;
        v46 = *(uint32_t *)(Blocka_1 + 72);
        v47 = *(uint32_t *)(Blocka_1 + 68);
        v48 = *(uint32_t *)(Blocka_1 + 64);
        v49 = *(uint32_t *)(Blocka_1 + 60);
        v50 = *(uint32_t *)(Blocka_1 + 56);
        *(uint32_t *)(Blocka_1 + 72) = v47;
        *(uint32_t *)(Blocka_1 + 68) = v48;
        *(uint32_t *)(Blocka_1 + 64) = v49;
        *(uint32_t *)(Blocka_1 + 60) = v50;
        *(uint32_t *)(Blocka_1 + 56) = v46;
        v46 += 56;
        *(uint32_t *)(Blocka_1 + 76) = v46;
        v50 += 56;
        *(uint32_t *)(Blocka_1 + 80) = v50;
        *(uint32_t *)(Blocka_1 + 84) = v49 + 56;
        *(uint32_t *)(Blocka_1 + 88) = v48 + 56;
        *(uint32_t *)(Blocka_1 + 92) = v47 + 56;
        LOBYTE(v50) = *(uint16_t *)(v50 + 8) == 0;
        *(uint8_t *)(v46 + 4) = v50;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) - 2) = v50;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) - 9) = v50;
        LOBYTE(v50) = *(uint16_t *)(*(uint32_t *)(Blocka_1 + 80) + 16) == 0;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) + 6) = v50;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) - 1) = v50;
        *(uint8_t *)(*(uint32_t *)(Blocka_1 + 76) + 7) = *(uint16_t *)(*(uint32_t *)(Blocka_1 + 80) + 24) == 0;
        v51 = *(uint8_t **)(Blocka_1 + 80);
        v52 = *(uint8_t **)(Blocka_1 + 84);
        *(uint32_t *)(Blocka_1 + 76) += 8;
        v51 += 8;
        *(uint32_t *)(Blocka_1 + 80) = v51;
        *(uint32_t *)(Blocka_1 + 88) += 8;
        v52 += 8;
        *(uint32_t *)(Blocka_1 + 84) = v52;
        *(uint32_t *)(Blocka_1 + 92) += 8;
        *(uint8_t *)(Blocka_1 + 1078692) = v51[26] + v51[18] + v51[10] + v51[2] + v51[34] - 5;
        LOBYTE(v50) = v52[26];
        LOBYTE(v47) = v52[10] + v52[2];
        LOBYTE(v51) = v52[18];
        LOBYTE(v52) = v52[34];
        *(uint8_t *)(Blocka_1 + 1078695) = 0;
        *(uint8_t *)(Blocka_1 + 1078694) = 0;
        LOBYTE(v50) = v50 + (uint8_t)v51 + v47 + (uint8_t)v52 - 5;
        v45 = v61;
        v53 = *(uint32_t *)Blocka_1;
        *(uint8_t *)(Blocka_1 + 1078693) = v50;
        if ( v53 > 0 )
        {
          v54 = 0;
          do
          {
            ++v54;
            *(uint16_t *)(*(uint32_t *)(Blocka_1 + 76) + 8 * v54 - 8) = *(uint16_t *)(v59 + 2 * v54 - 2);
            v53 = *(uint32_t *)Blocka_1;
          }
          while ( v54 < *(uint32_t *)Blocka_1 );
          v59 += 2 * v54;
        }
        if ( v53 > 0 )
        {
          v55 = 0;
          do
          {
            v56 = __fwd_sub_415380_sub_4159E0((int32_t *)Blocka_1, v55);
            __sub_416C90(Blocka_1);
            v55 += v56;
          }
          while ( v55 < *(uint32_t *)Blocka_1 );
          v45 = v61;
        }
      }
      while ( v45 < *(uint32_t *)(Blocka_1 + 4) );
    }
    __sub_414CE0();
    __fwd_sub_415380_sub_415270((void **)Blocka_1, 1);
  }
}
static inline void ** __fwd_sub_405840_sub_402E70(void *a0, char a1) { return __sub_402E70((void **)a0, a1); }
static inline char * __fwd_sub_405840_sub_407EF0(void *a0, void *a1, int32_t a2, char a3) { return __sub_407EF0((char *)a0, (char *)a1, a2, a3); }
static inline char * __fwd_sub_405840_sub_4081E0(void *a0, void *a1, int32_t a2, char a3) { return __sub_4081E0((char *)a0, (char *)a1, a2, a3); }
static inline uint32_t * __fwd_sub_405840_sub_40E590(void *a0, int32_t a1, char a2) { return __sub_40E590((uint32_t *)a0, a1, a2); }
static inline char * __fwd_sub_405840_sub_40E860(int32_t a0, char a1, void *a2, int32_t a3, int32_t a4, void *a5) { return __sub_40E860(a0, a1, (char *)a2, a3, a4, (uint8_t *)a5); }
static inline char * __fwd_sub_405840_sub_40F450(void *a0, char a1, int32_t a2, int32_t a3, int32_t a4) { return __sub_40F450((uint32_t *)a0, a1, a2, a3, a4); }
static inline uint8_t * __fwd_sub_405840_sub_410AC0(void *a0, int32_t a1, int32_t a2, void *a3) { return __sub_410AC0((uint8_t *)a0, a1, a2, (uint8_t *)a3); }
static inline uint32_t __fwd_sub_405840_sub_4111B0(void *a0, int32_t a1, int32_t a2, void *a3) { return __sub_4111B0((uint8_t *)a0, a1, a2, (uint8_t *)a3); }
static inline void __fwd_sub_405840_sub_415380(const __m128 &a0, const __m128 &a1, void *a2, void *a3, void *a4) { __sub_415380(a0, a1, (uint16_t *)a2, (uint8_t *)a3, (uint8_t *)a4); }

BMF_SSE void __sub_405840(char *Blockb, char *Srca_3, int32_t a3, char a4, const __m128 &a5__ref, const __m128 &a6__ref)
{
  alignas(16) uint8_t __hexrays_frame[76];
  char * &Blockb_1 = *(char * *)(__hexrays_frame + 0);
  void ** &Blocka = *(void ** *)(__hexrays_frame + 0);
  char * &Blockc = *(char * *)(__hexrays_frame + 0);
  char * &Src = *(char * *)(__hexrays_frame + 4);
  char * &Srca = *(char * *)(__hexrays_frame + 4);
  char * &v42 = *(char * *)(__hexrays_frame + 8);
  char * &v43 = *(char * *)(__hexrays_frame + 8);
  int32_t &Size = *(int32_t *)(__hexrays_frame + 12);
  int32_t &Sizea = *(int32_t *)(__hexrays_frame + 12);
  int32_t &n4_1 = *(int32_t *)(__hexrays_frame + 16);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 16);
  uint16_t (&p_i)[2] = *(uint16_t (*)[2])(__hexrays_frame + 20);
  int32_t &v49 = *(int32_t *)(__hexrays_frame + 24);
  int32_t &v50 = *(int32_t *)(__hexrays_frame + 28);
  int32_t &v51 = *(int32_t *)(__hexrays_frame + 32);
  int32_t &v52 = *(int32_t *)(__hexrays_frame + 36);
  int32_t &v53 = *(int32_t *)(__hexrays_frame + 40);
  ;
  __m128 a5 = a5__ref;
  __m128 a6 = a6__ref;
  char *Srca_1, v8, *Srca_2, v10, *__sub_405840_buf, v15, v20, *Srca_5, *Srca_6, v27, v29, v30,
       v31, *Srca_4;
  int32_t n1008, v17, v18, v19, Size_2, v24, Size_1, v26, Sizea_2, v34, Sizea_1, v36;
  uint32_t v12, *v14, *v28;
  void **Blockb_2;
  v53 = a3;
  Srca_1 = Srca_3;
  v8 = __byte_44339E[16 * a3];
  v52 = 16 * a3;
  __n2 = v8 & 3;
  __dword_443364 = (uint8_t)(__byte_44339E[16 * a3] & 4) >> 2;
  Srca_2 = Srca_3;
  __fwd_sub_405840_sub_407EF0(Blockb, Srca_3, a3, a4);
  __sub_405840_buf = ::__buf[0];
  v12 = (uint32_t)(::__buf[0] + 15) & 0xFFFFFFF0;
  *(uint64_t *)::__buf[0] = 0;
  *((uint32_t *)__sub_405840_buf + 2) = 0;
  *((uint16_t *)__sub_405840_buf + 6) = 0;
  __sub_405840_buf[14] = 0;
  *((uint64_t *)__sub_405840_buf + 126) = 0;
  *((uint64_t *)__sub_405840_buf + 127) = 0;
  n1008 = 1008;
  do
  {
    *(__m128i *)(v12 + n1008 - 16) = 0;
    *(__m128i *)(v12 + n1008 - 32) = 0;
    *(__m128i *)(v12 + n1008 - 48) = 0;
    *(__m128i *)(v12 + n1008 - 64) = 0;
    *(__m128i *)(v12 + n1008 - 80) = 0;
    *(__m128i *)(v12 + n1008 - 96) = 0;
    *(__m128i *)(v12 + n1008 - 112) = 0;
    n1008 -= 112;
  }
  while ( n1008 );
  if ( __dword_443384 )
  {
    v17 = *((uint32_t *)Blockb + 1);
    v18 = *((uint32_t *)Blockb + 2);
    v19 = *((uint32_t *)Blockb + 3);
    *(uint32_t *)p_i = *(uint32_t *)Blockb;
    v49 = v17;
    v50 = v18;
    BYTE2(v50) = 72;
    v51 = v19;
    if ( __n256_0[0] )
    {
      Srca_2 = (char *)malloc(p_i[1] * p_i[0]);
      Srca_4 = &Blockb[v53 + 16];
      Sizea = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
      n4 = __n4_5;
      if ( __n4_5 == 1 )
      {
        memcpy(Srca_2,&Blockb[v53 + 16],Sizea);
      }
      else
      {
        if ( (v43 = &Blockb[v53], Sizea <= 6)
          || __n4_5 <= 0
          || (__n4_5 > 1 || Srca_2 <= Srca_4 || Srca_2 - Srca_4 < (uint32_t)(Sizea * __n4_5))
          && (Srca_4 <= Srca_2 || Srca_4 - Srca_2 < (uint32_t)Sizea) )
        {
          Blockc = Blockb;
          Sizea_1 = 0;
          v36 = 0;
          Srca = Srca_1;
          do
          {
            Srca_2[Sizea_1] = v43[v36 + 16];
            v36 += n4;
            ++Sizea_1;
          }
          while ( Sizea_1 < Sizea );
        }
        else
        {
          Blockc = Blockb;
          Sizea_2 = 0;
          v34 = 0;
          Srca = Srca_1;
          do
          {
            Srca_2[Sizea_2] = v43[v34 + 16];
            v34 += n4;
            ++Sizea_2;
          }
          while ( Sizea_2 < Sizea );
        }
        Blockb = Blockc;
        Srca_1 = Srca;
      }
      if ( !__dword_443364 )
        __fwd_sub_405840_sub_4111B0(Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1), Srca_2);
    }
    if ( __n2 == 1 && !__dword_443364 )
      __sub_4108C0((int32_t)Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1));
    __fwd_sub_405840_sub_415380(a5, a6, p_i, (uint8_t *)Srca_1, Srca_2);
    if ( Srca_2 != Srca_1 )
    {
      if ( (uint8_t)__byte_44339C[v52] != __n4_5 - 1 )
        __fwd_sub_405840_sub_4081E0(Blockb, Srca_2, v53, v20);
      free(Srca_2);
    }
  }
  else
  {
    if ( __n256_0[0] )
    {
      Srca_5 = (char *)(__Buffer_0 + 32);
      Srca_6 = &Blockb[v53 + 16];
      Size = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
      n4_1 = __n4_5;
      if ( __n4_5 == 1 )
      {
        memcpy((char *)(__Buffer_0 + 32),&Blockb[v53 + 16],Size);
      }
      else
      {
        v42 = &Blockb[v53];
        if ( Size <= 6
          || __n4_5 <= 0
          || (__n4_5 > 1 || Srca_5 <= Srca_6 || Srca_5 - Srca_6 < (uint32_t)(Size * __n4_5))
          && (Srca_6 <= Srca_5 || Srca_6 - Srca_5 < (uint32_t)Size) )
        {
          Blockb_1 = Blockb;
          Size_1 = 0;
          v26 = 0;
          Src = Srca_1;
          do
          {
            Srca_5[Size_1] = v42[v26 + 16];
            v26 += n4_1;
            ++Size_1;
          }
          while ( Size_1 < Size );
        }
        else
        {
          Blockb_1 = Blockb;
          Size_2 = 0;
          v24 = 0;
          Src = Srca_1;
          do
          {
            Srca_5[Size_2] = v42[v24 + 16];
            v24 += n4_1;
            ++Size_2;
          }
          while ( Size_2 < Size );
        }
        Blockb = Blockb_1;
        Srca_1 = Src;
      }
      if ( __n2 )
      {
        if ( __n2 == 1 )
        {
          __fwd_sub_405840_sub_410AC0(Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1), Srca_5);
        }
        else
        {
          v28 = malloc(0x30ACu);
          if ( v28 )
          {
            Blocka = (void **)__fwd_sub_405840_sub_40E590(v28, *(uint16_t *)Blockb, *Srca_1);
            __fwd_sub_405840_sub_40E860((int32_t)Blocka, v30, Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1), Srca_5);
          }
          else
          {
            Blocka = nullptr;
            __fwd_sub_405840_sub_40E860(0, v29, Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1), Srca_5);
          }
          if ( Blocka )
            __fwd_sub_405840_sub_402E70(Blocka, 1);
        }
      }
      else
      {
        __fwd_sub_405840_sub_4111B0(Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1), Srca_5);
      }
      if ( (uint8_t)__byte_44339C[v52] != __n4_5 - 1 )
        __fwd_sub_405840_sub_4081E0(Blockb, Srca_5, v53, v27);
    }
    else if ( __n2 == 1 )
    {
      __sub_4108C0((int32_t)Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1));
    }
    else if ( __n2 == 2 )
    {
      v14 = malloc(0x30ACu);
      if ( v14 )
        Blockb_2 = (void **)__fwd_sub_405840_sub_40E590(v14, *(uint16_t *)Blockb, *Srca_1);
      else
        Blockb_2 = nullptr;
      __fwd_sub_405840_sub_40F450(Blockb_2, v15, (int32_t)Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1));
      if ( Blockb_2 )
        __fwd_sub_405840_sub_402E70(Blockb_2, 1);
    }
    __sub_408510(
      (int32_t)Srca_1,
      *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1),
      *(uint16_t *)Blockb);
  }
}

static inline char * __fwd_sub_407B30_sub_407EF0(void *a0, void *a1, int32_t a2, char a3) { return __sub_407EF0((char *)a0, (char *)a1, a2, a3); }
static inline char * __fwd_sub_407B30_sub_4081E0(void *a0, void *a1, int32_t a2, char a3) { return __sub_4081E0((char *)a0, (char *)a1, a2, a3); }
static inline uint8_t * __fwd_sub_407B30_sub_410AC0(void *a0, int32_t a1, int32_t a2, void *a3) { return __sub_410AC0((uint8_t *)a0, a1, a2, (uint8_t *)a3); }
static inline uint32_t __fwd_sub_407B30_sub_4111B0(void *a0, int32_t a1, int32_t a2, void *a3) { return __sub_4111B0((uint8_t *)a0, a1, a2, (uint8_t *)a3); }
static inline void __fwd_sub_407B30_sub_415380(const __m128 &a0, const __m128 &a1, void *a2, void *a3, void *a4) { __sub_415380(a0, a1, (uint16_t *)a2, (uint8_t *)a3, (uint8_t *)a4); }

BMF_SSE void __sub_407B30(uint16_t *p_i, int32_t a2, char a3, const __m128 &a4__ref, const __m128 &a5__ref)
{
  alignas(16) uint8_t __hexrays_frame[76];
  int32_t &v32 = *(int32_t *)(__hexrays_frame + 0);
  int32_t &v33 = *(int32_t *)(__hexrays_frame + 4);
  int32_t &Size = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n4 = *(int32_t *)(__hexrays_frame + 12);
  char * &Srca = *(char * *)(__hexrays_frame + 16);
  int32_t &n4_3 = *(int32_t *)(__hexrays_frame + 20);
  int32_t &n4_4 = *(int32_t *)(__hexrays_frame + 20);
  uint16_t * &p_i_1 = *(uint16_t * *)(__hexrays_frame + 24);
  char * &Src = *(char * *)(__hexrays_frame + 28);
  uint16_t * &Srca_3 = *(uint16_t * *)(__hexrays_frame + 32);
  char * &p_ia = *(char * *)(__hexrays_frame + 36);
  char * &Buffer_1 = *(char * *)(__hexrays_frame + 40);
  ;
  __m128 a4 = a4__ref;
  __m128 a5 = a5__ref;
  char *__sub_407B30_Buffer, *p_ia_1, v9, *Src_1, v11, *Src_3, *Src_2, *v20, v25, v31;
  int32_t n4_1, v14, __sub_407B30_n2, v16, Size_3, n4_2, v21, i, Size_4, v24, Size_2, v28,
          Size_1, v30;
  uint32_t Srca_2;
  uint8_t *Srca_1;
  memset(__buf[0],0,4096);
  __sub_407B30_Buffer = (char *)::__Buffer;
  p_ia_1 = (char *)::__Buffer + 16;
  *((uint32_t *)::__Buffer + 4) = *(uint32_t *)p_i;
  *((uint32_t *)p_ia_1 + 1) = *((uint32_t *)p_i + 1);
  *((uint32_t *)p_ia_1 + 2) = *((uint32_t *)p_i + 2);
  *((uint32_t *)p_ia_1 + 3) = *((uint32_t *)p_i + 3);
  Srca_1 = (uint8_t *)(p_i + 8);
  memcpy(__sub_407B30_Buffer + 32,(char *)p_i + 16,*((uint32_t *)p_i + 3));
  Src_1 = (char *)malloc(*p_i * p_i[1]);
  Src_3 = Src_1;
  if ( __n4_5 > 0 )
  {
    Src = Src_1;
    Srca_3 = p_i + 8;
    p_ia = p_ia_1;
    Buffer_1 = __sub_407B30_Buffer;
    n4_1 = 0;
    do
    {
      ++n4_1;
      v14 = BYTE1(__n256_2[4 * n4_1]);
      __sub_407B30_n2 = __byte_44339E[16 * v14] & 3;
      ::__n2 = __sub_407B30_n2;
      v16 = (uint8_t)(__byte_44339E[16 * v14] & 4) >> 2;
      __dword_443364 = v16;
      if ( ((__byte_44339E[16 * v14] & 8) != 0 || __sub_407B30_n2 || __n256_0[0]) && (!v16 || !__dword_443384) )
      {
        __fwd_sub_407B30_sub_407EF0(p_ia, Src, v14, v11);
        if ( ::__n2 != 2 )
        {
          if ( __n256_0[0] )
          {
            Srca = (char *)malloc(p_i[1] * *p_i);
            n4 = __n4_5;
            Size = *(uint16_t *)p_ia * *((uint16_t *)p_ia + 1);
            if ( __n4_5 == 1 )
            {
              memcpy(Srca,&Buffer_1[v14 + 32],Size);
            }
            else
            {
              v33 = (int32_t)&Buffer_1[v14 + 16];
              if ( Size <= 6
                || (Srca_2 = (uint32_t)&Buffer_1[v14 + 32], __n4_5 <= 0)
                || (__n4_5 > 1 || Srca_2 >= (uint32_t)Srca || (uint32_t)&Srca[-Srca_2] < Size * __n4_5)
                && (Srca_2 <= (uint32_t)Srca || Srca_2 - (uint32_t)Srca < Size) )
              {
                v32 = v14;
                Size_1 = 0;
                v30 = 0;
                n4_4 = n4_1;
                p_i_1 = p_i;
                do
                {
                  Srca[Size_1] = *(uint8_t *)(v30 + v33 + 16);
                  v30 += n4;
                  ++Size_1;
                }
                while ( Size_1 < Size );
              }
              else
              {
                v32 = v14;
                Size_2 = 0;
                v28 = 0;
                n4_4 = n4_1;
                p_i_1 = p_i;
                do
                {
                  Srca[Size_2] = *(uint8_t *)(v28 + v33 + 16);
                  v28 += n4;
                  ++Size_2;
                }
                while ( Size_2 < Size );
              }
              v14 = v32;
              n4_1 = n4_4;
              p_i = p_i_1;
            }
            if ( ::__n2 )
              __fwd_sub_407B30_sub_410AC0(Src, *p_i, p_i[1], Srca);
            else
              __fwd_sub_407B30_sub_4111B0(Src, *p_i, p_i[1], Srca);
            if ( n4_1 != __n4_5 )
              __fwd_sub_407B30_sub_4081E0(p_ia, Srca, v14, v31);
            free(Srca);
          }
          else if ( ::__n2 == 1 )
          {
            __sub_4108C0((int32_t)Src, *p_i, p_i[1]);
          }
        }
        Size_3 = *p_i * p_i[1];
        n4_2 = __n4_5;
        Src_2 = (char *)p_i + v14 + 16;
        if ( __n4_5 == 1 )
        {
          memcpy((char *)p_i + v14 + 16,Src,Size_3);
        }
        else
        {
          v20 = (char *)p_i + v14;
          if ( Size_3 <= 6
            || __n4_5 <= 0
            || (Src_2 <= Src || Size_3 > (uint32_t)(Src_2 - Src))
            && (__n4_5 > 1 || Src_2 >= Src || Src - Src_2 < (uint32_t)(Size_3 * __n4_5)) )
          {
            Size_4 = 0;
            v24 = 0;
            do
            {
              v20[v24 + 16] = Src[Size_4];
              v24 += n4_2;
              ++Size_4;
            }
            while ( Size_4 < Size_3 );
          }
          else
          {
            n4_3 = n4_1;
            v21 = 0;
            for ( i = 0; i < Size_3; ++i )
            {
              v20[v21 + 16] = Src[i];
              v21 += n4_2;
            }
            n4_1 = n4_3;
          }
        }
      }
    }
    while ( n4_1 < __n4_5 );
    Src_3 = Src;
    Srca_1 = (uint8_t *)Srca_3;
  }
  free(Src_3);
  if ( __dword_443384 )
  {
    __fwd_sub_407B30_sub_415380(a4, a5, p_i, Srca_1, Srca_1);
  }
  else
  {
    __dword_443364 = 0;
    ::__n2 = 0;
    __sub_408510((int32_t)Srca_1, *((uint32_t *)p_i + 3), p_i[2]);
  }
}

static inline void ** __fwd_sub_403820_sub_402E70(void *a0, char a1) { return __sub_402E70((void **)a0, a1); }
static inline char * __fwd_sub_403820_sub_4081E0(void *a0, void *a1, int32_t a2, char a3) { return __sub_4081E0((char *)a0, (char *)a1, a2, a3); }
static inline void __fwd_sub_403820_sub_40CF80(int32_t a0, void *a1, int32_t a2) { __sub_40CF80(a0, (uint8_t *)a1, a2); }
static inline uint32_t * __fwd_sub_403820_sub_40E590(void *a0, int32_t a1, char a2) { return __sub_40E590((uint32_t *)a0, a1, a2); }
static inline char * __fwd_sub_403820_sub_40FAC0(void *a0, char a1, int32_t a2, int32_t a3, int32_t a4) { return __sub_40FAC0((int32_t *)a0, a1, a2, a3, a4); }
static inline void __fwd_sub_403820_sub_417DB0(char a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { __sub_417DB0(a0, a1, a2, (uint16_t *)a3, (uint8_t *)a4); }

BMF_SSE char * __sub_403820(int32_t a1, const __m128 &a2__ref, const __m128 &a3__ref, int32_t a4, int32_t *p_dwLowDateTime)
{
  alignas(16) uint8_t __hexrays_frame[104];
  char * &Block = *(char * *)(__hexrays_frame + 0);
  char * &Blocka = *(char * *)(__hexrays_frame + 0);
  void ** &Blockb = *(void ** *)(__hexrays_frame + 0);
  uint16_t (&p_i)[2] = *(uint16_t (*)[2])(__hexrays_frame + 4);
  int32_t &v81 = *(int32_t *)(__hexrays_frame + 8);
  int32_t &n4_10 = *(int32_t *)(__hexrays_frame + 12);
  uint32_t &Size = *(uint32_t *)(__hexrays_frame + 16);
  int32_t &n4_1 = *(int32_t *)(__hexrays_frame + 20);
  void * &Src = *(void * *)(__hexrays_frame + 24);
  int32_t &v86 = *(int32_t *)(__hexrays_frame + 28);
  char * &p_i_2 = *(char * *)(__hexrays_frame + 32);
  int32_t &v88 = *(int32_t *)(__hexrays_frame + 36);
  uint32_t &ElementCount_3 = *(uint32_t *)(__hexrays_frame + 40);
  uint16_t (&Buffer_2)[5] = *(uint16_t (*)[5])(__hexrays_frame + 44);
  uint8_t &v91 = *(uint8_t *)(__hexrays_frame + 54);
  char &v92 = *(char *)(__hexrays_frame + 55);
  uint32_t &ElementCount = *(uint32_t *)(__hexrays_frame + 56);
  int32_t &Buffer_ = *(int32_t *)(__hexrays_frame + 60);
  uint32_t &ElementSize = *(uint32_t *)(__hexrays_frame + 64);
  uint32_t &__sub_403820_Buffer = *(uint32_t *)(__hexrays_frame + 68);
  ;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  FILE *Stream_1, *Stream_v;
  char v10, *p_i_1, v17, v18, v20, v34, v35, *Buffer_3, *n4_6, v57, *n4_7, *v64, v72;
  int32_t v5, Buffer__1, dwLowDateTime, v21, n4, v24, __sub_403820_n2, v27, v28, v29, v30,
          ArgList, v33, n4_4, v37, n2_1, i, Size_4, Size_5, n4_3, v44, Size_2, Size_3, n4_2, v48,
          n2_2, __sub_403820_n4_5, v55, Src_2, v58, n4_8, v61, i_1, n4_9, *Blockb_1, v76;
  uint16_t i_2;
  uint32_t __sub_403820_Buffer_1, v12, *v13, ElementCount_5, ElementCount_2, v23, v25, Size_1,
           ElementCount_1, ElementCount_4, v67, v68, v69, v70, *v71, v74, v75;
  uint8_t *Src_1;
  void *Src_3;
  v5 = a1;
  if ( p_dwLowDateTime )
    *p_dwLowDateTime = 0;
  Stream_1 = *(FILE **)(a1 + 4);
  if ( !Stream_1 )
    return nullptr;
  while ( 1 )
  {
    if ( fread(&__sub_403820_Buffer, 4u, 1u, Stream_1) != 1 )
    {
      Stream_v = *(FILE **)(v5 + 4);
      if ( feof(Stream_v) )
        return nullptr;
      goto LABEL_15;
    }
    __sub_403820_Buffer_1 = __sub_403820_Buffer;
    if ( (uint16_t)__sub_403820_Buffer != 0x9081 )
      break;
    __n512[0] = ((BYTE2(__sub_403820_Buffer) << 8) - 12288) | (HIBYTE(__sub_403820_Buffer) - 48);
    if ( __n512[0] != 512 || fread(&Buffer_, 8u, 1u, *(FILE **)(v5 + 4)) != 1 )
      break;
    fseek(*(FILE **)(v5 + 4), ElementSize, 1);
    Stream_1 = *(FILE **)(v5 + 4);
  }
  if ( (uint16_t)__sub_403820_Buffer_1 != 0x8A81
    || (__n512[0] = ((BYTE2(__sub_403820_Buffer_1) << 8) - 12288) | (HIBYTE(__sub_403820_Buffer_1) - 48), __n512[0] != 512)
    || fread(Buffer_2, 0x10u, 1u, *(FILE **)(v5 + 4)) != 1 )
  {
    Stream_v = *(FILE **)(v5 + 4);
LABEL_15:
    fclose(Stream_v);
    *(uint32_t *)(v5 + 4) = 0;
    return nullptr;
  }
  v10 = v92;
  ++*(uint32_t *)v5;
  if ( v10 < 0 )
  {
    fread(&Buffer_, 8u, 1u, *(FILE **)(v5 + 4));
    if ( p_dwLowDateTime )
    {
      Buffer__1 = Buffer_;
      v12 = (ElementSize + (ElementSize == 0) + 3) & 0xFFFFFFFC;
      v13 = malloc(v12 + 8);
      *v13 = Buffer__1;
      v13[1] = v12;
      *(uint32_t *)((char *)v13 + v12 + 4) = 0;
      *p_dwLowDateTime = (int32_t)v13;
      fread(v13 + 2, ElementSize, 1u, *(FILE **)(v5 + 4));
    }
    else
    {
      fseek(*(FILE **)(v5 + 4), ElementSize, 1);
    }
  }
  ElementCount_5 = 3 << (v91 & 31);
  if ( (v91 & 0x80) == 0 )
    ElementCount_5 = v91 & 0x80;
  ElementCount_3 = ElementCount_5;
  if ( a4 )
  {
    fseek(*(FILE **)(v5 + 4), ElementCount_3 + ElementCount, 1);
    return nullptr;
  }
  p_i_1 = (char *)__sub_42B830(Buffer_2[0], Buffer_2[1], v91 & 0x3F, (uint8_t)(v91 & 0x80) >> 7, 1);
  v88 = v91;
  p_i_1[10] = v91;
  if ( p_dwLowDateTime )
  {
    dwLowDateTime = *p_dwLowDateTime;
    if ( *p_dwLowDateTime )
      LOBYTE(dwLowDateTime) = 1;
  }
  else
  {
    LOBYTE(dwLowDateTime) = 0;
  }
  v17 = v92;
  v18 = v88;
  p_i_1[11] |= v92 & 2 | ((uint8_t)dwLowDateTime << 7);
  ::__n4_5 = ((v18 & 0x3Fu) + 7) >> 3;
  if ( (v17 & 0x20) == 0 )
  {
    ElementCount_1 = ElementCount;
    if ( fread(p_i_1 + 16, 1u, ElementCount, *(FILE **)(v5 + 4)) != ElementCount_1 )
      goto LABEL_31;
    goto LABEL_109;
  }
  __n256_0[0] = 0;
  __dword_443388 = 0;
  __dword_443384 = (uint8_t)(v17 & 4) >> 2;
  __ElementCount_0 = ElementCount;
  ::__Buffer = malloc(ElementCount);
  __Buffer_0 = (int32_t)::__Buffer;
  __n8 = 0;
  __n256 = 0;
  ::__Buffer_1 = (int32_t)::__Buffer;
  __buf[0] = (char *)::__Buffer + __ElementCount_0 - 4096;
  ElementCount_2 = ElementCount;
  if ( fread(::__Buffer, 1u, ElementCount, *(FILE **)(v5 + 4)) != ElementCount_2 )
  {
LABEL_31:
    fclose(*(FILE **)(v5 + 4));
    *(uint32_t *)(v5 + 4) = 0;
    return nullptr;
  }
  v20 = p_i_1[10];
  if ( (v20 & 0x3Fu) <= 4 || (v92 & 0x10) == 0 )
  {
    ::__n2 = 0;
    __dword_443364 = 0;
    if ( __dword_443384 )
      __fwd_sub_403820_sub_417DB0(ElementCount_2, a2, a3, (uint16_t *)p_i_1, (uint8_t *)p_i_1 + 16);
    else
      __fwd_sub_403820_sub_40CF80(*((uint16_t *)p_i_1 + 2), p_i_1 + 16, *((uint32_t *)p_i_1 + 3));
    goto LABEL_106;
  }
  if ( ::__n4_5 == 1 )
  {
    if ( (v20 & 0x40) == 0 )
      goto LABEL_42;
  }
  else if ( ::__n4_5 <= 2 )
  {
    goto LABEL_42;
  }
  __n8 -= 4;
  if ( __n8 < 0 )
  {
    v75 = *(uint32_t *)__Buffer_0;
    __Buffer_0 += 4;
    v76 = v75 << ((__n8 + 4) & 31);
    ElementCount_2 = v75 >> (-(char)__n8 & 31);
    v21 = __n256 | v76 & 0xF;
    __n256 = ElementCount_2;
    __n8 += 32;
  }
  else
  {
    v21 = __n256 & 0xF;
    __n256 = (uint32_t)__n256 >> 4;
  }
  __n256_0[0] = v21;
LABEL_42:
  if ( ::__n4_5 > 0 )
  {
    LOBYTE(ElementCount_2) = 63;
    *(uint32_t *)p_i = 255;
    p_i_2 = p_i_1;
    v86 = v5;
    n4 = 0;
    do
    {
      __n8 -= 6;
      if ( __n8 < 0 )
      {
        v74 = *(uint32_t *)__Buffer_0;
        __Buffer_0 += 4;
        v23 = __n256 | (v74 << ((__n8 + 6) & 31)) & 0x3F;
        __n256 = v74 >> (-(char)__n8 & 31);
        __n8 += 32;
      }
      else
      {
        v23 = __n256 & 0x3F;
        __n256 = (uint32_t)__n256 >> 6;
      }
      v24 = 16 * n4;
      v25 = v23 >> 2;
      __sub_403820_n2 = v23 & 3;
      __byte_44339E[v24] = v25;
      __byte_44339C[v24] = __sub_403820_n2;
      __byte_44339D[16 * __sub_403820_n2] = n4;
      if ( (__byte_44339E[16 * n4] & 8) != 0 )
      {
        __n8 -= 8;
        if ( __n8 < 0 )
        {
          v70 = *(uint32_t *)__Buffer_0;
          __Buffer_0 += 4;
          v27 = __n256 | *(uint32_t *)p_i & (v70 << ((__n8 + 8) & 31));
          __n256 = v70 >> (-(char)__n8 & 31);
          __n8 += 32;
        }
        else
        {
          LOBYTE(v27) = __n256 & LOBYTE(p_i[0]);
          __n256 = (uint32_t)__n256 >> 8;
        }
        __byte_44339F[16 * n4] = v27;
        if ( __sub_403820_n2 > 1 )
        {
          __n8 -= 8;
          if ( __n8 < 0 )
          {
            v69 = *(uint32_t *)__Buffer_0;
            __Buffer_0 += 4;
            v28 = __n256 | *(uint32_t *)p_i & (v69 << ((__n8 + 8) & 31));
            __n256 = v69 >> (-(char)__n8 & 31);
            __n8 += 32;
          }
          else
          {
            v28 = __n256 & *(uint32_t *)p_i;
            __n256 = (uint32_t)__n256 >> 8;
          }
          __dword_4433A0[4 * n4] = v28 - 64;
          __n8 -= 8;
          if ( __n8 < 0 )
          {
            v68 = *(uint32_t *)__Buffer_0;
            __Buffer_0 += 4;
            v29 = __n256 | *(uint32_t *)p_i & (v68 << ((__n8 + 8) & 31));
            __n256 = v68 >> (-(char)__n8 & 31);
            __n8 += 32;
          }
          else
          {
            v29 = __n256 & *(uint32_t *)p_i;
            __n256 = (uint32_t)__n256 >> 8;
          }
          __dword_4433A4[4 * n4] = v29 - 64;
          if ( __sub_403820_n2 > 2 )
          {
            __n8 -= 8;
            if ( __n8 < 0 )
            {
              v67 = *(uint32_t *)__Buffer_0;
              __Buffer_0 += 4;
              v30 = __n256 | *(uint32_t *)p_i & (v67 << ((__n8 + 8) & 31));
              __n256 = v67 >> (-(char)__n8 & 31);
              __n8 += 32;
            }
            else
            {
              v30 = __n256 & *(uint32_t *)p_i;
              __n256 = (uint32_t)__n256 >> 8;
            }
            __dword_4433A8[4 * n4] = v30 - 64;
          }
        }
      }
      ++n4;
    }
    while ( n4 < ::__n4_5 );
    p_i_1 = p_i_2;
    v5 = v86;
  }
  Src_1 = (uint8_t *)malloc(*(uint16_t *)p_i_1 * *((uint16_t *)p_i_1 + 1));
  if ( (v92 & 8) != 0 )
  {
    *(uint32_t *)p_i = *(uint32_t *)p_i_1;
    v81 = *((uint32_t *)p_i_1 + 1);
    n4_10 = *((uint32_t *)p_i_1 + 2);
    Size = *((uint32_t *)p_i_1 + 3);
    BYTE2(n4_10) = 72;
    if ( ::__n4_5 > 0 )
    {
      v86 = v5;
      ArgList = 0;
      do
      {
        v33 = (uint8_t)__byte_44339D[16 * ArgList];
        ::__n2 = __byte_44339E[16 * v33] & 3;
        __dword_443364 = (uint8_t)(__byte_44339E[16 * v33] & 4) >> 2;
        if ( __dword_443384 )
          __fwd_sub_403820_sub_417DB0(ArgList, a2, a3, p_i, Src_1);
        else
          __fwd_sub_403820_sub_40CF80(*(uint16_t *)p_i_1, Src_1, *(uint16_t *)p_i_1 * *((uint16_t *)p_i_1 + 1));
        if ( ::__n2 )
        {
          if ( ::__n2 == 1 )
          {
            if ( !__dword_443384 || !__dword_443364 )
              __sub_410310((int32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
          }
          else if ( !__dword_443384 && ::__n2 == 2 )
          {
            v71 = malloc(0x30ACu);
            Blockb_1 = v71 ? __fwd_sub_403820_sub_40E590(v71, *(uint16_t *)p_i_1, *Src_1) : nullptr;
            Blockb = (void **)Blockb_1;
            __fwd_sub_403820_sub_40FAC0(Blockb_1, v72, (int32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
            if ( Blockb )
              __fwd_sub_403820_sub_402E70(Blockb, 1);
          }
        }
        else
        {
          __sub_410650((uint32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
        }
        __fwd_sub_403820_sub_4081E0(p_i_1, (char *)Src_1, v33, v34);
        ++ArgList;
      }
      while ( ArgList < ::__n4_5 );
LABEL_104:
      v5 = v86;
    }
  }
  else
  {
    ::__n2 = __byte_44339E[0] & 3;
    __dword_443364 = (uint8_t)(__byte_44339E[0] & 4) >> 2;
    if ( __dword_443384 )
    {
      __fwd_sub_403820_sub_417DB0(ElementCount_2, a2, a3, (uint16_t *)p_i_1, (uint8_t *)p_i_1 + 16);
      if ( __dword_443364 )
        goto LABEL_105;
    }
    else
    {
      __dword_443364 = 0;
      ::__n2 = 0;
      __fwd_sub_403820_sub_40CF80(*((uint16_t *)p_i_1 + 2), p_i_1 + 16, *((uint32_t *)p_i_1 + 3));
    }
    if ( ::__n4_5 > 0 )
    {
      v86 = v5;
      n4_4 = 0;
      do
      {
        ++n4_4;
        v37 = BYTE1(__n256_2[4 * n4_4]);
        n2_1 = __byte_44339E[16 * v37] & 3;
        ::__n2 = n2_1;
        if ( (__byte_44339E[16 * v37] & 8) != 0 || n2_1 || __n256_0[0] )
        {
          i = *(uint16_t *)p_i_1;
          Src = &p_i_1[v37 + 16];
          Size_1 = i * *((uint16_t *)p_i_1 + 1);
          n4_1 = ::__n4_5;
          Size = Size_1;
          if ( ::__n4_5 == 1 )
          {
            memcpy((char *)Src_1,(char *)Src,Size);
            n2_2 = ::__n2;
          }
          else
          {
            Block = &p_i_1[v37];
            if ( (int32_t)Size <= 6
              || n4_1 <= 0
              || (n4_1 > 1 || Src_1 <= Src || Src_1 - (uint8_t *)Src < Size * ::__n4_5)
              && (Src_1 >= Src || (uint8_t *)Src - Src_1 < Size) )
            {
              *(uint32_t *)p_i = n2_1;
              v81 = v37;
              n4_10 = n4_4;
              Size_2 = Size;
              p_i_2 = p_i_1;
              Size_3 = 0;
              n4_2 = n4_1;
              v48 = 0;
              do
              {
                Src_1[Size_3] = Block[v48 + 16];
                v48 += n4_2;
                ++Size_3;
              }
              while ( Size_3 < Size_2 );
            }
            else
            {
              *(uint32_t *)p_i = n2_1;
              v81 = v37;
              n4_10 = n4_4;
              Size_4 = Size;
              p_i_2 = p_i_1;
              Size_5 = 0;
              n4_3 = n4_1;
              v44 = 0;
              do
              {
                Src_1[Size_5] = Block[v44 + 16];
                v44 += n4_3;
                ++Size_5;
              }
              while ( Size_5 < Size_4 );
            }
            n2_2 = *(uint32_t *)p_i;
            v37 = v81;
            n4_4 = n4_10;
            p_i_1 = p_i_2;
          }
          if ( n2_2 )
          {
            if ( n2_2 == 1 )
              __sub_410310((int32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
          }
          else
          {
            __sub_410650((uint32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
          }
          __fwd_sub_403820_sub_4081E0(p_i_1, (char *)Src_1, v37, v35);
        }
      }
      while ( n4_4 < ::__n4_5 );
      goto LABEL_104;
    }
  }
LABEL_105:
  free(Src_1);
LABEL_106:
  if ( (char *)::__Buffer + ElementCount != (void *)__Buffer_0 )
  {
LABEL_107:
    fclose(*(FILE **)(v5 + 4));
    *(uint32_t *)(v5 + 4) = 0;
    return nullptr;
  }
  free(::__Buffer);
  v88 = v91;
LABEL_109:
  if ( (v88 & 0x80) != 0 )
  {
    Buffer_3 = p_i_1[10] < 0 ? &p_i_1[*((uint32_t *)p_i_1 + 3) + 16] : nullptr;
    ElementCount_4 = fread(Buffer_3, 1u, ElementCount_3, *(FILE **)(v5 + 4));
    if ( ElementCount_4 != ElementCount_3 )
      goto LABEL_107;
  }
  if ( (p_i_1[11] & 2) != 0 )
  {
    n4_6 = (char *)malloc(*((uint32_t *)p_i_1 + 3));
    __sub_403820_n4_5 = ::__n4_5;
    v55 = *((uint16_t *)p_i_1 + 1);
    n4_1 = (int32_t)n4_6;
    Src_2 = ::__n4_5 * (v55 - 1);
    memcpy(n4_6,p_i_1 + 16,*((uint32_t *)p_i_1 + 3));
    LOWORD(v58) = *((uint16_t *)p_i_1 + 1);
    if ( (uint16_t)v58 )
    {
      Src = (void *)Src_2;
      n4_7 = (char *)n4_1;
      Blocka = n4_6;
      n4_8 = 0;
      v61 = 0;
      do
      {
        i_1 = *(uint16_t *)p_i_1;
        n4_1 = n4_8;
        v86 = v61;
        Src_3 = Src;
        p_i_2 = p_i_1;
        v64 = &p_i_1[n4_8 + 16];
        do
        {
          n4_9 = __sub_403820_n4_5;
          do
          {
            *v64++ = *n4_7++;
            --n4_9;
          }
          while ( n4_9 );
          v64 = &v64[(uint32_t)Src_3];
          --i_1;
        }
        while ( i_1 );
        p_i_1 = p_i_2;
        v58 = *((uint16_t *)p_i_2 + 1);
        n4_8 = __sub_403820_n4_5 + n4_1;
        v61 = v86 + 1;
      }
      while ( v86 + 1 < v58 );
      n4_6 = Blocka;
    }
    i_2 = *(uint16_t *)p_i_1;
    *(uint16_t *)p_i_1 = v58;
    p_i_1[11] ^= 2u;
    *((uint16_t *)p_i_1 + 1) = i_2;
    *((uint16_t *)p_i_1 + 2) = v58 * __sub_403820_n4_5;
    free(n4_6);
  }
  return p_i_1;
}

static inline void __fwd_sub_4043E0_sub_405840(void *a0, void *a1, int32_t a2, char a3, const __m128 &a4, const __m128 &a5) { __sub_405840((char *)a0, (char *)a1, a2, a3, a4, a5); }
static inline void __fwd_sub_4043E0_sub_407B30(void *a0, int32_t a1, char a2, const __m128 &a3, const __m128 &a4) { __sub_407B30((uint16_t *)a0, a1, a2, a3, a4); }

BMF_SSE uint32_t __sub_4043E0(uint16_t *p_i, char a2, const __m128 &a3__ref, const __m128 &a4__ref)
{
  alignas(16) uint8_t __hexrays_frame[164];
  char * &v175 = *(char * *)(__hexrays_frame + 0);
  char * &v176 = *(char * *)(__hexrays_frame + 0);
  uint64_t (&v177)[2] = *(uint64_t (*)[2])(__hexrays_frame + 4);
  char * (&v178)[2] = *(char * (*)[2])(__hexrays_frame + 20);
  int32_t (&v179)[2] = *(int32_t (*)[2])(__hexrays_frame + 28);
  int32_t &v180 = *(int32_t *)(__hexrays_frame + 36);
  uint32_t (&v181)[4] = *(uint32_t (*)[4])(__hexrays_frame + 40);
  int32_t &v182 = *(int32_t *)(__hexrays_frame + 56);
  void * &Srca_7 = *(void * *)(__hexrays_frame + 60);
  int32_t &v184 = *(int32_t *)(__hexrays_frame + 64);
  char * &n5_1 = *(char * *)(__hexrays_frame + 68);
  void * &Src = *(void * *)(__hexrays_frame + 100);
  char * &n4_10 = *(char * *)(__hexrays_frame + 104);
  int32_t &v188 = *(int32_t *)(__hexrays_frame + 108);
  int32_t &v189 = *(int32_t *)(__hexrays_frame + 112);
  char * &n4_15 = *(char * *)(__hexrays_frame + 116);
  uint16_t * &p_i_2 = *(uint16_t * *)(__hexrays_frame + 120);
  char * &Blockb = *(char * *)(__hexrays_frame + 124);
  ;
  __m128 a3 = a3__ref;
  __m128 a4 = a4__ref;
  bool v35, v162;
  char *n4_1, v19, *v24, *v26, *v27, *n4_13, *Blockb_2, *Srca_1, *Srca_6, v44, *v46, v49, *v54,
       v62, v63, *n4_14, *v67, v70, *v74, v77, v81, v82, v86, v87, *n4_17, v93, *v101, v103,
       *v105, *Blockb_6, *v110, *v111, *v112, *Blockb_7, v116, v122, *v124, *v130, v134, *v136,
       *Blockb_8, *v141, *v142, *v143, *Blockb_9, *Blockb_4, *Srca_3, *Blockb_5, *Srca_4,
       *n0x7FFFFFFF_5, *Srca_5, *Blockb_3, *Srca_2, *Blockb_1, *Srca, *n5_6;
  int16_t v114, v115, v145, v146;
  int32_t i, i_2, i_1, i_3, n4, v10, v14, v16, v17, v18, n4_4, v21, v22, v23, Size, n0x7FFFFFFF,
          v32, n0x7FFFFFFF_8, n0x7FFFFFFF_2, n5, n2, n0x7FFFFFFF_7, v40, v41, v42, v43, v45,
          n4_2, v48, v50, v51, v52, i_4, n4_3, n16_1, n4_7, n4_8, v61, n16_2, n4_9, v69, v71,
          n16, __sub_4043E0_n4_5, v76, n4_20, n4_6, n16_3, n4_11, v85, n4_16, n16_4, n4_12,
          n4_18, v96, v98, v99, v100, v102, v104, v106, v108, v109, n4_19, v118, v119, v120,
          v121, v123, v125, v126, i_6, v133, v135, v137, v139, v140, v149, n0x7FFFFFFF_10,
          n0x7FFFFFFF_4, n5_3, v155, n0x7FFFFFFF_11, n5_4, v160, n0x7FFFFFFF_6, n5_5, v166,
          n0x7FFFFFFF_9, n0x7FFFFFFF_3, n5_2, v172, n0x7FFFFFFF_1;
  uint16_t *p_i_1, i_5, i_7, v132;
  uint32_t v13, v15, n64_2, n64_1, n64, n64_3, n64_4, v97;
  uint8_t *v55, *v128, *v129;
  p_i_1 = p_i;
  i = *p_i;
  i_2 = p_i[1];
  if ( (uint32_t)__n7_0 < 9 )
  {
    i_1 = p_i[1];
    if ( i >= i_2 )
      i_1 = i;
    i_3 = (((i_1 * (__dword_443384 + __n7_0 + 1) + i_1) >> 4) + 3) & 0xFFFFFFFC;
    if ( i_3 < (__dword_443384 + __n7_0 + 1) << 6 )
      i_3 = (__dword_443384 + __n7_0 + 1) << 6;
    if ( i_3 < i )
      i = i_3;
    if ( i_3 < i_2 )
      i_2 = i_3;
  }
  if ( i < 4 || i_2 < 3 )
  {
    if ( ::__n4_5 > 0 )
    {
      n4 = 0;
      do
      {
        v10 = 16 * n4;
        __byte_44339E[v10] = 0;
        __byte_44339D[v10] = n4;
        __byte_44339C[v10] = n4++;
      }
      while ( n4 < ::__n4_5 );
    }
    return 0;
  }
  __sub_405CF0((int32_t)p_i_1, i_2, a2);
  if ( __n2_4 == 2 )
  {
    n4_1 = (char *)::__n4_5;
    v13 = __dword_4410A4[0];
    if ( (HIWORD(__dword_4410A4[0]) & 0xF) == ::__n4_5 + 8 * __dword_443384 )
    {
      v14 = __dword_4410A4[0];
      if ( ::__n4_5 - 1 >= 0 )
      {
        v179[0] = ::__n4_5 / 2;
        if ( ::__n4_5 / 2 )
        {
          v178[0] = (char *)::__n4_5;
          v178[1] = (char *)__dword_4410A4[0];
          p_i_2 = p_i_1;
          v15 = 0;
          v16 = 4 * ::__n4_5;
          v17 = 0;
          do
          {
            ++v15;
            __byte_44339E[16 * BYTE1(__n256_2[v17 + v16])] = v14 & 0xF;
            __byte_44339E[16 * *(uint8_t *)(v17 * 4 + v16 * 4 + BMF_BLOB(0x0044337D))] = (uint8_t)v14 >> 4;
            v14 >>= 8;
            v17 -= 8;
          }
          while ( v15 < v179[0] );
          n4_1 = v178[0];
          v13 = (uint32_t)v178[1];
          p_i_1 = p_i_2;
          v18 = 2 * v15 + 1;
        }
        else
        {
          v18 = 1;
        }
        if ( (uint32_t)n4_1 > v18 - 1 )
          __byte_44339E[16 * (uint8_t)__byte_44339D[16 * (uint32_t)&n4_1[-v18]]] = v14 & 0xF;
      }
      if ( (v13 & 0x200000) != 0 )
      {
        v46 = (char *)malloc(*((uint32_t *)p_i_1 + 3));
        n4_2 = ::__n4_5;
        v48 = p_i_1[1];
        v178[1] = v46;
        v180 = ::__n4_5 * (v48 - 1);
        memcpy(v46,(char *)p_i_1 + 16,*((uint32_t *)p_i_1 + 3));
        LOWORD(v50) = p_i_1[1];
        if ( (uint16_t)v50 )
        {
          v178[0] = v46;
          v51 = 0;
          v52 = 0;
          do
          {
            i_4 = *p_i_1;
            v179[1] = v52;
            v179[0] = v51;
            v54 = v178[1];
            p_i_2 = p_i_1;
            v55 = (char *)p_i_1 + v52 + 16;
            do
            {
              n4_3 = n4_2;
              do
              {
                *v55++ = *v54++;
                --n4_3;
              }
              while ( n4_3 );
              v55 += v180;
              --i_4;
            }
            while ( i_4 );
            p_i_1 = p_i_2;
            v50 = p_i_2[1];
            v178[1] = v54;
            v52 = n4_2 + v179[1];
            v51 = v179[0] + 1;
          }
          while ( v179[0] + 1 < v50 );
          v46 = v178[0];
        }
        i_5 = *p_i_1;
        *p_i_1 = v50;
        *((uint8_t *)p_i_1 + 11) ^= 2u;
        p_i_1[1] = i_5;
        p_i_1[2] = v50 * n4_2;
        free(v46);
        v13 = __dword_4410A4[0];
      }
      return (v13 & 0x100000) >> 20;
    }
  }
  Blockb = (char *)__sub_42B830(i, i_2, p_i_1[5] & 0x3F, 0, 0);
  __ElementCount_0 = *((uint32_t *)Blockb + 3) + 0x20000;
  __Buffer = malloc(__ElementCount_0);
  __Buffer_0 = (int32_t)__Buffer;
  __n8 = 0;
  __n256 = 0;
  __Buffer_1 = (int32_t)__Buffer;
  v179[0] = i_2 * i;
  __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
  Srca_7 = malloc(i_2 * i);
  n4_4 = ::__n4_5;
  v21 = (p_i_1[1] - i_2) >> 1;
  v22 = *p_i_1 - i;
  v23 = v21 * p_i_1[2];
  v178[1] = (char *)v21;
  v24 = (char *)p_i_1 + ::__n4_5 * (v22 >> 1) + v23 + 16;
  Src = Blockb + 16;
  v178[0] = Blockb + 16;
  if ( v21 < i_2 + v21 )
  {
    Size = *((uint16_t *)Blockb + 2);
    v179[1] = i_2;
    v26 = v178[1];
    p_i_2 = p_i_1;
    v27 = v178[0];
    do
    {
      memcpy(v27,v24,Size);
      Size = *((uint16_t *)Blockb + 2);
      v27 += Size;
      v24 += p_i_2[2];
      ++v26;
    }
    while ( (int32_t)v26 < v179[1] + ((p_i_2[1] - v179[1]) >> 1) );
    p_i_1 = p_i_2;
    n4_4 = ::__n4_5;
  }
  n4_10 = nullptr;
  v188 = 0;
  v189 = 0;
  n4_15 = nullptr;
  if ( n4_4 > 0 )
  {
    p_i_2 = p_i_1;
    n4_13 = n4_10;
    n5_1 = nullptr;
    v182 = 0;
    v179[0] *= 8;
    while ( 1 )
    {
      v179[1] = (uint8_t)__byte_44339D[16 * v182];
      v180 = 16 * v179[1];
      if ( n4_13 )
      {
        n0x7FFFFFFF = 0x7FFFFFFF;
      }
      else
      {
        Blockb_1 = Blockb;
        Srca = (char *)Srca_7;
        v172 = v179[1];
        __byte_44339E[16 * v179[1]] = 0;
        __fwd_sub_4043E0_sub_405840(Blockb_1, Srca, v172, v19, a3, a4);
        n0x7FFFFFFF_1 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
        n0x7FFFFFFF = __n256_2[0] - __n8 + n0x7FFFFFFF_1 + 32;
        v35 = __dword_443384 == 0;
        *(uint32_t *)__Buffer_1 = __n256;
        if ( !v35 )
          n0x7FFFFFFF = n0x7FFFFFFF_1;
        __n8 = 0;
        __n256 = 0;
        __Buffer_0 = (int32_t)__Buffer;
        __Buffer_1 = (int32_t)__Buffer;
        n5_6 = n5_1;
        __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
        if ( n0x7FFFFFFF == 0x7FFFFFFF )
          n0x7FFFFFFF = 0x7FFFFFFF;
        else
          n5_6 = nullptr;
        n5_1 = n5_6;
      }
      Blockb_2 = Blockb;
      Srca_1 = (char *)Srca_7;
      v32 = v179[1];
      __byte_44339E[v180] = 5;
      __fwd_sub_4043E0_sub_405840(Blockb_2, Srca_1, v32, v19, a3, a4);
      n0x7FFFFFFF_8 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
      n0x7FFFFFFF_2 = __n256_2[0] - __n8 + n0x7FFFFFFF_8 + 32;
      v35 = __dword_443384 == 0;
      *(uint32_t *)__Buffer_1 = __n256;
      __Buffer_0 = (int32_t)__Buffer;
      __Buffer_1 = (int32_t)__Buffer;
      if ( !v35 )
        n0x7FFFFFFF_2 = n0x7FFFFFFF_8;
      __n8 = 0;
      __n256 = 0;
      __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
      n5 = (int32_t)n5_1;
      if ( n0x7FFFFFFF_2 < n0x7FFFFFFF )
      {
        n0x7FFFFFFF = n0x7FFFFFFF_2;
        n5 = 5;
      }
      n5_1 = (char *)n5;
      if ( n0x7FFFFFFF_2 < n0x7FFFFFFF + (n0x7FFFFFFF >> 5) || n4_13 )
      {
        Blockb_3 = Blockb;
        Srca_2 = (char *)Srca_7;
        v166 = v179[1];
        __byte_44339E[v180] = 6;
        __fwd_sub_4043E0_sub_405840(Blockb_3, Srca_2, v166, v19, a3, a4);
        n0x7FFFFFFF_9 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
        n0x7FFFFFFF_3 = __n256_2[0] - __n8 + n0x7FFFFFFF_9 + 32;
        v35 = __dword_443384 == 0;
        *(uint32_t *)__Buffer_1 = __n256;
        __Buffer_0 = (int32_t)__Buffer;
        __Buffer_1 = (int32_t)__Buffer;
        if ( !v35 )
          n0x7FFFFFFF_3 = n0x7FFFFFFF_9;
        __n8 = 0;
        __n256 = 0;
        n5_2 = (int32_t)n5_1;
        __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
        if ( n0x7FFFFFFF_3 < n0x7FFFFFFF )
        {
          n0x7FFFFFFF = n0x7FFFFFFF_3;
          n5_2 = 6;
        }
        n5_1 = (char *)n5_2;
        if ( v182 )
        {
          if ( !n4_13 || (uint32_t)__n7_0 > 5 )
          {
LABEL_191:
            Blockb_4 = Blockb;
            Srca_3 = (char *)Srca_7;
            v149 = v179[1];
            __byte_44339E[v180] = 8;
            __fwd_sub_4043E0_sub_405840(Blockb_4, Srca_3, v149, v19, a3, a4);
            n0x7FFFFFFF_10 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
            n0x7FFFFFFF_4 = __n256_2[0] - __n8 + n0x7FFFFFFF_10 + 32;
            v35 = __dword_443384 == 0;
            *(uint32_t *)__Buffer_1 = __n256;
            __Buffer_0 = (int32_t)__Buffer;
            __Buffer_1 = (int32_t)__Buffer;
            if ( !v35 )
              n0x7FFFFFFF_4 = n0x7FFFFFFF_10;
            __n8 = 0;
            __n256 = 0;
            n5_3 = (int32_t)n5_1;
            __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
            if ( n0x7FFFFFFF_4 < n0x7FFFFFFF )
            {
              n0x7FFFFFFF = n0x7FFFFFFF_4;
              n5_3 = 8;
            }
            n5_1 = (char *)n5_3;
          }
          Blockb_5 = Blockb;
          Srca_4 = (char *)Srca_7;
          v155 = v179[1];
          __byte_44339E[v180] = 13;
          __fwd_sub_4043E0_sub_405840(Blockb_5, Srca_4, v155, v19, a3, a4);
          n0x7FFFFFFF_11 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
          n0x7FFFFFFF_5 = (char *)(__n256_2[0] - __n8 + n0x7FFFFFFF_11 + 32);
          v35 = __dword_443384 == 0;
          *(uint32_t *)__Buffer_1 = __n256;
          __Buffer_0 = (int32_t)__Buffer;
          __Buffer_1 = (int32_t)__Buffer;
          if ( !v35 )
            n0x7FFFFFFF_5 = (char *)n0x7FFFFFFF_11;
          v178[0] = n0x7FFFFFFF_5;
          __n8 = 0;
          __n256 = 0;
          n5_4 = (int32_t)n5_1;
          __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
          if ( (int32_t)n0x7FFFFFFF_5 < n0x7FFFFFFF )
          {
            n0x7FFFFFFF = (int32_t)n0x7FFFFFFF_5;
            n5_4 = 13;
          }
          n5_1 = (char *)n5_4;
          n2 = n5_4 & 3;
          if ( n2 == 2 || n0x7FFFFFFF + (n0x7FFFFFFF >> 5) > (int32_t)v178[0] )
          {
            Srca_5 = (char *)Srca_7;
            v160 = v179[1];
            __byte_44339E[v180] = 14;
            __fwd_sub_4043E0_sub_405840(Blockb, Srca_5, v160, v19, a3, a4);
            n0x7FFFFFFF_6 = __n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32;
            if ( __dword_443384 )
              n0x7FFFFFFF_6 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
            v162 = n0x7FFFFFFF_6 < n0x7FFFFFFF;
            if ( n0x7FFFFFFF_6 < n0x7FFFFFFF )
              n0x7FFFFFFF = n0x7FFFFFFF_6;
            *(uint32_t *)__Buffer_1 = __n256;
            __n8 = 0;
            __n256 = 0;
            __Buffer_0 = (int32_t)__Buffer;
            __Buffer_1 = (int32_t)__Buffer;
            __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
            n5_5 = (int32_t)n5_1;
            if ( v162 )
              n5_5 = 14;
            n5_1 = (char *)n5_5;
            if ( v162 )
              n2 = 2;
          }
          goto LABEL_43;
        }
        n2 = n5_2 & 3;
      }
      else
      {
        if ( v182 )
          goto LABEL_191;
        n2 = (uint8_t)n5_1 & 3;
      }
LABEL_43:
      if ( __dword_443384 || 3 * n0x7FFFFFFF >= v179[0] )
      {
        v184 = (uint8_t)n5_1 & 8;
      }
      else
      {
        v184 = (uint8_t)n5_1 & 8;
        if ( n2 != 2 )
        {
          Srca_6 = (char *)Srca_7;
          v178[1] = (char *)((uint8_t)n5_1 & 8 | 1);
          __byte_44339E[v180] = (char)v178[1];
          __fwd_sub_4043E0_sub_405840(Blockb, Srca_6, v179[1], v19, a3, a4);
          n0x7FFFFFFF_7 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
          if ( !__dword_443384 )
            n0x7FFFFFFF_7 = __n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32;
          *(uint32_t *)__Buffer_1 = __n256;
          __n8 = 0;
          __n256 = 0;
          __Buffer_0 = (int32_t)__Buffer;
          __Buffer_1 = (int32_t)__Buffer;
          __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
          if ( n0x7FFFFFFF_7 < n0x7FFFFFFF )
          {
            n0x7FFFFFFF = n0x7FFFFFFF_7;
            n5_1 = v178[1];
            n2 = (int32_t)v178[1] & 3;
            v184 = (int32_t)v178[1] & 8;
          }
        }
      }
      n4_15 += n0x7FFFFFFF;
      v40 = n2 == 2;
      if ( n2 != 1 )
        n2 = 0;
      v41 = v180;
      n4_13 = (char *)((uintptr_t)n4_13 + (v40));
      v188 += n2;
      v42 = v184;
      v181[v179[1]] = n0x7FFFFFFF;
      v43 = v182;
      v189 += v42 != 0;
      __byte_44339E[v41] = (char)n5_1;
      v182 = v43 + 1;
      if ( v43 + 1 >= ::__n4_5 )
      {
        n4_10 = n4_13;
        p_i_1 = p_i_2;
        break;
      }
    }
  }
  if ( (uint32_t)__n7_0 > 6 )
  {
    v101 = (char *)malloc(*((uint32_t *)Blockb + 3));
    v102 = *((uint16_t *)Blockb + 1);
    v178[1] = (char *)::__n4_5;
    v180 = (int32_t)v101;
    v182 = ::__n4_5 * (v102 - 1);
    memcpy(v101,(char *)Src,*((uint32_t *)Blockb + 3));
    LOWORD(v104) = *((uint16_t *)Blockb + 1);
    if ( (uint16_t)v104 )
    {
      v178[0] = v101;
      v105 = v178[1];
      p_i_2 = p_i_1;
      v106 = 0;
      Blockb_6 = Blockb;
      v108 = 0;
      do
      {
        v109 = *(uint16_t *)Blockb_6;
        v179[1] = v108;
        v179[0] = v106;
        v110 = &Blockb_6[v108 + 16];
        v111 = (char *)v180;
        do
        {
          v112 = v105;
          do
          {
            *v110++ = *v111++;
            v112 = (char *)((uintptr_t)v112 - 1);
          }
          while ( v112 );
          v110 += v182;
          --v109;
        }
        while ( v109 );
        Blockb_6 = Blockb;
        v104 = *((uint16_t *)Blockb + 1);
        v180 = (int32_t)v111;
        v108 = (int32_t)&v105[v179[1]];
        v106 = v179[0] + 1;
      }
      while ( v179[0] + 1 < v104 );
      v101 = v178[0];
      p_i_1 = p_i_2;
    }
    Blockb_7 = Blockb;
    v114 = *(uint16_t *)Blockb;
    v115 = v104 * LOWORD(v178[1]);
    *(uint16_t *)Blockb = v104;
    *((uint16_t *)Blockb_7 + 1) = v114;
    Blockb_7[11] ^= 2u;
    *((uint16_t *)Blockb_7 + 2) = v115;
    free(v101);
    n4_19 = 0;
    if ( ::__n4_5 > 0 )
    {
      p_i_2 = p_i_1;
      v118 = 0;
      while ( 1 )
      {
        v119 = (uint8_t)__byte_44339D[16 * v118];
        v178[0] = (char *)v119;
        __fwd_sub_4043E0_sub_405840(Blockb, (char *)Srca_7, v119, v116, a3, a4);
        v120 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
        if ( !__dword_443384 )
          v120 = __n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32;
        *(uint32_t *)__Buffer_1 = __n256;
        __n8 = 0;
        n4_19 += v120;
        __n256 = 0;
        __Buffer_0 = (int32_t)__Buffer;
        __Buffer_1 = (int32_t)__Buffer;
        __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
        if ( v120 - (v120 >> 8) > v181[(int32_t)v178[0]] )
          break;
        if ( ++v118 >= ::__n4_5 )
        {
          p_i_1 = p_i_2;
          goto LABEL_172;
        }
      }
      p_i_1 = p_i_2;
      n4_19 += (int32_t)(n4_15 + 1);
    }
LABEL_172:
    if ( n4_19 + (n4_19 >> 12) >= (int32_t)n4_15 )
    {
      v178[0] = (char *)malloc(*((uint32_t *)Blockb + 3));
      v133 = ::__n4_5 * (*((uint16_t *)Blockb + 1) - 1);
      v178[1] = (char *)::__n4_5;
      v180 = (int32_t)v178[0];
      memcpy(v178[0],(char *)Src,*((uint32_t *)Blockb + 3));
      LOWORD(v135) = *((uint16_t *)Blockb + 1);
      if ( (uint16_t)v135 )
      {
        v181[0] = v133;
        v136 = v178[1];
        p_i_2 = p_i_1;
        v137 = 0;
        Blockb_8 = Blockb;
        v139 = 0;
        do
        {
          v140 = *(uint16_t *)Blockb_8;
          v179[1] = v139;
          v179[0] = v137;
          v141 = &Blockb_8[v139 + 16];
          v142 = (char *)v180;
          do
          {
            v143 = v136;
            do
            {
              *v141++ = *v142++;
              v143 = (char *)((uintptr_t)v143 - 1);
            }
            while ( v143 );
            v141 += v181[0];
            --v140;
          }
          while ( v140 );
          Blockb_8 = Blockb;
          v135 = *((uint16_t *)Blockb + 1);
          v180 = (int32_t)v142;
          v139 = (int32_t)&v136[v179[1]];
          v137 = v179[0] + 1;
        }
        while ( v179[0] + 1 < v135 );
        p_i_1 = p_i_2;
      }
      Blockb_9 = Blockb;
      v176 = v178[0];
      v145 = *(uint16_t *)Blockb;
      v146 = v135 * LOWORD(v178[1]);
      *(uint16_t *)Blockb = v135;
      *((uint16_t *)Blockb_9 + 1) = v145;
      Blockb_9[11] ^= 2u;
      *((uint16_t *)Blockb_9 + 2) = v146;
      free(v176);
    }
    else
    {
      n4_15 = (char *)n4_19;
      v178[0] = (char *)malloc(*((uint32_t *)p_i_1 + 3));
      v121 = p_i_1[1];
      v178[1] = (char *)::__n4_5;
      v179[0] = (int32_t)v178[0];
      v181[0] = ::__n4_5 * (v121 - 1);
      memcpy(v178[0],(char *)p_i_1 + 16,*((uint32_t *)p_i_1 + 3));
      LOWORD(v123) = p_i_1[1];
      if ( (uint16_t)v123 )
      {
        v124 = v178[1];
        v125 = 0;
        v126 = 0;
        do
        {
          i_6 = *p_i_1;
          v179[1] = v126;
          v180 = v125;
          v128 = (char *)p_i_1 + v126 + 16;
          p_i_2 = p_i_1;
          v129 = (uint8_t *)v179[0];
          do
          {
            v130 = v124;
            do
            {
              *v128++ = *v129++;
              v130 = (char *)((uintptr_t)v130 - 1);
            }
            while ( v130 );
            v128 += v181[0];
            --i_6;
          }
          while ( i_6 );
          v179[0] = (int32_t)v129;
          p_i_1 = p_i_2;
          v123 = p_i_2[1];
          v126 = (int32_t)&v124[v179[1]];
          v125 = v180 + 1;
        }
        while ( v180 + 1 < v123 );
      }
      v175 = v178[0];
      i_7 = *p_i_1;
      v132 = v123 * LOWORD(v178[1]);
      *p_i_1 = v123;
      p_i_1[1] = i_7;
      *((uint8_t *)p_i_1 + 11) ^= 2u;
      p_i_1[2] = v132;
      free(v175);
    }
  }
  free(Srca_7);
  if ( ::__n4_5 > 2 && __dword_443384 )
  {
    if ( v188 )
    {
      n16 = 16;
      do
      {
        v179[n16 + 1] = __n256_0[n16];
        v179[n16] = *(int32_t *)((char *)&::__n4_5 + n16 * 4);
        v178[n16 + 1] = (char *)__n512[n16];
        v74 = (char *)__n256_2[n16];
        v178[n16] = v74;
        n16 -= 4;
      }
      while ( n16 * 4 );
      if ( ::__n4_5 > 0 )
      {
        __sub_4043E0_n4_5 = 0;
        do
        {
          v76 = 16 * __sub_4043E0_n4_5;
          v77 = __byte_44339E[16 * __sub_4043E0_n4_5++] & 8 | 5;
          __byte_44339E[v76] = v77;
        }
        while ( __sub_4043E0_n4_5 < ::__n4_5 );
      }
      __fwd_sub_4043E0_sub_407B30((uint16_t *)Blockb, (int32_t)v74, v44, a3, a4);
      n4_20 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
      if ( !__dword_443384 )
        n4_20 = __n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32;
      v162 = n4_20 <= (int32_t)n4_15;
      *(uint32_t *)__Buffer_1 = __n256;
      __n8 = 0;
      __n256 = 0;
      __Buffer_0 = (int32_t)__Buffer;
      __Buffer_1 = (int32_t)__Buffer;
      __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
      if ( v162 )
      {
        n4_15 = (char *)n4_20;
        n4_6 = ::__n4_5;
        v45 = 0;
      }
      else
      {
        n64 = 64;
        do
        {
          *(uint64_t *)(n64 + BMF_BLOB(0x00443394)) = *(uint64_t *)&v179[n64 / 4];
          a3 = (__m128)*(uint64_t *)&v178[n64 / 4];
          *(uint64_t *)(n64 + BMF_BLOB(0x0044338C)) = a3.m128_u64[0];
          *(uint64_t *)(n64 + BMF_BLOB(0x00443384)) = v177[n64 / 8 + 1];
          a4 = (__m128)(uint64_t)v177[n64 / 8];
          *(uint64_t *)(n64 + BMF_BLOB(0x0044337C)) = a4.m128_u64[0];
          n64 -= 32;
        }
        while ( n64 );
        n4_6 = ::__n4_5;
        v45 = 1;
      }
      if ( n4_6 <= 2 || !__dword_443384 )
        goto LABEL_63;
    }
    else
    {
      v45 = 1;
    }
    if ( &n4_10[v45 == 0] )
    {
      n16_1 = 16;
      do
      {
        v179[n16_1 + 1] = __n256_0[n16_1];
        v179[n16_1] = *(int32_t *)((char *)&::__n4_5 + n16_1 * 4);
        v178[n16_1 + 1] = (char *)__n512[n16_1];
        v178[n16_1] = (char *)__n256_2[n16_1];
        n16_1 -= 4;
      }
      while ( n16_1 * 4 );
      n4_7 = ::__n4_5;
      if ( ::__n4_5 > 0 )
      {
        n4_8 = 0;
        do
        {
          v61 = 16 * n4_8;
          v62 = __byte_44339E[16 * n4_8++] & 8 | 6;
          __byte_44339E[v61] = v62;
          n4_7 = ::__n4_5;
        }
        while ( n4_8 < ::__n4_5 );
      }
      if ( (char *)n4_7 == n4_10 && n4_7 - 1 == v189 )
      {
        v45 = 0;
      }
      else
      {
        __fwd_sub_4043E0_sub_407B30((uint16_t *)Blockb, v45, v44, a3, a4);
        n4_14 = (char *)(8 * (__Buffer_0 - (uint32_t)__Buffer));
        if ( !__dword_443384 )
          n4_14 = (char *)(__n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32);
        v162 = (int32_t)n4_14 <= (int32_t)n4_15;
        v178[0] = n4_14;
        *(uint32_t *)__Buffer_1 = __n256;
        __n8 = 0;
        __n256 = 0;
        __Buffer_0 = (int32_t)__Buffer;
        __Buffer_1 = (int32_t)__Buffer;
        __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
        if ( v162 )
        {
          n4_15 = n4_14;
          n4_10 = n4_14;
          if ( ::__n4_5 - 1 == v189 )
          {
            v45 = 0;
          }
          else
          {
            n16_2 = 16;
            do
            {
              v179[n16_2 + 1] = __n256_0[n16_2];
              v179[n16_2] = *(int32_t *)((char *)&::__n4_5 + n16_2 * 4);
              v178[n16_2 + 1] = (char *)__n512[n16_2];
              v67 = (char *)__n256_2[n16_2];
              v178[n16_2] = v67;
              n16_2 -= 4;
            }
            while ( n16_2 * 4 );
            if ( ::__n4_5 > 0 )
            {
              n4_9 = 0;
              do
              {
                v69 = 16 * n4_9;
                v70 = __byte_44339C[16 * n4_9++];
                __byte_44339E[v69] |= 8 * (v70 != 0);
              }
              while ( n4_9 < ::__n4_5 );
            }
            __fwd_sub_4043E0_sub_407B30((uint16_t *)Blockb, (int32_t)v67, v63, a3, a4);
            v71 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
            if ( !__dword_443384 )
              v71 = __n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32;
            v162 = v71 <= (int32_t)v178[0];
            *(uint32_t *)__Buffer_1 = __n256;
            __Buffer_0 = (int32_t)__Buffer;
            __Buffer_1 = (int32_t)__Buffer;
            __n8 = 0;
            __n256 = 0;
            __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
            if ( v162 )
            {
              v45 = 0;
            }
            else
            {
              n64_1 = 64;
              do
              {
                *(uint64_t *)(n64_1 + BMF_BLOB(0x00443394)) = *(uint64_t *)&v179[n64_1 / 4];
                a3 = (__m128)*(uint64_t *)&v178[n64_1 / 4];
                *(uint64_t *)(n64_1 + BMF_BLOB(0x0044338C)) = a3.m128_u64[0];
                *(uint64_t *)(n64_1 + BMF_BLOB(0x00443384)) = v177[n64_1 / 8 + 1];
                a4 = (__m128)(uint64_t)v177[n64_1 / 8];
                *(uint64_t *)(n64_1 + BMF_BLOB(0x0044337C)) = a4.m128_u64[0];
                n64_1 -= 32;
              }
              while ( n64_1 );
              v45 = 0;
            }
          }
        }
        else
        {
          n64_2 = 64;
          do
          {
            *(uint64_t *)(n64_2 + BMF_BLOB(0x00443394)) = *(uint64_t *)&v179[n64_2 / 4];
            a3 = (__m128)*(uint64_t *)&v178[n64_2 / 4];
            *(uint64_t *)(n64_2 + BMF_BLOB(0x0044338C)) = a3.m128_u64[0];
            *(uint64_t *)(n64_2 + BMF_BLOB(0x00443384)) = v177[n64_2 / 8 + 1];
            a4 = (__m128)(uint64_t)v177[n64_2 / 8];
            *(uint64_t *)(n64_2 + BMF_BLOB(0x0044337C)) = a4.m128_u64[0];
            n64_2 -= 32;
          }
          while ( n64_2 );
        }
      }
    }
  }
  else
  {
    v45 = 1;
  }
LABEL_63:
  if ( !n4_10 && ::__n4_5 > 1 )
  {
    v178[0] = (char *)malloc(*((uint32_t *)Blockb + 3));
    memcpy(v178[0],(char *)Src,*((uint32_t *)Blockb + 3));
    n16_3 = 16;
    do
    {
      v179[n16_3 + 1] = __n256_0[n16_3];
      v179[n16_3] = *(int32_t *)((char *)&::__n4_5 + n16_3 * 4);
      v178[n16_3 + 1] = (char *)__n512[n16_3];
      v178[n16_3] = (char *)__n256_2[n16_3];
      n16_3 -= 4;
    }
    while ( n16_3 * 4 );
    if ( ::__n4_5 > 0 )
    {
      n4_11 = 0;
      do
      {
        v85 = 16 * n4_11;
        v86 = __byte_44339E[16 * n4_11++] & 0xFB;
        __byte_44339E[v85] = v86;
      }
      while ( n4_11 < ::__n4_5 );
    }
    __fwd_sub_4043E0_sub_407B30((uint16_t *)Blockb, v45, v82, a3, a4);
    n4_17 = (char *)(8 * (__Buffer_0 - (uint32_t)__Buffer));
    if ( !__dword_443384 )
      n4_17 = (char *)(__n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32);
    v178[1] = n4_17;
    *(uint32_t *)__Buffer_1 = __n256;
    __n8 = 0;
    __n256 = 0;
    __Buffer_0 = (int32_t)__Buffer;
    __Buffer_1 = (int32_t)__Buffer;
    n4_16 = (int32_t)n4_15;
    __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
    if ( (int32_t)n4_17 > (int32_t)n4_15 )
    {
      n64_3 = 64;
      do
      {
        *(uint64_t *)(n64_3 + BMF_BLOB(0x00443394)) = *(uint64_t *)&v179[n64_3 / 4];
        a3 = (__m128)*(uint64_t *)&v178[n64_3 / 4];
        *(uint64_t *)(n64_3 + BMF_BLOB(0x0044338C)) = a3.m128_u64[0];
        *(uint64_t *)(n64_3 + BMF_BLOB(0x00443384)) = v177[n64_3 / 8 + 1];
        a4 = (__m128)(uint64_t)v177[n64_3 / 8];
        *(uint64_t *)(n64_3 + BMF_BLOB(0x0044337C)) = a4.m128_u64[0];
        n64_3 -= 32;
      }
      while ( n64_3 );
    }
    else
    {
      n4_15 = n4_17;
      v45 = 0;
    }
    if ( ((n4_16 >> 3) + n4_16 > (int32_t)v178[1] || __dword_443384) && v189 + v188 )
    {
      n16_4 = 16;
      do
      {
        v179[n16_4 + 1] = __n256_0[n16_4];
        v179[n16_4] = *(int32_t *)((char *)&::__n4_5 + n16_4 * 4);
        v178[n16_4 + 1] = (char *)__n512[n16_4];
        v178[n16_4] = (char *)__n256_2[n16_4];
        n16_4 -= 4;
      }
      while ( n16_4 * 4 );
      if ( ::__n4_5 > 0 )
      {
        n4_12 = 0;
        do
          __byte_44339E[16 * n4_12++] = 0;
        while ( n4_12 < ::__n4_5 );
      }
      memcpy((char *)Src,v178[0],*((uint32_t *)Blockb + 3));
      __fwd_sub_4043E0_sub_407B30((uint16_t *)Blockb, v45, v93, a3, a4);
      n4_18 = 8 * (__Buffer_0 - (uint32_t)__Buffer);
      if ( !__dword_443384 )
        n4_18 = __n256_2[0] - __n8 + 8 * (__Buffer_0 - (uint32_t)__Buffer) + 32;
      v162 = n4_18 <= (int32_t)n4_15;
      *(uint32_t *)__Buffer_1 = __n256;
      __n8 = 0;
      __n256 = 0;
      __Buffer_0 = (int32_t)__Buffer;
      __Buffer_1 = (int32_t)__Buffer;
      __buf[0] = (char *)__Buffer + __ElementCount_0 - 4096;
      if ( v162 )
      {
        v45 = 0;
      }
      else
      {
        n64_4 = 64;
        do
        {
          *(uint64_t *)(n64_4 + BMF_BLOB(0x00443394)) = *(uint64_t *)&v179[n64_4 / 4];
          *(uint64_t *)(n64_4 + BMF_BLOB(0x0044338C)) = *(uint64_t *)&v178[n64_4 / 4];
          *(uint64_t *)(n64_4 + BMF_BLOB(0x00443384)) = v177[n64_4 / 8 + 1];
          *(uint64_t *)(n64_4 + BMF_BLOB(0x0044337C)) = v177[n64_4 / 8];
          n64_4 -= 32;
        }
        while ( n64_4 );
      }
    }
    free(v178[0]);
  }
  free(__Buffer);
  free(Blockb);
  if ( __n2_4 == 1 )
  {
    if ( ::__n4_5 <= 0 )
    {
      v96 = 0;
    }
    else
    {
      v96 = 0;
      if ( ::__n4_5 / 2 )
      {
        v178[0] = (char *)v45;
        p_i_2 = p_i_1;
        v97 = 0;
        do
        {
          v98 = (uint8_t)__byte_44339E[16 * (uint8_t)__byte_44339D[32 * v97]];
          v99 = 16 * (uint8_t)__byte_4433AD[32 * v97++];
          v96 = (uint8_t)__byte_44339E[v99] | (16 * (v98 | (16 * v96)));
        }
        while ( v97 < ::__n4_5 / 2 );
        v45 = (int32_t)v178[0];
        p_i_1 = p_i_2;
        v100 = 2 * v97 + 1;
      }
      else
      {
        v100 = 1;
      }
      if ( ::__n4_5 > (uint32_t)(v100 - 1) )
        v96 = (uint8_t)__byte_44339E[16 * BYTE1(__n256_2[4 * v100])] | (16 * v96);
    }
    __dword_4410A4[0] = (::__n4_5 << 16)
                 | (__dword_443384 << 19)
                 | (v45 << 20)
                 | ((uint8_t)(*((uint8_t *)p_i_1 + 11) & 2) >> 1 << 21)
                 | v96;
  }
  return v45;
}

static inline char * __fwd_sub_402EF0_sub_403820(int32_t a0, const __m128 &a1, const __m128 &a2, int32_t a3, void *a4) { return __sub_403820(a0, a1, a2, a3, (int32_t *)a4); }

BMF_SSE int32_t __sub_402EF0(uint32_t *v2, char *FileName, int32_t a2)
{
  ;
  FILE *Stream_v, *Stream_1;
  __m128 v3, v4;
  const char *a_b;
  int32_t v5, v8, v9;
  v5 = (int32_t)v2;
  a_b = "a+b";
  if ( a2 )
    a_b = "rb";
  *v2 = 0;
  Stream_v = fopen(FileName, a_b);
  *(uint32_t *)(v5 + 4) = Stream_v;
  if ( !Stream_v )
    __exit_402E40(6, FileName);
  *(uint32_t *)v5 = 0;
  v8 = fseek(Stream_v, 0, 0);
  v9 = *(uint32_t *)(v5 + 4);
  if ( v8 )
  {
LABEL_10:
    if ( !v9 )
LABEL_11:
      __exit_402E40(3, FileName);
    return v5;
  }
  if ( !v9 )
    goto LABEL_11;
  if ( !feof(*(FILE **)(v5 + 4)) )
  {
    __fwd_sub_402EF0_sub_403820(v5, v3, v4, 1, nullptr);
    Stream_1 = *(FILE **)(v5 + 4);
    if ( !Stream_1 )
      goto LABEL_11;
    if ( !feof(*(FILE **)(v5 + 4)) )
    {
      *(uint32_t *)v5 = 0;
      fseek(Stream_1, 0, 0);
      v9 = *(uint32_t *)(v5 + 4);
      goto LABEL_10;
    }
  }
  return v5;
}

static inline char * __fwd_sub_402FE0_sub_403820(int32_t a0, const __m128 &a1, const __m128 &a2, int32_t a3, void *a4) { return __sub_403820(a0, a1, a2, a3, (int32_t *)a4); }
static inline uint32_t __fwd_sub_402FE0_sub_4043E0(void *a0, char a1, const __m128 &a2, const __m128 &a3) { return __sub_4043E0((uint16_t *)a0, a1, a2, a3); }
static inline void __fwd_sub_402FE0_sub_405840(void *a0, void *a1, int32_t a2, char a3, const __m128 &a4, const __m128 &a5) { __sub_405840((char *)a0, (char *)a1, a2, a3, a4, a5); }
static inline void __fwd_sub_402FE0_sub_407B30(void *a0, int32_t a1, char a2, const __m128 &a3, const __m128 &a4) { __sub_407B30((uint16_t *)a0, a1, a2, a3, a4); }
static inline void __fwd_sub_402FE0_sub_415380(const __m128 &a0, const __m128 &a1, void *a2, void *a3, void *a4) { __sub_415380(a0, a1, (uint16_t *)a2, (uint8_t *)a3, (uint8_t *)a4); }

BMF_SSE int32_t __sub_402FE0(int32_t a1, const __m128 &a2__ref, const __m128 &a3__ref, uint16_t *p_i, void *__Buffer)
{
  alignas(16) uint8_t __hexrays_frame[80];
  char * &Buffera_4 = *(char * *)(__hexrays_frame + 0);
  uint32_t &ElementCount = *(uint32_t *)(__hexrays_frame + 16);
  int32_t &ElementCounta = *(int32_t *)(__hexrays_frame + 16);
  uint32_t &ElementCountb = *(uint32_t *)(__hexrays_frame + 16);
  char * &Buffera = *(char * *)(__hexrays_frame + 20);
  int32_t &n4_2 = *(int32_t *)(__hexrays_frame + 24);
  char * &Buffera_1 = *(char * *)(__hexrays_frame + 28);
  int32_t &v64 = *(int32_t *)(__hexrays_frame + 32);
  int32_t &v65 = *(int32_t *)(__hexrays_frame + 36);
  int32_t &v66 = *(int32_t *)(__hexrays_frame + 40);
  void * &Buffer_2 = *(void * *)(__hexrays_frame + 44);
  ;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  FILE *i;
  bool v38;
  char __sub_402FE0_Buffer_1, *Buffera_5, v12, v13, v15, v17, *Buffera_6, v21, *Srca, v36, v42,
       *Buffera_2, *Buffera_3;
  int32_t v5, __sub_402FE0_n4_5, v11, v18, n4_6, __sub_402FE0_n8, n4, v27, n8_1,
          __sub_402FE0_n256, n4_1, v40, v41, v43, n4_3, v47, i_1, v50, n4_4, v56;
  uint16_t *p_i_1, i_2, v53;
  uint32_t ElementCount_1, n7, v25, v26, v28, v30, v31, v32, Size, v55;
  uint8_t v39, *v49, v54;
  v5 = a1;
  if ( !*(uint32_t *)(a1 + 4) )
    return 0;
  if ( !feof(*(FILE **)(a1 + 4)) )
  {
    __fwd_sub_402FE0_sub_403820(v5, a2, a3, 1, nullptr);
    for ( i = *(FILE **)(v5 + 4); i; i = *(FILE **)(v5 + 4) )
    {
      if ( feof(*(FILE **)(v5 + 4)) )
        break;
      if ( feof(i) )
        break;
      __fwd_sub_402FE0_sub_403820(v5, a2, a3, 1, nullptr);
    }
  }
  __sub_402FE0_Buffer_1 = (char)__Buffer;
  p_i_1 = p_i;
  __sub_402FE0_n4_5 = *((uint32_t *)p_i + 1);
  if ( __Buffer )
    __sub_402FE0_Buffer_1 = 1;
  Buffera = *(char **)p_i;
  *((uint8_t *)p_i + 11) |= __sub_402FE0_Buffer_1 << 7;
  Buffera_5 = *((char **)p_i + 2);
  v11 = *((uint32_t *)p_i + 3);
  n4_2 = __sub_402FE0_n4_5;
  __n512[0] = 512;
  v12 = *((uint8_t *)p_i + 10);
  __n256_0[0] = 0;
  Buffera_1 = Buffera_5;
  v64 = v11;
  ::__n4_5 = ((v12 & 0x3Fu) + 7) >> 3;
  if ( fwrite("\x81\x8A""20\x81\x90""20a+b", 4u, 1u, *(FILE **)(v5 + 4)) != 1 )
    return 0;
  v15 = *((uint8_t *)p_i + 10);
  ++*(uint32_t *)v5;
  ElementCount_1 = v15 & 0x80;
  if ( v15 < 0 )
    ElementCount_1 = 3 << (v15 & 31);
  if ( !__dword_441098 || *((uint32_t *)p_i + 3) < 0x10u )
    goto LABEL_76;
  __dword_443384 = __dword_441090;
  v17 = *((uint8_t *)p_i + 10);
  HIBYTE(Buffera_1) |= (uint8_t)(4 * __dword_441090) | 0x20;
  if ( (v17 & 0x3Fu) <= 4 || !__dword_44108C )
  {
    __ElementCount_0 = *((uint32_t *)p_i + 3) + 0x20000;
    ::__Buffer = malloc(__ElementCount_0);
    ::__n8 = 0;
    ::__n256 = 0;
    __Buffer_0 = (int32_t)::__Buffer;
    ::__Buffer_1 = (int32_t)::__Buffer;
    __buf[0] = (char *)::__Buffer + __ElementCount_0 - 4096;
    __n2 = 0;
    __dword_443364 = 0;
    __dword_443388 = 0;
    if ( __dword_443384 )
      __fwd_sub_402FE0_sub_415380(a2, a3, p_i, (uint8_t *)p_i + 16, (uint8_t *)p_i + 16);
    else
      __sub_408510((int32_t)(p_i + 8), *((uint32_t *)p_i + 3), p_i[2]);
    goto LABEL_57;
  }
  ElementCount = __fwd_sub_402FE0_sub_4043E0(p_i, v13, a2, a3);
  HIBYTE(Buffera_1) |= 0x10u;
  if ( (*((uint8_t *)p_i + 11) & 2) != 0 )
  {
    n4_6 = *((uint32_t *)p_i + 1);
    Buffera_6 = *((char **)p_i + 2);
    Buffera = *(char **)p_i;
    v18 = *((uint32_t *)p_i + 3);
    n4_2 = n4_6;
    Buffera_1 = Buffera_6;
    v64 = v18;
    HIBYTE(Buffera_1) = (4 * __dword_443384) | 0x30 | HIBYTE(Buffera_6);
  }
  else
  {
    v18 = *((uint32_t *)p_i + 3);
  }
  __ElementCount_0 = v18 + 0x20000;
  ::__Buffer = malloc(v18 + 0x20000);
  __Buffer_0 = (int32_t)::__Buffer;
  ::__n8 = 0;
  ::__n256 = 0;
  ::__Buffer_1 = (int32_t)::__Buffer;
  __buf[0] = (char *)::__Buffer + __ElementCount_0 - 4096;
  if ( ::__n4_5 == 1 )
  {
    if ( (p_i[5] & 0x40) != 0 )
    {
LABEL_22:
      n7 = __n7_1;
      __n256_0[0] = __n7_1;
      if ( ::__n8 < 4 )
      {
        *(uint32_t *)::__Buffer_1 = ::__n256 | (2 * (__n7_1 << ((31 - ::__n8) & 31)));
        ::__Buffer_1 = __Buffer_0;
        ::__n256 = n7 >> (::__n8 & 31);
        ::__n8 += 28;
        __Buffer_0 += 4;
      }
      else
      {
        ::__n256 |= __n7_1 << (-(char)::__n8 & 31);
        ::__n8 -= 4;
      }
    }
  }
  else if ( ::__n4_5 > 2 )
  {
    goto LABEL_22;
  }
  __dword_443388 = 0;
  if ( ::__n4_5 > 0 )
  {
    __sub_402FE0_n8 = ::__n8;
    v66 = v5;
    n4 = 0;
    do
    {
      v25 = (4 * (uint8_t)__byte_44339E[16 * n4]) | (uint8_t)__byte_44339C[16 * n4];
      if ( __sub_402FE0_n8 < 6 )
      {
        *(uint32_t *)::__Buffer_1 = ::__n256 | (2 * (v25 << ((31 - __sub_402FE0_n8) & 31)));
        ::__Buffer_1 = __Buffer_0;
        __Buffer_0 += 4;
        __sub_402FE0_n8 = ::__n8 + 26;
        ::__n256 = v25 >> (::__n8 & 31);
      }
      else
      {
        ::__n256 |= v25 << (-(char)__sub_402FE0_n8 & 31);
        __sub_402FE0_n8 = ::__n8 - 6;
      }
      ::__n8 = __sub_402FE0_n8;
      if ( (__byte_44339E[16 * n4] & 8) != 0 )
      {
        v26 = (uint8_t)__byte_44339F[16 * n4];
        if ( __sub_402FE0_n8 < 8 )
        {
          *(uint32_t *)::__Buffer_1 = ::__n256 | (2 * (v26 << ((31 - __sub_402FE0_n8) & 31)));
          ::__Buffer_1 = __Buffer_0;
          __Buffer_0 += 4;
          __sub_402FE0_n8 = ::__n8 + 24;
          ::__n256 = v26 >> (::__n8 & 31);
        }
        else
        {
          v27 = v26 << (-(char)__sub_402FE0_n8 & 31);
          __sub_402FE0_n8 -= 8;
          ::__n256 |= v27;
        }
        ::__n8 = __sub_402FE0_n8;
        if ( (uint8_t)__byte_44339C[16 * n4] > 1u )
        {
          v28 = __dword_4433A0[4 * n4] + 64;
          if ( __sub_402FE0_n8 < 8 )
          {
            *(uint32_t *)::__Buffer_1 = ::__n256 | (2 * (v28 << ((31 - __sub_402FE0_n8) & 31)));
            ::__Buffer_1 = __Buffer_0;
            __Buffer_0 += 4;
            n8_1 = ::__n8 + 24;
            ::__n256 = v28 >> (::__n8 & 31);
          }
          else
          {
            ::__n256 |= v28 << (-(char)__sub_402FE0_n8 & 31);
            n8_1 = ::__n8 - 8;
          }
          ::__n8 = n8_1;
          v30 = __dword_4433A4[4 * n4] + 64;
          if ( n8_1 < 8 )
          {
            *(uint32_t *)::__Buffer_1 = ::__n256 | (2 * (v30 << ((31 - n8_1) & 31)));
            ::__Buffer_1 = __Buffer_0;
            __Buffer_0 += 4;
            __sub_402FE0_n8 = ::__n8 + 24;
            ::__n256 = v30 >> (::__n8 & 31);
          }
          else
          {
            v31 = v30 << (-(char)n8_1 & 31);
            __sub_402FE0_n8 = n8_1 - 8;
            ::__n256 |= v31;
          }
          ::__n8 = __sub_402FE0_n8;
          if ( (uint8_t)__byte_44339C[16 * n4] > 2u )
          {
            v32 = __dword_4433A8[4 * n4] + 64;
            if ( __sub_402FE0_n8 < 8 )
            {
              *(uint32_t *)::__Buffer_1 = ::__n256 | (2 * (v32 << ((31 - __sub_402FE0_n8) & 31)));
              ::__Buffer_1 = __Buffer_0;
              __Buffer_0 += 4;
              __sub_402FE0_n256 = v32 >> (::__n8 & 31);
              __sub_402FE0_n8 = ::__n8 + 24;
              ::__n8 += 24;
              ::__n256 = __sub_402FE0_n256;
            }
            else
            {
              ::__n256 |= v32 << (-(char)__sub_402FE0_n8 & 31);
              __sub_402FE0_n8 = ::__n8 - 8;
              ::__n8 -= 8;
            }
          }
        }
      }
      ++n4;
    }
    while ( n4 < ::__n4_5 );
    v5 = v66;
  }
  if ( ElementCount )
  {
    Size = *p_i * p_i[1];
    HIBYTE(Buffera_1) |= 8u;
    Srca = (char *)malloc(Size);
    if ( ::__n4_5 > 0 )
    {
      v66 = v5;
      n4_1 = 0;
      do
        __fwd_sub_402FE0_sub_405840((char *)p_i, Srca, (uint8_t)__byte_44339D[16 * n4_1++], v36, a2, a3);
      while ( n4_1 < ::__n4_5 );
      v5 = v66;
    }
    free(Srca);
  }
  else
  {
    __fwd_sub_402FE0_sub_407B30(p_i, (int32_t)p_i, v21, a2, a3);
  }
LABEL_57:
  *(uint32_t *)::__Buffer_1 = ::__n256;
  v38 = (uint32_t)(__Buffer_0 - (uint32_t)::__Buffer) < *((uint32_t *)p_i + 3);
  ElementCounta = __Buffer_0 - (uint32_t)::__Buffer;
  v64 = __Buffer_0 - (uint32_t)::__Buffer;
  if ( v38 )
  {
    v39 = fwrite(&Buffera, 1u, 0x10u, *(FILE **)(v5 + 4)) == 16;
    if ( __Buffer )
      v39 &= fwrite(__Buffer, 1u, *((uint32_t *)__Buffer + 1) + 8, *(FILE **)(v5 + 4)) == *((uint32_t *)__Buffer + 1) + 8;
    v40 = (fwrite(::__Buffer, 1u, ElementCounta, *(FILE **)(v5 + 4)) == ElementCounta) & v39;
    free(::__Buffer);
    if ( v40 && (p_i[5] & 0x80) != 0 )
      fwrite((char *)p_i + *((uint32_t *)p_i + 3) + 16, 1u, ElementCount_1, *(FILE **)(v5 + 4));
    fflush(*(FILE **)(v5 + 4));
    if ( v40 )
      return v64;
    return v40;
  }
  free(::__Buffer);
  if ( (*((uint8_t *)p_i + 11) & 2) != 0 )
  {
    Buffera = (char *)malloc(*((uint32_t *)p_i + 3));
    v41 = p_i[1];
    n4_2 = ::__n4_5;
    Buffera_1 = Buffera;
    v64 = ::__n4_5 * (v41 - 1);
    Buffer_2 = p_i + 8;
    memcpy(Buffera,(char *)p_i + 16,*((uint32_t *)p_i + 3));
    LOWORD(v43) = p_i[1];
    if ( (uint16_t)v43 )
    {
      n4_3 = n4_2;
      ElementCountb = ElementCount_1;
      Buffera_2 = Buffera_1;
      Buffera_3 = nullptr;
      v47 = 0;
      v66 = v5;
      do
      {
        i_1 = *p_i_1;
        v65 = v47;
        Buffera_1 = Buffera_3;
        v49 = (char *)p_i_1 + v47 + 16;
        v50 = v64;
        do
        {
          n4_4 = n4_3;
          do
          {
            *v49++ = *Buffera_2++;
            --n4_4;
          }
          while ( n4_4 );
          v49 += v50;
          --i_1;
        }
        while ( i_1 );
        p_i_1 = p_i;
        v43 = p_i[1];
        v47 = n4_3 + v65;
        Buffera_3 = Buffera_1 + 1;
      }
      while ( (int32_t)(Buffera_1 + 1) < v43 );
      ElementCount_1 = ElementCountb;
      v5 = v66;
    }
    Buffera_4 = Buffera;
    i_2 = *p_i_1;
    v53 = v43 * n4_2;
    *p_i_1 = v43;
    p_i_1[1] = i_2;
    *((uint8_t *)p_i_1 + 11) ^= 2u;
    p_i_1[2] = v53;
    free(Buffera_4);
    goto LABEL_77;
  }
LABEL_76:
  Buffer_2 = p_i + 8;
LABEL_77:
  v54 = fwrite(p_i_1, 1u, 0x10u, *(FILE **)(v5 + 4)) == 16;
  if ( __Buffer )
    v54 &= fwrite(__Buffer, 1u, *((uint32_t *)__Buffer + 1) + 8, *(FILE **)(v5 + 4)) == *((uint32_t *)__Buffer + 1) + 8;
  v55 = fwrite(Buffer_2, 1u, ElementCount_1 + *((uint32_t *)p_i_1 + 3), *(FILE **)(v5 + 4));
  v56 = *((uint32_t *)p_i_1 + 3);
  if ( (v54 & (v55 == v56 + ElementCount_1)) == 0 )
    return 0;
  return v56;
}
static inline int32_t * __fwd_bmf_sub_42AB20(void *a0) { return __sub_42AB20((char *)a0); }
static inline int32_t __fwd_bmf_sub_402EF0(void *a0, void *a1, int32_t a2) { return __sub_402EF0((uint32_t *)a0, (char *)a1, a2); }
static inline int32_t __fwd_bmf_sub_402FE0(int32_t a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __sub_402FE0(a0, a1, a2, (uint16_t *)a3, (void *)a4); }
static inline char * __fwd_bmf_sub_403820(int32_t a0, const __m128 &a1, const __m128 &a2, int32_t a3, void *a4) { return __sub_403820(a0, a1, a2, a3, (int32_t *)a4); }
static inline int32_t __fwd_bmf_sub_42B0C0(int32_t a0, void *a1, int32_t a2) { return __sub_42B0C0(a0, (char *)a1, a2); }
static inline FILE ** __fwd_bmf_sub_402DF0(void *a0, char a1) { return __sub_402DF0((FILE **)a0, a1); }

// ---------------------------------------------------------------------------
// The two things this program does.
//
// BMF's own driver, sub_4015C0, sniffed the first four bytes of its argument
// to choose between six readers, derived the output name from the input's by
// swapping the extension, and picked the writer from a switch.  The command
// line is now
//
//     bmf c input.bmp output      compress a BMP into a BMF stream
//     bmf d input output.bmp      expand a BMF stream back into a BMP
//
// which names both files and fixes both formats, so what is left is one
// reader and one writer.  Everything the other five readers and the other two
// writers reached went with them; so did the wildcard walk, the .ini, the
// switch parser, the -O output name, the -D delete, the unique-name counter
// and the stored-file member handling.
// ---------------------------------------------------------------------------

// Compress InName, which must be a BMP, into a BMF stream named OutName.
BMF_SSE void __bmf_compress(const __m128 &a1__ref, const __m128 &a2__ref,
                            const char *InName, const char *OutName)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  FILE *Stream_v;
  const uint8_t *Palette;
  int32_t *p_i, Arc, Flags, Colours, Step, Grey, i;
  uint32_t Size;

  // The reader below answers "no" the same way whether the file is missing or
  // is not a BMP, and BMF told those apart -- so open it once first, as its
  // format sniffer used to, and keep the two messages.
  Stream_v = fopen(InName, "rb");
  if ( !Stream_v )
    __exit_402E40(6, InName);
  fclose(Stream_v);
  p_i = __fwd_bmf_sub_42AB20((void *)InName);
  if ( !p_i )
    __exit_402E40(4);
  printf(
    "File %16s, image %dx%dx%d, size - %d:",
    InName,
    *(uint16_t *)p_i,
    *((uint16_t *)p_i + 1),
    *((uint8_t *)p_i + 10) & 0x3F,
    p_i[3]);
  if ( void *__nb = malloc(8u) )
    Arc = __fwd_bmf_sub_402EF0((uint32_t *)__nb, (void *)OutName, 0);
  else
    Arc = 0;

  // A palette that is nothing but a grey ramp carries no information: drop it
  // (bit 0x80) and mark the image greyscale (bit 0x40) instead.  This is the
  // donor's, unchanged -- it decides what goes into the stream.
  Flags = *((uint8_t *)p_i + 10);
  if ( (Flags & 0x80) != 0 )
  {
    if ( (Flags & 0x40) != 0 )
    {
      *((uint8_t *)p_i + 10) = Flags ^ 0x80;
    }
    else
    {
      Colours = 1 << (Flags & 31);
      Step = 0x100u >> (Flags & 31);
      Palette = (const uint8_t *)p_i + p_i[3] + 16;
      Grey = 0;
      for ( i = 0; i < Colours; ++i )
      {
        if ( Palette[3 * i] != Grey || Palette[3 * i + 1] != Grey || Palette[3 * i + 2] != Grey )
          break;
        Grey += Step;
      }
      if ( i >= Colours )
        *((uint8_t *)p_i + 10) = (Flags | 0x40) ^ 0x80;
    }
  }

  Size = __fwd_bmf_sub_402FE0(Arc, a1, a2, (uint16_t *)p_i, (void *)__dwLowDateTime);
  if ( !Size )
    __exit_402E40(5, OutName);
  printf(
    "%6.3f bpp\n",
    (double)Size * 8.0 / (double)(*((uint16_t *)p_i + 1) * *(uint16_t *)p_i));
  free(p_i);
}

// Expand the BMF stream InName into a BMP named OutName.
BMF_SSE void __bmf_decompress(const __m128 &a1__ref, const __m128 &a2__ref,
                              const char *InName, const char *OutName)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  int32_t Number, Depth;
  uint32_t *p_i;
  void *Block;

  if ( void *__nb = malloc(8u) )
    Block = (void *)__fwd_bmf_sub_402EF0((uint32_t *)__nb, (void *)InName, 1);
  else
    Block = nullptr;
  printf("File %16s,\r", InName);
  Number = 0;
  while ( 1 )
  {
    p_i = (uint32_t *)__fwd_bmf_sub_403820((int32_t)Block, a1, a2, 0, (void *)&__dwLowDateTime);
    if ( !p_i )
    {
      printf("\n");
      if ( !*((uint32_t *)Block + 1) )
        __exit_402E40(3, InName);
      __fwd_bmf_sub_402DF0((FILE **)Block, 1);
      return;
    }
    ++Number;
    printf(
      "File %16s, image %dx%dx%d, size - %d, number: %d\r",
      InName,
      *(uint16_t *)p_i,
      *((uint16_t *)p_i + 1),
      *((uint8_t *)p_i + 10) & 0x3F,
      p_i[3],
      Number);
    // BMF sent 2, 15 and 16 bits per pixel to its TGA writer, because a BMP
    // cannot hold them.  The output is a BMP now, so those streams have no
    // answer here and saying so beats writing a file that is not one.
    Depth = *((uint8_t *)p_i + 10) & 0x3F;
    if ( Depth == 2 || Depth == 15 || Depth == 16 )
    {
      printf("\n%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
      exit(5);
    }
    if ( !__fwd_bmf_sub_42B0C0((int32_t)p_i, (void *)OutName, __dword_441098) )
      __exit_402E40(5, OutName);
    free(__dwLowDateTime);
    __dwLowDateTime = 0;
    free(p_i);
  }
}

 BMF_SSE int32_t __main(int32_t argc, const char **argv)
{
  ;
  __m128 v3, v4;
  int32_t Mode;

  bmf_set_denormal_mode();
  __sub_42CBB0((int32_t)__sub_402E30);
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");

  Mode = argc == 4 && !argv[1][1] ? toupper(argv[1][0]) : 0;
  if ( Mode != 'C' && Mode != 'D' )
  {
    printf(
      "e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
      "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
      "       bmf d input output.bmp     expand\n");
    return 1;
  }

  // BMF read these from its .ini and then from the switches; there are no
  // switches now, so the compression mode is set here and the rest keep the
  // values BMF.exe's data segment starts them at -- filters on, near-lossless
  // error 0, no filter template, packed output.
  __dword_441090 = 1;               // -S, slow but efficient
  __n7_0 = 9;                       // -Q9, filter selection quality

  if ( Mode == 'C' )
    __bmf_compress(v3, v4, argv[2], argv[3]);
  else
    __bmf_decompress(v3, v4, argv[2], argv[3]);
  return 0;
}
void __sub_402E30() { __exit_402E40(7); }
int32_t main(int32_t argc, char **argv) {
  bmf_blob_relocate();
  __sub_419680();
  return __main(argc, (const char **)argv);
}
