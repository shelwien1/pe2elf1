
use Time::Piece;

open( O, ">fp_u.log" );

open( I,  "<fp.bin" )     || die;
open( I1, "<fp_ip.bin" )  || die;
open( I2, "<fp_ua.txt" )  || die;
open( I3, "<fp_url.txt" ) || die;
open( I4, "<fp_ref.txt" ) || die;
open( I5, "<fp_get.txt" ) || die;
open( I6, "<fp_htp.txt" ) || die;
open( I7, "<fp_tim.bin" ) || die;

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

#12.1.239.250 - - [04/Feb/2003:14:32:03 +0100] "GET /info/mig31.htm HTTP/1.1" 200 3642 "http://www.google.com/search?sourceid=navclient&ie=UTF-8&oe=UTF-8&q=mIg%2D31" "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)"

$i=0;
while(1) {
  $l = sysread(I1,$x,4); last if $l<4;
  @ip = unpack("CCCC",$x);
  $ip0 = unpack("N",$x);
  $ip1 = sprintf("%i.%i.%i.%i",$ip[0],$ip[1],$ip[2],$ip[3]);
#print "$ip1\n";
  $mip{$i++}=$ip1;
}

$i=0; while( <I2> ) { chomp; $mua{$i++}=$_; }

$i=0; while( <I3> ) { chomp; $murl{$i++}=$_; }

$i=0; while( <I4> ) { chomp; $mref{$i++}=$_; }

$i=0; while( <I5> ) { chomp; $mget{$i++}=$_; }

$i=0; while( <I6> ) { chomp; $mhtp{$i++}=$_; }

$i=0; while(1) { 
  $l = sysread(I7,$x,4); last if $l<4;
  $epoch = unpack("N",$x);
  $date = gmtime($epoch)->strftime("%d/%b/%Y:%H:%M:%S"); # adjust format to taste
  $mtim{$i++}=$date; 
}

=rem
    print O pack("n",$mtim{$tim});
    print O pack("n",$mip{$ip});
    print O chr($mget{$g}).chr($mhtp{$h});
    print O pack("n",$murl{$url});
    print O pack("n",$mref{$ref});
    print O pack("n",$mua{$ua});
    print O pack("N",0);
=cut

while(1) {
  $l = sysread(I,$x,16); last if $l<16;
  ($tim,$ip,$g,$h,$url,$ref,$ua,$pad) = unpack("nnCCnnnN",$x);

  ($tim,$ip,$g,$h,$url,$ref,$ua) = ($mtim{$tim},$mip{$ip},$mget{$g},$mhtp{$h},$murl{$url},$mref{$ref},$mua{$ua});

  if( $url=~/^([^ ]*) ([^ ]*) (.*)/ ) {
    ($u1,$u2,$u3) = ($1,$2,$3); #split(" ",$url);
  }

  $ref=~ s!^/!http://!;

#  print O "$ip - - [$tim +0100] \"$g/$url$h $ref $ua\n";
  print O "$ip - - [$tim +0100] \"$g/$u1$h $u2 $u3 \"$ref\" \"$ua\"\n";
}


