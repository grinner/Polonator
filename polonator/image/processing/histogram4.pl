#!/usr/bin/perl
# Call this to display a histogram for a cycle of images; the value for a 
# given bead will be its mean value in all 4 channels
#

$argc = $#ARGV+1;

if($argc == 3){
    $ARGV[3] = 0;
}

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
      $exec_string = "./histogram4 $dirlist[0] beads/$ARGV[1]_" . $ARGV[0] . " $ARGV[1] $ARGV[2] $ARGV[3]";
      print "EXECUTING:\t$exec_string\n";
      system "$exec_string";
  }
}
