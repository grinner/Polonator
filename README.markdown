
# Polonator G.007 source

The Polonator Source is going github. 
MIT license on everything

## Release Background

[The Wyss Institute](http://wyss.harvard.edu) is funding improvements to the Polonator source.
The Polonator was originally a rush job, so the code a bit rough around the edges.
As a commitment to the project, and generally building open software and hardware
I'm majorly refactoring the code base.  
This is the start of an organization process of the code, which has been slowly accumulating
over the last year.  
  
The fundamental things is that we are doing:

1. More Python
2. Python gets access to C data structures through [swig](http://www.swig.org/)'d [numpy](http://numpy.scipy.org) arrays (open to anyone changing this, to Boost.Python, or whatever)
3. The result is a more scriptable automated microscope
4. Accelerating and multithreading the image processing and basecalling
     
If you want to contact me mail: nick.conway at wyss.harvard.edu


## Installation

For now all I have is the source, this will change as updates proceed

## Usage

Polonator will be an installable python module. You might find
it most useful for tasks involving DNA sequencing. Typical usage
often looks like this::

    #!/usr/bin/env python

    from polonator import maestro
    from polonator import camera

    if camera.isReady():
        print "Taking another photo:", maestro.snap()

## Links

* [Image ingestion simulator](https://github.com/grinner/PolonatorProcessorSim) - a hack of the G.007 image ingestion system to create a standalone simulator for developing new image processing and basecalling algorithms for DNA sequencing
* http://polonator.org , it's true the site needs updating
