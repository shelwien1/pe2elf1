# idx2inc.pl -- an IDX declaration and its template to C++ (IDX-FORMAT.md):
#
#   perl idx2inc.pl foo.idx [UseNew]
#
# reads foo.idx and its template, foo.inc or else family.inc beside it (a
# template for every .idx there: %F% in it is the family's name, the Prefix
# after its first "_"), and writes foo_h.inc (the mappings, constants,
# Volumes, the USES list and the tables' struct) and foo_p.inc (the
# template, its MakeIndex lines expanded).
#
# This is aacpack's (h264p's) generator with three forms of the IDX library
# that mp3c's model is written in and h264p's IDX has none of, ported from
# mp3c's old generator (IDX-FORMAT.md sec.16): `Rate` (a counter's rate
# schedule, a row of TTable), `ADD` (a factor with no mapping behind it) and
# `b&` (the node of a binary tree, as the library's masking_b).
#
# UseNew 1 (the default): Table() members are pointers, mapped in %M%_Init()
# and unmapped in %M%_Quit(); 0: fixed-size arrays, members of the struct,
# which needs constant sizes, i.e. the Const build.  IDX_USENEW=0 in the
# environment does the same as the argument.
$UseNew = 1;
$UseNew = $ENV{IDX_USENEW} if defined $ENV{IDX_USENEW};
$UseNew = $ARGV[1]         if defined $ARGV[1];
# $NoConst=1 (IDX_NOCONST=1) folds every line, whatever the .idx says: its
# Const lines, a line's "-", the lines before its first Const -- the shipping
# form, which mk.sh makes so.  The default, 0, is the form the .idx states.
$NoConst = 0;
$NoConst = $ENV{IDX_NOCONST} if defined $ENV{IDX_NOCONST};

$gdebug = 1; # global debug
$gconst = 0;
$prefix = "M_";

$file = $ARGV[0];
($file1=$file) =~ s/\.idx$/.inc/;
($file1=$file) =~ s/[^\/]*$/family.inc/ unless -e $file1;
($file2=$file) =~ s/\.idx$/_p.inc/;
($fileH=$file) =~ s/\.idx$/_h.inc/;

open I1, "<$file" or die "idx2inc.pl: cannot read $file\n";

$index = "";
$xdesc = "";

# The variables the indices read, in the order they first appear, for
# ${prefix}_USES: all that a model needs to compute (aacpack's computes
# every one of its families' variables, and reads no USES).  In the folded form
# (Const 1) a line counts when its pattern has a 1 (a line of zeros adds
# nothing to its index); in the tuning form also when it is live, since
# opt.pl may set any of its bits.  Of an expression, every name not
# called counts.
@uses = (); %used = ();
# a pattern as it stands, which must be of 0s and 1s: the shipping build
# counts its 1s and the tuning build reads its bits (mapping.inc's S[i]&1),
# which would disagree on anything else
sub binary {
  my $m = shift;
  die "idx2inc.pl: $file line $.: pattern \"$m\" is not of 0s and 1s\n" unless $m =~ /^[01]*$/;
  return $m;
}

# a base as it goes into an expression: a number as it is, else in parentheses
sub paren { my $b = shift; return $b =~ /^\s*-?\d+\s*$/ ? $b : "($b)"; }

