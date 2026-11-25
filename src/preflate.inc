#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

typedef unsigned char byte;
typedef unsigned int uint;

struct PreflateConstants {
  enum {
    LITERAL_COUNT = 256,
    NONLEN_CODE_COUNT = LITERAL_COUNT+1,
    LEN_CODE_COUNT = 29,
    LITLEN_CODE_COUNT = NONLEN_CODE_COUNT+LEN_CODE_COUNT,
    DIST_CODE_COUNT = 30,
    LITLENDIST_CODE_COUNT = LITLEN_CODE_COUNT+DIST_CODE_COUNT,
    CODETREE_CODE_COUNT = 19,
    MIN_MATCH = 3,
    MAX_MATCH = 258,
    MAX_BITS = 15,
    MIN_LOOKAHEAD = MAX_MATCH+MIN_MATCH+1,
  };
  static const byte distCodeTable[512];
  static const byte lengthCodeTable[MAX_MATCH-MIN_MATCH+1];
  static const byte lengthBaseTable[LEN_CODE_COUNT];
  static const uint16_t distBaseTable[DIST_CODE_COUNT];
  static const byte lengthExtraTable[LEN_CODE_COUNT];
  static const byte distExtraTable[DIST_CODE_COUNT];
  static const byte treeCodeOrderTable[CODETREE_CODE_COUNT];
  static inline uint DCode(const uint dist) {
    return distCodeTable[dist<=256 ? dist-1 : 256+((dist-1)>>7)];
  }

  static inline uint LCode(const uint len) {
    return lengthCodeTable[len-MIN_MATCH];
  }
};
class PreflateInput {
public:
PreflateInput(const std::vector<byte> &v) : _data(v.size()>0 ? &v[0] : nullptr), _size(v.size()), _pos(0) {
}

const uint pos() const {
  return _pos;
}

const uint size() const {
  return _size;
}

const byte*curChars(int offset = 0) const {
  return _data+_pos+offset;
}

const byte curChar(int offset = 0) const {
  return _data[_pos+offset];
}

void advance(const uint l) {
  _pos += l;
}

const uint remaining() const {
  return _size-_pos;
}

private:
const byte* _data;
uint _size;
uint _pos;
};
struct PreflateHashIterator {
  const uint16_t* chain;
  const uint* chainDepth;
  const uint refPos;
  const uint maxDist;
  uint curPos, curDist;
  bool isValid;
  PreflateHashIterator(const uint16_t* chain_, const uint* depth_, const uint refPos_, const uint maxDist_, uint startPos_) : chain(chain_), chainDepth(depth_), refPos(refPos_), maxDist(maxDist_), curPos(startPos_), curDist(dist(refPos_, startPos_)) {
    isValid = curDist<=maxDist;
  }

  inline bool valid() const {
    return isValid;
  }

  inline bool operator!() const {
    return !isValid;
  }

  static inline uint dist(const uint p1, const uint p2) {
    return p1-p2;
  }

  inline uint dist() const {
    return curDist;
  }

  inline uint depth() const {
    return chainDepth[curPos];
  }

  inline bool next() {
    curPos = chain[curPos];
    curDist = dist(refPos, curPos);
    isValid = curPos>0&&curDist<=maxDist;
    return isValid;
  }
};
struct PreflateHashChainExt {
  PreflateInput _input;
  uint16_t* head;
  uint* chainDepth;
  uint16_t* prev;
  byte hashBits, hashShift;
  uint16_t runningHash, hashMask;
  uint totalShift;
  PreflateHashChainExt(const std::vector<byte> &input_, const byte memLevel);
  ~PreflateHashChainExt();
  uint nextHash(const byte b) const {
    return ((runningHash<<hashShift)^b);
  }

  uint nextHash(const byte b1, const byte b2) const {
    return ((((runningHash<<hashShift)^b1)<<hashShift)^b2);
  }

  void updateRunningHash(const byte b) {
    runningHash = (runningHash<<hashShift)^b;
  }

  void reshift();
  uint getHead(const uint hash) const {
    return head[hash&hashMask];
  }

  uint getNodeDepth(const uint node) const {
    return chainDepth[node];
  }

  uint getRelPosDepth(const uint refPos, const uint head) const {
    return chainDepth[head]-chainDepth[refPos-totalShift];
  }

  PreflateHashIterator iterateFromHead(const uint hash, const uint refPos, const uint maxDist) const {
    return PreflateHashIterator(prev, chainDepth, refPos-totalShift, maxDist, head[hash&hashMask]);
  }

  PreflateHashIterator iterateFromNode(const uint node, const uint refPos, const uint maxDist) const {
    return PreflateHashIterator(prev, chainDepth, refPos-totalShift, maxDist, node);
  }

  PreflateHashIterator iterateFromPos(const uint pos, const uint refPos, const uint maxDist) const {
    return PreflateHashIterator(prev, chainDepth, refPos-totalShift, maxDist, pos-totalShift);
  }

  const PreflateInput &input() const {
    return _input;
  }

  uint curHash() const {
    return nextHash(_input.curChar(2));
  }

  uint curPlus1Hash() const {
    return nextHash(_input.curChar(2), _input.curChar(3));
  }

  void updateHash(const uint l);
  void updateHashLong(const uint l);
  void skipHash(const uint l);

private:
  void _updateHashSimple(const uint l);
};
struct PreflateToken {
  enum typeLit { LITERAL };
  enum typeRef { REFERENCE };
  enum typeNon { NONE };
  uint16_t len : 9;
  uint16_t irregular258 : 1;
  uint16_t dist;
  PreflateToken(typeNon n) : len(0), irregular258(0), dist(0) {
  }

  PreflateToken(typeLit l) : len(1), irregular258(0), dist(0) {
  }

  PreflateToken(typeRef r, uint16_t l, uint16_t d, bool irregular258_ = false) : len(l), irregular258(irregular258_), dist(d) {
  }
};
struct PreflateTokenBlock {
  enum Type { STORED, DYNAMIC_HUFF, STATIC_HUFF };
  enum StoredBlockType { STORED_X };
  enum HuffBlockType { DYNAMIC_HUFF_X, STATIC_HUFF_X };
  Type type;
  uint64_t uncompressedStartPos;
  uint64_t uncompressedLen;
  int32_t contextLen;
  uint16_t nlen, ndist, ncode;
  uint8_t paddingBitCount, paddingBits;
  std::vector<byte> treecodes;
  std::vector<PreflateToken> tokens;
  PreflateTokenBlock() : type(STORED), uncompressedLen(0) {
  }

  PreflateTokenBlock(StoredBlockType, int len_) : type(STORED), uncompressedLen(len_) {
  }

  PreflateTokenBlock(HuffBlockType t) : type(t==DYNAMIC_HUFF_X ? DYNAMIC_HUFF : STATIC_HUFF), uncompressedLen(0) {
  }

  void setHuffLengths(int nlen_, int ndist_, int ncode_) {
    nlen = nlen_;
    ndist = ndist_;
    ncode = ncode_;
  }

  void addTreeCode(int code) {
    treecodes.push_back(code);
  }

  void addToken(const PreflateToken &token) {
    tokens.push_back(token);
  }
};
bool isEqual(const PreflateTokenBlock &, const PreflateTokenBlock &);
uint bitLength(uint value);
uint bitReverse(const uint value, const uint bits);
uint bitLeadingZeroes(const uint value);
uint bitTrailingZeroes(const uint value);
class InputStream {
public:
virtual ~InputStream() {
}

virtual bool eof() const = 0;
virtual size_t read(byte* buffer, const size_t size) = 0;
};
class OutputStream {
public:
virtual ~OutputStream() {
}

virtual size_t write(const byte* buffer, const size_t size) = 0;
};
class SeekableStream {
public:
virtual ~SeekableStream() {
}

virtual uint64_t tell() const = 0;
virtual uint64_t seek(const uint64_t newPos) = 0;
};
class SeekableInputStream : public InputStream, public SeekableStream {};
class SeekableInputOutputStream : public SeekableInputStream, public OutputStream {};
class BitInputStream {
public:
BitInputStream(InputStream &);
bool eof() const {
  return _eof&&_bufPos==_bufSize&&!_bitsRemaining;
}

size_t bitPos() const {
  return _totalBitPos;
}

size_t peek(const uint n) {
  if( _bitsRemaining<n ) {
    _fill();
  }
  return _bits&((1<<n)-1);
}

void skip(const uint n) {
  _bitsRemaining -= std::min(n, _bitsRemaining);
  _bits >>= n;
  _totalBitPos += n;
}

size_t get(const uint n) {
  size_t v = peek(n);
  skip(n);
  return v;
}

size_t getReverse(const uint n) {
  return bitReverse(get(n), n);
}

void skipToByte() {
  skip(_bitsRemaining&7);
}

bool checkLastBitsOfByteAreZero() {
  return peek(_bitsRemaining&7)==0;
}

void fastFill(const uint n) {
  if( _bitsRemaining<n ) {
    _fill();
  }
}

size_t fastPeek(const uint n) {
  return _bits&((1<<n)-1);
}

size_t fastGet(const uint n) {
  size_t v = fastPeek(n);
  skip(n);
  return v;
}

size_t copyBytesTo(OutputStream &output, const size_t len);
size_t getBytes(uint8_t* data, const size_t size);
uint64_t getVLI();

private:
void _fillBytes();
void _fill();
enum { BUF_SIZE = 1024, PRE_BUF_EXTRA = 16, BITS = sizeof(size_t)*8 };
InputStream &_input;
byte _buffer[PRE_BUF_EXTRA+BUF_SIZE];
uint _bufPos, _bufSize, _bufFastLimit;
bool _eof;
size_t _bits;
uint _bitsRemaining;
size_t _totalBitPos;
};
class BitOutputStream {
public:
BitOutputStream(OutputStream &);
void put(const size_t value, const uint n) {
  if( _bitPos+n>=BITS ) {
    _flush();
  }
  _bits |= (value&((1<<n)-1))<<_bitPos;
  _bitPos += n;
}

void putReverse(const size_t value, const uint n) {
  put(bitReverse(value, n), n);
}

void fillByte() {
  _bitPos = (_bitPos+7)&~7;
}

void flush();
uint bitPos() const {
  return _bitPos;
}

void putBytes(const uint8_t* data, const size_t size);
void putVLI(const uint64_t size);

private:
void _flush();
enum { BUF_SIZE = 1024, BUF_EXTRA = 64, BITS = sizeof(size_t)*8 };
OutputStream &_output;
byte _buffer[BUF_SIZE+BUF_EXTRA];
uint _bufPos;
size_t _bits;
uint _bitPos;
};
class HuffmanDecoder {
public:
HuffmanDecoder(const byte* symbolBitLengths, const size_t symbolCount, const bool disableZeroBitSymbols, const byte maxBitsPerTable);
bool error() const {
  return _error;
}

size_t decode(BitInputStream &bis) const {
  size_t v = bis.peek(_table0.peekBits);
  int16_t w = _table0.lookup[v];
  if( w>=0 ) {
    bis.skip(w&0xf);
    return w>>4;
  }
  return _decodeDeeper(bis, ~w);
}

private:
size_t _decodeDeeper(BitInputStream &bis, const size_t tableId) const;
bool _constructTables(const byte* symbolBitLengths, const size_t symbolCount, const bool disableZeroBitSymbols, const byte maxBitsPerTable);
void _constructErrorTable();

private:
struct Table {
  byte peekBits;
  std::vector<int16_t> lookup;
};
Table _table0;
std::vector<Table> _tables;
bool _error;
};
class OutputCacheStream : public OutputStream {
public:
OutputCacheStream(OutputStream &os);
virtual ~OutputCacheStream();
size_t write(const byte* buffer, const size_t size) {
  _cache.insert(_cache.end(), buffer, buffer+size);
  return size;
}

void reserve(const size_t len) {
  size_t cap = _cache.capacity();
  if( _cache.size()+len>cap ) {
    _cache.reserve(cap+std::max(cap>>1, len));
  }
}

void flush() {
  flushUpTo(cacheEndPos());
}

void flushUpTo(const uint64_t newStartPos);
uint64_t cacheStartPos() const {
  return _cacheStartPos;
}

uint64_t cacheEndPos() const {
  return _cacheStartPos+_cache.size();
}

const byte*cacheData(const uint64_t pos) const {
  return _cache.data()+(std::ptrdiff_t)(pos-_cacheStartPos);
}

const byte*cacheEnd() const {
  return _cache.data()+_cache.size();
}

const size_t cacheSize() const {
  return _cache.size();
}

private:
OutputStream &_os;
std::vector<byte> _cache;
uint64_t _cacheStartPos;
};
class PreflateBlockDecoder {
public:
enum ErrorCode { OK, STORED_BLOCK_LEN_MISMATCH, STORED_BLOCK_PADDING_MISMATCH, BADLY_CODED_MAX_LENGTH };
PreflateBlockDecoder(BitInputStream &input, OutputCacheStream &output);
bool readBlock(PreflateTokenBlock &, bool &last);
ErrorCode status() const {
  return _errorCode;
}

private:
bool _error(const ErrorCode);
byte _readBit() {
  return _input.get(1);
}

uint _readBits(const uint bits) {
  return _input.get(bits);
}

void _skipToByte() {
  _input.skipToByte();
}

bool _checkLastBitsOfByte() {
  return _input.checkLastBitsOfByteAreZero();
}

void _writeLiteral(const byte l) {
  _output.write(&l, 1);
}

void _writeReference(const size_t dist, const size_t len) {
  _output.reserve(len);
  if( len<=dist ) {
    _output.write(_output.cacheEnd()-dist, len);
  } else {
    const uint8_t* ptr = _output.cacheEnd()-dist;
    for( size_t i = 0; i<len; ++i ) {
      _output.write(&ptr[i], 1);
    }
  }
}

void _setupStaticTables();
bool _readDynamicTables(PreflateTokenBlock &);
BitInputStream &_input;
OutputCacheStream &_output;
ErrorCode _errorCode;
const HuffmanDecoder* _litLenDecoder;
const HuffmanDecoder* _distDecoder;
HuffmanDecoder _dynamicLitLenDecoder;
HuffmanDecoder _dynamicDistDecoder;
};
class HuffmanEncoder {
public:
HuffmanEncoder(const byte* symbolBitLengths, const uint symbolCount, const bool disableZeroBitSymbols);
bool error() const {
  return _error;
}

void encode(BitOutputStream &bos, const uint symbol) const {
  uint v = _lookup[symbol];
  bos.put(v>>5, v&0x1f);
}

private:
bool _constructTables(const byte* symbolBitLengths, const uint symbolCount, const bool disableZeroBitSymbols);
void _constructErrorTable(const uint symbolCount);

private:
std::vector<uint> _lookup;
bool _error;
};
struct PreflateBlockTrees {
  static const HuffmanDecoder*staticLitLenTreeDecoder();
  static const HuffmanDecoder*staticDistTreeDecoder();
  static const HuffmanEncoder*staticLitLenTreeEncoder();
  static const HuffmanEncoder*staticDistTreeEncoder();
};
PreflateBlockDecoder::PreflateBlockDecoder(BitInputStream &input, OutputCacheStream &output) : _input(input), _output(output), _errorCode(OK), _dynamicLitLenDecoder(nullptr, 0, false, 0), _dynamicDistDecoder(nullptr, 0, false, 0) {
}

bool PreflateBlockDecoder::_error(const ErrorCode code) {
  _errorCode = code;
  return false;
}

bool PreflateBlockDecoder::readBlock(PreflateTokenBlock &block, bool &last) {
  block.uncompressedStartPos = _output.cacheEndPos();
  int32_t earliest_reference = INT32_MAX, curPos = 0;
  if( _input.eof() ) {
    return false;
  }
  last = _readBit()!=0;
  byte mode = _readBits(2);
  switch( mode ) {
  default:
    return false;
  case 0: {
    block.type = PreflateTokenBlock::STORED;
    block.paddingBitCount = (-_input.bitPos())&7;
    block.paddingBits = _input.get(block.paddingBitCount);
    size_t len = _readBits(16);
    size_t ilen = _readBits(16);
    if( (len^ilen)!=0xffff ) {
      return _error(STORED_BLOCK_LEN_MISMATCH);
    }
    block.uncompressedLen = len;
    block.contextLen = 0;
    return _input.copyBytesTo(_output, len)==len;
  }
  case 1:
  case 2:
    if( mode==1 ) {
      block.type = PreflateTokenBlock::STATIC_HUFF;
      _setupStaticTables();
    } else {
      block.type = PreflateTokenBlock::DYNAMIC_HUFF;
      if( !_readDynamicTables(block) ) {
        return false;
      }
    }
    while( true ) {
      if( _input.eof() ) {
        return false;
      }
      uint litLen = _litLenDecoder->decode(_input);
      if( litLen<256 ) {
        _writeLiteral(litLen);
        block.tokens.push_back(PreflateToken(PreflateToken::LITERAL));
        curPos++;
      } else if( litLen==256 ) {
        block.uncompressedLen = _output.cacheEndPos()-block.uncompressedStartPos;
        block.contextLen = -earliest_reference;
        return true;
      } else {
        uint lcode = litLen-PreflateConstants::NONLEN_CODE_COUNT;
        if( lcode>=PreflateConstants::LEN_CODE_COUNT ) {
          return false;
        }
        uint len = PreflateConstants::MIN_MATCH+PreflateConstants::lengthBaseTable[lcode]+_readBits(PreflateConstants::lengthExtraTable[lcode]);
        bool irregular258 = len==258&&lcode!=PreflateConstants::LEN_CODE_COUNT-1;
        uint dcode = _distDecoder->decode(_input);
        if( dcode>=PreflateConstants::DIST_CODE_COUNT ) {
          return false;
        }
        uint dist = 1+PreflateConstants::distBaseTable[dcode]+_readBits(PreflateConstants::distExtraTable[dcode]);
        if( dist>_output.cacheEndPos() ) {
          return false;
        }
        _writeReference(dist, len);
        block.tokens.push_back(PreflateToken(PreflateToken::REFERENCE, len, dist, irregular258));
        earliest_reference = std::min(earliest_reference, curPos-(int32_t)dist);
        curPos += len;
      }
    }
  }
}

void PreflateBlockDecoder::_setupStaticTables() {
  _litLenDecoder = PreflateBlockTrees::staticLitLenTreeDecoder();
  _distDecoder = PreflateBlockTrees::staticDistTreeDecoder();
}

bool PreflateBlockDecoder::_readDynamicTables(PreflateTokenBlock &block) {
  block.nlen = PreflateConstants::NONLEN_CODE_COUNT+_readBits(5);
  block.ndist = 1+_readBits(5);
  block.ncode = 4+_readBits(4);
  if( block.nlen>PreflateConstants::LITLEN_CODE_COUNT||block.ndist>PreflateConstants::DIST_CODE_COUNT ) {
    return false;
  }
  block.treecodes.clear();
  block.treecodes.reserve(block.nlen+block.ndist+block.ncode);
  byte tcBitLengths[PreflateConstants::CODETREE_CODE_COUNT];
  byte ldBitLengths[PreflateConstants::LITLENDIST_CODE_COUNT];
  memset(tcBitLengths, 0, sizeof(tcBitLengths));
  memset(ldBitLengths, 0, sizeof(ldBitLengths));
  for( uint i = 0, n = block.ncode; i<n; ++i ) {
    byte tc = _readBits(3);
    block.treecodes.push_back(tc);
    tcBitLengths[PreflateConstants::treeCodeOrderTable[i]] = tc;
  }
  HuffmanDecoder tcTree(tcBitLengths, PreflateConstants::CODETREE_CODE_COUNT, true, 7);
  if( tcTree.error() ) {
    return false;
  }
  for( uint i = 0, n = block.nlen+block.ndist; i<n; ++i ) {
    byte code = tcTree.decode(_input);
    if( code>18 ) {
      return false;
    }
    block.treecodes.push_back(code);
    if( code<16 ) {
      ldBitLengths[i] = code;
      continue;
    }
    byte len = 0, tocopy = 0;
    switch( code ) {
    case 16:
      if( i==0 ) {
        return false;
      }
      tocopy = ldBitLengths[i-1];
      len = 3+_readBits(2);
      break;
    case 17:
      tocopy = 0;
      len = 3+_readBits(3);
      break;
    case 18:
      tocopy = 0;
      len = 11+_readBits(7);
      break;
    }
    if( i+len>n ) {
      return false;
    }
    block.treecodes.push_back(len);
    memset(ldBitLengths+i, tocopy, len);
    i += len-1;
  }
  if( !ldBitLengths[256] ) {
    return false;
  }
  _dynamicLitLenDecoder = HuffmanDecoder(ldBitLengths, block.nlen, true, 15);
  if( _dynamicLitLenDecoder.error() ) {
    return false;
  }
  _litLenDecoder = &_dynamicLitLenDecoder;
  _dynamicDistDecoder = HuffmanDecoder(ldBitLengths+block.nlen, block.ndist, true, 15);
  if( _dynamicDistDecoder.error() ) {
    return false;
  }
  _distDecoder = &_dynamicDistDecoder;
  return true;
}

class PreflateBlockReencoder {
public:
enum ErrorCode {
  OK,
  LITERAL_OUT_OF_BOUNDS,
  TREE_OUT_OF_RANGE,
  BAD_CODE_TREE,
  BAD_LD_TREE,
};
PreflateBlockReencoder(BitOutputStream &bos, const std::vector<byte> &uncompressedData, const size_t uncompressedOffset);
bool writeBlock(const PreflateTokenBlock &, const bool last);
void flush();
ErrorCode status() const {
  return _errorCode;
}

private:
bool _error(const ErrorCode);
void _setupStaticTables();
bool _buildAndWriteDynamicTables(const PreflateTokenBlock &);
bool _writeTokens(const std::vector<PreflateToken> &tokens);
BitOutputStream &_output;
const std::vector<byte> &_uncompressedData;
size_t _uncompressedDataPos;
ErrorCode _errorCode;
const HuffmanEncoder* _litLenEncoder;
const HuffmanEncoder* _distEncoder;
HuffmanEncoder _dynamicLitLenEncoder;
HuffmanEncoder _dynamicDistEncoder;
};
PreflateBlockReencoder::PreflateBlockReencoder(BitOutputStream &bos, const std::vector<byte> &uncompressedData, const size_t uncompressedOffset) : _output(bos), _uncompressedData(uncompressedData), _uncompressedDataPos(uncompressedOffset), _errorCode(OK), _dynamicLitLenEncoder(nullptr, 0, false), _dynamicDistEncoder(nullptr, 0, false) {
}

bool PreflateBlockReencoder::_error(const ErrorCode code) {
  _errorCode = code;
  return false;
}

void PreflateBlockReencoder::_setupStaticTables() {
  _litLenEncoder = PreflateBlockTrees::staticLitLenTreeEncoder();
  _distEncoder = PreflateBlockTrees::staticDistTreeEncoder();
}

bool PreflateBlockReencoder::_buildAndWriteDynamicTables(const PreflateTokenBlock &block) {
  if( block.ncode<4||block.ncode>PreflateConstants::CODETREE_CODE_COUNT||block.treecodes.size()<(size_t)block.ncode||block.nlen<PreflateConstants::NONLEN_CODE_COUNT||block.nlen>PreflateConstants::LITLEN_CODE_COUNT||block.ndist<1||block.ndist>PreflateConstants::DIST_CODE_COUNT ) {
    return _error(TREE_OUT_OF_RANGE);
  }
  byte tcBitLengths[PreflateConstants::CODETREE_CODE_COUNT];
  byte ldBitLengths[PreflateConstants::LITLENDIST_CODE_COUNT];
  memset(tcBitLengths, 0, sizeof(tcBitLengths));
  memset(ldBitLengths, 0, sizeof(ldBitLengths));
  for( uint i = 0, n = block.ncode; i<n; ++i ) {
    byte tc = block.treecodes[i];
    _output.put(tc, 3);
    tcBitLengths[PreflateConstants::treeCodeOrderTable[i]] = tc;
  }
  HuffmanEncoder tcTree(tcBitLengths, PreflateConstants::CODETREE_CODE_COUNT, true);
  if( tcTree.error() ) {
    return _error(BAD_CODE_TREE);
  }
  uint o = 0, maxo = block.nlen+block.ndist;
  for( auto i = block.treecodes.begin()+block.ncode, e = block.treecodes.end(); i!=e; ++i ) {
    byte code = *i;
    if( code>18 ) {
      return _error(BAD_LD_TREE);
    }
    tcTree.encode(_output, code);
    if( code<16 ) {
      if( o>=maxo ) {
        return _error(BAD_LD_TREE);
      }
      ldBitLengths[o++] = code;
      continue;
    }
    if( i+1==e ) {
      return _error(BAD_LD_TREE);
    }
    if( code==16&&o==0 ) {
      return _error(BAD_LD_TREE);
    }
    byte len = *++i;
    byte tocopy = code==16 ? ldBitLengths[o-1] : 0;
    static byte repExtraBits[3] = {2, 3, 7};
    static byte repOffset[3] = {3, 3, 11};
    _output.put(len-repOffset[code-16], repExtraBits[code-16]);
    if( o+len>maxo ) {
      return _error(BAD_LD_TREE);
    }
    memset(ldBitLengths+o, tocopy, len);
    o += len;
  }
  if( o!=maxo ) {
    return _error(BAD_LD_TREE);
  }
  if( !ldBitLengths[256] ) {
    return _error(BAD_LD_TREE);
  }
  _dynamicLitLenEncoder = HuffmanEncoder(ldBitLengths, block.nlen, true);
  if( _dynamicLitLenEncoder.error() ) {
    return _error(BAD_LD_TREE);
  }
  _litLenEncoder = &_dynamicLitLenEncoder;
  _dynamicDistEncoder = HuffmanEncoder(ldBitLengths+block.nlen, block.ndist, true);
  if( _dynamicDistEncoder.error() ) {
    return _error(BAD_LD_TREE);
  }
  _distEncoder = &_dynamicDistEncoder;
  return true;
}

bool PreflateBlockReencoder::_writeTokens(const std::vector<PreflateToken> &tokens) {
  for( size_t i = 0; i<tokens.size(); ++i ) {
    PreflateToken token = tokens[i];
    if( token.len==1 ) {
      if( _uncompressedDataPos>=_uncompressedData.size() ) {
        return _error(LITERAL_OUT_OF_BOUNDS);
      }
      byte literal = _uncompressedData[_uncompressedDataPos++];
      _litLenEncoder->encode(_output, literal);
    } else {
      if( token.irregular258 ) {
        _litLenEncoder->encode(_output, PreflateConstants::LITLEN_CODE_COUNT-2);
        _output.put(31, 5);
      } else {
        uint lencode = PreflateConstants::LCode(token.len);
        _litLenEncoder->encode(_output, PreflateConstants::NONLEN_CODE_COUNT+lencode);
        uint lenextra = PreflateConstants::lengthExtraTable[lencode];
        if( lenextra ) {
          _output.put(token.len-PreflateConstants::MIN_MATCH-PreflateConstants::lengthBaseTable[lencode], lenextra);
        }
      }
      uint distcode = PreflateConstants::DCode(token.dist);
      _distEncoder->encode(_output, distcode);
      uint distextra = PreflateConstants::distExtraTable[distcode];
      if( distextra ) {
        _output.put(token.dist-1-PreflateConstants::distBaseTable[distcode], distextra);
      }
      _uncompressedDataPos += token.len;
    }
  }
  _litLenEncoder->encode(_output, PreflateConstants::LITERAL_COUNT);
  return true;
}

bool PreflateBlockReencoder::writeBlock(const PreflateTokenBlock &block, bool last) {
  if( status()!=OK ) {
    return false;
  }
  _output.put(last, 1);
  switch( block.type ) {
  case PreflateTokenBlock::DYNAMIC_HUFF:
    _output.put(2, 2);
    _output.put(block.nlen-PreflateConstants::NONLEN_CODE_COUNT, 5);
    _output.put(block.ndist-1, 5);
    _output.put(block.ncode-4, 4);
    if( !_buildAndWriteDynamicTables(block) ) {
      return false;
    }
    if( !_writeTokens(block.tokens) ) {
      return false;
    }
    break;
  case PreflateTokenBlock::STATIC_HUFF:
    _output.put(1, 2);
    _setupStaticTables();
    if( !_writeTokens(block.tokens) ) {
      return false;
    }
    break;
  case PreflateTokenBlock::STORED:
    _output.put(0, 2);
    _output.put(block.paddingBits, block.paddingBitCount);
    _output.fillByte();
    _output.put(block.uncompressedLen, 16);
    _output.put(~block.uncompressedLen, 16);
    if( _uncompressedDataPos+block.uncompressedLen>_uncompressedData.size() ) {
      return _error(LITERAL_OUT_OF_BOUNDS);
    }
    _output.putBytes(_uncompressedData.data()+_uncompressedDataPos, block.uncompressedLen);
    _uncompressedDataPos += block.uncompressedLen;
    break;
  }
  return true;
}

