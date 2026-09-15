#!/usr/bin/perl
#
# track.pl - insert memory-write journaling calls into compiler-generated
#            x86-64 assembly (GNU as, Intel syntax).
#
# Usage:  perl track.pl [options] [input.s [output.s]]
#         defaults: input=coder.s output=coder1.s
#
# Options:
#   --abi=win64|sysv   calling convention of the target. Default: auto-detected
#                      from the input (COFF .seh_proc/.def directives => win64,
#                      ELF ".type sym, @function" => sysv).
#   --flags            also wrap each call in pushfq/popfq. Not needed with the
#                      assembly track stubs from track.inc (they preserve EFLAGS);
#                      only for a stub implementation that may clobber flags.
#   --lax              warn instead of failing on memory-writing instructions
#                      this script does not know how to instrument.
#   -v                 list every instrumented / skipped store.
#
# For every instruction that writes a fixed-width block of memory - a plain
#   mov  SIZE PTR <mem>, <src>
# a vector store (movdqa/movdqu/movaps/movq/vextract..., SIZE = byte through
# zmmword), or a read-modify-write (add/inc/and/xadd/... with a memory
# destination) - the script inserts, in front of it:
#
#       push ARG                ; ARG = rcx (win64) / rdi (sysv)
#       lea  ARG, <mem>
#       call trackN             ; N = 1/2/4/8/16/32/64 = store width in bytes
#       pop  ARG
#
# so that trackN() can record the address and the old contents before they are
# overwritten (see track.inc: NEST()/UNDO() later restore them). An AVX-512
# write mask on the destination is dropped from the lea: journaling the whole
# width is still correct, since restoring bytes the store never wrote is a no-op.
# Read-modify-write instructions are journaled the same way and for the same
# reason - UNDO() only needs the destination's contents from before the
# instruction ran, not a description of what it did to them.
#
# Stores relative to rsp are NOT instrumented, whatever the instruction: they
# are the function's own stack frame, which is dead by the time UNDO() runs (and
# UNDO()'s own frame would then live at the same addresses). Any OTHER
# memory-writing instruction the script does not recognise is an error, because
# a store the journal does not see cannot be undone.
#
# RED ZONE: the inserted "push ARG" writes at [rsp-8], so the 128-byte SysV red
# zone below rsp must not be in use. Compile the instrumented translation unit
# with -mno-red-zone; the script checks for negative rsp displacements and stops
# if it finds one. Win64 has no red zone.

use strict;
use warnings;

my ($abi, $flags, $lax, $verbose) = ('', 0, 0, 0);
my @files;
for (@ARGV) {
  if    (/^--abi=(win64|sysv)$/) { $abi = $1 }
  elsif ($_ eq '--flags')        { $flags = 1 }
  elsif ($_ eq '--lax')          { $lax = 1 }
  elsif ($_ eq '-v')             { $verbose = 1 }
  elsif (/^-/)                   { die "track.pl: unknown option $_\n" }
  else                           { push @files, $_ }
}
my $in  = $files[0] // 'coder.s';
my $out = $files[1] // 'coder1.s';

open(my $I, '<', $in)  or die "track.pl: cannot read $in: $!\n";
my @a = <$I>;
close $I;
s/\r?\n\z// for @a;

# --- sanity check / ABI detection ------------------------------------------

grep { /^\s*\.intel_syntax\s+noprefix/ } @a
  or die "track.pl: $in is not Intel-syntax assembly (compile with -masm=intel)\n";

if (!$abi) {
  if    (grep { /^\s*\.(?:seh_proc|def)\b/ } @a)               { $abi = 'win64' }
  elsif (grep { /^\s*\.type\s+\S+\s*,\s*[@%]function/ } @a)    { $abi = 'sysv'  }
  else { die "track.pl: cannot detect ABI of $in, use --abi=win64|sysv\n" }
}
my $ARG = $abi eq 'win64' ? 'rcx' : 'rdi';

# A weak (COMDAT) definition here is a trap: the normal build defines the same
# symbol from the same source, the linker keeps exactly one of the two, and if it
# keeps the normal one the instrumentation is silently dropped - the walk then
# runs unjournaled and corrupts the model instead of simulating it. Nothing later
# fails loudly, the output just compresses badly. So refuse the input.
my @weak = map { /^\s*\.weak\s+(\S+)/ ? $1 : () } @a;
if (@weak) {
  my $m = "track.pl: $in defines " . scalar(@weak) . " weak (COMDAT) symbol(s), e.g. $weak[0]\n"
        . "  The normal build defines the same symbol and the linker keeps only one copy, so the\n"
        . "  instrumented body can be discarded, leaving the speculative walk unjournaled.\n"
        . "  Mark those functions INLINE (always_inline) so they fold into encode_sim().\n";
  die $m unless $lax;
  print STDERR "track.pl: WARNING: $m";
}

