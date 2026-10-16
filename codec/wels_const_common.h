
#ifndef WELS_CONST_COMMON_H__
#define WELS_CONST_COMMON_H__

// Miscellaneous sizing infos
#ifndef MAX_FNAME_LEN
#define MAX_FNAME_LEN           256     // maximal length of file name in char size
#endif//MAX_FNAME_LEN

#ifndef WELS_LOG_BUF_SIZE
#define WELS_LOG_BUF_SIZE       4096
#endif//WELS_LOG_BUF_SIZE

#ifndef MAX_TRACE_LOG_SIZE
#define MAX_TRACE_LOG_SIZE      (50 * (1<<20))  // max trace log size: 50 MB, overwrite occur if log file size exceeds this size
#endif//MAX_TRACE_LOG_SIZE

/* MB width in pixels for specified colorspace I420 usually used in codec */
#define MB_WIDTH_LUMA           16
#define MB_WIDTH_CHROMA         (MB_WIDTH_LUMA>>1)
/* MB height in pixels for specified colorspace I420 usually used in codec */
#define MB_HEIGHT_LUMA          16
#define MB_HEIGHT_CHROMA        (MB_HEIGHT_LUMA>>1)
#define MB_COEFF_LIST_SIZE      (256+((MB_WIDTH_CHROMA*MB_HEIGHT_CHROMA)<<1))
#define MB_PARTITION_SIZE       4       // Macroblock partition size in 8x8 sub-blocks
#define MB_BLOCK4x4_NUM         16
#define MB_BLOCK8x8_NUM         4
#define MAX_SPS_COUNT           32      // Count number of SPS
#define BASE_QUALITY_ID         0


#endif//WELS_CONST_COMMON_H__
