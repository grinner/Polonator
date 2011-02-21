#!/usr/bin/perl

@proc_array = `ps -Afl`;
$num_procs = @proc_array;

for($i=0; $i<$num_procs; $i++){
    @curr_proc = split(/\t/, $proc_array[$i]);
    print "@curr_proc";
}
