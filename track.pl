#!/usr/bin/perl
#
# track.pl - insert memory-write journaling calls into compiler-generated
#            x86-64 assembly (GNU as, Intel syntax).
#
# Usage:  perl track.pl [options] <input.s> <output.s>
#
# Options:
#   --abi=win64|sysv   calling convention of the target. Default: auto-detected
#                      from the input (COFF .seh_proc/.def directives => win64,
#                      ELF ".type sym, @function" => sysv).
#   --entry=NAME       the one function this translation unit may export.
#                      Default: encode_sim.
#   --inline           write the cell at the store site instead of calling a
#                      stub, for the 1-, 2- and 4-byte widths. Measured 5%
#                      faster on tangelo_w and 48% more instrumented assembly;
#                      off by default for that reason. See SPEED.md.
#   --repeat=N         emit each journaling call N times instead of once. A
#                      measurement tool, not a feature: journaling the same
#                      location twice is harmless (both cells save the same
#                      pre-store value, and UNDO restores both), so N=1,2,3 give
#                      the marginal cost of a journal entry with the program
#                      still producing correct output. See SPEED.md.
#   --flags            also wrap each call in pushfq/popfq. Not needed with the
#                      assembly track stubs from track.inc (they preserve EFLAGS);
#                      only for a stub implementation that may clobber flags.
#   --lax              warn instead of failing on anything it refuses.
#   -v                 list every instrumented / skipped write.
#
# For every instruction that writes a fixed-width block of memory - a plain
#   mov  SIZE PTR <mem>, <src>
# a vector store (movdqa/movdqu/movaps/movq/vpmovwb/vextract..., SIZE = byte
# through zmmword), a setcc, or a read-modify-write (add/inc/and/xadd/... with a
# memory destination) - the script inserts, in front of it:
#
#       push ARG                ; ARG = rcx (win64) / rdi (sysv)
#       lea  ARG, <mem>
#       call trackN             ; N = 1/2/4/8/16/32/64 = write width in bytes
#       pop  ARG
#
# so that trackN() can record the address and the old contents before they are
# overwritten (see track.inc: NEST()/UNDO() later restore them).
# Read-modify-write instructions are journaled the same way and for the same
# reason - UNDO() only needs the destination's contents from before the
# instruction ran, not a description of what it did to them.
#
# Writes relative to rsp are NOT instrumented, whatever the instruction: they
# are the function's own stack frame, which is dead by the time UNDO() runs (and
# UNDO()'s own frame would then live at the same addresses).
#
# EVERYTHING ELSE IS REFUSED. A write the journal does not see cannot be undone,
# and the resulting corruption is silent - encoder and decoder make the same
# mistake, so the round trip still passes and only the compression suffers. So
# the script stops rather than guess. Refused, specifically:
#   - scatters and string operations: one instruction, many addresses;
#   - maskmovdqu / movdir64b: the address is implicit, with no operand to lea;
#   - bts/btr/btc with a register bit offset: the write lands at
#     <mem> + (reg DIV opsize_bits)*opsize_bytes, outside the operand;
#   - a destination under an AVX-512 write mask: the restore would be a harmless
#     no-op for the masked-off lanes, but the save is not - reading them can
#     fault exactly where the store would not;
#   - a destination with a segment prefix: lea ignores the segment, so the
#     journal would record an unrelated address;
#   - xchg/xadd/cmpxchg with the memory operand second, which the
#     memory-destination parse below does not cover;
#   - a call or jmp whose target this file does not define: the callee's writes
#     never reach the journal. Compilers synthesise such calls from ordinary
#     loops (clang turns a tail-zeroing loop into memset), which is why the
#     instrumented compile also gets -fno-builtin.
#
# It also refuses three whole-file conditions:
#   - ANY exported symbol other than the entry point. The normal build defines
#     the same symbols from the same source, the linker keeps one copy of each,
#     and if it keeps the uninstrumented one the walk runs unjournaled. On ELF
#     these are .weak; on COFF they are .globl in a .linkonce section, which is
#     why the test is on the symbol set and not on the directive spelling.
#     (MinGW's .refptr.* address thunks are exempt: they are COMDAT by design
#     and merging them is what should happen.)
#   - a static initialiser, whose constructors would be journaled before main()
#     with nothing to ever roll them back;
#   - an rbp-based destination in a file that establishes a frame pointer. The
#     stack skip identifies the dying frame by rsp; where rbp is a frame pointer
#     too, an rbp-based write may be a frame slot (which must not be journaled)
#     or a model pointer (which must be), and nothing in the text distinguishes
#     them. Both conditions are needed for the ambiguity: GCC here sets up rbp
#     to realign the stack but still addresses every local off rsp, while MinGW
#     has no frame pointer and uses rbp as the base for ContextMap writes. Only
#     a -fno-omit-frame-pointer build has both, and it is refused.
#
# RED ZONE: the inserted "push ARG" writes at [rsp-8], so the 128-byte SysV red
# zone below rsp must not be in use. Compile the instrumented translation unit
# with -mno-red-zone; the script checks for negative rsp displacements and stops
# if it finds one. Win64 has no red zone.

