#ifndef H264P_TABLES_H
#define H264P_TABLES_H

#include <cstdint>

namespace h264p {

extern const int8_t kCabacInit[460][4][2];
extern const int8_t kCabacInit444[564][4][2];  // ctxIdx 460..1023
extern const uint8_t kRangeLps[64][4];
extern const uint8_t kTransIdx[64][2];

// {code, length}; length 0 = no such code
extern const uint16_t kCoeffToken[5][17 * 4][2];
extern const uint16_t kCoeffTokenChromaDc422[9 * 4][2];
extern const uint8_t kTotalZeros[15][16][2];
extern const uint8_t kTotalZerosChromaDc420[3][4][2];
extern const uint8_t kTotalZerosChromaDc422[7][8][2];
extern const uint8_t kRunBefore[7][15][2];

extern const uint8_t kSig8x8Frame[64];
extern const uint8_t kSig8x8Field[64];
extern const uint8_t kLast8x8[64];

extern const uint8_t kCbpIntra[48];
extern const uint8_t kCbpInter[48];
extern const uint8_t kCbpIntraMono[16];
extern const uint8_t kCbpInterMono[16];

}  // namespace h264p

#endif
