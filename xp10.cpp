// xp10.cpp  --  file codec built on the Verilator-translated Project Zipline RTL.
//
//   ./xp10 d in.xp10 out.beats [in.config]    decode  (drives cr_cddip)
//   ./xp10 c in.beats out.xp10 [in.config]    encode  (drives cr_cceip_64; needs -DHAVE_ENCODER)
//
// I/O is the AXI4-Stream "beat" text format used by the repo's dv vectors, one
// beat per line:   0x<16 hex tdata>  [SoT|EoT]  0x<2 hex tstrb>
// (# starts a comment). This is the XP10 on-wire TLV framing; it round-trips and
// is directly diffable against dv/CDD_64/tests/*.outbound. Use -r to also emit
// the raw decompressed payload (data-frame bytes, honoring tstrb, command/stats
// frames stripped) to <out>.raw.
//
// Protocol (ported from dv/CDD_64/run/zipline_tb.v):
//   tuser bit0=SoT, bit1=EoT.  ib_tlast asserts on the EoT beat of the CQE
//   command frame (tdata[7:0]==0x09).  Output CQE EoT ends the stream; a stats
//   frame (tdata[7:0]==0x08) is informational.

#include "verilated.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>

typedef uint64_t qword; typedef uint32_t uint; typedef uint8_t byte;

// ---- DUT selection ---------------------------------------------------------
#if defined(ENCODE_BUILD)
  #include "Vcr_cceip_64.h"
  typedef Vcr_cceip_64 Dut;
#else
  #include "Vcr_cddip.h"
  typedef Vcr_cddip Dut;
#endif

struct Beat { qword tdata; byte tstrb; byte tuser; bool tlast; };

static const byte SOT = 0x1, EOT = 0x2;

// ---- beat-file parse / write ----------------------------------------------
static std::vector<Beat> read_beats(const char* path) {
  std::vector<Beat> v; FILE* f = fopen(path, "r");
  if (!f) { fprintf(stderr, "xp10: cannot open %s\n", path); exit(2); }
  char line[512];
  while (fgets(line, sizeof line, f)) {
    char* p = line; while (*p==' '||*p=='\t') ++p;
    if (*p=='#' || *p=='\n' || *p=='\0') continue;
    // tokenize on whitespace
    char* toks[8]; int n=0; char* s=p;
    for (char* q=p; ; ++q) {
      if (*q==' '||*q=='\t'||*q=='\n'||*q=='\r'||*q=='\0') {
        if (q>s && n<8) toks[n++]=s;
        bool end = (*q=='\0'||*q=='\n');
        *q='\0'; s=q+1; if (end) break;
      }
    }
    if (n<1) continue;
    Beat b; b.tuser=0; b.tstrb=0xff; b.tlast=false;
    b.tdata = strtoull(toks[0], nullptr, 16);          // first token = tdata
    for (int i=1;i<n;i++) {
      if (!strcmp(toks[i],"SoT")) b.tuser|=SOT;
      else if (!strcmp(toks[i],"EoT")) b.tuser|=EOT;
      else b.tstrb = (byte)strtoul(toks[i], nullptr, 16); // last 0x = tstrb
    }
    v.push_back(b);
  }
  fclose(f); return v;
}

static const char* user_str(byte u) {
  if ((u&(SOT|EOT))==(SOT|EOT)) return "SoB"; // both (single-beat frame)
  if (u&SOT) return "SoT"; if (u&EOT) return "EoT"; return "   ";
}
static void write_beats(const char* path, const std::vector<Beat>& v) {
  FILE* f = fopen(path,"w");
  if (!f){ fprintf(stderr,"xp10: cannot write %s\n",path); exit(2);}
  for (const Beat& b : v)
    fprintf(f,"0x%016llx  %s      0x%02x\n",(unsigned long long)b.tdata,user_str(b.tuser),b.tstrb);
  fclose(f);
}

// strb -> number of valid low bytes (contiguous from LSB). 0xff=8, 0x0f=4, ...
static int strb_bytes(byte s){ int n=0; for(int i=0;i<8;i++) if(s&(1u<<i)) n++; return n; }

// raw payload = bytes of TLV decompressed-data frames (header type byte == 0x05);
// the SoT beat is the frame header, payload follows until EoT. All other TLV
// types (0x00 request, 0x08 stats, 0x09 completion, ...) are metadata, skipped.
static const int DATA_TYPE = 0x05;
static void write_raw(const char* path, const std::vector<Beat>& v) {
  std::string out; bool is_data=false; bool header=false;
  for (const Beat& b : v) {
    if (b.tuser&SOT){ header=true; is_data=((int)(b.tdata&0xff)==DATA_TYPE); }
    if (is_data && !header) {                       // payload beats (not the header)
      int nb=strb_bytes(b.tstrb);
      for (int i=0;i<nb;i++) out.push_back((char)((b.tdata>>(8*i))&0xff));
    }
    header=false;
    if (b.tuser&EOT){ is_data=false; }
  }
  FILE* f=fopen(path,"wb"); if(!f){fprintf(stderr,"xp10: cannot write %s\n",path);exit(2);}
  fwrite(out.data(),1,out.size(),f); fclose(f);
  fprintf(stderr,"[xp10] raw payload: %zu bytes -> %s\n", out.size(), path);
}

// ---- simulation plumbing ---------------------------------------------------
static Dut* dut; static qword tickcnt=0;
static void step(){ dut->clk=0; dut->eval(); dut->clk=1; dut->eval(); tickcnt++; }

