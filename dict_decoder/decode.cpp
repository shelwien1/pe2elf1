/*
 * decode - self-contained C++ decoder for the NNCP-style dictionary
 * preprocessor format produced by Fabrice Bellard's "preprocess" tool
 * (an LZW-like scheme over a 16-bit alphabet).  It reconstructs the
 * original file from a dictionary ("dict") and a preprocessed stream of
 * big-endian 16-bit symbols ("book1p").
 *
 * Equivalent to:   preprocess d <dict> <book1p> <out>
 *
 * Everything lives in this one translation unit: the DictDecoder class
 * (all methods defined inline in the class body) and the main() driver.
 * It is a bit-exact reimplementation of the decode path of preprocess.c.
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
 */
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

class DictDecoder {
public:
    DictDecoder() { reset(); }

    /* Load the dictionary from the textual "dict" file produced by
       `preprocess c ...`.  Throws std::runtime_error on failure. */
    explicit DictDecoder(const char *dict_filename)
    {
        reset();
        load_dict(dict_filename);
    }

    /* (Re)load the dictionary from a file.  Returns the number of entries
       (i.e. the number of valid symbol values). */
    size_t load_dict(const char *dict_filename)
    {
        FILE *f = fopen(dict_filename, "rb");
        if (!f)
            throw std::runtime_error(std::string("cannot open dictionary: ") +
                                     dict_filename);
        fseek(f, 0, SEEK_END);
        long n = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> data(n > 0 ? (size_t)n : 0);
        if (n > 0 && fread(data.data(), 1, (size_t)n, f) != (size_t)n) {
            fclose(f);
            throw std::runtime_error("short read on dictionary");
        }
        fclose(f);
        return load_dict_mem(data.data(), data.size());
    }

    /* Same, but parse a "dict" file image already held in memory. */
    size_t load_dict_mem(const uint8_t *data, size_t len)
    {
        /* Parse the textual dictionary exactly like word_load() in
           preprocess.c:
             - entries are separated by a raw '\n' (0x0a)
             - "\n"  (backslash + 'n')  decodes to a newline byte
             - "\\"  (backslash + '\\') decodes to a backslash byte
             - empty entries are skipped (so symbol N maps to the N-th
               non-empty line, matching the reference loader)
           A trailing entry not terminated by '\n' is dropped, as in C. */
        words_.clear();
        std::string cur;
        for (size_t i = 0; i < len; ++i) {
            uint8_t c = data[i];
            if (c == '\n') {
                if (!cur.empty())
                    words_.push_back(cur);
                cur.clear();
            } else if (c == '\\') {
                if (++i >= len)
                    break;              /* truncated escape: stop, as in C */
                uint8_t e = data[i];
                if (e == 'n')
                    cur.push_back('\n');
                else if (e == '\\')
                    cur.push_back('\\');
                else
                    throw std::runtime_error("invalid escape in dictionary");
            } else {
                cur.push_back((char)c);
            }
        }
        return words_.size();
    }

    /* Unpack one 16-bit symbol, appending the resulting 8-bit bytes to
       'out' (out is grown, never shrunk).  This is the core method: it
       turns one input symbol into the bytes that can be written to the
       output file.  Throws std::out_of_range if 'sym' is not a valid
       dictionary index. */
    void unpack(uint16_t sym, std::vector<uint8_t> &out)
    {
        if (sym >= words_.size())
            throw std::out_of_range("invalid symbol index");
        const std::string &w = words_[sym];
        for (size_t i = 0; i < w.size(); ++i) {
            /* dictionary bytes are unsigned (0..255); control codes 1..4
               are handled by the state machine, everything else may be
               uppercased or suppressed depending on the pending state. */
            int b = case_space_decode((unsigned char)w[i]);
            if (b >= 0)
                out.push_back((uint8_t)b);
        }
    }

    /* Convenience: decode a whole big-endian 16-bit symbol stream
       (e.g. the contents of "book1p") into 'out'.  A trailing odd byte,
       if any, is ignored, exactly as the reference tool does. */
    void decode_stream(const uint8_t *be16, size_t nbytes,
                       std::vector<uint8_t> &out)
    {
        size_t n = nbytes & ~(size_t)1;     /* ignore a trailing odd byte */
        for (size_t i = 0; i < n; i += 2) {
            uint16_t sym = (uint16_t(be16[i]) << 8) | be16[i + 1];
            unpack(sym, out);
        }
    }

    /* Reset the case/space state machine to its initial value so the same
       dictionary can decode another independent stream. */
    void reset()
    {
        ch_type_    = 0;
        has_space_  = true;
        ch_type1_   = 0;
        has_escape_ = false;
    }

    /* Number of dictionary entries; valid symbols are 0 .. symbol_count()-1. */
    size_t symbol_count() const { return words_.size(); }

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

    /* words_[sym] = raw expansion bytes of that symbol (before case/space). */
    std::vector<std::string> words_;

    /* case/space machine state (mirror of CaseSpaceDecodeState). */
    bool has_space_;
    int  ch_type_;
    int  ch_type1_;
    bool has_escape_;
};

/****************************************************************/

static const size_t FLUSH_AT = 1u << 20;   /* flush the buffer past 1 MiB */

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <dict> <book1p> <out>\n", argv[0]);
        return 1;
    }
    const char *dict_file = argv[1];
    const char *in_file   = argv[2];
    const char *out_file  = argv[3];

    DictDecoder dec(dict_file);

    FILE *fi = fopen(in_file, "rb");
    if (!fi) { perror(in_file); return 1; }
    FILE *fo = fopen(out_file, "wb");
    if (!fo) { perror(out_file); fclose(fi); return 1; }

    std::vector<uint8_t> out;
    out.reserve(FLUSH_AT + 4096);

    uint8_t hdr[2];
    while (fread(hdr, 1, 2, fi) == 2) {
        uint16_t sym = (uint16_t(hdr[0]) << 8) | hdr[1];   /* big-endian */
        dec.unpack(sym, out);
        if (out.size() >= FLUSH_AT) {
            fwrite(out.data(), 1, out.size(), fo);
            out.clear();
        }
    }
    if (!out.empty())
        fwrite(out.data(), 1, out.size(), fo);

    fclose(fi);
    fclose(fo);
    return 0;
}
