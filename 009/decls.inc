// decls.inc -- the structs, and the free functions, declared before any of it is.
//
// This is what lets the include list in bmf.cpp be a reading order instead of a
// dependency order: a body can call anything, wherever it ends up, and a class
// can define its methods in its own body without waiting for what they call.
// Without it the file has exactly one legal order -- the one the decompiler
// emitted -- and every one of these units would have to sit where its callers
// put it.  Deleting this file is not a thought experiment: it was tried, and
// the answer is 90 errors, the first three of them struct names.
//
// **This is not codec.inc, and the two do not overlap.**  A fair question once
// `BMFCodec` existed is whether this file still has a job, since most of what
// it used to declare is now a method declared inside the class.  It does, and
// the split is exactly:
//
//   * codec.inc declares `BMFCodec`'s *methods*, inside the class, where C++
//     requires them and where a method may be defined in any later file;
//   * this declares the twenty-two *structs* and the twenty-six *free*
//     functions -- the block helpers, the BMP reader and writer, the file
//     layer -- none of which belong to a class and none of which C++ will let
//     a caller reach before it has seen a declaration.
//
// The header used to open "every function this program defines", which stopped
// being true the round the codec became a class and stayed on the page for
// three more.
//
// Six declarations here had gone dead in the same three rounds: `alt_init_tables`,
// `rc_decode_flat`, `update_binary_pair`, `reduce_alphabet` and the two
// `alt_model_p1_d8_*` wrappers all became methods, and the line declaring the
// free function stayed.  That compiles indefinitely -- a declaration with no
// definition is legal until something calls it -- so nothing said a word, while
// the file went on telling a reader there was a free `reduce_alphabet` in this
// program.  `tools/deadcheck.py` has the check now.

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
static inline void init_byte_lists(BMFState* cx, SymList* list, uint32_t nbytes);
static void free_sym_lists(SymList* lists);
static inline SymListBlock*sym_list_block(SymList* list);
static inline uint32_t sym_list_count(const SymList* list);
static inline FreqPair*bit_tree(BMFState* cx, uint16_t* freq, int32_t lvl);
static inline int32_t p2_pred(int32_t weighted, int32_t rate);
static inline int16_t p2_bump(BMFState* cx, int32_t w2, int32_t err, int32_t shift);
static inline void p2_update(BMFState* cx, P2Count* p, int32_t res, int32_t shift);
// The default argument lives here and only here, which is where C++ requires
// it: a default given on the declaration cannot be repeated on the definition
// in `alt_p2_block.inc`.  Worth saying out loud because the definition reads
// like a three-argument function and three of its four call sites pass two, so
// a reader who finds the definition first will look for the 2 and not find it.
// Takes no state: it spells the bump out rather than calling `p2_bump`, which
// is the one of these four helpers that does not read the deadzone.
static inline void p2_nudge(P2Count* p, int32_t res, int32_t shift = 2);
static inline uint8_t med_predict(int32_t west, int32_t north, int32_t northwest);
static inline void p2_rescale(P2Freq* r);
static inline void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num);
void bmf_close_file(BmfFile* arc);
CounterNode*init_counter_node(CounterNode* node);
// The templates the merged encode/decode pairs are instantiated from, declared
// here for the same reason as everything else in this file: a caller that is
// itself one of these templates -- P2Freq::code_three_way is -- can sit in a file
// the definition comes after.
// `code_symbol_tree` is a `BMFState` method now and is declared there; the six
// that had to be, and why, are named in state.inc.

void alt_p1_encode_symbol(BMFState* cx, CounterNode* node, int32_t ctx, int32_t sym);
int32_t alt_p1_decode_symbol(BMFState* cx, CounterNode* node, int32_t ctx);
static inline bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym);
BmfImage*alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed);
int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle);
static inline int32_t bmf_pixels(const BmfImage* img);
uint8_t*unpredict_med(uint8_t* pixels, int32_t width, int32_t height);
int32_t estimate_cost(const int32_t* bin, int32_t n);
BmfImage* read_bmp(char* path);
static uint32_t bytes_left(FILE* fp);
BmfFile*bmf_open_file(BmfFile* out, char* path, int32_t read_only);
