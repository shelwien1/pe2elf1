#include "blob2.inc"

char b24[16] = {-3,-2,-1,-1,-1,0,0,0,0,0,0,0,2,3,4,0};                                              
char b10[36] = {2,3,3,3,4,4,4,5,5,6,6,6,7,7,7,7,8,8,8,8,8,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10};  
char b17[16] = {10,-19,-24,17,34,27,2,5,2,31,20,-16,17,30,0,0};                                     
char b18[16] = {45,73,84,90,116,122,121,126,-106,-109,-100,-75,-96,-38,0,0};                        
char b20[16] = {5,-4,12,7,47,0,-3,1,16,25,5,11,-2,0,0,0};                                           
char b21[24] = {1,2,5,1,2,5,0,5,20,18,24,24,39,44,64,65,77,90,99,99,124,-119,-82,-53};              
char b22[8]  = {-128,-64,-56,-32,-40,0,0,0};                                                        
char b23[28] = {76,-118,-122,-95,-70,0,0,0,0,0,0,0,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};

#define I128(hi, lo) ( (((unsigned __int128)(0x ## hi ## ULL)) << 64) | ((unsigned __int128)(0x ## lo ## ULL)) )
__int128 x2  = I128(3030303030303030,3030303030303030);
__int128 x3  = I128(4040404040404040,4040404040404040);
__int128 x14 = I128(0000000400000004,0000000400000004);
__int128 x15 = I128(0000000300000002,0000000100000000);
__int128 x16 = I128(0000000300000003,0000000300000003);
__int128 x17 = I128(0000000800000008,0000000800000008);
__int128 x18 = I128(0000000100000001,0000000100000001);
__int128 x19 = I128(0000000200000002,0000000200000002);
__int128 x8  = I128(8080808080808080,8080808080808080);
__int128 x11 = I128(0606060606060606,0606060606060606);
__int128 x25 = I128(5555555555555555,5555555555555555);

char b3[16] = {1,2,3,4,5,6,8,11,13,20,50,68,117,0,0,0};
char b5[16] = {0,1,3,10,14,27,44,65,88,125,-87,-42,-15,0,0,0};
char b7[8]  = {1,2,7,17,0,0,0,0};
char b9[8]  = {1,13,43,-74,0,0,0,0};
char b41[32]= {17,63,68,26,110,20,119,24,110,20,-85,10,-84,25,-126,22,105,74,60,84,44,92,23,100,44,121,0,0,0,0,0,0};
char* b42 = &b41[1];

//__int64& q7 = *(__int64*)(blob1+ 0x140025600 -0x1400227B0);
__int64 q7;

//typedef char (*t_off_140027060)[4096]; t_off_140027060& __off_140027060 = *(t_off_140027060*)(blob1+ 0x140027060 -0x1400227B0);
char (*__off_140027060)[4096];

typedef int t_dword_1400285E0[1];      t_dword_1400285E0& __dword_1400285E0 = *(t_dword_1400285E0*)(blob1+ 0x1400285E0 -0x1400227B0);
typedef int t_dword_1400285E8[1];      t_dword_1400285E8& __dword_1400285E8 = *(t_dword_1400285E8*)(blob1+ 0x1400285E8 -0x1400227B0);
typedef int t_dword_1400285EC[1];      t_dword_1400285EC& __dword_1400285EC = *(t_dword_1400285EC*)(blob1+ 0x1400285EC -0x1400227B0);
typedef int t_dword_1400285F0[521];    t_dword_1400285F0& __dword_1400285F0 = *(t_dword_1400285F0*)(blob1+ 0x1400285F0 -0x1400227B0);

typedef int t_arch_hdr[16]; t_arch_hdr& __arch_hdr = *(t_arch_hdr*)(blob1+ 0x140028E50 -0x1400227B0);
char& b11 = *(char*)(blob1+ 0x140028EFF -0x1400227B0);

typedef __int128 t_xmmword_140028F00[13];t_xmmword_140028F00& x4 = *(t_xmmword_140028F00*)(blob1+ 0x140028F00 -0x1400227B0);
__int128* x5 = &x4[1];
__int128* x6 = &x4[2];
__int128* x7 = &x4[3];

typedef char t_byte_140029000[256];   t_byte_140029000& b1 = *(t_byte_140029000*)(blob1+ 0x140029000 -0x1400227B0);
//char b1[256];

