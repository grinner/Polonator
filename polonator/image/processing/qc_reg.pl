#!/usr/bin/perl
#
# usage:
# ./qc_reg cyclename
#  where:
#   cyclename is the cyclename (without extension) we want to look at;
#      software assumes it is looking for files cyclename.beadsums and
#      cyclename.register-log
#
# OUTPUT is: fcnum\tarraynum\timgnum\tnum_objs\tbeadsum\tdiff_x\tdiff_y\n
#
# Greg Porreca (Church Lab) 12-05-2007
#

@array = glob('*.info');
$num_files = @array;
$num_files+=0;
if($num_files ne 1){
    print STDERR "ERROR: found $num_files info files in current directory (must be 1)\n";
    exit;
}
open INF, $array[0];
binmode INF;

$beadsumfilename = $ARGV[0] . ".beadsums";
open BEADSUM, $beadsumfilename;

$reglogfilename = $ARGV[0] . ".register-log";
open REGLOG, $reglogfilename;

while(<REGLOG>){
    read(INF, $buffer, 8);
    @line = unpack('SSSS', $buffer);
    $curr_img = $line[2];
    if($curr_img == 0){
	$last_x = 0;
	$last_y = 0;
    }

    read(BEADSUM, $buffer, 8);
    @sumline = unpack('q', $buffer);

    chop $_;
    @regline = split(/\t/, $_);
    $curr_x = $regline[0];
    $curr_y = $regline[1];
    $x_diff = $curr_x - $last_x;
    $y_diff = $curr_y - $last_y;

    $last_x = $curr_x;
    $last_y = $curr_y;

    print "$line[0]\t$line[1]\t$line[2]\t$line[3]\t$sumline[0]\t$x_diff\t$y_diff\t$curr_x\t$curr_y\n";
}

close INF;    
