# opt.pl -- parameter optimizer for aacpack's model (h264p's, from oggcomp's).
#
# Flips the bits of the "!MAP!" pattern strings the tuning build embeds in the
# executable, keeping whatever shrinks the corpus.  It does not parse or rebuild
# anything: the patterns are patched in the binary in place, so one build serves
# the whole hill-climb.  The build must therefore be the tuning build of a
# level -- ./mk.sh tuning L, which writes ../aacpack.  ./mk.sh release folds
# every parameter to a literal and there is nothing left to find.
#
#   perl IDX/opt.pl corpus-file-list [exe]       (in aacpack_src)
#
# corpus-file-list: text file, one AAC stream path per line (opt.lst if not
# given).  Optimizing on one stream overfits it, and the corpus has to reach
# every context the parameters cover: streams of different encoders, rates
# and object types, mono and stereo, long and short windows, SBR and PS.
#
# exe: which binary to patch (default ../aacpack).  Point it at a copy while
# tuning; opt.pl rewrites the file it is given.
#
# Results are written continuously to export.!!! (mdesc lines) and progress to
# opttimes.!!!.  Fold them back into the .idx sources with:
#   cd IDX/lL && for f in *.idx; do perl ../import.pl $f ../../export.!!! > t && mv t $f; done
# then ./mk.sh mod, and ./mk.sh check L to prove the shipping build still
# agrees.  (IDX/ftune.py tunes a family at a time against its dumps, faster.)
#
# One caution: aacpack's parameters are part of its format.  A stream coded by a
# patched binary decodes only under that same patched binary, so throw away
# anything mid-hill-climb writes.  Interrupted (INT, TERM, HUP), it puts the
# best patterns so far back in the binary, as export.!!! has them, and stops.

$exe     = $ARGV[1] || "../aacpack";
$tmp     = "$exe.opt_tmp";
$rndfrac = 0.75;   # bit is flipped in the initial random kick when rand>this

#---------------------------------------------------------------- corpus

