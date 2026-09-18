# setp.pl -- set IDX knobs in a tuning build without rebuilding it.
#
#   perl IDX/setp.pl <exe> NAME=VALUE [NAME=VALUE ...]
#
# NAME is a map name as opt.pl lists it (S0_W_, S0_c2, ...); the trailing
# underscore of Number/Rate knobs may be left out (S0_W).  VALUE is the final
# value of a Number knob (pattern + base, i.e. what the generated header
# defines), or &<bits> to write a raw pattern -- the only form for masks.
# The pattern length is fixed by the build, so a value that does not fit
# that many bits is an error and nothing is written.  Prints each change as
# an mdesc line, the same format opt.pl exports and import.pl reads.
#
# The patch is the same in-place edit of the "!MAP!name!base\0pattern\0"
# strings that opt.pl performs, so an exe patched here measures exactly what
# a hill-climb reaching the same pattern would.

$exe = shift @ARGV or die "usage: setp.pl <exe> NAME=VALUE ...\n";
undef $/;
open INP, "<$exe" or die "no $exe\n"; binmode INP; $exe_data = <INP>; close INP;

while( $exe_data=~/!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`)+5+(length($1)+1+length($2))+1;
  $ofs{$1} = $2;
  $len{$1} = length($3);
}
die "no !MAP! markers in $exe -- this is a Const/release build\n" if !%adr;

for my $a (@ARGV) {
  my ($name,$val) = $a =~ /^(.*?)=(.*)$/ or die "bad argument: $a\n";
  my $k = exists $adr{$name} ? $name : exists $adr{"${name}_"} ? "${name}_" : die "no map named $name in $exe\n";
  my $bits;
  if( $val =~ /^&([01]+)$/ ) {
    $bits = $1;
    die "$k: pattern must be $len{$k} bits, got ".length($bits)."\n" if length($bits) != $len{$k};
  } else {
    die "$k: value must be an integer or &bits\n" if $val !~ /^-?\d+$/;
    my $v = $val - $ofs{$k};
    die "$k: value $val is below base $ofs{$k}\n" if $v < 0;
    die "$k: value $val needs more than $len{$k} bits (max ".((1<<$len{$k})-1+$ofs{$k}).")\n" if $v >= (1<<$len{$k});
    $bits = sprintf "%0$len{$k}b", $v;
  }
  push @w, [$k,$bits];
}

open EXE, "+<$exe" or die "cannot write $exe\n"; binmode EXE;
for (@w) {
  my ($k,$bits) = @$_;
  seek EXE, $adr{$k}, 0; print EXE $bits;
  printf "mdesc( %s, %s, \"%s\" );\n", $k, $ofs{$k}, $bits;
}
close EXE;
