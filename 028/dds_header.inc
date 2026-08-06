/* dds_header.inc - DirectDraw Surface header: parse, validate, size.
 *
 * Include inside the model struct, like cab_header.inc.  Parse(M) pulls bytes
 * through M->get() and returns the number consumed; M->f_quit signals EOF.
 *
 * The awkward part of carving DDS is that, unlike CAB's cbCabinet, a .dds file
 * does not declare its own length -- the payload size has to be *derived* from
 * width/height/depth/mip count/array size and the pixel format.  So this file
 * carries a full surface-size model (the DirectXTex GetSurfaceInfo rules) and
 * refuses to size anything it does not recognise exactly: an unknown FourCC or
 * DXGI format returns kind==KIND_NONE and the candidate is passed through
 * untouched rather than carved at a guessed length.
 *
 * Validation is deliberately strict, because "DDS " is only 32 bits and will
 * occur by chance in large binaries.  A candidate must have dwSize==124 and
 * ddspf.dwSize==32 (two more exact dwords), sane dimensions, a mip count that
 * matches the dimensions, a recognised format, and -- when the corresponding
 * flag is set and the field is non-zero -- a dwPitchOrLinearSize consistent
 * with the computed level-0 pitch or linear size.
 */

struct DDSHEADER {
  enum {
    DDS_MAGIC  = 0x20534444,          /* "DDS " little-endian                 */
    HDR_SIZE   = 124,                 /* DDS_HEADER.dwSize, exact             */
    PF_SIZE    = 32,                  /* DDS_PIXELFORMAT.dwSize, exact        */
    DX10_SIZE  = 20,

    DDSD_PITCH        = 0x8,
    DDSD_MIPMAPCOUNT  = 0x20000,
    DDSD_LINEARSIZE   = 0x80000,

    DDPF_ALPHA        = 0x2,
    DDPF_FOURCC       = 0x4,
    DDPF_PALETTE8     = 0x20,
    DDPF_RGB          = 0x40,
    DDPF_YUV          = 0x200,
    DDPF_LUMINANCE    = 0x20000,

    DDSCAPS2_CUBEMAP  = 0x200,
    CUBE_FACES_MASK   = 0xFC00,       /* the six DDSCAPS2_CUBEMAP_* face bits */
    DDSCAPS2_VOLUME   = 0x200000,

    DX10_MISC_CUBE    = 0x4,          /* D3D11_RESOURCE_MISC_TEXTURECUBE      */
    DX10_DIM_3D       = 4,

    /* sanity bounds: junk that happens to spell "DDS " must not size          */
    MAX_DIM   = 1<<16,
    MAX_MIPS  = 32,
    MAX_ARRAY = 1<<16,

    KIND_NONE = 0,                    /* unrecognised -> never carve          */
    KIND_BLOCK4 = 1,                  /* 4x4 block compressed                 */
    KIND_PACKED2 = 2,                 /* 2-pixel packed (R8G8_B8G8 etc)       */
    KIND_LINEAR = 3                   /* bits-per-pixel raster                */
  };

  /* raw header ---------------------------------------------------------- */
  uint sign;
  uint dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize, dwDepth, dwMipMapCount;
  uint dwReserved1[11];
  uint pfSize, pfFlags, pfFourCC, pfRGBBitCount, pfRMask, pfGMask, pfBMask, pfAMask;
  uint dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2;

  uint has_dx10;
  uint dxgiFormat, resourceDimension, miscFlag, arraySize, miscFlags2;
  /* Header oddities that used to be fatal.  Kept so a caller can report them;
     dwPitchOrLinearSize in particular is advisory and frequently wrong -- one
     file here declares a 24bpp linear size with dwRGBBitCount 8. */
  bool odd_reserved, odd_pitch;