void PreflateBlockReencoder::flush() {
  _output.flush();
}

static HuffmanDecoder* staticLitLenDecoder;
static HuffmanDecoder* staticDistDecoder;
static HuffmanEncoder* staticLitLenEncoder;
static HuffmanEncoder* staticDistEncoder;
static void setLitLenBitLengths(byte (&a)[288]) {
  std::fill(a+0, a+144, 8);
  std::fill(a+144, a+256, 9);
  std::fill(a+256, a+280, 7);
  std::fill(a+280, a+288, 8);
}

static void setDistBitLengths(byte (&a)[32]) {
  std::fill(a, a+32, 5);
}

const HuffmanDecoder*PreflateBlockTrees::staticLitLenTreeDecoder() {
  if( !staticLitLenDecoder ) {
    byte l_lengths[288];
    setLitLenBitLengths(l_lengths);
    staticLitLenDecoder = new HuffmanDecoder(l_lengths, 288, true, 15);
  }
  return staticLitLenDecoder;
}

const HuffmanDecoder*PreflateBlockTrees::staticDistTreeDecoder() {
  if( !staticDistDecoder ) {
    byte d_lengths[32];
    setDistBitLengths(d_lengths);
    staticDistDecoder = new HuffmanDecoder(d_lengths, 32, true, 15);
  }
  return staticDistDecoder;
}

const HuffmanEncoder*PreflateBlockTrees::staticLitLenTreeEncoder() {
  if( !staticLitLenEncoder ) {
    byte l_lengths[288];
    setLitLenBitLengths(l_lengths);
    staticLitLenEncoder = new HuffmanEncoder(l_lengths, 288, true);
  }
  return staticLitLenEncoder;
}

const HuffmanEncoder*PreflateBlockTrees::staticDistTreeEncoder() {
  if( !staticDistEncoder ) {
    byte d_lengths[32];
    setDistBitLengths(d_lengths);
    staticDistEncoder = new HuffmanEncoder(d_lengths, 32, true);
  }
  return staticDistEncoder;
}

struct PreflateParserConfig {
  byte good_length;
  uint16_t max_lazy;
  uint16_t nice_length;
  uint16_t max_chain;
};
extern const PreflateParserConfig fastPreflateParserSettings[3];
extern const PreflateParserConfig slowPreflateParserSettings[6];
struct SeqChainEntry {
  uint16_t distToNext;
  uint16_t length;
};
struct PreflateSeqIterator {
  const SeqChainEntry* chain;
  const uint refPos;
  uint curDist;
  PreflateSeqIterator(const SeqChainEntry* chain_, const uint refPos_) : chain(chain_), refPos(refPos_), curDist(chain_[refPos_].distToNext) {
  }

  inline bool valid() const {
    return curDist<=refPos-8;
  }

  inline bool operator!() const {
    return !valid();
  }

  inline uint dist() const {
    return curDist;
  }

  inline uint16_t len() const {
    return chain[refPos-curDist].length;
  }

  inline bool next() {
    curDist += chain[refPos-curDist].distToNext;
    return valid();
  }
};
struct PreflateSeqChain {
  PreflateInput _input;
  SeqChainEntry* prev;
  uint totalShift;
  uint curPos;
  uint16_t heads[256];
  PreflateSeqChain(const std::vector<byte> &input_);
  ~PreflateSeqChain();
  bool valid(const uint refPos) const {
    return prev[refPos-totalShift].distToNext!=0xffff;
  }

  uint16_t len(const uint refPos) const {
    return prev[refPos-totalShift].length;
  }

  PreflateSeqIterator iterateFromPos(const uint refPos) const {
    return PreflateSeqIterator(prev, refPos-totalShift);
  }

  void updateSeq(const uint l) {
    curPos += l;
    while( curPos-totalShift>=0xfe08 ) {
      _reshift();
    }
  }

private:
  void _reshift();
  void _build(const uint off0, const uint size);
};
struct PreflatePreviousMatchInfo {
  PreflateToken previousMatches[256];
};
struct PreflateNextMatchInfo {
  uint16_t nextChainDepth;
  uint16_t nextLen;
  uint16_t nextDist;
};
struct PreflateRematchInfo {
  uint16_t firstMatchDepth;
  uint16_t firstMatchDist;
  uint16_t requestedMatchDepth;
  uint16_t condensedHops;
};
struct PreflatePredictorState {
  const PreflateHashChainExt &hash;
  const PreflateSeqChain &seq;
  uint16_t windowBytes;
  uint maxTokenCount;
  const PreflateParserConfig &config;
  PreflatePredictorState(const PreflateHashChainExt &, const PreflateSeqChain &, const PreflateParserConfig &, const int wbits, const int mbits);
  uint currentInputPos() const {
    return hash.input().pos();
  }

  const byte*inputCursor() const {
    return hash.input().curChars();
  }

  uint windowSize() const {
    return windowBytes;
  }

  uint totalInputSize() const {
    return hash.input().size();
  }

  uint availableInputSize() const {
    return hash.input().remaining();
  }

  uint maxChainLength() const {
    return config.max_chain;
  }

  uint niceMatchLength() const {
    return config.nice_length;
  }

  uint goodMatchLength() const {
    return config.good_length;
  }

  uint lazyMatchLength() const {
    return config.max_lazy;
  }

  uint calculateHash() const {
    return hash.curHash();
  }

  uint calculateHashNext() const {
    return hash.curPlus1Hash();
  }

  uint getCurrentHashHead(const uint hashNext) const {
    return hash.getHead(hashNext);
  }

  PreflateHashIterator iterateFromHead(const uint hash_, const uint refPos, const uint maxDist) const {
    return hash.iterateFromHead(hash_, refPos, maxDist);
  }

  PreflateHashIterator iterateFromNode(const uint node_, const uint refPos, const uint maxDist) const {
    return hash.iterateFromNode(node_, refPos, maxDist);
  }

  PreflateHashIterator iterateFromDist(const uint dist_, const uint refPos, const uint maxDist) const {
    return hash.iterateFromPos(refPos-dist_, refPos, maxDist);
  }

  static uint prefixCompare(const byte* s1, const byte* s2, const uint bestLen, const uint maxLen);
  static uint suffixCompare(const byte* s1, const byte* s2, const uint bestLen, const uint maxLen);
  bool betterMatchPossible(const uint prevLen, const uint startPos);
  PreflateToken matchHop0MaxDist(const uint hashHead, const uint prevLen, const uint offset, const bool veryFarMatches, const bool matchesToStart);
  PreflateToken match(const uint hashHead, const uint prevLen, const uint offset, const bool veryFarMatches, const bool matchesToStart, const uint maxDepth);
  PreflateToken seqMatch(const uint startPos, const uint hashHead, const uint prevLen, const bool veryFarMatches, const bool matchesToStart, const uint maxDepth);
  PreflateNextMatchInfo nextMatchInfo(const uint hashHead, const PreflateToken &targetReference, const PreflateHashChainExt &);
  PreflateRematchInfo rematchInfo(const uint hashHead, const PreflateToken &targetReference);
  uint firstMatch(const uint len);
  uint hopMatch(const PreflateToken &token, const uint hops);

private:
  struct MatchHelper {
    uint startPos;
    uint maxLen;
    uint curMaxDistHop0;
    uint curMaxDistHop1Plus;
    uint maxChain;
    uint niceLen;
    bool validHop0Dist(const uint d) const {
      return d<=curMaxDistHop0;
    }

    bool validHop1PlusDist(const uint d) const {
      return d<=curMaxDistHop1Plus;
    }
  };
  bool createMatchHelper(MatchHelper &helper, const uint prevLen, const uint startPos, const bool veryFarMatches, const bool matchesToStart, const uint maxDepth);
};
struct PreflateCompLevelInfo {
  uint possibleCompressionLevels;
  uint recommendedCompressionLevel;
  bool zlibCompatible;
  uint referenceCount;
  uint unfoundReferences;
  uint16_t maxChainDepth;
  uint16_t longestLen3Dist;
  uint16_t longestDistAtHop0;
  uint16_t longestDistAtHop1Plus;
  bool matchToStart;
  bool veryFarMatches;
  bool farLen3Matches;
};
struct PreflateCompLevelEstimatorState {
  PreflateHashChainExt slowHash;
  PreflateHashChainExt fastL1Hash;
  PreflateHashChainExt fastL2Hash;
  PreflateHashChainExt fastL3Hash;
  const std::vector<PreflateTokenBlock> &blocks;
  PreflateCompLevelInfo info;
  uint16_t wsize;
  size_t off0;
  PreflateCompLevelEstimatorState(const int wbits, const int mbits, const std::vector<byte> &unpacked_output, const size_t off0, const std::vector<PreflateTokenBlock> &blocks);
  void updateHash(const uint len);
  void updateOrSkipHash(const uint len);
  void checkMatch(const PreflateToken &token);
  void checkDump(bool early_out);
  void recommend();

private:
  void updateOrSkipSingleFastHash(PreflateHashChainExt &, const uint len, const PreflateParserConfig &);
  bool checkMatchSingleFastHash(const PreflateToken &token, const PreflateHashChainExt &, const PreflateParserConfig &, const uint hashHead);
  uint16_t matchDepth(const uint hashHead, const PreflateToken &targetReference, const PreflateHashChainExt &hash);
  uint windowSize() const {
    return wsize;
  }
};
PreflateCompLevelInfo estimatePreflateCompLevel(const int wbits, const int mbits, const std::vector<byte> &unpacked_output, const size_t off0, const std::vector<PreflateTokenBlock> &blocks, const bool early_out);
PreflateCompLevelEstimatorState::PreflateCompLevelEstimatorState(const int wbits, const int mbits, const std::vector<byte> &unpacked_output_, const size_t off0_, const std::vector<PreflateTokenBlock> &blocks_) : slowHash(unpacked_output_, mbits), fastL1Hash(unpacked_output_, mbits), fastL2Hash(unpacked_output_, mbits), fastL3Hash(unpacked_output_, mbits), blocks(blocks_), wsize(1<<wbits), off0(off0_) {
  memset(&info, 0, sizeof(info));
  info.possibleCompressionLevels = (1<<10)-(1<<1);
  updateHash(off0);
}

void PreflateCompLevelEstimatorState::updateHash(const uint len) {
  if( info.possibleCompressionLevels&(1<<1) ) {
    fastL1Hash.updateHash(len);
  }
  if( info.possibleCompressionLevels&(1<<2) ) {
    fastL2Hash.updateHash(len);
  }
  if( info.possibleCompressionLevels&(1<<3) ) {
    fastL3Hash.updateHash(len);
  }
  slowHash.updateHash(len);
}

void PreflateCompLevelEstimatorState::updateOrSkipSingleFastHash(PreflateHashChainExt &hash, const uint len, const PreflateParserConfig &config) {
  if( len<=config.max_lazy ) {
    hash.updateHash(len);
  } else {
    hash.skipHash(len);
  }
}

void PreflateCompLevelEstimatorState::updateOrSkipHash(const uint len) {
  if( info.possibleCompressionLevels&(1<<1) ) {
    updateOrSkipSingleFastHash(fastL1Hash, len, fastPreflateParserSettings[0]);
  }
  if( info.possibleCompressionLevels&(1<<2) ) {
    updateOrSkipSingleFastHash(fastL2Hash, len, fastPreflateParserSettings[1]);
  }
  if( info.possibleCompressionLevels&(1<<3) ) {
    updateOrSkipSingleFastHash(fastL3Hash, len, fastPreflateParserSettings[2]);
  }
  slowHash.updateHash(len);
}

uint16_t PreflateCompLevelEstimatorState::matchDepth(const uint hashHead, const PreflateToken &targetReference, const PreflateHashChainExt &hash) {
  uint curPos = hash.input().pos();
  uint curMaxDist = std::min(curPos, windowSize());
  uint startDepth = hash.getNodeDepth(hashHead);
  PreflateHashIterator chainIt = hash.iterateFromPos(curPos-targetReference.dist, curPos, curMaxDist);
  if( !chainIt.curPos||targetReference.dist>curMaxDist ) {
    return 0xffffu;
  }
  uint endDepth = chainIt.depth();
  return std::min(startDepth-endDepth, 0xffffu);
}

bool PreflateCompLevelEstimatorState::checkMatchSingleFastHash(const PreflateToken &token, const PreflateHashChainExt &hash, const PreflateParserConfig &config, const uint hashHead) {
  uint mdepth = matchDepth(hash.getHead(hashHead), token, hash);
  if( mdepth>config.max_chain ) {
    return false;
  }
  return true;
}

void PreflateCompLevelEstimatorState::checkMatch(const PreflateToken &token) {
  uint hashHead = slowHash.curHash();
  if( slowHash.input().pos()>=token.dist+off0 ) {
    if( info.possibleCompressionLevels&(1<<1) ) {
      if( !checkMatchSingleFastHash(token, fastL1Hash, fastPreflateParserSettings[0], hashHead) ) {
        info.possibleCompressionLevels &= ~(1<<1);
      }
    }
    if( info.possibleCompressionLevels&(1<<2) ) {
      if( !checkMatchSingleFastHash(token, fastL2Hash, fastPreflateParserSettings[1], hashHead) ) {
        info.possibleCompressionLevels &= ~(1<<2);
      }
    }
    if( info.possibleCompressionLevels&(1<<3) ) {
      if( !checkMatchSingleFastHash(token, fastL3Hash, fastPreflateParserSettings[2], hashHead) ) {
        info.possibleCompressionLevels &= ~(1<<3);
      }
    }
  }
  if( slowHash.input().pos()>=token.dist ) {
    info.referenceCount++;
    uint16_t mdepth = matchDepth(slowHash.getHead(hashHead), token, slowHash);
    if( mdepth>=0x8001 ) {
      info.unfoundReferences++;
    } else {
      info.maxChainDepth = std::max(info.maxChainDepth, mdepth);
    }
    if( token.dist==slowHash.input().pos() ) {
      info.matchToStart = true;
    }
    if( mdepth==0 ) {
      info.longestDistAtHop0 = std::max(info.longestDistAtHop0, token.dist);
    } else {
      info.longestDistAtHop1Plus = std::max(info.longestDistAtHop1Plus, token.dist);
    }
    if( token.len==3 ) {
      info.longestLen3Dist = std::max(info.longestLen3Dist, token.dist);
    }
    if( info.possibleCompressionLevels&((1<<10)-(1<<4)) ) {
      for( uint i = 0; i<6; ++i ) {
        if( !(info.possibleCompressionLevels&(1<<(4+i))) ) {
          continue;
        }
        const PreflateParserConfig &config = slowPreflateParserSettings[i];
        if( mdepth>config.max_chain ) {
          info.possibleCompressionLevels &= ~(1<<(4+i));
        }
      }
    }
  }
}

void PreflateCompLevelEstimatorState::checkDump(bool early_out) {
  for( uint i = 0, n = blocks.size(); i<n; ++i ) {
    const PreflateTokenBlock &b = blocks[i];
    if( b.type==PreflateTokenBlock::STORED ) {
      updateHash(b.uncompressedLen);
      continue;
    }
    for( uint j = 0, m = b.tokens.size(); j<m; ++j ) {
      const PreflateToken &t = b.tokens[j];
      if( t.len==1 ) {
        updateHash(1);
      } else {
        checkMatch(t);
        updateOrSkipHash(t.len);
      }
      if( early_out&&(info.possibleCompressionLevels&(info.possibleCompressionLevels-1))==0 ) {
        return;
      }
    }
  }
}

void PreflateCompLevelEstimatorState::recommend() {
  info.recommendedCompressionLevel = 9;
  info.veryFarMatches = !(info.longestDistAtHop0<=windowSize()-PreflateConstants::MIN_LOOKAHEAD&&info.longestDistAtHop1Plus<windowSize()-PreflateConstants::MIN_LOOKAHEAD);
  info.farLen3Matches = info.longestLen3Dist>4096;
  info.zlibCompatible = info.possibleCompressionLevels>1&&!info.matchToStart&&!info.veryFarMatches&&(!info.farLen3Matches||(info.possibleCompressionLevels&0xe)!=0);
  if( info.unfoundReferences ) {
    return;
  }
  if( info.zlibCompatible&&info.possibleCompressionLevels>1 ) {
    uint l = info.possibleCompressionLevels>>1;
    info.recommendedCompressionLevel = 1;
    while( (l&1)==0 ) {
      info.recommendedCompressionLevel++;
      l >>= 1;
    }
    return;
  }
  for( int i = 0; i<6; ++i ) {
    const PreflateParserConfig &config = slowPreflateParserSettings[i];
    if( info.maxChainDepth<=config.max_chain ) {
      info.recommendedCompressionLevel = 4+i;
      return;
    }
  }
}

PreflateCompLevelInfo estimatePreflateCompLevel(const int wbits, const int mbits, const std::vector<byte> &unpacked_output, const size_t off0, const std::vector<PreflateTokenBlock> &blocks, const bool early_out) {
  PreflateCompLevelEstimatorState state(wbits, mbits, unpacked_output, off0, blocks);
  state.checkDump(early_out);
  state.recommend();
  return state.info;
}

const byte PreflateConstants::distCodeTable[512] = {0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
                                                             14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0, 0, 16, 17, 18, 18, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
                                                             26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29};
const byte PreflateConstants::lengthCodeTable[MAX_MATCH-MIN_MATCH+1] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
                                                                                 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28};
