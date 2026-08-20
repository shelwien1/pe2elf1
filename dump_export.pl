# dump_export.pl <exe> -- print the !MAP! descriptors of a binary in the exact
# form auto_opt2.pl's export.!!! uses.  Two jobs: it is the round-trip check for
# import.sh (dump an untouched build, import it, and no .idx may change), and it
# turns any patched binary into an importable export without waiting for the
# climber to find its next improvement.
undef $/;
my $exe = $ARGV[0] || "xadpcm";
open INP, "<$exe" or die "$exe: $!";
binmode INP;
my $d = <INP>;
close INP;
my (%adr, %ofs, %len);
while( $d =~ /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`)+5+(length($1)+1+length($2))+1;
  $ofs{$1} = $2;
  $len{$1} = length($3);
}
for my $key (sort keys %adr) {
  printf "mdesc( %s, %s, \"%s\" );\n", $key, $ofs{$key}, substr($d,$adr{$key},$len{$key});
}
