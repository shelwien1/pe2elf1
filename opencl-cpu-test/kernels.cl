// OpenCL C kernels used by ocltest.
//
// Three kernels, chosen so that every result can be checked exactly (or to a
// tight tolerance) against a plain C++ reference running on the host:
//
//   saxpy      - float math, one work-item per element
//   hash_chain - integer math, compute bound, bit-exact
//   reduce_sum - local memory + barriers, exercises the work-group machinery

// c[i] = a[i] * s + b[i]
__kernel void saxpy(__global const float* a,
                    __global const float* b,
                    const float            s,
                    __global float*        c)
{
    const size_t i = get_global_id(0);
    c[i] = a[i] * s + b[i];
}

// A xorshift64* chain run ROUNDS times per element. Pure integer ops with a
// serial dependency inside each work-item, so the result is bit-exact on any
// conforming device while still being heavy enough to be worth measuring.
#define ROUNDS 64

__kernel void hash_chain(__global const uint* seed,
                         __global ulong*      out)
{
    const size_t i = get_global_id(0);

    ulong x = (ulong)seed[i] * 0x2545F4914F6CDD1DUL + 0x9E3779B97F4A7C15UL;
    for (int r = 0; r < ROUNDS; r++) {
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        x *= 0x2545F4914F6CDD1DUL;
    }
    out[i] = x;
}

// Sum of `n` ulongs. Each work-item accumulates a grid-strided slice, then the
// work-group folds its values through local memory; one partial per group is
// written out and the (short) tail is summed by the host.
__kernel void reduce_sum(__global const ulong* in,
                         const ulong           n,
                         __local ulong*        scratch,
                         __global ulong*       partials)
{
    const size_t lid    = get_local_id(0);
    const size_t lsize  = get_local_size(0);
    const size_t stride = get_global_size(0);

    ulong acc = 0;
    for (size_t i = get_global_id(0); i < n; i += stride)
        acc += in[i];

    scratch[lid] = acc;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (size_t s = lsize / 2; s > 0; s >>= 1) {
        if (lid < s)
            scratch[lid] += scratch[lid + s];
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (lid == 0)
        partials[get_group_id(0)] = scratch[0];
}