const byte PreflateConstants::lengthBaseTable[LEN_CODE_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 255};
const uint16_t PreflateConstants::distBaseTable[DIST_CODE_COUNT] = {0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576};
const byte PreflateConstants::lengthExtraTable[LEN_CODE_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const byte PreflateConstants::distExtraTable[DIST_CODE_COUNT] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const byte PreflateConstants::treeCodeOrderTable[CODETREE_CODE_COUNT] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
template <uint N> struct divider_int_t;
template <uint N> struct divider_uint_t;
template <> struct divider_int_t<16> {
  typedef int16_t type;
};
template <> struct divider_int_t<32> {
  typedef int32_t type;
};
template <> struct divider_uint_t<16> {
  typedef uint16_t type;
};
template <> struct divider_uint_t<32> {
  typedef uint32_t type;
};
template <> struct divider_uint_t<64> {
  typedef uint64_t type;
};
template <uint N> struct udivider_t {
  typename divider_uint_t<N>::type magic1;
  typename divider_uint_t<N>::type magic2;
  uint8_t shift;
};
template <uint N> struct ucdivider_t {
  typename divider_uint_t<N>::type magic;
  uint8_t ctrl;
};
template <uint N> struct sdivider_t {
  typename divider_uint_t<N>::type magic1;
  typename divider_uint_t<N>::type magic2;
  uint8_t shift;
  int8_t sign;
};
template <uint N> struct scdivider_t {
  typename divider_uint_t<N>::type magic;
  uint8_t ctrl;
};
udivider_t<16> build_udivider_16(const uint16_t d);
udivider_t<32> build_udivider_32(const uint32_t d);
ucdivider_t<16> build_ucdivider_16(const uint16_t d);
ucdivider_t<32> build_ucdivider_32(const uint32_t d);
sdivider_t<16> build_sdivider_16(const int16_t d);
sdivider_t<32> build_sdivider_32(const int32_t d);
scdivider_t<16> build_scdivider_16(const int16_t d);
scdivider_t<32> build_scdivider_32(const int32_t d);
template <uint N1, uint N2> inline typename divider_uint_t<N1>::type divide_template(const typename divider_uint_t<N1>::type dividend, const udivider_t<N2> &divisor) {
  typedef typename divider_uint_t<N1* 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T1 t = ((T1)dividend)*divisor.magic1+divisor.magic2;
  T2 u = (T2)(t>>N2);
  return u>>divisor.shift;
}

template <uint N1, uint N2> inline typename divider_uint_t<N1>::type divide_template(const typename divider_uint_t<N1>::type dividend, const ucdivider_t<N2> &divisor) {
  typedef typename divider_uint_t<N1* 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T1 t = ((T1)dividend)*divisor.magic+(divisor.ctrl&0x80 ? divisor.magic : 0);
  T2 u = (T2)(t>>N2);
  return u>>(divisor.ctrl&(N2-1));
}

template <uint N1, uint N2> inline typename divider_int_t<N1>::type divide_template(const typename divider_int_t<N1>::type dividend, const sdivider_t<N2> &divisor) {
  typedef typename divider_uint_t<N1* 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T2 s = dividend<0 ? -1 : 0;
  T1 t = ((T1)(T2)((dividend^s)-s))*divisor.magic1+divisor.magic2;
  T2 u = (T2)(t>>N2)>>divisor.shift;
  s ^= divisor.sign;
  return (u^s)-s;
}

template <uint N1, uint N2> inline typename divider_int_t<N1>::type divide_template(const typename divider_int_t<N1>::type dividend, const scdivider_t<N2> &divisor) {
  typedef typename divider_uint_t<N1* 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T2 s = dividend<0 ? -1 : 0;
  T1 t = ((T1)(T2)((dividend^s)-s))*divisor.magic+(divisor.ctrl&0x80 ? divisor.magic : 0);
  T2 u = (T2)(t>>N2)>>(divisor.ctrl&(N2-1));
  s ^= (divisor.ctrl&0x40 ? -1 : 0);
  return (u^s)-s;
}

inline uint16_t divide(const uint16_t dividend, const udivider_t<16> &divisor) {
  return divide_template<16, 16>(dividend, divisor);
}

inline uint32_t divide(const uint32_t dividend, const udivider_t<16> &divisor) {
  return divide_template<32, 16>(dividend, divisor);
}

inline uint32_t divide(const uint32_t dividend, const udivider_t<32> &divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

inline uint16_t divide(const uint16_t dividend, const ucdivider_t<16> &divisor) {
  return divide_template<16, 16>(dividend, divisor);
}

inline uint32_t divide(const uint32_t dividend, const ucdivider_t<16> &divisor) {
  return divide_template<32, 16>(dividend, divisor);
}

inline uint32_t divide(const uint32_t dividend, const ucdivider_t<32> &divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

inline int16_t divide(const int16_t dividend, const sdivider_t<16> &divisor) {
  return divide_template<16, 16>(dividend, divisor);
}

inline int32_t divide(const int32_t dividend, const sdivider_t<16> &divisor) {
  return divide_template<32, 16>(dividend, divisor);
}

inline int32_t divide(const int32_t dividend, const sdivider_t<32> &divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

inline int16_t divide(const int16_t dividend, const scdivider_t<16> &divisor) {
  return divide_template<16, 16>(dividend, divisor);
}

inline int32_t divide(const int32_t dividend, const scdivider_t<16> &divisor) {
  return divide_template<32, 16>(dividend, divisor);
}

inline int32_t divide(const int32_t dividend, const scdivider_t<32> &divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

class ArithmeticCodecBase {
public:
ArithmeticCodecBase();
static const uint8_t _normCheckLUT[8];
bool _needsNormalization() const {
  return (_normCheckLUT[_low>>29]&(1<<(_high>>29)))!=0;
}

uint32_t _low;
uint32_t _high;
};
class ArithmeticEncoder : public ArithmeticCodecBase {
public:
ArithmeticEncoder(BitOutputStream &bos);
void flush();
void encode(const uint32_t scale, const uint32_t low, const uint32_t high) {
  uint32_t step = ((_high-_low)+1)/scale;
  _high = _low+step*high-1;
  _low += step*low;
  _checkNormalize();
}

void encodeShiftScale(const uint32_t shift, const uint32_t low, const uint32_t high) {
  uint32_t step = ((_high-_low)+1)>>shift;
  _high = _low+step*high-1;
  _low += step*low;
  _checkNormalize();
}

void encode(const udivider_t<32> &scale, const uint32_t low, const uint32_t high) {
  uint32_t step = divide((_high-_low)+1, scale);
  _high = _low+step*high-1;
  _low += step*low;
  _checkNormalize();
}

void encodeBits(const uint32_t value, const uint32_t bits) {
  uint32_t step = ((_high-_low)+1)>>bits;
  _low += step*value;
  _high = _low+step-1;
  _normalize();
}

private:
void _checkNormalize() {
  if( _needsNormalization() ) {
    _normalize();
  }
}

void _normalize();
void _writeE3(const uint w);
BitOutputStream &_bos;
uint32_t _e3cnt;
};
class ArithmeticDecoder : public ArithmeticCodecBase {
public:
ArithmeticDecoder(BitInputStream &bis);
uint decode(const uint32_t scale, const uint bounds[], const uint N) {
  uint32_t step = ((_high-_low)+1)/scale;
  return _decode(step, bounds, N);
}

uint decodeShiftScale(const uint32_t shift, const uint bounds[], const uint N) {
  uint32_t step = ((_high-_low)+1)>>shift;
  return _decode(step, bounds, N);
}

uint decode(const udivider_t<32> &scale, const uint bounds[], const uint N) {
  uint32_t step = divide((_high-_low)+1, scale);
  return _decode(step, bounds, N);
}

uint decodeBinary(const uint32_t scale, const uint bounds[]) {
  uint32_t step = ((_high-_low)+1)/scale;
  return _decodeBinary(step, bounds);
}

uint decodeBinaryShiftScale(const uint32_t shift, const uint bounds[]) {
  uint32_t step = ((_high-_low)+1)>>shift;
  return _decodeBinary(step, bounds);
}

uint decodeBinary(const udivider_t<32> &scale, const uint bounds[]) {
  uint32_t step = divide((_high-_low)+1, scale);
  return _decodeBinary(step, bounds);
}

uint decodeBits(const uint32_t bits) {
  uint32_t step = ((_high-_low)+1)>>bits;
  uint result = (_value-_low)/step;
  _low += step*result;
  _high = _low+step-1;
  _normalize();
  return result;
}

private:
uint _findIndex(const uint bounds[], const uint N, const uint val) {
  for( uint i = N; i>1; --i ) {
    if( val>=bounds[i-1] ) {
      return i-1;
    }
  }
  return 0;
}

uint _decode(const uint32_t step, const uint bounds[], const uint N) {
  uint32_t val = (_value-_low)/step;
  uint result = _findIndex(bounds, N, val);
  _high = _low+step*bounds[result+1]-1;
  _low += step*bounds[result];
  _checkNormalize();
  return result;
}

uint _decodeBinary(const uint32_t step, const uint bounds[]) {
  uint result = (_value>=_low+bounds[1]*step);
  _high = _low+step*bounds[result+1]-1;
  _low += step*bounds[result];
  _checkNormalize();
  return result;
}

void _checkNormalize() {
  if( _needsNormalization() ) {
    _normalize();
  }
}

void _normalize();
BitInputStream &_bis;
uint32_t _value;
};
bool modelCheckFixed(uint bounds[], uint16_t ids[], uint16_t rids[], const uint N);
void modelSortBounds(uint bounds[], uint16_t ids[], uint16_t rids[], uint backup[], const uint N);
void modelRecreateBounds(uint bounds[], const uint N);
template <uint N> struct ACModelBase {
  static const uint L = N;
  bool isEqualTo(const ACModelBase &m) const {
    for( uint i = 0; i<N; ++i ) {
      if( bounds[i]!=m.bounds[i] ) {
        return false;
      }
      if( bounds[i+1]>0&&ids[i]!=m.ids[i] ) {
        return false;
      }
    }
    if( bounds[N]!=m.bounds[N] ) {
      return false;
    }
    return true;
  }

  uint bounds[N+1];
  uint16_t ids[N], rids[N];
  bool _fixed;
};
struct ACFixedScaleBinaryModel : public ACModelBase<2> {
  ACFixedScaleBinaryModel() {
  }

  ACFixedScaleBinaryModel(const uint (&arr)[2]) {
    memcpy(this->bounds, arr, sizeof(arr));
    build();
  }

  void build();
  void encode(ArithmeticEncoder* encoder, const uint item) {
    if( !this->_fixed ) {
      uint pos = this->rids[item];
      encoder->encodeShiftScale(16, this->bounds[pos], this->bounds[pos+1]);
    }
  }
};
template <uint N> struct ACFixedScaleModel : public ACModelBase<N> {
  ACFixedScaleModel() {
  }

  ACFixedScaleModel(const uint (&arr)[N]) {
    memcpy(this->bounds, arr, sizeof(arr));
    build();
  }

  void build() {
    uint backup[N];
    if( !(this->_fixed = modelCheckFixed(this->bounds, this->ids, this->rids, N)) ) {
      modelSortBounds(this->bounds, this->ids, this->rids, backup, N);
      modelRecreateBounds(this->bounds, N);
    }
  }

  void encode(ArithmeticEncoder* encoder, const uint item) {
    if( !this->_fixed ) {
      uint pos = this->rids[item];
      encoder->encodeShiftScale(16, this->bounds[pos], this->bounds[pos+1]);
    }
  }
};
class MemStream : public SeekableInputOutputStream {
public:
MemStream();
MemStream(const std::vector<uint8_t> &content);
MemStream(const std::vector<uint8_t> &content, const size_t off, const size_t sz);
virtual bool eof() const;
virtual size_t read(byte* buffer, const size_t size);
virtual size_t write(const byte* buffer, const size_t size);
virtual uint64_t tell() const;
virtual uint64_t seek(const uint64_t newPos);
void replaceData(const std::vector<uint8_t> &content) {
  _data = content;
}

const std::vector<uint8_t> &data() const {
  return _data;
}

std::vector<uint8_t> extractData() {
  return std::move(_data);
}

private:
std::vector<uint8_t> _data;
size_t _pos;
};
struct PreflateStreamInfo {
  uint tokenCount;
  uint literalCount;
  uint referenceCount;
  uint maxDist;
  uint maxTokensPerBlock;
  uint countBlocks;
  uint countStoredBlocks;
  uint countHuffBlocks;
  uint countRLEBlocks;
  uint countStaticHuffTreeBlocks;
};
PreflateStreamInfo extractPreflateInfo(const std::vector<PreflateTokenBlock> &blocks);
enum PreflateStrategy { PREFLATE_DEFAULT, PREFLATE_RLE_ONLY, PREFLATE_HUFF_ONLY, PREFLATE_STORE };
enum PreflateHuffStrategy {
  PREFLATE_HUFF_DYNAMIC,
  PREFLATE_HUFF_MIXED,
  PREFLATE_HUFF_STATIC,
};
struct PreflateParameters {
  PreflateStrategy strategy;
  PreflateHuffStrategy huffStrategy;
  bool zlibCompatible;
  byte windowBits;
  byte memLevel;
  byte compLevel;
  bool farLen3MatchesDetected;
  bool veryFarMatchesDetected;
  bool matchesToStartDetected;
  byte log2OfMaxChainDepthM1;
  bool isFastCompressor() const {
    return compLevel>=1&&compLevel<=3;
  }

  bool isSlowCompressor() const {
    return compLevel>=4&&compLevel<=9;
  }

  const PreflateParserConfig &config() const {
    return isFastCompressor() ? fastPreflateParserSettings[compLevel-1] : slowPreflateParserSettings[isSlowCompressor() ? compLevel-4 : 5];
  }
};
byte estimatePreflateMemLevel(const uint maxBlockSize);
PreflateStrategy estimatePreflateStrategy(const PreflateStreamInfo &);
PreflateHuffStrategy estimatePreflateHuffStrategy(const PreflateStreamInfo &);
byte estimatePreflateWindowBits(const uint maxDist);
PreflateParameters estimatePreflateParameters(const std::vector<byte> &unpacked_output, const size_t off0, const std::vector<PreflateTokenBlock> &blocks);
struct PreflateStatisticsCounter {
  struct BlockPrediction {
public:
    void incBlockType(const uint bt) {
      blockType[bt]++;
    }

    void incEOBPredictionWrong(const bool mispredicted) {
      EOBMisprediction[mispredicted]++;
    }

    void incNonZeroPadding(const bool nonzeropadding) {
      nonZeroPadding[nonzeropadding]++;
    }

    static uint totalModels() {
      return 3;
    }

    uint checkDefaultModels() const;
    void print();

private:
    uint blockType[3];
    uint EOBMisprediction[2];
    uint nonZeroPadding[2];
    friend struct PreflateBlockPredictionModel;
  };
  struct TreeCodePrediction {
public:
    void incTreeCodeCountPredictionWrong(const bool mispredicted) {
      TCCountMisprediction[mispredicted]++;
    }

    void incTreeCodeLengthDiffToPrediction(const int len_diff) {
      TCBitlengthCorrection[std::max(std::min(len_diff, 3), -3)+3]++;
    }

    void incLiteralCountPredictionWrong(const bool mispredicted) {
      LCountMisprediction[mispredicted]++;
    }

    void incDistanceCountPredictionWrong(const bool mispredicted) {
      DCountMisprediction[mispredicted]++;
    }

    void incLDCodeTypePredictionWrong(const uint codetype, const bool mispredicted) {
      LDTypeMisprediction[codetype][mispredicted]++;
    }

    void incLDCodeTypeReplacement(const uint replacement_codetype) {
      LDTypeReplacement[replacement_codetype]++;
    }

    void incLDCodeRepeatDiffToPrediction(const int len_diff) {
      LDRepeatCountCorrection[std::max(std::min(len_diff, 1), -1)+1]++;
    }

    void incLDCodeLengthDiffToPrediction(const int len_diff) {
      LDBitlengthCorrection[std::max(std::min(len_diff, 4), -4)+4]++;
    }

    static uint totalModels() {
      return 11;
    }

    uint checkDefaultModels() const;
    void print();

private:
    uint TCCountMisprediction[2];
    uint TCBitlengthCorrection[7];
    uint LCountMisprediction[2];
    uint DCountMisprediction[2];
    uint LDTypeMisprediction[4][2];
    uint LDTypeReplacement[4];
    uint LDRepeatCountCorrection[3];
    uint LDBitlengthCorrection[9];
    friend struct PreflateTreeCodePredictionModel;
  };
  struct TokenPrediction {
public:
    void incLiteralPredictionWrong(const bool mispredicted) {
      LITMisprediction[mispredicted]++;
    }

    void incReferencePredictionWrong(const bool mispredicted) {
      REFMisprediction[mispredicted]++;
    }

    void incLengthDiffToPrediction(const int len_diff) {
      LENCorrection[std::max(std::min(len_diff, 6), -6)+6]++;
    }

    void incIrregularLength258Encoding(const bool irregular) {
      LEN258IrregularEncoding[irregular]++;
    }

    void incDistanceDiffToPredictionAfterIncorrectLengthPrediction(const int len_diff) {
      DISTAfterLenCorrection[std::min(len_diff, 3)]++;
    }

    void incDistanceDiffToPredictionAfterCorrectLengthPrediction(const int len_diff) {
      DISTOnlyCorrection[std::min(len_diff, 3)]++;
    }

    static uint totalModels() {
      return 6;
    }

    uint checkDefaultModels() const;
    void print();

private:
    uint LITMisprediction[2];
    uint REFMisprediction[2];
    uint LENCorrection[13];
    uint LEN258IrregularEncoding[2];
    uint DISTAfterLenCorrection[4];
    uint DISTOnlyCorrection[4];
    friend struct PreflateTokenPredictionModel;
  };

public:
  PreflateStatisticsCounter() {
  }

  BlockPrediction block;
  TreeCodePrediction treecode;
  TokenPrediction token;
  void print();
};
template <uint N> struct PreflateSubModel {
  static const uint L = N;
  PreflateSubModel() {
  }

  void build(const uint (&arr)[N], const uint defval, const uint8_t prec = 16) {
    build_impl(arr, defval, prec);
  }

  void buildDefault(const uint defval);
  void extract(uint (&arr)[N]) {
    for( uint i = 0; i<N; ++i ) {
      arr[i] = bounds[rids[i]+1]-bounds[rids[i]];
    }
  }

  void read(ArithmeticDecoder &, const uint8_t);
  void write(ArithmeticEncoder &, const uint8_t) const;
  void encode(ArithmeticEncoder &codec, const uint item) const {
    if( !isFixed ) {
      size_t idx = rids[item];
      codec.encodeShiftScale(scaleDownBits, scaledDownBounds[idx], scaledDownBounds[idx+1]);
    }
  }

  uint decode(ArithmeticDecoder &codec) const {
    if( isFixed ) {
      return ids[N-1];
    }
    uint val = codec.decodeShiftScale(scaleDownBits, scaledDownBounds, N);
    return ids[val];
  }

  bool isEqualTo(const PreflateSubModel<N> &m) const;
  uint bounds[N+1];
  uint scaledDownBounds[N+1];
  uint16_t ids[N+1], rids[N+1];
  uint8_t scaleDownBits;
  bool isDefault, isFixed;

private:
  void build_impl(const uint* arr, const uint defval, const uint8_t prec);
  void build_scale_down();
  template <uint NEG, uint POS> friend struct PreflateCorrectionSubModel;
};
template <> struct PreflateSubModel<0u> {
  static const uint L = 0u;
  PreflateSubModel() {
  }

  void buildDefault(const uint defval) {
  }

  void read(ArithmeticDecoder &, const uint8_t) {
  }

  void write(ArithmeticEncoder &, const uint8_t) const {
  }

  void encode(ArithmeticEncoder &codec, const uint item) const {
  }

  uint decode(ArithmeticDecoder &codec) const {
    return 0;
  }

  bool isEqualTo(const PreflateSubModel<0u> &m) const {
    return true;
  }

  enum { isDefault = 1, isFixed = 1 };

private:
  void build_impl(const uint* arr, const uint defval, const uint8_t prec) {
  }

  template <uint NEG, uint POS> friend struct PreflateCorrectionSubModel;
};
template <uint NEG, uint POS> struct PreflateCorrectionSubModel {
  static const uint LNEG = NEG;
  static const uint LPOS = POS;
  PreflateCorrectionSubModel() {
  }

  void build(const uint (&arr)[NEG+1+POS], const int defval, const uint8_t prec = 16) {
    build_impl(arr, defval, prec);
  }

  void buildDefault(const uint defval);
  void read(ArithmeticDecoder &, const uint8_t);
  void write(ArithmeticEncoder &, const uint8_t) const;
  void encode(ArithmeticEncoder &codec, const uint actvalue, const uint refvalue, const uint minvalue, const uint maxvalue) {
    int diff = actvalue-refvalue;
    if( diff==0 ) {
      sign.encode(codec, 0);
      return;
    }
    if( diff>0 ) {
      sign.encode(codec, 1);
      if( diff>=(int)POS ) {
        pos.encode(codec, POS-1);
        codec.encodeBits(diff-POS, bitLength(maxvalue-POS-refvalue));
      } else {
        pos.encode(codec, diff-1);
      }
    } else {
      sign.encode(codec, 2);
      if( -diff>=(int)NEG ) {
        neg.encode(codec, NEG-1);
        codec.encodeBits(-diff-NEG, bitLength(refvalue-NEG-minvalue));
      } else {
        neg.encode(codec, -diff-1);
      }
    }
  }

  uint decode(ArithmeticDecoder &codec, const uint refvalue, const uint minvalue, const uint maxvalue) {
    uint s = sign.decode(codec);
    if( s==0 ) {
      return refvalue;
    }
    if( s==1 ) {
      int diff = pos.decode(codec);
      if( diff>=(int)(POS-1) ) {
        return refvalue+codec.decodeBits(bitLength(maxvalue-POS-refvalue))+POS;
      } else {
        return refvalue+diff+1;
      }
    } else {
      int diff = neg.decode(codec);
      if( diff>=(int)(NEG-1) ) {
        return refvalue-codec.decodeBits(bitLength(refvalue-NEG-minvalue))-NEG;
      } else {
        return refvalue-diff-1;
      }
    }
  }

  bool isEqualTo(const PreflateCorrectionSubModel<NEG, POS> &m) const;
  PreflateSubModel<3> sign;
  PreflateSubModel<POS> pos;
  PreflateSubModel<NEG> neg;
  bool isDefault;

private:
  void build_impl(const uint* arr, const int defval, const uint8_t prec);
};
struct PreflateModelCodec {
  PreflateSubModel<2> nonDefaultValue;
  uint8_t MBprecision;
  uint8_t MBprecisionP1;
  bool blockFullDefault;
  bool treecodeFullDefault;
  bool tokenFullDefault;
  uint totalModels, defaultingModels;
  PreflateModelCodec();
  void initDefault();
  void read(const PreflateStatisticsCounter &);
  void readFromStream(ArithmeticDecoder &);
  void writeToStream(ArithmeticEncoder &);
};
struct PreflateBaseModel {
public:
  PreflateBaseModel();
  void setEncoderStream(ArithmeticEncoder*);
  void setDecoderStream(ArithmeticDecoder*);
  static void encodeValue(ArithmeticEncoder &codec, const uint value, const uint maxBits) {
    return codec.encodeBits(value, maxBits);
  }

  void encodeValue(const uint value, const uint maxBits) {
    encodeValue(*encoder, value, maxBits);
  }

  static uint decodeValue(ArithmeticDecoder &codec, const uint maxBits) {
    return codec.decodeBits(maxBits);
  }

  uint decodeValue(const uint maxBits) {
    return decodeValue(*decoder, maxBits);
  }

protected:
  template <uint N> void readSubModel(PreflateSubModel<N> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec = 16);
  template <uint N, uint M> void readSubModel(PreflateCorrectionSubModel<N, M> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec = 16);
  template <uint N> void writeSubModel(const PreflateSubModel<N> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec = 16);
  template <uint N, uint M> void writeSubModel(const PreflateCorrectionSubModel<N, M> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec = 16);
  ArithmeticEncoder* encoder;
  ArithmeticDecoder* decoder;
};
struct PreflateBlockPredictionModel : public PreflateBaseModel {
public:
  void read(const PreflateStatisticsCounter::BlockPrediction &, const PreflateModelCodec &);
  void readFromStream(const PreflateModelCodec &);
  void writeToStream(const PreflateModelCodec &);
  uint decodeBlockType() {
    return blockType.decode(*decoder);
  }

  bool decodeEOBMisprediction() {
    return EOBMisprediction.decode(*decoder);
  }

  bool decodeNonZeroPadding() {
    return nonZeroPadding.decode(*decoder);
  }

  void encodeBlockType(const uint type) {
    blockType.encode(*encoder, type);
  }

  void encodeEOBMisprediction(const bool misprediction) {
    EOBMisprediction.encode(*encoder, misprediction);
  }

  void encodeNonZeroPadding(const bool nonzeropadding) {
    nonZeroPadding.encode(*encoder, nonzeropadding);
  }

  bool isEqualTo(const PreflateBlockPredictionModel &m) const;

private:
  PreflateSubModel<3> blockType;
  PreflateSubModel<2> EOBMisprediction;
  PreflateSubModel<2> nonZeroPadding;
  uint precision;
};
struct PreflateTreeCodePredictionModel : public PreflateBaseModel {
public:
  void read(const PreflateStatisticsCounter::TreeCodePrediction &, const PreflateModelCodec &cc);
  void readFromStream(const PreflateModelCodec &cc);
  void writeToStream(const PreflateModelCodec &cc);
  bool decodeTreeCodeCountMisprediction() {
    return TCCountMisprediction.decode(*decoder);
  }

  bool decodeLiteralCountMisprediction() {
    return LCountMisprediction.decode(*decoder);
  }

  bool decodeDistanceCountMisprediction() {
    return DCountMisprediction.decode(*decoder);
  }

  int decodeTreeCodeBitLengthCorrection(uint predval) {
    return TCBitlengthCorrection.decode(*decoder, predval, 0, 7);
  }

  uint decodeLDTypeCorrection(uint predtype) {
    return DerivedLDTypeReplacement[predtype].decode(*decoder);
  }

  uint decodeRepeatCountCorrection(const uint predval, const uint ldtype) {
    static const uint8_t minVal[4] = {0, 3, 3, 11};
    static const uint8_t lenVal[4] = {0, 3, 7, 127};
    return LDRepeatCountCorrection.decode(*decoder, predval, minVal[ldtype], minVal[ldtype]+lenVal[ldtype]);
  }

  int decodeLDBitLengthCorrection(uint predval) {
    return LDBitlengthCorrection.decode(*decoder, predval, 0, 15);
  }

  void encodeTreeCodeCountMisprediction(const bool misprediction) {
    TCCountMisprediction.encode(*encoder, misprediction);
  }

  void encodeLiteralCountMisprediction(const bool misprediction) {
    LCountMisprediction.encode(*encoder, misprediction);
  }

  void encodeDistanceCountMisprediction(const bool misprediction) {
    DCountMisprediction.encode(*encoder, misprediction);
  }

  void encodeTreeCodeBitLengthCorrection(const uint predval, const uint actval) {
    TCBitlengthCorrection.encode(*encoder, actval, predval, 0, 7);
  }

  void encodeLDTypeCorrection(const uint predval, const uint actval) {
    DerivedLDTypeReplacement[predval].encode(*encoder, actval);
  }

  void encodeRepeatCountCorrection(const uint predval, const uint actval, uint ldtype) {
    static const uint8_t minVal[4] = {0, 3, 3, 11};
    static const uint8_t lenVal[4] = {0, 3, 7, 127};
    LDRepeatCountCorrection.encode(*encoder, actval, predval, minVal[ldtype], minVal[ldtype]+lenVal[ldtype]);
  }

  void encodeLDBitLengthCorrection(const uint predval, const uint actval) {
    LDBitlengthCorrection.encode(*encoder, actval, predval, 0, 15);
  }

  bool isEqualTo(const PreflateTreeCodePredictionModel &m) const;

private:
  void deriveLDTypeReplacement();
  PreflateSubModel<2> TCCountMisprediction;
  PreflateSubModel<2> LCountMisprediction;
  PreflateSubModel<2> DCountMisprediction;
  PreflateSubModel<2> LDTypeMisprediction[4];
  PreflateSubModel<4> LDTypeReplacementBase;
  PreflateCorrectionSubModel<1, 1> LDRepeatCountCorrection;
  PreflateCorrectionSubModel<3, 3> TCBitlengthCorrection;
  PreflateCorrectionSubModel<4, 4> LDBitlengthCorrection;
  PreflateSubModel<4> DerivedLDTypeReplacement[4];
};
struct PreflateTokenPredictionModel : public PreflateBaseModel {
public:
  void read(const PreflateStatisticsCounter::TokenPrediction &, const PreflateModelCodec &cc);
  void readFromStream(const PreflateModelCodec &cc);
  void writeToStream(const PreflateModelCodec &cc);
  bool decodeLiteralPredictionWrong() {
    return LITMisprediction.decode(*decoder);
  }

  bool decodeReferencePredictionWrong() {
    return REFMisprediction.decode(*decoder);
  }

  int decodeLenCorrection(const uint predval) {
    return LENCorrection.decode(*decoder, predval, 3, 258);
  }

  uint decodeDistOnlyCorrection() {
    return DISTOnlyCorrection.decode(*decoder, 0, 0, 32767);
  }

  uint decodeDistAfterLenCorrection() {
    return DISTAfterLenCorrection.decode(*decoder, 0, 0, 32767);
  }

  bool decodeIrregularLen258() {
    return IrregularLen258Encoding.decode(*decoder);
  }

  void encodeLiteralPredictionWrong(const bool misprediction) {
    LITMisprediction.encode(*encoder, misprediction);
  }

  void encodeReferencePredictionWrong(const bool misprediction) {
    REFMisprediction.encode(*encoder, misprediction);
  }

  void encodeLenCorrection(const uint predval, const uint actval) {
    LENCorrection.encode(*encoder, actval, predval, 3, 258);
  }

  void encodeDistOnlyCorrection(const uint hops) {
    DISTOnlyCorrection.encode(*encoder, hops, 0, 0, 32767);
  }

  void encodeDistAfterLenCorrection(const uint hops) {
    DISTAfterLenCorrection.encode(*encoder, hops, 0, 0, 32767);
  }

  void encodeIrregularLen258(const bool irregular) {
    IrregularLen258Encoding.encode(*encoder, irregular);
  }

  bool isEqualTo(const PreflateTokenPredictionModel &m) const;

private:
  PreflateSubModel<2> LITMisprediction;
  PreflateSubModel<2> REFMisprediction;
  PreflateCorrectionSubModel<6, 6> LENCorrection;
  PreflateCorrectionSubModel<0, 3> DISTAfterLenCorrection;
  PreflateCorrectionSubModel<0, 3> DISTOnlyCorrection;
  PreflateSubModel<2> IrregularLen258Encoding;
};
struct PreflatePredictionModel {
  PreflatePredictionModel();
  ~PreflatePredictionModel();
  void read(const PreflateStatisticsCounter &model, const PreflateModelCodec &cc);
  void setEncoderStream(ArithmeticEncoder* codec);
  void setDecoderStream(ArithmeticDecoder* codec);
  void readFromStream(const PreflateModelCodec &cc);
  void writeToStream(const PreflateModelCodec &cc);
  bool isEqualTo(const PreflatePredictionModel &m) const;

protected:
  PreflateBlockPredictionModel block;
  PreflateTreeCodePredictionModel treecode;
  PreflateTokenPredictionModel token;
};
struct PreflatePredictionEncoder : public PreflatePredictionModel {
  PreflatePredictionEncoder();
  void start(const PreflatePredictionModel &, const PreflateParameters &, const uint modelId);
  std::vector<uint8_t> end();
  void encodeValue(const uint value, const uint maxBits) {
    encoder->encodeBits(value, maxBits);
  }

  void encodeBlockType(const uint type) {
    block.encodeBlockType(type);
  }

  void encodeEOBMisprediction(const bool misprediction) {
    block.encodeEOBMisprediction(misprediction);
  }

  void encodeNonZeroPadding(const bool nonzeropadding) {
    block.encodeNonZeroPadding(nonzeropadding);
  }

  void encodeTreeCodeCountMisprediction(const bool misprediction) {
    treecode.encodeTreeCodeCountMisprediction(misprediction);
  }

  void encodeLiteralCountMisprediction(const bool misprediction) {
    treecode.encodeLiteralCountMisprediction(misprediction);
  }

  void encodeDistanceCountMisprediction(const bool misprediction) {
    treecode.encodeDistanceCountMisprediction(misprediction);
  }

  void encodeTreeCodeBitLengthCorrection(const uint predval, const uint actval) {
    treecode.encodeTreeCodeBitLengthCorrection(predval, actval);
  }

  void encodeLDTypeCorrection(const uint predval, const uint actval) {
    treecode.encodeLDTypeCorrection(predval, actval);
  }

  void encodeRepeatCountCorrection(const uint predval, const uint actval, uint ldtype) {
    treecode.encodeRepeatCountCorrection(predval, actval, ldtype);
  }

  void encodeLDBitLengthCorrection(const uint predval, const uint actval) {
    treecode.encodeLDBitLengthCorrection(predval, actval);
  }

  void encodeLiteralPredictionWrong(const bool misprediction) {
    token.encodeLiteralPredictionWrong(misprediction);
  }

  void encodeReferencePredictionWrong(const bool misprediction) {
    token.encodeReferencePredictionWrong(misprediction);
  }

  void encodeLenCorrection(const uint predval, const uint actval) {
    token.encodeLenCorrection(predval, actval);
  }

  void encodeDistOnlyCorrection(const uint hops) {
    token.encodeDistOnlyCorrection(hops);
  }

  void encodeDistAfterLenCorrection(const uint hops) {
    token.encodeDistAfterLenCorrection(hops);
  }

  void encodeIrregularLen258(const bool irregular) {
    token.encodeIrregularLen258(irregular);
  }

  const PreflateParameters &parameters() const {
    return params;
  }

  uint modelId() const {
    return modelid;
  }

private:
  PreflateParameters params;
  uint modelid;
  MemStream* storage;
  BitOutputStream* bos;
  ArithmeticEncoder* encoder;
};
struct PreflatePredictionDecoder : public PreflatePredictionModel {
  PreflatePredictionDecoder();
  void start(const PreflatePredictionModel &, const PreflateParameters &, const std::vector<uint8_t> &, size_t off0, size_t size);
  void end();
  uint decodeValue(const uint maxBits) {
    return decoder->decodeBits(maxBits);
  }

  uint decodeBlockType() {
    return block.decodeBlockType();
  }

  bool decodeEOBMisprediction() {
    return block.decodeEOBMisprediction();
  }

  bool decodeNonZeroPadding() {
    return block.decodeNonZeroPadding();
  }

  bool decodeTreeCodeCountMisprediction() {
    return treecode.decodeTreeCodeCountMisprediction();
  }

  bool decodeLiteralCountMisprediction() {
    return treecode.decodeLiteralCountMisprediction();
  }

  bool decodeDistanceCountMisprediction() {
    return treecode.decodeDistanceCountMisprediction();
  }

  int decodeTreeCodeBitLengthCorrection(uint predval) {
    return treecode.decodeTreeCodeBitLengthCorrection(predval);
  }

  uint decodeLDTypeCorrection(uint predtype) {
    return treecode.decodeLDTypeCorrection(predtype);
  }

  uint decodeRepeatCountCorrection(const uint predval, const uint ldtype) {
    return treecode.decodeRepeatCountCorrection(predval, ldtype);
  }

  uint decodeLDBitLengthCorrection(uint predval) {
    return treecode.decodeLDBitLengthCorrection(predval);
  }

  bool decodeLiteralPredictionWrong() {
    return token.decodeLiteralPredictionWrong();
  }

  bool decodeReferencePredictionWrong() {
    return token.decodeReferencePredictionWrong();
  }

  int decodeLenCorrection(const uint predval) {
    return token.decodeLenCorrection(predval);
  }

  uint decodeDistOnlyCorrection() {
    return token.decodeDistOnlyCorrection();
  }

  uint decodeDistAfterLenCorrection() {
    return token.decodeDistAfterLenCorrection();
  }

  bool decodeIrregularLen258() {
    return token.decodeIrregularLen258();
  }

private:
  PreflateParameters params;
  MemStream* storage;
  BitInputStream* bis;
  ArithmeticDecoder* decoder;
};
struct PreflateMetaEncoder {
  PreflateMetaEncoder();
  ~PreflateMetaEncoder();
  bool error() const {
    return inError;
  }

  uint addModel(const PreflateStatisticsCounter &, const PreflateParameters &);
  bool beginMetaBlockWithModel(PreflatePredictionEncoder &, const uint modelId);
  bool endMetaBlock(PreflatePredictionEncoder &, const size_t uncompressed);
  std::vector<byte> finish();

private:
  struct modelType {
    uint writtenId;
    PreflateStatisticsCounter counter;
    PreflatePredictionModel model;
    PreflateParameters params;
    PreflateModelCodec mcodec;
  };
  struct metaBlockInfo {
    uint modelId;
    size_t reconSize;
    size_t uncompressedSize;
  };
  bool inError;
  std::vector<modelType> modelList;
  std::vector<metaBlockInfo> blockList;
  std::vector<uint8_t> reconData;
};
struct PreflateMetaDecoder {
  PreflateMetaDecoder(const std::vector<uint8_t> &reconData, const uint64_t uncompressedSize);
  ~PreflateMetaDecoder();
  bool error() const {
    return inError;
  }

  size_t metaBlockCount() const {
    return blockList.size();
  }

  uint64_t metaBlockUncompressedStartOfs(const size_t metaBlockId) const {
    return blockList[metaBlockId].uncompressedStartOfs;
  }

  size_t metaBlockUncompressedSize(const size_t metaBlockId) const {
    return blockList[metaBlockId].uncompressedSize;
  }

  bool beginMetaBlock(PreflatePredictionDecoder &, PreflateParameters &, const size_t index);
  bool endMetaBlock(PreflatePredictionDecoder &);
  void finish();

private:
  struct modelType {
    PreflatePredictionModel model;
    PreflateParameters params;
    PreflateModelCodec mcodec;
  };
  struct metaBlockInfo {
    uint modelId;
    size_t reconStartOfs;
    size_t reconSize;
    uint64_t uncompressedStartOfs;
    uint64_t uncompressedSize;
  };
  bool inError;
  const std::vector<uint8_t> &reconData;
  const uint64_t uncompressedSize;
  std::vector<modelType> modelList;
  std::vector<metaBlockInfo> blockList;
};
bool isEqual(const PreflatePredictionModel &, const PreflatePredictionModel &);
class TaskPool {
public:
TaskPool();
~TaskPool();
template <class F, class ... Args> auto addTask(F &&f, Args &&... args) -> std::future<typename std::result_of<F (Args...)>::type> {
  using R = typename std::result_of<F (Args...)>::type;
  auto task = std::make_shared<std::packaged_task<R()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  if( _state==INIT ) {
    _init();
  }
  std::future<R> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _tasks.emplace([task]() {
        (*task)();
      });
  }
  _condition.notify_one();
  return res;
}

size_t extraThreadCount() const {
  return _threadLimit;
}

private:
enum State { INIT, RUN, FINISH };
void _init();
State _state;
size_t _threadLimit;
std::vector<std::thread> _workers;
std::mutex _mutex;
std::condition_variable _condition;
std::queue<std::function<void()> > _tasks;
};
extern TaskPool globalTaskPool;
struct PreflateTokenPredictor;
struct PreflateTreePredictor;
class PreflateDecoderTask {
public:
class Handler {
public:
virtual ~Handler() {
}

virtual uint32_t setModel(const PreflateStatisticsCounter &, const PreflateParameters &) = 0;
virtual bool beginEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder &, const uint32_t modelId) = 0;
virtual bool endEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder &, const size_t uncompressedSize) = 0;
virtual void markProgress() = 0;
};
PreflateDecoderTask(Handler &handler, const uint32_t metaBlockId, std::vector<PreflateTokenBlock> &&tokenData, std::vector<uint8_t> &&uncompressedData, const size_t uncompressedOffset, const bool lastMetaBlock, const uint32_t paddingBits);
bool analyze();
bool encode();
uint32_t id() {
  return metaBlockId;
}

private:
Handler &handler;
uint32_t metaBlockId;
std::vector<PreflateTokenBlock> tokenData;
std::vector<uint8_t> uncompressedData;
size_t uncompressedOffset;
bool lastMetaBlock;
uint32_t paddingBits;
PreflateParameters params;
PreflateStatisticsCounter counter;
std::unique_ptr<PreflateTokenPredictor> tokenPredictor;
std::unique_ptr<PreflateTreePredictor> treePredictor;
};
bool preflate_decode(OutputStream &unpacked_output, std::vector<byte> &preflate_diff, uint64_t &deflate_size, InputStream &deflate_raw, std::function<void(void)> block_callback, const size_t min_deflate_size, const size_t metaBlockSize = INT32_MAX);
bool preflate_decode(std::vector<byte> &unpacked_output, std::vector<byte> &preflate_diff, const std::vector<byte> &deflate_raw, const size_t metaBlockSize = INT32_MAX);
bool preflate_decode(std::vector<byte> &unpacked_output, std::vector<byte> &preflate_diff, uint64_t &deflate_size, InputStream &deflate_raw, std::function<void(void)> block_callback, const size_t min_deflate_size, const size_t metaBlockSize = INT32_MAX);
struct PreflateStatisticalModel;
struct PreflateStatisticalCodec;
struct PreflateTokenPredictor {
  PreflatePredictorState state;
  PreflateHashChainExt hash;
  PreflateSeqChain seq;
  PreflateParameters params;
  bool predictionFailure;
  bool fast;
  uint prevLen;
  PreflateToken pendingToken;
  uint currentTokenCount;
  bool emptyBlockAtEnd;
  struct BlockAnalysisResult {
    PreflateTokenBlock::Type type;
    uint tokenCount;
    bool blockSizePredicted;
    bool inputEOF;
    bool lastBlock;
    uint8_t paddingBits, paddingCounts;
    std::vector<byte> tokenInfo;
    std::vector<int32_t> correctives;
  };
  std::vector<BlockAnalysisResult> analysisResults;
  PreflateTokenPredictor(const PreflateParameters &params, const std::vector<byte> &uncompressed, const size_t offset);
  void analyzeBlock(const uint blockno, const PreflateTokenBlock &block);
  void updateCounters(PreflateStatisticsCounter*, const uint blockno);
  void encodeBlock(PreflatePredictionEncoder*, const uint blockno);
  void encodeEOF(PreflatePredictionEncoder*, const uint blockno, const bool lastBlock);
  PreflateTokenBlock decodeBlock(PreflatePredictionDecoder*);
  bool decodeEOF(PreflatePredictionDecoder*);
  bool inputEOF();
  bool predictEOB();
  PreflateToken predictToken();
  bool repredictReference(PreflateToken &token);
  PreflateRematchInfo repredictMatch(const PreflateToken &);
  uint recalculateDistance(const PreflateToken &, const uint hops);
  void commitToken(const PreflateToken &);
};
struct PreflateStatisticsCounter;
struct PreflatePredictionDecoder;
struct PreflatePredictionEncoder;
enum TreeCodeType { TCT_BITS = 0, TCT_REP = 1, TCT_REPZS = 2, TCT_REPZL = 3 };
struct PreflateTreePredictor {
  PreflateInput input;
  bool predictionFailure;
  struct BlockAnalysisResult {
    PreflateTokenBlock::Type blockType;
    std::vector<byte> tokenInfo;
    std::vector<int32_t> correctives;
  };
  std::vector<BlockAnalysisResult> analysisResults;
  void collectTokenStatistics(uint LcodeFrequencies[], uint DcodeFrequencies[], uint &Lcount, uint &Dcount, const PreflateTokenBlock &block);
  uint buildLBitlenghs(byte bitLengths[], uint Lcodes[]);
  uint buildDBitlenghs(byte bitLengths[], uint Dcodes[]);
  uint buildTCBitlengths(byte (&bitLengths)[PreflateConstants::CODETREE_CODE_COUNT], uint (&BLfreqs)[PreflateConstants::CODETREE_CODE_COUNT]);
  uint calcBitLengths(byte* symBitLen, const uint* symFreq, const uint symCount, const uint maxBits, const uint minMaxCode);
  TreeCodeType predictCodeType(const byte* symBitLen, const uint symCount, const bool first);
  byte predictCodeData(const byte* symBitLen, const TreeCodeType type, const uint symCount, const bool first);
  void predictLDTrees(BlockAnalysisResult &analysis, uint* frequencies, const byte* symBitLen, const uint symLCount, const uint symDCount, const byte* targetCodes, const uint targetCodeSize);
  uint reconstructLDTrees(PreflatePredictionDecoder* codec, uint* frequencies, byte* targetCodes, uint targetCodeSize, const byte* symBitLen, const uint symLCount, const uint symDCount);
  PreflateTreePredictor(const std::vector<byte> &dump, const size_t offset);
  void analyzeBlock(const uint blockno, const PreflateTokenBlock &block);
  void updateCounters(PreflateStatisticsCounter*, const uint blockno);
  void encodeBlock(PreflatePredictionEncoder*, const uint blockno);
  bool decodeBlock(PreflateTokenBlock &block, PreflatePredictionDecoder*);
};
class PreflateDecoderHandler : public PreflateDecoderTask::Handler {
public:
PreflateDecoderHandler(std::function<void(void)> progressCallback_) : progressCallback(progressCallback_) {
}

bool finish(std::vector<uint8_t> &reconstructionData) {
  reconstructionData = encoder.finish();
  return !encoder.error();
}

bool error() const {
  return encoder.error();
}

virtual uint32_t setModel(const PreflateStatisticsCounter &counters, const PreflateParameters &parameters) {
  return encoder.addModel(counters, parameters);
}

virtual bool beginEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder &codec, const uint32_t modelId) {
  return encoder.beginMetaBlockWithModel(codec, modelId);
}

virtual bool endEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder &codec, const size_t uncompressedSize) {
  return encoder.endMetaBlock(codec, uncompressedSize);
}

virtual void markProgress() {
  std::unique_lock<std::mutex> lock(this->_mutex);
  progressCallback();
}

private:
PreflateMetaEncoder encoder;
std::function<void(void)> progressCallback;
std::mutex _mutex;
};
PreflateDecoderTask::PreflateDecoderTask(PreflateDecoderTask::Handler &handler_, const uint32_t metaBlockId_, std::vector<PreflateTokenBlock> &&tokenData_, std::vector<uint8_t> &&uncompressedData_, const size_t uncompressedOffset_, const bool lastMetaBlock_, const uint32_t paddingBits_) : handler(handler_), metaBlockId(metaBlockId_), tokenData(tokenData_), uncompressedData(uncompressedData_), uncompressedOffset(uncompressedOffset_), lastMetaBlock(lastMetaBlock_), paddingBits(paddingBits_) {
}