typedef __int128 t_xmmword_140029100[13];t_xmmword_140029100& x9 = *(t_xmmword_140029100*)(blob1+ 0x140029100 -0x1400227B0);
__int128* x13 = &x9[1];
__int128* x10 = &x9[2];
__int128* x12 = &x9[3];

//typedef __int128 t_xmmword_140029200[4]; t_xmmword_140029200& x20 = *(t_xmmword_140029200*)(blob1+ 0x140029200 -0x1400227B0);
__int128 x20[4];

typedef char t_byte_140029240[128]; t_byte_140029240& b2 = *(t_byte_140029240*)(blob1+ 0x140029240 -0x1400227B0);
char* b40 = &b2[126];
typedef char t_byte_1400292C0[256]; t_byte_1400292C0& b4 = *(t_byte_1400292C0*)(blob1+ 0x1400292C0 -0x1400227B0);
typedef char t_byte_1400293C0[128]; t_byte_1400293C0& b6 = *(t_byte_1400293C0*)(blob1+ 0x1400293C0 -0x1400227B0);
typedef char t_byte_140029440[256];   t_byte_140029440& b8 = *(t_byte_140029440*)(blob1+ 0x140029440 -0x1400227B0);

//typedef __int128 t_xmmword_140029500[65]; t_xmmword_140029500& x24 = *(t_xmmword_140029500*)(blob1+ 0x140029500 -0x1400227B0);
__int128 x24[65];
__int128* x23 = &x24[1];
__int128* x22 = &x24[2];
__int128* x21 = &x24[3];

// blob ends at 29560

//typedef int t_dword_140029540[256]; t_dword_140029540& d94 = *(t_dword_140029540*)(blob1+ 0x140029540 -0x1400227B0);
int d94[256];

// memset((int*)b12, 0, 0x2358D0);
//typedef char t_byte_140029940[0x40818]; t_byte_140029940& b12 = *(t_byte_140029940*)(blob1+ 0x140029940 -0x1400227B0);
typedef char t_byte_140029940[0x2358D0]; t_byte_140029940& b12 = *(t_byte_140029940*)(blob1+ 0x140029940 -0x1400227B0);
__int64& q12 = *(__int64*)(blob1+ 0x14002A150 -0x1400227B0);

typedef char t_byte_14002A158[0x40000]; t_byte_14002A158& b26 = *(t_byte_14002A158*)(blob1+ 0x14002A158 -0x1400227B0);
//char* b26 = &b12[818]; 

// memset((int*)b13, 0x55u, 0x80000);
typedef char t_byte_14006A158[0x80000]; t_byte_14006A158& b13 = *(t_byte_14006A158*)(blob1+ 0x14006A158 -0x1400227B0);
typedef char t_byte_14007A158[65536]; t_byte_14007A158& b28 = *(t_byte_14007A158*)(blob1+ 0x14007A158 -0x1400227B0);
typedef char t_byte_14008A158[65536]; t_byte_14008A158& b29 = *(t_byte_14008A158*)(blob1+ 0x14008A158 -0x1400227B0);
typedef char t_byte_14009A158[65536]; t_byte_14009A158& b30 = *(t_byte_14009A158*)(blob1+ 0x14009A158 -0x1400227B0);
typedef char t_byte_1400AA158[65536]; t_byte_1400AA158& b31 = *(t_byte_1400AA158*)(blob1+ 0x1400AA158 -0x1400227B0);
typedef char t_byte_1400BA158[28087];    t_byte_1400BA158& b34 = *(t_byte_1400BA158*)(blob1+ 0x1400BA158 -0x1400227B0);
char* b35 = &b34[1];
typedef char t_byte_1400CA158[56797];   t_byte_1400CA158& b36 = *(t_byte_1400CA158*)(blob1+ 0x1400CA158 -0x1400227B0);
char* b37 = &b36[1];
char* b33 = &b36[2];
char* b32 = &b36[3];
typedef char t_byte_1400DA158[65536]; t_byte_1400DA158& b27 = *(t_byte_1400DA158*)(blob1+ 0x1400DA158 -0x1400227B0);
typedef char t_byte_1400EA158[131072]; t_byte_1400EA158& b14 = *(t_byte_1400EA158*)(blob1+ 0x1400EA158 -0x1400227B0);

typedef int t_dword_14010A158[635]; t_dword_14010A158& d81 = *(t_dword_14010A158*)(blob1+ 0x14010A158 -0x1400227B0);
int* d73 = &d81[256];
int* d77 = &d81[512];

//typedef char t_byte_14010AD58[262144]; t_byte_14010AD58& b15 = *(t_byte_14010AD58*)(blob1+ 0x14010AD58 -0x1400227B0);
char b15[0x40000];

