
$nam = $ARGV[0];
($dnam=$nam)=~s/\.([^\.]*)$/_macro.$1/;

open( I, "<$nam" ) || die;
open( D, ">$dnam" ) || die;

$f_func=0;
$f_if=0;
while( <I> ) {
  s/[\r\n]//g;
  s/INLINE//g;

  $a=$_;

  $a="// $a" if /^template|^struct|^\}\;/;

  if( m!^(\s*)#(\s*)if 0! ) {
    print D "/* #if 0\n";
    $f_if=1;
    next;
  }

  if( m!^(\s*)#(\s*)if 1! ) {
    print D "/* #if 1 */\n";
    $f_if=2;
    next;
  }

  if( m!^(\s*)#(\s*)else! ) {
    print D "*/\n" if $f_if==1;
    print D "/*\n" if $f_if==2;
    print D "$_\n" if $f_if==0;
    next;
  }

  if( m!^(\s*)#(\s*)endif! ) {
    print D "*/\n" if $f_if==2;
    print D "$_\n" if $f_if==0;
    $f_if=0;
    next;
  }

  if( ($f_func==1) && ($a=~m!^$stop(?:\s*)$!) ) {
    chop $stop;
    print D "$stop\n#enddef\n";
    $f_func=0;
    next;
  }

  # the type and the name have to be identifiers: ([^\s]+) also matches a bare
  # "(", which lets a line like `if( a || ((b)<c) ) {` look like a function
  if( ($f_func==0) && ($a=~m!^(\s*)([A-Za-z_][A-Za-z0-9_\*]*)(?:\s+)([A-Za-z_][A-Za-z0-9_]*)(?:\s*)\((.*)\)(?:\s*)\{!) ) {
    ($func,$args)=($3,$4);
    $f_func=1;
    $stop=$1.'}';

    undef @list;
    $par="{ ";
    $paq="";
    while( $args=~m!(?:\s*)([^\s]+)(?:\s+)([^\s,]+)(?:\s*)(?:,)?!g ) {
#      print D "!!!$1!$2!\n";
      push @list, $2;
      $par .= "$1 _$2_ = __$2__; ";
      $paq .= "$1 $2 = _$2_; ";
    }

    $list = join(",",@list);
    $parmpat = @list ? join('|', map { quotemeta } @list) : undef;
    print D "#define $func\($list\)\n";

    next;
  }

  # Inside a macro body, every use of a parameter gets parenthesised. Without
  # it the body's operator precedence silently applies to whatever the caller
  # passed: ShiftLow's `n*8` turns `ShiftLow( sh>>3 )` into `sh>>3*8`, which
  # is `sh>>24`, which encodes a stream that does not decode. A parenthesised
  # lvalue is still an lvalue, so a parameter the body assigns to is fine.
  $a =~ s/\b($parmpat)\b(?![\w(])/($1)/g if $f_func==1 && defined $parmpat;

  print D "$a\n";
}

__END__

  void rc_Process( uint rcidx, uint cumFreq, uint freq, uint totFreq ) {
