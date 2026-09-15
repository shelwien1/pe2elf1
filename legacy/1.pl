             
$h{"BYTE"} =1;
$h{"WORD"} =2;
$h{"DWORD"}=4;
$h{"QWORD"}="!8!";
$h{"byte"} =1;
$h{"word"} =2;
$h{"dword"}=4;
$h{"qword"}="!8!";

$form = <<TEXT;
pushf\r
push rcx\r
lea rcx,%s\r
call "track%i"\r
pop rcx\r
popf\r
TEXT

$form1 = <<TEXT;
.def track%i; .scl 2; .type 32; .endef\r
TEXT


open(I,"<coder.s")||die;
open(O,">coder1.s")||die;

@a = <I>;

for( $i=0; $i<=$#a; $i++ ) {
  $_ = $a[$i];
  if( /^(?:\s*)mov(?:\s+)(BYTE|WORD|.WORD)(?:\s+)PTR(?:\s+)([^\],]+\])(?:\s*),(?:\s*)(.+)/i ) {
    print "mov $1 ptr $2, $3\n"; 
    printf O $form, $2, $h{$1};
    $def{"track".$h{$1}} = sprintf($form1,$h{$1});
  }
  print O "$_";
}

for( sort keys %def ) {
  print O $def{$_};
}

=rem
	mov	WORD PTR 1028[r8], ax
=cut
