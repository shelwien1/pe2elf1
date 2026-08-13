// arc.inc -- the file at each end: one image out, one image in.
//
// This was an archive.  The donor put several images in one file -- its command
// line took a list and appended each to the archive named by `-o` -- and the
// reader walked the members until one failed to parse.  `bmf c in.bmp out`
// writes one image and `bmf d in out.bmp` reads one, so the walk, the append
// point it existed to find, and the member count are all gone.  What is left is
// the handle and the two ways of opening it.

struct BmfArc {
  FILE* fp;
  FILE*bmf_close_archive() {
    FILE* f = fp;
    if( f ) {
      fseek(f, 0, 2);
      fclose(fp);
      return nullptr;
    }
    return f;
  }
};

BmfArc*__bmf_destroy_archive(BmfArc* arc, int8_t do_free) {
  (arc)->bmf_close_archive();
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

BmfArc*__bmf_open_archive(BmfArc* out, char* path, int32_t read_only) {
  // `wb` to write and `rb` to read.  The donor opened `a+b` -- read *and*
  // append -- because it had members already there to walk past; there is one
  // image in a file now, so writing never reads and reading never writes.
  out->fp = fopen(path, read_only ? "rb" : "wb");
  if( !out->fp )
    __exit_402E40(6, path);
  return out;
}
