
enum {
  probN = 128, 
  max_trail_FFs=2, //513,
  max_quants_log = 23,
  max_quants = (1<<max_quants_log), // <2^N to be able to store the value to *_log bits
  quantbufsize = (1<<max_quants_log),
  fast_quants = max_quants
};

// Block header, and the only thing the coder stage still exports to file
// scope: the per-class chunk pointer table that model_dec() reads bits
// through.  rle_get() builds it at the head of qbuf, model_dec() copies it
// out.  Everything else that used to be `struct rle_class' now lives inside
// Model (model_sort/model_freq/model_rle1/model_rle_get/model_rle_put are
// included as members there).
struct rle_state {
  byte* restrict ptr[probN];  // RLE chunk pointers for decoding
};
