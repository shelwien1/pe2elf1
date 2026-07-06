
use Time::Piece;

open( I, "<fp.log" ) || die;

open( O1, ">fp_ip.bin" );
open( O2, ">fp_ua.txt" );
open( O3, ">fp_url.txt" );
open( O4, ">fp_ref.txt" );
open( O5, ">fp_get.txt" );
open( O6, ">fp_htp.txt" );
open( O7, ">fp_tim.bin" );

# $0 = log line record (16 bytes)
# +$1-4 = ip                   4 (2?)      +
# +$5 - datetime               4 (2?)      +
# +$6 - GET etc                1           +
# +$7 - url                    2
# +$8 - HTTP                   1           +
# +$9 - status                 in url
# +$10 - filesize              in url
# +$11 - referer               2
# +$12 - user-agent            2

while( <I> ) {

  if( /^(\d+)\.(\d+)\.(\d+)\.(\d+) \- \- \[(.*) \+0100\] "(GET |HEAD |OPTIONS |PROPFIND |LINK |GET http:\/)\/(.*?)((?: HTTP\/\d\.\d")|") (\d+) ((?:\d+)|\-) "(.*)" "([^\"]*)"/ ) {
    #print O1 chr($1).chr($2).chr($3).chr($4);
    $tim = $5;
    $g=$6; $h=$8; $ua=$12;
    $ip = chr($1).chr($2).chr($3).chr($4);
    $url = $7.chr(32).$9.chr(32).$10;
    $ref = $11; $ref=~s/^http:\///;
    $mip{$ip} = 0;
    $mua{$ua} = 0;
    $murl{$url} = 0;
    $mref{$ref} = 0;
    $mget{$g} = 0;
    $mhtp{$h} = 0;
    $mtim{$tim} = 1;
#print "$12\n";
  } else {
    print "A line doesn't match!\n";
    print "$_";
  }

}

#12.1.239.250 - - [04/Feb/2003:14:32:03 +0100] "GET /info/mig31.htm HTTP/1.1" 200 3642 "http://www.google.com/search?sourceid=navclient&ie=UTF-8&oe=UTF-8&q=mIg%2D31" "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)"

$i=0;
for( sort keys %mip ) {
  $mip{$_}=$i++;
  print O1 $_;
}

$i=0;
for( sort keys %mua ) {
  $mua{$_}=$i++;
  print O2 $_.chr(10);
}

$i=0;
for( sort keys %murl ) {
  $murl{$_}=$i++;
  print O3 $_.chr(10);
}

$i=0;
for( sort keys %mref ) {
  $mref{$_}=$i++;
  print O4 $_.chr(10);
}

$i=0;
for( sort keys %mget ) {
  $mget{$_}=$i++;
  print O5 $_.chr(10);
}

$i=0;
for( sort keys %mhtp ) {
  $mhtp{$_}=$i++;
  print O6 $_.chr(10);
}

$i=0;
for( sort keys %mtim ) {
  $x = $_;
  $mtim{$x}=$i++;
  $t = Time::Piece->strptime( $x, "%d/%b/%Y:%H:%M:%S\n" );
  print O7 pack("N",$t->epoch);
#  $mtim{$_}=$i++;
#  print O7 $_.chr(10);
}


close I;

open( I, "<fp.log" ) || die;
open( O, ">fp.bin" );

while( <I> ) {

  if( /^(\d+)\.(\d+)\.(\d+)\.(\d+) \- \- \[(.*) \+0100\] "(GET |HEAD |OPTIONS |PROPFIND |LINK |GET http:\/)\/(.*?)((?: HTTP\/\d\.\d")|") (\d+) ((?:\d+)|\-) "(.*)" "([^\"]*)"/ ) {
    #print O1 chr($1).chr($2).chr($3).chr($4);
    $tim = $5;
    $g=$6; $h=$8; $ua=$12;
#    $ipt = "$1.$2.$3.$4";
    $ip = chr($1).chr($2).chr($3).chr($4);
    $url = $7.chr(32).$9.chr(32).$10;
    $ref = $11; $ref=~s/^http:\///;
    #$mip{$ip} = 0;
    #$mua{$12} = 0;
    #$murl{$url} = 0;
    #$mref{$ref} = 0;
    #$mget{$6} = 0;
    #$mhtp{$8} = 0;
    #$mtim{$5} = 0;

    print O pack("n",$mtim{$tim});
    print O pack("n",$mip{$ip});
    print O chr($mget{$g}).chr($mhtp{$h});
    print O pack("n",$murl{$url});
    print O pack("n",$mref{$ref});
    print O pack("n",$mua{$ua});
    print O pack("N",0);

  } else {
    print "A line doesn't match!\n";
    print "$_";
  }

}
