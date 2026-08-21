# merge_opt.pl -- recombine several optimizer profiles into one hybrid.
#
#   perl merge_opt.pl [options] <export> [<export> ...]
#
# auto_opt2.pl is a hill climber: one instance, one basin.  Four of them started
# from four random seeds end up in four DIFFERENT basins -- run_opt.sh's whole
# premise -- and best_exp.pl then throws three of them away.  That is a waste,
# because the four disagree on only a handful of the 133 descriptors, and a
# parameter that helped instance b is not automatically worthless to instance a.
#
# This script does the recombination best_exp.pl skips:
#
#   1. extract the profile EMBEDDED IN THE EXECUTABLE (the build's own parameter
#      set) and treat it as one more candidate -- it is the baseline every
#      instance departed from, and on a descriptor nobody improved it is the
#      value all of them still carry;
#   2. read every export file given, and dedup: two instances that converged on
#      the same set are one candidate, measured once;
#   3. measure each WHOLE profile and take the best as the starting point, so
#      the result can never be worse than the best input;
#   4. for every descriptor where the candidates DISAGREE, try each rival value
#      in turn against the current hybrid, keeping any that shrinks the output.
#      Sweep until a whole pass changes nothing.
#
# Step 4 is the point.  A descriptor's value is only ever accepted or rejected
# against the rest of the parameter set it sits in, so a value that lost inside
# instance c's basin can still win inside instance a's -- and that combination
# is one no single climber can reach, because reaching it would mean walking
# uphill first.
#
# The search space here is tiny compared to auto_opt2.pl's: it tries only values
# some instance already found worth keeping, never a fresh bit flip.  That makes
# it a cheap post-pass over a set of climber outputs, not a replacement for them
# -- run it when the instances have diverged, fold the result back in with
# import.sh, and let the climbers continue from the merged binary.
#
# Every measurement patches a COPY of the binary (--work), never the original,
# so this is safe to run while the climbers are still going.
use strict;
use warnings;

my $exe      = "xadpcm";
my $cmd      = undef;              # default depends on $exe/$corpus/$pak
my $corpus   = "wavs3";
my $pak      = "merge.tmp.pak";
my $work     = "merge.work";       # the binary this script patches
my $out      = "export.!!!";
my $log      = "mergetimes.!!!";
my $maxpass  = 0;                  # 0 = until a pass changes nothing
my $pairs    = 0;                  # also try pairs of rival values
my $verbose  = 0;
my @exports;

for my $a (@ARGV) {
  if    ( $a =~ /^--exe=(.*)$/     ) { $exe     = $1 }
  elsif ( $a =~ /^--cmd=(.*)$/     ) { $cmd     = $1 }
  elsif ( $a =~ /^--corpus=(.*)$/  ) { $corpus  = $1 }
  elsif ( $a =~ /^--pak=(.*)$/     ) { $pak     = $1 }
  elsif ( $a =~ /^--work=(.*)$/    ) { $work    = $1 }
  elsif ( $a =~ /^--out=(.*)$/     ) { $out     = $1 }
  elsif ( $a =~ /^--log=(.*)$/     ) { $log     = $1 }
  elsif ( $a =~ /^--passes=(\d+)$/ ) { $maxpass = $1 }
  elsif ( $a =~ /^--pairs$/        ) { $pairs   = 1 }
  elsif ( $a =~ /^-v$/             ) { $verbose = 1 }
  elsif ( $a =~ /^--/              ) { die "merge_opt.pl: unknown option $a\n" }
  else                               { push @exports, $a }
}
@exports or die <<"USAGE";
usage: perl merge_opt.pl [options] <export> [<export> ...]
  --exe=PATH      binary whose !MAP! table defines the descriptors, and whose
                  own embedded profile joins the candidates (default xadpcm)
  --corpus=PATH   file to compress when measuring (default wavs3)
  --cmd=CMD       full measure command; overrides --corpus/--pak wiring
  --pak=PATH      the coded output whose size is the score (default $pak)
  --work=PATH     scratch copy of the binary that gets patched (default $work)
  --out=PATH      where to write the hybrid profile (default export.!!!)
  --log=PATH      append "{ time, size }," for each new best (default $log)
  --passes=N      stop after N sweeps (default: until a sweep changes nothing)
  --pairs         after the single-value sweeps converge, also try PAIRS of
                  rival values -- catches parameters that only pay together
  -v              print every measurement, not just improvements
USAGE

# ---------------------------------------------------------------- descriptors

# The !MAP! layout, identical to auto_opt2.pl's and dump_export.pl's: the marker,
# the descriptor name, '!', the Ofs argument, NUL, the pattern, NUL.  $adr is the
# file offset of the pattern itself.
sub read_file {
  my ($p) = @_;
  open my $h, "<", $p or die "merge_opt.pl: $p: $!\n";
  binmode $h;
  local $/;
  my $d = <$h>;
  close $h;
  return $d;
}

