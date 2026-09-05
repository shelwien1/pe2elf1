
# Optional argv: coro_fsm.pl [in [out]].  Defaults match gc.bat, which calls
# this with no arguments from the source directory.
my $inp = $ARGV[0] || "pjpg0i.inc";
my $out = $ARGV[1] || "pjpg0j.inc";

open( I, "<", $inp ) || die "coro_fsm.pl: cannot read $inp: $!";
open( O, ">", $out ) || die "coro_fsm.pl: cannot write $out: $!";

@lin = <I>;

#-----------------------------------------------------------------------------
# The rewrites below are line-oriented: a line containing coro2_get() becomes
# "state=&&mN;return; mN: <that line>", and a line containing coro2_get2()/
# coro2_get4() gets its byte reads hoisted onto lines above it.  That is only a
# correct translation for a few source shapes, and the wrong shape produces code
# that compiles and misbehaves.  Refuse it instead of relying on discipline.
#-----------------------------------------------------------------------------
sub bad { my ($n,$why,$txt) = @_; $txt =~ s/\s+$//;
          die "coro_fsm.pl: $inp line $n: $why\n  $txt\n"; }

# Is a coro2_get() on this line enclosed by the parentheses of a for/while/if/
# switch header?  Walk the line tracking, for every open paren, whether it was
# opened by a control keyword; a get inside one of those is the bad shape.
sub in_header {
  my ($l) = @_;
  my @ctl;                       # one flag per currently-open paren
  my $n = length($l);
  for( my $i=0; $i<$n; $i++ ) {
    my $c = substr($l,$i,1);
    if( $c eq '(' ) {
      my $before = substr($l,0,$i);
      push @ctl, ($before =~ /(?:^|[^A-Za-z0-9_])(?:for|while|if|switch)\s*$/) ? 1 : 0;
    } elsif( $c eq ')' ) {
      pop @ctl;
    } elsif( substr($l,$i) =~ /^coro2_get[24]?\(/ ) {
      return 1 if grep { $_ } @ctl;
    }
  }
  return 0;
}

my $n = 0; my $prev = '';
for my $l ( @lin ) {
  $n++;
  my $code = $l; $code =~ s{//.*$}{};

  my $g1 = () = $code =~ /coro2_get\(/g;          # plain single-byte read
  my $gm = () = $code =~ /coro2_get[24]\(/g;      # multi-byte read

  if( $g1 + $gm > 0 ) {
    # A=..;B=.. are single shared registers, so only one multi-byte read per
    # line can be hoisted, and mixing kinds would reorder the reads.
    bad($n, "more than one coro2_get2()/coro2_get4() on a line", $l) if $gm > 1;
    bad($n, "coro2_get2()/coro2_get4() mixed with coro2_get() on a line", $l) if $gm && $g1;
    bad($n, "more than one coro2_get() on a line", $l) if $gm==0 && $g1 > 1;

    # A get inside a loop header or condition would put the resume label
    # outside the construct it belongs to.
    bad($n, "coro2_get() inside a for/while/if/switch header", $l)
      if $g1+$gm && in_header($code);

    # An unbraced control body would become just the injected "state=..;return;".
    bad($n, "coro2_get() is the unbraced body of the preceding statement", $l)
      if $prev =~ /^\s*(?:\}\s*)?(?:else\s+)?(?:if|for|while)\s*\(.*\)\s*$/
      || $prev =~ /^\s*else\s*$/
      || $prev =~ /^\s*do\s*$/;
  }
  $prev = $code unless $code =~ /^\s*$/;
}


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
