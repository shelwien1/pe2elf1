
# Optional argv: coro_fsm.pl [in [out]].  Defaults match gc.bat, which calls
# this with no arguments from the source directory.
my $inp = $ARGV[0] || "pjpg0i.inc";
my $out = $ARGV[1] || "pjpg0j.inc";

open( I, "<", $inp ) || die "coro_fsm.pl: cannot read $inp: $!";
open( O, ">", $out ) || die "coro_fsm.pl: cannot write $out: $!";

@lin = <I>;

undef @out;
for( @lin ) {
  if( /coro2_get2\(\)/ ) {
    s/coro2_get2\(\)/((A<<8)|B)/g;
    push @out, "A=coro2_get();\r\n";
    push @out, "B=coro2_get();\r\n";
  }
  push @out, $_;
}
@lin = @out;

undef @out;
for( @lin ) {
  if( /coro2_get4\(\)/ ) {
    s/coro2_get4\(\)/((A<<24)|(B<<16)|(C<<8)|D)/g;
    push @out, "A=coro2_get();\r\n";
    push @out, "B=coro2_get();\r\n";
    push @out, "C=coro2_get();\r\n";
    push @out, "D=coro2_get();\r\n";
  }
  push @out, $_;
}
@lin = @out;

$i=1; 
$j=0; undef @p;
undef @out;
for( @lin ) {

  $a = $_;

  if( /coro2_get\(\)/ ) {
    $a=~ s/coro2_get\(\)/coro2_c/g;
    $a = "state=&&m$i;return; m$i: $a"; 
    $p[$j] .= "  case $i: goto m$i;\r\n";
    $i++;
  } else {
    if( /CORO_END;/ ) {
      $j++;
      $a =~ s/CORO_END;/state=0;/g;
    }
  }
  push @out, $a;
}
@lin = @out;

$j=0;
for( @lin ) {
  $a = $_;
  if( /CORO_START;/ ) {
#    $x = "switch(state) {\r\n" . $p[$j] . "default: break;  }\r\n";
    $x = "if_e1( state!=0 ) goto *state;\r\n";
    $a =~ s/CORO_START;/$x/;
    $j++;
  }
  print O $a;
}

#for( @p ) {
#  print O "$_";
#}
