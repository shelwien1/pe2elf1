
struct Rangecoder {
  unsigned int Range;
  unsigned int Code;
  unsigned int Cache;
  unsigned int ff_count;
  unsigned __int64 Low;

  unsigned int SubRange;

  unsigned int GetSubRange(unsigned int freq, unsigned int totFreq) {
    return freq * (Range / totFreq);
  }

  void EncodeMatched(unsigned int subRange) {
    SubRange = subRange;
  }

  void EncodeNotMatched(unsigned int subRange) {
    Low += subRange;
    Range -= subRange;
  }

  bool IsDecodeMatched(unsigned int subRange) {
    if (Code < subRange) {
      SubRange = subRange;
      return true;
    }
    return false;
  }

  void DecodeNotMatched(unsigned int subRange) {
    Code -= subRange;
    Range -= subRange;
  }

  void CommitRange() {
    Range = SubRange;
  }

  void InitEncode() {
    Low = 0;
    Cache = 0;
    Range = -1;
    ff_count = 0;
  }

  void InitDecode(FILE *f) {
    Range = -1;
    int v13 = getc(f);
    int v15 = getc(f);
    int v18 = getc(f);
    int v21 = getc(f);
    int v24 = getc(f);
    int v16 = v15 | (v13 << 8);
    int v19 = v18 | (v16 << 8);
    int v22 = v21 | (v19 << 8);
    Code = v24 | (v22 << 8);
  }

  void EncodeShift(FILE *f) {
    if ((Low ^ 0xFF000000LL) <= 0xFFFFFF) {
      ++ff_count;
    } else {
      putc(Cache + (int)(Low >> 32), f);
      int carry = (int)(Low >> 32) + 255;
      if (ff_count) {
        do {
          putc(carry, f);
        } while (--ff_count);
        Low = (unsigned int)Low;
      }
      Cache = (Low >> 24) & 0xFF;
    }
    Low = (unsigned int)(Low << 8);
    Range <<= 8;
  }

  void EncodeNormalize(FILE *f) {
    while (Range < 0x1000000) {
      EncodeShift(f);
    }
  }

  void DecodeNormalize(FILE *f) {
    while (Range < 0x1000000) {
      Code = getc(f) | (Code << 8);
      Range <<= 8;
    }
  }

  void Flush(FILE *f) {
    for (int i = 0; i < 5; ++i) {
      if ((Low ^ 0xFF000000LL) <= 0xFFFFFF) {
        ++ff_count;
      } else {
        putc(Cache + (int)(Low >> 32), f);
        int carry = (int)(Low >> 32) + 255;
        if (ff_count) {
          do {
            putc(carry, f);
          } while (--ff_count);
          Low = (unsigned int)Low;
        }
        Cache = (Low >> 24) & 0xFF;
      }
      Low = (unsigned int)(Low << 8);
    }
  }
};
