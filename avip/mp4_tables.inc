// MP4/MOV container
//
// Track streams hold the sample data of each track in sample order (moov stbl samples
// first, then moof/trun fragments in file order). Conversions: avc1/avc3/hvc1/hev1 ->
// Annex B (.h264/.hevc), mp4a AAC -> ADTS (.aac), mp4a Vorbis (OTI 0xDD) -> Ogg (.ogg);
// sowt/raw/ulaw/alaw, little-endian lpcm/in24/in32/fl32/fl64 -> WAV (header prepended);
// MPEG-4 part 2 (mp4v OTI 0x20, xvid/DIVX/..), MPEG-1/2 video (m2v1, mp4v OTI 0x60..0x6A,
// xdv*/hdv*/mx*), Motion JPEG (jpeg, mjpa, mp4v OTI 0x6C) are elementary streams, with the
// esds/glbl codec config prepended when the first frame lacks it (the config is then cut
// from the struct); MPEG-1/2 audio, ac-3, ec-3 are elementary streams already. iTunes cover art
// (ilst/covr/data) is cut out as blob streams. Truncated files: a track keeps its whole
// samples up to EOF; the partial last sample and table entries for missing samples stay
// in the struct. Regenerated on restore:
//   - stco/co64 entries           : from actual chunk positions in the restored layout
//   - stsz and trun sample_size   : from self-delimiting sample data when it parses
//                                   (start-code units checked in stream context), from
//                                   ADTS frame_length or Ogg packet boundaries
//   - stts, ctts, stss, sdtp,     : per-track timing model: decode times
//     trun sample_duration /        q*round((x*m+c)/2^k) over the sample index or the
//     composition offset, tfdt      Vorbis sample position; composition offsets from the
//                                   H.264/HEVC display order; sync samples from
//                                   IDR/IRAP pictures (other video: I-VOP / I-picture /
//                                   intra frame classes); sdtp from key/intra/non-reference
//                                   class; only residuals are stored
//   - trun sample_flags           : a value per key/intra/non-reference class (key frames
//                                   have their own), exceptions stored
//   - stsc                        : runs of samples-per-chunk, or aligned to another
//                                   track's chunk start times (interleaving muxers)
//   - chunk order in the file     : round-robin prediction over the tracks
//   - avcC/hvcC parameter sets    : from the Annex B stream header
//   - tfhd base_data_offset,
//     trun data_offset            : stored as residuals vs field/chunk position (~0)
//   - fragments of one sample     : the whole moof and mdat header from the track's
//                                   previous fragment of the same shape (see FragTmpl),
//                                   the tracks' fragments in decode time order
// stsc/stsz/trun (when kept) are read back by the decoder directly from the struct.
// The encoder runs the decoder in memory and falls back to raw track streams, then to
// no timing/chunk models, then to a plain copy.

// MP4: stsz/stz2/stsc parsers and trun headers (shared by encoder and decoder)
namespace mp4 {

//--- shared table parsers (encoder and decoder use the same code) ------------

// sample sizes: either constant (csize!=0, no array) or per-sample array
struct SizeInfo {
  uint csize; qword count; std::vector<uint> v;
  SizeInfo() : csize(0), count(0) {}
  uint get( qword i ) const { return csize ? csize : v[size_t(i)]; }
};

// stsz / stz2 payload (starting at version/flags)
static bool ParseSizes( const byte* p, qword len, bool stz2, qword maxbytes, SizeInfo& s ) {
  s.v.clear(); s.csize=0; s.count=0;
  if( len<12 ) return false;
  qword n = BE32(p+8);
  if( !stz2 ) {
    uint cs = BE32(p+4);
    if( cs ) {
      if( n>MAXCOUNT || n*cs>maxbytes ) return false;
      s.csize=cs; s.count=n; return true;
    }
    if( 12+4*n>len ) return false;
    s.v.resize( size_t(n) );
    for( qword i=0; i<n; i++ ) s.v[size_t(i)] = BE32(p+12+4*i);
  } else {
    uint fs = p[7];
    if( fs!=4 && fs!=8 && fs!=16 ) return false;
    if( 12+(n*fs+7)/8>len ) return false;
    s.v.resize( size_t(n) ); const byte* q=p+12;
    for( qword i=0; i<n; i++ ) {
      uint x;
      if( fs==4 ) x = (i&1) ? (q[i>>1]&15) : (q[i>>1]>>4);
      else if( fs==8 ) x = q[i];
      else x = BE16(q+2*i);
      s.v[size_t(i)] = x;
    }
  }
  s.count = n;
  return true;
}

// stsc payload -> samples-per-chunk for nchunks chunks
static bool ParseStsc( const byte* p, qword len, qword nchunks, std::vector<uint>& spc ) {
  if( len<8 || nchunks>MAXCOUNT ) return false;
  qword n = BE32(p+4);
  if( 8+12*n>len ) return false;
  spc.assign( size_t(nchunks), 0 );
  qword prev = 0;
  for( qword i=0; i<n; i++ ) {
    qword f = BE32(p+8+12*i); uint s = BE32(p+8+12*i+4);
    if( f<prev ) return false;
    prev=f;
    qword nf = (i+1<n) ? qword(BE32(p+8+12*(i+1))) : nchunks+1;
    if( f<1 ) f=1;
    for( qword c=f; c<nf && c<=nchunks; c++ ) spc[size_t(c-1)] = s;
  }
  return true;
}

//--- trun header (shared) ----------------------------------------------------

struct TrunHdr { uint flags, count, recsize, szoff; qword hlen; };

static bool ParseTrunHdr( const byte* p, qword len, TrunHdr& h ) {
  if( len<8 ) return false;
  h.flags = BE32(p)&0xFFFFFF; h.count = BE32(p+4);
  h.hlen = 8 + ((h.flags&1)?4:0) + ((h.flags&4)?4:0);
  h.recsize = 4*( ((h.flags>>8)&1)+((h.flags>>9)&1)+((h.flags>>10)&1)+((h.flags>>11)&1) );
  h.szoff = (h.flags&0x100) ? 4 : 0;
  return h.hlen<=len && h.count<=MAXCOUNT;
}

} // namespace mp4