bool PreflateDecoderTask::analyze() {
  params = estimatePreflateParameters(uncompressedData, uncompressedOffset, tokenData);
  memset(&counter, 0, sizeof(counter));
  tokenPredictor.reset(new PreflateTokenPredictor(params, uncompressedData, uncompressedOffset));
  treePredictor.reset(new PreflateTreePredictor(uncompressedData, uncompressedOffset));
  for( uint i = 0, n = tokenData.size(); i<n; ++i ) {
    tokenPredictor->analyzeBlock(i, tokenData[i]);
    treePredictor->analyzeBlock(i, tokenData[i]);
    if( tokenPredictor->predictionFailure||treePredictor->predictionFailure ) {
      return false;
    }
    tokenPredictor->updateCounters(&counter, i);
    treePredictor->updateCounters(&counter, i);
    handler.markProgress();
  }
  counter.block.incNonZeroPadding(paddingBits!=0);
  return true;
}

bool PreflateDecoderTask::encode() {
  PreflatePredictionEncoder pcodec;
  uint modelId = handler.setModel(counter, params);
  if( !handler.beginEncoding(metaBlockId, pcodec, modelId) ) {
    return false;
  }
  for( uint i = 0, n = tokenData.size(); i<n; ++i ) {
    tokenPredictor->encodeBlock(&pcodec, i);
    treePredictor->encodeBlock(&pcodec, i);
    if( tokenPredictor->predictionFailure||treePredictor->predictionFailure ) {
      return false;
    }
    if( lastMetaBlock ) {
      tokenPredictor->encodeEOF(&pcodec, i, i+1==tokenData.size());
    }
  }
  if( lastMetaBlock ) {
    pcodec.encodeNonZeroPadding(paddingBits!=0);
    if( paddingBits!=0 ) {
      uint bitsToSave = bitLength(paddingBits);
      pcodec.encodeValue(bitsToSave, 3);
      if( bitsToSave>1 ) {
        pcodec.encodeValue(paddingBits&((1<<(bitsToSave-1))-1), bitsToSave-1);
      }
    }
  }
  return handler.endEncoding(metaBlockId, pcodec, uncompressedData.size()-uncompressedOffset);
}

bool preflate_decode(OutputStream &unpacked_output, std::vector<byte> &preflate_diff, uint64_t &deflate_size, InputStream &deflate_raw, std::function<void(void)> block_callback, const size_t min_deflate_size, const size_t metaBlockSize) {
  deflate_size = 0;
  uint64_t deflate_bits = 0;
  size_t prevBitPos = 0;
  BitInputStream decInBits(deflate_raw);
  OutputCacheStream decOutCache(unpacked_output);
  PreflateBlockDecoder bdec(decInBits, decOutCache);
  if( bdec.status()!=PreflateBlockDecoder::OK ) {
    return false;
  }
  bool last;
  uint i = 0;
  std::vector<PreflateTokenBlock> blocks;
  std::vector<uint32_t> blockSizes;
  uint64_t sumBlockSizes = 0;
  uint64_t lastEndPos = 0;
  uint64_t uncompressedMetaStart = 0;
  size_t MBSize = std::min<size_t>(std::max<size_t>(metaBlockSize, 1u<<18), (1u<<31)-1);
  size_t MBThreshold = (MBSize*3)>>1;
  PreflateDecoderHandler encoder(block_callback);
  size_t MBcount = 0;
  std::queue<std::future<std::shared_ptr<PreflateDecoderTask> > > futureQueue;
  size_t queueLimit = std::min(2*globalTaskPool.extraThreadCount(), (1<<26)/MBThreshold);
  bool fail = false;
  do {
    PreflateTokenBlock newBlock;
    bool ok = bdec.readBlock(newBlock, last);
    if( !ok ) {
      fail = true;
      break;
    }
    uint64_t blockSize = decOutCache.cacheEndPos()-lastEndPos;
    lastEndPos = decOutCache.cacheEndPos();
    if( blockSize>=(1<<31) ) {
      fail = true;
      break;
    }
    blocks.push_back(newBlock);
    blockSizes.push_back(blockSize);
    ++i;
    block_callback();
    deflate_bits += decInBits.bitPos()-prevBitPos;
    prevBitPos = decInBits.bitPos();
    sumBlockSizes += blockSize;
    if( last||sumBlockSizes>=MBThreshold ) {
      size_t blockCount, blockSizeSum;
      if( last ) {
        blockCount = blockSizes.size();
        blockSizeSum = sumBlockSizes;
      } else {
        blockCount = 0;
        blockSizeSum = 0;
        for( const auto bs : blockSizes ) {
          blockSizeSum += bs;
          ++blockCount;
          if( blockSizeSum>=MBSize ) {
            break;
          }
        }
      }
      std::vector<PreflateTokenBlock> blocksForMeta;
      for( size_t j = 0; j<blockCount; ++j ) {
        blocksForMeta.push_back(std::move(blocks[j]));
      }
      blocks.erase(blocks.begin(), blocks.begin()+blockCount);
      blockSizes.erase(blockSizes.begin(), blockSizes.begin()+blockCount);
      sumBlockSizes -= blockSizeSum;
      size_t uncompressedOffset = MBcount==0 ? 0 : 1<<15;
      std::vector<uint8_t> uncompressedDataForMeta(decOutCache.cacheData(uncompressedMetaStart-uncompressedOffset), decOutCache.cacheData(uncompressedMetaStart-uncompressedOffset)+blockSizeSum+uncompressedOffset);
      uncompressedMetaStart += blockSizeSum;
      size_t paddingBits = 0;
      if( last ) {
        uint8_t remaining_bit_count = (8-deflate_bits)&7;
        paddingBits = decInBits.get(remaining_bit_count);
        deflate_bits += decInBits.bitPos()-prevBitPos;
        prevBitPos = decInBits.bitPos();
      }
      if( futureQueue.empty()&&(queueLimit==0||last) ) {
        PreflateDecoderTask task(encoder, MBcount, std::move(blocksForMeta), std::move(uncompressedDataForMeta), uncompressedOffset, last, paddingBits);
        if( !task.analyze()||!task.encode() ) {
          fail = true;
          break;
        }
      } else {
        if( futureQueue.size()>=queueLimit ) {
          std::future<std::shared_ptr<PreflateDecoderTask> > first = std::move(futureQueue.front());
          futureQueue.pop();
          std::shared_ptr<PreflateDecoderTask> data = first.get();
          if( !data||!data->encode() ) {
            fail = true;
            break;
          }
        }
        std::shared_ptr<PreflateDecoderTask> ptask;
        ptask.reset(new PreflateDecoderTask(encoder, MBcount, std::move(blocksForMeta), std::move(uncompressedDataForMeta), uncompressedOffset, last, paddingBits));
        futureQueue.push(globalTaskPool.addTask([ptask, &fail]() {
          if( !fail&&ptask->analyze() ) {
            return ptask;
          } else {
            return std::shared_ptr<PreflateDecoderTask>();
          }
        }));
      }
      if( !last ) {
        decOutCache.flushUpTo(uncompressedMetaStart-(1<<15));
      }
      MBcount++;
    }
  } while( !fail&&!last );
  while( !futureQueue.empty() ) {
    std::future<std::shared_ptr<PreflateDecoderTask> > first = std::move(futureQueue.front());
    futureQueue.pop();
    std::shared_ptr<PreflateDecoderTask> data = first.get();
    if( fail||!data||!data->encode() ) {
      fail = true;
    }
  }
  decOutCache.flush();
  deflate_size = (deflate_bits+7)>>3;
  if( deflate_size<min_deflate_size ) {
    return false;
  }
  return !fail&&encoder.finish(preflate_diff);
}

bool preflate_decode(std::vector<byte> &unpacked_output, std::vector<byte> &preflate_diff, uint64_t &deflate_size, InputStream &deflate_raw, std::function<void(void)> block_callback, const size_t min_deflate_size, const size_t metaBlockSize) {
  MemStream uncompressedOutput;
  bool result = preflate_decode(uncompressedOutput, preflate_diff, deflate_size, deflate_raw, block_callback, min_deflate_size, metaBlockSize);
  unpacked_output = uncompressedOutput.extractData();
  return result;
}

bool preflate_decode(std::vector<byte> &unpacked_output, std::vector<byte> &preflate_diff, const std::vector<byte> &deflate_raw, const size_t metaBlockSize) {
  MemStream mem(deflate_raw);
  uint64_t raw_size;
  return preflate_decode(unpacked_output, preflate_diff, raw_size, mem, [] {
  }, 0, metaBlockSize)&&raw_size==deflate_raw.size();
}

PreflateHashChainExt::PreflateHashChainExt(const std::vector<byte> &input_, const byte memLevel) : _input(input_), totalShift(-8) {
  hashBits = memLevel+7;
  hashShift = (hashBits+PreflateConstants::MIN_MATCH-1)/PreflateConstants::MIN_MATCH;
  hashMask = (1<<hashBits)-1;
  head = new uint16_t[hashMask+1];
  prev = new uint16_t[1<<16];
  chainDepth = new uint[1<<16];
  memset(head, 0, sizeof(short)*(hashMask+1));
  memset(prev, 0, sizeof(short)*(1<<16));
  memset(chainDepth, 0, sizeof(uint)*(1<<16));
  runningHash = 0;
  if( _input.remaining()>2 ) {
    updateRunningHash(_input.curChar(0));
    updateRunningHash(_input.curChar(1));
  }
}

PreflateHashChainExt::~PreflateHashChainExt() {
  delete[] head;
  delete[] chainDepth;
  delete[] prev;
}

void PreflateHashChainExt::updateHash(const uint l) {
  if( l>0x180 ) {
    uint l_ = l;
    while( l_>0 ) {
      uint blk = std::min(l_, 0x180u);
      updateHash(blk);
      l_ -= blk;
    }
    return;
  }
  const byte* b = _input.curChars();
  uint pos = _input.pos();
  if( pos-totalShift>=0xfe08 ) {
    reshift();
  }
  for( uint i = 2; i<std::min(l+2, _input.remaining()); ++i ) {
    updateRunningHash(b[i]);
    uint h = runningHash&hashMask;
    uint p = (pos+i-2)-totalShift;
    chainDepth[p] = chainDepth[head[h]]+1;
    prev[p] = head[h];
    head[h] = p;
  }
  _input.advance(l);
}

void PreflateHashChainExt::skipHash(const uint l) {
  const byte* b = _input.curChars();
  uint pos = _input.pos();
  if( pos-totalShift>=0xfe08 ) {
    reshift();
  }
  uint remaining = _input.remaining();
  if( remaining>2 ) {
    updateRunningHash(b[2]);
    uint h = runningHash&hashMask;
    uint p = (pos)-totalShift;
    chainDepth[p] = chainDepth[head[h]]+1;
    prev[p] = head[h];
    head[h] = p;
    for( uint i = 1; i<l; ++i ) {
      uint p = (pos+i)-totalShift;
      chainDepth[p] = 0xffff8000;
    }
    if( remaining>l ) {
      updateRunningHash(b[l]);
      if( remaining>l+1 ) {
        updateRunningHash(b[l+1]);
      }
    }
  }
  _input.advance(l);
}

void PreflateHashChainExt::reshift() {
  const uint16_t delta = 0x7e00;
  for( uint i = 0, n = hashMask+1; i<n; ++i ) {
    head[i] = std::max(head[i], delta)-delta;
  }
  for( uint i = delta+8, n = 1<<16; i<n; ++i ) {
    prev[i-delta] = std::max(prev[i], delta)-delta;
  }
  memmove(chainDepth+8, chainDepth+8+delta, (0x10000-8-delta)*sizeof(chainDepth[0]));
  totalShift += delta;
}

PreflateStreamInfo extractPreflateInfo(const std::vector<PreflateTokenBlock> &blocks) {
  PreflateStreamInfo result;
  memset(&result, 0, sizeof(result));
  result.countBlocks = blocks.size();
  for( uint i = 0, n = result.countBlocks; i<n; ++i ) {
    const PreflateTokenBlock &b = blocks[i];
    if( b.type==PreflateTokenBlock::STORED ) {
      result.countStoredBlocks++;
      continue;
    }
    if( b.type==PreflateTokenBlock::STATIC_HUFF ) {
      result.countStaticHuffTreeBlocks++;
    }
    result.tokenCount += b.tokens.size();
    result.maxTokensPerBlock = std::max(result.maxTokensPerBlock, (uint)b.tokens.size());
    uint blockMaxDist = 0;
    for( uint j = 0, m = b.tokens.size(); j<m; ++j ) {
      const PreflateToken &t = b.tokens[j];
      if( t.len==1 ) {
        result.literalCount++;
      } else {
        result.referenceCount++;
        blockMaxDist = std::max(blockMaxDist, (uint)t.dist);
      }
    }
    result.maxDist = std::max(result.maxDist, blockMaxDist);
    if( blockMaxDist==0 ) {
      result.countHuffBlocks++;
    } else if( blockMaxDist==1 ) {
      result.countRLEBlocks++;
    }
  }
  return result;
}

byte estimatePreflateMemLevel(const uint maxBlockSize_) {
  uint maxBlockSize = maxBlockSize_;
  uint mbits = 0;
  while( maxBlockSize>0 ) {
    ++mbits;
    maxBlockSize >>= 1;
  }
  mbits = std::min(std::max(mbits, 7u), 15u);
  return mbits-6;
}

byte estimatePreflateWindowBits(const uint maxDist_) {
  uint maxDist = maxDist_;
  maxDist += PreflateConstants::MIN_LOOKAHEAD;
  uint wbits = bitLength(maxDist-1);
  wbits = std::min(std::max(wbits, 9u), 15u);
  return wbits;
}

PreflateStrategy estimatePreflateStrategy(const PreflateStreamInfo &info) {
  if( info.countStoredBlocks==info.countBlocks ) {
    return PREFLATE_STORE;
  }
  if( info.countHuffBlocks==info.countBlocks ) {
    return PREFLATE_HUFF_ONLY;
  }
  if( info.countRLEBlocks==info.countBlocks ) {
    return PREFLATE_RLE_ONLY;
  }
  return PREFLATE_DEFAULT;
}

PreflateHuffStrategy estimatePreflateHuffStrategy(const PreflateStreamInfo &info) {
  if( info.countStaticHuffTreeBlocks==info.countBlocks ) {
    return PREFLATE_HUFF_STATIC;
  }
  if( info.countStaticHuffTreeBlocks==0 ) {
    return PREFLATE_HUFF_DYNAMIC;
  }
  return PREFLATE_HUFF_MIXED;
}

PreflateParameters estimatePreflateParameters(const std::vector<byte> &unpacked_output, const size_t off0, const std::vector<PreflateTokenBlock> &blocks) {
  PreflateStreamInfo info = extractPreflateInfo(blocks);
  PreflateParameters result;
  result.windowBits = estimatePreflateWindowBits(info.maxDist);
  result.memLevel = estimatePreflateMemLevel(info.maxTokensPerBlock);
  result.strategy = estimatePreflateStrategy(info);
  result.huffStrategy = estimatePreflateHuffStrategy(info);
  PreflateCompLevelInfo cl = estimatePreflateCompLevel(result.windowBits, result.memLevel, unpacked_output, off0, blocks, false);
  result.compLevel = cl.recommendedCompressionLevel;
  result.zlibCompatible = cl.zlibCompatible;
  result.farLen3MatchesDetected = cl.farLen3Matches;
  result.veryFarMatchesDetected = cl.veryFarMatches;
  result.matchesToStartDetected = cl.matchToStart;
  result.log2OfMaxChainDepthM1 = cl.maxChainDepth==0 ? 0 : bitLength(cl.maxChainDepth-1);
  return result;
}

const PreflateParserConfig fastPreflateParserSettings[3] = {
  {4, 4, 8, 4},
  {4, 5, 16, 8},
  {4, 6, 32, 32},
};
const PreflateParserConfig slowPreflateParserSettings[6] = {
  {4, 4, 16, 16}, {8, 16, 32, 32}, {8, 16, 128, 128}, {8, 32, 128, 256}, {32, 128, 258, 1024}, {32, 258, 258, 4096},
};
PreflatePredictorState::PreflatePredictorState(const PreflateHashChainExt &hash_, const PreflateSeqChain &seq_, const PreflateParserConfig &config_, const int wbits, const int mbits) : hash(hash_), seq(seq_), windowBytes(1<<wbits), maxTokenCount((1<<(6+mbits))-1), config(config_) {
}

uint PreflatePredictorState::prefixCompare(const byte* s1, const byte* s2, const uint bestLen, const uint maxLen) {
  if( s1[bestLen]!=s2[bestLen] ) {
    return 0;
  }
  if( s1[0]!=s2[0]||s1[1]!=s2[1]||s1[2]!=s2[2] ) {
    return 0;
  }
  const byte* scan = s2+3;
  const byte* match = s1+3;
  const byte* scanend = s2+maxLen;
  while( scan<scanend&&*scan==*match ) {
    ++scan;
    ++match;
  }
  return scan-s2;
}

uint PreflatePredictorState::suffixCompare(const byte* s1, const byte* s2, const uint bestLen, const uint maxLen) {
  if( s1[bestLen]!=s2[bestLen] ) {
    return 0;
  }
  uint len = 0;
  while( s1[len]==s2[len]&&++len<maxLen ) {
  }
  return len;
}

bool PreflatePredictorState::createMatchHelper(MatchHelper &helper, const uint prevLen, const uint startPos, const bool veryFarMatches, const bool matchesToStart, const uint maxDepth) {
  helper.maxLen = std::min(totalInputSize()-startPos, (uint)PreflateConstants::MAX_MATCH);
  if( helper.maxLen<std::max<uint32_t>(prevLen+1, PreflateConstants::MIN_MATCH) ) {
    return false;
  }
  helper.startPos = startPos;
  uint maxDistToStart = startPos-(matchesToStart ? 0 : 1);
  if( veryFarMatches ) {
    helper.curMaxDistHop1Plus = helper.curMaxDistHop0 = std::min(maxDistToStart, windowSize());
  } else {
    uint maxDist = windowSize()-PreflateConstants::MIN_LOOKAHEAD;
    helper.curMaxDistHop0 = std::min(maxDistToStart, maxDist);
    helper.curMaxDistHop1Plus = std::min(maxDistToStart, maxDist-1);
  }
  if( maxDepth>0 ) {
    helper.maxChain = maxDepth;
    helper.niceLen = helper.maxLen;
  } else {
    helper.maxChain = maxChainLength();
    helper.niceLen = std::min(niceMatchLength(), helper.maxLen);
    if( prevLen>=goodMatchLength() ) {
      helper.maxChain >>= 2;
    }
  }
  return true;
}

PreflateToken PreflatePredictorState::match(const uint hashHead, const uint prevLen, const uint offset, const bool veryFarMatches, const bool matchesToStart, const uint maxDepth) {
  PreflateToken bestMatch(PreflateToken::NONE);
  MatchHelper h;
  if( !createMatchHelper(h, prevLen, currentInputPos()+offset, veryFarMatches, matchesToStart, maxDepth) ) {
    return bestMatch;
  }
  PreflateHashIterator chainIt = iterateFromNode(hashHead, h.startPos, h.curMaxDistHop1Plus);
  if( chainIt.dist()>h.curMaxDistHop0 ) {
    return bestMatch;
  }
  const byte* input = inputCursor()+offset;
  uint bestLen = prevLen;
  do {
    const byte* match = input-chainIt.dist();
    uint matchLength = prefixCompare(match, input, bestLen, h.maxLen);
    if( matchLength>bestLen ) {
      bestLen = matchLength;
      bestMatch = PreflateToken(PreflateToken::REFERENCE, matchLength, chainIt.dist());
      if( bestLen>=h.niceLen ) {
        break;
      }
    }
  } while( chainIt.next()&&h.maxChain-->1 );
  return bestMatch;
}

PreflateToken PreflatePredictorState::seqMatch(const uint startPos, const uint hashHead, const uint prevLen, const bool veryFarMatches, const bool matchesToStart, const uint maxDepth) {
  PreflateToken bestMatch(PreflateToken::NONE);
  MatchHelper h;
  if( !createMatchHelper(h, prevLen, startPos, veryFarMatches, matchesToStart, maxDepth) ) {
    return bestMatch;
  }
  PreflateSeqIterator chainIt = seq.iterateFromPos(startPos);
  if( !chainIt ) {
    return bestMatch;
  }
  uint curSeqLen = std::min<uint32_t>(seq.len(startPos), h.maxLen);
  uint curMaxDist = h.curMaxDistHop1Plus;
  uint bestLen = prevLen;
  if( curSeqLen<PreflateConstants::MIN_MATCH ) {
    curSeqLen = std::min(chainIt.len()-chainIt.dist(), h.maxLen);
    if( curSeqLen>prevLen&&1<=h.curMaxDistHop0 ) {
      bestLen = curSeqLen;
      bestMatch = PreflateToken(PreflateToken::REFERENCE, curSeqLen, 1);
    }
    if( bestLen>=h.niceLen||!chainIt.next() ) {
      return bestMatch;
    }
    if( chainIt.dist()>h.curMaxDistHop1Plus+chainIt.len()-PreflateConstants::MIN_MATCH ) {
      return bestMatch;
    }
  } else {
    uint minDistOff = chainIt.len()-PreflateConstants::MIN_MATCH;
    if( chainIt.dist()>h.curMaxDistHop1Plus+minDistOff ) {
      if( chainIt.dist()>h.curMaxDistHop0+minDistOff ) {
        return bestMatch;
      }
      uint latestPos = h.startPos-chainIt.dist()+minDistOff;
      uint depth = hash.getRelPosDepth(latestPos, hashHead);
      if( depth==0 ) {
        curMaxDist = h.curMaxDistHop0;
      }
    }
  }
  const byte* input = inputCursor()+startPos-currentInputPos();
  uint bestSeqLen = std::min(curSeqLen, bestLen);
  do {
    if( chainIt.len()<bestSeqLen ) {
      continue;
    }
    uint oldBestSeqLen = bestSeqLen;
    bestSeqLen = std::min<uint32_t>(std::min<uint32_t>(curSeqLen, chainIt.len()), h.niceLen);
    uint bestDist = chainIt.dist()-chainIt.len()+bestSeqLen;
    uint error = 0;
    if( bestDist>curMaxDist ) {
      error = bestDist-curMaxDist;
      if( error>chainIt.len()-PreflateConstants::MIN_MATCH ) {
        break;
      }
    }
    uint bestChainDepth = hash.getRelPosDepth(h.startPos-bestDist+error, hashHead);
    if( bestChainDepth>=h.maxChain ) {
      error += bestChainDepth-h.maxChain+1;
      if( error>chainIt.len()-PreflateConstants::MIN_MATCH ) {
        break;
      }
    }
    if( error ) {
      if( bestSeqLen>std::max<uint32_t>(oldBestSeqLen, PreflateConstants::MIN_MATCH-1)+error ) {
        bestMatch = PreflateToken(PreflateToken::REFERENCE, bestSeqLen-error, bestDist-error);
      }
      break;
    }
    if( bestSeqLen==h.maxLen ) {
      bestMatch = PreflateToken(PreflateToken::REFERENCE, bestSeqLen, bestDist);
      break;
    } else {
      const byte* match = input-bestDist;
      uint matchLength = bestSeqLen+suffixCompare(match+bestSeqLen, input+bestSeqLen, std::max(bestLen, bestSeqLen)-bestSeqLen, h.maxLen-bestSeqLen);
      if( matchLength>bestLen ) {
        bestLen = matchLength;
        bestMatch = PreflateToken(PreflateToken::REFERENCE, matchLength, bestDist);
        if( bestLen>=h.niceLen ) {
          break;
        }
      }
    }
    curMaxDist = h.curMaxDistHop1Plus;
  } while( chainIt.next() );
  return bestMatch;
}

