
# $UseNew=1 -> Table() members are pointers, allocated in %M%_Init()
# $UseNew=0 -> Table() members are fixed-size arrays (needs const-expression
#              sizes, i.e. the Const build).  Override per invocation with
#              IDX_USENEW=0 in the environment or a second argument:
#                perl idx2inc.pl foo.idx 0
$UseNew = 0;
$UseNew = $ENV{IDX_USENEW} if defined $ENV{IDX_USENEW};
$UseNew = $ARGV[1]         if defined $ARGV[1];
$NoConst = 1;
$SizeOut= 0; # =1 with UseNew=1 only
$Verify = 0; # =1 with UseNew=0 only

$gdebug = 1; # global debug
$gconst = 0;
$prefix = "M_";

$file = $ARGV[0];
($file1=$file) =~ s/\.idx/.inc/;
($file2=$file) =~ s/\.idx/_p.inc/;
($fileT=$file) =~ s/\.idx/_t.inc/;
($fileH=$file) =~ s/\.idx/_h.inc/;

open I1, "<$file";

$index = "";
$xdesc = "";

while( <I1> ) {
  s/\r//g;
  s/\#(.*)$//g;
  
  $const = s/^\-//; $const ^= $gconst;
  $debug = s/^\!//; $debug ^= $gdebug;

  if( /^\s*Prefix\s+([^\s]*)/ ) {
    $prefix = $1;
    print "Prefix=<$prefix>\n";
  } elsif( /^\s*Debug\s+([^\s]*)/ ) {
    $gdebug = $1;
#    print STDERR "Debug=<$gdebug>\n";
  } elsif( /^\s*Const\s+([^\s]*)/ ) {
    $gconst = $NoConst ? 1 : $1;
#    print STDERR "Const=<$gconst>\n";
  } elsif( /^\s*ADD\s+(.*?):\s*(.*)/ ) {
    ($coef,$var) = ($1,$2);
    $coef=~s/\%M\%/${prefix}_/g;
    $var=~s/\%M\%/${prefix}_/g;
    print "ADD $coef:$var\n";
    $volume{$index} .= "* $coef";
    $count{$index} *= $coef;
    $porder{$index} .= "*(1+$coef";
    $code{$index} .= "\n$index = $index*($coef) + ($var);";
  } elsif( /^\s*Index\s+([^\s]*)/ ) {
    $index = $1;
    print "Index=<$index>\n";
    $code{$index} = "\n$index = 0;";
    $volume{$index} = "static const int ${prefix}_${index}_Volume = 1";
    $porder{$index} = "static const int ${prefix}_${index}_POrder = 1";
    $count{$index} = 1;
    $maps{$index} = "";
  } elsif( /^\s*Number\s+(.*?),\s*(.*?)\s*,\s*(.*?)!(.*)/ ) {
    print "Number: <$1><$2><$3><$4>\n";
    if( $const==0 ) {
      $xdesc .= ($debug>0?'p':'m')."desc( ${prefix}_${1}_, $3, \"$4\" );\n";
      $xdesc .= "static const int ${prefix}_$1 = ${prefix}_${1}_.value * ($2);\n";
    } else {
      ($var,$coef,$base,$map) = ($1,$2,$3,$4);
      $q=0; $q+=$q+($_&1) for( unpack("c*",$map) );
      $xdesc .= "static const int ${prefix}_$var = ($q+$base) * ($coef);\n";
    }
  } elsif( /^\s*Rate\s+(.*?),\s*(.*?)!(.*?)\s*$/ ) {
    print "Rate: <$1><$2><$3>\n";
    if( $const==0 ) {
      $xdesc .= ($debug>0?'p':'m')."desc( ${prefix}_${1}_, $2, \"$3\" );\n";
      $xdesc .= "static const word* ${prefix}_$1 = TTable[${prefix}_${1}_.value];\n";
    } else {
      ($var,$base,$map) = ($1,$2,$3);
      $q=0; $q+=$q+($_&1) for( unpack("c*",$map) );
      $xdesc .= "static const word* ${prefix}_$var = TTable[$q+$base];\n";
    }
  } elsif( /^\s*Rate1\s+(.*?),\s*(.*?)!(.*?)\s*$/ ) {
    ($var,$base,$map) = ($1,$2,$3);
    print "Rate: <$1><$2><$3>\n";
    if( $const==0 ) {
      $xdesc .= ($debug>0?'p':'m')."desc( ${prefix}_${var}_, $base, \"$map\" );\n";
      $xdesc .= "static const int ${prefix}_$var = eSCALE/${prefix}_${var}_.value;\n";
    } else {
      $q=0; $q+=$q+($_&1) for( unpack("c*",$map) );
      $xdesc .= "static const int ${prefix}_$var = (eSCALE/($q+$base));\n";
    }
  } elsif( /^\s*(.*?):\s*(.*?),\s*b\&(.*?)\s*$/ ) {
    print "$1:$2,$3!\n";
    ($tag,$var,$map) = ($1,$2,$3);
    $ccount = ($map=~s/1/1/g);
    if( $const==0 ) {
      $volume{$index} .= "* ${prefix}_${tag}.Size";
      $code{$index} .= "\n${prefix}_${tag}.inc( $index, $var );";
      $maps{$index} .= ($debug>0?'p':'m')."mask2( ${prefix}_${tag}, \"$map\" );\n"
    } else {
      $lmap = length($map);
      $h = 1 << $lmap;
      $sz = 1 << $lmap;
      if( $ccount>0 ) {
        $mask = unpack( "N", pack( "B32", substr(('0' x 32).$map,-32) ) );
        undef @pmap;
        for( $i=0; $i<$h; $i++ ) { $pmap[$i]=0; }
        for( $b=$lmap-1,$k=0; $b>=0; $b-- ) {
          for( $i=(1 << ($lmap-1-$b)); $i<(2 << ($lmap-1-$b)); $i++ ) {
            $j = ($i & (($mask^0xFFFFFFFF) >> ($b+1)));
            $pmap[$i] = ( $j<$i ) ? $pmap[$j] : $k++;
          }
        }
        $volume{$index} .= "* $k";
        $count{$index} *= $k;
        $sz1 = $sz-1;
        $code{$index} .= "\n$index = ($index*$k) + ${prefix}_${tag}[($var)<1?1:(($var)>$sz1?$sz1:($var))]; // $map"; # clamped == masking_b::map

        $j = 0; 
        $q = sprintf( "static const %s ${prefix}_${tag}[$sz]={ ", (($k<256)?"byte":"uint") );
        $c = "";
        for( $i=0; $i<$sz; $i++ ) {
          $c .= sprintf "$q%i",$pmap[$i]; $q=',';
        }
        $c .= " };\n";
        $maps{$index} .= $c;
      } else {
        $volume{$index} .= "* $sz";
        $count{$index} *= $k;
        $sz1 = $sz-1;
        $code{$index} .= "\n$index = ($index*$sz) + (($var)<1?1:(($var)>$sz1?$sz1:($var))); // $map"; # clamped == masking_b::map
      }
    }
  } elsif( /^\s*(.*?):\s*(.*?),\s*\&(.*?)\s*$/ ) {
    print "$1:$2,$3!\n";
    ($tag,$var,$map) = ($1,$2,$3);
    $ccount = ($map=~s/1/1/g);
    if( $const==0 ) {
      $volume{$index} .= "* ${prefix}_${tag}.Size";
      $code{$index} .= "\n${prefix}_${tag}.inc( $index, $var );";
      $maps{$index} .= ($debug>0?'p':'m')."mask( ${prefix}_${tag}, \"$map\" );\n"
    } else {
      if( $ccount>0 ) {
        $volume{$index} .= "* (1<<$ccount)";
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
    ($tag,$var,$base,$map) = ($1,$2,$3,$4);
    $var =~ tr/|/,/; $var=~s/\%M\%/${prefix}_/g;
    $ccount = ($map=~s/1/1/g) + 1;
    $count{$index} *= $ccount;
    if( $const==0 ) {
      $volume{$index} .= "* ${prefix}_${tag}.Size";
      $porder{$index} .= "*(1+${prefix}_${tag}.Size";
      $code{$index} .= "\n${prefix}_${tag}.inc( $index, $var );";
      $maps{$index} .= ($debug>0?'p':'m')."desc( ${prefix}_${tag}, $base, \"$map\" );\n"
    } else {
      $volume{$index} .= "* $ccount";
      $porder{$index} .= "*(1+$ccount";
      $lmap = length($map);
      if( $ccount > 8 ) {
        $code{$index} .= "\n$index = $index*$ccount + ${prefix}_${tag}[__min($lmap,__max(0,$var-($base-1)))];";
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
              $c .= sprintf( "$q($var>%i+($base-1))", $j ); $q="+";
            }
            $j++;
          }
          $code{$index} .= "\n$index = $index*$ccount + ($c);";
        }
      }
    }
  }
  
}

