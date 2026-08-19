// file.inc -- the file at each end: one image out, one image in.
//
// This was an archive.  The donor put several images in one file -- its command
// line took a list and appended each to the archive named by `-o` -- and the
// reader walked the members until one failed to parse.  `bmf c in.bmp out`
// writes one image and `bmf d in out.bmp` reads one, so the walk, the append
// point it existed to find, and the member count are all gone.  What is left is
// the handle and the two ways of opening it.

// Each member of a BMF file opens with a four-byte tag: two signature bytes,
// then the version as two ASCII digits.  Two signatures exist -- one introduces
// an image and the other introduces a member `expand_image` skips by the length
// in its eight-byte header -- and this build reads and writes 2.0 only.
//
// The version arithmetic was `((major<<8)-12288)|(minor-48)` tested against
// 512, which is three constants for one idea: 12288 is `'0'<<8`, 48 is `'0'`,
// and 512 is `2<<8`.  It reads the two digits and packs them into a byte each.
//
// The tag is handled as a word at both ends -- the reader `fread`s four bytes
// into a `uint32_t` and masks, and `bmf_tag` builds the same word for the
// writer -- so the signature is the low half on a little-endian host and this
// says nothing useful on any other.  That is the whole program's assumption,
// not this pair's.
enum : uint16_t {
  bmf_sig_image = 0x8A81,
  bmf_sig_other = 0x9081,
};

// `2<<8`, the only version this build accepts.
static constexpr int32_t bmf_version_20 = 2<<8;

static inline int32_t bmf_tag_version(uint32_t tag) {
  return (((uint8_t)(tag>>16)-'0')<<8)|((uint8_t)(tag>>24)-'0');
}

static inline uint32_t bmf_tag(uint16_t sig, char major, char minor) {
  return sig|((uint32_t)(uint8_t)major<<16)|((uint32_t)(uint8_t)minor<<24);
}

struct BmfFile {
  FILE* fp;
  // Both returns were the same constant -- `nullptr` on the branch that
  // closes, and `f` on the branch where `f` is already null -- and the one
  // caller discarded it.  It also left `fp` pointing at a closed `FILE`, which
  // was harmless only because that caller frees the struct immediately.
  // Close the input and give up.  Eleven sites in `image_expand` wrote
  // `fclose(arc->fp); arc->fp = 0; return nullptr;` out, which is eleven places
  // a reader had to check the file was actually closed.
  BmfImage* fail() {
    close();
    return nullptr;
  }

  void close() {
    if( fp ) {
      fseek(fp, 0, SEEK_END);
      fclose(fp);
      fp = nullptr;
    }
  }
};

// Same shape as `AltP2Block::alt_p2_free`: it returned `arc` after `free(arc)`
// and tested a `do_free` its one caller always passes as 1.
void bmf_close_file(BmfFile* arc) {
  arc->close();
  free(arc);
}

static uint32_t bytes_left(FILE* fp) {
  long here = ftell(fp);
  if( here<0||fseek(fp, 0, SEEK_END)!=0 )
    return 0;
  long end = ftell(fp);
  if( end<0||fseek(fp, here, SEEK_SET)!=0 )
    return 0;
  return (uint32_t)(end-here);
}

BmfFile*bmf_open_file(BmfFile* out, char* path, int32_t read_only) {
  // `wb` to write and `rb` to read.  The donor opened `a+b` -- read *and*
  // append -- because it had members already there to walk past; there is one
  // image in a file now, so writing never reads and reading never writes.
  out->fp = fopen(path, read_only ? "rb" : "wb");
  if( !out->fp )
    bmf_fatal(bmf_no_open, path);
  return out;
}
