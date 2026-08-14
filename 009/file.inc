// file.inc -- the file at each end: one image out, one image in.
//
// This was an archive.  The donor put several images in one file -- its command
// line took a list and appended each to the archive named by `-o` -- and the
// reader walked the members until one failed to parse.  `bmf c in.bmp out`
// writes one image and `bmf d in out.bmp` reads one, so the walk, the append
// point it existed to find, and the member count are all gone.  What is left is
// the handle and the two ways of opening it.

struct BmfFile {
  FILE* fp;
  // Both returns were the same constant -- `nullptr` on the branch that
  // closes, and `f` on the branch where `f` is already null -- and the one
  // caller discarded it.  It also left `fp` pointing at a closed `FILE`, which
  // was harmless only because that caller frees the struct immediately.
  // Close the input and give up.  Eleven sites in `image_expand` wrote
  // `fclose(arc->fp); arc->fp = 0; return nullptr;` out, which is eleven places
  // a reader had to check the file was actually closed.
  uint8_t*fail() {
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

BmfFile*__bmf_close_file(BmfFile* arc, int8_t do_free) {
  arc->close();
  if( do_free&1 )
    free(arc);
  return arc;
}

static uint32_t __bytes_left(FILE* fp) {
  long here = ftell(fp);
  if( here<0||fseek(fp, 0, SEEK_END)!=0 )
    return 0;
  long end = ftell(fp);
  if( end<0||fseek(fp, here, SEEK_SET)!=0 )
    return 0;
  return (uint32_t)(end-here);
}

BmfFile*__bmf_open_file(BmfFile* out, char* path, int32_t read_only) {
  // `wb` to write and `rb` to read.  The donor opened `a+b` -- read *and*
  // append -- because it had members already there to walk past; there is one
  // image in a file now, so writing never reads and reading never writes.
  out->fp = fopen(path, read_only ? "rb" : "wb");
  if( !out->fp )
    bmf_fatal(bmf_no_open, path);
  return out;
}