int& d82 = *(int*)(blob1+ 0x14014AD58 -0x1400227B0);
char& b25 = *(char*)(blob1+ 0x14014AD5C -0x1400227B0);
__int64& q26 = *(__int64*)(blob1+ 0x14014AD60 -0x1400227B0);
int& d17 = *(int*)(blob1+ 0x14014AD68 -0x1400227B0);
int& d72 = *(int*)(blob1+ 0x14014AD6C -0x1400227B0);
int& d18 = *(int*)(blob1+ 0x14014AD70 -0x1400227B0);
int& d20 = *(int*)(blob1+ 0x14014AD74 -0x1400227B0);
int& d19 = *(int*)(blob1+ 0x14014AD78 -0x1400227B0);
int& d78 = *(int*)(blob1+ 0x14014AD7C -0x1400227B0);
int& d83 = *(int*)(blob1+ 0x14014AD80 -0x1400227B0);
int& d84 = *(int*)(blob1+ 0x14014AD84 -0x1400227B0);
int& d85 = *(int*)(blob1+ 0x14014AD88 -0x1400227B0);
int& d92 = *(int*)(blob1+ 0x14014AD8C -0x1400227B0);
int& d86 = *(int*)(blob1+ 0x14014AD90 -0x1400227B0);
int& d87 = *(int*)(blob1+ 0x14014AD94 -0x1400227B0);
int& d52 = *(int*)(blob1+ 0x14014AD98 -0x1400227B0);
int& d50 = *(int*)(blob1+ 0x14014AD9C -0x1400227B0);
int& d54 = *(int*)(blob1+ 0x14014ADA0 -0x1400227B0);
int& d53 = *(int*)(blob1+ 0x14014ADA4 -0x1400227B0);
int& d56 = *(int*)(blob1+ 0x14014ADA8 -0x1400227B0);
int& d55 = *(int*)(blob1+ 0x14014ADAC -0x1400227B0);
int& d62 = *(int*)(blob1+ 0x14014ADB0 -0x1400227B0);
int& d64 = *(int*)(blob1+ 0x14014ADB4 -0x1400227B0);
int& d80 = *(int*)(blob1+ 0x14014ADB8 -0x1400227B0);
int& d91 = *(int*)(blob1+ 0x14014ADBC -0x1400227B0);

//typedef int t_dword_14014ADC0[4096];   t_dword_14014ADC0& d90 = *(t_dword_14014ADC0*)(blob1+ 0x14014ADC0 -0x1400227B0);
int d90[4096];
int& d79 = d90[2];
int& d66 = d90[3];
int& d68 = d90[4];
int& d65 = d90[5];
int& d67 = d90[6];
int& d76 = d90[7];
int& d88 = d90[8];
int& d89 = d90[9];
int& d109 = d90[10];
int& d108 = d90[11];
int& d107 = d90[12];
int& d75 = d90[13];
int& d63 = d90[14];
int* d71 = &d90[15];

typedef __int128 t_xmmword_14014ED90[64]; t_xmmword_14014ED90& x31 = *(t_xmmword_14014ED90*)(blob1+ 0x14014ED90 -0x1400227B0);
//__int128 x31[64] = {0};
__int128* x30 = &x31[1];
__int128* x29 = &x31[2];
__int128* x28 = &x31[3];
__int128* x27 = &x31[4];
__int128* x26 = &x31[5];

typedef int t_dword_14014EDF0[253];    t_dword_14014EDF0& d22 = *(t_dword_14014EDF0*)(blob1+ 0x14014EDF0 -0x1400227B0);
int* d23 = &d22[3];

__int64& q13 = *(__int64*)(blob1+ 0x14014F1F0 -0x1400227B0);
int& d21 = *(int*)(blob1+ 0x14014F1F8 -0x1400227B0);

//typedef int t_dword_14014F1FC[131072]; t_dword_14014F1FC& d74 = *(t_dword_14014F1FC*)(blob1+ 0x14014F1FC -0x1400227B0);
int d74[131072];

//typedef int t_dword_1401CF1FC[65536];  t_dword_1401CF1FC& d24 = *(t_dword_1401CF1FC*)(blob1+ 0x1401CF1FC -0x1400227B0);
int d24[0x10000];

//typedef int t_dword_14020F1FC[16384];  t_dword_14020F1FC& d57 = *(t_dword_14020F1FC*)(blob1+ 0x14020F1FC -0x1400227B0);
int d57[0x4000];

