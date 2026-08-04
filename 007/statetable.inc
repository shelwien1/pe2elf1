// statetable.inc -- the bit-history state machine, generated rather than tabulated.
//
// Ported from fxcmv1's `StateTable`.  Instead of 1012 tabulated bytes it takes
// seven numbers and *derives* the 256-state automaton from them, so the shape
// of the machine is described by its construction rule rather than by its
// output.  The seven live in IDX/paq8-N0.idx.
//
// WHAT THE SEVEN NUMBERS MEAN
//
// A state is a pair (x,y) = (zeros seen, ones seen), bounded and quantised:
//
//   B0..B4  the largest x allowed at y = 0,1,2,3,4.  y is capped at 4 (`B`=5
//           in the original), so the machine tracks a long run of one bit and
//           at most four of the other -- the classic paq8 asymmetry.
//   B5      the doubling threshold: a cell with y>0 and x+y < B5 gets TWO
//           states rather than one, which is how the low-count region gets
//           finer resolution than the tail.
//   MDC     maximum discount.  When the opposite bit arrives, the majority
//           count is not kept: `discount()` maps x to the number of i in
//           [1,MDC) with x>=i, i.e. min(x, MDC-1) for x>2.  That is what
//           stops a long run from making the state unreachable in one step.
//
// `generate()` assigns state numbers by walking (x,y) in order of x+y, then
// for each state computes where a 0 and a 1 take it, falling back down the
// (x,y) lattice until it lands on a cell that exists.  ns[s*4] = next on 0,
// [s*4+1] = next on 1, [s*4+2] = x, [s*4+3] = y -- exactly the layout
// State_table already had, which is why this drops in.
//
// Clamps (IDX-FORMAT.md section 5): opt.pl will visit the ends of these
// patterns, and two of them are load-bearing for termination.  `next_state`
// walks down the lattice with `while(!t[x][y][1])`, so a B that admits no
// states at all would walk x negative and index out of bounds; B >= 1
// guarantees (0,0) exists and the walk terminates on it.  MDC only bounds a
// loop counter.  Both are clamped at the point of use, in N0_Init below.

struct StateTable {
  int mdc;                       // maximum discount
  enum { B = 5, N = 64 };        // y is capped at B; the lattice is N x N
  int b[6];                      // b[0..4]: max x per y.  b[5]: doubling threshold
  U8 ns[1024];                   // state*4 -> next0, next1, x, y
  U8 t[N][N][2];                 // (x,y) -> first state number, how many states

  // How many state numbers the cell (x,y) is worth: 0 if it is outside the
  // declared bounds, else 1, or 2 in the low-count region.
  int num_states(int x, int y) {
    if( x<y ) return num_states(y, x);
    if( x<0||y<0||x>=N||y>=N||y>=B||x>=b[y] ) return 0;
    return 1+(y>0&&x+y<b[5]);
  }

  // New value of count x once the opposite bit has been observed.
  void discount(int& x) {
    int y = 0;
    if( x>2 ) {
      for( int i = 1; i<mdc; i++ )
        y += x>=i;
      x = y;
    }
  }

  // Where (x,y) goes on input b, then down the lattice until a cell exists.
  void next_state(int& x, int& y, int b_) {
    if( x<y ) {
      next_state(y, x, 1-b_);
    } else {
      if( b_ ) { ++y; discount(x); }
      else     { ++x; discount(y); }
      while( !t[x][y][1] ) {
        if( y<2 ) --x;
        else { x = (x*(y-1)+(y/2))/y; --y; }
      }
    }
  }

  void generate( void ) {
    memset(ns, 0, sizeof(ns));
    memset(t, 0, sizeof(t));

    // number the cells, in order of x+y
    int state = 0;
    for( int i = 0; i<256; ++i ) {
      for( int y = 0; y<=i; ++y ) {
        int x = i-y;
        int n = num_states(x, y);
        if( n ) {
          t[x][y][0] = state;
          t[x][y][1] = n;
          state += n;
        }
      }
    }

    // fill in the transitions
    state = 0;
    for( int i = 0; i<N; ++i ) {
      for( int y = 0; y<=i; ++y ) {
        int x = i-y;
        for( int k = 0; k<t[x][y][1]; ++k ) {
          int x0 = x, y0 = y, x1 = x, y1 = y;
          next_state(x0, y0, 0);
          next_state(x1, y1, 1);
          ns[state*4]   = t[x0][y0][0];
          ns[state*4+1] = t[x1][y1][0]+(t[x1][y1][1]>1);
          ns[state*4+2] = x;
          ns[state*4+3] = y;
          // The original's asserts are gone (NDEBUG is always on here); this
          // is the one that is not a debug check -- a parameter set that runs
          // out of lattice before 256 states leaves the rest of ns zero, and
          // stopping is what the original does too.
          if( state>0xff||t[x][y][1]==0||t[x0][y0][1]==0||t[x1][y1][1]==0 ) return;
          ++state;
          if( state>0xff ) return;
        }
      }
    }
  }

  void Init( int s0, int s1, int s2, int s3, int s4, int s5, int s6, U8* table ) {
    b[0]=s0; b[1]=s1; b[2]=s2; b[3]=s3; b[4]=s4; b[5]=s5; mdc=s6;
    generate();
    memcpy(table, ns, 1024);
  }
};

// The live table.  Built once at static-initialisation time from the IDX
// parameters, which are declared by the generated headers at the top of this
// file and are therefore constructed first.  Nothing reads it before main():
// StateMap's constructor runs when the model is new'd.
//
// The generator is a local inside the constructor rather than a member -- it
// carries an 8 KB (x,y) lattice that is scaffolding, not state, and keeping it
// around would put it in every Predictor instantiation.
static struct StateTableGen {
  U8 t[256][4];
  StateTableGen() {
    StateTable g;
    g.Init( pclamp(N0_B0,  1, 63), pclamp(N0_B1, 1, 63), pclamp(N0_B2, 1, 63),
            pclamp(N0_B3,  1, 63), pclamp(N0_B4, 1, 63), pclamp(N0_B5, 1, 63),
            pclamp(N0_MDC, 1, 63), &t[0][0] );
  }
} state_table;
