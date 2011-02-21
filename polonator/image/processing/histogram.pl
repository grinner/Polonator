#!/usr/bin/perl
# Call this to display a histogram for a single set of images
# (i.e. either a set of primer images, or one color from a sequencing cycle)
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
      $exec_string = "./histogram $dirlist[0] beads/$ARGV[1]_" . $ARGV[0] . ".beads $ARGV[1] $ARGV[2] $ARGV[3]";
      print "EXECUTING:\t$exec_string\n";
      system "$exec_string";
  }
}
