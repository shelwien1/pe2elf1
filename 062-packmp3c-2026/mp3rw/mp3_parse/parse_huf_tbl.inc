              
struct hufproptab {
  byte xlen;
  byte ylen;  
  byte linbits;
};

const hufproptab htprop[] = {
  {  0,  0,  0 },          // 0
  {  2,  2,  0 },          // 1
  {  3,  3,  0 },          // 2
  {  3,  3,  0 },          // 3
  {  0,  0,  0 },          // 4
  {  4,  4,  0 },          // 5
  {  4,  4,  0 },          // 6
  {  6,  6,  0 },          // 7
  {  6,  6,  0 },          // 8
  {  6,  6,  0 },          // 9
  {  8,  8,  0 },          // 10
  {  8,  8,  0 },          // 11
  {  8,  8,  0 },          // 12
  { 16, 16,  0 },          // 13
  {  0,  0,  0 },          // 14
  { 16, 16,  0 },          // 15
  { 16, 16,  1 }, //  \                // 16
  { 16, 16,  2 },                      // 17
  { 16, 16,  3 },                      // 18
  { 16, 16,  4 }, //    table 16       // 19
  { 16, 16,  6 },                      // 20
  { 16, 16,  8 },                      // 21
  { 16, 16, 10 },                      // 22
  { 16, 16, 13 }, //  /                // 23
  { 16, 16,  4 }, //  \                // 24
  { 16, 16,  5 },                      // 25
  { 16, 16,  6 },                      // 26
  { 16, 16,  7 }, //    table 24       // 27
  { 16, 16,  8 },                      // 28
  { 16, 16,  9 },                      // 29
  { 16, 16, 11 },                      // 30
  { 16, 16, 13 }, //  /                // 31
  {  1, 16,  0 },
  {  1, 16,  0 }
};            
              
//uint* huff_ctab[34];
//uint* huff_dtab[34];
//byte* huff_hlen[34];
//word  c1tab[2][4][4][4][4];

const uint huff_ctab[34][256] = {
#include "huff_ctab.inc"
};

const uint huff_dtab[34][257] = {
#include "huff_dtab.inc"
};

const byte huff_hlen[34][256] = {
#include "huff_hlen.inc"
};

const word c1tab[2][4][4][4][4] = {
#include "huff_c1tab.inc"
};
             
//#include "parse_huf_tree.inc"
//#include "parse_huf_init.inc"
              