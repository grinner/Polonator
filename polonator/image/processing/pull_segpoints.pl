#!/usr/bin/perl
#
# Used to generate a binary 'image' of all objects at some array
# position.  Object locations are from the location file.  Execute
# as:
# ./pull_segpoints.pl a b c > image_filename
# where:
#   a = the flowcell number ([0..1])
#   b = the array number    ([0..17])
#   c = the image number    ([0..IMGS_PER_ARRAY])
#
# Written by Greg Porreca (Church Lab) 12-14-2007
#


$num_arrays = 8;
$num_imgs = 2180;
$reg_pixels = 2000;
$num_args = @ARGV;
if($num_args < 3){
    print "ERROR: must call as ./pull_segpoints fcnum arraynum imgnum $num_args\n";
    exit(0);
}

$fc = $ARGV[0];
$array = $ARGV[1];
$img = $ARGV[2];

# OUTPUT WILL BE BINARY IMAGE W/ 1s AT BEAD PIXELS
for($i=0; $i<1000; $i++){
    for($j=0; $j<1000; $j++){
	$image[$j][$i] = 0;
    }
}

print stderr "Seeking to position $fc $array $img...\n";

# DETERMINE INFO AND SEG FILENAMES
@array = glob('*.info');
$num_files = @array;
$num_files+=0;
if($num_files ne 1){
    print STDERR "ERROR: found $num_files info files in current directory (must be 1)\n";
    exit;
}
if($array[0]=~/(.+)\.info$/){
    $segfn = $1;
}
open INFO, $array[0];
open SEG, $segfn;
binmode INFO;
binmode SEG;


# LOOK FOR CORRECT RECORD IN INFO FILE
$index = 0;
while($found ne 1){
    read(INFO, $buffer, 8);
    @line = unpack('SSSS', $buffer);
    if(($line[0] eq $fc) && ($line[1] eq $array) && ($line[2] eq $img)){
	$index = $index + 20;
	$num_objs = $line[3];
	$found = 1;
    }
    else{
	$index = $index + 20 + ($line[3] * 4) + 2;
    }
}


# NOW GO THERE IN THE SEGFILE AND READ THE VALUES
seek(SEG, $index, SEEK_SET);
for($i=0; $i<$num_objs; $i++){
    read(SEG, $buffer, 4);
    @line = unpack('SS', $buffer);
    if(($line[0]>1000)||$line[1]>1000){
	print stderr "POSITION ERROR\n";
	exit(1);
    }

    $image[$line[0]][$line[1]] = 1;
}


# OUTPUT IMAGE
for($i=0; $i<1000; $i++){
    for($j=0; $j<1000; $j++){
	print "$image[$j][$i]\t";
    }
    print "\n";
}

close SEG;
close INFO;
# READ HEADER, THEN SEEK TO NEXT HEADER IF NECESSARY
#while($found ne 1){
#    read(SEG, $buffer, 20);
#    @line = unpack('iiiii', $buffer);
#    if(($line[1] eq $fc) && ($line[2] eq $array) && ($line[3] eq $img)){
#	# WE'RE LOOKING AT THE RIGHT RECORD; GENERATE THE IMAGE
#	for($i=0; $i<$line[4]; $i++){
#	    read(SEG, $buffer, 4);
#	    @line = unpack('SS', $buffer);
#	    $image[$line[0]][$line[1]] = 1;
#	}
#	$found = 1;
#    }
#    else{
#	# WE'RE AT THE WRONG RECORD; SEEK AHEAD
#	$index = ($line[4] * 4) + 2;
#	seek(SEG, $index, SEEK_SET);
#    }
#}


