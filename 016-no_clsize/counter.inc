
struct Counter {
  word state; 
  static uint P( uint _s ) { return pp[_s]; }
  uint Get( void ) const { return state; }
  void Reset( void ) { state = 0; }

  // FSM[s].s[bit] as ONE scaled index. sizeof(fsm) is 2*sizeof(word) and s[]
  // is the whole struct, so the word at FSM + s*4 + bit*2 is exactly
  // ((word*)FSM)[s*2+bit] -- same address, same word, but base + idx*2 instead
  // of a base plus two scaled terms. The two-term form is what made clang
  // materialise the FSM base into a register on six of the eight bit slots of
  // the unrolled model pass: a `leaq (,%rbit,2)` plus an `addq %rFSM` ahead of
  // every load. One term folds the whole address into the load.
  void Update( uint bit ) { 
    state = ((const word*)FSM)[ state*2 + bit ];
  }
  void Update( uint _state, uint bit ) { 
    state = ((const word*)FSM)[ _state*2 + bit ];
  }
};

// RC_FUSE_PP=1: the same counter with pp[] folded in. What the model carries
// per context is a dword {p:16, state:16} instead of a word, so the step reads
// ONE table instead of two -- and, decisive for the decoder, the probability
// arrives with the counter rather than one load later. `p = pp[state]` was a
// load whose address came out of the load before it, sitting on the per-bit
// serial chain the whole decode loop waits on.
struct Counter2 {
  uint sp;                                   // { pp[state] : state }
  static uint P( uint _sp ) { return _sp & 0xFFFF; }
  uint Get( void ) const { return sp; }
  void Reset( void ) { sp = pp[0]; }         // state 0, and its probability
  void Update( uint _sp, uint bit ) {
    sp = FSM2[ (_sp>>16)*2 + bit ];
  }
};

template< int FUSE > struct SelCounter { typedef Counter2 t; };
template<>           struct SelCounter<0> { typedef Counter t; };

