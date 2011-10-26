#!/bin/bash
# Build instructions for Polonator Java source, as an alternative to netbeans
# puts classes in a jar file and cleans up afterwards
#

SCRIPT=$(readlink -f $0) # doesn't work for os x
SCRIPT=`python -c 'import os, sys; print os.path.realpath(sys.argv[1])' $0`
#SCRIPT=`perl -e 'use Cwd "abs_path";print abs_path(shift)' $0`
SCRIPTPATH=`dirname $SCRIPT`

echo $SCRIPTPATH
# cd $SCRIPTPATH
mkdir -p $SCRIPTPATH/bin
cd $SCRIPTPATH/src
javac -Xlint:unchecked PolonatorAcquisitionControl.java
echo Main-Class: PolonatorAcquisitionControl > manifest.txt
jar cvfm $SCRIPTPATH/bin/PolonatorAcquisitionControl.jar manifest.txt *.class
rm *.class
rm manifest.txt
cd $SCRIPTPATH

