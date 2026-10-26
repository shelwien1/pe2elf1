
// Layer I/II tables (ISO 11172-3 annex B, 13818-3 annex B)

// bitrates in kbps: [ID][layer II][bitrate]
const word l12_bitrat[2][2][15] = {
 { { 0,32,48,56, 64, 80, 96,112,128,144,160,176,192,224,256 },   // MPEG2 layer I
   { 0, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160 } }, // MPEG2 layer II
 { { 0,32,64,96,128,160,192,224,256,288,320,352,384,416,448 },   // MPEG1 layer I
   { 0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384 } }  // MPEG1 layer II
};

// layer II quantization classes: number of levels, and bits per sample
// (negative: bits per group of 3 samples)
const word l2_levels[17] = { 3,5,7,9,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535 };
const signed char l2_qbits[17] = { -5,-7,3,-10,4,5,6,7,8,9,10,11,12,13,14,15,16 };

// layer II bit allocation rows: [0] = bits of the allocation field,
// [b] = quantization class for allocation b
const byte l2_rows[8][16] = {
  { 4, 0, 2, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16 },
  { 4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,16 },
  { 3, 0, 1, 2, 3, 4, 5,16 },
  { 2, 0, 1,16 },
  { 4, 0, 1, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
  { 3, 0, 1, 3, 4, 5, 6, 7 },
  { 4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14 },
  { 2, 0, 1, 3 }
};

// allocation tables 0..3: MPEG1 B.2a-d, 4: MPEG2 B.1; row of each subband
const byte l2_sblimit[5] = { 27,30,8,12,30 };
const byte l2_row[5][30] = {
  { 0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3 },
  { 0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3 },
  { 4,4,5,5,5,5,5,5 },
  { 4,4,5,5,5,5,5,5,5,5,5,5 },
  { 6,6,6,6,5,5,5,5,5,5,5,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7 }
};

// layer II: scalefactors sent for each scfsi value
const byte l2_nscf[4] = { 3,2,1,2 };
