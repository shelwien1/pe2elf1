// Generated into MOD/tc_model-M0_p.inc: the M0 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).

static const byte M0_e[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };


static const int M0_W0 = (10756+0) * (1);
static const int M0_Wclip = (326+0) * (1);
static const int M0_Bclip = (190+0) * (1);
static const int M0_Pmin = (21475+0) * (1);
static const int M0_M1w = (523792+0) * (1);
static const int M0_M2w = (73728+0) * (1);
static const int M0_NWw = (329760+0) * (1);
static const int M0_INCw = (81924+0) * (1);
static const int M0_STEPw = (461056+0) * (1);
static const int M0_G2w = (126+0) * (1);
static const int M0_DCw = (377+0) * (1);
static const int M0_RCw = (128+0) * (1);
static const int M0_NAGw = (1+0) * (1);
static const int M0_M1b = (524160+0) * (1);
static const int M0_M2b = (32784+0) * (1);
static const int M0_NWb = (663904+0) * (1);
static const int M0_INCb = (147969+0) * (1);
static const int M0_STEPb = (485521+0) * (1);
static const int M0_G2b = (0+0) * (1);
static const int M0_DCb = (97+0) * (1);
static const int M0_RCb = (16384+0) * (1);
static const int M0_NAGb = (1+0) * (1);
static const int M0_AGAw = (1220+0) * (1);
static const int M0_AGAb = (528+0) * (1);
static const int M0_AGB = (8+0) * (1);
static const int M0_E2E = (256+0) * (1);



static uint M0_MakeCx( uint k, uint st, uint e ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<1) + (((k)&1)); // 1
  Cx = Cx*4 + ((st>0+(1-1))+(st>1+(1-1))+(st>2+(1-1)));
  Cx = Cx*17 + M0_e[__min(16,__max(0,e-(1-1)))];
  return uint(Cx);
}

