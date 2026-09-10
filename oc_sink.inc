/*  The sink: where the walk hands each value it finds.

    io.inc, link_walk.inc and codec.inc take an Ogg Vorbis stream apart and
    put it back, and at every point where a value crosses -- a page field, a
    header record, a floor post, a residue class, a digit -- they hand it to
    `sink_t`, a type the including file names before io.inc is read:

        typedef oc_tsv   sink_t;    balrogg wrote the value as a record; gone
        typedef oc_model sink_t;    oggcomp: codes it

    Every method has the shape of io::val: in mode c it is given the value
    and returns it; in mode d it decodes one and returns that.  What differs
    from a record stream is that each method takes the context the walk has
    at that site -- the residue, the pass, the vector, the partition, the
    class, the book -- so a model does not have to walk the structure a
    second time to know what a value means.  OGGCOMP-PLAN.md sec.2 is the
    contract; oc_model.inc is the implementation (balrogg's oc_tsv.inc, the
    record stream, was the other, and left with balrogg).

      link(more)                        link.more; 0 ends the file
      link_begin()                      after a link's setup is dropped
      page(p, cont)                     the page header fields, in p
      spill(n)                          bytes a packet continues onto later pages
      hdr(tag, x, hi)                   a header-packet record; x < hi
      setup_done(v)                     the setup header has been read
      mode(md, nmd), wprev(w), wnext(w) an audio packet's mode and window flags
      floor(k, fno, f, y, used)         channel k's floor: used, then the posts
      residue_begin(rno, r, nz, nch, n) a residue is about to be walked
      cls(rno, j, pc, k, c)             place k of vector j's classword at pc
      part_begin(rno, pass, j, pc, r, bk, bn, c)   a partition is about to be coded
      vec_begin()                       a codebook vector starts
      digit(d)                          one residue digit

    A sink validates what it decodes to the bound the record stream used to
    enforce in tsv::get_u, so io.inc's own checks stay what they were.  */

/*  The largest residue digit either direction will carry.  */
constexpr i32 RS_DIGIT_MAX = 0x7FFFFF;