# z_p:   p,  -7!000000000000000
# ADD    2:  (n&1)
# M_FF_z_col.Size
# Number wlim,  16,0!11010111
# Rate   w_wr,     0!00000
# mdesc( M_Bx_mw,  0, "00000000" );
#const int Bx_mw  = M_Bx_mw.value *16;
#mdesc( M_Ba_wr, 0, "00000" );
#const word* Ba_wr = TTable[M_Ba_wr.value];

close I1;

open I2, "<$file1";

$hdr = "\n";
for (sort keys %maps) {
  $hdr .= "$maps{$_}\n";
} $hdr .= "\n";

$hdr .= "$xdesc\n";

$body = "";

$t_dat = $t_sta = $t_ptr = $t_con = $t_des = "";
$t_siz = "";   # per-table byte counts, summed into ${prefix}_Size

$f_init=0;
$init_code = "";

$f_data=0;

while( <I2> ) {
  s/\%M\%/${prefix}_/g;

  if( /^(\s*)def_Data\s*$/ ) { $f_data=1; next; } 
  elsif( /^(\s*)end_Data\s*$/ ) { $f_data=0; next; } 
  elsif( $f_data==1 ) {
    $t_dat .= $_; next;
  }

  if( /^(\s*)def_Init\s*$/ ) { $f_init=1; next; } 
  elsif( /^(\s*)end_Init\s*$/ ) { $f_init=0; next; } 
  elsif( $f_init==1 ) {
    $init_code .= $_; next;
  }

  if( /^(\s*)MakeTables\s*$/ ) { 
    $ident = $1;
    $hdr =~ s/\n/\n$ident/gs;
    $body .= "$hdr\n"; $hdr=""; next;
  }
#Table( Node2i,     M_Ak_Bt, M_Ak_t_Volume );
  if( /^\s*Table\(\s*(.*?)\s*,\s*(.*?)\s*,\s*(.*?)\s*\)\;/ ) {
    ($t,$v,$sz) = ($1,$2,$3);
    $t =~ s/\|/,/g; # for template support like SSE<6|6|2>
    $t_sta .= "$t ${v}[${sz}];\n";
    $t_ptr .= "$t* $v;\n";
    # tbl_n() bounds-checks the count so the compiler can prove the byte size
    # cannot overflow -- without it every one of these lines draws a
    # -Walloc-size-larger-than from GCC 13+ about its own overflow guard.
    # No () on the new[]: every table is filled by Codec::init() before it is
    # read, and value-initializing here would just be a second pass over a few
    # hundred MiB.  It also keeps the Debug build's semantics identical to the
    # Const build, where these are array members of a default-initialized
    # object and are equally indeterminate until init() fills them.
    $t_con .= "  $v = anew<${t}>( tbl_n(${sz}), \"${v}\" );\n";
    # Running total of this module's table bytes.  Const mode can fold the sum
    # at compile time; Debug cannot, because the counts come from the mapping
    # objects opt.pl patches at load, so there it accumulates in _Init().
    $t_con .= "  ${prefix}_Size += (unsigned long long)sizeof(${t}) * (unsigned long long)tbl_n(${sz});\n";
    $t_siz .= "\n                              + (unsigned long long)sizeof(${t}) * (unsigned long long)(${sz})";

$t_sta1 .= <<TEXT;
  $t ${v}__[${sz}];
  __declspec(property( get=get_${v} )) ${t} ${v}[];
  ${t}& get_${v}( int i ) { 
    if( (i>=0) && (i<${sz}) ) return ${v}__[i]; else {
      printf( "!!! Wrong access: ${v}[%i] access !!!\\n", i );
      return ${v}__[0];
    }
  }
TEXT

#printf( "sizeof("#Type" "#Name"[%i]) = %i\n", Size, sizeof( Type[Size] )
    if( $SizeOut==1 ) {
      $t_con .= "printf( \"sizeof($t ${v}[%i]) = %i\\n\", ${sz}, sizeof( ${t}[${sz}] ) );\n";
    }

    $t_des .= "  delete[] $v;\n";
    next;
  }
  if( /^(\s*)MakeIndex\s+([^\s]+)/ ) {
    $ident = $1;
    ($idx=$2) =~ s/\s//;
    for( split(",",$idx) ) {
      $c = $code{$_};
      $c =~ s/\n/\n$ident/gs;
#      $c =~ s/^\n//s;
      $body .= "$c\n";
    }
  } else {
    $body .= $_;
  }
}

