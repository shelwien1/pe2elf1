// bmf-P1.inc -- the template idx2inc.pl expands into MOD/bmf-P1_p.inc.
// The declarations go to MOD/bmf-P1_h.inc; these are the index builders.

uint32_t MakeP1Result( uint32_t msum, uint32_t quiet, uint32_t flag, uint32_t lvl ) {
  int32_t P1Result;

  P1Result = 0;
  P1Result = P1Result*(P1_ctxw_range) + (msum);
  P1Result = P1Result*(2) + (quiet);
  P1Result = P1Result*(2) + (flag);
  P1Result = P1Result*(8) + (lvl);
  return (uint32_t)P1Result;
}

uint32_t MakeP1Alt( uint32_t msum, uint32_t ctx0 ) {
  int32_t P1Alt;

  P1Alt = 0;
  P1Alt = P1Alt*(P1_ctxw_range) + (msum);
  P1Alt = P1Alt*(32) + (ctx0);
  return (uint32_t)P1Alt;
}

uint32_t MakeP1Low( uint32_t slot, uint32_t grp ) {
  int32_t P1Low;

  P1Low = 0;
  P1Low = P1Low*(8) + (slot);
  P1Low = P1Low*(8) + (grp);
  return (uint32_t)P1Low;
}

uint32_t MakeGuess( int32_t guess ) {
  int32_t Guess;

  Guess = 0;
  Guess = Guess*3 + ((guess>22+(1-1))+(guess>216+(1-1)));
  return (uint32_t)Guess;
}
