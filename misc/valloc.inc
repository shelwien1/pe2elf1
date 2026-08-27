
void* VAlloc( size_t dwSize, uint nAlign=0x1000 ) {
  char* r; // = _aligned_malloc( 0x1000, dwSize );

  size_t d, l = dwSize+sizeof(void*)+(nAlign-1)+(16-1);
  l -= l % 16;

//  r = (char*)malloc( l );
  r = (char*)new byte[l];
  if( r ) {
    d = (r - ((char*)0)) & (nAlign-1);
    l = d + sizeof(void*) + (nAlign-1); l-=l % nAlign;
    d = l - d;
    *(char**)(r+d-sizeof(void*)) = r;
    r+= d;
  }

  return r;
}

unsigned VFree( void* lpAddress ) {
  char* r = (char*)lpAddress;
  if( r ) {
    r = *(char**)(r-sizeof(void*));
//    free( r );
    delete[] r;
  }
  return 1;
}


