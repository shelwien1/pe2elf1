/*  The template IDX/idx2inc.pl reads alongside pcm.idx: the pcm
    family's index builder and the tables its indices address.  Copied
    through into MOD/pcm_p.inc with `H_pcm_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/pcm_h.inc.  Edit this file and
    pcm.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_pcm_ND (or H_pcm_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_pcm_make( const H_pcm_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int pl = D.pl, dp = D.dp, pd = D.pd, pd2 = D.pd2;
  (void) pl;  (void) dp;  (void) pd;  (void) pd2;

  a = 0;
  a = a*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  a = a*7 + H_pcm_a_dp[__min(6,__max(0,dp-(1-1)))];
  a = a*16 + H_pcm_a_pd[__min(15,__max(0,pd-(1-1)))];

  b = 0;
  b = b*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  b = b*8 + H_pcm_b_pd[__min(7,__max(0,pd-(1-1)))];
  b = b*8 + H_pcm_b_pd2[__min(7,__max(0,pd2-(1-1)))];

  c = 0;
  c = c*3 + ((pl>0+(1-1))+(pl>1+(1-1)));

  d = 0;
  d = d*7 + H_pcm_d_dp[__min(6,__max(0,dp-(1-1)))];

  e = 0;
  e = e*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  e = e*16 + H_pcm_e_pd[__min(15,__max(0,pd-(1-1)))];
  e = e*8 + H_pcm_e_pd2[__min(7,__max(0,pd2-(1-1)))];

  h = 0;
  h = h*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  h = h*7 + H_pcm_h_dp[__min(6,__max(0,dp-(1-1)))];

  i = 0;
  i = i*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  i = i*16 + H_pcm_i_pd[__min(15,__max(0,pd-(1-1)))];

  j = 0;
  j = j*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  j = j*7 + H_pcm_j_dp[__min(6,__max(0,dp-(1-1)))];
  j = j*8 + H_pcm_j_pd2[__min(7,__max(0,pd2-(1-1)))];

  k = 0;
  k = k*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  k = k*8 + H_pcm_k_pd2[__min(7,__max(0,pd2-(1-1)))];

  l = 0;
  l = l*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  l = l*16 + H_pcm_l_pd[__min(15,__max(0,pd-(1-1)))];
  l = l*7 + H_pcm_l_dp[__min(6,__max(0,dp-(1-1)))];

  s = 0;
  s = s*3 + ((pl>0+(1-1))+(pl>1+(1-1)));

  f = 0;
  f = f*3 + ((pl>0+(1-1))+(pl>1+(1-1)));

  m = 0;
  m = m*3 + ((pl>0+(1-1))+(pl>1+(1-1)));

  w = 0;
  w = w*7 + H_pcm_w_dp[__min(6,__max(0,dp-(1-1)))];

  t = 0;
  t = t*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  t = t*7 + H_pcm_t_dp[__min(6,__max(0,dp-(1-1)))];

  n = 0;
  n = n*3 + ((pl>0+(1-1))+(pl>1+(1-1)));

  g = 0;
  g = g*3 + ((pl>0+(1-1))+(pl>1+(1-1)));
  g = g*4 + H_pcm_g_pd[__min(3,__max(0,pd-(1-1)))];
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.i = i;  X.j = j;  X.k = k;  X.l = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

