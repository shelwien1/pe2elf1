
# z_p:   p,  -7!000000000000000

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
  $orig = $_;
  $comment = "";
  if( s/(\s*#.*)$// ) {
    $comment = $1;
  }
  if( /^\s*$/ && $comment ne "" ) {
    # line was only a comment
    print $orig;
    next;
  } elsif( /^\s*Prefix\s+([^\s]*)/ ) {
    $prefix = $1;
    print STDERR "Prefix=<$prefix>\n";
  } elsif( /^\s*ADD\s+(.*?):\s*(.*)/ ) {
    print STDERR "ADD $1:$2\n";
  } elsif( /^\s*Index\s+([^\s]*)/ ) {
    $index = $1;
    print STDERR "Index=<$index>\n";
  } elsif( /^([!\-]?\s*Number\s+(.*?),\s*(.*?)\s*,\s*)(.*?)!(.*)/ ) {
    print STDERR "Number: <$1><$2><$3><$4>\n";
    $z = $imp{"${prefix}_${2}_"};
    $_ = "$1$z" if defined $z;
  } elsif( /^([!\-]?\s*Rate\s+(.*?),\s*)(.*?)!(.*?)\s*$/ ) {
    print STDERR "Rate: <$1><$2><$3>\n";
    $z = $imp{"${prefix}_${2}_"};
    $_ = "$1$z" if defined $z;
  } elsif( /^([!\-]?\s*Rate1\s+(.*?),\s*)(.*?)!(.*?)\s*$/ ) {
    print STDERR "Rate: <$1><$2><$3>\n";
    $z = $imp{"${prefix}_${2}_"};
    $_ = "$1$z" if defined $z;
  } elsif( /^([!\-]?\s*(.*?):\s*(.*?),\s*)(.*?)!(.*)$/ ) {
    print STDERR "<$1>:<$2>,<$3>!\n";
    $z = $imp{"${prefix}_${2}"};
    $_ = "$1$z" if defined $z;
  } elsif( /^([!\-]?\s*(.*?):\s*(.*?),\s*)\&(.*)$/ ) {
    print STDERR "<$1>:<$2>,<$3>&\n";
    $s = $1;
    $z = $imp{"${prefix}_${2}"};
    $z =~ s/^[^!]*!//;
    $_ = "$s\&$z" if defined $z;
  } elsif( /^([!\-]?\s*(.*?):\s*(.*?),\s*)b\&(.*)$/ ) {
    print STDERR "<$1>:<$2>,<$3>&b\n";
    $s = $1;
    $z = $imp{"${prefix}_${2}"};
    $z =~ s/^[^!]*!//;
    $_ = "${s}b\&$z" if defined $z;
  }
  chomp;
  print "$_$comment\n";
}

# z_p:   p,  -7!000000000000000
# ADD    2:  (n&1)
# M_FF_z_col.Size

close I1;

