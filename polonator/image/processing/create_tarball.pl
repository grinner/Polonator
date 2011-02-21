#!/usr/bin/perl

@d = (localtime)[3..5];
$datestamp = sprintf("%02d-%02d-%04d", $d[1]+1, $d[0], $d[2] + 1900); 
$tarfilename = "processor_pipeline_$datestamp";
@args = ("tar", "-cPvf", $tarfilename, glob("*.pl"), glob("*.mtemplate"), glob("G007.positions*"), "src", "create_tarball.pl", "GUI-data", "lib", "PolonatorProcessorControl.jar", "disp_cfd.m", glob("*.ctf"), glob("*.sh"), "disp_regQC", "disp_delta", "disp_tetra", "display_objects", "display_color_raw", "disp_delta_mcr", "disp_regQC_mcr", "disp_tetra_mcr", "display_objects_mcr", "display_color_raw_mcr", "/home/polonator/NetBeansProjects");
system(@args)==0
    or die "system @args failed: $?"
