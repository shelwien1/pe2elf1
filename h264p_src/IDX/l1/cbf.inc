/*  The template IDX/idx2inc.pl reads alongside cbf.idx: the cbf
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/cbf_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/cbf_h.inc.  Edit this file
    and cbf.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_cbf_ND (or H_cbf_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_cbf_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int cat = D.cat, na = D.na, nb = D.nb, intra = D.intra, q6 = D.q6, np = D.np, pos = D.pos, crcb = D.crcb, col = D.col, nc = D.nc, kn = D.kn, md = D.md, en = D.en, pg = D.pg, em = D.em, pz = D.pz, tn = D.tn, pq = D.pq, ch = D.ch, ds = D.ds, hv = D.hv, ly = D.ly, dq = D.dq;
  (void) cat;  (void) na;  (void) nb;  (void) intra;  (void) q6;  (void) np;  (void) pos;  (void) crcb;  (void) col;  (void) nc;  (void) kn;  (void) md;  (void) en;  (void) pg;  (void) em;  (void) pz;  (void) tn;  (void) pq;  (void) ch;  (void) ds;  (void) hv;  (void) ly;  (void) dq;
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_cbf_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_cbf_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_cbf_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_cbf_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_cbf_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_cbf_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_cbf_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_cbf_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_cbf_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_cbf_NG );
