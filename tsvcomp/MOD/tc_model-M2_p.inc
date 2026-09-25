// Generated into MOD/tc_model-M2_p.inc: the M2 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).



static const int M2_W0 = (10756+0) * (1);
static const int M2_Wclip = (326+0) * (1);
static const int M2_Bclip = (190+0) * (1);
static const int M2_Pmin = (21475+0) * (1);
static const int M2_M1w = (523792+0) * (1);
static const int M2_M2w = (73728+0) * (1);
static const int M2_NWw = (329760+0) * (1);
static const int M2_INCw = (81924+0) * (1);
static const int M2_STEPw = (461056+0) * (1);
static const int M2_G2w = (126+0) * (1);
static const int M2_DCw = (377+0) * (1);
static const int M2_RCw = (128+0) * (1);
static const int M2_NAGw = (1+0) * (1);
static const int M2_M1b = (524160+0) * (1);
static const int M2_M2b = (32784+0) * (1);
static const int M2_NWb = (663904+0) * (1);
static const int M2_INCb = (147969+0) * (1);
static const int M2_STEPb = (485521+0) * (1);
static const int M2_G2b = (0+0) * (1);
static const int M2_DCb = (97+0) * (1);
static const int M2_RCb = (16384+0) * (1);
static const int M2_NAGb = (1+0) * (1);
static const int M2_AGAw = (1220+0) * (1);
static const int M2_AGAb = (528+0) * (1);
static const int M2_AGB = (8+0) * (1);
static const int M2_E2E = (256+0) * (1);



static uint M2_MakeCx( uint k, uint st, uint pq ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<1) + (((k)&1)); // 1
  Cx = Cx*4 + ((st>0+(1-1))+(st>1+(1-1))+(st>2+(1-1)));
  Cx = (Cx<<6) + (((pq)&63)); // 111111
  return uint(Cx);
}

