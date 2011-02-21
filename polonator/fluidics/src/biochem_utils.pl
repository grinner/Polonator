#!/usr/bin/perl

$cmd = join(' ', @ARGV);
$cmd = "python /home/polonator/G.007/G.007_fluidics/src/biochem_utils.py " . $cmd;
print "$cmd\n";
system "$cmd";

