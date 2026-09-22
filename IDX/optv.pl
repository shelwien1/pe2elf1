# optv.pl -- value-step coordinate descent on the knobs of a tuning build
# (SSE-DESIGN.md sec.6.10, coder0_optimizer_improvements_v3.md A4).
#
# Same contract as opt.pl: it patches the "!MAP!name!base\0pattern" strings
# of the executable in place, keeps whatever shrinks the corpus, and writes
# the winners continuously to export.!!! (mdesc lines for import.pl).  The
# difference is the move: opt.pl flips single bits of a pattern in random
# order after a random kick, which for a 20-bit rate knob is either a huge
# jump or noise; optv.pl treats a Number knob as a value and moves it by
# +-step, step starting at a quarter of the value and halving down to
# 1/256 of it (0.4%, about the noise floor), accelerating (step doubles)
# while a direction keeps winning.  Knobs are revisited in the order of
# their last measured gain, and passes repeat while a pass still gains.
# Masks (pmask/pmask2, names without the trailing '_') are left to opt.pl.
#
#   perl IDX/optv.pl [corpus-file-list] [exe] [map-name-regex]
#
#   OPT_JOBS=N      compress up to N corpus files at a time (default 1)
#   OPT_MINREL=x    stop refining a knob below a step of x*value (default 1/256)
#   OPT_STEP0=x     first step as a fraction of the value (default 1/4)
#   OPT_MAXPASS=n   passes over the knob list (default 8)
#   OPT_ORDER=a,b,c knobs to climb first, in this order (e.g. the biggest
#                   gains of a previous run); the rest follow alphabetically
#   OPT_SCAN=k      before a knob's local search, try value*2^j for j in
#                   -k..k (j != 0) and start from the best: a momentum or a
#                   rate can have its optimum a decade away from the seed,
#                   where +-step never arrives (C0 M2_k: 32 -> 2048 events)
#
# Progress goes to stdout, the best total so far to opttimes.!!! as in opt.pl.

$exe     = $ARGV[1] || "./coder0t";
$tmp     = "$exe.opt_tmp";
$jobs    = $ENV{OPT_JOBS} || 1;
$minrel  = defined $ENV{OPT_MINREL} ? $ENV{OPT_MINREL} : 1/256;
$step0f  = defined $ENV{OPT_STEP0}  ? $ENV{OPT_STEP0}  : 1/4;
$maxpass = $ENV{OPT_MAXPASS} || 8;
$scan    = $ENV{OPT_SCAN} || 0;

