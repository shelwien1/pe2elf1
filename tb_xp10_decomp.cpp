// tb_xp10_decomp.cpp — C++ harness for the Verilator-translated XP10 DECODER
// core (cr_xp10_decomp). Proves the translated RTL constructs, resets, clocks,
// and reports liveness; writes a VCD. Field accessors in zipline_axi.h.
//
// A functional bit-exact decode additionally needs RBUS register programming,
// a framed XP10 payload on ib, and the im_* credit model — see README "Roadmap".
#include "Vcr_xp10_decomp.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "zipline_axi.h"
#include <cstdio>

static Vcr_xp10_decomp* dut;
static VerilatedVcdC*   tfp;
static qword            tick = 0;
static void edge(int v){ dut->clk=v; dut->eval(); if(tfp) tfp->dump(tick++); }
static void cyc(int n=1){ for(int i=0;i<n;i++){ edge(0); edge(1); } }

int main(int argc, char** argv){
  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);
  dut = new Vcr_xp10_decomp;
  tfp = new VerilatedVcdC; dut->trace(tfp, 8); tfp->open("xp10_decomp.vcd");

  dut->clk=0; dut->rst_n=0;
  dut->scan_en=0; dut->scan_mode=0; dut->scan_rst_n=1;
  dut->ovstb=1; dut->lvm=0; dut->mlvm=0;
  dut->su_afull_n=1; dut->cceip_cfg=0; dut->xp10_decomp_module_id=0;
  dut->cfg_start_addr=0x00000; dut->cfg_end_addr=0xFFFFF;
  dut->im_consumed_xpd=0; dut->im_consumed_lz77d=0; dut->im_consumed_htf_bl=0;
  DpBus::set(dut->xp10_decomp_ib_in, 0,0,0,0,0,0);   // idle input
  dut->xp10_decomp_ob_in = 1;                        // downstream ready
  RbusRing::set(dut->rbus_ring_i, 0,0,0,0,0,0,0);

  cyc(20); dut->rst_n=1; cyc(40);

  uint in_ready=0, ob_valid=0;
  for(int i=0;i<2000;i++){ cyc(1);
    if(dut->xp10_decomp_ib_out & 1u) in_ready=1;
    if(DpBus::tvalid(dut->xp10_decomp_ob_out)) ob_valid=1; }

  printf("[dec] cycles               : %llu\n",(unsigned long long)(tick/2));
  printf("[dec] reset released, no crash\n");
  printf("[dec] ib tready observed hi: %s\n", in_ready?"YES":"no");
  printf("[dec] ob tvalid observed hi: %s\n", ob_valid?"YES":"no");
  printf("[dec] hufd_stat_events     : 0x%016llx\n",(unsigned long long)dut->xp10_decomp_hufd_stat_events);
  printf("[dec] VCD                  : xp10_decomp.vcd\n");
  tfp->close(); dut->final(); delete dut; delete tfp; return 0;
}
