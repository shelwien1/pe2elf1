/*
MPZ_Codec3a.obj : error LNK2005: "unsigned short (* bandInfo)[23]" (?bandInfo@@3PAY0BH@GA) already defined in MPZ_Codec3.obj
MPZ_Codec3a.obj : error LNK2005: "struct hufproptab * htprop" (?htprop@@3PAUhufproptab@@A) already defined in MPZ_Codec3.obj

MPZ_Codec3a.obj : error LNK2005: "unsigned int (* huff_ctab)[256]" (?huff_ctab@@3PAY0BAA@IA) already defined in MPZ_Codec3.obj
MPZ_Codec3a.obj : error LNK2005: "unsigned int (* huff_dtab)[257]" (?huff_dtab@@3PAY0BAB@IA) already defined in MPZ_Codec3.obj
MPZ_Codec3a.obj : error LNK2005: "unsigned char (* huff_hlen)[256]" (?huff_hlen@@3PAY0BAA@EA) already defined in MPZ_Codec3.obj
MPZ_Codec3a.obj : error LNK2005: "unsigned short (* c1tab)[4][4][4][4]" (?c1tab@@3PAY33333GA) already defined in MPZ_Codec3.obj
*/

// mpeg2 scalefactors lens
const byte stab[3][6][4] = {
   { { 6, 5, 5,5 } , { 6, 5, 7,3 } , { 11,10,0,0} ,
     { 7, 7, 7,0 } , { 6, 6, 6,3 } , {  8, 8,5,0} } ,
   { { 9, 9, 9,9 } , { 9, 9,12,6 } , { 18,18,0,0} ,
     {12,12,12,0 } , {12, 9, 9,6 } , { 15,12,9,0} } ,
   { { 6, 9, 9,9 } , { 6, 9,12,6 } , { 15,18,0,0} ,
     { 6,15,12,0 } , { 6,12, 9,6 } , {  6,18,9,0} } }; 

// sideinfo block lengths 
const byte l_si[4] = {9,17,17,32}; // [mode!=3][ID]

const int bitrat[2][15] = {
{0, 8000,16000,24000,32000,40000,48000,56000, 64000, 80000, 96000,112000,128000,144000,160000}, // mpeg2[.5] ID=0
{0,32000,40000,48000,56000,64000,80000,96000,112000,128000,160000,192000,224000,256000,320000}  // mpeg1     ID=1
};

const int spfreq[2][2][4] = {
 {
  {11025,12000, 8000,0}, // ID=0, IDex=0 -> mpeg2.5
  {22050,24000,16000,0}  // ID=0, IDex=1 -> mpeg2
 },{
  {    0,    0,    0,0}, // ID=1, IDex=0 -> bug
  {44100,48000,32000,0}  // ID=1, IDex=1 -> mpeg1
 }
};

//------------------------ диапазоны scalefactors ----------------------------
const int slen1_tab[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
const int slen2_tab[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };


const word bandInfo[9][23] = { 
/* MPEG 1.0 */
 {0,4,8,12,16,20,24,30,36,44,52,62,74, 90,110,134,162,196,238,288,342,418,576},
 {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,384,576},
 {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576},
/* MPEG 2.0 */
 {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,330,394,464,540,576},
 {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
/* MPEG 2.5 */
 {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
 {0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576}
};

#include "parse_huf_tbl.inc"
