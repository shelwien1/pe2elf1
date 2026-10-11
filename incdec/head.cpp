// dummy.cpp head: incdec infrastructure for vbn.exe (incdec.md §7, adapted to i386).
//
// build.sh assembles dummy.cpp from this head, one #include per accepted .inc
// (accepted.txt order, callees first) and a generated dummy_init() that patches
// each moved entry point.  The result is dummy.so, which `pe2elf32 --inject`
// makes a DT_NEEDED of vbn.elf, so its constructor runs after the PE image and
// libvbnshim.so are mapped and before the PE entry point.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <csignal>
#include <ucontext.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Hex-Rays vocabulary.  defs.h is Hex-Rays' own header, verbatim.  vbn.exe.c
// also includes <windows.h>, which is neither available nor wanted: declare
// _WINDOWS_ and give BYTE/WORD/DWORD the unsigned meaning windows.h has, or
// defs.h would define them as signed types.
// ---------------------------------------------------------------------------
#define _WINDOWS_
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef long           LONG;
typedef int            BOOL;
#include "defs.h"

// Intrinsics the bodies use, with the x86 semantics the decompiler assumed.
// bsr leaves its destination unchanged for a zero source (as the CPU does).
static inline unsigned char _BitScanReverse(unsigned int *idx, unsigned int v)
{ if (!v) return 0; *idx = 31 - __builtin_clz(v); return 1; }
static inline unsigned char _BitScanReverse(int *idx, unsigned int v)
{ return _BitScanReverse((unsigned int *)idx, v); }
// bt/btc/bts with a *memory* operand address a bit string: the offset is
// signed and may run past the first dword.  (The register forms take the
// offset mod 32; a body that uses one gets its own spelling via fixups.txt.)
static inline unsigned char _bittest(const void *base, int bit)
{ return (((const unsigned char *)base)[bit >> 3] >> (bit & 7)) & 1; }
static inline unsigned char _bittestandcomplement(void *base, int bit)
{ unsigned char *p = (unsigned char *)base + (bit >> 3), m = 1 << (bit & 7), o = *p & m; *p ^= m; return o != 0; }
static inline unsigned char _bittestandset(void *base, int bit)
{ unsigned char *p = (unsigned char *)base + (bit >> 3), m = 1 << (bit & 7), o = *p & m; *p |= m; return o != 0; }
static inline unsigned char _bittestandreset(void *base, int bit)
{ unsigned char *p = (unsigned char *)base + (bit >> 3), m = 1 << (bit & 7), o = *p & m; *p &= ~m; return o != 0; }
template <class T, class V> static inline T _InterlockedExchange(T *p, V v)
{ T o = *p; *p = (T)v; return o; }
#define SDWORD1(x) SDWORDn(x, 1)

// ---------------------------------------------------------------------------
// DOS services.  vbn reaches them through its .shim thunks, which push the
// registers (pushfd; pushad) and call vbnshim.dll!vbn_interrupt with a pointer
// to that frame.  A moved body does the same from C: fill a frame in pushad
// order and call the function the IAT slot at 0x0FFF3064 holds.  The shim
// rejects any pointer outside the 64 MB guest image, so only image addresses
// (globals, and the stack, which lives inside the image) may be passed.
// ---------------------------------------------------------------------------
struct DosRegs { uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, eflags; };
typedef void (*vbn_interrupt_t)(uint32_t *);
static inline int vbn_dos(DosRegs &r)          // returns CF
{
  r.eflags = 0x202;
  (*(vbn_interrupt_t *)0x0FFF3064)((uint32_t *)&r);
  return r.eflags & 1;
}

// ---------------------------------------------------------------------------
// The compressor's early exit.  sub_1000703B records the stack slot where the
// *next* call's return address will be pushed (and EBP); sub_10006E22 restores
// both and executes `ret 4`, which returns from that next call as if it had
// finished.  Only ESP and EBP are restored, so a g++-compiled caller cannot make
// the pair of calls itself: it keeps values in EBX/ESI/EDI across the second
// one.  vbn_restartable() makes them from assembly, with those registers saved
// in its own frame, which the early exit never unwinds.
// fn is a stdcall function taking one argument.
// ---------------------------------------------------------------------------
extern "C" __attribute__((naked, used, visibility("hidden"))) int vbn_restartable(void *fn, int arg)
{
  __asm__ volatile(
    "push %ebp\n"
    "mov %esp, %ebp\n"
    "push %ebx\n"
    "push %esi\n"
    "push %edi\n"
    "push $0\n"
    "mov $0x1000703B, %eax\n"
    "call *%eax\n"                 // the original records ESP/EBP here; ret 4
    "push 12(%ebp)\n"
    "mov 8(%ebp), %eax\n"
    "call *%eax\n"                 // fn(arg); an early exit lands after this too
    "pop %edi\n"
    "pop %esi\n"
    "pop %ebx\n"
    "pop %ebp\n"
    "ret\n");
}

