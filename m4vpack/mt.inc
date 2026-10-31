// mt.inc -- c -t N and d of its streams: the stream in parts, on threads (MTwrap).

// c -t N: the stream cut into parts by M4vSplit (on the main thread), each part a stream by itself,
// and the parts dealt in turn to N workers by MTwrap (MTwrap_v3, its parts mode), a model for each
// part (M4vPart runs an M4vCoro on it). A part ends:
//   - before the first I-VOP at least -c's part size after the part's start, or before the run of
//     headers (VOS, VO, VOL, user data, GOV) right before that I-VOP;
//   - where no I-VOP comes, before the first VOP but a B-VOP at least mt_cap() after its start (a
//     B-VOP's syntax depends on the VOP before it);
//   - where no VOP comes within half again of the cap, at any start code, and where none comes
//     within twice the cap, there.
// VOPs that aren't coded (vop_coded 0: the placeholders packed B-VOPs leave) don't count as I-VOPs
// or other VOPs here. The headers m4vpack's parser reads (the last VOL, the last DivX and XviD
// user data), as defined before a part, are copied in at its start (their first 32 KB, which is
// what the parser reads), unless the part defines them itself before its first VOP: so each part
// parses as it would in the whole stream, but where the parser guessed the VOP header's time
// increment size.
//
// A worker's stream is a record for each of its parts:
//   u64 plen   the size of the part as compressed (with the headers copied in; never 0)
//   u64 ins    where in it those are: [ins, ins + xlen), removed after decoding
//   u64 xlen
//   u64 part   its place in the stream, from 0: worker w's k-th part is w + kN
//   u32 crc    the CRC-32 of these 32 bytes; u32 0
//   then the part compressed, a stream of the single kind (magic, version, level, chunks, CRC),
//   which ends where its decoder stops reading (rc.inc's decoder reads exactly the bytes its coder
//   wrote): the next record follows it
// and its end, a record of plen 0 (ins and xlen 0) alone, whose part is the place its next part
// would have had (little endian). So d finds a part lost, added or moved, or a worker's stream cut
// short. A worker writes its stream as it codes it, 64 KB at a time (and the rest of a part as it
// ends), and MTwrap puts each block in its container as it comes: so the container has the
// workers' streams interleaved as they were made, and d, whose workers decode about as fast as c's
// coded, finds each worker's input about when it needs it. MTwrap's container holds the N streams,
// after m4vpack's header: mt_magic, the format version, the threads, the part size in KB (4
// bytes). d needs as many threads as c had; each part ends a round of MTwrap's decoder (Y_PART),
// which takes the parts from the workers in turn, as dealt.

// the header: mt_magic, the version, the threads, the part size in KB (MT_PART_BYTES)
enum { MT_MAGIC = 5, MT_AT_VERSION = MT_MAGIC, MT_AT_THREADS, MT_AT_PART, MT_PART_BYTES = U32_BYTES,
       MT_HEAD = MT_AT_PART + MT_PART_BYTES };
static const U8 mt_magic[MT_MAGIC] = { 'm', '4', 'v', 'p', 'm' };
constexpr int HEAD_MAX = (int)MT_HEAD > (int)M4V_HEAD ? (int)MT_HEAD : (int)M4V_HEAD;  // either kind of stream's

constexpr int MAX_THREADS = 64;  // (-t)
// a worker's record: its fields' places, and its size
enum {
  MT_R_PLEN = 0,                        // u64 plen
  MT_R_INS = MT_R_PLEN + U64_BYTES,     // u64 ins
  MT_R_XLEN = MT_R_INS + U64_BYTES,     // u64 xlen
  MT_R_PART = MT_R_XLEN + U64_BYTES,    // u64 part
  MT_R_CRC = MT_R_PART + U64_BYTES,     // u32 crc, of the fields before it
  MT_R_ZERO = MT_R_CRC + U32_BYTES,     // u32 0
  MT_REC = MT_R_ZERO + U32_BYTES
};
// a part as the splitter frames it: u64 L, u64 ins, u64 xlen, then L - MT_F_PAST bytes (L counts
// what follows it)
enum {
  MT_F_LEN = 0,
  MT_F_INS = MT_F_LEN + U64_BYTES,
  MT_F_XLEN = MT_F_INS + U64_BYTES,
  MT_FRAME = MT_F_XLEN + U64_BYTES,
  MT_F_PAST = MT_FRAME - MT_F_INS
};
constexpr U32 MT_RING = 1 * MIB;       // MTwrap's buffers to and from each worker
constexpr U32 MT_COPY_MAX = HDR_PEEK;  // the bytes of a header copied in, at most (what the parser reads)
constexpr unsigned MTW_NO_THREAD = 7;  // MTwrap's Init(): a thread didn't start (1-6: no memory)

