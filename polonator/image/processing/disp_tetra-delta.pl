#!/usr/bin/perl

#$TETRA_FRAME = 1151;
$TETRA_FRAME = 1000;
$PERCENT_TOKEEP = 50;
$PERCENT_BGSUBTRACT = 5;

# Verify basecaller input files are present
#
$beadfn1 = "beads/0_" . $ARGV[0] . "_A";
$beadfn2 = "beads/0_" . $ARGV[0] . "_C";
$beadfn3 = "beads/0_" . $ARGV[0] . "_G";
$beadfn4 = "beads/0_" . $ARGV[0] . "_T";
if((@x=glob("$beadfn1*")) eq 0){
  print stderr "ERROR:$file_present\tbead file $beadfn1 does not exist.  Exiting\n";
  exit(1);
}
if((@x=glob("$beadfn2*")) eq 0){
  print stderr "ERROR:\tbead file $beadfn2 does not exist.  Exiting\n";
  exit(1);
}
if((@x=glob("$beadfn3*")) eq 0){
  print stderr "ERROR:\tbead file $beadfn3 does not exist.  Exiting\n";
  exit(1);
}
if((@x=glob("$beadfn4*")) eq 0){
  print stderr "ERROR:\tbead file $beadfn4 does not exist.  Exiting\n";
  exit(1);
}


# Run basecaller to generate .tetra files and .delta files
#
@dirlist = glob("*.info");
$num_infofiles = @dirlist;
if($num_infofiles>1){
  print stderr "ERROR:\ttoo many info files (expect 1); exiting\n";
  exit(1);
}
elsif($num_infofiles eq 0){
  print stderr "ERROR:\tunable to find a .info file; exiting\n";
  exit(1);
}
$info_filename = $dirlist[0];

$exec_cmd = "./basecaller notruncmdln 0 1 $ARGV[0]-QC $info_filename $PERCENT_TOKEEP $PERCENT_BGSUBTRACT $beadfn1 $beadfn2 $beadfn3 $beadfn4";
print "EXECUTING:\t$exec_cmd\n";
system "$exec_cmd";


# Run matlab QC routines
#$exec_cmd = "./disp_regQC.pl $ARGV[0]";
#print "EXECUTING:\t$exec_cmd\n";
#system "$exec_cmd";

$exec_cmd = "./disp_delta.pl tetrahedra/$ARGV[0]-QC.delta QC-$ARGV[0]-delta";
print "EXECUTING:\t$exec_cmd\n";
system "$exec_cmd";


for($i=0; $i<8; $i++){
    $numstr = sprintf("%02d", $i);
    $exec_cmd = "./disp_tetra.pl tetrahedra/$ARGV[0]-QC_0_" . $numstr . ".tetracoords $TETRA_FRAME QC-$ARGV[0]-0_" . $numstr . "-tetra";
    print "EXECUTING:\t$exec_cmd\n";
    system "$exec_cmd";
}