  /* derived ------------------------------------------------------------- */
  uint  parse_err;
  const char* why;       /* set at every rejection; reported by ddsdet --why */
  uint  kind, unit;      /* unit: bytes/block (BLOCK4) or bits/pixel (LINEAR) */
  uint  mips, slices;    /* mip levels; array slices * cube faces             */
  uint  depth;
  qword hdr_len;         /* 4 + 124 (+20 for DX10)                            */
  qword data_len;        /* payload bytes                                     */
  qword total_len;       /* hdr_len + data_len == the whole .dds file          */
  /* Block formats whose width/height is not a multiple of 4 are ambiguous:
     nearly every writer rounds the block grid up, but some drop the partial
     edge row/column (the BTC files in the wild do).  Both extents are computed
     -- alt_len is the shorter, floor-grid one, 0 when there is no ambiguity --
     and the caller resolves it by looking at what actually follows.  Guessing
     is what this file otherwise refuses to do, so alt_len is only ever taken
     when the bytes at that offset are themselves a "DDS " signature or EOF. */
  qword alt_len[2];      /* shorter candidates, ascending; alts says how many  */
  uint  alts;
  int   floor_grid;      /* set while surface() is being asked for the alt size */

  static uint mk4( uint a, uint b, uint c, uint d ) { return a|(b<<8)|(c<<16)|(d<<24); }

  /* One little-endian dword.  get() yields uint(-1) at EOF, whose high bits
     are set, so a single OR test catches a truncated header.                 */
  template<class T> uint rd32( T* M ) {
    uint a = M->get(), b = M->get(), c = M->get(), d = M->get();
    if( (a|b|c|d) & ~0xFFu ) { parse_err = 1; return 0; }
    return a|(b<<8)|(c<<16)|(d<<24);
  }

  /* Read magic + DDS_HEADER, plus DDS_HEADER_DXT10 when the FourCC says so.
     Returns bytes consumed; parse_err is set if the stream ran dry.          */
  template<class T> qword Parse( T* M ) {
    parse_err = 0; has_dx10 = 0; why = "";
    odd_reserved = odd_pitch = false;
    kind = KIND_NONE; unit = 0; mips = 0; slices = 0; depth = 0;
    data_len = total_len = 0; alt_len[0] = alt_len[1] = 0; alts = 0; floor_grid = 0;
    dxgiFormat = resourceDimension = miscFlag = arraySize = miscFlags2 = 0;

    sign                = rd32(M);
    dwSize              = rd32(M);
    dwFlags             = rd32(M);
    dwHeight            = rd32(M);
    dwWidth             = rd32(M);
    dwPitchOrLinearSize = rd32(M);
    dwDepth             = rd32(M);
    dwMipMapCount       = rd32(M);
    for( uint i=0; i<11; i++ ) dwReserved1[i] = rd32(M);
    pfSize              = rd32(M);
    pfFlags             = rd32(M);
    pfFourCC            = rd32(M);
    pfRGBBitCount       = rd32(M);
    pfRMask             = rd32(M);
    pfGMask             = rd32(M);
    pfBMask             = rd32(M);
    pfAMask             = rd32(M);
    dwCaps              = rd32(M);
    dwCaps2             = rd32(M);
    dwCaps3             = rd32(M);
    dwCaps4             = rd32(M);
    dwReserved2         = rd32(M);
    hdr_len = 4 + HDR_SIZE;

    if( !parse_err && (pfFlags & DDPF_FOURCC) && pfFourCC == mk4('D','X','1','0') ) {
      dxgiFormat        = rd32(M);
      resourceDimension = rd32(M);
      miscFlag          = rd32(M);
      arraySize         = rd32(M);
      miscFlags2        = rd32(M);
      has_dx10 = 1;
      hdr_len += DX10_SIZE;
    }
    return hdr_len;
  }

