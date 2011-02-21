#!/usr/bin/perl
use Cwd;

$cw = getcwd;
$ENV{'MATLABPATH'} = $cw;
$ENV{'LIBXCB_ALLOW_SLOPPY_LOCK'} = 1;

system "./run_disp_tetra.sh /opt/MATLAB/MATLAB_Component_Runtime/v77/ $ARGV[0] $ARGV[1] $ARGV[2]"


#open(OUTFILE, ">disp_tetra.m");
#open(TEMPL, "disp_tetra.mtemplate");
#
#print OUTFILE "filename = '$ARGV[0]';\n";
#print OUTFILE "frame = $ARGV[1];\n";
#print OUTFILE "outputfilename = '$ARGV[2]';\n";
#
#while(<TEMPL>){
#    print OUTFILE "$_";
#}
#close OUTFILE;
#close TEMPL;
#
#system "matlab -display :1 -nosplash -r disp_tetra";


