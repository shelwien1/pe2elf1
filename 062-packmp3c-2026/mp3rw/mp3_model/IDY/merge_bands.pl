
#open( I, "<sh_model_Sm0A.idx" ) || die;
#open( O, ">1" ) || die;

if( not defined $ARGV[0] ) {

  while( <STDIN> ) {

    if( /^(?:([!\-]?\s)*(.*?):\s*(.*?),\s*)(.*?)!\+(.*)$/ ) {
      ($a0,$a1,$a2,$a3,$a4)=($1,$2,$3,$4,$5);

      if( not defined $band ) {
        $band = "$a0$a1: $a2, $a3!$a4";
      } else {
        $band .= $a4;
      }

      #print STDERR "$a0$a1: $a2, $a3!$a4\n";

    } else {

      if( defined $band ) {
        print "$band\n";
        undef $band;
      }

      print $_;

    }

  }

} else {

  print while( <STDIN> );

}