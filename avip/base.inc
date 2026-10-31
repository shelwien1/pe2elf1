// base types, byte order helpers, platform glue

typedef uint8_t  byte;
typedef uint32_t uint;
typedef uint64_t qword;
typedef std::vector<byte> Buf;
typedef std::vector<std::pair<qword,qword> > PosLen;   // (position, length) pairs
typedef std::vector<std::pair<qword,qword> > IdxLen;   // (index, length) pairs: transform side info

#if defined(_MSC_VER)
  #define fseek64 _fseeki64
  #define ftell64 _ftelli64
#elif defined(_WIN32)
  #define fseek64 fseeko64
  #define ftell64 ftello64
#else
  #define fseek64 fseeko
  #define ftell64 ftello
#endif

#define FCC_BE(a,b,c,d) ((uint(byte(a))<<24)|(uint(byte(b))<<16)|(uint(byte(c))<<8)|uint(byte(d)))   // MP4/MKV order
#define FCC_LE(a,b,c,d) ((uint(byte(a)))|(uint(byte(b))<<8)|(uint(byte(c))<<16)|(uint(byte(d))<<24))   // RIFF order

static int verbose = 0;
static const qword MAXCOUNT = qword(1)<<31;

static inline uint  BE16( const byte* p ) { return (uint(p[0])<<8)|p[1]; }
static inline uint  BE32( const byte* p ) { return (uint(p[0])<<24)|(uint(p[1])<<16)|(uint(p[2])<<8)|uint(p[3]); }
static inline qword BE64( const byte* p ) { return (qword(BE32(p))<<32)|BE32(p+4); }
static inline void  PutBE32( byte* p, uint x )  { p[0]=byte(x>>24); p[1]=byte(x>>16); p[2]=byte(x>>8); p[3]=byte(x); }
static inline void  PutBE64( byte* p, qword x ) { PutBE32(p,uint(x>>32)); PutBE32(p+4,uint(x)); }
static inline uint  LE16( const byte* p ) { return uint(p[0])|(uint(p[1])<<8); }
static inline uint  LE32( const byte* p ) { return uint(p[0])|(uint(p[1])<<8)|(uint(p[2])<<16)|(uint(p[3])<<24); }
static inline qword LE64( const byte* p ) { return qword(LE32(p))|(qword(LE32(p+4))<<32); }
static inline void  PutLE32( byte* p, uint x )  { p[0]=byte(x); p[1]=byte(x>>8); p[2]=byte(x>>16); p[3]=byte(x>>24); }
static inline void  PutLE64( byte* p, qword x ) { PutLE32(p,uint(x)); PutLE32(p+4,uint(x>>32)); }
