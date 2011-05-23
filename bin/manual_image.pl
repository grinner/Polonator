#!/usr/bin/perl
#
# Usage:
# ./manual_image.pl cyclename fcnum
#  where
#  cyclename is the name of the cycle; for WL imaging, this
#          is WL1 or WL2 (WL1 for single-flowcell, WL2 for dual)
#          and fcnum is 0 for single flowcell; if dual-flowcell,
#          call w/ WL2 and then 2 or 3 for fcnum (2 for fc 0, 3 for fc 1)
#          For regular cycles, use 4-character cycle name and 0 or 1
#          for fcnum
#
# This script is normally called by the PolonatorAcquisitionControl GUI
#

$cmd = join(' ', @ARGV);
$cmd = "python test-img.py " . $cmd;
print "$cmd\n";
system "$cmd";
