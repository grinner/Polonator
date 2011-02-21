#!/usr/bin/perl

open(STATUS_FILE, ">/home/polonator/PROCESS_STATUS");

open(PS, "ps aux|");
while(<PS>){
    chop $_;
    if($_=~/.+ (.+)\/processor acq/){
	print "PROCESSOR PROCESS RUNNING: $1\n";
	print STATUS_FILE "P $1\n";
    }

    if($_=~/.+ (.+)\/initialize_processor acq/){
	print "INITIALIZE_PROCESSOR PROCESS RUNNING: $1\n";
	print STATUS_FILE "I $1\n";
    }

    if($_=~/.+ (.+)\/make_regfile /){
	print "MAKE_REGFILE PROCESS RUNNING: $1\n";
	print STATUS_FILE "R $1\n";
    }
}
close PS;

close STATUS_FILE;