$lst = $ARGV[0] // "opt.lst";
open L, "<$lst" or die "opt.pl: no corpus list $lst (one stream path per line)\n";
while(<L>) { s/[\r\n]+$//; push @files,$_ if /\S/ && !/^\s*#/; } close L;
die "opt.pl: $lst lists no stream\n" if !@files;
printf "corpus: %i file(s): %s\n", scalar(@files), join(" ",@files);

# a signal: the binary back to the best patterns so far (the pattern being
# tried, $addr, is the only one that differs), and stop
$stop = 0;
$SIG{INT} = $SIG{TERM} = $SIG{HUP} = sub { $stop = 1; };
sub interrupted {
  wexe( $addr, substr($exe_data,$addr,$leng) ) if defined $addr;
  unlink $tmp;
  die "opt.pl: interrupted -- $exe has the best patterns so far, as export.!!! has them\n";
}

# the binary compressing a stream, its output nowhere (no shell: the paths are
# as they are); its exit status
sub run_c {
  open my $so, '>&', \*STDOUT or die "opt.pl: $!\n";
  open my $se, '>&', \*STDERR or die "opt.pl: $!\n";
  open STDOUT, '>', '/dev/null'; open STDERR, '>', '/dev/null';
  my $rc = system { $exe } $exe, 'c', @_;
  open STDOUT, '>&', $so; open STDERR, '>&', $se;
  return $rc;
}

sub measure {
  my $t = 0;
  for my $f (@files) {
    unlink $tmp;
    my $rc = run_c( $f, $tmp );
    # (system() ignores INT and QUIT while the binary runs: a ^C ends the binary)
    interrupted() if $stop || ($rc & 127) == 2 || ($rc & 127) == 15 || ($rc & 127) == 1;
    my $s = -s $tmp;
    # The exit status decides, not the size.  A run that dies partway through
    # has already flushed part of its output, and a part of an 800 kB stream
    # is 64 kB -- which, measured by size alone, is the best result the search
    # has ever seen, so it walks towards whatever killed it and stays there.
    # aacpack removes its own half-written output, but a signal it cannot
    # catch (the OOM killer) still leaves one, and this is the check that does
    # not depend on the program getting the chance to tidy up.
    return 0x7FFFFFFF if $rc != 0 || !defined($s) || $s < 64;
    $t += $s;
  }
  return $t;
}

#---------------------------------------------------------------- map table

undef $/;
open INP, "<$exe" or die "no $exe -- build it first (./mk.sh tuning L)\n";
binmode INP; $exe_data = <INP>; close INP;

$lsum = 0;
while( $exe_data=~/!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`)+5+(length($1)+1+length($2))+1;
  $ofs{$1} = $2;
  $lsum += ($len{$1} = length($3));
}
for (sort keys %adr) { printf "%08X %-16s %i\n", $adr{$_}, $_, $len{$_}; }
printf "%i maps, %i bits total\n", scalar(keys %adr), $lsum;
die "no !MAP! markers -- this is a Const/release build\n" if $lsum==0;

sub wexe {                      # write $_[1] at offset $_[0]
  open EXE,"+<$exe" or die "opt.pl: cannot write $exe: $!\n"; binmode EXE;
  seek EXE,$_[0],0; print EXE $_[1]; close EXE;
}
sub export {
  open OUT, ">export.!!!" or die "opt.pl: cannot write export.!!!: $!\n";
  for my $k (sort keys %adr) {
    printf OUT "mdesc( %s, %s, \"%s\" );\n", $k, $ofs{$k}, substr($exe_data,$adr{$k},$len{$k});
  }
  close OUT;
}

$bass = measure(); print "=== start $bass\n";
die "opt.pl: $exe fails on a stream of $lst (or cannot be run): nothing to measure against\n" if $bass == 0x7FFFFFFF;
export();

$flag=1; $ct0=0;

while( $flag==1 ) {

  $flag = 0; $lcnt=$lsum; $stim=time();
  for $key (sort keys %adr) {
    $addr = $adr{$key};
    $leng = $len{$key};
    printf "!!! %06X: %s !!!\n", $addr, $key;

    $rmap = substr($exe_data,$addr,$leng);
    $vmap = join( "", map( chr((rand>$rndfrac)), 1..$leng ) ); # random kick
    $vmap ^= $rmap;

    wexe( $addr, $vmap );
    $s = measure();

    $vflag=1; $bass0=$bass;
    while( $vflag==1 ) {
      $vflag = 0;
      @list = 0..($leng-1);
      print "=== $bass\n";
      for( $n=$#list; $n>=0; $n-- ) {
        $i = splice(@list,int(rand($#list+1)),1);
        $f = (substr($vmap,$i,1) ^= 1);
        wexe( $addr, $vmap );
        $cs = measure();
        if( ($cs>$s) || (($cs==$s) && ($f&1)) ) {
          $f = (substr($vmap,$i,1) ^= 1);       # revert
          wexe( $addr+$i, $f );
          $ns = $s;
        } else {
          $vflag=1 if $s!=$cs;
          $ns = $cs;
        }
        $s = $ns;
        $vflag=0 if( ($s eq $bass0) and (substr($exe_data,$addr,$leng) eq $vmap) );
        if( $s<$bass ) {
          $flag=1;
          substr($exe_data,$addr,$leng) = $vmap;
          $bass = $s;
          open OUT, ">>opttimes.!!!" or die "opt.pl: cannot write opttimes.!!!: $!\n";
          printf OUT "{ %10i, %7i },\n", time, $bass; close OUT;
          export();
        }
        $ctim=time()-$stim;
        printf "%3i $f $s $cs $vflag$flag $lcnt ${ctim}/${ct0}\n", $i;
        $lcnt--;
      }
    }
    wexe( $addr, substr($exe_data,$addr,$leng) );   # restore best
  } $ct0 = $ctim;
}

unlink $tmp;
print "Done: $bass\n";
