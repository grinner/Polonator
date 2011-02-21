#!/usr/bin/perl

open INF, $ARGV[0];
open BEADSUM, $ARGV[1];

binmode INF;
binmode BEADSUM;

while(read(INF, $buffer, 8) and @line = unpack('SSSS', $buffer) and print "$line[0]\t$line[1]\t$line[2]\t$line[3]\t" and read(BEADSUM, $buffer2, 8) and @line = unpack('q', $buffer2) and print "$line[0]\n"){;}
close INF;
close BEADSUM;
