// tb_huf_comp.cpp — C++ harness for the Verilator-translated Huffman ENCODER
// engine (cr_huf_comp), the entropy-coding stage of XP10 encode. Same port
// shape as the decoder (83b AXI4-Stream + 88b RBUS ring), so it reuses
// zipline_axi.h. Liveness only; a functional encode needs the same RBUS
// programming + ib framing layering described in README "Roadmap".
#include "Vcr_huf_comp.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "zipline_axi.h"
#include <cstdio>

static Vcr_huf_comp* dut;
static VerilatedVcdC* tfp;
static qword tick = 0;
static void edge(int v){ dut->clk=v; dut->eval(); if(tfp) tfp->dump(tick++); }
static void cyc(int n=1){ for(int i=0;i<n;i++){ edge(0); edge(1); } }

int main(int argc, char** argv){
  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);
  dut = new Vcr_huf_comp;
  tfp = new VerilatedVcdC; dut->trace(tfp, 8); tfp->open("huf_comp.vcd");

  dut->clk=0; dut->rst_n=0;
  dut->scan_en=0; dut->scan_mode=0; dut->scan_rst_n=1;
  dut->ovstb=1; dut->lvm=0; dut->mlvm=0;
  dut->su_ready=1;
  dut->huf_comp_in_module_id=0; dut->huf_comp_out_module_id=0;
  dut->cfg_start_addr=0x00000; dut->cfg_end_addr=0xFFFFF;
  dut->im_consumed_huf=0; dut->im_consumed_he_sh=0; dut->im_consumed_he_lng=0;
  dut->im_consumed_he_st_sh=0; dut->im_consumed_he_st_lng=0;
  DpBus::set(dut->huf_comp_ib_in, 0,0,0,0,0,0);  // idle input
  dut->huf_comp_ob_in = 1;                       // downstream ready
  RbusRing::set(dut->rbus_ring_i, 0,0,0,0,0,0,0);

  cyc(20); dut->rst_n=1; cyc(40);

  uint in_ready=0, ob_valid=0;
  for(int i=0;i<2000;i++){ cyc(1);
    if(dut->huf_comp_ib_out & 1u) in_ready=1;
    if(DpBus::tvalid(dut->huf_comp_ob_out)) ob_valid=1; }

  printf("[enc] cycles               : %llu\n",(unsigned long long)(tick/2));
  printf("[enc] reset released, no crash\n");
  printf("[enc] ib tready observed hi: %s\n", in_ready?"YES":"no");
  printf("[enc] ob tvalid observed hi: %s\n", ob_valid?"YES":"no");
  printf("[enc] stat_events          : 0x%016llx\n",(unsigned long long)dut->huf_comp_stat_events);
  printf("[enc] VCD                  : huf_comp.vcd\n");
  tfp->close(); dut->final(); delete dut; delete tfp; return 0;
}
