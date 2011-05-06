#!/usr/bin/sh
# Build instructions for Polonator Java source, as an alternative to netbeans
# puts classes in a jar file and cleans up afterwards
#

SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`
cd $SCRIPTPATH/src
javac -Xlint:unchecked PolonatorAcquisitionControl.java
echo Main-Class: PolonatorAcquisitionControl > manifest.txt
jar cvfm $SCRIPTPATH/bin/PolonatorAcquisitionControl.jar manifest.txt *.class
rm *.class
rm manifest.txt
cd $SCRIPTPATH
