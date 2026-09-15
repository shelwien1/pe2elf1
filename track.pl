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
# For every plain store   mov  SIZE PTR <mem>, <src>   (SIZE = byte/word/dword/qword)
# the script inserts, in front of it:
#
#       push ARG                ; ARG = rcx (win64) / rdi (sysv)
#       lea  ARG, <mem>
#       call trackN             ; N = 1/2/4/8 = store width in bytes
#       pop  ARG
#
# so that trackN() can record the address and the old contents before they are
# overwritten (see track.inc: NEST()/UNDO() later restore them).
#
# Stores relative to rsp are *not* instrumented: they are the function's own
# stack frame, which is dead by the time UNDO() runs (and UNDO()'s own frame
# would then live at the same addresses). Any other instruction that writes
# memory (read-modify-write ALU ops, SSE/AVX stores, string ops, ...) is an
# error, because a store the journal does not see cannot be undone.

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

# --- instruction classification --------------------------------------------

my %width = ( byte => 1, word => 2, dword => 4, qword => 8 );

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
my ($skipped, $unhandled) = (0, 0);
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

    if ($mn =~ $implicit_write
        || ($mn =~ /^movs[bwdq]$/ && $ops eq '')) {
      report_unhandled($line);
      last;
    }
    if ($mn =~ /^(?:xchg|xadd|cmpxchg\w*)$/ && $ops =~ /\[/) {
      report_unhandled($line);
      last;
    }
    last unless $dst =~ /\[/;                      # destination is not memory

    if ($mn eq 'mov' && $dst =~ /^(byte|word|dword|qword)\s+ptr\s+(.+)$/i) {
      my ($sz, $mem) = (lc $1, $2);
      my $n = $width{$sz};
      if ($mem =~ /\brsp\b/) {
        $skipped++;
        print STDERR "  skip stack store : $line\n" if $verbose;
        last;
      }
      print STDERR "  track$n : $line\n" if $verbose;
      push @o, "\tpushfq" if $flags;
      push @o, "\tpush\t$ARG", "\tlea\t$ARG, $mem", "\tcall\ttrack$n", "\tpop\t$ARG";
      push @o, "\tpopfq" if $flags;
      $used{"track$n"} = 1;
      $count{$sz}++;
      last;
    }
    last if $mn =~ $readonly;
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
my @by_size = map { "$count{$_} $_" } grep { $count{$_} } qw(byte word dword qword);
printf STDERR "track.pl: %s -> %s [%s]: %d stores instrumented (%s), %d stack stores skipped\n",
  $in, $out, $abi, $total, (@by_size ? join(", ", @by_size) : "none"), $skipped;

if ($unhandled) {
  die "track.pl: $unhandled memory-writing instruction(s) not instrumented (see above)\n" unless $lax;
  print STDERR "track.pl: WARNING: $unhandled memory write(s) left unjournaled (--lax)\n";
}

sub report_unhandled {
  my ($line) = @_;
  $unhandled++;
  print STDERR "track.pl: unhandled memory write: $line\n";
}
