// Main model - predicts next bit probability from previous data

struct Model {

  uint mask, mask2, word0, word1;
  uint c0, c4;

  uint cxt[15]; 
  uint t1[0x100];
  word t2[0x10000];

  ALIGN(64) ContextMap<MEM * 32, 22> cm;
  APM<0x100> a1;
  APM<0x10000> a2;
  APM<0x10000> a3;
  MatchModel match;
  Mixer<73, 1160, 5> mixer;

  void Init() {
    mask = 0, mask2 = 0, word0 = 0, word1 = 0;
    c0=1; c4=0;
    cm.Init();
    a1.Init(); a2.Init(); a3.Init();
    match.Init();
    mixer.Init();
  }

  // INLINE is load-bearing, not an optimisation: an out-of-line copy of this
  // function is a weak COMDAT symbol, the normal build defines the same symbol,
  // and the linker keeps only one of them - which would silently discard the
  // instrumented body and leave the speculative walk unjournaled. Folding it
  // into encode_sim() leaves that translation unit with one unique symbol.
  INLINE
  int predictNext() {
    MatchModel::t_buf& buf = match.buf;
    int& pos = match.buf.pos;

    W(c0) += c0 + y;
    if (c0 >= 256) {
      W(buf[pos]) = c0; W(pos)++;
      W(c4) = (c4 << 8) + c0 - 256;
      W(c0) = 1;
    }   
    W(bpos) = (bpos + 1) & 7;  
    int c1 = c4 & 0xff, c2 = (c4 & 0xff00) >> 8;

    mixer.update();
    int ismatch = ilog(match.matchModel(mixer,c0));
  //  dmcModel(mixer);

    if (bpos == 0) {
      for (int i = 14; i > 0; --i) {
        W(cxt[i]) = hash(cxt[i - 1], c1);
      }
      cm.set(0);
      cm.set(c1);
      cm.set(c4 & 0x0000ffff);
      cm.set(c4 & 0x00ffffff);
      cm.set(c4);
      cm.set(cxt[5]);
      cm.set(cxt[6]);
      cm.set(cxt[14]);
      cm.set(c4 & 0xf8f8c0ff);
      cm.set(c4 & 0x00e0e0e0);
      cm.set(c4 & 0xffc0ff80);
      cm.set(ismatch | (c4 & 0xffff0000));
      cm.set(ismatch | (c4 & 0x0000ff00));
      cm.set(ismatch | (c4 & 0x00ff0000));
      W(mask) = (mask << 3) | (!c1 ? 0 : is_alpha(c1) ? 1 : is_punct(c1) ? 2 :
        is_space(c1) ? 3 : (c1 == 255) ? 4 : (c1 < 16) ? 5 : (c1 < 64) ? 6 : 7);
      cm.set(mask);
      W(mask2) = (mask2 << 3) | ((mask >> 27) & 7);
      cm.set(hash(mask << 5, mask2 << 2));
      uint& ic1r = t1[c2];
      W(ic1r) = ic1r << 8 | c1;
      word& ic2r = t2[(buf(3) << 8) | c2];
      W(ic2r) = ic2r << 8 | c1;
      const uint ic1 = c1 | t1[c1] << 8;
      const uint ic2 = ((c2 << 8) | c1) | t2[(c2 << 8) | c1] << 16;
      cm.set((ic1 >> 8) & ((1 << 16) - 1));
      cm.set((ic2 >> 16) & ((1 << 8) - 1));
      cm.set(ic1 & ((1 << 16) - 1));
      cm.set(ic2 & ((1 << 24) - 1));  
      //cm.set(hash(4 * (buf(5) == 0x0f) + 8 * (buf(5) == 0x66)
      //       + 12 * (buf(5) == 0x67) + (buf(4) == 0x0f) + 2 * (buf(4) == 0x66)
      //       + 3 * (buf(4) == 0x67), buf(3), buf(2) & 0xc7));   
      int c = (c1 >= 'A' && c1 <= 'Z') ? c1 + 'a' - 'A' : c1;
      if ((c >= 'a' && c <= 'z') || c >= 128) W(word0) = hash(word0, c);
      else if (word0) W(word1) = word0, W(word0) = 0;
      cm.set(word0);
      cm.set(hash(word0, word1));
    }
    
    int o = cm.mix(mixer,buf,c0);
    mixer.set(c1 + 8, 264);
    mixer.set(c0, 256);
    mixer.set(o + ((c1 > 32) << 4) + ((bpos == 0) << 5) + ((c1 == c2) << 6), 128);
    mixer.set(c2, 256);
    mixer.set(ismatch, 256);  
       
    int pr0 = mixer.p();
    return (a1.p(pr0, c0) * 5
           + a2.p(pr0, c0 + (c1 << 8)) * 15
           + a3.p(pr0, hash(bpos, c1, c2) & 0xffff) * 12
           + 16) >> 5; // Probability adjusted with 3 APMs                       
  }

};

//ALIGN(64) Model M;
//ALIGN(64) byte M_memory[sizeof(Model)];
