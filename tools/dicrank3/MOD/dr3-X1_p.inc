// dicrank3's final MixN: stream x stage x small context

static const byte X1_e[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int X1_W0 = (11841+0) * (1);
static const int X1_Wclip = (258+0) * (1);
static const int X1_Bclip = (12+0) * (1);
static const int X1_Pmin = (21475+0) * (1);
static const int X1_M1w = (523792+0) * (1);
static const int X1_M2w = (40960+0) * (1);
static const int X1_NWw = (262176+0) * (1);
static const int X1_INCw = (2048+0) * (1);
static const int X1_STEPw = (461056+0) * (1);
static const int X1_G2w = (43+0) * (1);
static const int X1_DCw = (377+0) * (1);
static const int X1_RCw = (6210+0) * (1);
static const int X1_NAGw = (1+0) * (1);
static const int X1_M1b = (352432+0) * (1);
static const int X1_M2b = (98304+0) * (1);
static const int X1_NWb = (663904+0) * (1);
static const int X1_INCb = (147969+0) * (1);
static const int X1_STEPb = (485521+0) * (1);
static const int X1_G2b = (0+0) * (1);
static const int X1_DCb = (97+0) * (1);
static const int X1_RCb = (2+0) * (1);
static const int X1_NAGb = (1+0) * (1);
static const int X1_AGAw = (1220+0) * (1);
static const int X1_AGAb = (528+0) * (1);
static const int X1_AGB = (8+0) * (1);
static const int X1_E2E = (256+0) * (1);
static const int X1_XW = (0+0) * (1);
static const int X1_GON = (1+0) * (1);
static const int X1_G0 = (1150+0) * (1);
static const int X1_Glo = (512+0) * (1);
static const int X1_Ghi = (768+0) * (1);
static const int X1_M1g = (352432+0) * (1);
static const int X1_M2g = (98304+0) * (1);
static const int X1_NWg = (2500+0) * (1);
static const int X1_INCg = (147969+0) * (1);
static const int X1_STEPg = (2000+0) * (1);
static const int X1_G2g = (0+0) * (1);
static const int X1_DCg = (97+0) * (1);
static const int X1_RCg = (2+0) * (1);
static const int X1_NAGg = (1+0) * (1);



static uint X1_MakeCx( uint sid, uint st, uint e ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<2) + (((sid)&3)); // 11
  Cx = (Cx<<2) + (((st)&3)); // 11
  Cx = Cx*16 + X1_e[__min(15,__max(0,e-(1-1)))];
  return uint(Cx);
}

