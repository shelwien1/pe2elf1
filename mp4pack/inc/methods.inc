// inc/methods.inc -- what goes where: the groups of -t/-c/-l, the methods
//  (packers) and which group each is in, a stream's method by its extension
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- what goes where

// -t/-c positions: the order the groups are given in
enum { G_VIDEO, G_AUDIO, G_FONT, G_OTHER, N_GROUP };
static const char* g_name[N_GROUP] = { "video", "audio", "ttfcomp", "tangelo" };

enum { M_STORE, M_TANGELO, M_H264P, M_AACPACK, M_MP3PACK, M_OGGCOMP, M_TTFCOMP, M_M4VPACK, M_AC3PACK, N_METHOD };
static const char* m_name[N_METHOD] = { "store", "tangelo", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp", "m4vpack", "ac3pack" };
static const char* m_tool[N_METHOD] = { "", "tangelo", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp", "m4vpack", "ac3pack" };
static const int m_group[N_METHOD] = { G_OTHER, G_OTHER, G_VIDEO, G_AUDIO, G_AUDIO, G_AUDIO, G_FONT, G_VIDEO, G_AUDIO };

// by the extension avip gave the stream
static int Method( const string& name ) {
  size_t d = name.rfind( '.' );
  string e = (d==string::npos) ? string() : name.substr( d+1 );
  for( size_t i=0; i<e.size(); i++ ) if( (e[i]>='A') && (e[i]<='Z') ) e[i] += 'a'-'A';
  if( (e=="h264") || (e=="264") ) return M_H264P;
  if( e=="m4v" ) return M_M4VPACK; // (MPEG-4 Part 2: XviD, DivX)
  if( e=="aac" ) return M_AACPACK;
  if( (e=="ac3") || (e=="eac3") ) return M_AC3PACK;
  if( (e=="mp3") || (e=="mp2") || (e=="mp1") || (e=="mpa") ) return M_MP3PACK;
  if( (e=="ogg") || (e=="oga") ) return M_OGGCOMP;
  if( (e=="ttf") || (e=="otf") || (e=="ttc") ) return M_TTFCOMP;
  return M_TANGELO;
}