static void reset_and_tie() {
  dut->rst_n=0;
  dut->scan_en=0; dut->scan_mode=0; dut->scan_rst_n=1;
  dut->ovstb=1; dut->lvm=0; dut->mlvm=0;
  dut->dbg_cmd_disable=0; dut->xp9_disable=0;
  dut->ib_tvalid=0; dut->ib_tlast=0; dut->ib_tid=0; dut->ib_tstrb=0; dut->ib_tuser=0; dut->ib_tdata=0;
  dut->ob_tready=1; dut->sch_update_tready=1;
  dut->apb_psel=0; dut->apb_penable=0; dut->apb_pwrite=0; dut->apb_paddr=0; dut->apb_pwdata=0;
#if defined(ENCODE_BUILD)
  dut->key_mode=0;                 // no encryption (KME reduced in this RTL)
#endif
  for (int i=0;i<40;i++) step();
  dut->rst_n=1;
  for (int i=0;i<20;i++) step();
}

// APB access (standard 2-phase). Returns prdata for reads.
static uint apb_xfer(uint addr, uint wdata, bool write) {
  dut->apb_psel=1; dut->apb_penable=0; dut->apb_pwrite=write?1:0;
  dut->apb_paddr=addr; dut->apb_pwdata=wdata; step();
  dut->apb_penable=1; step();
  int guard=0; while(!dut->apb_pready && guard++<10000) step();
  uint rd = dut->apb_prdata;
  dut->apb_psel=0; dut->apb_penable=0; step();
  return rd;
}

// optional <name>.config: lines "r 0xADDR 0xDATA" (read+verify) / "w 0xADDR 0xDATA"
static void do_config(const char* path) {
  FILE* f=fopen(path,"r"); if(!f) return;
  char line[256];
  while (fgets(line,sizeof line,f)) {
    char op[8]; unsigned long long a,d;
    if (sscanf(line," %7s 0x%llx 0x%llx",op,&a,&d)==3) {
      if (op[0]=='r'||op[0]=='R') { uint got=apb_xfer((uint)a,0,false);
        if (got!=(uint)d) fprintf(stderr,"[xp10] APB read 0x%llx = 0x%08x (expected 0x%08llx)\n",a,got,d);
      } else if (op[0]=='w'||op[0]=='W') apb_xfer((uint)a,(uint)d,true);
    }
  }
  fclose(f);
}

// ---- core: stream beats in, collect beats out ------------------------------
static std::vector<Beat> run_stream(const std::vector<Beat>& in) {
  std::vector<Beat> out;
  size_t i=0; bool saw_cqe_in=false;
  bool done=false; int idle=0; const int IDLE_MAX=2000; qword cap=20000000;

  while (!done && tickcnt<cap) {
    // drive input beat i (AXI master: hold until accepted)
    if (i<in.size()) {
      Beat b=in[i];
      bool tlast=false;
      if ((b.tuser&SOT) && (b.tdata&0xff)==0x09) saw_cqe_in=true;
      if ((b.tuser&EOT) && saw_cqe_in){ tlast=true; saw_cqe_in=false; }
      dut->ib_tvalid=1; dut->ib_tdata=b.tdata; dut->ib_tstrb=b.tstrb;
      dut->ib_tuser=b.tuser; dut->ib_tlast=tlast?1:0;
    } else { dut->ib_tvalid=0; dut->ib_tlast=0; }
    dut->ob_tready=1;

    // settle comb, sample handshakes at this cycle, then advance clock
    dut->clk=0; dut->eval();
    bool in_acc  = (i<in.size()) && dut->ib_tvalid && dut->ib_tready;
    bool out_val = dut->ob_tvalid && dut->ob_tready;
    Beat ob; if (out_val){ ob.tdata=dut->ob_tdata; ob.tstrb=dut->ob_tstrb;
                           ob.tuser=dut->ob_tuser & (SOT|EOT); ob.tlast=dut->ob_tlast; }
    dut->clk=1; dut->eval(); tickcnt++;

    if (in_acc) i++;
    if (out_val) {
      out.push_back(ob); idle=0;
      // end when the output CQE completion frame closes (tlast / EoT of 0x09)
      if (ob.tlast) done=true;
    } else {
      if (i>=in.size()) { if (++idle>IDLE_MAX) done=true; }
    }
  }
  return out;
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  // parse: mode in out [config] [-r]
  bool raw=false; std::vector<const char*> pos;
  for (int i=1;i<argc;i++){ if(!strcmp(argv[i],"-r")||!strcmp(argv[i],"--raw")) raw=true; else pos.push_back(argv[i]); }
  if (pos.size()<3){ fprintf(stderr,
      "Usage: %s c|d input output [config] [-r]\n"
      "  d: decode (drives cr_cddip).  c: encode (needs -DHAVE_ENCODER build).\n"
      "  -r: also write raw payload bytes to <output>.raw\n", argv[0]); return 1; }
  const char* mode=pos[0]; const char* in_path=pos[1]; const char* out_path=pos[2];
  const char* cfg = pos.size()>=4 ? pos[3] : nullptr;

#if defined(ENCODE_BUILD)
  if (mode[0]!='c'){ fprintf(stderr,"this binary was built for encode only\n"); return 1; }
#else
  if (mode[0]!='d'){ fprintf(stderr,"this binary decodes only; build with -DENCODE_BUILD for 'c'\n"); return 1; }
#endif

  dut = new Dut;
  reset_and_tie();
  if (cfg) do_config(cfg);

  std::vector<Beat> in = read_beats(in_path);
  fprintf(stderr,"[xp10] %s: %zu input beats\n", mode[0]=='d'?"decode":"encode", in.size());
  std::vector<Beat> out = run_stream(in);
  fprintf(stderr,"[xp10] produced %zu output beats in %llu cycles\n",
          out.size(),(unsigned long long)tickcnt);

  write_beats(out_path, out);
  if (raw){ std::string rp=std::string(out_path)+".raw"; write_raw(rp.c_str(), out); }

  dut->final(); delete dut; return 0;
}
