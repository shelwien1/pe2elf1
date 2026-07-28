Dual predictor cascade (rev 012)
================================

The two parameter sets disagreed structurally, not marginally: 62 of ~107
Numbers differ, and the NLMS shapes are

              IDX            IDX1
  N1/N2/N3    48/192/192     14/113/10
  N4           8             24
  NCONF        4              2
  MATCH_ORDER2 12            63

One is a long-horizon predictor with a narrow cross-channel filter, the other
short-horizon with a wide one.  Each wins on some material by a lot (your table
has IDX ahead by 5.9% on Mona and behind by 1.1% on Cinematic).  That pattern
says the two shapes see different things, so both now run and the mixer decides
per file.

What changed
------------
  * Pred carries its own shape (n1..n4, mu1..mu4, fl1..fl4, pw0) via setvar(),
    so two instances can differ.  Each channel runs pd and pdB over the same
    reconstructed signal.
  * 13 new IDX Numbers per set: N1B..N4B, MU1B..MU4B, EFL1B..EFL4B, PW0B.
    Each set is seeded with the OTHER set's cascade, so IDX now carries IDX1's
    shape as its B side and vice versa.  Set them equal to the primary and the
    second cascade becomes redundant rather than harmful -- it then only costs
    its two mixer inputs.
  * Two new contexts: tQB (q^ from cascade B with its own confidence) and tQAB
    (q^ from A crossed with q^ from B).  tQAB is the point of the exercise: when
    both shapes agree the next code is far more predictable than either alone
    implies, and a single-cascade model cannot express that at all.

Measured
--------
                     IDX       IDX1      dual      vs best single
  Cinematic_690    366,330   362,302   360,999    -1,303  (-0.36%)
  music_00010      421,672   415,496   415,067      -429  (-0.10%)
  music_00002      641,413   627,182   625,441    -1,741  (-0.28%)

The dual build beats BOTH singles on every file tested.  Note the globals still
matter independently of the cascade -- building with IDX1's globals beat IDX's
by ~0.5% on these three even with the same pair of cascades -- so both sets are
kept.  MOD/ here is generated from IDX1; ./mk.sh release regenerates it from
whichever directory is named IDX.

Var_Mona is the case this should help most and the one file I could not test.

A bug this uncovered
--------------------
MIX_MAXIN (the mixer's st[] capacity) was 24 while rev 011 already used exactly
24 inputs -- zero margin.  The two new contexts pushed it to 26, overflowing
st[] into the Mixer's own nx/cx fields.  That is an INTRA-object overflow, which
ASan does not instrument, so it presented as a segfault at -O0 and a live spin
at -O2 while the ASan build ran clean.  MIX_MAXIN is now 40 with a comment
explaining why it must stay ahead of MIX_NIN.  Worth knowing if you add contexts.

Tuning the new parameters
-------------------------
The B cascade is 13 ordinary IDX Numbers per set (N1B..N4B, MU1B..MU4B,
EFL1B..EFL4B, PW0B), appended to xad-G0.idx exactly like every other tunable.
Nothing about it is hard-coded, so opt.pl reaches it with no special handling:

  set $NoConst = 0 in IDX/idx2inc.pl   (your trees ship with 1 for fast builds)
  ./mk.sh                              (or g.bat with no argument)
  perl IDX/opt.pl opt.lst

Verified on this tree: the Debug binary exposes 123 maps / 1639 search bits, up
from 110 / 1442, with N1B..N4B at 9 bits each, MU*B/EFL*B at 4, PW0B at 17.

Both sets carry both cascades, each seeded with the other's shape, so either
directory is a valid starting point for a tuning run.  The B group has never
been optimized -- only seeded -- so it is the obvious first target.


Where the model numbers live
----------------------------
Everything xadpcm.cpp used to hard-code as a #define or a bare literal, and
that is SAFE TO SWEEP, is now in xad-G0.idx alongside the rest of the tunables:

  ST_SCALE  WSH  PRED_DMAX  PRED_SHMAX  SGN_MAXNB
  LIT_CTX_TAILBITS  LIT_CTX_NIBBLE  MSMAP  MSCONF  HJ_BITS

MSMAP and MSCONF were #if blocks and are now runtime branches, so opt.pl can
actually move them.  The tuning build goes from 123 maps / 1639 search bits to
133 / 1706.

What stays in the .cpp is the arithmetic contract: P_BITS, CTR_BITS, CTR_NRATE,
MIX_NIN, MIX_MAXIN, PRED_NMAX, HB, SGN_CBITS, SGN_NMODEL, LIT_W, LIT_NSTREAM,
RC_TOP, RC_PRIME, MAX_CHANS, MS_MAXCOEF, MS_DLOG_SUB/E0/EMAX, MIX_LRSH.  These
size arrays, name streams, or fix the fixed-point convention -- sweeping them
does not tune the model, it breaks it, so they are deliberately out of opt.pl's
reach.  MIX_LRSH is there for a second reason too: it only shifts the exponent
window that MLR already spans, so as a tunable it would be redundant with MLR.

MS_DLOG_SUB is the one that looks tunable and is not: it sets how many input
positions C0's Md pattern has to cover, so moving it silently invalidates that
pattern's length.  If you ever want to sweep it, relength Md in the same edit.

Also format constants, and not parameters at all: the IMA step table, the MS
adaptation table, RIFF chunk offsets, the MS iDelta floor of 16.  Member
initialisers that init() overwrites are zeroed, so no number left in the file
reads as a model choice.

Verified byte-identical after the move, on both parameter sets:
  IDX  Cinematic_690 362,766      IDX1  Cinematic_690 360,999
                                        music_00010   415,067


Format-specific parameters are in their own idx files
-----------------------------------------------------
Three of the tunables and two of the three context quantizers only ever run on
one format, so they no longer sit in the shared files:

  IDX/xad-I0.idx   IMA only -- the step-index quantizer (Ib)          88 bits
  IDX/xad-M0.idx   MS only  -- the iDelta quantizer (Md), plus
                   MSMAP, MSCONF, MSDSH                               80 bits
  IDX/xad-C0.idx   shared   -- the match-length quantizer (Ml)        24 bits
  IDX/xad-G0.idx   shared   -- everything else                      1514 bits

The point is not tidiness.  A tuning run on an MS-only corpus (nz_wins is one)
previously had 88 bits of IMA quantizer in its search space with no IMA file to
score them against, so opt.pl was free to drift them anywhere; whatever it
landed on was noise that shipped as if it were tuned.  Freezing a whole file is
now one edit -- put `!` on its Index line, or just leave that idx out of the
build for that run.

What is still SHARED but arguably should not be
-----------------------------------------------
Everything in G0 -- counter rate schedules, mixer, APM, both NLMS cascades --
is one set of numbers serving both formats, and the two formats are measurably
different.  Per-tree-position cost on the same audio:

  MS   0.490 / 0.338 / 0.621 / 0.799   (MSB..LSB)
  IMA  0.376 / 0.517 / 0.654 / 0.849

MS bit 1 is cheap because the residual is peaked at zero; IMA's is not.  A
single tuned G0 is therefore a compromise between the two.  Cloning G0 per
format would remove the compromise, but it doubles the search space and there
is no mixed corpus to justify it yet, so it is left alone deliberately rather
than by oversight.
