#!/usr/bin/perl

# the expected[] array defines the expected sequences in the readfile
$expected[0] = "AAAAAA";
$expected[1] = "CCCCCC";
$expected[2] = "GGGGGG";
$expected[3] = "TTTTTT";
$expected_length = length $expected[0];

$deltafilename = "tetrahedra/" . $ARGV[0] . ".delta";
open(DELTAFILE, "<$deltafilename") or die("ERROR opening delta file $deltafilename\n");

$curr_array = -1;

while(<DELTAFILE>){
    chop $_;
    @line = split(/\t/, $_);

    # Open new read file if we're on a new array (each array has its own read file)
    if($line[1] ne $curr_array){
	$curr_array = $line[1];
	$readfilename = "output_data/" . $ARGV[0] . "_$line[0]_" . sprintf("%02d", $curr_array) . ".basecalls";
	open(READFILE, "<$readfilename") or die("ERROR opening read file $readfilename\n");
    }

    # Now load in the delta values for the current array position
    $curr_img = $line[2];
    $num_deltas = (@line) - 3;
    for($i=0; $i<$num_deltas; $i+=4){
	$mean_deltas[$i/4] = $line[$i+3];
	$mean_deltas[$i/4] += $line[$i+4];
	$mean_deltas[$i/4] += $line[$i+5];
	$mean_deltas[$i/4] += $line[$i+6];
	$mean_deltas[$i/4] = $mean_deltas[$i/4] / 4;
	$mean_deltas[$i/4] = int($mean_deltas[$i/4] * 1000);
    }
    $delta_mean = 0;
    for($i=0; $i<$expected_length; $i++){
	$delta_mean = $delta_mean+$mean_deltas[$i];
    }
    $delta_mean = $delta_mean / $expected_length;

    @correct = ();
    @incorrect = ();
    @total = ();

    # Now, iterate over the current image in the readfile
    while($curr_read = <READFILE>){
	chop $curr_read;
	@readline = split(/\t/, $curr_read);
	# have we reached the next image?
	if($readline[2] ne $curr_img){
	    seek(READFILE, (-length($curr_read))-1, 1); #move file pointer back this line (plus newline)
	    last;
	}

	# if not, is this a 'bad' bead?
	elsif($readline[4]=~/\.+/){;}

	# if not, compute error rates
	else{

	    # first, determine what the read 'should' be; compare it to each of the
	    # expected sequences, and pick the one it's closest to; if it is equally close
	    # to 2 sequences, mark it as '='
	    $read = $readline[4];
	    for($i=0; $i<$expected_length; $i++){
		$score[$i]=0;
		for($j=0; $j<$num_deltas/4; $j++){
		    if(substr($read, $j, 1) eq substr($expected[$i], $j, 1)){
			$score[$i]+=1;
		    }		       
		}
	    }

	    $best_match = ".";
	    $best_score = 0;
	    for($i=0; $i<$expected_length; $i++){
		if($score[$i] > $best_score){
		    $best_score = $score[$i];
		    $best_match = $expected[$i];
		}
		elsif($score[$i] == $best_score){
		    $best_match = "=";
		}
	    }

	    # now, output each basecall, what we think it should be, its quality and delta score, and whether
	    # it's correct (+), incorrect (-), or incorrect because we don't know what the call should be (/)
	    for($i=0; $i<$num_deltas/4; $i++){
#	    for($i=6; $i<21; $i++){
		$r = substr($read, $i, 1);
		if($best_match eq "="){
		    $b = "."; #$b is the basecall; '.' means the correct basecall is ambiguous
		    $c = "/"; #$c is a code that describes whether the basecall is correct, incorrect, or ambiguous
		}
		else{
		    $b = substr($best_match, $i, 1);
		    if($r eq $b){ $c = "+";}
		    else{ $c = "-";}
		}
#		print "$readline[0]\t$readline[1]\t$readline[2]\t$readline[3]\t$best_score\t$i\t$r\t$b\t$c\t$readline[$i+5]\t$mean_deltas[$i]\n";
		if($c eq "+"){
		    $correct[$i]++;
#		    $total[$i]++;
		}
#		elsif($c eq "-"){
		else{ #incorrect and ambiguous basecalls are considered errors
		    $incorrect[$i]++;
#		    $total[$i]++;
		}
		$total[$i]++;
	    }
	}
    }#end while
#    for($i=6; $i<21; $i++){
    for($i=0; $i<$num_deltas/4; $i++){
	if($total[$i] > 0){
	    $accuracy[$i] = $correct[$i] / $total[$i];
	}
	else{
	    $accuracy[$i] = 0;
	    $total[$i] = 0;
	}
	if($total[$i] eq 1){
	    $accuracy[$i] = 0;
	    $total[$i]=0;
	    $correct[$i]=0;
	}
	if($correct[$i] eq ''){$correct[$i]=0;}
	if($incorrect[$i] eq ''){$incorrect[$i]=0;}
	$accuracy_str = sprintf("%0.05f", $accuracy[$i]);
	$delta_str = sprintf("%0.f", $mean_deltas[$i]);
	print "$line[0]\t$line[1]\t$line[2]\t$i\t$accuracy_str\t$delta_str\t$correct[$i]\t$incorrect[$i]\t$total[$i]\n";
	$|++;
    }
}


