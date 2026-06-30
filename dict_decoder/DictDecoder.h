/*
 * DictDecoder - C++ decoder for the NNCP-style dictionary preprocessor
 * format produced by Fabrice Bellard's "preprocess" tool (an LZW-like
 * scheme over a 16-bit alphabet).
 *
 * A compressed stream ("book1p") is a sequence of big-endian 16-bit
 * symbols.  Each symbol is an index into a dictionary ("dict" file).
 * Decoding happens in two stages:
 *
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
 * This is a clean reimplementation of the 'd' (decode) path of
 * preprocess.c and is bit-exact with it.
 */
#ifndef DICT_DECODER_H
#define DICT_DECODER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class DictDecoder {
public:
    DictDecoder();

    /* Load the dictionary from the textual "dict" file produced by
       `preprocess c ...`.  Throws std::runtime_error on failure. */
    explicit DictDecoder(const char *dict_filename);

    /* Load / replace the dictionary from a file.  Returns the number of
       entries (i.e. the number of valid symbol values). */
    size_t load_dict(const char *dict_filename);

    /* Same, but parse a "dict" file image already held in memory. */
    size_t load_dict_mem(const uint8_t *data, size_t len);

    /* Unpack one 16-bit symbol, appending the resulting 8-bit bytes to
       'out' (out is grown, never shrunk).  This is the core method: it
       turns one input symbol into the bytes that can be written to the
       output file.  Throws std::out_of_range if 'sym' is not a valid
       dictionary index. */
    void unpack(uint16_t sym, std::vector<uint8_t> &out);

    /* Convenience: decode a whole big-endian 16-bit symbol stream
       (e.g. the contents of "book1p") into 'out'.  A trailing odd byte,
       if any, is ignored, exactly as the reference tool does. */
    void decode_stream(const uint8_t *be16, size_t nbytes,
                       std::vector<uint8_t> &out);

    /* Reset the case/space state machine to its initial value so the same
       dictionary can decode another independent stream. */
    void reset();

    /* Number of dictionary entries; valid symbols are 0 .. symbol_count()-1. */
    size_t symbol_count() const { return words_.size(); }

private:
    /* One dictionary byte in -> output byte (0..255), or -1 for "no output".
       Mirror of case_space_decode() in preprocess.c. */
    int case_space_decode(int c);

    /* words_[sym] = raw expansion bytes of that symbol (before case/space). */
    std::vector<std::string> words_;

    /* case/space machine state (mirror of CaseSpaceDecodeState). */
    bool has_space_;
    int  ch_type_;
    int  ch_type1_;
    bool has_escape_;
};

#endif /* DICT_DECODER_H */