//typedef int t_dword_14021F1FC[65536];  t_dword_14021F1FC& d60 = *(t_dword_14021F1FC*)(blob1+ 0x14021F1FC -0x1400227B0);
int d60[0x10000];

int& d69 = *(int*)(blob1+ 0x14025F1FC -0x1400227B0);
int& d58 = *(int*)(blob1+ 0x14025F200 -0x1400227B0);
int& d70 = *(int*)(blob1+ 0x14025F204 -0x1400227B0);
int& d61 = *(int*)(blob1+ 0x14025F208 -0x1400227B0);
int& d59 = *(int*)(blob1+ 0x14025F20C -0x1400227B0);
typedef __int64 t_qword_14025F220[1]; t_qword_14025F220& q16 = *(t_qword_14025F220*)(blob1+ 0x14025F220 -0x1400227B0);
__int64& q27 = *(__int64*)(blob1+ 0x14025F228 -0x1400227B0);
typedef __int64 t_qword_14025F230[2]; t_qword_14025F230& q28 = *(t_qword_14025F230*)(blob1+ 0x14025F230 -0x1400227B0);
typedef char t_byte_14025F240[2016]; t_byte_14025F240& b38 = *(t_byte_14025F240*)(blob1+ 0x14025F240 -0x1400227B0);
typedef int t_dword_14025FA20[425985];  t_dword_14025FA20& d26 = *(t_dword_14025FA20*)(blob1+ 0x14025FA20 -0x1400227B0);
short& w1 = *(short*)(blob1+ 0x14025FA24 -0x1400227B0);
typedef __int16 t_word_14025FA26[983037];t_word_14025FA26& w2 = *(t_word_14025FA26*)(blob1+ 0x14025FA26 -0x1400227B0);
typedef char t_byte_14043FA20[6];      t_byte_14043FA20& b16 = *(t_byte_14043FA20*)(blob1+ 0x14043FA20 -0x1400227B0);
typedef __int16 t_word_14043FA26[65661]; t_word_14043FA26& w3 = *(t_word_14043FA26*)(blob1+ 0x14043FA26 -0x1400227B0);
typedef int t_dword_14045FB20[393280]; t_dword_14045FB20& d34 = *(t_dword_14045FB20*)(blob1+ 0x14045FB20 -0x1400227B0);
int* d35 = &d34[1];
typedef int t_dword_1405DFC20[1];      t_dword_1405DFC20& d27 = *(t_dword_1405DFC20*)(blob1+ 0x1405DFC20 -0x1400227B0);
typedef __int16 t_word_1405DFC24[1];     t_word_1405DFC24& w4 = *(t_word_1405DFC24*)(blob1+ 0x1405DFC24 -0x1400227B0);
typedef __int16 t_word_1405DFC26[32768]; t_word_1405DFC26& w5 = *(t_word_1405DFC26*)(blob1+ 0x1405DFC26 -0x1400227B0);
typedef __int16 t_word_1405EFC26[819197];t_word_1405EFC26& w8 = *(t_word_1405EFC26*)(blob1+ 0x1405EFC26 -0x1400227B0);
typedef char t_byte_14077FC20[6];      t_byte_14077FC20& b19 = *(t_byte_14077FC20*)(blob1+ 0x14077FC20 -0x1400227B0);
typedef __int16 t_word_14077FC26[32768]; t_word_14077FC26& w7 = *(t_word_14077FC26*)(blob1+ 0x14077FC26 -0x1400227B0);
typedef __int16 t_word_14078FC26[32893]; t_word_14078FC26& w6 = *(t_word_14078FC26*)(blob1+ 0x14078FC26 -0x1400227B0);
typedef int t_dword_14079FD20[640];    t_dword_14079FD20& d30 = *(t_dword_14079FD20*)(blob1+ 0x14079FD20 -0x1400227B0);

//typedef int t_dword_1407A0720[0x200040]; t_dword_1407A0720& d36 = *(t_dword_1407A0720*)(blob1+ 0x1407A0720 -0x1400227B0);
int d36[0x200040];
int* d37 = &d36[1];

typedef int t_dword_140FA0820[196672];  t_dword_140FA0820& d39 = *(t_dword_140FA0820*)(blob1+ 0x140FA0820 -0x1400227B0);
int* d40 = &d39[1];
typedef int t_dword_141060920[172094]; t_dword_141060920& d42 = *(t_dword_141060920*)(blob1+ 0x141060920 -0x1400227B0);
int* d43 = &d42[1];

