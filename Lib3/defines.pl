
$nam = $ARGV[0];
($dnam=$nam)=~s/\.([^\.]*)$/_D.$1/;
($unam=$nam)=~s/\.([^\.]*)$/_U.$1/;

open( I, "<$nam" ) || die;
open( D, ">$dnam" ) || die;
open( U, ">$unam" ) || die;

$f_def = 0;

while( <I> ) {
  s/[\r\n]//g;

  s!\/\/(.*)$!/* $1 */!;

  if( /^#enddef/ ) {
    $f_def=0;
    $_ = "}";
  }

  # only a define with nothing after its parameter list opens a block -- that is
  # what rc_macro.pl emits. An ordinary object-like define has a body on the
  # same line and must be left alone, or it collects the rest of the file.
  if( /^#define\s+([^\s\(]+)(?:\([^\)]*\))?\s*$/ ) {
    $f_def=1;
    $_ .= " {";
    print U "#undef $1\n";
  }

  if( $f_def ) { $_ .= "\\"; }

  print D "$_\n";

}