use strict;
use warnings;

my ($abi, $flags, $lax, $verbose) = ('', 0, 0, 0);
my $repeat = 1;
my $inline = 0;
my $entry = 'encode_sim';
my @files;
for (@ARGV) {
  if    (/^--abi=(win64|sysv)$/) { $abi = $1 }
  elsif (/^--entry=(\S+)$/)      { $entry = $1 }
  elsif (/^--repeat=(\d+)$/)     { $repeat = $1 }
  elsif ($_ eq '--inline')       { $inline = 1 }
  elsif ($_ eq '--flags')        { $flags = 1 }
  elsif ($_ eq '--lax')          { $lax = 1 }
  elsif ($_ eq '-v')             { $verbose = 1 }
  elsif (/^-/)                   { die "track.pl: unknown option $_\n" }
  else                           { push @files, $_ }
}
# No defaults: the build names its files per program (coder-<prog>.s), and the
# old coder.s / coder1.s from legacy/g.bat are never produced now.
@files==2 or die "usage: track.pl [options] <input.s> <output.s>\n";
my ($in, $out) = @files;

open(my $I, '<', $in)  or die "track.pl: cannot read $in: $!\n";
my @a = <$I>;
close $I;
s/\r?\n\z// for @a;

# --- whole-file checks ------------------------------------------------------

grep { /^\s*\.intel_syntax\s+noprefix/ } @a
  or die "track.pl: $in is not Intel-syntax assembly (compile with -masm=intel)\n";

if (!$abi) {
  if    (grep { /^\s*\.(?:seh_proc|def)\b/ } @a)               { $abi = 'win64' }
  elsif (grep { /^\s*\.type\s+\S+\s*,\s*[@%]function/ } @a)    { $abi = 'sysv'  }
  else { die "track.pl: cannot detect ABI of $in, use --abi=win64|sysv\n" }
}
my $ARG = $abi eq 'win64' ? 'rcx' : 'rdi';

# Every symbol this file defines. A call to anything else leaves the
# instrumented code, and whatever it writes never reaches the journal.
my %defined;
for (@a) { $defined{$1} = 1 if /^([A-Za-z_.\$][\w.\$]*):/ }

my @exported = map { /^\s*\.(?:glob(?:a)?l|weak)\s+(\S+)/ ? $1 : () } @a;
my @extra = grep { $_ ne $entry && !/^\.refptr\./ && $defined{$_} } @exported;
if (@extra) {
  refuse("$in exports " . scalar(@extra) . " symbol(s) besides $entry, e.g. $extra[0]\n"
       . "  The normal build defines the same symbol from the same source and the linker keeps\n"
       . "  only one copy, so the instrumented body can be discarded, leaving the speculative\n"
       . "  walk unjournaled. Mark those functions INLINE (always_inline) so they fold into\n"
       . "  $entry(), leaving this translation unit with one entry point.\n");
}

if (grep { /^\s*\.section\s+\.(?:init_array|ctors)\b/ || /^\s*\.section\s+\.CRT\$XC/ || /^_GLOBAL__sub_I/ } @a) {
  refuse("$in contains a static initialiser (.init_array / .ctors / _GLOBAL__sub_I).\n"
       . "  Its constructors would be instrumented too, filling the journal at start-up with\n"
       . "  entries nothing ever undoes. Define objects with constructors in the main\n"
       . "  translation unit only, inside the #ifndef SIM_FUNC guard.\n");
}

