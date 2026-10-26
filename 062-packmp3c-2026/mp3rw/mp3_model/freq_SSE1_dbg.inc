
//static const int M_sm6Q = 8;
//static const int M_sm6x_Volume = 123;
// SSEi<8>* s6 = new SSEi<8>[ %M%sm6x_Volume ];       // old
// SSEj<*(int*)& %M%sm6Q,*(int*)& %M%sm6x_Volume> s6; // new

template< int& Q, int& N >
struct SSEjQ {

  uint init_sign; // 0xD1515A11 if init done
  uint _i; // current subscript index

  byte* p;

  void Init_SSEi( int Wi=0, int Ti=0 ) { Init(Wi,Ti); }

  void Init( int Wi=0, int Ti=0 ) {
    if( init_sign!=0xD1515A11 ) {
      init_sign=0xD1515A11;

      //p = new SSEii<Q>[ N ]; 
      switch( Q ) {
        case  2: ((SSEii< 2>* &)p) = new SSEii< 2>[N]; break; 
        case  3: ((SSEii< 3>* &)p) = new SSEii< 3>[N]; break; 
        case  4: ((SSEii< 4>* &)p) = new SSEii< 4>[N]; break; 
        case  5: ((SSEii< 5>* &)p) = new SSEii< 5>[N]; break; 
        case  6: ((SSEii< 6>* &)p) = new SSEii< 6>[N]; break; 
        case  7: ((SSEii< 7>* &)p) = new SSEii< 7>[N]; break; 
        case  8: ((SSEii< 8>* &)p) = new SSEii< 8>[N]; break; 
        case  9: ((SSEii< 9>* &)p) = new SSEii< 9>[N]; break; 
        case 10: ((SSEii<10>* &)p) = new SSEii<10>[N]; break; 
        case 11: ((SSEii<11>* &)p) = new SSEii<11>[N]; break; 
        case 12: ((SSEii<12>* &)p) = new SSEii<12>[N]; break; 
        case 13: ((SSEii<13>* &)p) = new SSEii<13>[N]; break; 
        case 14: ((SSEii<14>* &)p) = new SSEii<14>[N]; break; 
        case 15: ((SSEii<15>* &)p) = new SSEii<15>[N]; break; 
        case 16: ((SSEii<16>* &)p) = new SSEii<16>[N]; break; 
        case 17: ((SSEii<17>* &)p) = new SSEii<17>[N]; break; 
      }

      if( p==0 ) exit(1);
    }

//    p[_i].Init( Wi );
    switch( Q ) {
      case  2: ((SSEii< 2>*)p)[_i].Init( Wi, Ti ); break; 
      case  3: ((SSEii< 3>*)p)[_i].Init( Wi, Ti ); break; 
      case  4: ((SSEii< 4>*)p)[_i].Init( Wi, Ti ); break; 
      case  5: ((SSEii< 5>*)p)[_i].Init( Wi, Ti ); break; 
      case  6: ((SSEii< 6>*)p)[_i].Init( Wi, Ti ); break; 
      case  7: ((SSEii< 7>*)p)[_i].Init( Wi, Ti ); break; 
      case  8: ((SSEii< 8>*)p)[_i].Init( Wi, Ti ); break; 
      case  9: ((SSEii< 9>*)p)[_i].Init( Wi, Ti ); break; 
      case 10: ((SSEii<10>*)p)[_i].Init( Wi, Ti ); break; 
      case 11: ((SSEii<11>*)p)[_i].Init( Wi, Ti ); break; 
      case 12: ((SSEii<12>*)p)[_i].Init( Wi, Ti ); break; 
      case 13: ((SSEii<13>*)p)[_i].Init( Wi, Ti ); break; 
      case 14: ((SSEii<14>*)p)[_i].Init( Wi, Ti ); break; 
      case 15: ((SSEii<15>*)p)[_i].Init( Wi, Ti ); break; 
      case 16: ((SSEii<16>*)p)[_i].Init( Wi, Ti ); break; 
      case 17: ((SSEii<17>*)p)[_i].Init( Wi, Ti ); break; 
    }
  }



  int SSE_Pred( int iP, SSEii_updstr& X ) {
//  int SSE_Pred( int iP, SSEi_updstr& X ) {
    //return p[_i].SSE_Pred( iP, X );
    switch( Q ) {
      case  2: return ((SSEii< 2>*)p)[_i].SSE_Pred( iP, X );
      case  3: return ((SSEii< 3>*)p)[_i].SSE_Pred( iP, X );
      case  4: return ((SSEii< 4>*)p)[_i].SSE_Pred( iP, X );
      case  5: return ((SSEii< 5>*)p)[_i].SSE_Pred( iP, X );
      case  6: return ((SSEii< 6>*)p)[_i].SSE_Pred( iP, X );
      case  7: return ((SSEii< 7>*)p)[_i].SSE_Pred( iP, X );
      case  8: return ((SSEii< 8>*)p)[_i].SSE_Pred( iP, X );
      case  9: return ((SSEii< 9>*)p)[_i].SSE_Pred( iP, X );
      case 10: return ((SSEii<10>*)p)[_i].SSE_Pred( iP, X );
      case 11: return ((SSEii<11>*)p)[_i].SSE_Pred( iP, X );
      case 12: return ((SSEii<12>*)p)[_i].SSE_Pred( iP, X );
      case 13: return ((SSEii<13>*)p)[_i].SSE_Pred( iP, X );
      case 14: return ((SSEii<14>*)p)[_i].SSE_Pred( iP, X );
      case 15: return ((SSEii<15>*)p)[_i].SSE_Pred( iP, X );
      case 16: return ((SSEii<16>*)p)[_i].SSE_Pred( iP, X );
      case 17: return ((SSEii<17>*)p)[_i].SSE_Pred( iP, X );
    }
    return iP;
  }

  void SSE_Update( byte c, const word* wr0, SSEii_updstr& X ) {
//  void SSE_Update( byte c, int wr0, SSEi_updstr& X ) {
    //p[_i].SSE_Update( c, wr0, X );
    switch( Q ) {
      case  2: ((SSEii< 2>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  3: ((SSEii< 3>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  4: ((SSEii< 4>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  5: ((SSEii< 5>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  6: ((SSEii< 6>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  7: ((SSEii< 7>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  8: ((SSEii< 8>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case  9: ((SSEii< 9>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 10: ((SSEii<10>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 11: ((SSEii<11>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 12: ((SSEii<12>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 13: ((SSEii<13>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 14: ((SSEii<14>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 15: ((SSEii<15>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 16: ((SSEii<16>*)p)[_i].SSE_Update( c, wr0, X ); break; 
      case 17: ((SSEii<17>*)p)[_i].SSE_Update( c, wr0, X ); break; 
    }
  }

  template< class RangeCoder >
  void Process( RangeCoder& rc, Node2ii &M, const word* MTb, const int Mmw, int &s, const word* Tbl, uint f_update=0 ) {
    SSEii_updstr X;
    int f = SSE_Pred( M.P, X );
    if( f_update==0 ) rc.BProcess( f, s );
    SSE_Update( s, Tbl, X );
    M.Update( s, MTb, Mmw );
  }

};

template< int& Q, int& N >
struct SSEj { 

  SSEjQ<Q,N> S;

  SSEjQ<Q,N>& operator[]( uint i ) {
    S._i = i;
    return S;
  }

};

