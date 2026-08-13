// decls.inc -- every function this program defines, declared before any of it is.
//
// This is what lets the include list in bmf.cpp be a reading order instead of a
// dependency order: a body can call anything, wherever it ends up, and a class
// can define its methods in its own body without waiting for what they call.
// Without it the file has exactly one legal order -- the one the decompiler
// emitted -- and every one of these units would have to sit where its callers
// put it.

struct CtxWeight;
struct CounterNode;
struct P1Ctx;
struct AltP1Block;
struct FreqRec;
struct SymEntry;
struct SymList;
struct SymListBlock;
struct SymPair;
struct FreqPair;
struct BitCtr;
struct PixRec;
struct ModelBlock;
struct P2Count;
struct P2Freq;
struct P2Ctx;
struct AltP2Block;
struct CtxWeights;
struct BmfArc;
struct BmfImage;
struct BmpHeader;

static inline void free_sym_entries(SymList* list, uint32_t n);
static inline SymListBlock*sym_list_block(SymList* list);
static inline uint32_t sym_list_count(const SymList* list);
static inline FreqPair*bit_tree(uint16_t* freq, int32_t lvl);
static inline int32_t p2_pred(int32_t weighted, int32_t rate);
static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift);
BmfArc*__bmf_destroy_archive(BmfArc* arc, int8_t do_free);
void __expand_predictor_mode0(uint8_t* unread_src, int32_t i, int32_t unread_h);
uint32_t __predict_med(uint8_t* pixels, int32_t width, int32_t height);
uint32_t __alt_init_tables(uint8_t* fold, int8_t* unfold);
uint32_t __rc_decode_flat(uint32_t tot);
CounterNode*__init_counter_node(CounterNode* node);
// The templates the merged encode/decode pairs are instantiated from, declared
// here for the same reason as everything else in this file: a caller that is
// itself one of these templates -- P2Freq::code_symbol is -- can sit in a file
// the definition comes after.
template<int f_DEC> int32_t __code_symbol_tree(uint16_t* freq, int32_t sym);

void __encode_symbol_tree(uint16_t* freq, int32_t sym);
void __alt_p1_encode_symbol(uint16_t* freq, int32_t unread_ctx, int32_t ctx, int32_t sym);
int32_t __decode_symbol_tree(uint16_t* freq);
int32_t __alt_p1_decode_symbol(uint16_t* freq, int32_t unread_arg, int32_t ctx);
int32_t __rescale_counter_pair(BitCtr* ctr);
void __rc_end_decode();
void __rc_end_encode();
void**__free_workspace(ModelBlock* blk, int8_t do_free);
static inline bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym);
int32_t __update_binary_pair(uint16_t* _this, int32_t symbol);
uint16_t*__rc_begin_encode();
void __alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
int32_t*__alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed);
int32_t __write_bmp(BmfImage* img, char* path, int32_t want_rle);
void __init_symbol_list(SymList* list, int32_t n_syms, int32_t dense);
static inline int32_t bmf_pixels(const uint8_t* p);
uint8_t*__interleave_plane(uint8_t* img, uint8_t* src, int32_t plane, int8_t unread_flag);
uint8_t*__colour_transform(uint8_t* img, uint8_t* dst, int32_t plane, int8_t unread_flag);
void __rc_begin_decode(int8_t unread_flag);
uint8_t*__unpredict_med(uint8_t* pixels, int32_t width, int32_t height);
int32_t __estimate_cost(uint8_t* bins, int32_t n);
void**__alt_model_p1_d8_decode(int8_t unread_flag, uint8_t* out, int32_t i, int32_t height);
int32_t __alt_model_p1_decode(uint16_t* hdr, uint8_t* out);
int32_t __alt_p2_filter(float (*_this)[4], float (*a2)[4], CtxWeights* w, int32_t mode);
int32_t __alt_p2_context(AltP2Block* blk, AltP2Block* refa, AltP2Block* refb);
void __reduce_alphabet(ModelBlock* blk, int8_t unread_flag, uint8_t* src);
int32_t __cost_candidate(uint8_t* img, int32_t cand, uint8_t* desc, int8_t unread4, int32_t unread5, int32_t unread6, int32_t unread7, uint32_t* costs);
int32_t __choose_plane_coding(BmfImage* img, int32_t unused_h, int8_t unused_c);
int32_t*__read_bmp(char* path);
int32_t __decode_symbol_list(SymList* syms);
ModelBlock*__layout_workspace(ModelBlock* blk, int32_t unread_flag, int32_t img_w, int32_t img_h, int32_t img_depth);
int32_t __alt_model_p1_encode(uint16_t* hdr, uint8_t* src);
uint32_t __alt_p2_model(AltP2Block* blk, int32_t sample_in, uint8_t a4, int32_t resid_in);
void __alt_p2_d8_decode_body(AltP2Block* blk, int8_t unread_flag, uint8_t* out, int32_t width, int32_t height);
void __alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height);
int32_t __alt_model_p2_decode(uint16_t* p_i, uint8_t* out);
void __unmodel_plane(int8_t unread_flag, uint16_t* p_i, uint8_t* out);
void __alt_p2_d8_encode_body(AltP2Block* blk, uint8_t* src, int32_t width, int32_t height, uint8_t* out);
void __alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
int32_t __alt_model_p2_encode(BmfImage* p_i, uint8_t* a2);
void __model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw);
void __model_planes(uint8_t* img, uint8_t* pixels, int32_t plane, int8_t unread_flag);
void __transform_planes(BmfImage* p_i, int8_t unread_flag);
static uint32_t __bytes_left(FILE* fp);
static uint32_t __packer_word();
uint8_t*__expand_image(BmfArc* arc_in, int32_t want_pal, void** p_coded_buf);
uint32_t __search_filter(BmfImage* img, int8_t mode);
BmfArc*__bmf_open_archive(BmfArc* out, char* path, int32_t read_only);
int32_t __compress_image(BmfArc* arc_in, BmfImage* p_i, void* coded_buf);
