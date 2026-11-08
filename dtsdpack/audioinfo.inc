// audioinfo.inc -- AudioInfo: what the audio is, as a run has it (its core frames), for the progress
// line (progress.inc), and in words.

// What the audio is, as the core frames have it: known 0, not yet
struct AudioInfo {
  int known, amode, lfe, srate, kbps, blocks;

  void set(const CoreRec& c) {
    static const int sfreq[16] = {0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0};
    memset(this, 0, sizeof *this);
    known = 1;
    amode = c.amode;
    lfe = c.lff != 0;
    srate = sfreq[c.sfreq];
    blocks = c.nblks + 1;
    kbps = srate ? (int)(((S64)(c.fsize + 1) * BYTE_BITS * srate / (blocks * 32) + 500) / 1000) : 0;
  }
  bool same(const AudioInfo& a) const { return !memcmp(this, &a, sizeof a); }

  // what the audio is, in words, into b (n bytes)
  void words(char* b, size_t n) const {
    static const char* const mode[10] = {"1/0", "1+1", "2/0", "2/0 (sum-difference)", "2/0 (Lt/Rt)",
                                         "3/0", "2/1", "3/1", "2/2", "3/2"};
    size_t k = 0;
    put_text(b, n, k, "DTS core, %d Hz, ", srate);
    if (amode < 10) put_text(b, n, k, "%s", mode[amode]);
    else put_text(b, n, k, "AMODE %d", amode);
    put_text(b, n, k, "%s, %d kbps, %d samples a frame", lfe ? "+LFE" : "", kbps, blocks * 32);
  }
};
