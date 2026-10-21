
# import.pl -- an opt.pl export folded back into an IDX declaration:
#
#   perl import.pl foo.idx export.!!! > new.idx
#
# each knob the export names (its mdesc lines, by the generated name) takes
# the exported pattern; the rest of the file is copied as it is.

open I2, "<$ARGV[1]";

while( <I2> ) {
  s/[\r\n]+$//gs;
# mdesc( M_FF_z_p, -7, "000010000000000" );
  if( /^\s*mdesc\(\s*(.*?)\s*,\s*(.*?)\s*,\s*\"(.*?)\"\s*\)\;/ ) {
    $imp{$1}="$2!$3";
    print STDERR "$1: $2!$3\n";
  }
}

$file = $ARGV[0];
open I1, "<$file";

$prefix = "M_";
$index = "";

while( <I1> ) {
  s/\r//g;
  # Hold any trailing comment aside and put it back after the rewrite.  Every
  # match regex below captures to end of line, so without this the first import
  # would strip every annotation out of the .idx sources.
  $cmt = (s/(\s*\#.*?)[ \t]*$//) ? $1 : '';
  $eol = s/\n$// ? "\n" : '';
  if( /#(.*)$/ ) {
  } elsif( /^\s*Prefix\s+([^\s]*)/ ) {
    $prefix = $1;
    print STDERR "Prefix=<$prefix>\n";
  } elsif( /^\s*Index\s+([^\s]*)/ ) {
    $index = $1;
    print STDERR "Index=<$index>\n";
  } elsif( /^([!\-]?\s*Number\s+(.*?),\s*(.*?)\s*,\s*)(.*?)!(.*)/ ) {
    print STDERR "Number: <$1><$2><$3><$4>\n";
    $z = $imp{"${prefix}_${2}_"};
    $_ = "$1$z\n" if defined $z;
  } elsif( /^([!\-]?\s*(.*?):\s*(.*?),\s*)(.*?)!(.*)$/ ) {
    print STDERR "<$1>:<$2>,<$3>!\n";
    $z = $imp{"${prefix}_${2}"};
    $_ = "$1$z\n" if defined $z;
  } elsif( /^([!\-]?\s*(.*?):\s*(.*?),\s*)\&(.*)$/ ) {
    print STDERR "<$1>:<$2>,<$3>&\n";
    $s = $1;
    $z = $imp{"${prefix}_${2}"};
    $z =~ s/^[^!]*!//;
    $_ = "$s\&$z\n" if defined $z;
  }
  s/\n$//;
  print "$_$cmt$eol";
}

close I1;

