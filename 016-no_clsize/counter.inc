
struct Counter {
  word state; 
  const uint P() const { return pp[state]; } //FSM[state].pp; }
  void Update( uint bit ) { 
//    state = FSM[state].s[bit]; 
    uint delta = ((byte*)&FSM[state])-((byte*)&FSM[0]);
    byte* __restrict p = (byte*)&FSM[0].s[bit];
    p += delta;
    state = (word&)p[0];
  }
  void Update( uint _state, uint bit ) { 
//    state = FSM[state].s[bit]; 
    uint delta = ((byte*)&FSM[_state])-((byte*)&FSM[0]);
    byte* __restrict p = (byte*)&FSM[0].s[bit];
    p += delta;
    state = (word&)p[0];
  }
};

