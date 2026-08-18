# opt.pl -- parameter optimizer for dxt5comp (adapted from auto_opt2.pl)
#
# Flips the bits of the "!MAP!" pattern strings embedded in the executable by
# the Debug-mode pdesc/pmask macros, keeping whatever shrinks the total output.
# The build must therefore be the Debug build (mk.sh / g.bat with no argument).
#
#   perl IDX/opt.pl [corpus-file-list]
#
# corpus-file-list: text file, one .dds path per line (default: opt.lst,
# and if that is missing, the single file $deffile below). Optimizing on one
# image overfits it; 5-20 assorted dds files is a much better target.
#
# Results are written continuously to export.!!! (mdesc lines) and progress
# to opttimes.!!!. Fold them back into the .idx sources with:
#   cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done

$exe     = "./dxt5comp";
$deffile = "0000006F.dds";
$tmp     = "opt_tmp.d5c";
$rndfrac = 0.75;   # bit is flipped in the initial random kick when rand>this

#---------------------------------------------------------------- corpus

$lst = $ARGV[0];
$lst = "opt.lst" if !defined($lst) && -e "opt.lst";
if( defined($lst) && -e $lst ) {
  open L, "<$lst"; while(<L>) { s/[\r\n]+$//; push @files,$_ if /\S/ && !/^\s*#/; } close L;
} else {
  @files = ($deffile);
}
printf "corpus: %i file(s): %s\n", scalar(@files), join(" ",@files);

sub measure {
  my $t = 0;
  for my $f (@files) {
    unlink $tmp;
    system( "$exe c \"$f\" $tmp >/dev/null 2>&1" );
    my $s = -s $tmp;
    return 0x7FFFFFFF if !defined($s) || $s < 64;   # crashed / refused
    $t += $s;
  }
  return $t;
}

#---------------------------------------------------------------- map table

undef $/;
open INP, "<$exe" or die "no $exe -- build the Debug version first\n";
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
  1 while not open EXE,"+<$exe"; binmode EXE;
  seek EXE,$_[0],0; print EXE $_[1]; close EXE;
}
sub export {
  1 while not open OUT, ">export.!!!";
  for my $k (sort keys %adr) {
    printf OUT "mdesc( %s, %s, \"%s\" );\n", $k, $ofs{$k}, substr($exe_data,$adr{$k},$len{$k});
  }
  close OUT;
}

$bass = measure(); print "=== start $bass\n";
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
          1 while not open OUT, ">>opttimes.!!!";
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

print "Done: $bass\n";
