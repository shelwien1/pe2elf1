
// Encoder and decoder are separate translation units (model0.cpp / model1.cpp),
// each instantiating the model for its own f_DEC. coder.cpp only sees this API.

template< int f_DEC > uint C_get_object_size( void );
template< int f_DEC > uint C_init( void* p, void* r0 );
template< int f_DEC > void C_quit( void* p );
template< int f_DEC > uint C_process( void* p );
template< int f_DEC > uint C_getoutsize( void* p );
template< int f_DEC > void C_addinp( void* p, byte* inp,uint inplen );
template< int f_DEC > void C_addout( void* p, byte* out,uint outlen );

template< int f_DEC > void* C_save( void* p );
