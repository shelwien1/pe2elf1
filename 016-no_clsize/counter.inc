
struct Counter {
  word state; 
  const uint P() const { return pp[state]; } //FSM[state].pp; }

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