namespace mp4 {

// fragment index predictions (sidx, tfra), shared by encoder and decoder. marks: output
// positions of the top-level boxes except mdat from the first moof on, ascending.
// Per chunk c of a track: position cpos[c], first sample cfirst[c], spc[c] samples.

// sidx entries k<rc starting at mark j0: referenced_size = next mark - mark, duration =
// sum of the track's sample durations (dur may be null) in the chunks inside the range
static void SidxPredict( const std::vector<qword>& marks, qword j0, qword rc, const std::vector<qword>& cpos,
                         const std::vector<qword>& cfirst, const std::vector<qword>& spc, const qword* dur,
                         std::vector<qword>& psz, std::vector<qword>& pdur ) {
  psz.assign( size_t(rc), 0 ); pdur.assign( size_t(rc), 0 );
  for( qword k=0; k<rc; k++ ) if( j0+k+1<marks.size() ) psz[size_t(k)] = marks[size_t(j0+k+1)]-marks[size_t(j0+k)];
  if( !dur || j0>=marks.size() ) return;
  for( size_t c=0; c<cpos.size() && c<cfirst.size(); c++ ) {
    qword p = cpos[c];
    size_t j = size_t( std::upper_bound( marks.begin(), marks.end(), p ) - marks.begin() );
    if( j==0 || j-1<j0 || j-1>=j0+rc || j>=marks.size() ) continue;     // inside [mark j-1, mark j)
    qword s = 0; for( qword n=0; n<spc[c]; n++ ) s += dur[size_t(cfirst[c]+n)];
    pdur[size_t(j-1-j0)] += s;
  }
}

// tfra entry k <-> the track's trun chunk npc+k: moof_offset = last mark at or before the
// chunk, time = tbase + dts + composition offset of its first sample (dts/cto may be null)
static void TfraPredict( const std::vector<qword>& marks, qword n, size_t npc, const std::vector<qword>& cpos,
                         const std::vector<qword>& cfirst, const qword* dts, const long long* cto, long long tbase,
                         std::vector<qword>& poff, std::vector<qword>& ptime ) {
  poff.assign( size_t(n), 0 ); ptime.assign( size_t(n), 0 );
  for( qword k=0; k<n; k++ ) {
    size_t c = npc+size_t(k);
    if( c>=cpos.size() ) break;
    size_t j = size_t( std::upper_bound( marks.begin(), marks.end(), cpos[c] ) - marks.begin() );
    if( j ) poff[size_t(k)] = marks[j-1];
    if( dts ) ptime[size_t(k)] = qword(tbase) + dts[size_t(cfirst[c])] + qword( cto ? cto[size_t(cfirst[c])] : 0 );
  }
}

// a/ta < b/tb: signed times in timescales ta, tb > 0
static bool TimeLess( long long a, qword ta, long long b, qword tb ) {
  if( (a<0)!=(b<0) ) return a<0;
  if( a>=0 ) return U128::Mul( qword(a), tb ).Lt( U128::Mul( qword(b), ta ) );
  return U128::Mul( qword(-(b+1))+1, ta ).Lt( U128::Mul( qword(-(a+1))+1, tb ) );
}

//--- one-sample fragments (ops 0xF5..0xF7) --------------------------------------
//
// A fragment of one sample: a moof with an mfhd and one traf (a tfhd, at most one tfdt, one
// trun of one sample, any other boxes), then an mdat holding just that sample. Its template is
// the bytes of the moof and of the mdat header; a fragment from a template is those bytes with
// regenerated fields: sequence number, base_data_offset (the template's distance to the moof
// position), the sample's duration and size (tfhd defaults when the trun has no such field,
// else the trun record), its decode time (tfdt), composition offset (trun), the mdat size.
// Everything else (flags, track id, data_offset, sample flags, other boxes) is the template's.
struct FragTmpl {
  Buf b; qword msz; int hl;                      // moof + mdat header; moof size; mdat header length (8, 16)
  qword seqo, bdoo, duro, szo, tfdto, reco;      // field offsets in b (0: none)
  int tfdtv; uint rfl; long long relbdo;         // tfdt version, trun flags, base_data_offset - moof position
};

// box at o in [o,e) of p, with a 32-bit size
static bool FragBox( const byte* p, qword o, qword e, qword& sz, uint& ty ) {
  if( e-o<8 ) return false;
  sz = BE32(p+o); ty = BE32(p+o+4);
  return sz>=8 && sz<=e-o;
}

// template of the n bytes at p (moof + mdat header, the moof at stream position pos)
static bool FragParse( const byte* p, qword n, qword pos, FragTmpl& f ) {
  qword msz, ms; uint ty;
  if( !FragBox( p, 0, n, msz, ty ) || ty!=FCC_BE('m','o','o','f') || n-msz<8 ) return false;
  ms = BE32(p+msz); f.hl = ms==1 ? 16 : 8;
  if( BE32(p+msz+4)!=FCC_BE('m','d','a','t') || (ms>1 && ms<8) || !ms || n-msz!=qword(f.hl) ) return false;
  f.b.assign( p, p+n ); f.msz = msz;
  f.seqo = f.bdoo = f.duro = f.szo = f.tfdto = f.reco = 0; f.tfdtv = 0; f.rfl = 0; f.relbdo = 0;
  int nmfhd = 0, ntraf = 0, ntfhd = 0, ntfdt = 0, ntrun = 0;
  for( qword o=8, sz; o<msz; o+=sz ) {
    if( !FragBox( p, o, msz, sz, ty ) ) return false;
    if( ty==FCC_BE('m','f','h','d') ) { if( sz!=16 ) return false; f.seqo = o+12; nmfhd++; }
    if( ty!=FCC_BE('t','r','a','f') ) continue;
    ntraf++;
    for( qword a=o+8, s2; a<o+sz; a+=s2 ) {
      if( !FragBox( p, a, o+sz, s2, ty ) ) return false;
      const byte* q = p+a+8; qword pl = s2-8;
      if( ty==FCC_BE('t','f','h','d') ) {
        if( pl<8 ) return false;
        uint fl = BE32(q)&0xFFFFFF; qword fo = a+16;       // after version/flags and track_ID
        if( fl&1 ) f.bdoo = fo, fo += 8;
        if( fl&2 ) fo += 4;
        if( fl&8 ) f.duro = fo, fo += 4;
        if( fl&0x10 ) f.szo = fo, fo += 4;
        if( fl&0x20 ) fo += 4;
        if( fo>a+s2 ) return false;
        ntfhd++;
      } else if( ty==FCC_BE('t','f','d','t') ) {
        if( pl<8 || q[0]>1 || (q[0] && pl<12) ) return false;
        f.tfdtv = q[0]; f.tfdto = a+12; ntfdt++;
      } else if( ty==FCC_BE('t','r','u','n') ) {
        TrunHdr h;
        if( !ParseTrunHdr( q, pl, h ) || h.count!=1 || h.hlen+h.recsize>pl ) return false;
        f.rfl = h.flags; f.reco = a+8+h.hlen; ntrun++;
      }
    }
  }
  if( nmfhd!=1 || ntraf!=1 || ntfhd!=1 || ntfdt>1 || ntrun!=1 ) return false;
  if( f.bdoo ) f.relbdo = (long long)( BE64(p+f.bdoo)-pos );
  return true;
}

// the fragment from template f at stream position pos, sequence number seq, of a sample of
// duration dur, size size, decode time tdec, composition offset cto
static void FragRegen( const FragTmpl& f, qword pos, qword seq, qword dur, qword size, qword tdec, long long cto, Buf& o ) {
  o = f.b; byte* p = o.data();
  PutBE32( p+f.seqo, uint(seq) );
  if( f.bdoo ) PutBE64( p+f.bdoo, pos+qword(f.relbdo) );
  if( f.duro && !(f.rfl&0x100) ) PutBE32( p+f.duro, uint(dur) );
  if( f.szo && !(f.rfl&0x200) ) PutBE32( p+f.szo, uint(size) );
  if( f.tfdto ) { if( f.tfdtv ) PutBE64( p+f.tfdto, tdec ); else PutBE32( p+f.tfdto, uint(tdec) ); }
  qword r = f.reco;
  if( f.rfl&0x100 ) PutBE32( p+r, uint(dur) ), r += 4;
  if( f.rfl&0x200 ) PutBE32( p+r, uint(size) ), r += 4;
  if( f.rfl&0x400 ) r += 4;
  if( f.rfl&0x800 ) PutBE32( p+r, uint(cto) );
  if( f.hl==8 ) PutBE32( p+f.msz, uint(8+size) ); else PutBE64( p+f.msz+8, 16+size );
}

} // namespace mp4
