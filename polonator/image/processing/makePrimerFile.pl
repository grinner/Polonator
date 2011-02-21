#!/usr/bin/perl
# Call this to generate a .beads file for the basecaller which is a 4-color composite from
# a single ligation cycle.  This is used to determine which beads are amplified and which
# are not.
#
# Execute as ./makePrimerFile.pl cyclename
#
# Greg Porreca (Church Lab) 11-10-2008
#
# 
# NOTE: fix for dual-flowcell by removing flowcellnum arg and auto-determine how many
# flowcells' worth of data are present
#

@dirlist = glob("*.info");
$num_infofiles = @dirlist;

if($num_infofiles>1){
  print stderr "ERROR:\tthere are $num_infofiles possible input files, and I don't know which to use:\n";
  for($i=0; $i<$num_infofiles; $i++){
    print stderr "\t$dirlist[$i]\n";
  }
}

else{
  if($dirlist[0]=~/(.+).info/){
      $exec_string = "./makePrimerFile $dirlist[0] 0 $ARGV[0]";
      print "EXECUTING:\t$exec_string\n";
      system "$exec_string";
  }
}
