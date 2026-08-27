Shape benchmarks for `rc_decoder_opencl_plan_v1.md`.

Neither is a decoder: both run arbitrary data through the exact dependency
chain and instruction mix of a decoder step, which is what the plan's cost
model is built on. Build and run:

    g++ -O3 -march=native -o rc_vecdec_shape  rc_vecdec_shape.cpp
    g++ -O3 -march=native -o rc_specdec_shape rc_specdec_shape.cpp
    ./rc_vecdec_shape  <GHz>      # 16-lane AVX-512 step, six layouts
    ./rc_specdec_shape <GHz>      # serial vs depth-4 speculative model chain

The clock argument is only used to turn seconds into clk/bit; pass the real
sustained clock of the machine. `rc_vecdec_shape` needs AVX-512F/BW/DQ/VL.
Do not build these with clang: it dead-codes two of the variants.

`rc_decoder_proto.patch` is the working prototype the plan's sections 4 and 5.4
were measured with: `RC_STRIPE`, `RC_DECILV`, `RC_DECREFILL` and `RC_FOLDP`.
All four default to off and the default build's output is byte-identical to the
tree without the patch. It is kept as a patch rather than applied because it is
a format fork behind a build-time constant -- see section 8 of the plan.

    git apply misc/bench/rc_decoder_proto.patch
    ./build.sh -DRC_STRIPE=2 -DRC_DECILV=2 -DRC_DECREFILL=1
    ./t.sh "-DRC_STRIPE=2 -DRC_DECILV=2 -DRC_DECREFILL=1"
