// The AudioSpecificConfig (14496-3 1.6.2.1), parsed for its length and for
// the configuration the access units need (aac2tsv's asc.h; it follows
// aacdec's ff_mpeg4audio_get_config_gb(), decode_ga_specific_config(),
// decode_eld_specific_config()). Its bits are coded as they are; what does
// not parse is an error (err.inc) its caller's trap takes.

namespace aacpack {

// the channel elements of an ER access unit per channel configuration, in
// order (aacdec's ff_aac_channel_layout_map; ER is decoded for 1-7, 11, 12)
static const char* const er_layout[16] = {nullptr, "S",     "C",     "SC",    "SCS",   "SCC", "SCCL",  "SCCCL",
                                          nullptr, nullptr, nullptr, "SCCSL", "SCCCL", nullptr, nullptr, nullptr};

struct AscInfo {
  int aot;          // core object type (after explicit SBR/PS signalling)
  int sf_index;     // core sampling frequency index (15: explicit rate)
  int chan_config;
  int frame960;     // frameLengthFlag
  int er;           // 1 ER AAC LC/LD, 2 ER AAC ELD
  bool raw_blocks;  // its access units are parsed
};

static int asc_object_type(BitReader& b) {
  int aot = (int)b.get(5);
  if (aot == 31) aot = 32 + (int)b.get(6);
  return aot;
}
static int asc_sample_rate(BitReader& b) {
  int idx = (int)b.get(4);
  if (idx == 15) b.get(24);
  return idx;
}

// program_config_element() after element_instance_tag (decode_pce())
static void asc_skip_pce(BitReader& b, size_t align_ref) {
  b.get(2);  // object_type
  b.get(4);  // sampling_frequency_index
  int nf = (int)b.get(4), ns = (int)b.get(4), nb = (int)b.get(4), nl = (int)b.get(2), na = (int)b.get(3),
      nc = (int)b.get(4);
  if (b.get1()) b.get(4);
  if (b.get1()) b.get(4);
  if (b.get1()) b.get(3);
  b.skip(5 * (size_t)(nf + ns + nb + nc) + 4 * (size_t)(nl + na));
  b.skip((align_ref - b.pos) & 7);
  b.skip(8 * (size_t)b.get(8));  // comment_field_data
}

// An AudioSpecificConfig at b.pos (an error if not supported). The
// backward compatible sync extension is not parsed: in LATM version 1 the
// bits after the core config up to ascLen are kept as they are.
static AscInfo parse_asc(BitReader& b, size_t align_ref) {
  AscInfo a;
  a.aot = asc_object_type(b);
  a.sf_index = asc_sample_rate(b);
  a.chan_config = (int)b.get(4);
  a.frame960 = 0;
  a.er = 0;
  bool ldsbr = false;
  if (a.chan_config > 14) err_raise_s("invalid channel configuration");
  if (a.aot == 5 || (a.aot == 29 && !(b.peek(3) & 0x03 && !(b.peek(9) & 0x3f)))) {
    asc_sample_rate(b);  // extensionSamplingFrequencyIndex
    a.aot = asc_object_type(b);
    if (a.aot == 22) b.get(4);  // extensionChannelConfiguration
  }
  switch (a.aot) {
    case 1: case 2: case 3: case 4: case 17: case 23: {  // GASpecificConfig
      a.frame960 = (int)b.get1();
      if (b.get1()) b.get(14);  // dependsOnCoreCoder, coreCoderDelay
      int ext = (int)b.get1();  // extensionFlag
      if (a.chan_config == 0) {
        b.get(4);  // element_instance_tag
        asc_skip_pce(b, align_ref);
      }
      if (ext) {
        if (a.aot == 17 || a.aot == 23)
          if (b.get(3)) err_raise_s("error resilience tools");  // aacSection/Scalefactor/SpectralDataResilienceFlag
        b.get1();  // extensionFlag3
      }
      if (a.aot == 17 || a.aot == 23)
        if (b.get(2)) err_raise_s("epConfig");
      break;
    }
    case 39: {  // ELDSpecificConfig
      a.frame960 = (int)b.get1();
      if (b.get(3)) err_raise_s("error resilience tools");
      if (b.get1()) {  // ldSbrPresentFlag: aacdec does not decode it
        ldsbr = true;
        b.get(2);  // ldSbrSamplingRate, ldSbrCrcFlag
        static const int nhdr[16] = {0, 1, 1, 2, 3, 3, 3, 4, 0, 0, 0, 4, 4, 0, 4, 0};  // (as fdk-aac)
        for (int k = 0; k < nhdr[a.chan_config]; k++) {  // ld_sbr_header()
          b.get(14);  // bs_amp_res .. bs_reserved
          int x1 = (int)b.get1(), x2 = (int)b.get1();
          if (x1) b.get(5);  // bs_freq_scale, bs_alter_scale, bs_noise_bands
          if (x2) b.get(6);  // bs_limiter_bands .. bs_smoothing_mode
        }
      }
      while (b.get(4)) {  // eldExtType != ELDEXT_TERM
        int len = (int)b.get(4);
        if (len == 15) len += (int)b.get(8);
        if (len == 15 + 255) len += (int)b.get(16);
        b.skip(8 * (size_t)len);
      }
      if (b.get(2)) err_raise_s("epConfig");
      break;
    }
    default:
      err_raise_s("unsupported audio object type");
  }
  a.er = a.aot == 17 || a.aot == 23 ? 1 : a.aot == 39 ? 2 : 0;
  if (!a.er) {
    a.raw_blocks = a.aot >= 1 && a.aot <= 4 && a.sf_index <= 12;
  } else {
    bool ld = a.aot != 17;
    a.raw_blocks = a.sf_index <= 12 && er_layout[a.chan_config] && !ldsbr &&
                   (!ld || (a.frame960 ? ff_aac_num_swb_480 : ff_aac_num_swb_512)[a.sf_index]);
  }
  return a;
}

// an AudioSpecificConfig of nbits at buf (padded): whether it parses (into
// a), and in exactly nbits when exact
static bool asc_parses(const uint8_t* buf, size_t from, size_t nbits, bool exact, AscInfo& a) {
  ErrTrap t;
  if (setjmp(t.jb)) return false;
  err_push(t);
  BitReader b;
  b.Init(buf, from, from + nbits);
  a = parse_asc(b, from);
  bool ok = !exact || b.pos == b.end;
  err_pop(t);
  return ok;
}

}  // namespace aacpack
