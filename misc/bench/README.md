Shape benchmarks and prototype patches for `rc_decoder_opencl_plan_v1.md`.

Neither benchmark is a decoder: both run arbitrary data through the exact
dependency chain and instruction mix of a decoder step, which is what the
plan's cost model is built on. Build and run:

    g++ -O3 -march=native -o rc_vecdec_shape  rc_vecdec_shape.cpp
    g++ -O3 -march=native -o rc_specdec_shape rc_specdec_shape.cpp
    ./rc_vecdec_shape  <GHz>      # 16-lane AVX-512 step, seven layouts
    ./rc_specdec_shape <GHz>      # serial vs depth-4 speculative model chain

The clock argument is only used to turn seconds into clk/bit; pass the real
sustained clock of the machine. `rc_vecdec_shape` needs AVX-512F/BW/DQ/VL.
The plan quotes gcc numbers; clang compiles both and preserves every verdict,
but its absolute rows differ by 10-20%, so do not mix compilers in one table.
All variants share one compiled loop, so editing one variant shifts the
absolute numbers of the others -- compare rows only within one build.

`rc_decoder_proto.patch` is the working prototype the plan's sections 4 and
5.4 were measured with: `RC_STRIPE`, `RC_DECILV`, `RC_DECREFILL` and
`RC_FOLDP`. All four default to off and the default build's output is
byte-identical to the tree without the patch. It is kept as a patch rather
than applied because it is a format fork behind a build-time constant -- see
section 8 of the plan.

    git apply misc/bench/rc_decoder_proto.patch
    ./build.sh -DRC_STRIPE=2 -DRC_DECILV=2 -DRC_DECREFILL=1
    ./t.sh "-DRC_STRIPE=2 -DRC_DECILV=2 -DRC_DECREFILL=1"

`rc_decoder_probes.patch` is the `RC_PROBE` harness behind the plan's
sections 1.2, 2 and 9: numbered variants of the decoder's inner loop with
one piece removed or replaced each. Probe builds do not decode correctly by
design (all except `RC_PROBE=12`); they exist to be timed.

The two patches touch the same files and are alternatives -- apply one or
the other, not both.

`rc_splitdec.cpp` + `rc_splitdec.ispc` are the exact fused-vs-split decoder
harness behind `rc_split_ispc_v1.md`: NL interleaved lanes, a serial FSM
model, and the batched-coder split in intrinsics and in ISPC. Unlike the
shape benchmarks, every variant is verified bit-identical against the fused
reference. Needs ISPC (one static binary from github.com/ispc/ispc/releases;
1.24.0 was used) and AVX-512:

    ispc --target=avx512skx-x16 --arch=x86-64 -O2 --pic rc_splitdec.ispc \
         -o rc_splitdec_ispc.o -h rc_splitdec_ispc.h
    g++ -O3 -march=native -I. rc_splitdec.cpp rc_splitdec_ispc.o -o rc_splitdec
    ./rc_splitdec <GHz>          # cold mode (max-entropy)
    ./rc_splitdec <GHz> hot      # hot mode (skewed, no refill)

A variant printing anything but [identical]/[ref] is broken; the timing of a
broken run means nothing.
