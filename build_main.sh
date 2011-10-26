#!/bin/bash
# Build instructions for Polonator
# this builds the python source and dependencies
#

SCRIPT=`python -c 'import os, sys; print os.path.realpath(sys.argv[1])' $0`
PROJECT_PATH=`dirname $SCRIPT`

cd $PROJECT_PATH
cd polonator    # change to the src files directory

if [ $1 = "dmd" ] ; then
	make
	make python_illum python_proc_dmd
	echo "making with DMD support"
elif [ $1 = "clean" ] ; then 
	make clean
	echo "cleaned build"
else
	make
	echo "Default build"
fi


