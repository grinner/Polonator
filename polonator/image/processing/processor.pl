#!/usr/bin/perl
#
# processor.pl
#
# Execute this perl script to start the image processing pipeline.
# Initialize_processor must have already been run for this to work.
#
# Written by Greg Porreca (Church Lab) 01-23-2008
#
use Cwd;

$ACQUISITION_NAME = "acq";
$current_dir = getcwd();

# first, make sure there isn't another processor.pl or processor
# process running; if so, display information and exit
#
@running_processes1 = `ps -C processor -o pid=`;
@running_processes2 = `ps -C processor.pl -o pid=`;

if((@running_processes2 > 1)||(@running_processes1>0)){
    print stderr "ERROR: there are " . @running_processes1 . " instances of processor running,\n";
    print stderr "       and " . @running_processes2 . " instances (including this one) of processor.pl\n";
    system("ps -flC processor");
    system("ps -flC processor.pl");
    exit(0);
}

@dirlist = glob("*.info");
$num_infofiles = @dirlist;

# Only call processor.c if we can unambiguously determine the
# name of the bead position file.  There should be one position file
# and one info file (with the same name) in the directory.  If more
# are present, the directory was not cleaned up properly after the
# last run.
# 
if($num_infofiles>1){
  print stderr "ERROR:\tthere are $num_infofiles possible input files, and I don't know which to use:\n";
  for($i=0; $i<$num_infofiles; $i++){
    print stderr "\t$dirlist[$i]\n";
  }
}

else{
  if($dirlist[0]=~/(.+).info/){
    $position_filename = $1;
    
    open(NUM_FCS, "<NUMBER_OF_FLOWCELLS.dat") or die("ERROR: could not find file NUMBER_OF_FLOWCELLS.dat\n");
    $num_fcs = <NUM_FCS>;
    chop $num_fcs;
    close NUM_FCS;

    $exec_string = "$current_dir/processor $ACQUISITION_NAME $position_filename $num_fcs";
    print "EXECUTING:\t$exec_string\n";
    system "$exec_string";
  }
}
