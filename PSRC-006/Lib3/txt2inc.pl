
# txt2inc.pl [-raw] <in> <out> -- wrap a text file up as C string literals.
#
#   [[NAME]]                     starts   char NAME[] =
#   [[static const char NAME]]   starts   static const char NAME[] =
#
# Everything before the first marker is preamble and is not emitted, so a
# source file can carry a header comment that stays out of the generated
# string.
#
# By default the result is safe to hand to printf: % is doubled and $DATE,
# $DATA and $DATX interpolate. -raw turns all of that off, for a string that
# is not a format -- an OpenCL kernel, say, where a stray %% inside a
# `k%RCNUM` would go to the device compiler verbatim.

sub getdate {
  my ($s,$m,$h,$d,$mn,$y) = localtime(time);
  return sprintf "%02i.%02i.%04i %02i:%02i", $d,$mn+1,$y+1900, $h,$m;
}

$curdate = getdate();

my @a = grep { !/^-/ } @ARGV;
my $raw = grep { $_ eq "-raw" } @ARGV;

open( I, "<$a[0]" ) || die;
open( O, ">$a[1]" ) || die;

$flag=0;
print O "\n// generated from $a[0] by txt2inc.pl -- do not edit\n\n";
while( <I> ) {
  if( /\[\[(.*?)\]\]/ ) {
    my $decl = "char"; my $name = $1;
    if( $name =~ /^\s*(.*\S)\s+(\S+)\s*$/ ) { $decl = $1; $name = $2; }
    print O ";\n\n" if $flag;
    print O "$decl $name\[\] = \n"; $flag=1;
  } elsif( $flag ) {
    s/[\r\n]//g;
    s/\s+$//;
    if( /^;$/ ) {
    } else {
      s/\\/\\\\/g;
      s/"/\\"/g;
      if( !$raw ) {
        s/%/%%/g;
        s/\$DATA/%s/g;
        s/\$DATX/%X/g;
        s/\$DATE/$curdate/g;
      }
      print O "\"$_\\n\"\n";
    }
  }
}
print O ";\n" if $flag;
