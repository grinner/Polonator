#!/usr/bin/perl

$num_imgs=2180;

for($j=0; $j<$num_imgs; $j++){
    $correct{$j}=0;
    $incorrect{$j}=0;
}

$_=<STDIN>;
chop $_;
@line = split(/\t/, $_);
$curr_fc = $line[0];
$curr_array = $line[1];
$seq = $line[4];
$readlength = length($seq);

print STDERR "$curr_fc\t$curr_array\t$readlength\n";

for($i=0; $i<$readlength; $i++){
    $a = $a . "A";
    $c = $c . "C";
    $g = $g . "G";
    $t = $t . "T";
}

if($seq=~/.\../){;}
else{
    if(($seq eq $a) || ($seq eq $c) || ($seq eq $g) || ($seq eq $t)){
	$correct{$line[2]}++;
    }
    else{
	$incorrect{$line[2]}++;
    }
}

while(<>){
    chop $_;
    @line = split(/\t/, $_);
    $seq = $line[4];
    if($seq=~/.\../){;}
    else{
	if(($seq eq $a) || ($seq eq $c) || ($seq eq $g) || ($seq eq $t)){
	    $correct{$line[2]}++;
	}
	else{
	    $incorrect{$line[2]}++;
	}
    }
}


for($j=0; $j<$num_imgs; $j++){
    $sum = $correct{$j} + $incorrect{$j};
    if($sum>0){
	$error_rate = $incorrect{$j} / $sum;
    }
    else{
	$error_rate = 0;
    }
    print "$curr_fc\t$curr_array\t$j\t$correct{$j}\t$incorrect{$j}\t$sum\t$error_rate\n";
}