#---------------------------------------------------------------- corpus
$lst = $ARGV[0];
$lst = "opt.lst" if !defined($lst) && -e "opt.lst";
die "no corpus list\n" if !defined($lst) || !-e $lst;
open L, "<$lst"; while(<L>) { s/[\r\n]+$//; push @files,$_ if /\S/ && !/^\s*#/; } close L;
printf "corpus: %i file(s): %s\n", scalar(@files), join(" ",@files);

$nmeas = 0;
sub measure {
  my $t = 0; my (@pids, @outs); my $bad = 0;
  $nmeas++;
  for my $i (0..$#files) {
    my $f = $files[$i]; my $o = "$tmp.$i"; unlink $o;
    if( $jobs > 1 ) {
      while( @pids >= $jobs ) { waitpid( shift @pids, 0 ); $bad = 1 if $?; }
      my $pid = fork();
      if( !$pid ) { exec( "$exe c \"$f\" $o >/dev/null 2>&1" ); exit 1; }
      push @pids, $pid;
    } else { system( "$exe c \"$f\" $o >/dev/null 2>&1" ); $bad = 1 if $?; }
    push @outs, $o;
  }
  for (@pids) { waitpid( $_, 0 ); $bad = 1 if $?; }
  for my $o (@outs) { my $s = -s $o; unlink $o; return 0x7FFFFFFF if $bad || !defined($s) || $s < 64; $t += $s; }
  return $t;
}

#---------------------------------------------------------------- map table
{ local $/; open INP, "<$exe" or die "no $exe -- build it first (./gc.sh tune)\n"; binmode INP; $exe_data = <INP>; close INP; }
while( $exe_data=~/!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`)+5+(length($1)+1+length($2))+1;
  $ofs{$1} = $2; $len{$1} = length($3);
}
die "no !MAP! markers -- this is a Const/release build\n" if !%adr;

$filt = $ARGV[2];
@keys = grep { /_$/ } sort keys %adr;            # Number/Rate knobs only
@keys = grep { /$filt/ } @keys if defined $filt;
die "no Number map matches\n" if !@keys;
if( $ENV{OPT_ORDER} ) {                          # these first, in the given order, then the rest
  my %in = map { $_ => 1 } @keys;
  my @first = grep { $in{$_} } split /,/, $ENV{OPT_ORDER};
  my %f = map { $_ => 1 } @first;
  @keys = ( @first, grep { !$f{$_} } @keys );
}
printf "%i knobs, %i bits: %s\n", scalar(@keys), eval { my $b=0; $b+=$len{$_} for @keys; $b }, join(" ", @keys);

sub getv { my $k = shift; oct("0b".substr($exe_data,$adr{$k},$len{$k})); }
sub setv {                       # write value v (pattern int) of knob k into exe and exe_data
  my ($k,$v) = @_;
  my $bits = sprintf "%0$len{$k}b", $v;
  substr($exe_data,$adr{$k},$len{$k}) = $bits;
  1 while not open EXE,"+<$exe"; binmode EXE; seek EXE,$adr{$k},0; print EXE $bits; close EXE;
}
sub export {
  1 while not open OUT, ">export.!!!";
  for my $k (sort keys %adr) { printf OUT "mdesc( %s, %s, \"%s\" );\n", $k, $ofs{$k}, substr($exe_data,$adr{$k},$len{$k}); }
  close OUT;
}
sub note { 1 while not open OUT, ">>opttimes.!!!"; printf OUT "{ %10i, %7i },\n", time, $best; close OUT; }

$best = measure(); print "=== start $best\n"; export();
$t0 = time;

for my $pass (1..$maxpass) {
  my $pass_start = $best;
  my @order = @keys;
  @order = sort { ($gain{$b}||0) <=> ($gain{$a}||0) } @keys if $pass > 1;
  for my $k (@order) {
    my $vmax = (1<<$len{$k}) - 1;
    my $p = getv($k);
    my $kstart = $best;
    my $ref  = $p > 16 ? $p : 16;                  # scale of the first step / the floor
    my $step = int($ref * $step0f); $step = 1 if $step < 1;
    my $minstep = int($ref * $minrel); $minstep = 1 if $minstep < 1;
    printf "!!! %s = %i (+%s) [%i bits] step %i..%i  best %i  %is\n", $k, $p, $ofs{$k}, $len{$k}, $step, $minstep, $best, time-$t0;
    if( $scan ) {                                  # coarse geometric scan first
      my %seen = ($p => 1); my $bp = $p;
      for my $j (-$scan..$scan) {
        next if !$j;
        my $q = int( ($p > 0 ? $p : 1) * 2**$j + 0.5 ); $q = $vmax if $q > $vmax;
        next if $seen{$q}++;
        setv($k, $q); my $s = measure();
        printf "  %s scan %i -> %i : %i (%+i)\n", $k, $p, $q, $s, $s-$best;
        if( $s < $best ) { $best = $s; $bp = $q; export(); note(); }
      }
      $p = $bp; setv($k, $p);
      $ref  = $p > 16 ? $p : 16;
      $step = int($ref * $step0f); $step = 1 if $step < 1;
      $minstep = int($ref * $minrel); $minstep = 1 if $minstep < 1;
    }
    while( $step >= $minstep ) {
      my $improved = 0;
      for my $dir (1, -1) {
        my $q = $p + $dir*$step; $q = 0 if $q < 0; $q = $vmax if $q > $vmax;
        next if $q == $p;
        setv($k, $q); my $s = measure();
        printf "  %s %i -> %i : %i (%+i)\n", $k, $p, $q, $s, $s-$best;
        if( $s < $best ) {
          $p = $q; $best = $s; $improved = 1; export(); note();
          my $st2 = $step*2;                       # accelerate in the winning direction
          while( 1 ) {
            my $q2 = $p + $dir*$st2; $q2 = 0 if $q2 < 0; $q2 = $vmax if $q2 > $vmax;
            last if $q2 == $p;
            setv($k, $q2); my $s2 = measure();
            printf "  %s %i -> %i : %i (%+i) [accel]\n", $k, $p, $q2, $s2, $s2-$best;
            if( $s2 < $best ) { $p = $q2; $best = $s2; export(); note(); $st2 *= 2; } else { last; }
          }
          last;
        }
      }
      setv($k, $p);                                # back to the incumbent value
      $step = int($step/2) if !$improved;
    }
    $gain{$k} = $kstart - $best;
    printf "=== %s done: %i, gain %i, %i evals, %is\n", $k, $p, $gain{$k}, $nmeas, time-$t0;
  }
  printf "=== pass %i: %i -> %i\n", $pass, $pass_start, $best;
  last if $best >= $pass_start;
}
print "Done: $best\n";
