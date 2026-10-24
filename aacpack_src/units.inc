// The containers, walked as the syntax below them (syntax.inc), by the
// same functions both ways (aac2tsv.cpp's adts_frame_r/_w, loas_frame_r/_w,
// stream_mux_config(), adif_header(), au_w(), as one each): a unit is an
// ADTS or LOAS frame, an ADIF header, a raw_data_block of ADIF or of raw
// access units (-asc), the AUS1 header or one of its access units. What a
// writer derives (lengths, CRCs, block positions) is derived, and the
// channel has the value where it is not what is derived.

namespace aacpack {

// ---------------- ADTS ----------------

// the frame length if p holds a usable ADTS header, else 0
static size_t adts_frame_len(const uint8_t* p, size_t avail) {
  if (avail < 7 || p[0] != 0xff || (p[1] & 0xf6) != 0xf0) return 0;  // syncword, layer 0
  if (((p[2] >> 2) & 15) > 12) return 0;
  size_t len = (size_t)(p[3] & 3) << 11 | (size_t)p[4] << 3 | p[5] >> 5;
  size_t min = 7 + ((p[1] & 1) ? 0 : 2 + 2 * (p[6] & 3));
  return len >= min ? len : 0;
}

static uint16_t adts_crc(const uint8_t* b, size_t hdr_start, size_t hdr_bits, const Buf<CrcReg>* regs) {
  uint16_t crc = 0xffff;
  crc = crc16_bits(crc, b, hdr_start, hdr_bits);
  if (regs)
    for (const CrcReg& r : *regs) {
      size_t len = r.end - r.start;
      if (r.max) {
        size_t n = len < (size_t)r.max ? len : (size_t)r.max;
        crc = crc16_bits(crc, b, r.start, n);
        crc = crc16_zeros(crc, r.max - n);
      } else {
        crc = crc16_bits(crc, b, r.start, len);
      }
    }
  return crc;
}

static void adts_config(SynState& st, const AdtsFixed& fx) {
  st.er = 0;
  st.aot = fx.profile + 1;
  st.sf_index = fx.sf_index;
  st.frame960 = 0;
  st.sbr_rate = 2 * sample_rates[fx.sf_index];
  st.blocks = 1;
}

// What a frame's walk keeps for its CRCs (a unit's walk is one of these at
// a time: Walk's, so that its room stays)
struct AdtsRegs {
  Buf<CrcReg> r[4];
  void Init() {
    for (Buf<CrcReg>& b : r) b.Init();
  }
  void Quit() {
    for (Buf<CrcReg>& b : r) b.Quit();
  }
};

// An ADTS frame: parsing, the bytes [fs, fs + flen) of the walk's buffer;
// writing, into its fw
static void adts_frame(Walk& c, AdtsRegs& rg, size_t fs, size_t flen) {
  BitReader& br = c.br;
  const size_t fbeg = fs * 8, fend = (fs + flen) * 8;
  AdtsFixed fx;
  memset((void*)&fx, 0, sizeof fx);
  int cib = 0, cis = 0, full = 0, nrdb = 0;
  if (c.R) {
    br.Init(c.buf, fbeg, fend);
    br.get(12);  // syncword
    fx.id = (int)br.get1();
    fx.layer = (int)br.get(2);
    fx.prot_absent = (int)br.get1();
    fx.profile = (int)br.get(2);
    fx.sf_index = (int)br.get(4);
    fx.private_bit = (int)br.get1();
    fx.chan_config = (int)br.get(3);
    fx.original = (int)br.get1();
    fx.home = (int)br.get1();
    cib = (int)br.get1();
    cis = (int)br.get1();
    br.get(13);  // aac_frame_length: derived
    full = (int)br.get(11);
    nrdb = (int)br.get(2);
  }
  // the fixed header: the last frame's, or its fields
  int same = (int)c.pass(E_ADTS_SAME, 1, c.st.have_fixed && !memcmp(&fx, &c.st.fixed, sizeof fx));
  if (same) {
    if (!c.st.have_fixed) c.bad("corrupt stream: an ADTS frame with the header of none before it");
    fx = c.st.fixed;
  } else {
    fx.id = (int)c.pass(E_ADTS_ID, 1, fx.id);
    fx.layer = (int)c.pass(E_ADTS_LAYER, 2, fx.layer);
    fx.prot_absent = (int)c.pass(E_ADTS_PROT, 1, fx.prot_absent);
    fx.profile = (int)c.pass(E_ADTS_PROFILE, 2, fx.profile);
    fx.sf_index = (int)c.pass(E_ADTS_SFI, 4, fx.sf_index);
    fx.private_bit = (int)c.pass(E_ADTS_PRIV, 1, fx.private_bit);
    fx.chan_config = (int)c.pass(E_ADTS_CHCFG, 3, fx.chan_config);
    fx.original = (int)c.pass(E_ADTS_ORIG, 1, fx.original);
    fx.home = (int)c.pass(E_ADTS_HOME, 1, fx.home);
    if (fx.sf_index > 12) c.bad_value(E_ADTS_SFI, fx.sf_index);
  }
  c.st.fixed = fx;
  c.st.have_fixed = 1;
  cib = (int)c.pass(E_ADTS_CIB, 1, cib);
  cis = (int)c.pass(E_ADTS_CIS, 1, cis);
  full = (int)c.pass(E_ADTS_FULL, 11, full);
  nrdb = (int)c.pass(E_ADTS_NRDB, 2, nrdb);
  BitWriter& w = c.fw;
  size_t flpos = 0;
  if (!c.R) {
    w.put(0xfff, 12);
    w.put(fx.id, 1);
    w.put(fx.layer, 2);
    w.put(fx.prot_absent, 1);
    w.put(fx.profile, 2);
    w.put(fx.sf_index, 4);
    w.put(fx.private_bit, 1);
    w.put(fx.chan_config, 3);
    w.put(fx.original, 1);
    w.put(fx.home, 1);
    w.put(cib, 1);
    w.put(cis, 1);
    flpos = w.pos;
    w.put(0, 13);
    w.put(full, 11);
    w.put(nrdb, 2);
  }
  adts_config(c.st, fx);

  unsigned pos[3] = {0, 0, 0}, hcrc = 0, bcrc[4] = {0, 0, 0, 0};
  size_t pospos = 0, hcrcpos = 0, bcrcpos[4] = {0, 0, 0, 0};
  if (!fx.prot_absent) {
    if (nrdb) {
      if (c.R) {
        for (int i = 0; i < nrdb; i++) pos[i] = br.get(16);
      } else {
        pospos = w.pos;
        for (int i = 0; i < nrdb; i++) w.put(0, 16);
      }
    }
    if (c.R)
      hcrc = br.get(16);
    else
      hcrcpos = w.pos, w.put(0, 16);
  }

  size_t bstart[4] = {0, 0, 0, 0};
  int nstarted = 0, nbcrc = 0;
  bool ok = true;
  for (int b = 0; b <= nrdb; b++) {
    bstart[b] = c.bitpos();
    nstarted++;
    ok = c.raw_data_block(fend, true);
    if (!ok) break;
    rg.r[b].copy(c.crcr);
    if (!fx.prot_absent && nrdb) {
      // (a frame cut short: the rest in place of the block's CRC)
      if (c.pass(E_ADTS_BREST, 1, c.R && br.left() < 16)) {
        if (c.R)
          c.rest_r(br.pos, fend);
        else
          c.rest_w();
        ok = false;
        break;
      }
      if (c.R)
        bcrc[b] = br.get(16);
      else
        bcrcpos[b] = w.pos, w.put(0, 16);
      nbcrc++;
    }
  }
  if (ok) {  // the bytes after the last raw_data_block (the frame's length is derived)
    if (c.R && (br.pos & 7)) c.bad("an ADTS frame's blocks end inside a byte");
    size_t tn = (size_t)c.cnt(E_TAIL_N, c.R ? (int64_t)((fend - br.pos) / 8) : 0, 0, 8191);
    c.bitstr(E_TAIL, tn * 8);
  }

  // the CRCs and block positions, where they are not the values computed
  // from the frame
  int pok = 1, hok = 1, bok[4] = {1, 1, 1, 1};
  if (!fx.prot_absent) {
    if (nrdb) {
      int match = 1;
      for (int i = 1; i <= nrdb; i++)
        if (i >= nstarted || pos[i - 1] != (bstart[i] - bstart[0]) / 8) match = 0;
      pok = (int)c.pass(E_ADTS_POS_OK, 1, match);
      if (!pok)
        for (int i = 0; i < nrdb; i++) pos[i] = (unsigned)c.pass(E_ADTS_POS, 16, pos[i], i);
      hok = (int)c.pass(E_ADTS_HCRC_OK, 1, c.R && adts_crc(c.buf, fbeg, 56 + 16 * nrdb, nullptr) == hcrc);
      if (!hok) hcrc = (unsigned)c.pass(E_ADTS_HCRC, 16, hcrc);
      for (int b = 0; b < nbcrc; b++) {
        bok[b] = (int)c.pass(E_ADTS_BCRC_OK, 1, c.R && adts_crc(c.buf, 0, 0, &rg.r[b]) == bcrc[b], b);
        if (!bok[b]) bcrc[b] = (unsigned)c.pass(E_ADTS_BCRC, 16, bcrc[b], b);
      }
    } else {
      hok = (int)c.pass(E_ADTS_HCRC_OK, 1, c.R && ok && adts_crc(c.buf, fbeg, 56, &rg.r[0]) == hcrc);
      if (!hok) hcrc = (unsigned)c.pass(E_ADTS_HCRC, 16, hcrc);
    }
  }
  if (c.R) return;
  if (w.pos % 8) c.bad("corrupt stream: an ADTS frame of other than whole bytes");
  if (w.pos / 8 > 8191) c.bad("corrupt stream: an ADTS frame longer than 8191 bytes");
  w.patch(flpos, (uint32_t)(w.pos / 8), 13);
  if (!fx.prot_absent) {
    if (nrdb) {
      for (int i = 1; i <= nrdb; i++) {
        unsigned v = !pok ? pos[i - 1] : i < nstarted ? (unsigned)((bstart[i] - bstart[0]) / 8) : 0;
        w.patch(pospos + 16 * (i - 1), v, 16);
      }
      w.patch(hcrcpos, hok ? adts_crc(w.b.p, 0, 56 + 16 * nrdb, nullptr) : hcrc, 16);
      for (int b = 0; b < nbcrc; b++) w.patch(bcrcpos[b], bok[b] ? adts_crc(w.b.p, 0, 0, &rg.r[b]) : bcrc[b], 16);
    } else {
      w.patch(hcrcpos, hok ? adts_crc(w.b.p, 0, 56, ok ? &rg.r[0] : nullptr) : hcrc, 16);
    }
  }
}

// ---------------- LOAS / LATM ----------------

static size_t loas_frame_len(const uint8_t* p, size_t avail) {
  if (avail < 3 || p[0] != 0x56 || (p[1] & 0xe0) != 0xe0) return 0;
  return ((size_t)(p[1] & 0x1f) << 8 | p[2]) + 3;
}

static void set_config(SynState& st, const AscInfo& a, bool blocks) {
  st.aot = a.aot;
  st.sf_index = a.sf_index;
  st.frame960 = a.frame960;
  st.sbr_rate = a.sf_index <= 12 ? 2 * sample_rates[a.sf_index] : 0;
  st.blocks = blocks && a.raw_blocks;
  st.er = a.er;
  st.chan_config = a.chan_config;
}

// LatmGetValue(): bytesForValue and the value
static int64_t latm_value(Walk& c, int enb, int ev) {
  int nb = (int)c.u(enb, 2);
  return c.u(ev, 8 * (nb + 1));
}

// An AudioSpecificConfig: of nbits_fixed bits (LATM version 1's ascLen),
// or (-1) as long as it parses to be, which must be so
static void asc_field(Walk& c, int64_t nbits_fixed) {
  AscInfo a;
  memset((void*)&a, 0, sizeof a);
  bool ok;
  if (c.R) {
    const size_t a0 = c.br.pos;
    size_t nbits;
    if (nbits_fixed < 0) {
      BitReader b = c.br;
      a = parse_asc(b, a0);  // (unparsable: the frame is stored as its bytes)
      nbits = b.pos - a0;
      c.cnt(E_ASC_NBITS, (int64_t)nbits, 0, 1 << 20);
      ok = true;
    } else {
      if ((size_t)nbits_fixed > c.br.left()) c.bad("AudioSpecificConfig overrun");
      nbits = (size_t)nbits_fixed;
      ok = asc_parses(c.buf, a0, nbits, false, a);
    }
    c.bitstr(E_ASC, nbits);
  } else {
    size_t nbits = nbits_fixed < 0 ? (size_t)c.cnt(E_ASC_NBITS, 0, 0, 1 << 20) : (size_t)nbits_fixed;
    const size_t a0 = c.w->pos;
    c.bitstr(E_ASC, nbits);
    ok = asc_parses(c.w->b.p, a0, nbits, nbits_fixed < 0, a);
    if (!ok && nbits_fixed < 0) c.bad("corrupt stream: an unparsable AudioSpecificConfig");
  }
  if (ok)
    set_config(c.st, a, true);
  else
    c.st.blocks = 0;
}

// StreamMuxConfig(), the subset with one program and layer, all streams
// in the same time framing, frameLengthType 0 (as aacdec)
static void stream_mux_config(Walk& c) {
  SynState& st = c.st;
  int v = (int)c.u(E_SMC_VER, 1);  // audioMuxVersion
  if (v && c.u(E_SMC_VERA, 1)) c.bad("audioMuxVersionA 1");
  if (v) latm_value(c, E_SMC_TARA_NB, E_SMC_TARA);  // taraBufferFullness
  if (!c.u(E_SMC_ASTF, 1)) c.bad("allStreamsSameTimeFraming 0");
  int nsub = (int)c.u(E_SMC_NSUB, 6);
  if (c.u(E_SMC_NPROG, 4)) c.bad("several programs");
  if (c.u(E_SMC_NLAYER, 3)) c.bad("several layers");
  if (v)
    asc_field(c, latm_value(c, E_ASC_LEN_NB, E_ASC_LEN));  // ascLen
  else
    asc_field(c, -1);
  if (c.u(E_SMCX_FLT, 3)) c.bad("frameLengthType other than 0");
  c.u(E_SMCX_FULL, 8);  // latmBufferFullness
  int odp = (int)c.u(E_SMCX_ODP, 1);
  int64_t obits = 0;
  if (odp) {
    if (v) {
      obits = latm_value(c, E_SMCX_OLEN_NB, E_SMCX_OLEN);  // otherDataLenBits
    } else {
      int n = 0;
      if (c.R) {
        int esc;
        do {  // otherDataLenEsc, otherDataLenTmp
          esc = (int)c.br.get1();
          obits = obits << 8 | c.br.get(8);
          if (++n > 3) c.bad("otherDataLenBits too large");
        } while (esc);
      }
      n = (int)c.cnt(E_SMCX_OESC_N, n, 1, 3);
      obits = c.pass(E_SMCX_OLEN, 8 * n, obits);
      if (!c.R)
        for (int k = n - 1; k >= 0; k--) {
          c.w->put(k > 0, 1);
          c.w->put((uint32_t)(obits >> (8 * k)) & 255, 8);
        }
    }
    if (obits > 8 * 8192) c.bad("otherDataLenBits larger than a LOAS frame");
  }
  if (c.u(E_SMCX_CRCP, 1)) c.u(E_SMCX_CRC, 8);  // crcCheckPresent, crcCheckSum
  st.latm_cfg = 1;
  st.mux_version = v;
  st.num_subframes = nsub;
  st.other_present = odp;
  st.other_bits = (int)obits;
}

// the bits of a payload after its raw_data_block or ER access unit, to
// its end at pe (parsing): none or zeros to the byte, else what they are
static void paypad(Walk& c, size_t pe) {
  if (c.R) {
    size_t pad = pe - c.br.pos;
    int x = pad >= 8 || (pad && c.br.peek((int)pad));
    c.cnt(E_PAYPAD_N, x ? (int64_t)pad : 0, 0, (int64_t)1 << 40);
    if (x) c.bitstr(E_PAYPAD, pad);
  } else {
    size_t n = (size_t)c.cnt(E_PAYPAD_N, 0, 0, (int64_t)c.fw.cap);
    if (n)
      c.bitstr(E_PAYPAD, n);
    else
      c.w->put(0, (int)((8 - c.w->pos % 8) % 8));
  }
}

// An AudioSyncStream frame: parsing, the bytes [fs, fs + flen); writing, into fw
static void loas_frame(Walk& c, size_t fs, size_t flen) {
  BitReader& br = c.br;
  const size_t fend = (fs + flen) * 8;
  BitWriter& w = c.fw;
  size_t lpos = 0;
  if (c.R) {
    br.Init(c.buf, fs * 8, fend);
    br.get(11);  // syncword
    br.get(13);  // audioMuxLengthBytes: derived
  } else {
    w.put(0x2b7, 11);
    lpos = w.pos;
    w.put(0, 13);
  }
  const size_t ame = c.bitpos();
  if (!c.u(E_LOAS_SAME, 1))  // useSameStreamMux
    stream_mux_config(c);
  else if (!c.st.latm_cfg)
    c.bad("a LOAS frame with no StreamMuxConfig yet");
  for (int k = 0; k <= c.st.num_subframes; k++) {
    size_t len = 0, ps = 0, pe = 0;
    if (c.R) {
      int t;
      do {  // MuxSlotLengthBytes
        t = (int)br.get(8);
        len += t;
      } while (t == 255);
      if (len * 8 > br.left()) c.bad("payload overrun");
      ps = br.pos, pe = ps + 8 * len;
    } else {
      c.push_sub();
    }
    if (c.st.blocks) {
      if (c.R) br.end = pe;
      bool ok = c.st.er ? c.er_frame(pe) : c.raw_data_block(pe, false);
      if (c.R) br.end = fend;
      if (ok) paypad(c, pe);
    } else {
      len = (size_t)c.cnt(E_PAYRAW_N, (int64_t)len, 0, 1 << 20);
      c.bitstr(E_PAYRAW, 8 * len);
    }
    if (c.R) {
      br.pos = pe;
    } else {
      if (c.w->pos % 8) c.bad("corrupt stream: a payload of other than whole bytes");
      BitWriter& s = c.pop_sub();
      size_t n = s.pos / 8;
      for (; n >= 255; n -= 255) w.put(255, 8);
      w.put((uint32_t)n, 8);
      w.append(s);
    }
  }
  if (c.st.other_present) {
    if (c.R && (size_t)c.st.other_bits > br.left()) c.bad("other data overrun");
    c.bitstr(E_OTHER, (size_t)c.st.other_bits);
  }
  c.u(E_LOAS_ALIGN, (int)((ame - c.bitpos()) & 7));  // byte_align()
  if (c.R && (br.pos & 7)) c.bad("a LOAS frame's payloads end inside a byte");
  size_t tn = (size_t)c.cnt(E_TAIL_N, c.R ? (int64_t)((fend - br.pos) / 8) : 0, 0, 8194);
  c.bitstr(E_TAIL, tn * 8);
  if (c.R) return;
  if (w.pos / 8 - 3 > 8191) c.bad("corrupt stream: a LOAS frame longer than 8191 bytes");
  w.patch(lpos, (uint32_t)(w.pos / 8 - 3), 13);
}

// ---------------- ADIF, raw access units, AUS1 ----------------

// adif_header(); hs is the bit position of "ADIF" (the PCEs' byte alignment)
static void adif_header(Walk& c, size_t hs) {
  if (c.R) {
    if (c.br.get(32) != 0x41444946) c.bad("no ADIF header");
  } else {
    c.w->put(0x41444946, 32);
  }
  if (c.u(E_ADIF_CIDP, 1)) c.bitstr(E_ADIF_CID, 72);  // copyright_id_present, copyright_id
  c.u(E_ADIF_ORIG, 1);                                // original_copy
  c.u(E_ADIF_HOME, 1);                                // home
  int bt = (int)c.u(E_ADIF_BST, 1);                   // bitstream_type
  c.u(E_ADIF_RATE, 23);                               // bitrate
  int npce = (int)c.u(E_ADIF_NPCE, 4);
  int ot = 1, sfi = 4;
  for (int i = 0; i <= npce; i++) {
    if (!bt) c.u(E_ADIF_FULL, 20, i);  // adif_buffer_fullness
    int o2, s2;
    c.pce_body(hs, &o2, &s2);
    if (!i) ot = o2, sfi = s2;
  }
  AscInfo a;
  a.aot = ot + 1;
  a.sf_index = sfi;
  a.chan_config = 0;
  a.frame960 = 0;
  a.er = 0;
  a.raw_blocks = sfi <= 12;
  set_config(c.st, a, true);
}

// The AudioSpecificConfig of raw access units (-asc, AUS1): its bytes
// configure the stream (not self-delimiting ER units: not with -asc)
static void asc_config(SynState& st, const uint8_t* asc, size_t n, bool rawasc) {
  AscInfo a;
  if (asc_parses(asc, 0, n * 8, false, a)) {
    set_config(st, a, true);
    if (rawasc && st.er) st.blocks = 0;
  } else {
    st.blocks = 0;
  }
}

// AUS1's header: "AUS1", the AudioSpecificConfig's length (le16) and its
// bytes, at byte fs of the walk's buffer
static void aus1_header(Walk& c, size_t fs, size_t alen) {
  if (c.R) c.br.Init(c.buf, (fs + 6) * 8, (fs + 6 + alen) * 8);
  size_t n = (size_t)c.cnt(E_ASC_N, (int64_t)alen, 0, 65535);
  if (!c.R) {
    c.w->put(0x41555331, 32);  // "AUS1"
    c.w->put((uint32_t)(n & 255), 8);
    c.w->put((uint32_t)(n >> 8), 8);
  }
  c.bitstr(E_ASC, 8 * n);
  // (the config from its bytes, which bitstr leaves in tmp)
  Buf<uint8_t>& t = c.tmp;
  t.resize(n + 16);
  memset(t.data() + n, 0, 16);
  asc_config(c.st, t.data(), n, false);
}

// An AUS1 access unit: its length (le32), its raw_data_block or ER access
// unit and the bits after it, or (a configuration that is not parsed) its
// bytes; parsing, the n bytes at byte fs + 4
static void au_unit(Walk& c, size_t fs, size_t n) {
  BitWriter& w = c.fw;
  size_t ae = 0;
  if (c.R) {
    ae = (fs + 4 + n) * 8;
    c.br.Init(c.buf, (fs + 4) * 8, ae);
  } else {
    w.put(0, 32);
  }
  if (!c.st.blocks) {
    n = (size_t)c.cnt(E_PAYRAW_N, (int64_t)n, 0, (int64_t)1 << 32);
    c.bitstr(E_PAYRAW, 8 * n);
  } else {
    bool ok = c.st.er ? c.er_frame(ae) : c.raw_data_block(ae, true);
    if (ok) paypad(c, ae);
  }
  if (c.R) return;
  uint32_t len = (uint32_t)(w.pos / 8 - 4);
  for (int k = 0; k < 4; k++) w.patch(8 * k, (len >> (8 * k)) & 255, 8);
}

}  // namespace aacpack
