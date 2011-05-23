#!/usr/local/bin/python

"""
--------------------------------------------------------------------------------
 Author: Mirko Palla.
 Date: May 8, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: given function command, this utility program performs any fluidics 
 method contained in biochemistry module.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
modified by Richard Terry Jun-6-2009 bug fix
"""

import os
import sys
import time
import logging

#----------------------------- Input argument handling -------------------------
log = logging.getLogger('biochem_utils')

if len(sys.argv) < 3:
    print '\n--> Error: not correct input!\n--> Usage: python ' + \
            'biochem_utils.py flowcell-number method\n'
    sys.exit()

else:
    import ConfigParser                    
    from biochem import Biochem            
    config = ConfigParser.ConfigParser()
    config.readfp(open(os.environ['POLONATOR_PATH'] + '/config_files/sequencing.cfg'))
    home_dir = os.environ['HOME'] + config.get("communication","home_dir")

    # Initialize biochemistry object - cycle-name and flowcell-number 
    # need to be set later
    biochem = Biochem('WL1', int(sys.argv[1]))  
    method = sys.argv[2]
    log.info("---\t-\t--> Started %s method execution - biochem_utils.py" % \
                (method))



    # usage: biochem_utils.py fcnum cycle_ligation cyclename
    # cyclename must be a valid cyclename that has a nonamer/hyb
    # valve/port mapping in sequencing.cfg
    # cyclename can be either 3 or 4 chars in length
    if method == 'cycle_ligation':
        biochem.flowcell = int(sys.argv[1])
        biochem.cycle_name = sys.argv[3]
        biochem.cycle = biochem.cycle_name[0:3]
        biochem.cycle_ligation()


    # usage: biochem_utils.py fcnum flush_flowcell
    elif method == 'flush_flowcell':
        reagent = "Wash 1"
        if reagent == 'Wash 1':
            port = 9
        elif reagent == 'dH2O':
            port = 7

        biochem.flush_flowcell(9)
        #biochem.rinse(int(sys.argv[1]))

    # usage: biochem_utils.py fcnum draw_into_flowcell
    elif method == 'draw_into_flowcell':
        reagent = "Wash 1"
        biochem.draw_into_flowcell(9, sys.argv[3], int(sys.argv[4]), \ 
            int(sys.argv[5])) 


    # usage: biochem_utils.py fcnum draw_into_flowcell_bufferchase
    elif method == 'draw_into_flowcell_bufferchase':
        reagent = "Wash 1"
        biochem.draw_into_flowcell_bufferchase(9, sys.argv[3], \
            int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6]), \
            int(sys.argv[7]) ) 



    # usage: biochem_utils.py fcnum hyb hybValve hybPort
    # hybValve = [V1..V3]
    # hybPort = [1..7]
    elif method == 'hyb':
        biochem.hyb(sys.argv[3], int(sys.argv[4]))


    # usage: biochem_utils.py fcnum lig_stepup_peg ligValve ligPort
    # ligValve = [V1..V3]
    # port = [1..6]; assumes lig buffer is on port 7
    elif method == 'lig_stepup_peg':
        biochem.lig_stepup_peg(sys.argv[3], int(sys.argv[4]))

    # usage: biochem_utils.py fcnum prime_reagent_block V4flag
    # V4flag==1 to prime V4, V4flag==0 to not prime V4
    elif method == 'prime_reagent_block':
        biochem.primeReagentBlock(int(sys.argv[1]), int(sys.argv[3]))


    # usage: biochem_utils.py fcnum react reagentValve reagentPort temp time 
    # bufferBeforeFlag bufferAfterFlag
    # OR biochem_utils.py fcnum react reagentValve reagentPort temp time 
    # bufferBeforeFlag bufferAfterFlag bufferPort bufferVolume
    # depending on whether a reaction buffer is used before/after the reagent 
    # to prime/flush the lines
    elif method == 'react':
        if(len(sys.argv) == 9):
            biochem.react(sys.argv[3], int(sys.argv[4]), int(sys.argv[5]), \
                int(sys.argv[6]), int(sys.argv[7]), int(sys.argv[8]))
        else:
            biochem.react(sys.argv[3], int(sys.argv[4]), int(sys.argv[5]), \
                int(sys.argv[6]), int(sys.argv[7]), int(sys.argv[8]), \
                int(sys.argv[9]), int(sys.argv[10]))


    # usage: biochem_utils.py fcnum strip_chem [V1..V3]
    elif method == 'strip_chem':
        biochem.strip_chem('V4', 5)


    # usage: biochem_utils.py fcnum syringe_pump_init
    elif method == 'syringe_pump_init':
        biochem.syringe_pump_init()


    elif method == 'set_temp':
        biochem.setTemp(int(sys.argv[1]), int(sys.argv[3]))


    # LEGACY FUNCTIONS
    elif method == 'prime_rotary_valve1':
        biochem.prime_rotary_valve1()

    elif method == 'prime_rotary_valve2':
        biochem.prime_rotary_valve2()

    elif method == 'prime_rotary_valve3':
        biochem.prime_rotary_valve3()

    elif method == 'temperature_control_init':
        biochem.temperature_control_init()


    # ILLUMINA FUNCTIONS
    #
    # usage: biochem_utils.py fcnum ilmnDeblock
    elif method == 'ilmnDeblock':
        biochem.runILMNDeblock(int(sys.argv[1]), sys.argv[3])

    # usage: biochem_utils.py fcnum ilmnCycle
    elif method == 'ilmnCycle':
        biochem.runILMNCycle(int(sys.argv[1]), sys.argv[3])


    # usage: biochem_utils.py fcnum illumina
    elif method == 'illumina':
        biochem.runILMNDeblock(int(sys.argv[1]), sys.argv[3])
        biochem.runILMNCycle(int(sys.argv[1]), sys.argv[4])


    # usage: biochem_utils.py fcnum ilmnHyb
    elif method == 'ilmnHyb':
        biochem.runILMNHyb(int(sys.argv[1]), sys.argv[3])




    else:
        print '\nWARN\t ---\t-\t--> Error: not correct method ' + \
              'input!\nWARN\t ---\t-\t--> Double check method name ' + \
              '(2nd argument)\n'
        sys.exit()

