#!/usr/bin/perl

open INF, $ARGV[0];
$argc = @ARGV;
binmode INF;
$count = 0;

while(read(INF, $buffer, 8)){
    @line = unpack('SSSS', $buffer);
    if($argc>1){
	if($line[1] eq $ARGV[1]){
	    $count += $line[3];
	    print "$line[0]\t$line[1]\t$line[2]\t$line[3]\n";
	}
    }
    else{
	$totals{$line[1]} += $line[3];
	$count += $line[3];
	print "$line[0]\t$line[1]\t$line[2]\t$line[3]\n";
    }
}
close INF;
print "TOTAL BEADS: ", &commify($count), "\n";
print "PER ARRAY:\n";
for($i=0; $i<$line[1]+1; $i++){ # the last line in the list tells us how many arrays
    print "$i\t", &commify($totals{$i}), "\n";
}

sub commify{
    local($_)=shift;
    1 while s/^(-?\d+)(\d{3})/$1,$2/;
    return $_;
}

