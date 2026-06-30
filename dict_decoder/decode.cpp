/*
 * decode - self-contained C++ decoder for the NNCP-style dictionary
 * preprocessor format produced by Fabrice Bellard's "preprocess" tool
 * (an LZW-like scheme over a 16-bit alphabet).  It reconstructs the
 * original file from a dictionary ("dict") and a preprocessed stream of
 * big-endian 16-bit symbols ("book1p").
 *
 * Equivalent to:   preprocess d <dict> <book1p> <out>
 *
 * No STL: like the original preprocessor this is plain C data (malloc'd
 * tables, a fixed global output array), wrapped in one DictDecoder class
 * with its methods defined inline.  It is bit-exact with preprocess.c.
 *
 * Two-stage decode:
 *   1. symbol (uint16) -> dictionary word: a short byte string that may
 *      contain the reserved control codes 1..4 and a leading-space
 *      convention introduced by the encoder.
 *   2. each word byte -> 0 or 1 output bytes through a small "case/space"
 *      state machine that restores the spaces and upper-case letters the
 *      encoder factored out.
 *
 * The case/space machine carries state from one symbol to the next, so a
 * single DictDecoder instance decodes one whole stream; call reset() to
 * start decoding a fresh, independent stream with the same dictionary.
 *
 * Because case/space decoding only ever drops or 1:1-maps bytes, a single
 * symbol expands to at most the length of the longest dictionary word, so
 * one unpack() never produces more than OUT_MAX bytes (see the printed
 * "max out.size()"; it is 20 for this sample).
 */
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

class DictDecoder {
public:
    DictDecoder()
        : buf_(0), word_(0), wlen_(0), nword_(0), max_wlen_(0) { reset(); }

    /* Load the dictionary from the textual "dict" file produced by
       `preprocess c ...`.  Errors are fatal (perror + exit), as in the
       original tool. */
    explicit DictDecoder(const char *dict_filename)
        : buf_(0), word_(0), wlen_(0), nword_(0), max_wlen_(0)
    {
        reset();
        load_dict(dict_filename);
    }

    ~DictDecoder()
    {
        free(buf_);
        free(word_);
        free(wlen_);
    }

    /* (Re)load the dictionary from a file.  Returns the number of entries
       (i.e. the number of valid symbol values). */
    size_t load_dict(const char *dict_filename)
    {
        FILE *f = fopen(dict_filename, "rb");
        if (!f) { perror(dict_filename); exit(1); }
        fseek(f, 0, SEEK_END);
        long n = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (n < 0) { perror(dict_filename); exit(1); }

        free(buf_); free(word_); free(wlen_);
        buf_  = (uint8_t *)malloc((size_t)n > 0 ? (size_t)n : 1);
        if (fread(buf_, 1, (size_t)n, f) != (size_t)n) {
            fprintf(stderr, "short read on dictionary\n");
            exit(1);
        }
        fclose(f);

        /* number of '\n' bytes is an upper bound on the entry count */
        size_t max_entries = 1;
        for (long i = 0; i < n; i++)
            if (buf_[i] == '\n')
                max_entries++;
        word_ = (uint8_t **)malloc(max_entries * sizeof(word_[0]));
        wlen_ = (uint32_t *)malloc(max_entries * sizeof(wlen_[0]));

        /* Parse the textual dictionary exactly like word_load() in
           preprocess.c, unescaping in place (unescaping only shrinks, so
           the write cursor 'w' never passes the read cursor 'i'):
             - entries are separated by a raw '\n' (0x0a)
             - "\n"  (backslash + 'n')  decodes to a newline byte
             - "\\"  (backslash + '\\') decodes to a backslash byte
             - empty entries are skipped (so symbol N is the N-th non-empty
               line); a trailing entry with no '\n' is dropped, as in C. */
        nword_    = 0;
        max_wlen_ = 0;
        size_t w = 0, start = 0;
        long i = 0;
        while (i < n) {
            uint8_t c = buf_[i++];
            if (c == '\n') {
                uint32_t len = (uint32_t)(w - start);
                if (len > 0) {
                    word_[nword_] = buf_ + start;
                    wlen_[nword_] = len;
                    if (len > max_wlen_)
                        max_wlen_ = len;
                    nword_++;
                }
                start = w;
            } else if (c == '\\') {
                if (i >= n)
                    break;                  /* truncated escape: stop, as in C */
                uint8_t e = buf_[i++];
                if (e == 'n')
                    buf_[w++] = '\n';
                else if (e == '\\')
                    buf_[w++] = '\\';
                else {
                    fprintf(stderr, "invalid escape in dictionary\n");
                    exit(1);
                }
            } else {
                buf_[w++] = c;
            }
        }
        return nword_;
    }

