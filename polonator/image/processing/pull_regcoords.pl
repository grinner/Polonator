#!/usr/bin/perl

$num_perframe = 2000;
$curr_num = $num_perframe;

open REG, "$ARGV[0]";

for($i=0; $i<45252; i++){
#    if($curr_num == $num_perframe){
#	$curr_num = 0;
    $fn = sprintf("%07d.coords", $i);
    open FILE, ">$fn";
#    }
    for($j=0; $j<$num_perframe; $j++){
	read(REG, $buffer, 4);
	@line = unpack('SS', $buffer);
	print FILE "$line[0]\t$line[1]\n";
    }
    close FILE;
}
close REG;


