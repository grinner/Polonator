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
print DateTime->now->ymd;

$full_date = DateTime->now->ymd;
$year = substr $full_date, 2,2;
$month = substr $full_date, 5,2;
$date = substr $full_date, 8,2;
print $month;
print $date;
print $year;

$ACQUISITION_NAME = "acq";
$final_position_filename = $month . $date. $year;;

# Before we do anything, we need to compile the source
#
$working_dir = getcwd();
$exec_string = "src/compile.pl $working_dir";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";

$exec_string = "$working_dir/merge_table $final_position_filename";
print "EXECUTING:\t$exec_string\n";
system "$exec_string";