static void put_le(U8* p, U64 v, int n) { int i; for (i = 0; i < n; i++) p[i] = (U8)(v >> (BYTE_BITS * i)); }
static U64 get_le(const U8* p, int n) {
  U64 v = 0;
  int i;
  for (i = n - 1; i >= 0; i--) v = (v << BYTE_BITS) | p[i];
  return v;
}

// the headers copied in
enum { K_VOL, K_DIVX, K_XVID, NKEY };
// the size past which a part ends at any VOP but a B-VOP: MT_CAP_PARTS part sizes, to MT_CAP_MAX
constexpr U64 MT_CAP_PARTS = 4, MT_CAP_MAX = 1ull << 31;
static U64 mt_cap(U64 part) { return MT_CAP_PARTS * part < MT_CAP_MAX ? MT_CAP_PARTS * part : MT_CAP_MAX; }
// where no VOP comes, a part ends at any start code past half again of the cap (mt_soft()), and
// anywhere at twice the cap (mt_hard())
static U64 mt_soft(U64 cap) { return cap + cap / 2; }
static U64 mt_hard(U64 cap) { return 2 * cap; }
// The most a part can be, as the splitter cuts it: mt_hard(), and the headers copied in. d takes a
// record of a larger part to be broken, which bounds the memory a part's output takes.
static U64 mt_part_max(U64 part) { return mt_hard(mt_cap(part)) + NKEY * (U64)MT_COPY_MAX; }

static void mt_head(U8* h, int threads, U32 part) {
  memcpy(h, mt_magic, sizeof mt_magic);
  h[MT_AT_VERSION] = M4V_VERSION;
  h[MT_AT_THREADS] = (U8)threads;
  put_le(h + MT_AT_PART, part / KIB, MT_PART_BYTES);
}

static U32 mt_rec_crc(const U8* h) { return crc32_upd(0, h, MT_R_CRC); }
static void mt_rec(U8* h, U64 plen, U64 ins, U64 xlen, U64 part) {
  put_le(h + MT_R_PLEN, plen, U64_BYTES);
  put_le(h + MT_R_INS, ins, U64_BYTES);
  put_le(h + MT_R_XLEN, xlen, U64_BYTES);
  put_le(h + MT_R_PART, part, U64_BYTES);
  put_le(h + MT_R_CRC, mt_rec_crc(h), U32_BYTES);
  put_le(h + MT_R_ZERO, 0, U32_BYTES);
}

#include "m4vsplit.inc"  // M4vSplit: c -t's splitter

struct MtArgs {
  int threads, stats;
  int level;        // c: the model's
  U64 part_max;     // d: the largest part a record can have (mt_part_max())
  Progress* meter;  // the call's progress line, which the workers' runs tell and poll (0: none)
};

#include "partc.inc"  // PartC: a worker's c of a part
#include "partd.inc"  // PartD: a worker's d of a part

#include "m4vpart.inc"  // M4vPart: a worker of c -t and d

// c: the splitter into MTwrap's coder; d: MTwrap's decoder
typedef MTwrap<0, M4vPart<0>, const MtArgs*> MtEncWorkers;  // (c's workers)
typedef CoroutinePair<M4vSplit, MtEncWorkers> MtEnc;
typedef MTwrap<1, M4vPart<1>, const MtArgs*> MtDec;

// whether a worker failed: the first one's exit status, its note copied to note (the workers go
// with MTwrap's Quit())
template <class M>
int mt_failed(M& m, char* note, size_t n) {
  uint i;
  for (i = 0; i < m.th_N; i++)
    if (m.C1[i].f_fail) {
      snprintf(note, n, "%s", m.C1[i].note);
      return m.C1[i].code ? m.C1[i].code : E_INTERNAL;
    }
  return 0;
}
// whether d's workers had the parts as c deals them, in turn: of P in all, worker i the i-th of each
// round of N, one more where P % N is past it
static int mt_dealt(MtDec& d) {
  U64 P = 0, N = d.th_N;
  uint i;
  for (i = 0; i < d.th_N; i++) P += d.C1[i].parts;
  for (i = 0; i < d.th_N; i++)
    if (d.C1[i].parts != P / N + (i < P % N)) return 0;
  return 1;
}
