
#include "mp3_parse/parse__mp3.inc"

#include "mp3rw.inc"
#include "mp3rs.inc"

template< int f_DEC > struct mp3det;
template<> struct mp3det<0> : mp3rw {};
template<> struct mp3det<1> : mp3rs {};
