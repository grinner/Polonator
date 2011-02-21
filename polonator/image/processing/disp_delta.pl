#!/usr/bin/perl
use Cwd;

$cw = getcwd;
$ENV{'MATLABPATH'} = $cw;
$ENV{'LIBXCB_ALLOW_SLOPPY_LOCK'} = 1;


system "./run_disp_delta.sh /opt/MATLAB/MATLAB_Component_Runtime/v77/ $ARGV[0] $ARGV[1]";

$cyclename = substr($ARGV[1],3,4);

open(QCDONE, "<qc_cycle_list_QC-done.dat");
$found = 0;
while(<QCDONE>){
    chop $_;
    if($_=~/$cyclename/){
	$found = 1;
    }
}
close QCDONE;

if($found eq 0){
    open(QCDONE, ">>qc_cycle_list_QC-done.dat");
    print QCDONE "$cyclename\n";
    close QCDONE;
}

