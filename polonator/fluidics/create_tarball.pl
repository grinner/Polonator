#!/usr/bin/perl

@d = (localtime)[3..5];
$datestamp = sprintf("%02d-%02d-%04d", $d[1]+1, $d[0], $d[2] + 1900); 
$tarfilename = "polonator_fluidics_$datestamp";
@args = ("tar", "-chvf", $tarfilename, "create_tarball.pl", "src");

system(@args)==0
    or die "system @args failed: $?"
