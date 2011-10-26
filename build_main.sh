#!/bin/bash
# Build instructions for Polonator
# this builds the python source and dependencies
#

SCRIPT=`python -c 'import os, sys; print os.path.realpath(sys.argv[1])' $0`
PROJECT_PATH=`dirname $SCRIPT`

cd $PROJECT_PATH
cd polonator    # change to the src files directory
make            # execute the make file

