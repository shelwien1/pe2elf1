/* d5c_codec_pal.inc -- included INSIDE struct Codec; not standalone.
 *
 * Palette construction (apalN / apal8 / cpal4) and the pair/word
 * match helper. */

  //------------------------------------------------- palettes (dxt5.cpp formulas)
  // BTC selects between the two levels themselves; everything else uses the
  // 8-entry BC4/DXT5 alpha ramp
  static void apalN(int a0, int a1, u8P ap, int np) {
    if(np == 2) { ap[0] = u8(a0); ap[1] = u8(a1); return; }
    apal8(a0, a1, ap);
  }
  static void apal8(int a0, int a1, u8P ap) {
    ap[0] = u8(a0); ap[1] = u8(a1);
    if(a0 > a1) for(int i = 1; i <= 6; i++) ap[i + 1] = u8(((7 - i) * a0 + i * a1 + 3) / 7);
    else {
      for(int i = 1; i <= 4; i++) ap[i + 1] = u8(((5 - i) * a0 + i * a1 + 2) / 5);
      ap[6] = 0; ap[7] = 255;
    }
  }
  // mode4: BC1 3-color when !mode4 (DXT1 c0<=c1); BC2/3 always 4-color
  static void cpal4(u16 c0, u16 c1, bool mode4, u8 cp[4][3]) {
    int r0 = c0 >> 11 & 31, g0 = c0 >> 5 & 63, b0 = c0 & 31;
    int r1 = c1 >> 11 & 31, g1 = c1 >> 5 & 63, b1 = c1 & 31;
    int R0 = r0 << 3 | r0 >> 2, G0 = g0 << 2 | g0 >> 4, B0 = b0 << 3 | b0 >> 2;
    int R1 = r1 << 3 | r1 >> 2, G1 = g1 << 2 | g1 >> 4, B1 = b1 << 3 | b1 >> 2;
    cp[0][0] = u8(R0); cp[0][1] = u8(G0); cp[0][2] = u8(B0);
    cp[1][0] = u8(R1); cp[1][1] = u8(G1); cp[1][2] = u8(B1);
    if(mode4) {
      cp[2][0] = u8((2 * R0 + R1 + 1) / 3); cp[2][1] = u8((2 * G0 + G1 + 1) / 3); cp[2][2] = u8((2 * B0 + B1 + 1) / 3);
      cp[3][0] = u8((R0 + 2 * R1 + 1) / 3); cp[3][1] = u8((G0 + 2 * G1 + 1) / 3); cp[3][2] = u8((B0 + 2 * B1 + 1) / 3);
    } else {
      cp[2][0] = u8((R0 + R1 + 1) / 2); cp[2][1] = u8((G0 + G1 + 1) / 2); cp[2][2] = u8((B0 + B1 + 1) / 2);
      cp[3][0] = cp[3][1] = cp[3][2] = 0;
    }
  }

  //------------------------------------------------- pair/word match helper
  // candidates: cW (if hasW), cN (if hasN, distinct from cW), cH (if valid, distinct)
  // returns 0=W 1=N 2=H -1=none; codes the needed bits
  template<class T>
  int match(bool hasW, T cW, bool hasN, T cN, bool hasH, T cH, bool hasH2, T cH2, u32& hist, u32& mtype,
            CtrP t1, CtrP t2, CtrP tn1, CtrP tn2,
            /* NOT restrict: the alpha ENDPOINT pair has only one hash family
             * (HAW2 is for index words, not pairs), so that call site passes
             * the same table -- and the same mixer -- for both hash slots.
             * Caught by D5C_RESTRICT_CHECK, not by inspection. */
            Ctr* th, Ctr* th2,
            Mixer& mW, Mixer& mN, Mixer& mH, Mixer& mH2, int actual /* -1 dec, else 0..3/-2 none */) {
#if D5C_RESTRICT_CHECK
    { const Ctr* rr[4] = {t1,t2,tn1,tn2};   // th/th2 may legitimately be equal
      for(int i=0;i<4;i++) for(int j=i+1;j<4;j++)
        if(rr[i] && rr[i]==rr[j]) { fprintf(stderr,"match(): aliasing tables %d,%d\n",i,j); abort(); } }
#endif
    int eq = hasW && hasN && cW == cN;
    int c1 = cap_ctx(int(MakeM1(hist, u32(eq), mtype)), M0_M1_Volume, PCAP(M0_M1_HB));
    int c2 = cap_ctx(int(MakeM2(hist)), M0_M2_Volume, PCAP(M0_M2_HB));
    int cm = int(MakeMM(u32(eq), hist, mtype));
    int res = -1;
    if(hasW) {
      Ctr* cs[2] = {&t1[c1], &t2[c2]};
      int b = mbit<2>(mW, cm, cs, nullptr, 0, actual < -1 ? 0 : actual < 0 ? -1 : actual == 0);
      if(b) res = 0;
    }
    if(res < 0 && hasN && !(hasW && cW == cN)) {
      Ctr* cs[2] = {&tn1[c1], &tn2[c2]};
      int b = mbit<2>(mN, cm, cs, nullptr, 0, actual < -1 ? 0 : actual < 0 ? -1 : actual == 1);
      if(b) res = 1;
    }
    if(res < 0 && hasH && !(hasW && cH == cW) && !(hasN && cH == cN)) {
      Ctr* cs[2] = {&th[c1], &t2[c2]};
      int b = mbit<2>(mH, cm, cs, nullptr, 0, actual < -1 ? 0 : actual < 0 ? -1 : actual == 2);
      if(b) res = 2;
    }
    if(res < 0 && hasH2 && !(hasW && cH2 == cW) && !(hasN && cH2 == cN) && !(hasH && cH2 == cH)) {
      Ctr* cs[2] = {&th2[c1], &t2[c2]};
      int b = mbit<2>(mH2, cm, cs, nullptr, 0, actual < -1 ? 0 : actual < 0 ? -1 : actual == 3);
      if(b) res = 3;
    }
    hist = hist << 1 | (res >= 0);
    mtype = u32(res + 1);
    return res;
  }
