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

    0  replication, as shipped:  (y,-j)=(y-1,0)  (y,W-1+j)=(y,W-1)  (-j,x)=(0,x-PADR-1)
    1  mirror:                   (y,-j)=(y-1,j)  (y,W-1+j)=(y,W-1-j)
    2  sheared top:              (-j,x)=(0,x-PADR-j)
    3  1 and 2
    5  point reflection:         (y,-j)=2*(y-1,0)-(y-1,j), clamped; and right likewise

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
