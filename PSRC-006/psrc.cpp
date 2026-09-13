
#include "common.inc"

#include "win_shim.inc"

template <class T>
  T* mAlloc( int Size ) 
    { return static_cast<T*>(VirtualAlloc(0,sizeof(T)*Size,MEM_COMMIT,PAGE_READWRITE)); }

#include "file_api.inc"

#include "sh_v1y.inc"

#include "sh_mapping.inc"
#include "MOD/sh_model_h.inc"
#include "MOD/sh_model_p.inc"

#include "qmaq1.inc"

#include "coro3b.inc"

// archive (compressed) bytes consumed, maintained by CoroFileProc::processfile
qword g_prog_in = 0;

#include "coro_fhp2.inc"

#include "counter.inc"

#include "model_rle.inc"

enum { 
  tmpbufsize_dec = AlignUp( quantbufsize + 2*255*max_trail_FFs*probN + sizeof(rle_state) +4, 4096 ),
  tmpbufsize_enc = AlignUp( quantbufsize + quantbufsize/8 + 4*probN +4, 4096 ),
  tmpbufsize_cod = AlignUp( quantbufsize + 4*probN +4, 4096 ), // RLE bytes of one block
  tmpbufsize = (tmpbufsize_dec<tmpbufsize_enc) ? tmpbufsize_enc : tmpbufsize_dec // max, unused now
};
// Model::qbuf takes tmpbufsize_dec or tmpbufsize_enc per mode -- with the two
// stages merged there is no single buffer that has to serve both directions.
// stage 2 (cc/dc) holds RLE bytes instead of quants, hence tmpbufsize_cod.

#include "model.inc"


template <class T>
void processfile( filehandle f, filehandle g, uint _f_len ) {
  T* P = mAlloc<T>(1);
  if( P==0 ) return;
  P->f_len = _f_len;
  P->processfile( f, g );
}

// c  : file -> archive           d  : archive -> file
// cr : file -> RLE stream        dr : RLE stream -> file      (model half)
// cc : RLE stream -> archive     dc : archive -> RLE stream   (coder half)
//
// so  `cr a b' + `cc b c'  ==  `c a c',  and  `dc c b' + `dr b a'  ==  `d c a'.

int main( int argc, char** argv ) {

  if( argc<4 ) return 1;

  const char* cmd = argv[1];
  uint dir   = (cmd[0]=='d');
  uint stage = (cmd[1]=='r') ? 1 : (cmd[1]=='c') ? 2 : 0;
  if( ((cmd[0]!='c') && (cmd[0]!='d')) || (cmd[1] && (!stage || cmd[2])) ) return 3;

  filehandle f( argv[2],0 ); if( f==0 ) return 1;
  filehandle g( argv[3],1 ); if( g==0 ) return 2;
  uint f_len = 0;

  // original length: computed where the input is the plain file, read from
  // the 4-byte header otherwise; every stream except the plain file carries
  // it, because both stages derive the block schedule from it.
  if( (dir==0) && (stage!=2) ) f_len = f.size(); else f.read( f_len );
  if( (dir==0) || (stage==2) ) g.writ( f_len );

  if( dir==0 ) {
    if( stage==0 ) processfile< CoroFileProc< Model<0,0> > >( f, g, f_len ); else
    if( stage==1 ) processfile< CoroFileProc< Model<0,1> > >( f, g, f_len ); else
                   processfile< CoroFileProc< Model<0,2> > >( f, g, f_len );
  } else {
    if( stage==0 ) processfile< CoroFileProc< Model<1,0> > >( f, g, f_len ); else
    if( stage==1 ) processfile< CoroFileProc< Model<1,1> > >( f, g, f_len ); else
                   processfile< CoroFileProc< Model<1,2> > >( f, g, f_len );
  }

  f.close();

/*
{
  int i,s;
  byte flag[probN]; memset(flag,1,sizeof(flag));
  for( i=0,s=0; i<sizeof(qmaq); i++ ) s+=flag[qmaq[i]], flag[qmaq[i]]=0;
  g.writ( &tmp, s*10 );
//  for( i=0; i<sizeof(flag); i++ ) printf( "%i", flag[i] );
}
*/

  g.close();

  return 0;
}

