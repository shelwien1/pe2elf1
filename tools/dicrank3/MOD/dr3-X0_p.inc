// dicrank3's MixN group mixers: group x stream x stage x small context

static const byte X0_e[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int X0_W0 = (11841+0) * (1);
static const int X0_Wclip = (258+0) * (1);
static const int X0_Bclip = (12+0) * (1);
static const int X0_Pmin = (21475+0) * (1);
static const int X0_M1w = (523792+0) * (1);
static const int X0_M2w = (40960+0) * (1);
static const int X0_NWw = (262176+0) * (1);
static const int X0_INCw = (2048+0) * (1);
static const int X0_STEPw = (461056+0) * (1);
static const int X0_G2w = (43+0) * (1);
static const int X0_DCw = (377+0) * (1);
static const int X0_RCw = (6210+0) * (1);
static const int X0_NAGw = (1+0) * (1);
static const int X0_M1b = (352432+0) * (1);
static const int X0_M2b = (98304+0) * (1);
static const int X0_NWb = (663904+0) * (1);
static const int X0_INCb = (147969+0) * (1);
static const int X0_STEPb = (485521+0) * (1);
static const int X0_G2b = (0+0) * (1);
static const int X0_DCb = (97+0) * (1);
static const int X0_RCb = (2+0) * (1);
static const int X0_NAGb = (1+0) * (1);
static const int X0_AGAw = (1220+0) * (1);
static const int X0_AGAb = (528+0) * (1);
static const int X0_AGB = (8+0) * (1);
static const int X0_E2E = (256+0) * (1);
static const int X0_XW = (0+0) * (1);
static const int X0_GON = (1+0) * (1);
static const int X0_G0 = (1150+0) * (1);
static const int X0_Glo = (512+0) * (1);
static const int X0_Ghi = (768+0) * (1);
static const int X0_M1g = (352432+0) * (1);
static const int X0_M2g = (98304+0) * (1);
static const int X0_NWg = (2500+0) * (1);
static const int X0_INCg = (147969+0) * (1);
static const int X0_STEPg = (2000+0) * (1);
static const int X0_G2g = (0+0) * (1);
static const int X0_DCg = (97+0) * (1);
static const int X0_RCg = (2+0) * (1);
static const int X0_NAGg = (1+0) * (1);



static uint X0_MakeCx( uint g, uint sid, uint st, uint e ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<3) + (((g)&7)); // 111
  Cx = (Cx<<2) + (((sid)&3)); // 11
  Cx = (Cx<<2) + (((st)&3)); // 11
  Cx = Cx*16 + X0_e[__min(15,__max(0,e-(1-1)))];
  return uint(Cx);
}

