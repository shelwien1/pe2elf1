# apply_export.pl <exe> <export.!!!> -- the inverse of dump_export.pl.
#
# Patches a binary's !MAP! pattern strings in place from an optimizer export, so
# an instance can be put back on a known-good parameter set without a rebuild.
# That is what a resume needs: auto_opt2.pl only writes its best set back into
# the exe when it FINISHES a descriptor, so a run killed mid-descriptor leaves a
# randomized map in the file, and restarting from it would throw away the gain
# that export.!!! already recorded.
#
# Refuses a length mismatch rather than truncating: a descriptor whose pattern
# changed length is a different parameter, and silently writing the short one
# would leave trailing bytes of the old map behind.
use strict; use warnings;
my ($exe, $exp) = @ARGV;
die "usage: apply_export.pl <exe> <export>\n" unless defined $exp;

my $d = do { local $/; open my $f, "<", $exe or die "$exe: $!"; binmode $f; <$f> };
my (%adr, %len);
while( $d =~ /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) {
  $adr{$1} = length($`)+5+(length($1)+1+length($2))+1;
  $len{$1} = length($3);
}

open my $i, "<", $exp or die "$exp: $!";
my ($n, $skip) = (0, 0);
open my $o, "+<", $exe or die "$exe: $!"; binmode $o;
while( <$i> ) {
  next unless /^\s*mdesc\(\s*(.*?)\s*,\s*(.*?)\s*,\s*"(.*?)"\s*\)\s*;/;
  my ($key, $map) = ($1, $3);
  unless( exists $adr{$key} ) { warn "apply_export: $key not in $exe\n"; $skip++; next }
  die "apply_export: $key length ".length($map)." != $len{$key} in $exe\n"
    if length($map) != $len{$key};
  seek $o, $adr{$key}, 0;
  print $o $map;
  $n++;
}
close $o; close $i;
print "apply_export: $exe <- $exp ($n patched, $skip skipped)\n";
