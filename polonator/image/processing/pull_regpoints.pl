#!/usr/bin/perl
#
# Used to generate a binary 'image' of all objects used for registration of
# a given array position.  Object locations are from the reg file.  Execute
# as:
# ./pull_regpoints.pl a b c > image_filename
# where:
#   a = the flowcell number ([0..1])
#   b = the array number    ([0..17])
#   c = the image number    ([0..IMGS_PER_ARRAY])
#
# Written by Greg Porreca (Church Lab) 12-14-2007
#

$num_arrays = 8;
$num_imgs = 2180;
$reg_pixels = 20000;

$fc = $ARGV[0];
$array = $ARGV[1];
$img = $ARGV[2];

print stderr "Seeking to position $fc $array $img: ";


# DETERMINE REG FILENAME
@array = glob('*.info');
$num_files = @array;
$num_files+=0;
if($num_files ne 1){
    print STDERR "ERROR: found $num_files info files in current directory (must be 1)\n";
    exit;
}
if($array[0]=~/(.+)\.info$/){
    $regfn = $1 . ".reg";
}
open REG, $regfn;
binmode REG;

$index = ($reg_pixels * $img * 4) + ($array * $num_imgs * $reg_pixels * 4) + ($fc * $num_arrays * $num_imgs * $reg_pixels * 4);

print stderr "byte $index...\n";

seek(REG, $index, SEEK_SET);

# INITIALIZE IMAGE
for($i=0; $i<1000; $i++){
    for($j=0; $j<1000; $j++){
	$image[$j][$i] = 0;
    }
}

# SET REG PIXELS TO 1
for($i=0; $i<$reg_pixels; $i++){
    read(REG, $buffer, 4);
    @line = unpack('SS', $buffer);
    $image[$line[0]][$line[1]] = 1;
}

for($i=0; $i<1000; $i++){
    for($j=0; $j<1000; $j++){
	print "$image[$j][$i]\t";
    }
    print "\n";
}

close REG;
