// Shared declarations for the Tangelo model.
//
// The model is compiled TWICE into one program: once with -DSIM_FUNC (the
// instrumented encode_sim whose stores are journaled, see ../ALGORITHM.md §5)
// and once normally (the real, persistent model update). Both copies must work
// on the SAME state, so every mutable global is only DECLARED here and is
// DEFINED exactly once, in the main translation unit (../tangelo_w.cpp).
// Free functions are static: they are pure, so a per-TU copy cannot diverge.

enum {
  SCALElog = 12,            // squash() and the APMs output a 12-bit probability
  SCALE    = 1<<SCALElog,   // ... so the coder's probability scale is 4096
  hSCALE   = SCALE/2
};

#define MEM (0x10000 << 7)

extern int y;     // the bit just coded; Model::predictNext() folds it into the state
extern int bpos;  // bit position inside the current byte, 0..7

static int min( int a, int b ) { return a < b ? a : b; }
static int max( int a, int b ) { return a < b ? b : a; }

// C-locale character classes, spelled out. Two reasons not to call <ctype.h>
// here: the classification must not depend on the process locale (it decides a
// model context, so a different locale would change the compressed output), and
// a call out of the instrumented translation unit could write memory that the
// journal never sees.
static int is_alpha( int c ) { return (c>='A'&&c<='Z') || (c>='a'&&c<='z'); }
static int is_digit( int c ) { return c>='0' && c<='9'; }
static int is_space( int c ) { return c==' ' || (c>=0x09 && c<=0x0D); }
static int is_punct( int c ) { return c>0x20 && c<0x7F && !is_alpha(c) && !is_digit(c); }