# Does this file establish a frame pointer? On its own that is harmless - GCC
# does it here purely to realign the stack, and still addresses every local off
# rsp. It only matters in combination with an rbp-based destination, which the
# main pass refuses.
my $frame_ptr = grep { /^\s*mov\s+rbp\s*,\s*rsp\b/i
                    || /cfi_def_cfa_register\s+(?:6|%?rbp)/
                    || /\.seh_setframe\s+rbp/ } @a;

# --- instruction classification --------------------------------------------

my %width = ( byte => 1, word => 2, dword => 4, qword => 8,
              xmmword => 16, ymmword => 32, zmmword => 64 );

# Instructions that overwrite their whole memory destination and read nothing
# of it, so saving the destination's width beforehand captures everything.
my $store = qr/^(?:
    mov | movabs | movbe | movnti | set[a-z]{1,3} |
    vpmov(?:s|us)?(?:qb|qw|qd|db|dw|wb) |
    v?mov[au]p[sd] |
    v?movdqa(?:32|64)? | v?movdqu(?:8|16|32|64)? |
    v?movnt(?:dqa|dq|ps|pd|i) |
    v?movd | v?movq | v?movss | v?movsd |
    v?mov(?:hp|lp)[sd] |
    vextract(?:i|f)(?:32x4|32x8|64x2|64x4|128) | vextractps | vpextr[bwdq]
  )$/xi;

# Read-modify-write instructions. Instrumenting these is exactly as sound as
# instrumenting a plain store, since the stub records the destination before the
# instruction runs - and the stubs preserve EFLAGS, so putting a call in front
# of a flag-setting instruction changes nothing.
my $rmw = qr/^(?:
    add|adc|sub|sbb|and|or|xor|inc|dec|neg|not|
    shl|sal|shr|sar|shld|shrd|rol|ror|rcl|rcr|
    bts|btr|btc|xadd|xchg|cmpxchg(?:8b|16b)?
  )$/xi;

# mnemonics whose first (or only) operand may be memory but is only read
my $readonly = qr/^(?:
    cmp|test|bt|push|nop|
    mul|imul|div|idiv|
    prefetch\w*|clflush\w*|
    fld\w*|fild|fcom\w*|ficom\w*|fucom\w*|fadd|fsub\w*|fmul|fdiv\w*|fiadd|fisub\w*|fimul|fidiv\w*|
    ldmxcsr|vldmxcsr|fldcw|fldenv|frstor|fxrstor\w*|xrstor\w*|
    ucomis[sd]|vucomis[sd]|comis[sd]|vcomis[sd]|
    invlpg|verr|verw|lgdt|lidt|lldt|ltr|lmsw
  )$/xi;

# writers with no memory operand in the text to point a lea at
my $implicit_write = qr/^(?:rep[ez]?|repn[ez]|stos[bwdq]?|ins[bwd]?|
    v?maskmov(?:dqu|q)|movdir64b|enqcmds?|
    xsave\w*|fxsave\w*|fn?save|fn?stenv|xlat|xlatb)$/xi;

# --- main pass ---------------------------------------------------------------

my (%used, %count);
my ($skipped, $unhandled, $redzone) = (0, 0, 0);
my @o;

