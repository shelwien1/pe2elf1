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
//   - stsc                        : runs of samples-per-chunk, or aligned to another
//                                   track's chunk start times (interleaving muxers)
//   - chunk order in the file     : round-robin prediction over the tracks
//   - avcC/hvcC parameter sets    : from the Annex B stream header
//   - tfhd base_data_offset,
//     trun data_offset            : stored as residuals vs field/chunk position (~0)
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

} // namespace mp4
