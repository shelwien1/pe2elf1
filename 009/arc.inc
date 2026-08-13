// arc.inc -- the archive: several images in one file.  `bmf c` writes one and
// creates its output; the walk over members that are already there is what
// reading one of several needs, and what appending used to need.

struct BmfArc {
  uint32_t images;
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
  FILE* fp, * fp2;
  const char* mode;
  int32_t seek_rc, live;
  BmfArc* arc = out;
  // `w+b`, not the donor's `a+b`: `bmf c in out` writes `out`, it does not add
  // to it.  Appending is what the original did -- BMF took a list of files and
  // put each one in the archive named by `-o` -- and this program takes one
  // input and one output, where the same command run twice quietly produced a
  // two-member file and a second `-o` of the same name grew it again.
  //
  // Everything below still walks the members that are already there, because
  // *reading* an archive of several is unchanged and the walk is shared.  A
  // truncated file simply has none, which is the case a brand-new output has
  // always taken.
  mode = "w+b";
  if( read_only )
    mode = "rb";
  out->images = 0;
  fp = fopen(path, mode);
  arc->fp = fp;
  if( !fp )
    __exit_402E40(6, path);
  arc->images = 0;
  seek_rc = fseek(fp, 0, 0);
  live = arc->fp!=nullptr;
  if( seek_rc ) {
    if( !live )
      __exit_402E40(3, path);
    return arc;
  }
  if( !live ) {
    __exit_402E40(3, path);
    return arc;
  }
  if( !feof(arc->fp) ) {
    __expand_image(arc, 1, (void**)nullptr);
    fp2 = arc->fp;
    if( !fp2 ) {
      __exit_402E40(3, path);
      return arc;
    }
    if( !feof(arc->fp) ) {
      arc->images = 0;
      fseek(fp2, 0, 0);
      live = arc->fp!=nullptr;
      {
        if( !live )
          __exit_402E40(3, path);
        return arc;
      }
    }
  }
  return arc;
}