typedef char t_byte_141108A20[256]; t_byte_141108A20& b39 = *(t_byte_141108A20*)(blob1+ 0x141108A20 -0x1400227B0);

typedef int t_dword_141108B20[20445];  t_dword_141108B20& d28 = *(t_dword_141108B20*)(blob1+ 0x141108B20 -0x1400227B0);
typedef __int16 t_word_141108B24[38841]; t_word_141108B24& w10 = *(t_word_141108B24*)(blob1+ 0x141108B24 -0x1400227B0);
__int16* w9 = &w10[1];
typedef int t_dword_141128C20[5077];    t_dword_141128C20& d29 = *(t_dword_141128C20*)(blob1+ 0x141128C20 -0x1400227B0);
short& w12 = *(short*)(blob1+ 0x141128C24 -0x1400227B0);
typedef __int16 t_word_141128C26[16509]; t_word_141128C26& w11 = *(t_word_141128C26*)(blob1+ 0x141128C26 -0x1400227B0);
typedef int t_dword_141130D20[2433];    t_dword_141130D20& d31 = *(t_dword_141130D20*)(blob1+ 0x141130D20 -0x1400227B0);
int* d32 = &d31[1];

__int128& x1 = *(__int128*)(blob1+ 0x141133520 -0x1400227B0);
__int64& q3 = *(__int64*)(blob1+ 0x141133550 -0x1400227B0);
__int64& q2 = *(__int64*)(blob1+ 0x141133558 -0x1400227B0);
__int64& q4 = *(__int64*)(blob1+ 0x141133560 -0x1400227B0);
__int64& q5 = *(__int64*)(blob1+ 0x141133568 -0x1400227B0);
__int64& q6 = *(__int64*)(blob1+ 0x141133570 -0x1400227B0);
__int64& q10 = *(__int64*)(blob1+ 0x141133580 -0x1400227B0);
__int64& q8 = *(__int64*)(blob1+ 0x141133588 -0x1400227B0);
__int64& q32 = *(__int64*)(blob1+ 0x141133590 -0x1400227B0);
__int64& q31 = *(__int64*)(blob1+ 0x141133598 -0x1400227B0);
__int64& q30 = *(__int64*)(blob1+ 0x1411335A0 -0x1400227B0);
__int64& q29 = *(__int64*)(blob1+ 0x1411335A8 -0x1400227B0);
__int64& q34 = *(__int64*)(blob1+ 0x1411335B0 -0x1400227B0);
__int64& q35 = *(__int64*)(blob1+ 0x1411335B8 -0x1400227B0);
__int64& q21 = *(__int64*)(blob1+ 0x1411335C0 -0x1400227B0);
__int64& q22 = *(__int64*)(blob1+ 0x1411335C8 -0x1400227B0);
__int64& q18 = *(__int64*)(blob1+ 0x1411335D0 -0x1400227B0);
__int64& q23 = *(__int64*)(blob1+ 0x1411335D8 -0x1400227B0);
__int64& q20 = *(__int64*)(blob1+ 0x1411335E0 -0x1400227B0);
__int64& q17 = *(__int64*)(blob1+ 0x1411335E8 -0x1400227B0);
__int64& q36 = *(__int64*)(blob1+ 0x1411335F0 -0x1400227B0);
__int64& q19 = *(__int64*)(blob1+ 0x1411335F8 -0x1400227B0);
__int64& q24 = *(__int64*)(blob1+ 0x141133600 -0x1400227B0);
__int64& q25 = *(__int64*)(blob1+ 0x141133608 -0x1400227B0);
__int64& q9 = *(__int64*)(blob1+ 0x141133610 -0x1400227B0);
__int64& q33 = *(__int64*)(blob1+ 0x141133618 -0x1400227B0);
__int64& q14 = *(__int64*)(blob1+ 0x141133620 -0x1400227B0);
__int64& q37 = *(__int64*)(blob1+ 0x141133628 -0x1400227B0);
__int64& q39 = *(__int64*)(blob1+ 0x141133630 -0x1400227B0);
__int64& q11 = *(__int64*)(blob1+ 0x141133638 -0x1400227B0);
int& d2 = *(int*)(blob1+ 0x141133648 -0x1400227B0);
int& d4 = *(int*)(blob1+ 0x14113364C -0x1400227B0);
int& d3 = *(int*)(blob1+ 0x141133650 -0x1400227B0);
int& d6 = *(int*)(blob1+ 0x141133654 -0x1400227B0);
int& d44 = *(int*)(blob1+ 0x141133658 -0x1400227B0);
int& d114 = *(int*)(blob1+ 0x14113365C -0x1400227B0);
int& d9 = *(int*)(blob1+ 0x141133660 -0x1400227B0);
int& d8 = *(int*)(blob1+ 0x141133664 -0x1400227B0);
int& d13 = *(int*)(blob1+ 0x141133674 -0x1400227B0);
int& d14 = *(int*)(blob1+ 0x141133678 -0x1400227B0);
int& d97 = *(int*)(blob1+ 0x14113367C -0x1400227B0);
int& d51 = *(int*)(blob1+ 0x141133680 -0x1400227B0);
int& d93 = *(int*)(blob1+ 0x141133684 -0x1400227B0);
int& d25 = *(int*)(blob1+ 0x141133688 -0x1400227B0);
int& d41 = *(int*)(blob1+ 0x14113368C -0x1400227B0);
int& d5 = *(int*)(blob1+ 0x141133690 -0x1400227B0);
int& d11 = *(int*)(blob1+ 0x141133694 -0x1400227B0);
int& d12 = *(int*)(blob1+ 0x141133698 -0x1400227B0);
int& d15 = *(int*)(blob1+ 0x14113369C -0x1400227B0);
int& d33 = *(int*)(blob1+ 0x1411336A0 -0x1400227B0);
int& d110 = *(int*)(blob1+ 0x1411336A4 -0x1400227B0);
int& d48 = *(int*)(blob1+ 0x1411336A8 -0x1400227B0);
int& d49 = *(int*)(blob1+ 0x1411336AC -0x1400227B0);
int& d46 = *(int*)(blob1+ 0x1411336B0 -0x1400227B0);
int& d47 = *(int*)(blob1+ 0x1411336B4 -0x1400227B0);
int& d99 = *(int*)(blob1+ 0x1411336B8 -0x1400227B0);
int& d100 = *(int*)(blob1+ 0x1411336BC -0x1400227B0);
int& d101 = *(int*)(blob1+ 0x1411336C0 -0x1400227B0);
int& d102 = *(int*)(blob1+ 0x1411336C4 -0x1400227B0);
int& d105 = *(int*)(blob1+ 0x1411336C8 -0x1400227B0);
int& d104 = *(int*)(blob1+ 0x1411336CC -0x1400227B0);
int& d38 = *(int*)(blob1+ 0x1411336D0 -0x1400227B0);
int& d45 = *(int*)(blob1+ 0x1411336D8 -0x1400227B0);
int& d111 = *(int*)(blob1+ 0x1411336DC -0x1400227B0);
int& d10 = *(int*)(blob1+ 0x1411336E0 -0x1400227B0);
int& d95 = *(int*)(blob1+ 0x1411336E4 -0x1400227B0);
int& d96 = *(int*)(blob1+ 0x1411336E8 -0x1400227B0);
int& d98 = *(int*)(blob1+ 0x1411336EC -0x1400227B0);
int& d103 = *(int*)(blob1+ 0x1411336F0 -0x1400227B0);
int& d106 = *(int*)(blob1+ 0x1411336F4 -0x1400227B0);
__int64& q38 = *(__int64*)(blob1+ 0x1411336F8 -0x1400227B0);
int& d112 = *(int*)(blob1+ 0x141133700 -0x1400227B0);
int& d113 = *(int*)(blob1+ 0x141133704 -0x1400227B0);
int& d16 = *(int*)(blob1+ 0x141133708 -0x1400227B0);
int& d7 = *(int*)(blob1+ 0x14113370C -0x1400227B0);

typedef void* t_qword_1411337D0; t_qword_1411337D0& q1 = *(t_qword_1411337D0*)(blob1+ 0x1411337D0 -0x1400227B0);
__int64& q15 = *(__int64*)(blob1+ 0x141133DB0 -0x1400227B0);

int& d1 = *(int*)(blob1+ 0x141134C28 -0x1400227B0);
typedef byte t_algn_141136BC4[1084+0x200]; t_algn_141136BC4& __algn_141136BC4 = *(t_algn_141136BC4*)(blob1+ 0x141136BC4 -0x1400227B0);

//void *& __Buffer = *(void **)(blob1+ 0x140028E40 -0x1400227B0);

void* __Buffer = 0;

void* __PPMblock = 0;
__int64 __PPMblock_size = 0;

int __f_ENC = 0;
int __f_LOG = 0;

