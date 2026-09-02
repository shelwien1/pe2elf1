// weights_io: loader for cpp_infer/data/weights.bin (SPEC.md section 4)
#pragma once

namespace fx2 {

enum WDtype : uint8_t { DT_I8 = 0, DT_BF16 = 1, DT_F32 = 2, DT_I32 = 3 };

struct WTensor {
  uint8_t dtype = 0;
  std::vector<uint32_t> shape;
  std::vector<uint8_t> data;
  size_t numel = 0;

  const int8_t* i8() const { return reinterpret_cast<const int8_t*>(data.data()); }
  const uint16_t* bf16_bits() const {
    return reinterpret_cast<const uint16_t*>(data.data());
  }
  const float* f32() const { return reinterpret_cast<const float*>(data.data()); }
  const int32_t* i32() const {
    return reinterpret_cast<const int32_t*>(data.data());
  }
};

// fp32 value of raw bfloat16 bits (bits << 16 reinterpreted as float)
inline float bf16_to_f32(uint16_t bits) {
  union {
    uint32_t u;
    float f;
  } v;
  v.u = static_cast<uint32_t>(bits) << 16;
  return v.f;
}

// Number of rope.sin / rope.cos rows load_compressed() materializes, or 0
// for all 131072 of them.  The tables are recomputed from rope.inv_freq at
// load time (they are not stored in the file), so generating only the
// positions the caller can actually reach saves ~0.2 s and 32 MB of RAM.
// A caller that sets this MUST handle positions past the shortened table
// (model_opt.cpp falls back to libm sin/cos there).
extern size_t g_rope_rows_limit;

struct WeightsFile {
  std::unordered_map<std::string, WTensor> tensors;
  // the tensors in the order the file held them (the loaders fill this in);
  // write_weights_v2 in this order reproduces a v2 file byte for byte
  std::vector<std::string> order;

  // parses the whole file; aborts with a message on any format error
  static WeightsFile load(const char* path);

  // parses a losslessly compressed file (magic FX2TFWC1/FX2TFWC2, written by
  // pysrc/weights_compress.py); yields tensors bit-identical to load() on the
  // matching uncompressed file (weights_io_compressed.cpp)
  static WeightsFile load_compressed(const char* path);

  // builds the same tensor set in memory with freshly initialized values
  // instead of reading a file (weights_init.cpp).  Deterministic in `seed`:
  // an encoder and a decoder that pass the same seed get identical weights.
  // Honours g_rope_rows_limit like the file loaders do.
  static WeightsFile random(uint64_t seed);

  bool has(const std::string& name) const { return tensors.count(name) != 0; }
  const WTensor& get(const std::string& name) const;
  // get + validate dtype and exact shape
  const WTensor& get(const std::string& name, uint8_t dtype,
                     std::initializer_list<uint32_t> shape) const;
};

}  // namespace fx2
