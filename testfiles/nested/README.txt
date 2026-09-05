JPEGs whose thumbnails nest, as deep as a JPEG can nest.

A JPEG can carry a JPEG thumbnail, and that thumbnail is an ordinary JPEG, so it
can carry one of its own.  Nothing in T.81 or in Exif bounds the depth.  What
bounds it is that every level except the outermost has to fit inside its parent's
APP segment, and that length field is 16 bits -- so wrapping the smallest
conforming baseline datastream (~130 bytes) until the next wrap will not fit
reaches 337 levels in 65512 bytes.

  nest1.jpg        1 deep    one Exif thumbnail -- the ordinary case
  nest2.jpg        2         Exif, then JFXX
  nest4.jpg        4         the deepest pjpg parses; its guard must stay quiet
  nest_max.jpg     5         one more, so the guard has to fire
  nest_deep.jpg    8         three levels past the guard, all skipped
  nest_exif5.jpg   5         every level an Exif APP1
  nest_jfxx5.jpg   5         every level a JFXX APP0
  nest_two.jpg     5 x 2     two chains side by side on one image
  nest_limit.jpg   337       as deep as a 16-bit segment length allows

There are exactly two ways into the recursion and these use both, alternating,
so neither of pjpg's two call sites is only ever reached from itself:

  * JFXX APP0 with extension code 0x10, where the rest of the segment is a whole
    JPEG datastream (JFIF 1.02 section 6);
  * Exif APP1 whose IFD says Compression (0x0103) = 6, with the thumbnail found
    through JPEGInterchangeFormat (0x0201) and its length (0x0202).

Every level of every file is a valid, decodable baseline JPEG in its own right --
381 images across the nine files, all of which djpeg opens.  Nothing here is
malformed, so a failure against them is a failure of the recursion and not of
error handling.  The images are flat grey: one all-zero block per MCU, which is
the smallest conforming baseline datastream there is, so the segment-length
budget goes to nesting rather than to pixels.

nest_two.jpg is the one that is not a chain.  Two thumbnails hang off the same
image, so the second chain reuses the parser objects the first one finished with
-- the only place in the recursion that depends on a parser being reusable.

manifest records what each file should be seen to make a tool do -- levels
walked, whether the depth guard fires, thumbnails carved -- so the tests do not
restate it.  `make nesting` reads it, and also regenerates the images into a
scratch directory and compares, so these files and make-nested.py cannot drift
apart.

    python3 make-nested.py          # rebuild them in place