  /* ---- format resolution ------------------------------------------------ */
  /* Sets kind/unit.  Anything not listed stays KIND_NONE, which makes check()
     reject the candidate: a carver must never guess a length.               */
  /* true for the tags resolve_legacy() knows -- lets a lax header whose ddspf
     is entirely zero still be identified by its FourCC alone */
  bool fourcc_known( void ) const {
    uint f = pfFourCC;
    return f==mk4('D','X','T','1') || f==mk4('D','X','T','2') || f==mk4('D','X','T','3') ||
           f==mk4('D','X','T','4') || f==mk4('D','X','T','5') || f==mk4('R','X','G','B') ||
           f==mk4('A','T','I','1') || f==mk4('A','T','I','2') ||
           f==mk4('B','C','4','U') || f==mk4('B','C','4','S') ||
           f==mk4('B','C','5','U') || f==mk4('B','C','5','S') ||
           f==mk4('B','T','C',' ') ||
           f==mk4('R','G','B','G') || f==mk4('G','R','G','B') ||
           f==mk4('U','Y','V','Y') || f==mk4('Y','U','Y','2') ||
           f==36 || (f>=110 && f<=117);
  }

  void resolve_legacy( void ) {
    if( (pfFlags & DDPF_FOURCC) || fourcc_known() ) {
      uint f = pfFourCC;
      if( f==mk4('D','X','T','1') || f==mk4('A','T','I','1') ||
          f==mk4('B','C','4','U') || f==mk4('B','C','4','S') ) { kind=KIND_BLOCK4; unit=8;  return; }
      if( f==mk4('D','X','T','2') || f==mk4('D','X','T','3') ||
          f==mk4('D','X','T','4') || f==mk4('D','X','T','5') ||
          f==mk4('R','X','G','B') ||
          f==mk4('A','T','I','2') || f==mk4('B','C','5','U') ||
          f==mk4('B','C','5','S') )                            { kind=KIND_BLOCK4; unit=16; return; }
      if( f==mk4('B','T','C',' ') )                            { kind=KIND_BLOCK4; unit=4;  return; }
      if( f==mk4('R','G','B','G') || f==mk4('G','R','G','B') ||
          f==mk4('U','Y','V','Y') || f==mk4('Y','U','Y','2') ) { kind=KIND_PACKED2; unit=0; return; }
      /* D3DFMT_* codes stored in the FourCC slot as small integers */
      switch( f ) {
        case 36: case 110: case 113: case 115: kind=KIND_LINEAR; unit=64;  return; /* 16.16.16.16 / f16x4 / f32x2 */
        case 111: case 117:                    kind=KIND_LINEAR; unit=16;  return; /* R16F / CxV8U8 */
        case 112: case 114:                    kind=KIND_LINEAR; unit=32;  return; /* G16R16F / R32F */
        case 116:                              kind=KIND_LINEAR; unit=128; return; /* A32B32G32R32F */
      }
      return;                                  /* unknown FourCC -> reject   */
    }
    if( pfFlags & DDPF_PALETTE8 ) return;      /* palette follows header; out of scope */
    if( pfFlags & (DDPF_RGB|DDPF_LUMINANCE|DDPF_ALPHA|DDPF_YUV) ) {
      uint b = pfRGBBitCount;
      if( b==8 || b==16 || b==24 || b==32 || b==64 || b==128 ) { kind=KIND_LINEAR; unit=b; }
    }
  }

  void resolve_dx10( void ) {
    uint f = dxgiFormat;
    /* block compressed */
    if( f>=70 && f<=99 ) {
      if( (f>=70&&f<=72) || (f>=79&&f<=81) ) { kind=KIND_BLOCK4; unit=8;  return; }  /* BC1, BC4 */
      if( (f>=73&&f<=78) || (f>=82&&f<=84) ||
          (f>=94&&f<=99) )                   { kind=KIND_BLOCK4; unit=16; return; }  /* BC2/3, BC5, BC6H/7 */
      return;
    }
    if( f==68 || f==69 ) { kind=KIND_PACKED2; unit=0; return; }   /* R8G8_B8G8, G8R8_G8B8 */
    uint b = 0;
    if( f>=1  && f<=4  ) b = 128;
    else if( f>=5  && f<=8  ) b = 96;
    else if( f>=9  && f<=22 ) b = 64;
    else if( f>=23 && f<=47 ) b = 32;
    else if( f>=48 && f<=57 ) b = 16;
    else if( f>=58 && f<=65 ) b = 8;
    else if( f==66 )          b = 1;
    else if( f==67 )          b = 32;
    else if( f==85 || f==86 ) b = 16;
    else if( f>=87 && f<=93 ) b = 32;
    else if( f==115 )         b = 16;   /* B4G4R4A4_UNORM */
    if( b ) { kind = KIND_LINEAR; unit = b; }
  }