#for (sort keys %code) {
#  print O1 "$code{$_}\n"
#}

open O1, ">$file2"; 
print O1 $body; 
close O1;

open O1, ">$fileH";
print O1 "$hdr\n";
for (sort keys %volume) {
  print O1 "$volume{$_}; // $count{$_}\n";
  $porder{$_} .= '*0' . (')' x ($porder{$_}=~s/\(/\(/g));
  $porder{$_} .= '; // ' . eval( substr( $porder{$_}, index( $porder{$_}, "=" )+1 ) );
#  print O1 "$porder{$_}\n";
} print O1 "\n";

chomp $t_con;
chomp $t_des;
chomp $init_code;
$init_code =~ s/(^|\n)/$1  /g;
$t_sta =~ s/(^|\n)/$1  /g;
$t_ptr =~ s/(^|\n)/$1  /g;
$t_con =~ s/(^|\n)/$1  /g;
$t_des =~ s/(^|\n)/$1  /g;

# Per-module copy of the flag, OUTSIDE the struct and never #undef'd.  USE_NEW
# itself is #undef/#define'd by every _h.inc, so whichever one is included LAST
# silently decides it for everything downstream -- a partially regenerated MOD/
# is not otherwise detectable, and the mixed build compiles and runs.  A named
# constant per module lets the consumer assert they all agree.
print O1 "static const int ${prefix}_USE_NEW = $UseNew;\n\n";

if( $UseNew==1 ) {
print O1 <<TEXT;
struct ${prefix}_T { \n#undef USE_NEW\n#define USE_NEW 1\n
$t_dat
$t_ptr
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long ${prefix}_Size = 0;
  void ${prefix}_Init( void ) {
    ${prefix}_Size = 0;
$t_con
$init_code
  }
  void ${prefix}_Quit( void ) {
$t_des
  }
};
TEXT
} else {
  $t_sta=$t_sta1 if $Verify;
#  print O1 "struct ${prefix}_T {\n$t_sta${prefix}_T( void ) {\n$init_code}\n };\n"
  print O1 <<TEXT;
struct ${prefix}_T { \n#undef USE_NEW\n#define USE_NEW 0\n
$t_dat
$t_sta
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long ${prefix}_Size = 0$t_siz;
  void ${prefix}_Init( void ) {
$init_code
  }
  void ${prefix}_Quit( void ) {
  }
};
TEXT
}

close O1;

#open O2, ">$fileT"; 
#print O2 "$t_ptr\n$t_con}\n$t_des}\n"; 
#close O2;