sub use_var {
  my ($expr, $map, $const, $debug) = @_;
  return unless $map =~ /1/ || ($const == 0 && $debug);
  while( $expr =~ /\b([A-Za-z_]\w*)\b(\s*\()?/g ) {
    next if defined $2;
    push @uses, $1 unless $used{$1}++;
  }
}

while( <I1> ) {
  s/\r//g;
  s/\#(.*)$//g;
  # Strip what the comment left behind.  Every pattern-bearing regex below
  # captures to end of line, so a single trailing space survives into the
  # pattern string as one extra zero bit -- silently doubling the value.
  s/\s+$//;

  $const = s/^\-//; $const ^= $gconst;
  $const = 1 if $NoConst;
  $debug = s/^\!//; $debug ^= $gdebug;

  if( /^\s*Prefix\s+([^\s]*)/ ) {
    $prefix = $1;
    print "Prefix=<$prefix>\n";
  } elsif( /^\s*Debug\s+([^\s]*)/ ) {
    $gdebug = $1;
  } elsif( /^\s*Const\s+([^\s]*)/ ) {
    $gconst = $NoConst ? 1 : $1;
  } elsif( /^\s*Index\s+([^\s]*)/ ) {
    $index = $1;
    print "Index=<$index>\n";
    $code{$index} = "\n$index = 0;";
    # A Volume is a product of factor sizes, and the table sizes multiply it
    # again by the node count and, for an APM, by the curve width.  In int
    # that product wraps, and the wrap that lands positive is the dangerous
    # one: the bound in tbl_n is then satisfied by the wrong number, a table
    # far too small is allocated, and the coder indexes off the end of it with
    # nothing said.  So: long long, and folded through a saturating multiply,
    # because with two dozen factors opt.pl can widen its way past 64 bits
    # too.  Saturating means whatever checks the size downstream sees a number
    # at least as large as what was asked for, and refuses it.
    $volume{$index} = "1";
    $vfactors{$index} = [];
    $count{$index} = 1;
    $maps{$index} = "";
  } elsif( /^\s*Number\s+(.*?),\s*(.*?)\s*,\s*(.*?)!(.*)/ ) {
    print "Number: <$1><$2><$3><$4>\n";
    binary($4);
    if( $const==0 ) {
      $xdesc .= ($debug>0?'p':'m')."desc( ${prefix}_${1}_, $3, \"$4\" );\n";
      $xdesc .= "static const int ${prefix}_$1 = ${prefix}_${1}_.value * ($2);\n";
    } else {
      ($var,$coef,$base,$map) = ($1,$2,$3,$4);
      $q=0; $q+=$q+($_&1) for( unpack("c*",$map) );
      $xdesc .= "static const int ${prefix}_$var = ($q+" . paren($base) . ") * ($coef);\n";
    }
  } elsif( /^\s*Rate\s+(.*?),\s*(.*?)!(.*?)\s*$/ ) {
    # mp3c's: a counter's rate schedule, the row of TTable (freq_tbl.inc, 32
    # rows) a pattern picks, as a Number does a value.  (Clamped in the
    # tuning form, where opt.pl can make the value anything.)
    print "Rate: <$1><$2><$3>\n";
    ($var,$base,$map) = ($1,$2,binary($3));
    if( $const==0 ) {
      $xdesc .= ($debug>0?'p':'m')."desc( ${prefix}_${var}_, $base, \"$map\" );\n";
      $xdesc .= "static const word* ${prefix}_$var = TTable[__min(31,__max(0,${prefix}_${var}_.value))];\n";
    } else {
      $q=0; $q+=$q+($_&1) for( unpack("c*",$map) );
      $xdesc .= "static const word* ${prefix}_$var = TTable[$q+" . paren($base) . "];\n";
    }
  } elsif( /^\s*ADD\s+(.*?):\s*(.*?)\s*$/ ) {
    # mp3c's: a factor with no mapping behind it -- the index times coef plus
    # var, var known to be in [0, coef): room for a sub-index the template
    # adds later (ADD %M%J..._Volume: 0), another index's value, or, with
    # coefs of 0 and 1, a sum of constants (av_total)
    ($coef,$var) = ($1,$2);
    $coef =~ s/\%M\%/${prefix}_/g;
    $var =~ s/\%M\%/${prefix}_/g;
    print "ADD $coef:$var\n";
    push @{$vfactors{$index}}, "($coef)";
    $count{$index} = ($coef =~ /^\d+$/ && $count{$index} =~ /^\d+$/) ? $count{$index}*$coef : "?";
    $code{$index} .= "\n$index = $index*($coef) + ($var);";
  } elsif( /^\s*(.*?):\s*(.*?),\s*b\&(.*?)\s*$/ ) {
    # mp3c's (the library's masking_b): var is the node of a binary tree, 1
    # for the first decision, then 2-3, 4-7, ... (a 1 and the decisions so
    # far); a 1 in map ignores the decision at that depth (left: the first),
    # so the nodes that differ only there share a row.  Size: the rows left.
    print "$1:$2,$3!\n";
    ($tag,$var,$map) = ($1,$2,binary($3));
    $var =~ s/\%M\%/${prefix}_/g;
    use_var($var, '1', $const, $debug);
    $ccount = ($map=~s/1/1/g);
    $lmap = length($map);
    die "idx2inc.pl: $file:$.: a b& mask of $lmap bits (20 at most)\n" if $lmap > 20;
    if( $const==0 ) {
      push @{$vfactors{$index}}, "${prefix}_${tag}.Size";
      $code{$index} .= "\n${prefix}_${tag}.inc( $index, $var );";
      $maps{$index} .= ($debug>0?'p':'m')."mask2( ${prefix}_${tag}, \"$map\" );\n"
    } else {
      $sz = 1 << $lmap;
      if( $ccount>0 ) {
        # the rows as masking_b numbers them (a node's row is its first
        # ancestor's, past the ignored decisions, or a new one)
        $mask = unpack( "N", pack( "B32", substr(('0' x 32).$map,-32) ) );
        undef @pmap;
        for( $i=0; $i<$sz; $i++ ) { $pmap[$i]=0; }
        for( $b=$lmap-1,$k=0; $b>=0; $b-- ) {
          for( $i=(1 << ($lmap-1-$b)); $i<(2 << ($lmap-1-$b)); $i++ ) {
            $j = ($i & (($mask^0xFFFFFFFF) >> ($b+1)));
            $pmap[$i] = ( $j<$i ) ? $pmap[$j] : $k++;
          }
        }
        push @{$vfactors{$index}}, "$k";
        $count{$index} *= $k if $count{$index} =~ /^\d+$/;
        $code{$index} .= "\n$index = ($index*$k) + ${prefix}_${tag}[$var]; // $map";
        $q = sprintf( "static const %s ${prefix}_${tag}[$sz]={ ", (($k<256)?"byte":"uint") );
        $c = "";
        for( $i=0; $i<$sz; $i++ ) { $c .= sprintf "$q%i",$pmap[$i]; $q=','; }
        $c .= " };\n";
        $maps{$index} .= $c;
      } else {
        # (no decision ignored: the node itself, 1 to 2^length-1 -- the tuning
        # form numbers the same nodes from 0, which partitions them alike)
        push @{$vfactors{$index}}, "$sz";
        $count{$index} *= $sz if $count{$index} =~ /^\d+$/;
        $code{$index} .= "\n$index = ($index*$sz) + ($var); // $map";
      }
    }
  } elsif( /^\s*(.*?):\s*(.*?),\s*\&(.*?)\s*$/ ) {
    print "$1:$2,$3!\n";
    ($tag,$var,$map) = ($1,$2,binary($3));
    use_var($var, $map, $const, $debug);
    $ccount = ($map=~s/1/1/g);
    # (the tuning form maps a variable through a table of 2^length entries, mapping.inc)
    die "idx2inc.pl: $file:$.: a mask of ".length($map)." bits, of which the tuning form would make a table of 2^".length($map)." (20 bits at most)\n"
      if $const==0 && length($map) > 20;
    if( $const==0 ) {
      push @{$vfactors{$index}}, "${prefix}_${tag}.Size";
      $code{$index} .= "\n${prefix}_${tag}.inc( $index, $var );";
      $maps{$index} .= ($debug>0?'p':'m')."mask( ${prefix}_${tag}, \"$map\" );\n"
    } else {
      if( $ccount>0 ) {
        push @{$vfactors{$index}}, "(1<<$ccount)";
        $count{$index} = $count{$index} << $ccount;
        $lmap = length($map);
        # Decompose the mask into runs of ones and emit shift/mask/or code.
        # The old general path built a lookup table of 2^length($map) entries,
        # so a wide declaration -- even one that is mostly zeros, e.g. a history
        # mask widened to give the optimizer room -- put megabytes of const data
        # in the binary (byte table under 8 selected bits, uint at or above).
        # Runs cost a couple of ALU ops instead and are faster than a table that
        # misses cache. Each term masks to its own width, so bits of $var above
        # the pattern cannot leak in -- same bound as masking::inc.
        @terms = (); $rem = $ccount; $p = 0;
        while( $p < $lmap ) {
          if( substr($map,$p,1) eq '1' ) {
            $a = $p;
            $p++ while( $p < $lmap && substr($map,$p,1) eq '1' );
            $b = $p-1;                  # run spans map[$a..$b]
            $w = $b-$a+1;               # width in bits
            $lo = $lmap-1-$b;           # variable bit holding the run's LSB
            $rem -= $w;
            $m = (1 << $w) - 1;
            $e = $lo ? "((($var)>>$lo)&$m)" : "(($var)&$m)";
            $e = "($e<<$rem)" if( $rem );
            push @terms, $e;
          } else { $p++; }
        }
        $code{$index} .= "\n$index = ($index<<$ccount) + (" . join(' + ', @terms) . "); // $map";
      }
    }
  } elsif( /^\s*(.*?):\s*(.*?),\s*(.*?)!(.*?)\s*$/ ) {
    print "$1:$2,$3!\n";
    ($tag,$var,$base,$map) = ($1,$2,$3,binary($4));
    use_var($var, $map, $const, $debug);
    $var=~s/\%M\%/${prefix}_/g;
    $ccount = ($map=~s/1/1/g) + 1;
    $count{$index} *= $ccount;
    if( $const==0 ) {
      push @{$vfactors{$index}}, "${prefix}_${tag}.Size";
      $code{$index} .= "\n${prefix}_${tag}.inc( $index, $var );";
      $maps{$index} .= ($debug>0?'p':'m')."desc( ${prefix}_${tag}, $base, \"$map\" );\n"
    } else {
      push @{$vfactors{$index}}, "$ccount";
      $lmap = length($map);
      # A table for four buckets or more, a compare chain below that.  The
      # chain is a compare and a set per threshold plus the adds, the table a
      # clamp and a load whatever the count -- at four buckets they cost the
      # same and past it the chain only grows.  It was a table above eight,
      # and those chains were a fifth of the coder's instructions (as oggcomp,
      # whose coder this is, measured them).  The tuning build maps through
      # pmap[] regardless, so both builds bucket the same way.
      $bp = paren($base);
      if( $ccount > 3 ) {
        $code{$index} .= "\n$index = $index*$ccount + ${prefix}_${tag}[__min($lmap,__max(0,($var)-($bp-1)))];";
        $j = 0; 
        $q = sprintf( "static const %s ${prefix}_${tag}[$lmap+1]={ ", (($ccount<256)?"byte":"word") );
        $c = "";
        for( unpack("c*",$map.'0') ) {
          $c .= sprintf "$q%i",$j; $q=',';
          $j += ($_&1);
        }
        $c .= " };\n";
        $maps{$index} .= $c;
      } else {
        if( $ccount > 1 ) {
          $j=0; $c=""; $q="";
          for( unpack("c*",$map) ) {
            if( ($_&1)==1 ) {
              $c .= sprintf( "$q(($var)>%i+($bp-1))", $j ); $q="+";
            }
            $j++;
          }
          $code{$index} .= "\n$index = $index*$ccount + ($c);";
        }
      }
    }
  } elsif( /\S/ ) {
    die "idx2inc.pl: $file:$.: not an IDX line: $_\n";
  }
}

close I1;

open I2, "<$file1" or die "idx2inc.pl: no template for $file ($file1)\n";
($fam = $prefix) =~ s/^[^_]*_//;

$hdr = "\n";
for (sort keys %maps) {
  $hdr .= "$maps{$_}\n";
} $hdr .= "\n";

$hdr .= "$xdesc\n";

$body = "";

$t_sta = $t_ptr = $t_con = $t_quit = "";
$t_siz = "";   # per-table byte counts, summed into ${prefix}_Size

while( <I2> ) {
  s/\%M\%/${prefix}_/g;
  s/\%F\%/$fam/g;

  # Table( type, %M%name, count ); -- a table of the struct
  if( /^\s*Table\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)\;/ ) {
    ($t,$v,$sz) = ($1,$2,$3);
    $t =~ s/\|/,/g; # for template support like SSE<6|6|2>
    $t_sta .= "$t ${v}[${sz}];\n";
    $t_ptr .= "$t* $v;\n";
    # tbl_n() bounds-checks the count so the compiler can prove the byte size
    # cannot overflow -- without it every one of these lines draws a
    # -Walloc-size-larger-than from GCC 13+ about its own overflow guard.
    # Mapped, not new[]: the host's tc_map hands back zeroed pages on demand,
    # so a table costs address space at once and memory only where the codec
    # reaches it.  That is what lets an index be wide -- new[] plus a fill
    # touches every page of a table the codec will visit a thousandth of, and
    # a table too large to touch is a table too large to have.  It also makes
    # this build's memory behaviour the Const build's, whose tables are
    # members of one mapped object, so `./mk.sh check` can compare the two on
    # a set neither could allocate outright.
    # (8 bytes more than its entries: the counters' vector path reads and
    # writes back 8 bytes at a 6-byte counter, 2 past a table's last one,
    # which in the Const build are the next member's)
    $t_con .= "  $v = (${t} *) tc_map( (unsigned long long)sizeof(${t}) * tbl_n(${sz}) + 8 );\n";
    $t_quit .= "  tc_unmap( $v, (unsigned long long)sizeof(${t}) * tbl_n(${sz}) + 8 );\n";
    # Running total of this module's table bytes.  Const mode can fold the sum
    # at compile time; Debug cannot, because the counts come from the mapping
    # objects opt.pl patches at load, so there it accumulates in _Init().
    $t_con .= "  ${prefix}_Size += (unsigned long long)sizeof(${t}) * (unsigned long long)tbl_n(${sz});\n";
    $t_siz .= "\n                              + (unsigned long long)sizeof(${t}) * (unsigned long long)(${sz})";
    next;
  }
  if( /^(\s*)MakeIndex\s+([^\s]+)/ ) {
    $ident = $1;
    ($idx=$2) =~ s/\s//;
    for( split(",",$idx) ) {
      $c = $code{$_};
      $c =~ s/\n/\n$ident/gs;
      $body .= "$c\n";
    }
  } else {
    $body .= $_;
  }
}