  /* bytes for one w*h surface of the resolved format */
  qword surface( uint w, uint h ) const {
    if( kind==KIND_BLOCK4 ) {
      qword bw, bh;
      if( floor_grid ) { bw = w/4; bh = h/4; }
      else             { bw = (w+3)/4; bh = (h+3)/4; }
      if( !bw ) bw = 1;
      if( !bh ) bh = 1;
      return bw*bh*unit;
    }
    if( kind==KIND_PACKED2 ) return (qword)((w+1)/2)*4*h;
    return (qword)(((qword)w*unit+7)/8)*h;
  }

  static uint max_levels( uint w, uint h, uint d ) {
    uint n = 1, m = w;
    if( h>m ) m = h;
    if( d>m ) m = d;
    while( m>1 ) { m >>= 1; n++; }
    return n;
  }

  static uint popcnt6( uint v ) { uint n=0; while(v) { n += v&1; v >>= 1; } return n; }

  /* ---- validation + sizing --------------------------------------------- */
  /* true iff this really looks like a DDS *and* its exact byte length is
     known.  Fills mips/slices/depth/data_len/total_len.                     */
  bool check( void ) {
    if( parse_err ) return why = "header truncated", false;
    if( sign != (uint)DDS_MAGIC ) return why = "signature", false;
    /* dwSize==124 is the strongest single filter after the signature.  The
       lax writers that zero ddspf.dwSize also zero this one; accept 0 only
       when dwFourCC by itself names a format we can size, so the candidate
       still has to match a 4-byte tag and every geometry check below.       */
    if( dwSize != HDR_SIZE && !(dwSize==0 && !has_dx10 && fourcc_known()) )
      return why = "dwSize != 124", false;
    /* Spec says ddspf.dwSize==32, and it is the second strong exact-value
       filter after dwSize==124.  But lax writers exist in the wild that leave
       it (and dwCaps) zero while the rest of the header is perfectly well
       formed -- DirectXTex rejects those, most game loaders do not.  Accepting
       {32,0} instead of {32} costs exactly one bit of false-positive
       resistance (2^-31 vs 2^-32 on random data), which is nothing next to
       silently skipping real textures. Any other value is still rejected.    */
    if( pfSize != PF_SIZE && pfSize != 0 ) return why = "ddspf.dwSize is neither 32 nor 0", false;
    /* dwCaps3, dwCaps4 and dwReserved2 are "unused" per the spec, and real
       writers use them anyway as private markers: one file here carries "DDSX"
       in dwCaps3 with 0xFFFFFFFF in dwCaps4 and dwReserved2, another "FYRC" in
       dwReserved2.  Requiring zero skipped both textures entirely.
       These were 96 bits of false-positive resistance, but what actually
       carries carving are dwSize==124, ddspf.dwSize in {32,0}, the format
       having to resolve to a known length, and the computed payload having to
       fit the remaining input -- that last one is the real filter.  Recorded
       rather than rejected. */
    odd_reserved = (dwCaps3 || dwCaps4 || dwReserved2);
    if( dwWidth==0 || dwHeight==0 ) return why = "zero width/height", false;
    if( dwWidth>(uint)MAX_DIM || dwHeight>(uint)MAX_DIM ) return why = "dimensions out of range", false;

    if( has_dx10 ) resolve_dx10(); else resolve_legacy();
    if( kind==KIND_NONE ) return why = "unrecognised pixel format (would have to guess the length)", false;

    /* geometry ---------------------------------------------------------- */
    depth = 1;
    slices = 1;
    if( has_dx10 ) {
      if( resourceDimension<2 || resourceDimension>4 ) return why = "bad DX10 resourceDimension", false;
      if( resourceDimension==(uint)DX10_DIM_3D ) depth = dwDepth ? dwDepth : 1;
      uint a = arraySize ? arraySize : 1;
      if( a>(uint)MAX_ARRAY ) return why = "DX10 arraySize out of range", false;
      slices = a;
      if( miscFlag & DX10_MISC_CUBE ) slices *= 6;
    } else {
      if( dwCaps2 & DDSCAPS2_VOLUME ) depth = dwDepth ? dwDepth : 1;
      if( dwCaps2 & DDSCAPS2_CUBEMAP ) {
        uint faces = popcnt6( dwCaps2 & CUBE_FACES_MASK );
        if( !faces ) return why = "cubemap with no face bits", false;        /* cubemap with no face bits: junk */
        slices = faces;
      }
    }
    if( depth>(uint)MAX_DIM ) return why = "depth out of range", false;

    /* mip count must be consistent with the dimensions ------------------- */
    mips = dwMipMapCount ? dwMipMapCount : 1;
    if( mips>(uint)MAX_MIPS ) return why = "mip count out of range", false;
    if( mips>max_levels(dwWidth, dwHeight, depth) ) return why = "mip count impossible for these dimensions", false;
    if( (dwFlags & DDSD_MIPMAPCOUNT) && dwMipMapCount==0 ) return why = "DDSD_MIPMAPCOUNT set but count is 0", false;

    /* level-0 cross-check against dwPitchOrLinearSize -------------------- */
    if( dwPitchOrLinearSize ) {
      qword lin = surface(dwWidth, dwHeight);
      qword pit = (kind==KIND_BLOCK4)
                  ? (qword)((dwWidth+3)/4 ? (dwWidth+3)/4 : 1)*unit
                  : (kind==KIND_PACKED2)
                    ? (qword)((dwWidth+1)/2)*4
                    : ((qword)dwWidth*unit+7)/8;
      if( (dwFlags & (DDSD_LINEARSIZE|DDSD_PITCH)) &&
          dwPitchOrLinearSize!=lin && dwPitchOrLinearSize!=pit ) odd_pitch = true;
    }

    /* payload ----------------------------------------------------------- */
    qword one = 0;
    uint w = dwWidth, h = dwHeight, d = depth;
    for( uint m=0; m<mips; m++ ) {
      one += surface(w, h) * d;
      if( w>1 ) w >>= 1;
      if( h>1 ) h >>= 1;
      if( d>1 ) d >>= 1;
    }
    data_len = one * slices;
    if( data_len==0 ) return why = "computed payload is empty", false;
    total_len = hdr_len + data_len;

    /* Shorter candidates for a block format whose dimensions are not a
       multiple of 4.  Two writer behaviours are known:
         - drop the partial edge row/column  -> floor block grid
         - size the payload from the raw dimensions, as if the format were
           linear at unit*8/16 bits per pixel (the BTC writers do this: a
           4421x8632 file holds exactly W*H/4 bytes)
       Both are computed, never guessed; the caller picks by looking at what
       follows.  Sorted ascending so the caller can walk them in order. */
    if( kind==KIND_BLOCK4 && (dwWidth&3 || dwHeight&3) && dwWidth>=4 && dwHeight>=4 ) {
      qword cand[2]; uint nc = 0;
      floor_grid = 1;
      qword two = 0; w = dwWidth; h = dwHeight; d = depth;
      for( uint m=0; m<mips; m++ ) {
        two += surface(w, h) * d;
        if( w>1 ) w >>= 1;
        if( h>1 ) h >>= 1;
        if( d>1 ) d >>= 1;
      }
      floor_grid = 0;
      two *= slices;
      if( two && two < data_len ) cand[nc++] = two;
      qword raw = 0; w = dwWidth; h = dwHeight; d = depth;
      for( uint m=0; m<mips; m++ ) {
        raw += ((qword)w*h*unit/16) * d;
        if( w>1 ) w >>= 1;
        if( h>1 ) h >>= 1;
        if( d>1 ) d >>= 1;
      }
      raw *= slices;
      if( raw && raw < data_len && (nc==0 || raw != cand[0]) ) cand[nc++] = raw;
      if( nc==2 && cand[0] > cand[1] ) { qword t = cand[0]; cand[0] = cand[1]; cand[1] = t; }
      for( uint i=0; i<nc; i++ ) alt_len[i] = cand[i];
      alts = nc;
    }
    return true;
  }

