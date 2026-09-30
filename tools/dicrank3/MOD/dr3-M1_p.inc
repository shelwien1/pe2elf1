// dicrank3's final Mix2 (mixer output vs its SSE): stream x stage x small context

static const byte M1_e[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int M1_W0 = (11152+0) * (1);
static const int M1_Wclip = (1542+0) * (1);
static const int M1_Bclip = (406+0) * (1);
static const int M1_Pmin = (21475+0) * (1);
static const int M1_M1w = (523792+0) * (1);
static const int M1_M2w = (49344+0) * (1);
static const int M1_NWw = (329760+0) * (1);
static const int M1_INCw = (81924+0) * (1);
static const int M1_STEPw = (493576+0) * (1);
static const int M1_G2w = (80+0) * (1);
static const int M1_DCw = (377+0) * (1);
static const int M1_RCw = (128+0) * (1);
static const int M1_NAGw = (1+0) * (1);
static const int M1_M1b = (524160+0) * (1);
static const int M1_M2b = (4352+0) * (1);
static const int M1_NWb = (828855+0) * (1);
static const int M1_INCb = (147969+0) * (1);
static const int M1_STEPb = (485521+0) * (1);
static const int M1_G2b = (0+0) * (1);
static const int M1_DCb = (103+0) * (1);
static const int M1_RCb = (16384+0) * (1);
static const int M1_NAGb = (1+0) * (1);
static const int M1_AGAw = (1220+0) * (1);
static const int M1_AGAb = (0+0) * (1);
static const int M1_AGB = (712+0) * (1);
static const int M1_E2E = (256+0) * (1);



static uint M1_MakeCx( uint sid, uint st, uint e ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<2) + (((sid)&3)); // 11
  Cx = (Cx<<2) + (((st)&3)); // 11
  Cx = Cx*16 + M1_e[__min(15,__max(0,e-(1-1)))];
  return uint(Cx);
}

