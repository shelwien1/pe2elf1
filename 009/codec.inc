// codec.inc -- the codec itself: the state, the model blocks, and the two things
// a caller wants done with an image.

// **One instance of this is one codec.**  It derives from `BMFState` -- which is
// every mutable thing the models touch -- and adds the three block pools, which
// cannot be in `BMFState` because they hold `AltP1Block`, `AltP2Block` and
// `ModelBlock` by value and each of those holds a `BMFState*`.  The split is
// what lets every block method stay inline: `BMFState` is complete before the
// first of them is parsed, and this is complete before the first thing that
// needs a pool.
//
// Two of these can exist in one process and run in opposite directions at the
// same time.  Nothing below is reached through a file-scope name.
//
// It is large -- the three pools are most of the program's 86 MB -- so it is a
// thing to allocate once and reuse, not to make per image.  `main` keeps one.
// Declared, not defined, because `plane_transform` returns one and
// plane_predict.inc comes after this file.
struct PlaneTransform;
struct PlaneSearch;

struct BMFCodec : BMFState {
  // Four blocks apiece because `plane_count` is at most 4; one `ModelBlock`
  // because nothing nests it.  See the note at `BlockPool` for how those bounds
  // were established.
  BlockPool<AltP1Block, 4> p1_blocks;
  BlockPool<AltP2Block, 4> p2_blocks;
  BlockPool<ModelBlock, 1> model_blocks;

  // Both alternate coders end the same way: every plane's block back to its
  // pool, and the plane count as the answer each returns.  The p2 side had a
  // name for it -- `alt_p2_planes_free` -- and `alt_model_p1` wrote the loop out
  // again, which is the five-line run `dupblock.py` has been carrying at the top
  // of its list.
  //
  // That run was looked at before and left, on the argument that sharing it
  // needed a member-function-pointer template or free-function overloads plumbed
  // across an include boundary, because the two blocks named their release
  // differently.  Neither block owns its release any more -- the pools do -- so
  // the only difference left is which pool, and a pool is an argument.
  //
  // The alloc sides are deliberately *not* shared: p1's block wants a height and
  // p2's does not, so they are two functions rather than one with a dead
  // parameter.
  //
  // Defined here, where everything else in this class is declared, because a
  // member template has to be defined before it is instantiated and both callers
  // come later in the include list.  Declaring it and defining it in whichever
  // of the two files is included first would work, and would break the next time
  // that order moved.
  template<class T, int32_t N>
  int32_t planes_free(BlockPool<T, N>& pool, T** plane) {
    for( int32_t k = 0; k<plane_count; ++k )
      pool.give(plane[k]);
    return plane_count;
  }

  // The two the world outside wants: an image in memory becomes a coded stream,
  // and a coded stream becomes an image.  Everything else here is how.
  void allow_refs_where_present();
  template<int32_t f_DEC>
  int32_t alt_model_p1(BmfImage* hdr, uint8_t* buf);
  int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out);
  int32_t alt_model_p1_encode(BmfImage* hdr, uint8_t* src);
  template<int32_t f_DEC>
  int32_t alt_model_p2(BmfImage* p_i, uint8_t* out);
  template<int32_t f_DEC>
  void alt_model_p2_d8(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
  void alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height);
  void alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
  int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out);
  int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2);
  template<int32_t f_DEC>
  bool alt_model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw);
  AltP1Block* alt_p1_block_alloc(int32_t width, int32_t height);
  int32_t choose_plane_coding(BmfImage* img);
  void clear_flags_on_all();
  template<int32_t f_DEC>
  void code_colour_plane(BmfImage* img, uint8_t* side, int32_t plane);
  template<int32_t f_DEC, typename T>
  void code_field(T& v, int32_t bits, int32_t bias);
  void code_image_body(BmfImage* p_i, BmfImage& hdr);
  template<int32_t f_DEC>
  void code_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw);
  template<int32_t f_DEC>
  void code_plane_descs();
  void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane);
  int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk);
  int32_t cost_candidate(BmfImage* img, int32_t cand, PlaneDesc* pd, uint32_t* costs);
  void deinterleave_plane(const BmfImage* img, uint8_t* dst, int32_t plane);
  void drop_alt_model_from_all();
  bool expand_coded(BmfStream* io, BmfImage* img_at, const BmfImage& hdr);
  BmfImage* expand_image(BmfFile* arc_in, CodedTail** p_coded_buf);
  void interleave_flat(BmfImage* img, const uint8_t* src, int32_t plane);
  void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane);
  void model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw);
  void model_planes(BmfImage* img, uint8_t* pixels, int32_t plane);
  ModelBlock* new_model_block(int32_t img_w, int32_t img_h, int32_t img_depth);
  void offer_to_all(int32_t want);
  void pack_bits(uint32_t word, int32_t n);
  void packer_flush();
  void packer_reset();
  uint32_t packer_word();
  int32_t plane_in_order(int32_t k);
  PlaneTransform plane_transform(int32_t plane);
  uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height);
  void read_plane_descs();
  void reset_descriptors();
  void restore_descriptors(const PlaneDesc* saved);
  void save_descriptors(PlaneDesc* saved);
  uint32_t search_filter(BmfImage* img);
  void stream_open(uint32_t data_size);
  int32_t transform_cost(BmfImage* tile_img);
  void transform_planes(BmfImage* p_i);
  void unmodel_described(BmfImage* img_at, uint8_t flags);
  void unmodel_plane(BmfImage* p_i, uint8_t* out);
  void unmodel_planes_apart(BmfImage* img_at, uint8_t* plane_buf);
  void unmodel_planes_together(BmfImage* img_at, uint8_t* plane_buf);
  uint32_t unpack_bits(int32_t n);
  void write_plane_descs();
  int32_t weight_pair_cost(const BmfImage* img, const uint8_t* plane0,
                           int32_t plane_a, int32_t plane_b,
                           int32_t w4, int32_t w8);
  PlaneSearch search_planes(BmfImage* tile_img, uint8_t* tile_buf,
                            uint32_t* plane_cost);
  void choose_alpha_plane(BmfImage* img, int32_t (&hists)[8*1024],
                          const uint8_t* data_end, int32_t row_stride);
  template<int32_t f_DEC>
  void alt_model_p1_d8(uint8_t* src, int32_t width, int32_t height, uint8_t* out);
  void alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
  void alt_model_p1_d8_decode(uint8_t* out, int32_t i, int32_t height);
  void alt_p2_planes_alloc(AltP2Block** plane, int32_t width);

  // The two an outside caller wants, and the reason this is a class: an image
  // in memory in, a coded stream in memory out, and back again.  Neither
  // touches a path -- `main` still does the file I/O, and `read_bmp` and
  // `write_bmp` are still the program's rather than the codec's.
  //
  // **The tail block is an argument and not the `coded_block` member**, which
  // is what `main` uses and what these two were first written to use.  It
  // cannot be: both begin with `reset()`, so a tail the caller had stored in
  // the member was zeroed before it could be read, and one an expand had
  // produced was zeroed without being freed on the next call.  Passed in and
  // passed out, neither can happen -- and a caller that wants neither says so
  // by leaving both off.
  uint8_t* compress_to_memory(BmfImage* p_i, size_t* out_len,
                              const CodedTail* tail = nullptr);
  BmfImage* expand_from_memory(const uint8_t* data, size_t n,
                               CodedTail** tail = nullptr);
};
