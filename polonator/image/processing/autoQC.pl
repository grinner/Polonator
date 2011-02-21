#!/usr/bin/perl
# keep track of cycles already done
# iterate over directory listing, and compare each to this list
# if not already done, look for all 4 colors present;
# if all 4 present, make sure all file sizes are equal and nonzero
# if this is true, assume the cycle is finished and run QC metrics
#
# Greg Porreca (Church Lab) 11-10-2008
#

open(LOGFILE,">>autoqc-log");

$qclistfn = "qc_cycle_list.dat";
$qclistdonefn = "qc_cycle_list_processing-done.dat";
$qclistqcdonefn = "qc_cycle_list_QC-done.dat";

%qc_complete = ();
%cycles_todo = ();
$num_cycles_done = 0;
$made_defaultprimer = 0; #the first time this runs and finds a cycle, make a primer file out of it

if(!(-e $qclistfn)){
    `touch $qclistfn`;
}
if(!(-e $qclistdonefn)){
    `touch $qclistdonefn`;
}
if(!(-e $qclistqcdonefn)){
    `touch $qclistqcdonefn`;
}


# load the list of cycles alreay done
if(-e $qclistfn){
    open (QCLIST, "<$qclistfn");
    while(<QCLIST>){
	chop $_;
	$qc_complete{$_}++;
	$num_cycles_done++;
	print "$num_cycles_done\n";
    }
    close QCLIST;
}

# now determine whether each cycle we have bead files for has already been QC'd
@beadlist = glob("beads/*.beads");
for $beadfile (@beadlist){
    if($beadfile=~/beads\/0_(....)_[ACGT].beads/){
	if($qc_complete{$1} eq 1){;}
	else{
	    $cycle_toqc{$1}++;
	    ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime, $ctime, $blksize, $blocks)=stat($beadfile);
	    $beadfile_size{$beadfile} = $size;
	    print LOGFILE "not yet done cycle $1 ($beadfile:$size)\n";
	}
    }
}

# now, for each outstanding cycle to be done, make sure all data is present; if it is,
# add the cycle name to the qclist file, then to the todo list
open (QCLIST, ">>$qclistfn");
while(($key, $value)=each(%cycle_toqc)){
    if($value eq 4){
	$full_filesize = $beadfile_size{"beads/0_" . $key . "_A.beads"};
	if($full_filesize > 0){
	    if($beadfile_size{"beads/0_" . $key . "_C.beads"} eq $full_filesize){
		if($beadfile_size{"beads/0_" . $key . "_G.beads"} eq $full_filesize){
		    if($beadfile_size{"beads/0_" . $key . "_T.beads"} eq $full_filesize){
			$cycles_todo{$key}++;
			print QCLIST "$key\n";
			$now = localtime time;

			open (QCDONELIST, ">>$qclistdonefn");
			print QCDONELIST "$key\n";
			close QCDONELIST;

			print LOGFILE "$now\tFound new complete cycle $key\n";
			if(($num_cycles_done==0)&&($made_defaultprimer==0)){
			    print LOGFILE "$now\tGenerating default primer file from cycle $key\n";
			    $output = `./makePrimerFile.pl $key`;
			    $made_defaultprimer = 1;
			}
		    }
		}
	    }
	}
    }
}

close QCLIST;

# start the Xvfb
$now = localtime time;
print LOGFILE "$now\t Start Xvfb\n";
$output = `/usr/bin/Xvfb :1 -fp built-ins -once -r -screen 0 1280x1024x24&`; 
#print LOGFILE "$output\n";


# now, execute the QC for each cycle in the todo list
while(($key, $value)=each(%cycles_todo)){
    $now = localtime time;
    print LOGFILE "$now\tExecuting QC on cycle $key\n";

    $now = localtime time;
    print LOGFILE "$now\t Start Xvfb\n";
    $output = `/usr/bin/Xvfb :1 -fp built-ins -once -r -screen 0 1280x1024x24&`; 

    $cmd = "./disp_regQC.pl $key";
    $now = localtime time;
    print LOGFILE "$now\t$cmd\n";

    $return = `$cmd`;
    print LOGFILE "$return\n";
    
    $cmd = "./disp_tetra-delta.pl $key";

    $now = localtime time;
    print LOGFILE "$now\t$cmd\n";

    $return = `$cmd`;
    print LOGFILE "$return\n";
    

}

close LOGFILE;
