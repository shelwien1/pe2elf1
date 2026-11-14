// flacdump: container structures: audio segments, metadata blocks, layout, .meta magic
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

enum { SEG_FRAME = 0, SEG_RAWFRAME = 1, SEG_RAWBYTES = 2, SEG_END = 3 };

struct Segment {
    int kind;
    u64 off;    // byte offset in flac
    u64 len;    // byte length
    u32 nsamp;  // samples (frames)
};

struct MBlock {
    int type;
    u32 len;
    u8* data;  // c: points into the input file; d: only valid while the block is decoded
    int structured;
    u64 off;  // file offset of the block header
};
struct FlacLayout {
    u8* prefix;  // c: bytes before "fLaC" (in the input buffer)
    size_t prefix_n;
    Vec<MBlock> blocks;
    StreamParams sp;
    int vendor;  // encoder hint from the vendor string
    u8 si[34];   // d: STREAMINFO contents
};
enum { BLOCKMAX = 1 << 24 };  // metadata block length is a 24-bit field
static u8 g_blockbuf[BLOCKMAX];  // d: contents of the metadata block being decoded

static const u8 k_magic[2] = {'F', 0xD1};
