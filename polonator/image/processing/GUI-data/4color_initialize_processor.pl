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
use DateTime;

$ACQUISITION_NAME = "acq";

$full_date = DateTime->now->ymd;
$year = substr $full_date, 2,2;
$month = substr $full_date, 5,2;
$date = substr $full_date, 8,2;

$final_position_filename = $month . $date. $year;;

# Before we do anything, we need to compile the source
#
$working_dir = getcwd();
$exec_string = "src/compile.pl $working_dir";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

# Only call make_regfile.c if we can unambiguously determine the
# name of the bead position file.  There should be one position file
# and one info file (with the same name) in the directory.  If more
# are present, the directory was not cleaned up properly after the
# last run.
# 


# First, execute initialize_processor.c to generate the bead position file
# and the .info file.  Then, when that is finished, run make_regfile to
# generate data files used by processor.c during fluorescence image processing.
#

# Don't go any further if a position file already exists, since we
# may have been run by accident and will clobber the existing pipeline
#
@dirlist = glob("*.info");
$num_infofiles = @dirlist;
if($num_infofiles>0){
  print stderr "ERROR:\tOne or more position info files already exist in the current directory;\n";
  print stderr "\tfirst pre-existing position info file found is $dirlist[0]\n";
  print stderr "\texiting to prevent accidental clobbering of existing processing pipeline\n";
  exit(0);
}

$exec_string = "$working_dir/initialize_processor $ACQUISITION_NAME";
print "EXECUTING:\t$exec_string\n";
$num_fcs = system("$exec_string") >> 8; #initialize_processor.c returns the number of flowcells
print "initialize_processor returned $num_fcs flowcells for this run\n";

$exec_string = "mv autoexp_pos autoexp_pos_fam";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

$exec_string = "$working_dir/initialize_processor $ACQUISITION_NAME";
print "EXECUTING:\t$exec_string\n";
$num_fcs = system("$exec_string") >> 8; #initialize_processor.c returns the number of flowcells
print "initialize_processor returned $num_fcs flowcells for this run\n";

$exec_string = "mv autoexp_pos autoexp_pos_cy3";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

$exec_string = "$working_dir/initialize_processor $ACQUISITION_NAME";
print "EXECUTING:\t$exec_string\n";
$num_fcs = system("$exec_string") >> 8; #initialize_processor.c returns the number of flowcells
print "initialize_processor returned $num_fcs flowcells for this run\n";

$exec_string = "mv autoexp_pos autoexp_pos_txr";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

$exec_string = "$working_dir/initialize_processor $ACQUISITION_NAME";
print "EXECUTING:\t$exec_string\n";
$num_fcs = system("$exec_string") >> 8; #initialize_processor.c returns the number of flowcells
print "initialize_processor returned $num_fcs flowcells for this run\n";

$exec_string = "mv autoexp_pos autoexp_pos_cy5";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

$exec_string = "$working_dir/merge_table $final_position_filename";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

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