my $exe_data = read_file($exe);
my (%adr, %ofs, %len);
while( $exe_data =~ /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`) + 5 + (length($1) + 1 + length($2)) + 1;
  $ofs{$1} = $2;
  $len{$1} = length($3);
}
%adr or die "merge_opt.pl: $exe has no !MAP! descriptors -- is it a Debug build?\n";
my @keys = sort keys %adr;
printf "%s: %d descriptors\n", $exe, scalar @keys;

# --------------------------------------------------------------- the profiles

# A profile is a { descriptor => pattern } covering exactly the binary's keys.
# Anything else is a mismatch worth refusing: a missing key would silently keep
# whatever the work binary happened to hold, and a wrong length would leave the
# tail of the previous pattern in place.
sub profile_ok {
  my ($p, $name) = @_;
  for my $k (@keys) {
    unless( exists $p->{$k} ) { warn "merge_opt.pl: $name lacks $k -- skipped\n"; return 0 }
    if( length($p->{$k}) != $len{$k} ) {
      warn "merge_opt.pl: $name has $k at length ".length($p->{$k})." != $len{$k} -- skipped\n";
      return 0;
    }
  }
  for my $k (keys %$p) {
    warn "merge_opt.pl: $name has $k, absent from $exe -- ignored\n" unless exists $adr{$k};
  }
  return 1;
}

sub read_export {
  my ($p) = @_;
  open my $h, "<", $p or die "merge_opt.pl: $p: $!\n";
  my %prof;
  while( <$h> ) {
    next unless /^\s*mdesc\(\s*(.*?)\s*,\s*(.*?)\s*,\s*"(.*?)"\s*\)\s*;/;
    $prof{$1} = $3;
  }
  close $h;
  return \%prof;
}

my @cand;   # [ name, profile ]
push @cand, [ "$exe:embedded", { map { $_ => substr($exe_data, $adr{$_}, $len{$_}) } @keys } ];
for my $f (@exports) {
  my $p = read_export($f);
  next unless profile_ok($p, $f);
  push @cand, [ $f, $p ];
}

# dedup -- two instances that converged on the same set are one candidate
sub sig { my ($p) = @_; return join("\x01", map { $p->{$_} } @keys) }
my (%seen_sig, @uniq);
for my $c (@cand) {
  my $s = sig($c->[1]);
  if( exists $seen_sig{$s} ) {
    print "dup: $c->[0] == $seen_sig{$s}\n";
    next;
  }
  $seen_sig{$s} = $c->[0];
  push @uniq, $c;
}
@cand = @uniq;
printf "candidates: %d unique profile(s)\n", scalar @cand;

# --------------------------------------------------------------- measurement

$cmd = "./$work c $corpus $pak >/dev/null 2>&1" unless defined $cmd;
# The work copy is what gets patched; the original binary is never written to,
# so the climbers can keep running against theirs.
system("cp", "-f", $exe, $work) == 0 or die "merge_opt.pl: cannot copy $exe -> $work\n";
chmod 0755, $work;

my %cache;      # profile signature -> size
my ($evals, $hits) = (0, 0);

sub measure {
  my ($prof) = @_;
  my $s = sig($prof);
  # The codec is deterministic, so the same profile always codes to the same
  # size -- a repeat is a lookup, not a re-run.  Sweeps revisit combinations
  # constantly, so this is most of the wall clock saved.
  if( exists $cache{$s} ) { $hits++; return $cache{$s} }

  open my $h, "+<", $work or die "merge_opt.pl: $work: $!\n";
  binmode $h;
  for my $k (@keys) {
    seek $h, $adr{$k}, 0;
    print $h $prof->{$k};
  }
  close $h;

  unlink $pak;
  system $cmd;
  my $sz = -s $pak;
  # Same guard auto_opt2.pl uses: a failed or truncated run must never look like
  # a win just because it wrote a short file.
  $sz = 0x7FFFFFFF if !defined($sz) || $sz < 100;
  $evals++;
  return $cache{$s} = $sz;
}

# ------------------------------------------------------- whole-profile scores

my ($best, $best_name, $best_size);
print "\n=== whole-profile scores\n";
for my $c (@cand) {
  my $sz = measure($c->[1]);
  printf "  %-24s %9d\n", $c->[0], $sz;
  if( !defined($best_size) || $sz < $best_size ) {
    ($best, $best_name, $best_size) = ({ %{$c->[1]} }, $c->[0], $sz);
  }
}
die "merge_opt.pl: every candidate failed to measure\n" if $best_size >= 0x7FFFFFFF;
printf "best input: %s at %d\n", $best_name, $best_size;

my $input_best = $best_size;
# provenance: which candidate each descriptor's current value came from
my %from = map { $_ => $best_name } @keys;

# ------------------------------------------------------------- rival values

# For each descriptor, every distinct value any candidate holds.  Descriptors
# where all candidates agree are not search space -- nothing to substitute.
my (%rivals, @contested);
for my $k (@keys) {
  my %v;
  for my $c (@cand) { push @{ $v{$c->[1]{$k}} }, $c->[0] }
  $rivals{$k} = \%v;
  push @contested, $k if scalar(keys %v) > 1;
}
printf "contested: %d of %d descriptors, %d distinct values among them\n\n",
  scalar @contested, scalar @keys,
  eval { my $n = 0; $n += scalar keys %{$rivals{$_}} for @contested; $n };

unless( @contested ) {
  print "merge_opt.pl: candidates agree everywhere -- nothing to recombine\n";
}

sub record {
  my ($size) = @_;
  open my $o, ">>", $log or die "merge_opt.pl: $log: $!\n";
  printf $o "{ %10i, %7i },\n", time, $size;
  close $o;
  write_export();
}

sub write_export {
  open my $o, ">", "$out.tmp" or die "merge_opt.pl: $out.tmp: $!\n";
  printf $o "mdesc( %s, %s, \"%s\" );\n", $_, $ofs{$_}, $best->{$_} for @keys;
  close $o;
  rename "$out.tmp", $out or die "merge_opt.pl: cannot rename $out.tmp: $!\n";
}

# -------------------------------------------------------- single-value sweeps

my $pass = 0;
while( 1 ) {
  $pass++;
  my $improved = 0;
  printf "=== pass %d (best %d)\n", $pass, $best_size;

  for my $k (@contested) {
    my $cur = $best->{$k};
    for my $v (sort keys %{ $rivals{$k} }) {
      next if $v eq $cur;
      my %trial = (%$best, $k => $v);
      my $sz = measure(\%trial);
      printf "  %-20s %-8s %9d%s\n", $k, join(",", @{ $rivals{$k}{$v} }), $sz,
             ($sz < $best_size ? "  <-- accepted" : "")
        if $verbose || $sz < $best_size;
      if( $sz < $best_size ) {
        $best      = \%trial;
        $best_size = $sz;
        $from{$k}  = join(",", @{ $rivals{$k}{$v} });
        $cur       = $v;
        $improved  = 1;
        record($best_size);
      }
    }
  }

  last if !$improved;
  last if $maxpass && $pass >= $maxpass;
}

# ---------------------------------------------------------------- pair sweeps

# A single-value sweep is greedy and order-dependent: it can only accept a value
# that pays off against everything already fixed.  Two parameters that are only
# worth changing TOGETHER are invisible to it.  This pass is quadratic in the
# contested set, so it is opt-in.
if( $pairs && @contested > 1 ) {
  print "\n=== pair sweep (best $best_size)\n";
  my $improved = 1;
  while( $improved ) {
    $improved = 0;
    for my $i (0 .. $#contested - 1) {
      for my $j ($i + 1 .. $#contested) {
        my ($k1, $k2) = @contested[$i, $j];
        for my $v1 (sort keys %{ $rivals{$k1} }) {
          next if $v1 eq $best->{$k1};
          for my $v2 (sort keys %{ $rivals{$k2} }) {
            next if $v2 eq $best->{$k2};
            my %trial = (%$best, $k1 => $v1, $k2 => $v2);
            my $sz = measure(\%trial);
            next unless $sz < $best_size;
            printf "  %s + %s  %9d  <-- accepted\n", $k1, $k2, $sz;
            $best      = \%trial;
            $best_size = $sz;
            $from{$k1} = join(",", @{ $rivals{$k1}{$v1} });
            $from{$k2} = join(",", @{ $rivals{$k2}{$v2} });
            $improved  = 1;
            record($best_size);
          }
        }
      }
    }
  }
}

# ------------------------------------------------------------------- results

write_export();

print "\n=== result\n";
printf "best input   : %-24s %9d\n", $best_name, $input_best;
printf "hybrid       : %-24s %9d\n", $out, $best_size;
printf "gain         : %d byte(s)\n", $input_best - $best_size;
printf "evaluations  : %d run, %d served from cache\n", $evals, $hits;

my %credit;
for my $k (@keys) {
  next if $best->{$k} eq $cand[0][1]{$k};   # unchanged from the embedded profile
  $credit{ $from{$k} }++;
}
if( %credit ) {
  print "descriptors taken from (vs the binary's embedded profile):\n";
  printf "  %-24s %d\n", $_, $credit{$_} for sort keys %credit;
}

unlink $pak;