    /* Unpack one 16-bit symbol into 'out', returning the number of 8-bit
       bytes written (0 .. max_word_len()).  This is the core method: one
       input symbol -> the bytes ready to be written to the output file.
       'out' must have room for at least max_word_len() bytes.  An invalid
       symbol is fatal, as in the reference tool. */
    int unpack(uint16_t sym, uint8_t *out)
    {
        if (sym >= nword_) {
            fprintf(stderr, "invalid symbol %u\n", (unsigned)sym);
            exit(1);
        }
        const uint8_t *w = word_[sym];
        uint32_t n = wlen_[sym];
        int k = 0;
        for (uint32_t i = 0; i < n; i++) {
            /* dictionary bytes are unsigned (0..255); control codes 1..4
               are handled by the state machine, everything else may be
               uppercased or suppressed depending on the pending state. */
            int b = case_space_decode(w[i]);
            if (b >= 0)
                out[k++] = (uint8_t)b;
        }
        return k;
    }

    /* Reset the case/space state machine so the same dictionary can decode
       another independent stream. */
    void reset()
    {
        ch_type_    = 0;
        has_space_  = true;
        ch_type1_   = 0;
        has_escape_ = false;
    }

    size_t   symbol_count()  const { return nword_; }     /* valid: 0..count-1 */
    uint32_t max_word_len()  const { return max_wlen_; }  /* upper bound of unpack() */

private:
    /* reserved control codes embedded in dictionary words (see preprocess.c) */
    enum {
        CH_NO_SPACE    = 1,
        CH_TO_UPPER    = 2,
        CH_FIRST_UPPER = 3,
        CH_ESCAPE      = 4
    };

    /* One dictionary byte in -> output byte (0..255), or -1 for "no output".
       Mirror of case_space_decode() in preprocess.c.

       syntax produced by the encoder:
         CH_NO_SPACE? [CH_TO_UPPER|CH_FIRST_UPPER]? SPACE  word-or-not-word
       CH_ESCAPE precedes a literal reserved code in the stream. */
    int case_space_decode(int c)
    {
        if (has_escape_) {
            has_escape_ = false;
        } else if (c == CH_TO_UPPER || c == CH_FIRST_UPPER) {
            ch_type_ = c;
            c = -1;
        } else if (c == CH_NO_SPACE) {
            has_space_ = false;
            c = -1;
        } else if (c == CH_ESCAPE) {
            has_escape_ = true;
            c = -1;
        } else if (c == ' ') {
            ch_type1_ = ch_type_;
            ch_type_ = 0;
            if (!has_space_)
                c = -1;
            has_space_ = true;
        } else {
            if (ch_type1_ == CH_TO_UPPER || ch_type1_ == CH_FIRST_UPPER) {
                if (c >= 'a' && c <= 'z')
                    c = c - 'a' + 'A';
                if (ch_type1_ == CH_FIRST_UPPER)
                    ch_type1_ = 0;
            }
            has_space_ = true;
        }
        return c;
    }

    uint8_t  *buf_;     /* malloc'd backing store: all word bytes, unescaped */
    uint8_t **word_;    /* word_[sym] -> first byte of that symbol in buf_   */
    uint32_t *wlen_;    /* wlen_[sym] = number of bytes of that symbol       */
    size_t    nword_;   /* number of dictionary entries                      */
    uint32_t  max_wlen_;/* longest word, == upper bound on unpack() output   */

    /* case/space machine state (mirror of CaseSpaceDecodeState). */
    bool has_space_;
    int  ch_type_;
    int  ch_type1_;
    bool has_escape_;
};

/****************************************************************/

/* A single symbol expands to at most the longest dictionary word; 512
   matches the encoder's own per-word limit (SORT_MAX_LEN in preprocess.c)
   and is far above the 20 bytes this sample actually needs. */
enum { OUT_MAX = 512 };

static uint8_t out[OUT_MAX];   /* plain global output array (no vector) */

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <dict> <book1p> <out>\n", argv[0]);
        return 1;
    }

    DictDecoder dec(argv[1]);
    if (dec.max_word_len() > OUT_MAX) {
        fprintf(stderr, "OUT_MAX too small: longest word is %u bytes\n",
                (unsigned)dec.max_word_len());
        return 1;
    }

    FILE *fi = fopen(argv[2], "rb");
    if (!fi) { perror(argv[2]); return 1; }
    FILE *fo = fopen(argv[3], "wb");
    if (!fo) { perror(argv[3]); fclose(fi); return 1; }

    size_t max_out = 0;          /* maximum out.size() over all unpack() calls */
    uint8_t hdr[2];
    while (fread(hdr, 1, 2, fi) == 2) {
        uint16_t sym = (uint16_t)((hdr[0] << 8) | hdr[1]);   /* big-endian */
        int n = dec.unpack(sym, out);    /* one 16-bit symbol -> out[0..n) */
        if ((size_t)n > max_out)
            max_out = (size_t)n;
        fwrite(out, 1, (size_t)n, fo);
    }

    fclose(fi);
    fclose(fo);

    printf("max out.size() = %zu\n", max_out);
    return 0;
}
