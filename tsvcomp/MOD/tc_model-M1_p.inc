// Generated into MOD/tc_model-M1_p.inc: the M1 knobs and context masks
// (MakeTables), the final-mixer context builder (MakeIndex).

static const byte M1_e[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };


static const int M1_W0 = (10756+0) * (1);
static const int M1_Wclip = (326+0) * (1);
static const int M1_Bclip = (190+0) * (1);
static const int M1_Pmin = (21475+0) * (1);
static const int M1_M1w = (523792+0) * (1);
static const int M1_M2w = (73728+0) * (1);
static const int M1_NWw = (329760+0) * (1);
static const int M1_INCw = (81924+0) * (1);
static const int M1_STEPw = (461056+0) * (1);
static const int M1_G2w = (126+0) * (1);
static const int M1_DCw = (377+0) * (1);
static const int M1_RCw = (128+0) * (1);
static const int M1_NAGw = (1+0) * (1);
static const int M1_M1b = (524160+0) * (1);
static const int M1_M2b = (32784+0) * (1);
static const int M1_NWb = (663904+0) * (1);
static const int M1_INCb = (147969+0) * (1);
static const int M1_STEPb = (485521+0) * (1);
static const int M1_G2b = (0+0) * (1);
static const int M1_DCb = (97+0) * (1);
static const int M1_RCb = (16384+0) * (1);
static const int M1_NAGb = (1+0) * (1);
static const int M1_AGAw = (1220+0) * (1);
static const int M1_AGAb = (528+0) * (1);
static const int M1_AGB = (8+0) * (1);
static const int M1_E2E = (256+0) * (1);



static uint M1_MakeCx( uint k, uint st, uint e ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<1) + (((k)&1)); // 1
  Cx = Cx*4 + ((st>0+(1-1))+(st>1+(1-1))+(st>2+(1-1)));
  Cx = Cx*17 + M1_e[__min(16,__max(0,e-(1-1)))];
  return uint(Cx);
}

