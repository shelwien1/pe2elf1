# tools

## entropy_probe.patch

The instrumentation behind `../ENTROPY.md`.  It is a measurement harness,
not a codec change: nothing it computes goes into the bitstream, and the
encoder it produces writes exactly the same file as the one it patches.

    cp *.cpp *.inc *.h /tmp/probe/ && cd /tmp/probe
    patch -p0 < tools/entropy_probe.patch
    g++ -O2 -std=c++17 -march=native -DNDEBUG -w -DMRP_OPENCL \
        mrpc.cpp mrpc_lib.cpp -lOpenCL -o mrpc-probe

    MRPC_MIX=1  ./mrpc-probe c image.bmp /dev/null   # the report in ENTROPY.md
    MRPC_DUMP=d.bin ./mrpc-probe c image.bmp /dev/null  # one 16-byte record
                                                       # per coded symbol

`MRPC_MIX` charges each alternative model on exactly the symbols the real
coder saw, inside `CodeImage`, after the optimisation has finished:

  * the rangecoder's own loss, from the byte positions it moved between;
  * the same model with its pdf left unquantised, which is what the
    `2^20` frequency tables cost;
  * a Bayesian mixture over neighbouring sigmas, and over every shape at
    the chosen group -- ready-made experts, nothing learned, nothing sent;
  * a 32-band multiplicative correction (PAQ's APM in the shape this
    alphabet wants) on two contexts;
  * a full adaptive histogram over the 511 residual positions, kept in a
    Fenwick tree and renormalised over the current 256-wide window, mixed
    with the static pmf at six fixed weights and at an adapted one.

Every alternative is decodable: the counts, weights and corrections depend
only on symbols already coded.

`MRPC_DUMP` writes `{cl, k, gr, sub, u, base, r, bits}` per symbol for
offline work.

## border_probe.patch

The border rules of `ALGORITHM.md` §2, made switchable at run time, plus a
probe that costs the same image under every rule **with the model held
fixed**.

    MRPC_BORDER=n        use rule n for the whole encode (default 0)
    MRPC_BORDER_PROBE=1  after the search, re-fill the borders under every
                         rule in turn, re-predict, and print CalcCost for each

The rule number is two overlapping things.  Bits 1/2/4 pick *where the border
cell is copied from*, and compose:

    1  mirror the left and right edges, instead of replicating them:
         (y,-j) := pixel(y-1, j)          (row 0 has no source: value 128, error 0)
         (y,W-1+j) := pixel(y, W-1-j)
       it has to come from row y-1 on the left: cell (y,-j) is read by pixel
       (y,0), the first of the row, so nothing on row y is coded yet
    2  shear the top border instead of repeating one column:
         (-j,x) := pixel(0, x-PADR-j)
       a vertical mirror is not available -- cell (-1,x) is first read by
       pixel (0,x-3) through tap (-1,3), which is coded before (0,x-1)
    4  point-reflect the right edge, extrapolating away from it:
         (y,W-1+j) := 2*pixel(y,W-1) - pixel(y,W-1-j), clamped
       (the left edge has no point-reflection variant; 4 acts on the right only)

Values 8..11 and 13..15 instead replace what is *written* into a cell, and are
matched exactly rather than by bit, so they do not compose with each other or
with 1/2/4.  8..11 set the sample; 13..15 set its entry in the `errB` activity
plane, leaving the sample replicated:

    8   0                       every border cell black
    9   128                     flat mid-grey
    10  e + (128-e)*j/(span+1)  linear decay from the edge value to mid-grey
    11  e + (m_k-e)*j/(span+1)  linear decay to that component's plane mean
    13  0                       "off the edge is perfectly smooth"
    14  q - q*j/(span+1)        activity decaying to zero away from the edge
    15  2*q                     "off the edge is twice as active"

`e`/`q` are the edge cell's sample and error, `j` the distance out, `span` the
pad width on that side (`PADL`, `PADR` or `PADT`).

So the useful values are:

    0   replication, as shipped:  (y,-j)=(y-1,0)  (y,W-1+j)=(y,W-1)  (-j,x)=(0,x-PADR-1)
    1   mirror left and right
    2   sheared top
    3   1 and 2
    5   mirror left, point-reflect right
    8   all zero
    9   flat mid-grey
    10  decay to mid-grey
    11  decay to the plane mean
    12  point-reflect right only (identical to 4 -- bit 8 falls through the
        exact-match switch)
    13  border errors zeroed
    14  border errors halved
    15  border errors doubled

`MRPC_BORDER_PROBE` walks 0,1,2,3,5,8,9,10,11,12,13,14,15.  All twelve
alternatives cost more than replication, in proportion to how far the fill
departs from the edge value; `../MODEL-IMPROVEMENTS.md` §10 has the table and
the reason (about half the taps of a column-0 pixel read outside, and the
coefficients carry a DC gain of 1 they cannot re-fit per column).

`MRPC_BORDER_PROBE` is the useful part, and not only for borders.  mrpc's
search is chaotic -- output swings 10-30% between adjacent class counts, and
a compiler inlining difference moves it 0.12% -- so a change worth a few
tenths of a percent cannot be measured by encoding twice and comparing sizes;
the noise is two orders of magnitude larger than the signal.  Holding the
model fixed and re-costing the *same* pixels removes the search from the
measurement entirely.

The one bias it has: the model was optimised under whichever rule was in
force, so the others are at a disadvantage.  Run it under two different rules
and compare the diagonal -- if A beats B under both A's model and B's, the
result is real.