  /* Short description of what the pixels actually are, for the carve report.
   * "block"/"packed"/"linear" alone said only how the data is laid out, not
   * which codec or channel layout it is -- a FourCC texture and an RGB565
   * raster both just read "linear"/"block". */
  void subfmt( char* out, size_t cap ) const {
    if( has_dx10 ) {
      /* DXGI_FORMAT names, index = the enum value.  Function-local because this
       * file is included INSIDE the carver's class body, where a namespace-scope
       * array would become a static data member with no out-of-class definition.
       * The carver meets only a handful of these, but a bare number in the
       * report makes the reader go and look up a header. */
    static const char* const N[] = {
        0, "R32G32B32A32_TYPELESS", "R32G32B32A32_FLOAT", "R32G32B32A32_UINT",
        "R32G32B32A32_SINT", "R32G32B32_TYPELESS", "R32G32B32_FLOAT", "R32G32B32_UINT",
        "R32G32B32_SINT", "R16G16B16A16_TYPELESS", "R16G16B16A16_FLOAT", "R16G16B16A16_UNORM",
        "R16G16B16A16_UINT", "R16G16B16A16_SNORM", "R16G16B16A16_SINT", "R32G32_TYPELESS",
        "R32G32_FLOAT", "R32G32_UINT", "R32G32_SINT", "R32G8X24_TYPELESS",
        "D32_FLOAT_S8X24_UINT", "R32_FLOAT_X8X24_TYPELESS", "X32_TYPELESS_G8X24_UINT", "R10G10B10A2_TYPELESS",
        "R10G10B10A2_UNORM", "R10G10B10A2_UINT", "R11G11B10_FLOAT", "R8G8B8A8_TYPELESS",
        "R8G8B8A8_UNORM", "R8G8B8A8_UNORM_SRGB", "R8G8B8A8_UINT", "R8G8B8A8_SNORM",
        "R8G8B8A8_SINT", "R16G16_TYPELESS", "R16G16_FLOAT", "R16G16_UNORM",
        "R16G16_UINT", "R16G16_SNORM", "R16G16_SINT", "R32_TYPELESS",
        "D32_FLOAT", "R32_FLOAT", "R32_UINT", "R32_SINT",
        "R24G8_TYPELESS", "D24_UNORM_S8_UINT", "R24_UNORM_X8_TYPELESS", "X24_TYPELESS_G8_UINT",
        "R8G8_TYPELESS", "R8G8_UNORM", "R8G8_UINT", "R8G8_SNORM",
        "R8G8_SINT", "R16_TYPELESS", "R16_FLOAT", "D16_UNORM",
        "R16_UNORM", "R16_UINT", "R16_SNORM", "R16_SINT",
        "R8_TYPELESS", "R8_UNORM", "R8_UINT", "R8_SNORM",
        "R8_SINT", "A8_UNORM", "R1_UNORM", "R9G9B9E5_SHAREDEXP",
        "R8G8_B8G8_UNORM", "G8R8_G8B8_UNORM", "BC1_TYPELESS", "BC1_UNORM",
        "BC1_UNORM_SRGB", "BC2_TYPELESS", "BC2_UNORM", "BC2_UNORM_SRGB",
        "BC3_TYPELESS", "BC3_UNORM", "BC3_UNORM_SRGB", "BC4_TYPELESS",
        "BC4_UNORM", "BC4_SNORM", "BC5_TYPELESS", "BC5_UNORM",
        "BC5_SNORM", "B5G6R5_UNORM", "B5G5R5A1_UNORM", "B8G8R8A8_UNORM",
        "B8G8R8X8_UNORM", "R10G10B10_XR_BIAS_A2_UNORM", "B8G8R8A8_TYPELESS", "B8G8R8A8_UNORM_SRGB",
        "B8G8R8X8_TYPELESS", "B8G8R8X8_UNORM_SRGB", "BC6H_TYPELESS", "BC6H_UF16",
        "BC6H_SF16", "BC7_TYPELESS", "BC7_UNORM", "BC7_UNORM_SRGB",
        "AYUV", "Y410", "Y416", "NV12",
        "P010", "P016", "420_OPAQUE", "YUY2",
        "Y210", "Y216", "NV11", "AI44",
        "IA44", "P8", "A8P8", "B4G4R4A4_UNORM",
      };
      const char* n = (dxgiFormat < sizeof N / sizeof *N) ? N[dxgiFormat] : 0;
      if( n ) snprintf(out, cap, "%s", n);
      else    snprintf(out, cap, "DXGI:%u", dxgiFormat);
      return;
    }
    if( (pfFlags & DDPF_FOURCC) || fourcc_known() ) {
      uint f = pfFourCC;
      /* The FourCC field doubles as a numeric D3DFMT enum for the float and
       * 16-bit-per-channel formats, so a small value is a code, not four
       * characters -- printing it as ASCII gave "o..." for D3DFMT_R16F. */
      if( f < 0x100 ) {
        const char* n = f==36  ? "A16B16G16R16" : f==110 ? "Q16W16V16U16" :
                        f==111 ? "R16F"         : f==112 ? "G16R16F"      :
                        f==113 ? "A16B16G16R16F": f==114 ? "R32F"         :
                        f==115 ? "G32R32F"      : f==116 ? "A32B32G32R32F":
                        f==117 ? "CxV8U8"       : 0;
        if( n ) snprintf(out, cap, "%s", n);
        else    snprintf(out, cap, "D3D:%u", f);
        return;
      }
      char c[5] = { char(f), char(f>>8), char(f>>16), char(f>>24), 0 };
      for( int i=0; i<4; i++ ) if( c[i] < 32 || c[i] > 126 ) c[i] = '.';
      for( int i=3; i>=0 && c[i]==' '; i-- ) c[i] = 0;   // "BTC " -> "BTC"
      snprintf(out, cap, "%s", c); return;
    }
    /* Uncompressed: name the channels in mask order, high bits first, which is
     * the DirectX convention (A8R8G8B8 has A in the top byte). */
    struct { char n; uint m; } ch[4] = {
      { (pfFlags & DDPF_LUMINANCE) ? 'L' : 'R', pfRMask }, { 'G', pfGMask },
      { 'B', pfBMask }, { 'A', pfAMask } };
    for( int i=0; i<4; i++ ) for( int j=i+1; j<4; j++ )
      if( ch[j].m > ch[i].m ) { auto t = ch[i]; ch[i] = ch[j]; ch[j] = t; }
    char b[32]; size_t n = 0; uint tot = 0; b[0] = 0;
    for( int i=0; i<4 && n+4 < sizeof b; i++ ) {
      if( !ch[i].m ) continue;
      uint bits = 0; for( uint m = ch[i].m; m; m >>= 1 ) bits += m & 1;
      tot += bits;
      n += (size_t)snprintf(b+n, sizeof b - n, "%c%u", ch[i].n, bits);
    }
    /* Only trust the channel names if they account for the declared depth.
       One file here is 8bpp (dwRGBBitCount 8, and the payload confirms it) but
       still carries 24-bit masks 00FF0000/0000FF00/000000FF from whatever it was
       converted from -- naming it "R8G8B8" claimed three times the real size.
       The bit count is what the sizing uses, so it wins the label too. */
    if( b[0] && tot == pfRGBBitCount ) snprintf(out, cap, "%s", b);
    else if( b[0] ) snprintf(out, cap, "%ubpp/masks%u", pfRGBBitCount, tot);
    else snprintf(out, cap, "%ubpp", pfRGBBitCount);
  }

  const char* kindname( void ) const {
    return kind==KIND_BLOCK4 ? "block" : kind==KIND_PACKED2 ? "packed" :
           kind==KIND_LINEAR ? "linear" : "?";
  }
};