for my $line (@a) {
  my $code = $line;
  $code =~ s/\s*#.*$//;                           # strip comments
  $code =~ s/^\s*[.\w\$]+:\s*//;                  # a label may share the line
  next_line: {
    last if $code =~ /^\s*$/;                     # empty, or label only
    last if $code =~ /^\s*\./;                    # directive
    $code =~ /^\s*(\S+)\s*(.*?)\s*$/ or last;
    my ($mn, $ops) = (lc $1, $2);
    my ($dst) = split /\s*,\s*/, $ops, 2;
    $dst //= '';

    if ($mn eq 'lock') {                           # a lock prefix: look past it
      $ops =~ /^\s*(\S+)\s*(.*?)\s*$/ or last;
      ($mn, $ops) = (lc $1, $2);
      ($dst) = split /\s*,\s*/, $ops, 2;
      $dst //= '';
    }

    if ($mn eq 'call' || $mn eq 'jmp') {
      my ($tgt) = ($ops =~ /^([\w.\$]+)\s*$/);
      if (!defined $tgt) {
        # An indirect branch. A switch jump table goes through a local .L symbol
        # and writes nothing; anything else is a call we cannot see inside.
        report_call($line, "indirect") unless $ops =~ /\.L/;
      } elsif ($tgt !~ /^\.L/ && !$defined{$tgt}) {
        report_call($line, "leaves this translation unit");
      }
      last;
    }

    if ($mn =~ $implicit_write
        || ($mn =~ /^movs[bwdq]$/ && $ops eq '')) {
      report_unhandled($line);          # writes a range no lea can express
      last;
    }
    if ($mn =~ /^v?p?scatter|^vp?scatter/i) {
      report_unhandled($line);          # one instruction, many addresses
      last;
    }
    last if $mn =~ $readonly;                      # memory operand only read

    # lea drops a segment prefix, so the journal would record a different
    # address than the instruction writes to. (Also catches a moffs
    # destination, "movabs ds:<imm64>, al".)
    if ($dst =~ /\b[cdefgs]s\s*:/i) {
      report_unhandled($line);
      last;
    }

    # These write whichever operand is memory; only the memory-first order is
    # parsed below, so refuse the other rather than miss the write.
    if ($mn =~ /^(?:xchg|xadd|cmpxchg(?:8b|16b)?)$/ && $dst !~ /\[/ && $ops =~ /\[/) {
      report_unhandled($line);
      last;
    }

    last unless $dst =~ /\[/;                      # destination is not memory

    # The function's own frame: dead once the speculative step returns. Tested
    # on the registers inside the address expression rather than on the printed
    # text, so a symbol whose name contains "rsp" is not taken for a frame slot.
    my ($inside) = ($dst =~ /\[([^\]]*)\]/);
    my @regs = defined $inside ? ($inside =~ /\b([re][a-z0-9]{1,3})\b/g) : ();
    if ($frame_ptr && grep { $_ eq 'rbp' } @regs) {
      $unhandled++;
      print STDERR "track.pl: rbp-based write in a frame-pointer build - it could be a frame\n"
                 . "  slot (must not be journaled) or a model pointer (must be), and the two are\n"
                 . "  indistinguishable here. Compile with -fomit-frame-pointer: $line\n"
        if $unhandled < 4;
      last;
    }

    if (grep { $_ eq 'rsp' } @regs) {
      if ($dst =~ /^[^\[]*-\s*\d/ || $dst =~ /\[[^\]]*-\s*\d+\s*\]/) {
        $redzone++;
        print STDERR "track.pl: red-zone access below rsp: $line\n" if $redzone < 4;
      }
      $skipped++;
      print STDERR "  skip stack write : $line\n" if $verbose;
      last;
    }

    # BTS/BTR/BTC address a BIT. With a register bit offset the write lands at
    # <mem> + (reg DIV operand-size-in-bits) * operand-size-in-bytes, which
    # "lea <mem>" does not reproduce - the journal would save a location the
    # instruction never touches and miss the one it does. An immediate offset is
    # taken modulo the operand size, so that form stays inside the operand.
    if ($mn =~ /^bt[src]$/ && $ops !~ /,\s*(?:0[xX][0-9a-fA-F]+|\d+)\s*$/) {
      report_unhandled($line);
      last;
    }

    # A write mask suppresses lanes, and a suppressed lane is allowed to address
    # unmapped memory without faulting. trackN would read the whole width and
    # fault where the store does not, so this cannot be journaled by widening.
    if ($dst =~ /\{\s*%?k[0-7]\s*\}/i) {
      report_unhandled($line);
      last;
    }

    if ($mn =~ $store || $mn =~ $rmw) {
      my ($sz, $mem);
      if ($dst =~ /^(byte|word|dword|qword|xmmword|ymmword|zmmword)\s+ptr\s+(.+)$/i) {
        ($sz, $mem) = (lc $1, $2);
      } else {
        $mem = $dst;               # width implied by the source register
      }
      $mem =~ s/\{[^}]*\}//g;      # a rounding-mode suffix is not part of the address
      $mem =~ s/\s+$//;

      my $n = $sz ? $width{$sz} : undef;
      if (!defined $n) {           # no SIZE PTR: take it from the source operand
        my ($src) = ($ops =~ /,\s*([%\w]+)/);
        $n = reg_width($src) if defined $src;
      }
      if (!defined $n) { report_unhandled($line); last; }

      print STDERR "  track$n : $line\n" if $verbose;
      # --inline: write the cell here instead of calling the stub. Saves the
      # call/ret pair per store; costs one extra push/pop, because inline code
      # needs three live values (target address, old value, journal position)
      # where the stub gets the target for free in its argument register.
      #
      # The cell layout is track.inc's: off = (address - trk_base) | tag<<30 at
      # +0, the overwritten bytes at +4. Like the stubs, this uses only mov,
      # movzx, lea, push and pop, so it preserves EFLAGS; the base is added
      # negated because lea cannot subtract a register and sub sets flags.
      #
      # "lea rax, $mem" has to come first, while the original rax and rdx are
      # both still live: the address expression may name either of them.
      if( $inline && $n<=4 ) {
        my %ld  = ( 1=>'movzx edx, byte ptr', 2=>'movzx edx, word ptr', 4=>'mov edx, dword ptr' );
        my %tag = ( 1=>'', 2=>'+0x40000000', 4=>'-0x80000000' );
        for my $r (1..$repeat) {
          push @o,
            "\tpush\trax", "\tpush\trdx",
            "\tlea\trax, $mem",                        # rax = target address
            "\t$ld{$n} [rax]",                          # edx = overwritten bytes
            "\tpush\trdx",
            "\tmov\trdx, qword ptr [rip+trk_negbase]",
            "\tlea\tedx, [rax+rdx$tag{$n}]",           # edx = offset | tag<<30
            "\tmov\trax, qword ptr [rip+trkptr]",      # rax = &trk[trkptr]
            "\tmov\tdword ptr [rax], edx",
            "\tlea\trdx, [rax+8]",
            "\tmov\tqword ptr [rip+trkptr], rdx",
            "\tpop\trdx",
            "\tmov\tdword ptr [rax+4], edx",
            "\tpop\trdx", "\tpop\trax";
        }
        $count{$n} += $repeat;
        last;
      }
      for my $r (1..$repeat) {
        push @o, "\tpushfq" if $flags;
        push @o, "\tpush\t$ARG", "\tlea\t$ARG, $mem", "\tcall\ttrack$n", "\tpop\t$ARG";
        push @o, "\tpopfq" if $flags;
      }
      $used{"track$n"} = 1;
      $count{$n}++;
      last;
    }

    report_unhandled($line);
  }
  push @o, $line;
}

if ($abi eq 'win64') {
  push @o, ".def $_; .scl 2; .type 32; .endef" for sort keys %used;
}

open(my $O, '>', $out) or die "track.pl: cannot write $out: $!\n";
print $O "$_\n" for @o;
close $O;

my $total = 0; $total += $_ for values %count;
my @by_size = map { "$count{$_}x${_}B" } sort { $a <=> $b } keys %count;
printf STDERR "track.pl: %s -> %s [%s]: %d writes instrumented (%s), %d stack writes skipped\n",
  $in, $out, $abi, $total, (@by_size ? join(", ", @by_size) : "none"), $skipped;

if ($redzone) {
  refuse("$redzone access(es) below rsp - the red zone is in use, and the inserted\n"
       . "  \"push\" would corrupt it. Compile this translation unit with -mno-red-zone.\n");
}

if ($unhandled) {
  die "track.pl: $unhandled memory write(s) not instrumented (see above)\n" unless $lax;
  print STDERR "track.pl: WARNING: $unhandled memory write(s) left unjournaled (--lax)\n";
}

# Width in bytes of a register name, for the rare operand printed without an
# explicit SIZE PTR.
sub reg_width {
  my ($r) = @_;
  $r =~ s/^%//;
  return 16 if $r =~ /^xmm\d+$/i;
  return 32 if $r =~ /^ymm\d+$/i;
  return 64 if $r =~ /^zmm\d+$/i;
  return 8  if $r =~ /^(?:r[a-z]x|r[sd]i|r[sb]p|r\d+)$/i;
  return 4  if $r =~ /^(?:e[a-z]x|e[sd]i|e[sb]p|r\d+d)$/i;
  return 2  if $r =~ /^(?:[a-z]x|[sd]i|[sb]p|r\d+w)$/i;
  return 1  if $r =~ /^(?:[a-z][lh]|[sd]il|[sb]pl|r\d+b)$/i;
  return undef;
}

sub report_call {
  my ($line, $why) = @_;
  $unhandled++;
  print STDERR "track.pl: call $why, so its memory writes cannot be journaled: $line\n";
}

sub report_unhandled {
  my ($line) = @_;
  $unhandled++;
  print STDERR "track.pl: unhandled memory write: $line\n";
}

sub refuse {
  my ($msg) = @_;
  die "track.pl: $msg" unless $lax;
  print STDERR "track.pl: WARNING: $msg";
}