open O1, ">$file2"; 
print O1 $body; 
close O1;

open O1, ">$fileH";
print O1 "$hdr\n";
for (sort keys %volume) {
  my $v = $volume{$_};
  $v = "tc_vmul($v, $_)" for @{$vfactors{$_}};
  print O1 "static const long long ${prefix}_${_}_Volume = $v; // $count{$_}\n";
} print O1 "\n";
print O1 "#define ${prefix}_USES(X)" . join('', map { " X($_)" } @uses) . "\n\n";

chomp $t_con;
chomp $t_quit;
$t_sta =~ s/(^|\n)/$1  /g;
$t_ptr =~ s/(^|\n)/$1  /g;
$t_con =~ s/(^|\n)/$1  /g;

if( $UseNew==1 ) {
print O1 <<TEXT;
struct ${prefix}_T {
$t_ptr
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here.  (Set there, not by an
   * initializer: mp3c's models are members of a union, mp3c.cpp, whose
   * members must be trivial to construct.) */
  unsigned long long ${prefix}_Size;
  void ${prefix}_Init( void ) {
    ${prefix}_Size = 0;
$t_con
  }
  void ${prefix}_Quit( void ) {
$t_quit
  }
};
TEXT
} else {
  print O1 <<TEXT;
struct ${prefix}_T {
$t_sta
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long ${prefix}_Size = 0$t_siz;
  void ${prefix}_Init( void ) {}
  void ${prefix}_Quit( void ) {}
};
TEXT
}

close O1;
