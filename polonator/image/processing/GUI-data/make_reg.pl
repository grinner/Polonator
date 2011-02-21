#!/usr/bin/perl
#
# initialize_processor.pl
#
# Execute this perl script to initialize the image processing pipeline.
# The directory should be clean (i.e. no data files present from a
# previous run).  Compile must have been run in the src/ directory to
# generate the binaries.
#
# Written by Greg Porreca (Church Lab) 01-23-2008
#

use Cwd;
$ACQUISITION_NAME = "acq";
# Before we do anything, we need to compile the source
#
$working_dir = getcwd();
$exec_string = "src/compile.pl $working_dir";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

$num_fcs = "1";
# Now, run make_regfile
@dirlist = glob("*.info");
$num_infofiles = @dirlist;

# Only call make_regfile.c if we can unambiguously determine the
# name of the bead position file.  There should be one position file
# and one info file (with the same name) in the directory.  If more
# are present, the directory was not cleaned up properly after the
# last run.
# 
if($num_infofiles==2){
  #print stderr "ERROR:\tthere are $num_infofiles possible input files, and I don't know which to use:\n";
#  for($i=0; $i<$num_infofiles; $i++){
    #print stderr "\t$dirlist[$i]\n";
 if($dirlist[0]=~/(.+).info/){
    $position_filename = $1;
    print "the 1st reg file name is : $1";
    print "the total files are : $num_infofiles";
    $exec_string = "$working_dir/make_regfile $position_filename $num_fcs";
    print "EXECUTING:\t$exec_string\n";
    system "$exec_string";

  }  

 if($dirlist[1]=~/(.+).info/){
    $position_filename = $1;
    print "the 2nd reg file name is : $1";
    print "the total files are : $num_infofiles";
    $exec_string = "$working_dir/make_regfile $position_filename $num_fcs";
    print "EXECUTING:\t$exec_string\n";
    system "$exec_string";

  }   

#  }
  #exit(0);
}

else{
  if($dirlist[0]=~/(.+).info/){
    $position_filename = $1;
    $exec_string = "$working_dir/make_regfile $position_filename $num_fcs";
    print "EXECUTING:\t$exec_string\n";
    system "$exec_string";
  }
  else{
    print stderr "ERROR:\tNo file in the directory matches the expected pattern (*.info);\n";
    print stderr "\tinitialize_processor.c must have failed unexpectedly.\n";
    print stderr "\tExiting, pipeline not initialized.\n";
    exit(0);
  }
}

print "initialize_processor.pl exiting w/ return value of $num_fcs\n";
open(NUM_FCS, ">NUMBER_OF_FLOWCELLS.dat");
print NUM_FCS "$num_fcs\n";
close NUM_FCS;
exit($num_fcs);