PreflateNextMatchInfo PreflatePredictorState::nextMatchInfo(const uint hashHead, const PreflateToken &targetReference, const PreflateHashChainExt &hash) {
  PreflateNextMatchInfo result;
  result.nextChainDepth = (uint16_t)~0u;
  result.nextLen = 0;
  result.nextDist = 0xffff;
  uint maxLen = std::min(availableInputSize(), (uint)PreflateConstants::MAX_MATCH);
  if( maxLen<(uint)PreflateConstants::MIN_MATCH ) {
    return result;
  }
  uint maxDist = windowSize()-PreflateConstants::MIN_LOOKAHEAD-1;
  uint curPos = currentInputPos();
  uint curMaxDist = std::min(curPos-1, maxDist);
  uint curMaxDistAlt = std::min(curPos-1, windowSize()-PreflateConstants::MIN_LOOKAHEAD);
  const byte* input = inputCursor();
  uint startDepth = hash.getNodeDepth(hashHead);
  uint maxChainOrg = maxChainLength();
  PreflateHashIterator chainIt = hash.iterateFromPos(curPos-targetReference.dist, curPos, curMaxDist);
  if( !chainIt.curPos||(hashHead==chainIt.curPos&&chainIt.dist()>curMaxDistAlt)||(hashHead!=chainIt.curPos&&chainIt.dist()>curMaxDist) ) {
    return result;
  }
  uint endDepth = chainIt.depth();
  uint maxChain = maxChainOrg-std::min(startDepth-endDepth, 0xffffu);
  uint bestLen = targetReference.len;
  while( maxChain>0 ) {
    if( !chainIt.next() ) {
      break;
    }
    const byte* match = input-chainIt.dist();
    uint matchLength = prefixCompare(match, input, bestLen, maxLen);
    if( matchLength>bestLen ) {
      result.nextLen = matchLength;
      result.nextChainDepth = maxChainOrg-maxChain;
      result.nextDist = chainIt.dist();
      break;
    }
    --maxChain;
  }
  return result;
}

PreflateRematchInfo PreflatePredictorState::rematchInfo(const uint hashHead, const PreflateToken &targetReference) {
  PreflateRematchInfo result;
  result.firstMatchDepth = 0xffff;
  result.requestedMatchDepth = 0xffff;
  result.condensedHops = 0;
  uint maxLen = std::min(availableInputSize(), (uint)PreflateConstants::MAX_MATCH);
  if( maxLen<targetReference.len ) {
    return result;
  }
  uint maxDist = windowSize();
  uint curPos = currentInputPos();
  uint curMaxDist = std::min(curPos, maxDist);
  PreflateHashIterator chainIt = hash.iterateFromNode(hashHead, curPos, curMaxDist);
  if( !chainIt ) {
    return result;
  }
  const byte* input = inputCursor();
  uint maxChainOrg = 0xffff;
  uint maxChain = maxChainOrg;
  uint bestLen = targetReference.len;
  do {
    const byte* match = input-chainIt.dist();
    uint matchLength = prefixCompare(match, input, bestLen-1, bestLen);
    if( matchLength>=bestLen ) {
      result.firstMatchDepth = std::min((uint)result.firstMatchDepth, maxChainOrg-maxChain);
      result.condensedHops++;
    }
    if( chainIt.dist()>=targetReference.dist ) {
      if( chainIt.dist()==targetReference.dist ) {
        result.requestedMatchDepth = maxChainOrg-maxChain;
      }
      return result;
    }
    chainIt.next();
  } while( !!chainIt&&maxChain-->1 );
  return result;
}

uint PreflatePredictorState::firstMatch(const uint len) {
  uint maxLen = std::min(availableInputSize(), (uint)PreflateConstants::MAX_MATCH);
  if( maxLen<std::max(len, (uint)PreflateConstants::MIN_MATCH) ) {
    return 0;
  }
  uint curPos = currentInputPos();
  uint curMaxDist = std::min(curPos, windowSize());
  uint hash = calculateHash();
  PreflateHashIterator chainIt = iterateFromHead(hash, curPos, curMaxDist);
  if( !chainIt ) {
    return 0;
  }
  const byte* input = inputCursor();
  do {
    const byte* match = input-chainIt.dist();
    uint matchLength = prefixCompare(match, input, len-1, len);
    if( matchLength>=len ) {
      return chainIt.dist();
    }
  } while( chainIt.next() );
  return 0;
}

uint PreflatePredictorState::hopMatch(const PreflateToken &targetReference, const uint hops) {
  if( hops==0 ) {
    return targetReference.dist;
  }
  uint curPos = currentInputPos();
  uint errorDist = 0;
  uint maxLen = std::min(availableInputSize(), (uint)PreflateConstants::MAX_MATCH);
  if( maxLen<targetReference.len ) {
    return errorDist;
  }
  uint maxDist = windowSize();
  uint curMaxDist = std::min(curPos, maxDist);
  PreflateHashIterator chainIt = iterateFromDist(targetReference.dist, curPos, curMaxDist);
  if( !chainIt ) {
    return 0;
  }
  const byte* input = inputCursor();
  uint bestLen = targetReference.len;
  for( uint todo = hops; todo>0; ) {
    if( !chainIt.next() ) {
      break;
    }
    const byte* match = input-chainIt.dist();
    uint matchLength = prefixCompare(match, input-targetReference.dist, bestLen-1, bestLen);
    if( matchLength>=bestLen ) {
      if( --todo==0 ) {
        return chainIt.dist();
      }
    }
  }
  return errorDist;
}

class PreflateReencoderTask {
public:
class Handler {
public:
virtual ~Handler() {
}

virtual bool beginDecoding(const uint32_t metaBlockId, PreflatePredictionDecoder &, PreflateParameters &) = 0;
virtual bool endDecoding(const uint32_t metaBlockId, PreflatePredictionDecoder &, std::vector<PreflateTokenBlock> &&tokenData, std::vector<uint8_t> &&uncompressedData, const size_t uncompressedOffset, const size_t paddingBitCount, const size_t paddingValue) = 0;
virtual void markProgress() = 0;
};
PreflateReencoderTask(Handler &handler, const uint32_t metaBlockId, std::vector<uint8_t> &&uncompressedData, const size_t uncompressedOffset, const bool lastMetaBlock);
bool decodeAndRepredict();
bool reencode();
uint32_t id() {
  return metaBlockId;
}

private:
Handler &handler;
uint32_t metaBlockId;
std::vector<uint8_t> uncompressedData;
size_t uncompressedOffset;
bool lastMetaBlock;
std::vector<PreflateTokenBlock> tokenData;
PreflatePredictionDecoder pcodec;
size_t paddingBitCount;
size_t paddingBits;
};
bool preflate_reencode(std::vector<byte> &deflate_raw, const std::vector<byte> &preflate_diff, const std::vector<byte> &unpacked_input);
bool preflate_reencode(OutputStream &os, const std::vector<byte> &preflate_diff, InputStream &unpacked_input, const uint64_t unpacked_size, std::function<void(void)> block_callback);
bool preflate_reencode(OutputStream &os, const std::vector<byte> &preflate_diff, const std::vector<byte> &unpacked_input, std::function<void(void)> block_callback);
class PreflateReencoderHandler : public PreflateReencoderTask::Handler {
public:
PreflateReencoderHandler(BitOutputStream &bos_, const std::vector<uint8_t> &reconData, const size_t uncompressedSize, std::function<void(void)> progressCallback_) : decoder(reconData, uncompressedSize), progressCallback(progressCallback_), bos(bos_) {
}

size_t metaBlockCount() const {
  return decoder.metaBlockCount();
}

size_t metaBlockUncompressedSize(const size_t metaBlockId) const {
  return decoder.metaBlockUncompressedSize(metaBlockId);
}

bool error() const {
  return decoder.error();
}

bool finish() {
  decoder.finish();
  return !decoder.error();
}

virtual bool beginDecoding(const uint32_t metaBlockId, PreflatePredictionDecoder &codec, PreflateParameters &params) {
  return decoder.beginMetaBlock(codec, params, metaBlockId);
}

virtual bool endDecoding(const uint32_t metaBlockId, PreflatePredictionDecoder &codec, std::vector<PreflateTokenBlock> &&tokenData, std::vector<uint8_t> &&uncompressedData, const size_t uncompressedOffset, const size_t paddingBitCount, const size_t paddingValue) {
  if( !decoder.endMetaBlock(codec) ) {
    return false;
  }
  PreflateBlockReencoder deflater(bos, uncompressedData, uncompressedOffset);
  for( size_t j = 0, n = tokenData.size(); j<n; ++j ) {
    deflater.writeBlock(tokenData[j], metaBlockId+1==decoder.metaBlockCount()&&j+1==n);
    markProgress();
  }
  bos.put(paddingValue, paddingBitCount);
  return true;
}

virtual void markProgress() {
  std::unique_lock<std::mutex> lock(this->_mutex);
  progressCallback();
}

private:
PreflateMetaDecoder decoder;
std::function<void(void)> progressCallback;
BitOutputStream &bos;
std::mutex _mutex;
};
PreflateReencoderTask::PreflateReencoderTask(PreflateReencoderHandler::Handler &handler_, const uint32_t metaBlockId_, std::vector<uint8_t> &&uncompressedData_, const size_t uncompressedOffset_, const bool lastMetaBlock_) : handler(handler_), metaBlockId(metaBlockId_), uncompressedData(uncompressedData_), uncompressedOffset(uncompressedOffset_), lastMetaBlock(lastMetaBlock_) {
}

bool PreflateReencoderTask::decodeAndRepredict() {
  PreflateParameters params;
  if( !handler.beginDecoding(metaBlockId, pcodec, params) ) {
    return false;
  }
  PreflateTokenPredictor tokenPredictor(params, uncompressedData, uncompressedOffset);
  PreflateTreePredictor treePredictor(uncompressedData, uncompressedOffset);
  bool eof = true;
  do {
    PreflateTokenBlock block = tokenPredictor.decodeBlock(&pcodec);
    if( !treePredictor.decodeBlock(block, &pcodec) ) {
      return false;
    }
    if( tokenPredictor.predictionFailure||treePredictor.predictionFailure ) {
      return false;
    }
    tokenData.push_back(std::move(block));
    if( !lastMetaBlock ) {
      eof = tokenPredictor.inputEOF();
    } else {
      eof = tokenPredictor.decodeEOF(&pcodec);
    }
    handler.markProgress();
  } while( !eof );
  paddingBitCount = 0;
  paddingBits = 0;
  if( lastMetaBlock ) {
    bool non_zero_bits = pcodec.decodeNonZeroPadding();
    if( non_zero_bits ) {
      paddingBitCount = pcodec.decodeValue(3);
      if( paddingBitCount>0 ) {
        paddingBits = (1<<(paddingBitCount-1))+pcodec.decodeValue(paddingBitCount-1);
      }
    }
  }
  return true;
}

bool PreflateReencoderTask::reencode() {
  return handler.endDecoding(metaBlockId, pcodec, std::move(tokenData), std::move(uncompressedData), uncompressedOffset, paddingBitCount, paddingBits);
}

bool preflate_reencode(OutputStream &os, const std::vector<byte> &preflate_diff, InputStream &is, const uint64_t unpacked_size, std::function<void(void)> block_callback) {
  BitOutputStream bos(os);
  PreflateReencoderHandler decoder(bos, preflate_diff, unpacked_size, block_callback);
  if( decoder.error() ) {
    return false;
  }
  std::vector<uint8_t> uncompressedData;
  std::queue<std::future<std::shared_ptr<PreflateReencoderTask> > > futureQueue;
  size_t maxMetaBlockSize = 1;
  for( size_t j = 0, n = decoder.metaBlockCount(); j<n; ++j ) {
    maxMetaBlockSize = std::max(maxMetaBlockSize, decoder.metaBlockUncompressedSize(j));
  }
  size_t queueLimit = std::min(2*globalTaskPool.extraThreadCount(), (1<<26)/maxMetaBlockSize);
  bool fail = false;
  for( size_t j = 0, n = decoder.metaBlockCount(); j<n; ++j ) {
    size_t curUncSize = uncompressedData.size();
    size_t newSize = decoder.metaBlockUncompressedSize(j);
    uncompressedData.resize(curUncSize+newSize);
    if( is.read(uncompressedData.data()+curUncSize, newSize)!=newSize ) {
      return false;
    }
    if( futureQueue.empty()&&(queueLimit==0||j+1==n) ) {
      PreflateReencoderTask task(decoder, j, std::vector<uint8_t>(uncompressedData), curUncSize, j+1==n);
      if( !task.decodeAndRepredict()||!task.reencode() ) {
        return false;
      }
    } else {
      if( futureQueue.size()>=queueLimit ) {
        std::future<std::shared_ptr<PreflateReencoderTask> > first = std::move(futureQueue.front());
        futureQueue.pop();
        std::shared_ptr<PreflateReencoderTask> data = first.get();
        if( fail||!data||!data->reencode() ) {
          fail = true;
        }
      }
      std::shared_ptr<PreflateReencoderTask> ptask;
      ptask.reset(new PreflateReencoderTask(decoder, j, std::vector<uint8_t>(uncompressedData), curUncSize, j+1==n));
      futureQueue.push(globalTaskPool.addTask([ptask, &fail]() {
        if( !fail&&ptask->decodeAndRepredict() ) {
          return ptask;
        } else {
          return std::shared_ptr<PreflateReencoderTask>();
        }
      }));
    }
    if( j+1<n ) {
      uncompressedData.erase(uncompressedData.begin(), uncompressedData.begin()+std::max<size_t>(uncompressedData.size(), 1<<15)-(1<<15));
    }
  }
  while( !futureQueue.empty() ) {
    std::future<std::shared_ptr<PreflateReencoderTask> > first = std::move(futureQueue.front());
    futureQueue.pop();
    std::shared_ptr<PreflateReencoderTask> data = first.get();
    if( fail||!data||!data->reencode() ) {
      fail = true;
    }
  }
  bos.flush();
  return !fail&&!decoder.error();
}

bool preflate_reencode(OutputStream &os, const std::vector<byte> &preflate_diff, const std::vector<byte> &unpacked_input, std::function<void(void)> block_callback) {
  MemStream is(unpacked_input);
  return preflate_reencode(os, preflate_diff, is, unpacked_input.size(), block_callback);
}

bool preflate_reencode(std::vector<byte> &deflate_raw, const std::vector<byte> &preflate_diff, const std::vector<byte> &unpacked_input) {
  MemStream mem;
  bool result = preflate_reencode(mem, preflate_diff, unpacked_input, [] {
  });
  deflate_raw = mem.extractData();
  return result;
}

PreflateSeqChain::PreflateSeqChain(const std::vector<byte> &input_) : _input(input_), totalShift(-8), curPos(0) {
  prev = new SeqChainEntry[1<<16];
  memset(heads, 0x00, sizeof(heads));
  _build(8, std::min<uint32_t>((1<<16)-8, _input.remaining()));
}

PreflateSeqChain::~PreflateSeqChain() {
  delete[] prev;
}

void PreflateSeqChain::_reshift() {
  const uint16_t delta = 0x7e00;
  uint remaining = (1<<16)-(delta+8);
  if( prev[delta+8].distToNext!=0xffff&&prev[delta+8].length<PreflateConstants::MIN_MATCH ) {
    uint d = prev[delta+8].distToNext;
    prev[delta+8].distToNext = 0xffff;
    prev[delta+8].length = prev[delta+8-d].length-d;
    for( uint i = 3; i<prev[delta+8].length; ++i ) {
      prev[delta+8+i-2].distToNext -= d;
    }
    uint8_t c = *_input.curChars(-(int)remaining);
    if( heads[c]==delta+8-d ) {
      heads[c] += d;
    } else {
      for( uint i = prev[delta+8].length; i<remaining; ++i ) {
        if( prev[delta+8+i].distToNext==i+d ) {
          prev[delta+8+i].distToNext -= d;
          break;
        }
      }
    }
  }
  for( uint i = 0; i<256; ++i ) {
    heads[i] = std::max(heads[i], delta)-delta;
  }
  memmove(prev+8, prev+(delta+8), sizeof(SeqChainEntry)*remaining);
  totalShift += delta;
  _build(8+remaining, std::min<uint32_t>(delta, _input.remaining()));
}

void PreflateSeqChain::_build(const uint off0, const uint size) {
  if( !size ) {
    return;
  }
  const byte* b = _input.curChars();
  uint8_t curChar = b[0];
  SeqChainEntry startOfSeq = {0xffff, 0x0}, * ptrToFirstOfSeq;
  uint startOff = off0;
  prev[off0] = startOfSeq;
  if( off0>8&&curChar==b[-1] ) {
    --startOff;
    if( curChar==b[-2] ) {
      --startOff;
      if( curChar==b[-3] ) {
        startOff = heads[curChar];
        prev[off0-2].distToNext = off0-startOff-2;
        prev[off0-1].distToNext = off0-startOff-1;
        prev[off0].distToNext = off0-startOff;
        prev[off0].length = 1;
      } else {
        prev[startOff].distToNext = startOff-heads[curChar];
        prev[startOff+1].distToNext = 1;
        prev[startOff+2].distToNext = 2;
        prev[startOff+2].length = 1;
        heads[curChar] = startOff;
      }
    } else {
      prev[startOff+1].distToNext = 1;
      prev[startOff+1].length = 1;
    }
  }
  ptrToFirstOfSeq = &prev[startOff];
  ++ptrToFirstOfSeq->length;
  uint8_t prevChar = curChar;
  for( uint i = 1; i<size; ++i ) {
    curChar = b[i];
    if( prevChar==curChar ) {
      if( ++ptrToFirstOfSeq->length==3 ) {
        prev[startOff].distToNext = startOff-heads[prevChar];
        heads[prevChar] = startOff;
      }
      prev[off0+i].distToNext = off0+i-startOff;
      prev[off0+i].length = 1;
    } else {
      if( ptrToFirstOfSeq->length>=2 ) {
        if( ptrToFirstOfSeq->length>=3 ) {
          prev[off0+i-2].distToNext = 0xffff;
        }
        prev[off0+i-1].distToNext = 0xffff;
      }
      prev[off0+i] = startOfSeq;
      startOff = off0+i;
      ptrToFirstOfSeq = &prev[startOff];
      ++ptrToFirstOfSeq->length;
    }
    prevChar = curChar;
  }
  if( ptrToFirstOfSeq->length>=2 ) {
    if( ptrToFirstOfSeq->length>=3 ) {
      prev[off0+size-2].distToNext = 0xffff;
    }
    prev[off0+size-1].distToNext = 0xffff;
  }
  _input.advance(size);
}

uint sumArray(const uint* data, const uint n);
template <uint N> inline uint sumArray(const uint (&data)[N]) {
  return sumArray(data, N);
}

template <uint N> void PreflateSubModel<N>::build_impl(const uint* arr, const uint defval, const uint8_t prec) {
  if( N==0 ) {
    isDefault = true;
    return;
  }
  for( uint i = 0; i<N; ++i ) {
    ids[i] = i;
  }
  std::sort(ids, ids+N, [=](uint i1, uint i2) {
    if( arr[i1]!=arr[i2] ) {
      return arr[i1]<arr[i2];
    }
    return i1<i2;
  });
  for( uint i = 0; i<N; ++i ) {
    bounds[i] = arr[ids[i]];
    rids[ids[i]] = i;
  }
  uint sum = sumArray(bounds, N), acc, prev;
  prev = bounds[0];
  bounds[0] = acc = 0;
  for( uint i = 0; i<N; ++i ) {
    if( prev ) {
      acc += prev;
      prev = bounds[i+1];
      int diff = (((uint64_t)acc)<<16)/sum-bounds[i];
      uint diff_bits = bitLength(diff);
      const uint k = 5;
      if( diff>0&&diff_bits>k ) {
        diff = diff&(((1<<k)-1)<<(diff_bits-k));
      }
      bounds[i+1] = bounds[i]+diff;
      if( bounds[i+1]<=bounds[i] ) {
        bounds[i+1] = bounds[i]+1;
      }
    } else {
      prev = bounds[i+1];
      bounds[i+1] = bounds[i];
    }
  }
  if( bounds[N]>0 ) {
    bounds[N] = 1<<16;
  }
  isDefault = N==0||bounds[N]==0||(bounds[N-1]==0&&ids[N-1]==defval);
  build_scale_down();
}

template <uint N> void PreflateSubModel<N>::buildDefault(const uint defval) {
  if( N==0 ) {
    isDefault = true;
    return;
  }
  memset(bounds, 0, N*sizeof(uint));
  memset(scaledDownBounds, 0, N*sizeof(uint));
  bounds[N] = 0x10000;
  ids[N-1] = defval;
  rids[defval] = N-1;
  isDefault = true;
  build_scale_down();
}

template <uint N> void PreflateSubModel<N>::build_scale_down() {
  uint boundBits = ~0xFFFFu;
  for( uint i = 0; i<=N; ++i ) {
    boundBits |= bounds[i];
  }
  uint zeroJunk = bitTrailingZeroes(boundBits);
  scaleDownBits = (16-zeroJunk);
  for( uint i = 0; i<=N; ++i ) {
    scaledDownBounds[i] = bounds[i]>>zeroJunk;
  }
  isFixed = bounds[N-1]==0;
}

static void encodeProb(ArithmeticEncoder &codec, const uint val) {
  uint bits = bitLength(val);
  codec.encodeBits(bits-1, 4);
  if( bits>=5 ) {
    codec.encodeBits((val>>(bits-5))&0xf, 4);
  } else {
    codec.encodeBits(val&~(1<<(bits-1)), bits-1);
  }
}

static void encodeId(ArithmeticEncoder &codec, const uint id, const uint count) {
  uint bits = bitLength(count-1);
  codec.encodeBits(id, bits);
}

static uint decodeProb(ArithmeticDecoder &codec) {
  uint bits = codec.decodeBits(4)+1;
  if( bits>=5 ) {
    return (codec.decodeBits(4)|0x10)<<(bits-5);
  } else {
    return codec.decodeBits(bits-1)|(1<<(bits-1));
  }
}

static uint decodeId(ArithmeticDecoder &codec, const uint count) {
  uint bits = bitLength(count-1);
  return codec.decodeBits(bits);
}

template <uint N> void PreflateSubModel<N>::write(ArithmeticEncoder &codec, const uint8_t) const {
  uint zeros = 0;
  for( uint i = 1; i<N; ++i ) {
    if( !bounds[i] ) {
      ++zeros;
    } else {
      break;
    }
  }
  codec.encodeBits(zeros, bitLength(N-1));
  for( uint i = 1+zeros; i<N; ++i ) {
    encodeProb(codec, bounds[i]-bounds[i-1]);
  }
  for( uint i = zeros; i<N; ++i ) {
    encodeId(codec, ids[i], N);
  }
}

template <uint N> void PreflateSubModel<N>::read(ArithmeticDecoder &codec, const uint8_t) {
  uint zeros = codec.decodeBits(bitLength(N-1));
  memset(bounds, 0, sizeof(bounds));
  for( uint i = 1+zeros; i<N; ++i ) {
    bounds[i] = decodeProb(codec)+bounds[i-1];
  }
  bounds[N] = 1<<16;
  for( uint i = zeros; i<N; ++i ) {
    ids[i] = decodeId(codec, N);
    rids[ids[i]] = i;
  }
  build_scale_down();
}

template <uint NEG, uint POS> void PreflateCorrectionSubModel<NEG, POS>::build_impl(const uint* arr, const int defval, const uint8_t prec) {
  uint signArr[3] = {arr[NEG], sumArray(arr+NEG+1, POS), sumArray(arr, NEG)};
  sign.build_impl(signArr, defval==0 ? 0 : (defval>0 ? 1 : 2), prec);
  uint posArr[POS+1];
  for( uint i = 0; i<POS; ++i ) {
    posArr[i] = arr[NEG+1+i];
  }
  pos.build_impl(posArr, defval>0&&defval<=POS ? defval-1 : POS-1, prec);
  uint negArr[NEG+1];
  for( uint i = 0; i<NEG; ++i ) {
    negArr[i] = arr[NEG-1-i];
  }
  neg.build_impl(negArr, -defval>0&&-defval<=NEG ? -defval-1 : NEG-1, prec);
  isDefault = sign.isDefault&&pos.isDefault&&neg.isDefault;
}

template <uint NEG, uint POS> void PreflateCorrectionSubModel<NEG, POS>::buildDefault(const uint defval) {
  sign.buildDefault(defval==0 ? 0 : (defval>0 ? 1 : 2));
  pos.buildDefault(defval>0&&defval<=POS ? defval-1 : POS-1);
  neg.buildDefault(-defval>0&&-defval<=NEG ? -defval-1 : NEG-1);
  isDefault = sign.isDefault&&pos.isDefault&&neg.isDefault;
}

template <uint NEG, uint POS> void PreflateCorrectionSubModel<NEG, POS>::write(ArithmeticEncoder &codec, const uint8_t prec) const {
  sign.write(codec, prec);
  if( POS>0 ) {
    pos.write(codec, prec);
  }
  if( NEG>0 ) {
    neg.write(codec, prec);
  }
}

template <uint NEG, uint POS> void PreflateCorrectionSubModel<NEG, POS>::read(ArithmeticDecoder &codec, const uint8_t prec) {
  sign.read(codec, prec);
  if( POS>0 ) {
    pos.read(codec, prec);
  }
  if( NEG>0 ) {
    neg.read(codec, prec);
  }
}

PreflateBaseModel::PreflateBaseModel() : encoder(nullptr), decoder(nullptr) {
}

void PreflateBaseModel::setEncoderStream(ArithmeticEncoder* codec_) {
  encoder = codec_;
}

void PreflateBaseModel::setDecoderStream(ArithmeticDecoder* codec_) {
  decoder = codec_;
}

template <uint N> void PreflateBaseModel::readSubModel(PreflateSubModel<N> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec) {
  if( isFullDef||cc.nonDefaultValue.decode(*decoder)==0 ) {
    sm.buildDefault(defVal);
  } else {
    sm.read(*decoder, prec);
  }
}

template <uint N, uint M> void PreflateBaseModel::readSubModel(PreflateCorrectionSubModel<N, M> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec) {
  if( isFullDef||cc.nonDefaultValue.decode(*decoder)==0 ) {
    sm.buildDefault(defVal);
  } else {
    sm.read(*decoder, prec);
  }
}

template <uint N> void PreflateBaseModel::writeSubModel(const PreflateSubModel<N> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec) {
  if( isFullDef ) {
    return;
  }
  bool ndef = !sm.isDefault;
  cc.nonDefaultValue.encode(*encoder, ndef);
  if( ndef ) {
    sm.write(*encoder, prec);
  }
}

template <uint N, uint M> void PreflateBaseModel::writeSubModel(const PreflateCorrectionSubModel<N, M> &sm, const bool isFullDef, const PreflateModelCodec &cc, const uint defVal, const uint8_t prec) {
  if( isFullDef ) {
    return;
  }
  bool ndef = !sm.isDefault;
  cc.nonDefaultValue.encode(*encoder, ndef);
  if( ndef ) {
    sm.write(*encoder, prec);
  }
}

