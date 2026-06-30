/*
 * DictDecoder - implementation.  See DictDecoder.h for the format notes.
 *
 * Bit-exact reimplementation of the decode path of preprocess.c:
 *   word_load()         -> load_dict() / load_dict_mem()
 *   case_space_decode() -> case_space_decode()
 *   word_decode()       -> unpack() / decode_stream()
 */
#include "DictDecoder.h"

#include <cstdio>
#include <stdexcept>

namespace {
/* reserved control codes embedded in dictionary words (see preprocess.c) */
const int CH_NO_SPACE    = 1;
const int CH_TO_UPPER    = 2;
const int CH_FIRST_UPPER = 3;
const int CH_ESCAPE      = 4;
}

DictDecoder::DictDecoder()
{
    reset();
}

DictDecoder::DictDecoder(const char *dict_filename)
{
    reset();
    load_dict(dict_filename);
}

void DictDecoder::reset()
{
    ch_type_    = 0;
    has_space_  = true;
    ch_type1_   = 0;
    has_escape_ = false;
}

size_t DictDecoder::load_dict(const char *dict_filename)
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

size_t DictDecoder::load_dict_mem(const uint8_t *data, size_t len)
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
                break;                  /* truncated escape: stop, as in C */
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

int DictDecoder::case_space_decode(int c)
{
    /*
      syntax produced by the encoder:
        CH_NO_SPACE? [CH_TO_UPPER|CH_FIRST_UPPER]? SPACE  word-or-not-word
      CH_ESCAPE precedes a literal reserved code in the stream.
    */
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

void DictDecoder::unpack(uint16_t sym, std::vector<uint8_t> &out)
{
    if (sym >= words_.size())
        throw std::out_of_range("invalid symbol index");
    const std::string &w = words_[sym];
    for (size_t i = 0; i < w.size(); ++i) {
        /* dictionary bytes are unsigned (0..255); control codes 1..4 are
           handled by the state machine, everything else may be uppercased
           or suppressed depending on the pending case/space state. */
        int b = case_space_decode((unsigned char)w[i]);
        if (b >= 0)
            out.push_back((uint8_t)b);
    }
}

void DictDecoder::decode_stream(const uint8_t *be16, size_t nbytes,
                                std::vector<uint8_t> &out)
{
    size_t n = nbytes & ~(size_t)1;     /* ignore a trailing odd byte */
    for (size_t i = 0; i < n; i += 2) {
        uint16_t sym = (uint16_t(be16[i]) << 8) | be16[i + 1];
        unpack(sym, out);
    }
}
