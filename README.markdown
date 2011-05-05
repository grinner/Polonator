
# Polonator G.007 source

The Polonator Source is going github. 
MIT license on everything

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

## Link

http://polonator.org , it's true the site needs updating