void PreflateBlockPredictionModel::read(const PreflateStatisticsCounter::BlockPrediction &blockModel, const PreflateModelCodec &cc) {
  blockType.build(blockModel.blockType, PreflateTokenBlock::DYNAMIC_HUFF, cc.MBprecision);
  EOBMisprediction.build(blockModel.EOBMisprediction, 0, cc.MBprecision);
  nonZeroPadding.build(blockModel.nonZeroPadding, 0, cc.MBprecisionP1);
}

void PreflateBlockPredictionModel::readFromStream(const PreflateModelCodec &cc) {
  readSubModel(blockType, cc.blockFullDefault, cc, PreflateTokenBlock::DYNAMIC_HUFF, cc.MBprecision);
  readSubModel(EOBMisprediction, cc.blockFullDefault, cc, 0, cc.MBprecision);
  readSubModel(nonZeroPadding, cc.blockFullDefault, cc, 0, cc.MBprecisionP1);
}

void PreflateBlockPredictionModel::writeToStream(const PreflateModelCodec &cc) {
  writeSubModel(blockType, cc.blockFullDefault, cc, PreflateTokenBlock::DYNAMIC_HUFF, cc.MBprecision);
  writeSubModel(EOBMisprediction, cc.blockFullDefault, cc, 0, cc.MBprecision);
  writeSubModel(nonZeroPadding, cc.blockFullDefault, cc, 0, cc.MBprecisionP1);
}

void PreflateTreeCodePredictionModel::read(const PreflateStatisticsCounter::TreeCodePrediction &treecodeModel, const PreflateModelCodec &cc) {
  TCCountMisprediction.build(treecodeModel.TCCountMisprediction, 0, cc.MBprecision);
  LCountMisprediction.build(treecodeModel.LCountMisprediction, 0, cc.MBprecision);
  DCountMisprediction.build(treecodeModel.DCountMisprediction, 0, cc.MBprecision);
  for( uint i = 0; i<4; ++i ) {
    LDTypeMisprediction[i].build(treecodeModel.LDTypeMisprediction[i], 0);
  }
  LDTypeReplacementBase.build(treecodeModel.LDTypeReplacement, 0);
  TCBitlengthCorrection.build(treecodeModel.TCBitlengthCorrection, 0);
  LDBitlengthCorrection.build(treecodeModel.LDBitlengthCorrection, 0);
  LDRepeatCountCorrection.build(treecodeModel.LDRepeatCountCorrection, 0);
  deriveLDTypeReplacement();
}

