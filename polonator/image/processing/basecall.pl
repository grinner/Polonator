#!/usr/bin/perl

$argc = @ARGV;

if($argc ne 3){
    die("ERROR: must call with 3 args: base name of output files, percentage of data to keep, and background subtraction pixel value expressed as a percentage\n");
}


open(READSPECFILE, "<readspec") or die("ERROR: readspec (file specifying cycles to call) not present\n");
open(READINFO, ">output_data/$ARGV[0].readfileinfo");

@d = (localtime)[3..5];
$datestamp = sprintf("%02d-%02d-%04d", $d[1]+1, $d[0], $d[2] + 1900);
print READINFO "Basecaller executed on $datestamp\n\n";
print READINFO "Perl script called as ./basecall.pl $ARGV[0] $ARGV[1] $ARGV[2]\n";
print READINFO "--basecall file name: $ARGV[0]\n--percentage of data to keep: $ARGV[1]\n--percentage pixel value for background subtraction: $ARGV[2]\n\n";
print READINFO "Using the following cycles:\n";

while(<READSPECFILE>){
    chop $_;
    $fn = "beads/0_" . $_;
    $beadfiles = $beadfiles . $fn . "_A ";
    $beadfiles = $beadfiles . $fn . "_C ";
    $beadfiles = $beadfiles . $fn . "_G ";
    $beadfiles = $beadfiles . $fn . "_T ";
    print READINFO "\t$_\n";
}
close(READSPECFILE);

@dirlist = glob("*.info");
$infofilename = $dirlist[0];
print READINFO "\nUsing info file $dirlist[0]\n";
print READINFO "\nUsing primer thresholds of: (FC0 lanes 1-8, FC1 lanes 1-8)\n";
open(PRIMERTHRESH, "<primer_thresholds.dat");
while(<PRIMERTHRESH>){
    print READINFO "$_";
}
close PRIMERTHRESH;

print READINFO "\nExecuting basecaller as follows:\n";

# Use this one to output tetrahedron info
$basecallercmd = "./basecaller notruncmdln 1 1 $ARGV[0] $infofilename $ARGV[1] $ARGV[2] $beadfiles";

# Use this one to not output tetrahedron info
#$basecallercmd = "./basecaller notruncmdln 1 0 $ARGV[0] $infofilename $ARGV[1] $ARGV[2] $beadfiles";
print READINFO "$basecallercmd\n";
close READINFO;

print "EXECUTING $basecallercmd\n";

system($basecallercmd);


