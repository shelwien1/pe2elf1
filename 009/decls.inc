// decls.inc -- every function this program defines, declared before any of it is.
//
// This is what lets the include list in bmf.cpp be a reading order instead of a
// dependency order: a body can call anything, wherever it ends up, and a class
// can define its methods in its own body without waiting for what they call.
// Without it the file has exactly one legal order -- the one the decompiler
// emitted -- and every one of these units would have to sit where its callers
// put it.

// Sixteen parameters here used to be named `unread*` or `unused*`, eight of
// them `unread_flag`, and this note used to say why they stayed: that the
// signatures match BMF.exe's and `tools/addrmap.py` keys on that, so dropping
// one would break the correspondence between a name here and an address there.
//
// That was checked and it is not true.  `addrmap.py`'s `DEF` pattern captures
// the identifier before the `(` and stops; it never reads a parameter list.
// The map survives the drop intact, so the trade the note described -- one
// broken correspondence against a saved token -- was not the trade on offer.
//
// So they are gone, by `tools/deadparam.py`, which had to iterate: four of the
// sixteen were not unread but *forwarded*, and a single pass over the list
// removes the leaves and leaves the chain above them looking load-bearing.
//
// The `__` prefix is gone as well, and the reason given for keeping it -- that
// `deadcheck.py`, `unnamed.py` and `addrmap.py` all match on it -- was true of
// the tools and not of the names.  `addrmap.txt` never stored the prefix, so
// the map's keys did not move; `addrmap.py`'s `DEF` pattern still requires
// `__name(`, because what it reads is the donor file's history and that is
// where the prefixed spelling lives.  Only `deadcheck.py`'s pin list and
// `abpair.py`'s pair table named a body directly, and both are one edit.

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
struct NbRow;
struct BmfFile;
struct BmfImage;
struct BmpHeader;

static inline void free_sym_entries(SymList* list, uint32_t n);
static void free_sym_lists(SymList* lists);
static inline SymListBlock*sym_list_block(SymList* list);
static inline uint32_t sym_list_count(const SymList* list);
static inline FreqPair*bit_tree(uint16_t* freq, int32_t lvl);
static inline int32_t p2_pred(int32_t weighted, int32_t rate);
static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift);
static inline void p2_update(P2Count* p, int32_t res, int32_t shift);
static inline void p2_nudge(P2Count* p, int32_t res);
static inline uint8_t med_predict(int32_t west, int32_t north, int32_t northwest);
static inline void p2_rescale(P2Freq* r);
static inline void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num);
BmfFile*bmf_close_file(BmfFile* arc, int8_t do_free);
void expand_predictor_mode0(int32_t i);
uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height);
uint32_t alt_init_tables(uint8_t* fold, int8_t* unfold);
uint32_t rc_decode_flat(uint32_t tot);
CounterNode*init_counter_node(CounterNode* node);
// The templates the merged encode/decode pairs are instantiated from, declared
// here for the same reason as everything else in this file: a caller that is
// itself one of these templates -- P2Freq::code_three_way is -- can sit in a file
// the definition comes after.
template<int32_t f_DEC> int32_t code_symbol_tree(uint16_t* freq, int32_t sym);

void encode_symbol_tree(uint16_t* freq, int32_t sym);
void alt_p1_encode_symbol(CounterNode* node, int32_t ctx, int32_t sym);
int32_t decode_symbol_tree(uint16_t* freq);
int32_t alt_p1_decode_symbol(CounterNode* node, int32_t ctx);
void rc_end_decode();
void rc_end_encode();
static inline bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym);
int32_t update_binary_pair(uint16_t* _this, int32_t symbol);
uint16_t*rc_begin_encode();
void alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
BmfImage*alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed);
int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle);
static inline int32_t bmf_pixels(const BmfImage* img);
void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane);
void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane);
void rc_begin_decode();
uint8_t*unpredict_med(uint8_t* pixels, int32_t width, int32_t height);
int32_t estimate_cost(const int32_t* bin, int32_t n);
void alt_model_p1_d8_decode(uint8_t* out, int32_t i, int32_t height);
int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out);
void reduce_alphabet(ModelBlock* blk, uint8_t* src);
int32_t cost_candidate(BmfImage* img, int32_t cand, PlaneDesc* pd, uint32_t* costs);
int32_t choose_plane_coding(BmfImage* img);
BmfImage* read_bmp(char* path);
int32_t decode_symbol_list(SymList* syms);
int32_t alt_model_p1_encode(uint16_t* hdr, uint8_t* src);
void alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height);
int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out);
void unmodel_plane(BmfImage* p_i, uint8_t* out);
void alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out);
int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2);
void model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw);
void model_planes(BmfImage* img, uint8_t* pixels, int32_t plane);
void transform_planes(BmfImage* p_i);
static uint32_t bytes_left(FILE* fp);
static uint32_t packer_word();
static void packer_reset();
static void pack_bits(uint32_t word, int32_t n);
static uint32_t unpack_bits(int32_t n);
BmfImage* expand_image(BmfFile* arc_in, CodedTail** p_coded_buf);
uint32_t search_filter(BmfImage* img, int8_t mode);
BmfFile*bmf_open_file(BmfFile* out, char* path, int32_t read_only);
int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk);
