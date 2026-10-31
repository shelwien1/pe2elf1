// audioinfo.inc -- AudioInfo: what the audio is, as a run has it (its frames), for the progress line
// (progress.inc), and in words.

// What the audio is, as the frames of the stream's first substream have it (AC-3, or E-AC-3's
// independent substream 0): known 0, not yet
struct AudioInfo {
  int known, eac3, bsid, srate, acmod, lfe, kbps, nblk;

  // from a frame of the first substream; false for one of another
  bool set(const Frame& f) {
    if (f.eac3 && (f.b.strmtyp == 1 || f.b.substreamid)) return false;
    memset(this, 0, sizeof *this);
    known = 1;
    eac3 = f.eac3;
    bsid = f.b.bsid;
    srate = f.srate;
    acmod = f.b.acmod;
    lfe = f.lfe;
    nblk = f.nblk;
    // the bit rate: AC-3's by frmsizecod; E-AC-3's from the frame size, rounded
    if (!f.eac3) kbps = bitrate_tab[f.b.frmsizecod >> 1] >> f.sr_shift;
    else kbps = (int)(((S64)f.fbytes * BYTE_BITS * f.srate / (NBIN * f.nblk) + 500) / 1000);
    return true;
  }
  bool same(const AudioInfo& a) const { return !memcmp(this, &a, sizeof a); }

  // what the audio is, in words, into b (n bytes)
  void words(char* b, size_t n) const {
    static const char* const mode[8] = {"1+1", "1/0", "2/0", "3/0", "2/1", "3/1", "2/2", "3/2"};
    size_t k = 0;
    put_text(b, n, k, "%s (bsid %d), %d Hz, %s%s, %d kbps", eac3 ? "E-AC-3" : "AC-3", bsid, srate, mode[acmod & 7],
             lfe ? "+LFE" : "", kbps);
    if (eac3) put_text(b, n, k, ", %d block%s a frame", nblk, nblk == 1 ? "" : "s");
  }
};
