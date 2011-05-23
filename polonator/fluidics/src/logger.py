"""
--------------------------------------------------------------------------------
 Author: Mirko Palla.
 Date: March 14, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab -
 Genetics Department, Harvard Medical School.

 Purpose: This program contains the complete code for class Logger, containing
 device operation logging facility in Python.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
-------------------------------------------------------------------------------
"""

import os
import errno
import time
import logging
import sys

class Logger:

    def __init__(self, config):
        """
        Initialize logging facility with default parameters
        """

        if os.access(config.get("communication","log_dir"), os.F_OK) is False:
            try:
                os.makedirs(os.environ['HOME']+ config.get("communication","log_dir"))
            except OSError, e:
                if e.args[0] == errno.EEXIST: # file exists
                    pass
                else: # it was something else
                    raise OSError(e)
            # end try thing
        # end if
        # set logger format configuration parameters
        logging.basicConfig(level=logging.DEBUG,
            format='%(asctime)s.%(msecs)03d %(name)-12s %(levelname)-8s %(message)s',
            datefmt='%m-%d %H:%M:%S',
            filename = config.get("communication","log_dir") + 'biochemistry.log',
            filemode='a')

         # set a format which is simpler for console use
        formatter = logging.Formatter('%(levelname)-8s %(message)s')

        # define a Handler which writes INFO messages or higher to the sys.stdout
        console = logging.StreamHandler(sys.stdout)
        console.setLevel(logging.INFO)
        console.setFormatter(formatter)    # tell the handler to use this format
        # add the handler to the root logger
        logging.getLogger('').addHandler(console)

        loglevel = 1

    def log(self, level,message):
        """
        Forwards log messages to the logging module instance.
        """
        logging.log(level,message)

    def warn(self, message):
        """
        Diplays a warning message via logging facility.
        """
        logging.warn(message)

    def info(self, message):
        """
        Diplays an informative message via logging facility.
        """
        logging.info(message)

    def error(self, message):
        """
        Diplays an error message via logging facility.
        """
        logging.error(message)

    def debug(self, message):
        """
        Diplays a debugging message via logging facility.
        """
        logging.debug(message)

