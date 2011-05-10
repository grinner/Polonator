
# Polonator G.007 source

The Polonator Source is going github. 
MIT license on everything

## Release Background

[The Wyss Institute](http://wyss.harvard.edu) is funding improvements to the Polonator source.
Much of the Polonator source code was originally a rush job, so the code is a bit rough around the edges.
As a commitment to the project, and generally building open software and hardware
I'm majorly refactoring the code base.  
This is the start of an organization process of the code, which has been slowly accumulating
over the last year.  
  
The fundamental things we are doing:

1. More Python
2. Python gets access to C data structures through [swig](http://www.swig.org/)'d [numpy](http://numpy.scipy.org) arrays (open to anyone changing this, to Boost.Python, or whatever)
3. The result is a more scriptable automated microscope
4. Accelerating and multithreading the image processing and basecalling
5. Updating the user interface.
6. Code prettying
     
If you want to contact me mail: nick.conway at wyss.harvard.edu


## Installation

For now all you can grab is the source. This will change as updates proceed.
Eventually, a full installer should be available.
The dependencies are:

1. [SWIG](http://www.swig.org/), [license](http://www.swig.org/legal.html) whatever license you like for redistribution
2. Python 2.X, v2.5 or greater
3. Perl
4. [Numpy](http://numpy.scipy.org), [license](http://numpy.scipy.org/license.html#license) BSD
5. Open source Phoenix drivers v5.59 for the [Phoenix](http://www.activesilicon.com/products_fg_phx.htm) frame grabber from [Active Silicon](http://www.activesilicon.com/)
6. Java, and even Netbeans for reviewing the old GUI

To build:

1. git clone the repository to a directory, such as /usr/local to create the directory such as /usr/local/Polonator
2. set the POLONATOR\_PATH environment variable to the above directory in your .profile, .bash_profile, or .bashrc. For instance in bash run
    
    `export POLONATOR_PATH = /usr/local/Polonator` 

3. change directories to the:
    
    `cd POLONATOR_PATH`
    
4. run: 
    
    `sh build_main.sh`


## Usage

Polonator will be an installable python module. You might find
it most useful for tasks involving DNA sequencing. Typical usage
often looks like this:

    #!/usr/bin/env python

    from polonator import maestro
    from polonator import camera

    if camera.isReady():
        print "Taking another photo:", maestro.snap()

## Links

* [Image ingestion simulator](https://github.com/grinner/PolonatorProcessorSim) - a hack of the G.007 image ingestion system to create a standalone simulator for developing new image processing and basecalling algorithms for DNA sequencing
* http://polonator.org , it's true the site needs updating