# --- instruction classification --------------------------------------------

my %width = ( byte => 1, word => 2, dword => 4, qword => 8,
              xmmword => 16, ymmword => 32, zmmword => 64 );

# Instructions that overwrite their whole memory destination and read nothing
# of it, so saving the destination's width beforehand captures everything.
my $store = qr/^(?:
    mov | movabs | movbe | movnti |
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
    cmp|test|bt|push|jmp|call|nop|
    mul|imul|div|idiv|
    prefetch\w*|clflush\w*|
    fld\w*|fild|fcom\w*|ficom\w*|fucom\w*|fadd|fsub\w*|fmul|fdiv\w*|fiadd|fisub\w*|fimul|fidiv\w*|
    ldmxcsr|vldmxcsr|fldcw|fldenv|frstor|fxrstor\w*|xrstor\w*|
    ucomis[sd]|vucomis[sd]|comis[sd]|vcomis[sd]|
    invlpg|verr|verw|lgdt|lidt|lldt|ltr|lmsw
  )$/xi;

# implicit-memory writers (no explicit memory operand in the text)
my $implicit_write = qr/^(?:rep[ez]?|repn[ez]|stos[bwdq]?|ins[bwd]?|xsave\w*|fxsave\w*|fn?save|fn?stenv|xlat|xlatb)$/i;

# --- main pass ---------------------------------------------------------------

my (%used, %count);
my ($skipped, $unhandled, $redzone) = (0, 0, 0);
my @o;

for my $line (@a) {
  my $code = $line;
  $code =~ s/\s*#.*$//;                           # strip comments
  next_line: {
    last if $code =~ /^\s*$/;                     # empty
    last if $code =~ /^\s*[.\w$@]+:/ && $code !~ /^\s*\w+\s+\w+\s+ptr/i;   # label
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

    if ($mn =~ $implicit_write
        || ($mn =~ /^movs[bwdq]$/ && $ops eq '')) {
      report_unhandled($line);
      last;
    }
    last if $mn =~ $readonly;                      # memory operand only read
    last unless $dst =~ /\[/;                      # destination is not memory

    # The function's own frame: dead once the speculative step returns.
    if ($dst =~ /\brsp\b/) {
      if ($dst =~ /^[^\[]*-\s*\d/ || $dst =~ /\[[^\]]*-\s*\d+\s*\]/) {
        $redzone++;
        print STDERR "track.pl: red-zone access below rsp: $line\n" if $redzone < 4;
      }
      $skipped++;
      print STDERR "  skip stack store : $line\n" if $verbose;
      last;
    }

    if ($mn =~ /^v?p?scatter|^vp?scatter/i) {
      report_unhandled($line);          # one instruction, many addresses: no lea
      last;
    }

    if ($mn =~ $store || $mn =~ $rmw) {
      my ($sz, $mem);
      if ($dst =~ /^(byte|word|dword|qword|xmmword|ymmword|zmmword)\s+ptr\s+(.+)$/i) {
        ($sz, $mem) = (lc $1, $2);
      } else {
        $mem = $dst;               # width implied by the source register
      }
      $mem =~ s/\{[^}]*\}//g;      # drop an AVX-512 write mask / rounding mode
      $mem =~ s/\s+$//;

      my $n = $sz ? $width{$sz} : undef;
      if (!defined $n) {           # no SIZE PTR: take it from the source operand
        my ($src) = ($ops =~ /,\s*([%\w]+)/);
        $n = reg_width($src) if defined $src;
      }
      if (!defined $n) { report_unhandled($line); last; }

      print STDERR "  track$n : $line\n" if $verbose;
      push @o, "\tpushfq" if $flags;
      push @o, "\tpush\t$ARG", "\tlea\t$ARG, $mem", "\tcall\ttrack$n", "\tpop\t$ARG";
      push @o, "\tpopfq" if $flags;
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
printf STDERR "track.pl: %s -> %s [%s]: %d stores instrumented (%s), %d stack stores skipped\n",
  $in, $out, $abi, $total, (@by_size ? join(", ", @by_size) : "none"), $skipped;

if ($redzone) {
  my $m = "track.pl: $redzone access(es) below rsp - the red zone is in use, and the\n"
        . "  inserted \"push\" would corrupt it. Compile this translation unit with -mno-red-zone.\n";
  die $m unless $lax;
  print STDERR "track.pl: WARNING: $m";
}

if ($unhandled) {
  die "track.pl: $unhandled memory-writing instruction(s) not instrumented (see above)\n" unless $lax;
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

sub report_unhandled {
  my ($line) = @_;
  $unhandled++;
  print STDERR "track.pl: unhandled memory write: $line\n";
}
