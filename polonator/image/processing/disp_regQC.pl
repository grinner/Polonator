#!/usr/bin/perl
use Cwd;

$cw = getcwd;
$ENV{'MATLABPATH'} = $cw;

$mlarg1 = "logs/0_$ARGV[0]_A.register-log"; # RED
$mlarg2 = "logs/0_$ARGV[0]_C.register-log"; # GREEN
$mlarg3 = "logs/0_$ARGV[0]_G.register-log"; # BLUE
$mlarg4 = "logs/0_$ARGV[0]_T.register-log"; # BLACK
$mlarg5 = "$ARGV[0]";

system "./run_disp_regQC.sh /opt/MATLAB/MATLAB_Component_Runtime/v77/ $mlarg1 $mlarg2 $mlarg3 $mlarg4 $mlarg5";


