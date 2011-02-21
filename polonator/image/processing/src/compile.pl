#!/usr/bin/perl
use Cwd;

$num_args = @ARGV;
if($num_args<1){
	$wd = getcwd;
}
else{
	$wd = $ARGV[0] . "/src";
}
print "Compiling directory $wd...\n";

print "Building processor...\n";
system "g++ -O3 -fexpensive-optimizations -funroll-loops -o $wd/../processor $wd/processor.c $wd/ProcessImage.c $wd/ProcessImage_register.c $wd/ProcessImage_extract.c $wd/ReceiveData.c $wd/ReceiveFilename.c $wd/ReceiveFCNum.c $wd/GetSock.c $wd/Polonator_logger.c";

#print "Building send_data...\n";
#system "g++ -O3 -fexpensive-optimizations -o $wd/../send_data $wd/SendData.c $wd/HandleSendRequest.c $wd/Polonator_logger.c";

print "Building initialize_processor...\n";
system "g++ -O3 -funroll-loops -Wno-deprecated -o $wd/../initialize_processor $wd/initialize_processor.c $wd/ReceiveInitData.c $wd/ReceiveFilename.c $wd/ReceiveFCNum.c $wd/GetSock.c $wd/find_objects.c $wd/img_tools.c $wd/Polonator_logger.c";

print "Building make_regfile...\n";
system "g++ -O3 -fexpensive-optimizations -o $wd/../make_regfile $wd/MakeRegfile.c $wd/Polonator_logger.c";

print "Building Basecaller...\n";
system "g++ -O3 -funroll-loops -o $wd/../basecaller $wd/Basecaller.c $wd/Polonator_logger.c";
#system "g++ -o $wd/../basecaller $wd/Basecaller.c $wd/Polonator_logger.c -g -pg";

system "g++ -O3 -o $wd/../histogram $wd/Histogram.c";
system "g++ -O3 -o $wd/../histogram4 $wd/Histogram4.c";
system "g++ -O3 -o $wd/../makePrimerFile $wd/MakePrimerFile.c";

