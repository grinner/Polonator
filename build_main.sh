#!/usr/bin/sh
# Build instructions for Polonator Java source, as an alternative to netbeans
# puts classes in a jar file and cleans up afterwards
#

SCRIPT=$(readlink -f $0) # doesn't work for os x
SCRIPT=`python -c 'import os, sys; print os.path.realpath(sys.argv[1])' $0`
PROJECT_PATH=`dirname $SCRIPT`

cd $PROJECT_PATH
cd polonator    # change to the src files directory
make            # execute the make file

