#!/usr/bin/env python
# encoding: utf-8
"""
setup.py

Created by Nick Conway on 2010-11-29.
Copyright (c) 2010 __Wyss_Institute__. All rights reserved.
"""

from distutils.core import setup

setup(
    name='Polonator',
    version='0.1.0',
    author='Nick Conway',
    author_email='nick.conway@wyss.harvard.edu',
    packages=['polonator', 'polonator.test'],
    scripts=['bin/front_end.py','bin/live_view.py'],
    url='http://pypi.python.org/pypi/polonator/',
    license='LICENSE.txt',
    description='Python and compiled C code for runnign a Polonator G.007',
    long_description=open('README.txt').read(),
)

