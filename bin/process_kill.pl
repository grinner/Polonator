#!/usr/bin/perl
#
# Kills acqiusition-associated processes running on either the acquisition
# computer (by passing "acq" as the first argument) or the processing
# computer (by passing "proc" as the argument).  If running on the acq,
# will execute the complete-scan routine after killing the processes.
#
# Written by Greg Porreca (Church Lab) 08-08-2008
#

# These lists define the names of the processes to kill.  We iterate over this list,
# and for each entry, kill all running processes matching the current list element.
# Then, we re-call ps to get a new process list, and kill all processes matching the
# next element in the list
# Note the order below is important -- parents must be killed before children, since
# parents can spawn new children
#
# On the acquisition computer, we also assume that the motion controller needs
# to be re-started, so we call PolonatorUtils complete-scan, and we assume
# the syringe pump needs to be re-initialized
#

if($ARGV[0] eq "acq"){
    @findcmd = ("polonator_main", "test-img", "manual_image", "Polonator-stagealign", "Polonator-acquirer", "biochem_utils", "PolonatorUtils");
    $acq=1;
}
elsif($ARGV[0] eq "proc"){
    @findcmd = ("perl ./initialize_processor", "initialize_processor", "perl ./processor", "processor");
}


$num_cmds = @findcmd;
for($i=0; $i<$num_cmds; $i++){
    open (PS_F, "ps -Af|");
    $discard = <PS_F>;
    while(<PS_F>){
	($user, $pid, $ppid, $nice, $stime, $tty, $rtime, $command) = split(" +", $_, 8);
	chop $command;
	if($command=~/$findcmd[$i]/){
	    if($acq eq 1){
		$exec_cmd = "sudo kill $pid";
	    }
	    else{
		$exec_cmd = "kill $pid";
	    }
	    print "$exec_cmd\n";
	    system($exec_cmd);
	}
    }
    close (PS_F);
}

if($ARGV[0] eq "acq"){
    $exec_cmd = $ENV{'POLONATOR_PATH'} . "/bin/PolonatorUtils complete-scan";
    print "$exec_cmd\n";
    system($exec_cmd);

    $exec_cmd = $ENV{'POLONATOR_PATH'} . "polonator/fluidics/src/biochem_utils.pl 0 syringe_pump_init";
    print "$exec_cmd\n";
    system($exec_cmd);
}

