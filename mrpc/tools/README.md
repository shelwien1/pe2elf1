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