void PreflateTreeCodePredictionModel::readFromStream(const PreflateModelCodec &cc) {
  readSubModel(TCCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  readSubModel(LCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  readSubModel(DCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  for( uint i = 0; i<4; ++i ) {
    readSubModel(LDTypeMisprediction[i], cc.treecodeFullDefault, cc, 0);
  }
  readSubModel(LDTypeReplacementBase, cc.treecodeFullDefault, cc, 0);
  readSubModel(TCBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  readSubModel(LDBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  readSubModel(LDRepeatCountCorrection, cc.treecodeFullDefault, cc, 0);
  deriveLDTypeReplacement();
}

void PreflateTreeCodePredictionModel::writeToStream(const PreflateModelCodec &cc) {
  writeSubModel(TCCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  writeSubModel(LCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  writeSubModel(DCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  for( uint i = 0; i<4; ++i ) {
    writeSubModel(LDTypeMisprediction[i], cc.treecodeFullDefault, cc, 0);
  }
  writeSubModel(LDTypeReplacementBase, cc.treecodeFullDefault, cc, 0);
  writeSubModel(TCBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  writeSubModel(LDBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  writeSubModel(LDRepeatCountCorrection, cc.treecodeFullDefault, cc, 0);
}

void PreflateTreeCodePredictionModel::deriveLDTypeReplacement() {
  uint arr[4], arr_mp[2], miss[4], hit[4], sumhit;
  LDTypeReplacementBase.extract(arr);
  for( uint i = 0; i<4; ++i ) {
    LDTypeMisprediction[i].extract(arr_mp);
    if( arr_mp[1]==0 ) {
      DerivedLDTypeReplacement[i].buildDefault(i);
    } else {
      if( arr_mp[0]==0 ) {
        arr_mp[1] = 1;
      }
      sumhit = 0;
      for( uint j = 0; j<4; ++j ) {
        hit[j] = arr[j]*arr_mp[0];
        miss[j] = arr[j]*arr_mp[1];
        sumhit += hit[j];
      }
      miss[i] = sumhit-hit[i];
      for( uint j = 0; j<4; ++j ) {
        if( miss[j]>0&&miss[j]<16 ) {
          miss[j] = 1;
        } else {
          miss[j] >>= 4;
        }
      }
      DerivedLDTypeReplacement[i].build(miss, i);
    }
  }
}

void PreflateTokenPredictionModel::read(const PreflateStatisticsCounter::TokenPrediction &tokenModel, const PreflateModelCodec &cc) {
  LITMisprediction.build(tokenModel.LITMisprediction, 0);
  REFMisprediction.build(tokenModel.REFMisprediction, 0);
  LENCorrection.build(tokenModel.LENCorrection, 0);
  DISTAfterLenCorrection.build(tokenModel.DISTAfterLenCorrection, 0);
  DISTOnlyCorrection.build(tokenModel.DISTOnlyCorrection, 0);
  IrregularLen258Encoding.build(tokenModel.LEN258IrregularEncoding, 0);
}

void PreflateTokenPredictionModel::readFromStream(const PreflateModelCodec &cc) {
  readSubModel(LITMisprediction, cc.tokenFullDefault, cc, 0);
  readSubModel(REFMisprediction, cc.tokenFullDefault, cc, 0);
  readSubModel(LENCorrection, cc.tokenFullDefault, cc, 0);
  readSubModel(DISTAfterLenCorrection, cc.tokenFullDefault, cc, 0);
  readSubModel(DISTOnlyCorrection, cc.tokenFullDefault, cc, 0);
  readSubModel(IrregularLen258Encoding, cc.tokenFullDefault, cc, 0);
}

void PreflateTokenPredictionModel::writeToStream(const PreflateModelCodec &cc) {
  writeSubModel(LITMisprediction, cc.tokenFullDefault, cc, 0);
  writeSubModel(REFMisprediction, cc.tokenFullDefault, cc, 0);
  writeSubModel(LENCorrection, cc.tokenFullDefault, cc, 0);
  writeSubModel(DISTAfterLenCorrection, cc.tokenFullDefault, cc, 0);
  writeSubModel(DISTOnlyCorrection, cc.tokenFullDefault, cc, 0);
  writeSubModel(IrregularLen258Encoding, cc.tokenFullDefault, cc, 0);
}

PreflatePredictionModel::PreflatePredictionModel() {
}

PreflatePredictionModel::~PreflatePredictionModel() {
}

void PreflatePredictionModel::read(const PreflateStatisticsCounter &model, const PreflateModelCodec &cc) {
  block.read(model.block, cc);
  treecode.read(model.treecode, cc);
  token.read(model.token, cc);
}

void PreflatePredictionModel::setEncoderStream(ArithmeticEncoder* codec) {
  block.setEncoderStream(codec);
  treecode.setEncoderStream(codec);
  token.setEncoderStream(codec);
}

void PreflatePredictionModel::setDecoderStream(ArithmeticDecoder* codec) {
  block.setDecoderStream(codec);
  treecode.setDecoderStream(codec);
  token.setDecoderStream(codec);
}

void PreflatePredictionModel::readFromStream(const PreflateModelCodec &cc) {
  block.readFromStream(cc);
  treecode.readFromStream(cc);
  token.readFromStream(cc);
}

void PreflatePredictionModel::writeToStream(const PreflateModelCodec &cc) {
  block.writeToStream(cc);
  treecode.writeToStream(cc);
  token.writeToStream(cc);
}

PreflateModelCodec::PreflateModelCodec() {
}

void PreflateModelCodec::initDefault() {
  blockFullDefault = true;
  treecodeFullDefault = true;
  tokenFullDefault = true;
  totalModels = 0;
  defaultingModels = 0;
  uint arr[2] = {1, 0};
  nonDefaultValue.build(arr, 0);
  MBprecision = 16;
  MBprecisionP1 = 16;
}

void PreflateModelCodec::read(const PreflateStatisticsCounter &m) {
  totalModels = 0;
  defaultingModels = 0;
  uint total_block = m.block.totalModels();
  uint defaulting_block = m.block.checkDefaultModels();
  blockFullDefault = total_block==defaulting_block;
  if( !blockFullDefault ) {
    totalModels += total_block;
    defaultingModels += defaulting_block;
  }
  uint total_tree = m.treecode.totalModels();
  uint defaulting_tree = m.treecode.checkDefaultModels();
  treecodeFullDefault = total_tree==defaulting_tree;
  if( !treecodeFullDefault ) {
    totalModels += total_tree;
    defaultingModels += defaulting_tree;
  }
  uint total_token = m.token.totalModels();
  uint defaulting_token = m.token.checkDefaultModels();
  tokenFullDefault = total_token==defaulting_token;
  if( !tokenFullDefault ) {
    totalModels += total_token;
    defaultingModels += defaulting_token;
  }
  if( totalModels>0 ) {
    uint arr[2] = {defaultingModels, totalModels-defaultingModels};
    nonDefaultValue.build(arr, 0);
  }
  MBprecision = 16;
  MBprecisionP1 = 16;
}

void PreflateModelCodec::readFromStream(ArithmeticDecoder &codec) {
  blockFullDefault = codec.decodeBits(1);
  treecodeFullDefault = codec.decodeBits(1);
  tokenFullDefault = codec.decodeBits(1);
  totalModels = 0;
  if( !blockFullDefault ) {
    totalModels += PreflateStatisticsCounter::BlockPrediction::totalModels();
  }
  if( !treecodeFullDefault ) {
    totalModels += PreflateStatisticsCounter::TreeCodePrediction::totalModels();
  }
  if( !tokenFullDefault ) {
    totalModels += PreflateStatisticsCounter::TokenPrediction::totalModels();
  }
  defaultingModels = PreflateBaseModel::decodeValue(codec, bitLength(totalModels));
  if( totalModels ) {
    uint arr[2] = {defaultingModels, totalModels-defaultingModels};
    nonDefaultValue.build(arr, 0);
  }
  MBprecision = 16;
  MBprecisionP1 = 16;
}

void PreflateModelCodec::writeToStream(ArithmeticEncoder &codec) {
  codec.encodeBits(blockFullDefault, 1);
  codec.encodeBits(treecodeFullDefault, 1);
  codec.encodeBits(tokenFullDefault, 1);
  codec.encodeBits(defaultingModels, bitLength(totalModels));
}

PreflatePredictionEncoder::PreflatePredictionEncoder() : storage(nullptr), bos(nullptr), encoder(nullptr) {
}

void PreflatePredictionEncoder::start(const PreflatePredictionModel &model_, const PreflateParameters &params_, const uint modelId_) {
  PreflatePredictionModel::operator=(model_);
  params = params_;
  modelid = modelId_;
  storage = new MemStream;
  bos = new BitOutputStream(*storage);
  encoder = new ArithmeticEncoder(*bos);
  setEncoderStream(encoder);
}

std::vector<uint8_t> PreflatePredictionEncoder::end() {
  setEncoderStream(nullptr);
  encoder->flush();
  delete encoder;
  bos->flush();
  delete bos;
  std::vector<byte> result = storage->extractData();
  delete storage;
  return result;
}

PreflatePredictionDecoder::PreflatePredictionDecoder() : storage(nullptr), bis(nullptr), decoder(nullptr) {
}

void PreflatePredictionDecoder::start(const PreflatePredictionModel &model_, const PreflateParameters &params_, const std::vector<uint8_t> &storage_, size_t off0, size_t size) {
  PreflatePredictionModel::operator=(model_);
  params = params_;
  storage = new MemStream(storage_, off0, size);
  bis = new BitInputStream(*storage);
  decoder = new ArithmeticDecoder(*bis);
  setDecoderStream(decoder);
}

void PreflatePredictionDecoder::end() {
  setDecoderStream(nullptr);
  delete decoder;
  delete bis;
  delete storage;
  decoder = nullptr;
  bis = nullptr;
  storage = nullptr;
}

PreflateMetaEncoder::PreflateMetaEncoder() : inError(false) {
}

PreflateMetaEncoder::~PreflateMetaEncoder() {
}

uint PreflateMetaEncoder::addModel(const PreflateStatisticsCounter &counter, const PreflateParameters &params) {
  uint modelId = modelList.size();
  modelType m;
  m.counter = counter;
  m.mcodec.read(counter);
  m.model.read(counter, m.mcodec);
  m.params = params;
  m.writtenId = 0;
  modelList.push_back(m);
  return modelId;
}

bool PreflateMetaEncoder::beginMetaBlockWithModel(PreflatePredictionEncoder &encoder, const uint modelId) {
  if( modelId>=modelList.size() ) {
    return false;
  }
  encoder.start(modelList[modelId].model, modelList[modelId].params, modelId);
  return true;
}

bool PreflateMetaEncoder::endMetaBlock(PreflatePredictionEncoder &encoder, const size_t uncompressed) {
  if( encoder.modelId()>=modelList.size() ) {
    return false;
  }
  metaBlockInfo m;
  std::vector<uint8_t> result = encoder.end();
  m.modelId = encoder.modelId();
  m.reconSize = result.size();
  m.uncompressedSize = uncompressed;
  blockList.push_back(m);
  reconData.insert(reconData.end(), result.begin(), result.end());
  return true;
}

std::vector<byte> PreflateMetaEncoder::finish() {
  MemStream mem;
  BitOutputStream bos(mem);
  bos.put(0, 1);
  bos.put(blockList.size()>1, 1);
  if( blockList.size()>1 ) {
    bos.putVLI(blockList.size()-2);
  }
  enum Mode { CREATE_NEW_MODEL };
  for( uint i = 0, n = blockList.size(); i<n; ++i ) {
    const metaBlockInfo &mb = blockList[i];
    Mode mode = CREATE_NEW_MODEL;
    if( i>0 ) {
      bos.put(3, 2);
    }
    switch( mode ) {
    case CREATE_NEW_MODEL: {
      modelType &mt = modelList[mb.modelId];
      bool perfectZLIB = mt.mcodec.blockFullDefault&&mt.mcodec.treecodeFullDefault&&mt.mcodec.tokenFullDefault&&mt.params.zlibCompatible;
      bos.put(!perfectZLIB, 1);
      bos.put(mt.params.compLevel, 4);
      bos.put(mt.params.memLevel, 4);
      bos.put(mt.params.windowBits-8, 3);
      if( !perfectZLIB ) {
        bos.put(mt.params.zlibCompatible, 1);
        if( !mt.params.zlibCompatible ) {
          bos.put(mt.params.veryFarMatchesDetected, 1);
          bos.put(mt.params.matchesToStartDetected, 1);
        }
        bos.put(mt.params.log2OfMaxChainDepthM1, 4);
        MemStream tmp_data;
        {
          BitOutputStream tmp_bos(tmp_data);
          ArithmeticEncoder tmp_codec(tmp_bos);
          mt.mcodec.writeToStream(tmp_codec);
          mt.model.setEncoderStream(&tmp_codec);
          mt.model.writeToStream(mt.mcodec);
          mt.model.setEncoderStream(nullptr);
          tmp_codec.flush();
          tmp_bos.flush();
        }
        std::vector<uint8_t> tmp_res = tmp_data.extractData();
        bos.putVLI(tmp_res.size());
        bos.putBytes(tmp_res.data(), tmp_res.size());
      }
      break;
    }
    }
    if( i!=n-1 ) {
      bos.putVLI(mb.reconSize);
      bos.putVLI(mb.uncompressedSize);
    }
  }
  bos.flush();
  std::vector<uint8_t> result = mem.extractData();
  result.insert(result.end(), reconData.begin(), reconData.end());
  return result;
}

PreflateMetaDecoder::PreflateMetaDecoder(const std::vector<uint8_t> &reconData_, const uint64_t uncompressedSize_) : inError(false), reconData(reconData_), uncompressedSize(uncompressedSize_) {
  if( reconData.size()==0 ) {
    inError = true;
    return;
  }
  MemStream mem(reconData);
  BitInputStream bis(mem);
  bool extension = bis.get(1);
  if( extension ) {
    inError = true;
    return;
  }
  bool singleBlock = bis.get(1)==0;
  size_t blockCount;
  if( singleBlock ) {
    blockCount = 1;
  } else {
    blockCount = 2+bis.getVLI();
  }
  enum Mode { CREATE_NEW_MODEL };
  for( size_t i = 0; i<blockCount; ++i ) {
    metaBlockInfo mb;
    Mode mode = CREATE_NEW_MODEL;
    if( i>0 ) {
      if( bis.get(2)!=3 ) {
        inError = true;
        return;
      }
    }
    switch( mode ) {
    case CREATE_NEW_MODEL: {
      modelType mt;
      memset(&mt, 0, sizeof(mt));
      bool perfectZLIB = bis.get(1)==0;
      mt.params.compLevel = bis.get(4);
      mt.params.memLevel = bis.get(4);
      mt.params.windowBits = bis.get(3)+8;
      if( perfectZLIB ) {
        mt.params.zlibCompatible = true;
        mt.mcodec.blockFullDefault = true;
        mt.mcodec.treecodeFullDefault = true;
        mt.mcodec.tokenFullDefault = true;
        mt.model.readFromStream(mt.mcodec);
      } else {
        mt.params.zlibCompatible = bis.get(1);
        if( !mt.params.zlibCompatible ) {
          mt.params.veryFarMatchesDetected = bis.get(1);
          mt.params.matchesToStartDetected = bis.get(1);
        }
        mt.params.log2OfMaxChainDepthM1 = bis.get(4);
        size_t res_size = bis.getVLI();
        {
          MemStream tmp_mem;
          bis.copyBytesTo(tmp_mem, res_size);
          tmp_mem.seek(0);
          BitInputStream tmp_bis(tmp_mem);
          ArithmeticDecoder tmp_codec(tmp_bis);
          mt.mcodec.readFromStream(tmp_codec);
          mt.model.setDecoderStream(&tmp_codec);
          mt.model.readFromStream(mt.mcodec);
          mt.model.setDecoderStream(nullptr);
        }
      }
      mb.modelId = modelList.size();
      modelList.push_back(mt);
      break;
    }
    }
    if( i!=blockCount-1 ) {
      mb.reconSize = bis.getVLI();
      mb.uncompressedSize = bis.getVLI();
    }
    blockList.push_back(mb);
  }
  bis.skipToByte();
  size_t reconStart = bis.bitPos()>>3;
  uint64_t uncStart = 0;
  for( size_t i = 0; i<blockCount; ++i ) {
    blockList[i].reconStartOfs = reconStart;
    blockList[i].uncompressedStartOfs = uncStart;
    if( i!=blockCount-1 ) {
      reconStart += blockList[i].reconSize;
      uncStart += blockList[i].uncompressedSize;
      if( reconStart>reconData.size()||uncStart>uncompressedSize ) {
        inError = true;
        return;
      }
    } else {
      blockList[i].reconSize = reconData.size()-blockList[i].reconStartOfs;
      blockList[i].uncompressedSize = uncompressedSize-blockList[i].uncompressedStartOfs;
    }
  }
}

PreflateMetaDecoder::~PreflateMetaDecoder() {
}

bool PreflateMetaDecoder::beginMetaBlock(PreflatePredictionDecoder &decoder, PreflateParameters &params, const size_t index) {
  if( index>=blockList.size() ) {
    return false;
  }
  const auto &mb = blockList[index];
  if( mb.modelId>=modelList.size() ) {
    return false;
  }
  const auto &model = modelList[mb.modelId];
  params = model.params;
  decoder.start(model.model, model.params, reconData, mb.reconStartOfs, mb.reconSize);
  return true;
}

bool PreflateMetaDecoder::endMetaBlock(PreflatePredictionDecoder &decoder) {
  decoder.end();
  return true;
}

void PreflateMetaDecoder::finish() {
}

uint PreflateStatisticsCounter::BlockPrediction::checkDefaultModels() const {
  uint cnt = 0;
  cnt += sumArray(blockType)==blockType[PreflateTokenBlock::DYNAMIC_HUFF];
  cnt += sumArray(EOBMisprediction)==EOBMisprediction[0];
  cnt += sumArray(nonZeroPadding)==nonZeroPadding[0];
  return cnt;
}

uint PreflateStatisticsCounter::TreeCodePrediction::checkDefaultModels() const {
  uint cnt = 0;
  cnt += sumArray(TCCountMisprediction)==TCCountMisprediction[0];
  cnt += sumArray(TCBitlengthCorrection)==TCBitlengthCorrection[3];
  cnt += sumArray(LCountMisprediction)==LCountMisprediction[0];
  cnt += sumArray(DCountMisprediction)==DCountMisprediction[0];
  for( uint i = 0; i<4; ++i ) {
    cnt += sumArray(LDTypeMisprediction[i])==LDTypeMisprediction[i][0];
  }
  cnt += sumArray(LDTypeReplacement)==0;
  cnt += sumArray(LDRepeatCountCorrection)==LDRepeatCountCorrection[1];
  cnt += sumArray(LDBitlengthCorrection)==LDBitlengthCorrection[4];
  return cnt;
}

uint PreflateStatisticsCounter::TokenPrediction::checkDefaultModels() const {
  uint cnt = 0;
  cnt += sumArray(LITMisprediction)==LITMisprediction[0];
  cnt += sumArray(REFMisprediction)==REFMisprediction[0];
  cnt += sumArray(LENCorrection)==LENCorrection[6];
  cnt += sumArray(DISTAfterLenCorrection)==DISTAfterLenCorrection[0];
  cnt += sumArray(DISTOnlyCorrection)==DISTOnlyCorrection[0];
  cnt += sumArray(LEN258IrregularEncoding)==LEN258IrregularEncoding[0];
  return cnt;
}

bool isEqual(const PreflateTokenBlock &b1, const PreflateTokenBlock &b2) {
  if( b1.type!=b2.type ) {
    return false;
  }
  if( b1.type!=PreflateTokenBlock::STORED ) {
    if( b1.type==PreflateTokenBlock::DYNAMIC_HUFF ) {
      if( b1.ncode!=b2.ncode||b1.nlen!=b2.nlen||b1.ndist!=b2.ndist ) {
        return false;
      }
      if( b1.treecodes!=b2.treecodes ) {
        return false;
      }
    }
    if( b1.tokens.size()!=b2.tokens.size() ) {
      return false;
    }
    for( uint i = 0, n = b1.tokens.size(); i<n; ++i ) {
      if( b1.tokens[i].len!=b2.tokens[i].len||b1.tokens[i].dist!=b2.tokens[i].dist ) {
        return false;
      }
    }
  }
  return true;
}

PreflateTokenPredictor::PreflateTokenPredictor(const PreflateParameters &params_, const std::vector<byte> &dump, const size_t offset) : state(hash, seq, params_.config(), params_.windowBits, params_.memLevel), hash(dump, params_.memLevel), seq(dump), params(params_), predictionFailure(false), fast(params_.isFastCompressor()), prevLen(0), pendingToken(PreflateToken::NONE), emptyBlockAtEnd(false) {
  if( state.availableInputSize()>=2 ) {
    hash.updateRunningHash(state.inputCursor()[0]);
    hash.updateRunningHash(state.inputCursor()[1]);
    seq.updateSeq(2);
  }
  hash.updateHash(offset);
  seq.updateSeq(offset);
}

bool PreflateTokenPredictor::predictEOB() {
  return state.availableInputSize()==0||currentTokenCount==state.maxTokenCount;
}

void PreflateTokenPredictor::commitToken(const PreflateToken &t) {
  if( fast&&t.len>state.lazyMatchLength() ) {
    hash.skipHash(t.len);
  } else {
    hash.updateHash(t.len);
  }
  seq.updateSeq(t.len);
}

PreflateToken PreflateTokenPredictor::predictToken() {
  if( state.currentInputPos()==0||state.availableInputSize()<PreflateConstants::MIN_MATCH ) {
    return PreflateToken(PreflateToken::LITERAL);
  }
  PreflateToken match(PreflateToken::NONE);
  uint hash = state.calculateHash();
  if( pendingToken.len>1 ) {
    match = pendingToken;
  } else {
    uint head = state.getCurrentHashHead(hash);
    if( !fast&&seq.valid(state.currentInputPos()) ) {
      match = state.seqMatch(state.currentInputPos(), head, prevLen, params.veryFarMatchesDetected, params.matchesToStartDetected, params.zlibCompatible ? 0 : (1<<params.log2OfMaxChainDepthM1));
    } else {
      match = state.match(head, prevLen, 0, params.veryFarMatchesDetected, params.matchesToStartDetected, params.zlibCompatible ? 0 : (1<<params.log2OfMaxChainDepthM1));
    }
  }
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  if( match.len<PreflateConstants::MIN_MATCH ) {
    return PreflateToken(PreflateToken::LITERAL);
  }
  if( fast ) {
    return match;
  }
  if( match.len==3&&match.dist>4096 ) {
    return PreflateToken(PreflateToken::LITERAL);
  }
  if( match.len<state.lazyMatchLength()&&state.availableInputSize()>=(uint)match.len+2 ) {
    PreflateToken matchNext(PreflateToken::NONE);
    uint hashNext = state.calculateHashNext();
    uint headNext = state.getCurrentHashHead(hashNext);
    if( !fast&&seq.valid(state.currentInputPos()+1) ) {
      matchNext = state.seqMatch(state.currentInputPos()+1, headNext, match.len, params.veryFarMatchesDetected, params.matchesToStartDetected, params.zlibCompatible ? 0 : (2<<params.log2OfMaxChainDepthM1));
    } else {
      matchNext = state.match(headNext, match.len, 1, params.veryFarMatchesDetected, params.matchesToStartDetected, params.zlibCompatible ? 0 : (2<<params.log2OfMaxChainDepthM1));
      if( ((hashNext^hash)&this->hash.hashMask)==0 ) {
        uint maxSize = std::min(state.availableInputSize()-1, (uint)PreflateConstants::MAX_MATCH);
        uint rle = 0;
        const byte* c = state.inputCursor();
        byte b = c[0];
        while( rle<maxSize&&c[1+rle]==b ) {
          ++rle;
        }
        if( rle>match.len&&rle>=matchNext.len ) {
          matchNext.len = rle;
          matchNext.dist = 1;
        }
      }
    }
    if( matchNext.len>match.len ) {
      prevLen = match.len;
      pendingToken = matchNext;
      if( !params.zlibCompatible ) {
        prevLen = 0;
        pendingToken = PreflateToken(PreflateToken::NONE);
      }
      return PreflateToken(PreflateToken::LITERAL);
    }
  }
  return match;
}

bool PreflateTokenPredictor::repredictReference(PreflateToken &token) {
  if( state.currentInputPos()==0||state.availableInputSize()<PreflateConstants::MIN_MATCH ) {
    return false;
  }
  uint hash = state.calculateHash();
  uint head = state.getCurrentHashHead(hash);
  PreflateToken match = state.match(head, 0, 0, params.veryFarMatchesDetected, params.matchesToStartDetected, (2<<params.log2OfMaxChainDepthM1));
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  if( match.len<PreflateConstants::MIN_MATCH ) {
    return false;
  }
  token = match;
  return true;
}

PreflateRematchInfo PreflateTokenPredictor::repredictMatch(const PreflateToken &token) {
  uint hash = state.calculateHash();
  uint head = state.getCurrentHashHead(hash);
  PreflateRematchInfo i = state.rematchInfo(head, token);
  return i;
}

uint PreflateTokenPredictor::recalculateDistance(const PreflateToken &token, const uint hops) {
  return state.hopMatch(token, hops);
}

void PreflateTokenPredictor::analyzeBlock(const uint blockno, const PreflateTokenBlock &block) {
  currentTokenCount = 0;
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  if( blockno!=analysisResults.size()||predictionFailure ) {
    return;
  }
  analysisResults.push_back(BlockAnalysisResult());
  BlockAnalysisResult &analysis = analysisResults[blockno];
  analysis.type = block.type;
  analysis.tokenCount = block.tokens.size();
  analysis.tokenInfo.resize(analysis.tokenCount);
  analysis.blockSizePredicted = true;
  analysis.inputEOF = false;
  if( analysis.type==PreflateTokenBlock::STORED ) {
    analysis.tokenCount = block.uncompressedLen;
    hash.updateHash(block.uncompressedLen);
    seq.updateSeq(block.uncompressedLen);
    analysis.inputEOF = state.availableInputSize()==0;
    analysis.paddingBits = block.paddingBits;
    analysis.paddingCounts = block.paddingBitCount;
    return;
  }
  for( uint i = 0, n = block.tokens.size(); i<n; ++i ) {
    PreflateToken targetToken = block.tokens[i];
    if( predictEOB() ) {
      analysis.blockSizePredicted = false;
    }
    PreflateToken predictedToken = predictToken();
    if( targetToken.len==1 ) {
      if( predictedToken.len>1 ) {
        analysis.tokenInfo[currentTokenCount] = 2;
      } else {
        analysis.tokenInfo[currentTokenCount] = 0;
      }
    } else {
      if( predictedToken.len==1 ) {
        analysis.tokenInfo[currentTokenCount] = 3;
        if( !repredictReference(predictedToken) ) {
          predictionFailure = true;
          return;
        }
      } else {
        analysis.tokenInfo[currentTokenCount] = 1;
      }
      PreflateRematchInfo rematch;
      if( predictedToken.len!=targetToken.len ) {
        analysis.tokenInfo[currentTokenCount] += 4;
        analysis.correctives.push_back(predictedToken.len);
        analysis.correctives.push_back(targetToken.len-predictedToken.len);
        rematch = repredictMatch(targetToken);
        if( rematch.requestedMatchDepth>=0xffff ) {
          predictionFailure = true;
          return;
        }
        analysis.correctives.push_back(rematch.condensedHops-1);
      } else {
        if( targetToken.dist!=predictedToken.dist ) {
          analysis.tokenInfo[currentTokenCount] += 8;
          rematch = repredictMatch(targetToken);
          if( rematch.requestedMatchDepth>=0xffff ) {
            predictionFailure = true;
            return;
          }
          analysis.correctives.push_back(rematch.condensedHops-1);
        }
      }
    }
    if( targetToken.len==258 ) {
      analysis.tokenInfo[currentTokenCount] += 16;
      if( targetToken.irregular258 ) {
        analysis.tokenInfo[currentTokenCount] += 32;
      }
    }
    commitToken(targetToken);
    ++currentTokenCount;
  }
  if( !predictEOB() ) {
    analysis.blockSizePredicted = false;
  }
  analysis.inputEOF = state.availableInputSize()==0;
}

void PreflateTokenPredictor::encodeBlock(PreflatePredictionEncoder* codec, const uint blockno) {
  BlockAnalysisResult &analysis = analysisResults[blockno];
  codec->encodeBlockType(analysis.type);
  if( analysis.type==PreflateTokenBlock::STORED ) {
    codec->encodeValue(analysis.tokenCount, 16);
    bool pad = analysis.paddingBits!=0;
    codec->encodeNonZeroPadding(pad);
    if( pad ) {
      uint bitsToSave = bitLength(analysis.paddingBits);
      codec->encodeValue(bitsToSave, 3);
      if( bitsToSave>1 ) {
        codec->encodeValue(analysis.paddingBits&((1<<(bitsToSave-1))-1), bitsToSave-1);
      }
    }
    return;
  }
  codec->encodeEOBMisprediction(!analysis.blockSizePredicted);
  if( !analysis.blockSizePredicted ) {
    uint blocksizeBits = bitLength(analysis.tokenCount);
    codec->encodeValue(blocksizeBits, 5);
    if( blocksizeBits>=2 ) {
      codec->encodeValue(analysis.tokenCount, blocksizeBits);
    }
  }
  uint correctivePos = 0;
  for( uint i = 0, n = analysis.tokenCount; i<n; ++i ) {
    byte info = analysis.tokenInfo[i];
    switch( info&3 ) {
    case 0:
      codec->encodeLiteralPredictionWrong(false);
      continue;
    case 2:
      codec->encodeReferencePredictionWrong(true);
      continue;
    case 1:
      codec->encodeReferencePredictionWrong(false);
      break;
    case 3:
      codec->encodeLiteralPredictionWrong(true);
      break;
    }
    if( info&4 ) {
      int pred = analysis.correctives[correctivePos++];
      int diff = analysis.correctives[correctivePos++];
      int hops = analysis.correctives[correctivePos++];
      codec->encodeLenCorrection(pred, pred+diff);
      codec->encodeDistAfterLenCorrection(hops);
    } else {
      codec->encodeLenCorrection(3, 3);
      if( info&8 ) {
        int hops = analysis.correctives[correctivePos++];
        codec->encodeDistOnlyCorrection(hops);
      } else {
        codec->encodeDistOnlyCorrection(0);
      }
    }
    if( info&16 ) {
      codec->encodeIrregularLen258((info&32)!=0);
    }
  }
}

void PreflateTokenPredictor::encodeEOF(PreflatePredictionEncoder* codec, const uint blockno, const bool lastBlock) {
  BlockAnalysisResult &analysis = analysisResults[blockno];
  if( analysis.inputEOF ) {
    codec->encodeValue(!lastBlock, 1);
  } else {
    if( lastBlock ) {
      predictionFailure = true;
    }
  }
}

void PreflateTokenPredictor::updateCounters(PreflateStatisticsCounter* model, const uint blockno) {
  BlockAnalysisResult &analysis = analysisResults[blockno];
  model->block.incBlockType(analysis.type);
  if( analysis.type==PreflateTokenBlock::STORED ) {
    model->block.incNonZeroPadding(analysis.paddingBits!=0);
    return;
  }
  model->block.incEOBPredictionWrong(!analysis.blockSizePredicted);
  uint correctivePos = 0;
  for( uint i = 0, n = analysis.tokenCount; i<n; ++i ) {
    byte info = analysis.tokenInfo[i];
    switch( info&3 ) {
    case 0:
      model->token.incLiteralPredictionWrong(false);
      continue;
    case 2:
      model->token.incReferencePredictionWrong(true);
      continue;
    case 1:
      model->token.incReferencePredictionWrong(false);
      break;
    case 3:
      model->token.incLiteralPredictionWrong(true);
      break;
    }
    if( info&4 ) {
      correctivePos++;
      int diff = analysis.correctives[correctivePos++];
      int hops = analysis.correctives[correctivePos++];
      model->token.incLengthDiffToPrediction(diff);
      model->token.incDistanceDiffToPredictionAfterIncorrectLengthPrediction(hops);
    } else {
      model->token.incLengthDiffToPrediction(0);
      if( info&8 ) {
        int hops = analysis.correctives[correctivePos++];
        model->token.incDistanceDiffToPredictionAfterCorrectLengthPrediction(hops);
      } else {
        model->token.incDistanceDiffToPredictionAfterCorrectLengthPrediction(0);
      }
    }
    if( info&16 ) {
      model->token.incIrregularLength258Encoding((info&32)!=0);
    }
  }
}

PreflateTokenBlock PreflateTokenPredictor::decodeBlock(PreflatePredictionDecoder* codec) {
  PreflateTokenBlock block;
  currentTokenCount = 0;
  prevLen = 0;
  pendingToken = PreflateToken(PreflateToken::NONE);
  uint blocksize = 0;
  bool checkEOB = true;
  uint bt = codec->decodeBlockType();
  switch( bt ) {
  case PreflateTokenBlock::STORED:
    block.type = PreflateTokenBlock::STORED;
    block.uncompressedLen = codec->decodeValue(16);
    block.paddingBits = 0;
    block.paddingBitCount = 0;
    if( codec->decodeNonZeroPadding() ) {
      block.paddingBitCount = codec->decodeValue(3);
      if( block.paddingBitCount>0 ) {
        block.paddingBits = (1<<(block.paddingBitCount-1))+codec->decodeValue(block.paddingBitCount-1);
      } else {
        block.paddingBits = 0;
      }
    }
    hash.updateHash(block.uncompressedLen);
    seq.updateSeq(block.uncompressedLen);
    return block;
  case PreflateTokenBlock::STATIC_HUFF:
    block.type = PreflateTokenBlock::STATIC_HUFF;
    break;
  case PreflateTokenBlock::DYNAMIC_HUFF:
    block.type = PreflateTokenBlock::DYNAMIC_HUFF;
    break;
  }
  if( codec->decodeEOBMisprediction() ) {
    uint blocksizeBits = codec->decodeValue(5);
    if( blocksizeBits>=2 ) {
      blocksize = codec->decodeValue(blocksizeBits);
    } else {
      blocksize = blocksizeBits;
    }
    block.tokens.reserve(blocksize);
    checkEOB = false;
  } else {
    block.tokens.reserve(1<<(6+params.memLevel));
  }
  while( (checkEOB&&!predictEOB())||(!checkEOB&&currentTokenCount<blocksize) ) {
    PreflateToken predictedToken = predictToken();
    if( predictedToken.len==1 ) {
      uint notok = codec->decodeLiteralPredictionWrong();
      if( !notok ) {
        block.tokens.push_back(predictedToken);
        commitToken(predictedToken);
        ++currentTokenCount;
        continue;
      }
      if( !repredictReference(predictedToken) ) {
        predictionFailure = true;
        return PreflateTokenBlock();
      }
    } else {
      uint notok = codec->decodeReferencePredictionWrong();
      if( notok ) {
        predictedToken.len = 1;
        predictedToken.dist = 0;
        block.tokens.push_back(predictedToken);
        commitToken(predictedToken);
        ++currentTokenCount;
        continue;
      }
    }
    uint newLen = codec->decodeLenCorrection(predictedToken.len);
    if( newLen!=predictedToken.len ) {
      uint hops = codec->decodeDistAfterLenCorrection();
      predictedToken.len = newLen;
      predictedToken.dist = state.firstMatch(predictedToken.len);
      if( hops ) {
        predictedToken.dist = recalculateDistance(predictedToken, hops);
      }
      if( predictedToken.len<3||predictedToken.len>258||predictedToken.dist==0 ) {
        predictionFailure = true;
        return PreflateTokenBlock();
      }
    } else {
      uint hops = codec->decodeDistOnlyCorrection();
      if( hops ) {
        predictedToken.dist = recalculateDistance(predictedToken, hops);
        if( predictedToken.dist==0 ) {
          predictionFailure = true;
          return PreflateTokenBlock();
        }
      }
    }
    if( predictedToken.len==258 ) {
      predictedToken.irregular258 = codec->decodeIrregularLen258();
    }
    block.tokens.push_back(predictedToken);
    commitToken(predictedToken);
    ++currentTokenCount;
  }
  return block;
}

bool PreflateTokenPredictor::decodeEOF(PreflatePredictionDecoder* codec) {
  if( state.availableInputSize()==0 ) {
    return codec->decodeValue(1)==0;
  }
  return false;
}

bool PreflateTokenPredictor::inputEOF() {
  return state.availableInputSize()==0;
}

PreflateTreePredictor::PreflateTreePredictor(const std::vector<byte> &dump, const size_t off) : input(dump), predictionFailure(false) {
  input.advance(off);
}

struct FreqIdxPair {
  uint freq;
  uint idx;
};
struct TreeNode {
  uint parent;
  uint idx;
};
bool pq_smaller(const FreqIdxPair &p1, const FreqIdxPair &p2, const byte* nodeDepth) {
  return p1.freq<p2.freq||(p1.freq==p2.freq&&nodeDepth[p1.idx]<=nodeDepth[p2.idx]);
}

void pq_downheap(FreqIdxPair* ptr, const uint index, const uint len, const byte* depth) {
  uint k = index;
  FreqIdxPair v = ptr[k];
  uint j = k*2+1;
  while( j<len ) {
    if( j+1<len&&pq_smaller(ptr[j+1], ptr[j], depth) ) {
      j++;
    }
    if( pq_smaller(v, ptr[j], depth) )
      break;
    ptr[k] = ptr[j];
    k = j;
    j = k*2+1;
  }
  ptr[k] = v;
}

void pq_makeheap(FreqIdxPair* ptr, const uint len, const byte* depth) {
  for( uint n = (len-1)/2+1; n>0; n-- ) {
    pq_downheap(ptr, n-1, len, depth);
  }
}

FreqIdxPair pq_remove(FreqIdxPair* ptr, uint &len, const byte* depth) {
  FreqIdxPair result = ptr[0];
  ptr[0] = ptr[--len];
  pq_downheap(ptr, 0, len, depth);
  return result;
}

uint PreflateTreePredictor::calcBitLengths(byte* symBitLen, const uint* symFreq, const uint symCount, const uint maxBits, const uint minMaxCode) {
  FreqIdxPair toSort[PreflateConstants::LITLEN_CODE_COUNT];
  TreeNode nodes[PreflateConstants::LITLEN_CODE_COUNT* 2+1];
  byte nodeBitLen[PreflateConstants::LITLEN_CODE_COUNT* 2+1];
  byte nodeDepth[PreflateConstants::LITLEN_CODE_COUNT* 2+1];
  memset(nodeBitLen, 0, sizeof(nodeBitLen));
  memset(nodeDepth, 0, sizeof(nodeDepth));
  uint maxCode = 0, len = 0, nodeCount = 0, nodeId = symCount;
  for( uint i = 0; i<symCount; ++i ) {
    if( symFreq[i] ) {
      toSort[len++] = FreqIdxPair{symFreq[i], maxCode = i};
    }
  }
  if( len<2 ) {
    memset(symBitLen, 0, symCount);
    symBitLen[maxCode] = 1;
    symBitLen[maxCode<2 ? ++maxCode : 0] = 1;
    return std::max(minMaxCode, maxCode+1);
  }
  pq_makeheap(toSort, len, nodeDepth);
  while( len>1 ) {
    FreqIdxPair least1 = pq_remove(toSort, len, nodeDepth);
    FreqIdxPair least2 = toSort[0];
    toSort[0] = FreqIdxPair{least1.freq+least2.freq, nodeId};
    nodes[nodeCount++] = TreeNode{nodeId, least1.idx};
    nodes[nodeCount++] = TreeNode{nodeId, least2.idx};
    nodeDepth[nodeId] = std::max(nodeDepth[least1.idx], nodeDepth[least2.idx])+1;
    pq_downheap(toSort, 0, len, nodeDepth);
    nodeId++;
  }
  uint overflow = 0;
  uint bl_count[16];
  memset(bl_count, 0, sizeof(bl_count));
  uint orgNodeCount = nodeCount;
  while( nodeCount-->0 ) {
    byte newLen = nodeBitLen[nodes[nodeCount].parent]+1;
    if( newLen>maxBits ) {
      newLen = maxBits;
      ++overflow;
    }
    uint idx = nodes[nodeCount].idx;
    nodeBitLen[idx] = newLen;
    if( idx<symCount ) {
      bl_count[newLen]++;
    }
  }
  if( overflow ) {
    uint bits;
    do {
      for( bits = maxBits-1; bl_count[bits]==0; bits-- ) {
      }
      bl_count[bits]--;
      bl_count[bits+1] += 2;
      bl_count[maxBits]--;
      overflow -= 2;
    } while( overflow>0 );
    for( bits = maxBits, nodeCount = orgNodeCount; nodeCount>0; ) {
      --nodeCount;
      uint idx = nodes[nodeCount].idx;
      if( idx>=symCount ) {
        continue;
      }
      while( bl_count[bits]==0 ) {
        bits--;
      }
      nodeBitLen[idx] = bits;
      bl_count[bits]--;
    }
  }
  memcpy(symBitLen, nodeBitLen, symCount);
  return std::max(minMaxCode, maxCode+1);
}

TreeCodeType PreflateTreePredictor::predictCodeType(const byte* symBitLen, const uint symCount, const bool first) {
  byte code = symBitLen[0];
  if( code==0 ) {
    byte curlen = 1;
    byte maxCurLen = std::min(symCount, 11u);
    while( curlen<maxCurLen&&symBitLen[curlen]==0 ) {
      ++curlen;
    }
    if( curlen>=11 ) {
      return TCT_REPZL;
    }
    if( curlen>=3 ) {
      return TCT_REPZS;
    }
    return TCT_BITS;
  }
  if( !first&&code==symBitLen[-1] ) {
    byte curlen = 1;
    byte maxCurLen = std::min(symCount, 3u);
    while( curlen<maxCurLen&&symBitLen[curlen]==code ) {
      ++curlen;
    }
    if( curlen>=3 ) {
      return TCT_REP;
    }
  }
  return TCT_BITS;
}

byte PreflateTreePredictor::predictCodeData(const byte* symBitLen, const TreeCodeType type, const uint symCount, const bool first) {
  byte code = symBitLen[0];
  switch( type ) {
  default:
  case TCT_BITS:
    return code;
  case TCT_REP: {
    byte curlen = 3;
    byte maxCurLen = std::min(symCount, 6u);
    while( curlen<maxCurLen&&symBitLen[curlen]==code ) {
      ++curlen;
    }
    return curlen;
  }
  case TCT_REPZS:
  case TCT_REPZL: {
    byte curlen = type==TCT_REPZS ? 3 : 11;
    byte maxCurLen = std::min(symCount, type==TCT_REPZS ? 10u : 138u);
    while( curlen<maxCurLen&&symBitLen[curlen]==0 ) {
      ++curlen;
    }
    return curlen;
  }
  }
}

void PreflateTreePredictor::predictLDTrees(BlockAnalysisResult &analysis, uint* frequencies, const byte* symBitLen, const uint symLCount, const uint symDCount, const byte* targetCodes, const uint targetCodeSize) {
  memset(frequencies, 0, sizeof(uint)*PreflateConstants::CODETREE_CODE_COUNT);
  const byte* ptr = symBitLen;
  const byte* code = targetCodes;
  uint codeSize = targetCodeSize;
  uint count1 = symLCount;
  uint count2 = symDCount;
  bool first = true;
  while( codeSize>0 ) {
    TreeCodeType targetTreeCodeType;
    switch( code[0] ) {
    case 16:
      targetTreeCodeType = TCT_REP;
      break;
    case 17:
      targetTreeCodeType = TCT_REPZS;
      break;
    case 18:
      targetTreeCodeType = TCT_REPZL;
      break;
    default:
      targetTreeCodeType = TCT_BITS;
      break;
    }
    if( codeSize<2&&targetTreeCodeType!=TCT_BITS ) {
      predictionFailure = true;
      return;
    }
    TreeCodeType predictedTreeCodeType = predictCodeType(ptr, count1, first);
    byte info = predictedTreeCodeType|((targetTreeCodeType!=predictedTreeCodeType)<<2);
    if( targetTreeCodeType!=predictedTreeCodeType ) {
      analysis.correctives.push_back(targetTreeCodeType);
    }
    byte targetTreeCodeData = code[targetTreeCodeType!=TCT_BITS];
    uint l = 1+(targetTreeCodeType!=TCT_BITS);
    code += l;
    codeSize -= l;
    byte predictedTreeCodeData = predictCodeData(ptr, targetTreeCodeType, count1, first);
    first = false;
    if( targetTreeCodeType!=TCT_BITS ) {
      analysis.correctives.push_back(predictedTreeCodeData);
      if( targetTreeCodeData!=predictedTreeCodeData ) {
        info |= 8;
        analysis.correctives.push_back(targetTreeCodeData);
      }
    } else {
      analysis.correctives.push_back(predictedTreeCodeData);
      analysis.correctives.push_back(targetTreeCodeData-predictedTreeCodeData);
    }
    if( targetTreeCodeType!=TCT_BITS ) {
      frequencies[targetTreeCodeType+15]++;
      l = targetTreeCodeData;
    } else {
      frequencies[targetTreeCodeData]++;
      l = 1;
    }
    ptr += l;
    if( count1>l ) {
      count1 -= l;
    } else {
      count1 += count2;
      count2 = 0;
      first = true;
      if( count1>=l ) {
        count1 -= l;
      } else {
        predictionFailure = true;
        return;
      }
    }
    analysis.tokenInfo.push_back(info);
  }
  analysis.tokenInfo.push_back(0xff);
  if( count1+count2!=0 ) {
    predictionFailure = true;
  }
}

void PreflateTreePredictor::collectTokenStatistics(uint Lcodes[], uint Dcodes[], uint &Lcount, uint &Dcount, const PreflateTokenBlock &block) {
  memset(Lcodes, 0, sizeof(uint)*PreflateConstants::LITLEN_CODE_COUNT);
  memset(Dcodes, 0, sizeof(uint)*PreflateConstants::DIST_CODE_COUNT);
  Lcount = 0;
  Dcount = 0;
  for( uint i = 0, n = block.tokens.size(); i<n; ++i ) {
    PreflateToken targetToken = block.tokens[i];
    if( targetToken.len==1 ) {
      Lcodes[input.curChar()]++;
      Lcount++;
      input.advance(1);
    } else {
      Lcodes[PreflateConstants::NONLEN_CODE_COUNT+PreflateConstants::LCode(targetToken.len)]++;
      Lcount++;
      Dcodes[PreflateConstants::DCode(targetToken.dist)]++;
      Dcount++;
      input.advance(targetToken.len);
    }
  }
  Lcodes[256] = 1;
}

uint PreflateTreePredictor::buildLBitlenghs(byte bitLengths[], uint Lcodes[]) {
  return calcBitLengths(bitLengths, Lcodes, PreflateConstants::LITLEN_CODE_COUNT, 15, PreflateConstants::NONLEN_CODE_COUNT);
}

uint PreflateTreePredictor::buildDBitlenghs(byte bitLengths[], uint Dcodes[]) {
  return calcBitLengths(bitLengths, Dcodes, PreflateConstants::DIST_CODE_COUNT, 15, 0);
}

uint PreflateTreePredictor::buildTCBitlengths(byte (&simpleCodeTree)[PreflateConstants::CODETREE_CODE_COUNT], uint (&BLfreqs)[PreflateConstants::CODETREE_CODE_COUNT]) {
  memset(simpleCodeTree, 0, sizeof(simpleCodeTree));
  calcBitLengths(simpleCodeTree, BLfreqs, PreflateConstants::CODETREE_CODE_COUNT, 7, 0);
  uint predictedCTreeSize = PreflateConstants::CODETREE_CODE_COUNT;
  while( predictedCTreeSize>4&&simpleCodeTree[PreflateConstants::treeCodeOrderTable[predictedCTreeSize-1]]==0 ) {
    --predictedCTreeSize;
  }
  return predictedCTreeSize;
}

void PreflateTreePredictor::analyzeBlock(const uint blockno, const PreflateTokenBlock &block) {
  if( blockno!=analysisResults.size()||predictionFailure ) {
    return;
  }
  analysisResults.push_back(BlockAnalysisResult());
  BlockAnalysisResult &analysis = analysisResults[blockno];
  analysis.blockType = block.type;
  if( analysis.blockType!=PreflateTokenBlock::DYNAMIC_HUFF ) {
    return;
  }
  uint Lcodes[PreflateConstants::LITLEN_CODE_COUNT], Dcodes[PreflateConstants::DIST_CODE_COUNT];
  uint Lcount = 0, Dcount = 0;
  collectTokenStatistics(Lcodes, Dcodes, Lcount, Dcount, block);
  byte bitLengths[PreflateConstants::LITLENDIST_CODE_COUNT];
  memset(bitLengths, 0, sizeof(bitLengths));
  uint predictedLTreeSize = buildLBitlenghs(bitLengths, Lcodes);
  analysis.tokenInfo.push_back(predictedLTreeSize!=block.nlen);
  if( predictedLTreeSize!=block.nlen ) {
    analysis.correctives.push_back(block.nlen);
  }
  predictedLTreeSize = block.nlen;
  uint predictedDTreeSize = buildDBitlenghs(bitLengths+predictedLTreeSize, Dcodes);
  analysis.tokenInfo.push_back(predictedDTreeSize!=block.ndist);
  if( predictedDTreeSize!=block.ndist ) {
    analysis.correctives.push_back(block.ndist);
  }
  predictedDTreeSize = block.ndist;
  uint BLfreqs[PreflateConstants::CODETREE_CODE_COUNT];
  const byte* targetCodes = &block.treecodes[0];
  uint targetCodeSize = block.treecodes.size();
  predictLDTrees(analysis, BLfreqs, bitLengths, predictedLTreeSize, predictedDTreeSize, targetCodes+block.ncode, targetCodeSize-block.ncode);
  byte simpleCodeTree[PreflateConstants::CODETREE_CODE_COUNT];
  uint predictedCTreeSize = buildTCBitlengths(simpleCodeTree, BLfreqs);
  analysis.tokenInfo.push_back(block.ncode);
  analysis.tokenInfo.push_back(predictedCTreeSize!=block.ncode);
  predictedCTreeSize = block.ncode;
  for( uint i = 0; i<predictedCTreeSize; ++i ) {
    uint predictedBL = simpleCodeTree[PreflateConstants::treeCodeOrderTable[i]];
    analysis.correctives.push_back(predictedBL);
    analysis.correctives.push_back(targetCodes[i]-predictedBL);
  }
}

void PreflateTreePredictor::encodeBlock(PreflatePredictionEncoder* codec, const uint blockno) {
  BlockAnalysisResult &analysis = analysisResults[blockno];
  if( analysis.blockType!=PreflateTokenBlock::DYNAMIC_HUFF ) {
    return;
  }
  uint infoPos = 0, correctivePos = 0;
  byte info = analysis.tokenInfo[infoPos++];
  codec->encodeLiteralCountMisprediction(info);
  if( info ) {
    codec->encodeValue(analysis.correctives[correctivePos++]-PreflateConstants::NONLEN_CODE_COUNT, 5);
  }
  info = analysis.tokenInfo[infoPos++];
  codec->encodeDistanceCountMisprediction(info);
  if( info ) {
    codec->encodeValue(analysis.correctives[correctivePos++], 5);
  }
  while( (info = analysis.tokenInfo[infoPos++])!=0xff ) {
    uint type = (info&3);
    if( info&4 ) {
      uint newType = analysis.correctives[correctivePos++];
      codec->encodeLDTypeCorrection(type, newType);
      type = newType;
    } else {
      codec->encodeLDTypeCorrection(type, type);
    }
    if( type!=TCT_BITS ) {
      uint predRepeat = analysis.correctives[correctivePos++];
      if( info&8 ) {
        uint newRepeat = analysis.correctives[correctivePos++];
        codec->encodeRepeatCountCorrection(predRepeat, newRepeat, type);
      } else {
        codec->encodeRepeatCountCorrection(predRepeat, predRepeat, type);
      }
    } else {
      uint bl_pred = analysis.correctives[correctivePos++];
      int bl_diff = analysis.correctives[correctivePos++];
      codec->encodeLDBitLengthCorrection(bl_pred, bl_pred+bl_diff);
    }
  }
  uint blcount = analysis.tokenInfo[infoPos++];
  info = analysis.tokenInfo[infoPos++];
  codec->encodeTreeCodeCountMisprediction(info);
  if( info ) {
    codec->encodeValue(blcount-4, 4);
  }
  for( uint i = 0; i<blcount; ++i ) {
    int bl_pred = analysis.correctives[correctivePos++];
    int bl_diff = analysis.correctives[correctivePos++];
    codec->encodeTreeCodeBitLengthCorrection(bl_pred, bl_pred+bl_diff);
  }
}

void PreflateTreePredictor::updateCounters(PreflateStatisticsCounter* model, const uint blockno) {
  BlockAnalysisResult &analysis = analysisResults[blockno];
  if( analysis.blockType!=PreflateTokenBlock::DYNAMIC_HUFF ) {
    return;
  }
  uint infoPos = 0, correctivePos = 0;
  byte info = analysis.tokenInfo[infoPos++];
  model->treecode.incLiteralCountPredictionWrong(info);
  if( info ) {
    correctivePos++;
  }
  info = analysis.tokenInfo[infoPos++];
  model->treecode.incDistanceCountPredictionWrong(info);
  if( info ) {
    correctivePos++;
  }
  while( (info = analysis.tokenInfo[infoPos++])!=0xff ) {
    uint type = (info&3);
    model->treecode.incLDCodeTypePredictionWrong(type, (info&4)!=0);
    if( info&4 ) {
      uint newType = analysis.correctives[correctivePos++];
      model->treecode.incLDCodeTypeReplacement(newType);
      type = newType;
    }
    if( type!=TCT_BITS ) {
      uint predRepeat = analysis.correctives[correctivePos++];
      if( info&8 ) {
        uint newRepeat = analysis.correctives[correctivePos++];
        model->treecode.incLDCodeRepeatDiffToPrediction(newRepeat-predRepeat);
      } else {
        model->treecode.incLDCodeRepeatDiffToPrediction(0);
      }
    } else {
      correctivePos++;
      int bl_diff = analysis.correctives[correctivePos++];
      model->treecode.incLDCodeLengthDiffToPrediction(bl_diff);
    }
  }
  uint blcount = analysis.tokenInfo[infoPos++];
  info = analysis.tokenInfo[infoPos++];
  model->treecode.incTreeCodeCountPredictionWrong(info);
  for( uint i = 0; i<blcount; ++i ) {
    correctivePos++;
    int bl_diff = analysis.correctives[correctivePos++];
    model->treecode.incTreeCodeLengthDiffToPrediction(bl_diff);
  }
}

uint PreflateTreePredictor::reconstructLDTrees(PreflatePredictionDecoder* codec, uint* frequencies, byte* targetCodes, const uint targetCodeSize, const byte* symBitLen, const uint symLCount, const uint symDCount) {
  memset(frequencies, 0, sizeof(uint)*PreflateConstants::CODETREE_CODE_COUNT);
  const byte* ptr = symBitLen;
  uint osize = 0;
  uint count1 = symLCount;
  uint count2 = symDCount;
  bool first = true;
  while( count1+count2>0 ) {
    TreeCodeType predictedTreeCodeType = predictCodeType(ptr, count1, first);
    uint newType = codec->decodeLDTypeCorrection(predictedTreeCodeType);
    switch( newType ) {
    case TCT_BITS:
      predictedTreeCodeType = TCT_BITS;
      break;
    case TCT_REP:
      predictedTreeCodeType = TCT_REP;
      break;
    case TCT_REPZS:
      predictedTreeCodeType = TCT_REPZS;
      break;
    case TCT_REPZL:
      predictedTreeCodeType = TCT_REPZL;
      break;
    }
    byte predictedTreeCodeData = predictCodeData(ptr, predictedTreeCodeType, count1, first);
    first = false;
    if( predictedTreeCodeType!=TCT_BITS ) {
      predictedTreeCodeData = codec->decodeRepeatCountCorrection(predictedTreeCodeData, predictedTreeCodeType);
    } else {
      predictedTreeCodeData = codec->decodeLDBitLengthCorrection(predictedTreeCodeData);
      ;
    }
    uint l;
    if( predictedTreeCodeType!=TCT_BITS ) {
      frequencies[predictedTreeCodeType+15]++;
      l = predictedTreeCodeData;
      if( osize+2>targetCodeSize ) {
        predictionFailure = true;
        break;
      }
      targetCodes[osize++] = predictedTreeCodeType+15;
      targetCodes[osize++] = predictedTreeCodeData;
    } else {
      frequencies[predictedTreeCodeData]++;
      l = 1;
      if( osize>=targetCodeSize ) {
        predictionFailure = true;
        break;
      }
      targetCodes[osize++] = predictedTreeCodeData;
    }
    ptr += l;
    if( count1>l ) {
      count1 -= l;
    } else {
      count1 += count2;
      count2 = 0;
      first = true;
      if( count1>=l ) {
        count1 -= l;
      } else {
        predictionFailure = true;
        break;
      }
    }
  }
  if( count1+count2!=0 ) {
    predictionFailure = true;
  }
  return predictionFailure ? 0 : osize;
}

bool PreflateTreePredictor::decodeBlock(PreflateTokenBlock &block, PreflatePredictionDecoder* codec) {
  if( block.type!=PreflateTokenBlock::DYNAMIC_HUFF ) {
    return true;
  }
  uint Lcodes[PreflateConstants::LITLEN_CODE_COUNT], Dcodes[PreflateConstants::DIST_CODE_COUNT];
  uint Lcount = 0, Dcount = 0;
  collectTokenStatistics(Lcodes, Dcodes, Lcount, Dcount, block);
  byte bitLengths[PreflateConstants::LITLENDIST_CODE_COUNT];
  memset(bitLengths, 0, sizeof(bitLengths));
  uint predictedLTreeSize = buildLBitlenghs(bitLengths, Lcodes);
  if( codec->decodeLiteralCountMisprediction() ) {
    predictedLTreeSize = codec->decodeValue(5)+PreflateConstants::NONLEN_CODE_COUNT;
  }
  block.nlen = predictedLTreeSize;
  uint predictedDTreeSize = buildDBitlenghs(bitLengths+predictedLTreeSize, Dcodes);
  if( codec->decodeDistanceCountMisprediction() ) {
    predictedDTreeSize = codec->decodeValue(5);
  }
  block.ndist = predictedDTreeSize;
  uint BLfreqs[PreflateConstants::CODETREE_CODE_COUNT];
  byte compressedLDtrees[PreflateConstants::LITLENDIST_CODE_COUNT];
  uint targetCodeSize = reconstructLDTrees(codec, BLfreqs, compressedLDtrees, PreflateConstants::LITLENDIST_CODE_COUNT, bitLengths, predictedLTreeSize, predictedDTreeSize);
  if( predictionFailure ) {
    return false;
  }
  byte simpleCodeTree[PreflateConstants::CODETREE_CODE_COUNT];
  uint predictedCTreeSize = buildTCBitlengths(simpleCodeTree, BLfreqs);
  if( codec->decodeTreeCodeCountMisprediction() ) {
    predictedCTreeSize = codec->decodeValue(4)+4;
  }
  block.ncode = predictedCTreeSize;
  byte shuffledCodeTree[PreflateConstants::CODETREE_CODE_COUNT];
  for( uint i = 0; i<predictedCTreeSize; ++i ) {
    uint predictedBL = simpleCodeTree[PreflateConstants::treeCodeOrderTable[i]];
    shuffledCodeTree[i] = codec->decodeTreeCodeBitLengthCorrection(predictedBL);
  }
  block.treecodes.reserve(predictedCTreeSize+targetCodeSize);
  block.treecodes.insert(block.treecodes.end(), shuffledCodeTree, shuffledCodeTree+predictedCTreeSize);
  block.treecodes.insert(block.treecodes.end(), compressedLDtrees, compressedLDtrees+targetCodeSize);
  return true;
}

const uint8_t ArithmeticCodecBase::_normCheckLUT[8] = {0x33, 0x77, 0xff, 0xff, 0x33, 0x77, 0xff, 0xff};
ArithmeticCodecBase::ArithmeticCodecBase() : _low(0), _high(0x7fffffff) {
}

ArithmeticEncoder::ArithmeticEncoder(BitOutputStream &bos) : _bos(bos), _e3cnt(0) {
}

void ArithmeticEncoder::_writeE3(const uint w) {
  while( _e3cnt>0 ) {
    uint32_t todo = std::min(_e3cnt, 16u);
    _bos.put(w, todo);
    _e3cnt -= todo;
  }
}

void ArithmeticEncoder::flush() {
  if( _low<0x20000000 ) {
    _bos.put(2, 2);
    _writeE3(~0u);
  } else {
    _bos.put(1, 1);
  }
  _low = 0;
  _high = 0x7fffffff;
}

void ArithmeticEncoder::_normalize() {
  uint32_t lh = ~_low&_high;
  if( (lh&0x40000000)==0 ) {
    uint w = (_low&0x40000000)!=0;
    _bos.put(w, 1);
    _writeE3(w-1);
    if( (lh&0x20000000)==0 ) {
      uint l = bitLeadingZeroes((lh<<2)+3);
      if( l<=16 ) {
        _bos.putReverse(_low>>(30-l), l);
      } else {
        _bos.putReverse(_low>>(30-16), 16);
        _bos.putReverse(_low>>(30-l), l-16);
      }
      _low = (_low<<(l+1))&0x7fffffff;
      _high = (((_high+1)<<(l+1))-1)&0x7fffffff;
    } else {
      _low = (_low<<1)&0x7fffffff;
      _high = ((_high<<1)+1)&0x7fffffff;
    }
  }
  lh = ~_low|_high;
  if( (lh&0x20000000)==0 ) {
    uint l = bitLeadingZeroes((lh<<2)+3);
    _e3cnt += l;
    _low = (_low<<l)&0x3fffffff;
    _high = ((((_high+1)<<l)-1)&0x3fffffff)|0x40000000;
  }
}

ArithmeticDecoder::ArithmeticDecoder(BitInputStream &bis) : _bis(bis), _value(0) {
  _value = _bis.getReverse(16)<<15;
  _value |= _bis.getReverse(15);
}

void ArithmeticDecoder::_normalize() {
  uint32_t lh = ~_low&_high;
  if( (lh&0x40000000)==0 ) {
    if( (lh&0x20000000)==0 ) {
      uint l = bitLeadingZeroes((lh<<2)+3);
      _low = (_low<<(l+1))&0x7fffffff;
      _high = (((_high+1)<<(l+1))-1)&0x7fffffff;
      if( l<=15 ) {
        _value = ((_value<<(l+1))+_bis.getReverse(l+1))&0x7fffffff;
      } else {
        _value = ((_value<<16)+_bis.getReverse(16))&0x7fffffff;
        _value = ((_value<<(l-15))+_bis.getReverse(l-15))&0x7fffffff;
      }
    } else {
      _low = (_low<<1)&0x7fffffff;
      _high = ((_high<<1)+1)&0x7fffffff;
      _value = ((_value<<1)+_bis.get(1))&0x7fffffff;
    }
  }
  lh = ~_low|_high;
  if( (lh&0x20000000)==0 ) {
    uint l = bitLeadingZeroes((lh<<2)+3);
    _low = (_low<<l)&0x3fffffff;
    _high = ((((_high+1)<<l)-1)&0x3fffffff)|0x40000000;
    if( l<=16 ) {
      _value = (((_value<<l)+_bis.getReverse(l))-0x40000000)&0x7fffffff;
    } else {
      _value = ((_value<<16)+_bis.getReverse(16));
      _value = (((_value<<(l-16))+_bis.getReverse(l-16))-0x40000000)&0x7fffffff;
    }
  }
}

bool modelCheckFixed(uint bounds[], uint16_t ids[], uint16_t rids[], const uint N) {
  uint idx = N;
  for( uint i = 0; i<N; ++i ) {
    if( bounds[i] ) {
      if( idx!=N ) {
        return false;
      }
      idx = i;
    }
  }
  ids[N-1] = idx;
  rids[idx] = N-1;
  bounds[idx] = 0;
  bounds[N] = 1<<16;
  return true;
}

void modelSortBounds(uint bounds[], uint16_t ids[], uint16_t rids[], uint backup[], const uint N) {
  for( uint i = 0; i<N; ++i ) {
    ids[i] = i;
    backup[i] = bounds[i];
  }
  std::sort(ids, ids+N, [=](uint i1, uint i2) {
    if( backup[i1]!=backup[i2] ) {
      return backup[i1]<backup[i2];
    }
    return i1<i2;
  });
  for( uint i = 0; i<N; ++i ) {
    bounds[i] = backup[ids[i]];
    rids[ids[i]] = i;
  }
}

void modelRecreateBounds(uint bounds[], const uint N) {
  uint sum = sumArray(bounds, N), acc, prev;
  prev = bounds[0];
  bounds[0] = acc = 0;
  for( uint i = 0; i<N; ++i ) {
    if( prev ) {
      acc += prev;
      prev = bounds[i+1];
      int diff = (((uint64_t)acc)<<16)/sum-bounds[i];
      uint diff_bits = bitLength(diff);
      const uint k = 5;
      if( diff>0&&diff_bits>k ) {
        diff = diff&(((1<<k)-1)<<(diff_bits-k));
      }
      bounds[i+1] = bounds[i]+diff;
      if( bounds[i+1]<=bounds[i] ) {
        bounds[i+1] = bounds[i]+1;
      }
    } else {
      prev = bounds[i+1];
      bounds[i+1] = bounds[i];
    }
  }
  if( bounds[N]>0 ) {
    bounds[N] = 1<<16;
  }
}

void ACFixedScaleBinaryModel::build() {
  if( bounds[0]==0||bounds[1]==0 ) {
    _fixed = true;
    ids[1] = bounds[0]==0;
    rids[ids[1]] = 1;
    bounds[1] = bounds[0] = 0;
    bounds[2] = 1<<16;
    return;
  }
  ids[0] = 0;
  ids[1] = 1;
  if( bounds[1]<bounds[0] ) {
    std::swap(ids[0], ids[1]);
    std::swap(bounds[0], bounds[1]);
  }
  rids[ids[0]] = 0;
  rids[ids[1]] = 1;
  modelRecreateBounds(bounds, 2);
}

uint sumArray(const uint* data, const uint n) {
  uint sum = 0;
  for( uint i = 0; i<n; ++i ) {
    sum += data[i];
  }
  return sum;
}

uint bitLength(uint value) {
  uint l = 0;
  while( value>0 ) {
    l++;
    value >>= 1;
  }
  return l;
}

static byte reverse4[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};
static uint bitReverse8(const uint value) {
  return (reverse4[value&0x0f]<<4)|reverse4[(value>>4)&0x0f];
}

static uint bitReverse16(const uint value) {
  return (bitReverse8(value&0xff)<<8)|bitReverse8(value>>8);
}

static uint bitReverse32(const uint value) {
  return (bitReverse16(value&0xffff)<<16)|bitReverse16(value>>16);
}

uint bitReverse(const uint value, const uint bits) {
  if( bits<=8 ) {
    return bitReverse8(value)>>(8-bits);
  }
  if( bits<=16 ) {
    return bitReverse16(value)>>(16-bits);
  }
  return bitReverse32(value)>>(32-bits);
}

static byte leading4[16] = {4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
uint bitLeadingZeroes(const uint value_) {
  if( value_==0 ) {
    return 32;
  }
  uint value = value_;
  uint result = 0;
  while( (value&0xf0000000)==0 ) {
    value <<= 4;
    result += 4;
  }
  return result+leading4[value>>28];
}

static byte trailing4[16] = {4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
uint bitTrailingZeroes(const uint value_) {
  if( value_==0 ) {
    return 32;
  }
  uint value = value_;
  uint result = 0;
  while( (value&0xf)==0 ) {
    value >>= 4;
    result += 4;
  }
  return result+trailing4[value&0xf];
}

BitInputStream::BitInputStream(InputStream &is) : _input(is), _bufPos(0), _bufSize(0), _bufFastLimit(0), _eof(false), _bits(0), _bitsRemaining(0), _totalBitPos(0) {
}

void BitInputStream::_fillBytes() {
  if( _bufPos>=_bufFastLimit ) {
    if( !_eof ) {
      uint remaining = _bufSize-_bufPos;
      memcpy(_buffer+PRE_BUF_EXTRA-remaining, _buffer+_bufPos, remaining);
      _bufPos = PRE_BUF_EXTRA-remaining;
      _bufSize = PRE_BUF_EXTRA+_input.read(_buffer+PRE_BUF_EXTRA, BUF_SIZE);
      _bufFastLimit = std::max(_bufPos, _bufSize-PRE_BUF_EXTRA);
      _eof = _bufSize!=PRE_BUF_EXTRA+BUF_SIZE;
    }
  }
}

void BitInputStream::_fill() {
  if( _bufPos>=_bufFastLimit ) {
    if( !_eof ) {
      _fillBytes();
    }
    while( _bitsRemaining<=BITS-8&&_bufPos<_bufSize ) {
      _bits |= ((size_t)_buffer[_bufPos++])<<_bitsRemaining;
      _bitsRemaining += 8;
    }
    return;
  }
  while( _bitsRemaining<=BITS-8 ) {
    _bits |= ((size_t)_buffer[_bufPos++])<<_bitsRemaining;
    _bitsRemaining += 8;
  }
}

size_t BitInputStream::copyBytesTo(OutputStream &output, const size_t len) {
  if( _bitsRemaining&7 ) {
    return 0;
  }
  uint8_t a[sizeof(_bits)];
  size_t l = 0;
  while( _bitsRemaining>0&&l<len ) {
    a[l++] = _bits&0xff;
    _bitsRemaining -= 8;
    _bits >>= 8;
    _totalBitPos += 8;
  }
  size_t w = output.write(a, l);
  if( w!=l ) {
    return w;
  }
  while( l<len ) {
    uint todo = std::min(len-l, (size_t)(_bufSize-_bufPos));
    w = output.write(_buffer+_bufPos, todo);
    _totalBitPos += 8*w;
    _bufPos += w;
    l += w;
    if( w!=todo||eof() ) {
      return l;
    }
    _fillBytes();
  }
  return l;
}

size_t BitInputStream::getBytes(uint8_t* data, const size_t size_) {
  skipToByte();
  size_t size = size_;
  while( _bitsRemaining>0&&size>0 ) {
    *data++ = _bits&0xff;
    _bitsRemaining -= 8;
    _bits >>= 8;
    _totalBitPos += 8;
    size--;
  }
  while( size>0 ) {
    uint todo = std::min(size, (size_t)(_bufSize-_bufPos));
    memcpy(data, _buffer+_bufPos, todo);
    data += todo;
    _totalBitPos += 8*todo;
    _bufPos += todo;
    size -= todo;
    if( eof() ) {
      return size_-size;
    }
    _fillBytes();
  }
  return size_;
}

uint64_t BitInputStream::getVLI() {
  uint64_t result = 0, o = 0;
  uint s = 0, c;
  uint bitsRemaining = ((_bitsRemaining-1)&7)+1;
  uint limit = 1<<(bitsRemaining-1);
  while( (c = get(bitsRemaining))>=limit ) {
    result += ((uint64_t)(c&(limit-1)))<<s;
    s += (bitsRemaining-1);
    o = (o+1)<<(bitsRemaining-1);
    bitsRemaining = 8;
    limit = 128;
  }
  return result+o+(((uint64_t)c)<<s);
}

BitOutputStream::BitOutputStream(OutputStream &output) : _output(output), _bufPos(0), _bits(0), _bitPos(0) {
}

void BitOutputStream::_flush() {
  while( _bitPos>=8 ) {
    _buffer[_bufPos++] = _bits&0xff;
    _bits >>= 8;
    _bitPos -= 8;
  }
  if( _bufPos>=BUF_SIZE ) {
    _output.write(_buffer, BUF_SIZE);
    memcpy(_buffer, _buffer+BUF_SIZE, _bufPos-BUF_SIZE);
    _bufPos -= BUF_SIZE;
  }
}

void BitOutputStream::flush() {
  _flush();
  if( _bitPos>0 ) {
    _buffer[_bufPos++] = _bits&0xff;
    _bits = 0;
    _bitPos = 0;
  }
  _output.write(_buffer, _bufPos);
  _bufPos = 0;
}

void BitOutputStream::putBytes(const uint8_t* data, const size_t size) {
  flush();
  _output.write(data, size);
}

void BitOutputStream::putVLI(const uint64_t size_) {
  uint64_t size = size_;
  uint bitsRemaining = 8-(_bitPos&7);
  uint limit = 1<<(bitsRemaining-1);
  while( size>=limit ) {
    put(size|limit, bitsRemaining);
    size = (size>>(bitsRemaining-1))-1;
    bitsRemaining = 8;
    limit = 128;
  }
  put(size, bitsRemaining);
}

template <uint N> udivider_t<N> build_udivider(const typename divider_uint_t<N>::type d) {
  typedef typename divider_uint_t<N* 2>::type T1;
  typedef typename divider_uint_t<N>::type T2;
  udivider_t<N> result;
  result.shift = bitLength(d)-1;
  if( (d&(d-1))==0 ) {
    result.magic1 = result.magic2 = ~(T2)0;
  } else {
    T2 shm = 1<<result.shift;
    T2 t = (((T1)shm)<<N)/d;
    T2 r = t*d+d;
    if( r<=shm ) {
      result.magic1 = t+1;
      result.magic2 = 0;
    } else {
      result.magic1 = t;
      result.magic2 = t;
    }
  }
  return result;
}

udivider_t<16> build_udivider_16(const uint16_t d) {
  return build_udivider<16>(d);
}

udivider_t<32> build_udivider_32(const uint32_t d) {
  return build_udivider<32>(d);
}

template <uint N> ucdivider_t<N> build_ucdivider(const typename divider_uint_t<N>::type d) {
  typedef typename divider_uint_t<N* 2>::type T1;
  typedef typename divider_uint_t<N>::type T2;
  ucdivider_t<N> result;
  result.ctrl = bitLength(d)-1;
  if( (d&(d-1))==0 ) {
    result.magic = ~(T2)0;
    result.ctrl |= 0x80;
  } else {
    T2 shm = 1<<result.ctrl;
    T2 t = (((T1)shm)<<N)/d;
    T2 r = t*d+d;
    if( r<=shm ) {
      result.magic = t+1;
    } else {
      result.magic = t;
      result.ctrl |= 0x80;
    }
  }
  return result;
}

ucdivider_t<16> build_ucdivider_16(const uint16_t d) {
  return build_ucdivider<16>(d);
}

ucdivider_t<32> build_ucdivider_32(const uint32_t d) {
  return build_ucdivider<32>(d);
}

template <uint N> sdivider_t<N> build_sdivider(const typename divider_int_t<N>::type d_) {
  sdivider_t<N> result;
  udivider_t<N> uresult = build_udivider<N>(d_<0 ? -d_ : d_);
  result.magic1 = uresult.magic1;
  result.magic2 = uresult.magic2;
  result.shift = uresult.shift;
  result.sign = d_<0 ? -1 : 0;
  return result;
}

sdivider_t<16> build_sdivider_16(const int16_t d) {
  return build_sdivider<16>(d);
}

sdivider_t<32> build_sdivider_32(const int32_t d) {
  return build_sdivider<32>(d);
}

template <uint N> scdivider_t<N> build_scdivider(const typename divider_int_t<N>::type d_) {
  scdivider_t<N> result;
  ucdivider_t<N> uresult = build_ucdivider<N>(d_<0 ? -d_ : d_);
  result.magic = uresult.magic;
  result.ctrl = uresult.ctrl;
  if( d_<0 ) {
    result.ctrl |= 0x40;
  }
  return result;
}

scdivider_t<16> build_scdivider_16(const int16_t d) {
  return build_scdivider<16>(d);
}

scdivider_t<32> build_scdivider_32(const int32_t d) {
  return build_scdivider<32>(d);
}

class HuffmanHelper {
public:
enum { MAX_BL = 25 };
static bool countSymbols(uint (&nextCode)[MAX_BL+2], byte &minLength, byte &maxLength, const byte* symbolBitLengths, const uint symbolCount, const bool disableZeroBitSymbols);
};
HuffmanDecoder::HuffmanDecoder(const byte* symbolBitLengths, const size_t symbolCount, const bool disableZeroBitSymbols, const byte maxBitsPerTable) : _error(false) {
  if( !_constructTables(symbolBitLengths, symbolCount, disableZeroBitSymbols, maxBitsPerTable) ) {
    _constructErrorTable();
  }
}

size_t HuffmanDecoder::_decodeDeeper(BitInputStream &bis, const size_t tableId_) const {
  bis.skip(_table0.peekBits);
  size_t tableId = tableId_;
  do {
    const Table* table = &_tables[tableId];
    size_t v = bis.peek(table->peekBits);
    int16_t w = table->lookup[v];
    if( w>=0 ) {
      bis.skip(w&0xf);
      return w>>4;
    }
    bis.skip(table->peekBits);
    tableId = ~w;
  } while( true );
}

bool HuffmanDecoder::_constructTables(const byte* symbolBitLengths, const size_t symbolCount, const bool disableZeroBitSymbols, const byte maxBitsPerTable) {
  if( maxBitsPerTable<1||maxBitsPerTable>15 ) {
    return false;
  }
  uint nextCode[HuffmanHelper::MAX_BL+2];
  byte minLength, maxLength;
  if( !HuffmanHelper::countSymbols(nextCode, minLength, maxLength, symbolBitLengths, symbolCount, disableZeroBitSymbols) ) {
    return false;
  }
  _table0.peekBits = std::min((byte)(maxLength-1), maxBitsPerTable);
  _table0.lookup.resize(1<<_table0.peekBits);
  std::fill(_table0.lookup.begin(), _table0.lookup.end(), 0);
  byte minL = disableZeroBitSymbols ? 2 : 1;
  for( uint i = 0; i<symbolCount; ++i ) {
    byte l = (byte)(symbolBitLengths[i]+1);
    if( l<minL ) {
      continue;
    }
    byte k = l-1, maxK = maxLength-1;
    uint code = bitReverse(nextCode[l]++, k);
    Table* t = &_table0;
    while( k>t->peekBits ) {
      k -= t->peekBits;
      maxK -= t->peekBits;
      uint subbits = code&((1<<t->peekBits)-1);
      code >>= t->peekBits;
      int16_t v = t->lookup[subbits];
      if( v>=0 ) {
        uint newTableId = _tables.size();
        t->lookup[subbits] = ~newTableId;
        _tables.push_back(Table());
        t = &_tables[newTableId];
        t->peekBits = std::min(maxK, maxBitsPerTable);
        t->lookup.resize(1<<t->peekBits);
        std::fill(t->lookup.begin(), t->lookup.end(), 0);
      } else {
        t = &_tables[~v];
      }
    }
    do {
      t->lookup[code] = (i<<4)|k;
      code += 1<<k;
    } while( code<t->lookup.size() );
  }
  return true;
}

void HuffmanDecoder::_constructErrorTable() {
  _error = true;
  _table0.peekBits = 0;
  _table0.lookup.resize(1);
  _table0.lookup[0] = 0;
}

HuffmanEncoder::HuffmanEncoder(const byte* symbolBitLengths, const uint symbolCount, const bool disableZeroBitSymbols) : _error(false) {
  if( !_constructTables(symbolBitLengths, symbolCount, disableZeroBitSymbols) ) {
    _constructErrorTable(symbolCount);
  }
}

bool HuffmanEncoder::_constructTables(const byte* symbolBitLengths, const uint symbolCount, const bool disableZeroBitSymbols) {
  uint nextCode[HuffmanHelper::MAX_BL+2];
  byte minLength, maxLength;
  if( !HuffmanHelper::countSymbols(nextCode, minLength, maxLength, symbolBitLengths, symbolCount, disableZeroBitSymbols) ) {
    return false;
  }
  byte minL = disableZeroBitSymbols ? 2 : 1;
  _lookup.resize(symbolCount);
  for( uint i = 0; i<symbolCount; ++i ) {
    byte l = (byte)(symbolBitLengths[i]+1);
    if( l<minL ) {
      _lookup[i] = 0;
      continue;
    }
    byte k = l-1;
    uint code = bitReverse(nextCode[l]++, k);
    _lookup[i] = (code<<5)|k;
  }
  return true;
}

void HuffmanEncoder::_constructErrorTable(const uint symbolCount) {
  _error = true;
  _lookup.resize(symbolCount);
  std::fill(_lookup.begin(), _lookup.end(), 0);
}

bool HuffmanHelper::countSymbols(uint (&nextCode)[MAX_BL+2], byte &minLength, byte &maxLength, const byte* symbolBitLengths, const uint symbolCount, const bool disableZeroBitSymbols) {
  if( symbolCount<1||symbolCount>=1024 ) {
    return false;
  }
  uint16_t blCount[MAX_BL+2];
  memset(blCount, 0, sizeof(blCount));
  for( uint i = 0; i<symbolCount; ++i ) {
    byte l = (byte)(symbolBitLengths[i]+1);
    if( l>MAX_BL+1 ) {
      return false;
    }
    blCount[l]++;
  }
  for( minLength = 1; minLength<=MAX_BL+1; ++minLength ) {
    if( blCount[minLength] ) {
      break;
    }
  }
  for( maxLength = MAX_BL+1; maxLength>=minLength; --maxLength ) {
    if( blCount[maxLength] ) {
      break;
    }
  }
  if( minLength>maxLength ) {
    return false;
  }
  blCount[0] = 0;
  if( disableZeroBitSymbols ) {
    blCount[1] = 0;
  }
  uint code = 0;
  for( uint i = minLength; i<=maxLength; ++i ) {
    code = (code+blCount[i-1])<<1;
    nextCode[i] = code;
  }
  if( minLength==maxLength&&blCount[maxLength]==1 ) {
    return true;
  }
  return nextCode[maxLength]+blCount[maxLength]==(uint)(1<<(maxLength-1));
}

MemStream::MemStream() : _pos(0) {
}

MemStream::MemStream(const std::vector<uint8_t> &content) : _data(content), _pos(0) {
}

MemStream::MemStream(const std::vector<uint8_t> &content, const size_t off, const size_t sz) : _data(std::max(std::min(content.size(), off+sz), off)-off), _pos(0) {
  memcpy(_data.data(), content.data()+off, _data.size());
}

bool MemStream::eof() const {
  return _pos==_data.size();
}

size_t MemStream::read(byte* buffer, const size_t size) {
  size_t toCopy = std::min(size, _data.size()-_pos);
  memcpy(buffer, _data.data()+_pos, toCopy);
  _pos += toCopy;
  return toCopy;
}

size_t MemStream::write(const byte* buffer, const size_t size) {
  size_t remaining = _data.size()-_pos;
  if( size>remaining ) {
    _data.resize(_pos+size);
  }
  memcpy(_data.data()+_pos, buffer, size);
  _pos += size;
  return size;
}

uint64_t MemStream::tell() const {
  return _pos;
}

uint64_t MemStream::seek(const uint64_t newPos) {
  size_t oldPos = _pos;
  _pos = std::min(newPos, (uint64_t)_data.size());
  return oldPos;
}

OutputCacheStream::OutputCacheStream(OutputStream &os) : _os(os), _cacheStartPos(0) {
}

OutputCacheStream::~OutputCacheStream() {
}

void OutputCacheStream::flushUpTo(const uint64_t newStartPos) {
  size_t toWrite = std::min(newStartPos-_cacheStartPos, (uint64_t)_cache.size());
  size_t written = _os.write(_cache.data(), toWrite);
  _cacheStartPos += written;
  _cache.erase(_cache.begin(), _cache.begin()+written);
}

TaskPool globalTaskPool;
TaskPool::TaskPool() : _state(INIT), _threadLimit(0) {
}

void TaskPool::_init() {
  _state = RUN;
}

TaskPool::~TaskPool() {
}