// ---------------------------------------------------------------------------
// Probe registry (§7.1).  The counter is 8-aligned: a lock cmpxchg8b that
// straddles a cache line becomes a bus lock the kernel traps.
// ---------------------------------------------------------------------------
struct Probe {
  Probe *next;
  const char *name;
  alignas(8) unsigned long long count;
  Probe(const char *n);
};
static Probe *g_probes = nullptr;
static int g_trap_armed;
static void arm_fault_locator();
Probe::Probe(const char *n) : next(g_probes), name(n), count(0) { g_probes = this; }
#define PROBE_DECL(sym) static Probe sym##_probe(#sym);
#define PROBE_HIT(sym) do { __sync_fetch_and_add(&sym##_probe.count, 1ULL); \
                            if (!g_trap_armed) arm_fault_locator(); } while (0)

// vbn ends through DOS function 4Ch, which the shim implements with exit().
// That runs atexit handlers but not this library's destructors: pe2elf32's
// entry trampoline never goes through __libc_start_main, so _dl_fini is never
// registered.  Hence atexit rather than __attribute__((destructor)) (§7.2's
// ExitProcess hook is the x64 shim's version of the same problem).
// Set VBN_PROBES=0 to silence the dump.
static void dump_probes()
{
  const char *e = getenv("VBN_PROBES");
  if (e && *e == '0') return;
  fprintf(stderr, "[probe] exit, call counts:\n");
  for (Probe *p = g_probes; p; p = p->next)
    fprintf(stderr, "[probe]   %-32s %llu\n", p->name, p->count);
}
__attribute__((constructor)) static void probes_init() { atexit(dump_probes); }

// ---------------------------------------------------------------------------
// Fault locator.  The shim installs its own SIGSEGV/SIGBUS/SIGILL handlers in
// vbn_start, after our constructor, so ours is installed on the first probe
// hit instead.  It names the module and, for dummy.so, the symbol.
// ---------------------------------------------------------------------------
static void fault(int sig, siginfo_t *si, void *ctx)
{
  ucontext_t *uc = (ucontext_t *)ctx;
  uintptr_t eip = uc->uc_mcontext.gregs[REG_EIP];
  Dl_info di;
  char where[256] = "PE image";
  if (dladdr((void *)eip, &di) && di.dli_fname)
    snprintf(where, sizeof where, "%s+0x%lx %s", di.dli_fname,
             (unsigned long)(eip - (uintptr_t)di.dli_fbase), di.dli_sname ? di.dli_sname : "");
  fprintf(stderr, "[fault] sig=%d addr=%p eip=%08lx esp=%08lx  %s\n", sig, si->si_addr,
          (unsigned long)eip, (unsigned long)uc->uc_mcontext.gregs[REG_ESP], where);
  _exit(125);
}
static void arm_fault_locator()
{
  g_trap_armed = 1;
  static char stack[65536];
  stack_t ss = {};
  ss.ss_sp = stack;
  ss.ss_size = sizeof stack;
  sigaltstack(&ss, nullptr);
  struct sigaction sa = {};
  sa.sa_sigaction = fault;
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGBUS, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
  sigaction(SIGFPE, &sa, nullptr);
}

// ---------------------------------------------------------------------------
// Redirection (§5).  On i386 `jmp rel32` reaches any address modulo 2^32, so
// there is nothing to range-check.
// ---------------------------------------------------------------------------
static void patch_jmp(uintptr_t orig, void *repl)
{
  uint32_t disp = (uint32_t)((uintptr_t)repl - (orig + 5));
  long pg = sysconf(_SC_PAGESIZE);
  uintptr_t a = orig & ~(uintptr_t)(pg - 1);
  size_t len = (orig + 5 - a + pg - 1) & ~(size_t)(pg - 1);
  if (mprotect((void *)a, len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    perror("dummy.so: mprotect");
    _exit(124);
  }
  unsigned char *p = (unsigned char *)orig;
  p[0] = 0xE9;
  memcpy(p + 1, &disp, 4);
  __builtin___clear_cache((char *)p, (char *)p + 5);
}

// Every moved body is entered from PE code, and two things about that code
// matter to its callees:
//  - it does not keep ESP 16-aligned (force_align_arg_pointer);
//  - it relies on a callee leaving alone every register the callee does not
//    use, ECX and EDX included: sub_10001DAB sets ECX=0 to pass a mode to the
//    open wrapper and then uses the same ECX as `repnz scasb`'s count.
//    no_caller_saved_registers makes the body save whatever it clobbers,
//    except the return value (EAX; nothing here returns in EDX:EAX).  It needs
//    -mgeneral-regs-only, which the in-scope code (integer only) is fine with.
#define VBN_ENTRY __attribute__((force_align_arg_pointer, no_caller_saved_registers))

// ---------------------------------------------------------------------------
// The moved functions follow, then dummy_init().
// ---------------------------------------------------------------------------
