# auto_opt2.pl -- linux port of the 022d optimizer driver.
#
# Same algorithm as the windows original, byte for byte in its search order:
# find every "!MAP!name!ofs\0pattern\0" descriptor that sh_mapping.inc's
# pdesc/pmask left in the executable, then hill-climb the pattern bits IN THE
# BINARY, one flip at a time, keeping a flip when the coded size does not grow.
#
# Three things had to change for linux and nothing else did:
#   $exe    -- "xadpcm", not "xadpcm.exe".
#   $exe1   -- ">nul" is a FILE on linux, so the run would silently write a
#              file called `nul` and print nothing; it is "/dev/null" here.
#   $exe1   -- "|| rm -f $pak" and a timeout.  A patched pattern can make the
#              codec abort (the arena overrun check) or run away, and a
#              half-written $pak is SMALLER than the record, so without this the
#              climber would accept a truncated file as an improvement.  The
#              missing file falls into the existing `$s<100` guard instead.
#              `ulimit -v` bounds a pattern that asks for an absurd table set;
#              a normal run peaks at ~1.6 GB of address space.
$exe = "xadpcm";
$exe1 = "ulimit -v 4194304; timeout 900 ./$exe c wavs3 book1bwt.ari >/dev/null 2>&1 || rm -f book1bwt.ari";
$pak = "book1bwt.ari";

undef $/;
open INP, "<$exe";
binmode INP;
$exe_data = <INP>;
close INP;

$lsum = 0;
while( $exe_data=~/!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`)+5+(length($1)+1+length($2))+1;
  $ofs{$1} = $2;
  $lsum += ($len{$1} = length($3));
}

for (sort keys %adr){
  printf "%08X %s %i\n", $adr{$_}, $_, $len{$_};
}
printf "maps=%i lsum=%i\n", scalar(keys %adr), $lsum;
$| = 1;

$flag=1; $ct0=0;

# initial run
system $exe1;
1 while not open( EXE,"+<$exe" ); close EXE;
$bass = -s $pak; # exe_data's result
$bass = 0x7FFFFFFF if $bass<100;

while( $flag==1 ) {

  $flag = 0; $lcnt=$lsum; $stim=time();
  for $key (sort keys %adr) {
    $addr = $adr{$key};
    $leng = $len{$key};

    $sadr = sprintf "%06X",$addr;
    print "!!! $sadr: $key !!!\n";

# -------------- single map loop {

    $rmap = substr($exe_data,$addr,$leng);
#    $vmap = join( "", map( chr(48+(rand>0.75)), 1..$leng ) ); # random map
    $vmap = join( "", map( chr((rand>0.75)), 1..$leng ) ); # random map
    $vmap ^= $rmap;

    # calc vmap's base size
    1 while not open( EXE,"+<$exe" );
    binmode EXE;
    seek EXE,$addr,0;
    print EXE $vmap;
    close EXE;
    system $exe1;
    1 while not open( EXE,"+<$exe" ); close EXE;
    $s = (-s "$pak"); # vmap's result
    $s = 0x7FFFFFFF if $s<100;

    $vflag=1; $bass0=$bass;
    while( $vflag==1 ) {
      $vflag = 0;
      @list = 0..($leng-1);
      print "=== $bass\n";
      for( $n=$#list; $n>=0; $n-- ) {
        $i = splice(@list,int(rand($#list+1)),1);
        unlink "$pak";
        1 while not open EXE,"+<$exe";
        binmode EXE;
        $f = (substr($vmap,$i,1) ^= 1);
        seek EXE,$addr,0;
        print EXE $vmap;
        close EXE;
        system $exe1;
        1 while not open( EXE,"+<$exe" ); close EXE;
        $cs = (-s "$pak");
        $cs = 0x7FFFFFFF if $cs<100;
        if( ($cs>$s) || (($cs==$s) && ($f&1)) ) {
          1 while not open EXE,"+<$exe";
          binmode EXE;
          seek EXE,$addr+$i,0; $f = (substr($vmap,$i,1) ^= 1);
          print EXE $f;
          close EXE;
          $ns = $s;
        } else {
          if( $s!=$cs ) { $vflag=1; }
          $ns = $cs;
        }
        $s = $ns;
        $vflag=0 if( ($s eq $bass0) and (substr($exe_data,$addr,$leng) eq $vmap) );
        if( $s<$bass ) {
          $flag=1;
          substr($exe_data,$addr,$leng) = $vmap;
          $bass = $s;
          {
            local ($key,$addr,$leng,$sadr,$f);

            1 while not open OUT, ">>opttimes.!!!";
            printf OUT "{ %10i, %7i },\n", time, $bass;
            close OUT;

            1 while not open OUT, ">export.!!!";
            for $key (sort keys %adr) {
              $addr = $adr{$key};
              $leng = $len{$key};
              $sadr = sprintf "%06X",$addr;
              $f = substr $exe_data, $addr, $leng;
              printf OUT "mdesc( %s, %s, \"%s\" );\n", $key, $ofs{$key}, $f;
            }
            close OUT;
          }
        }
        $ctim=time()-$stim;
        printf "%3i $f $s $cs $vflag$flag $lcnt ${ctim}/${ct0}\n", $i;
        $lcnt--;
      }
    }
    1 while not open EXE,"+<$exe";
    binmode EXE;
    seek EXE,$addr,0;
    print EXE substr($exe_data,$addr,$leng);
    close EXE;
# -------------- single map loop }


  } $ct0 = $ctim;
}

print "Done\n";
