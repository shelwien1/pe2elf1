
open(I,"<log.txt")||die;
open(O,">log1.txt")||die;

while(<I>) {

  s/[\r\n]//g;

  $cmt=$_ if m!//!;
  $t_e=$1 if /([\d\.]+s): (.*) c /;
  $t_d=$1 if /([\d\.]+s): (.*) d /;

  if( /^\d+$/ ) {
    printf O "%7i %6.3f %6.3f %s\n", $_ , $t_e, $t_d, $cmt;
    ($cmt,$t_e,$t_d)=("","","");
  }

}
