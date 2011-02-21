import struct
import random
import glob
import basecall
import objectTable
import os

"""
        Nick Conway Wyss Institute  02-02-2010
        Converts a basecall file to an object table file.  This is used for and was created for 
        testing so all the user needed to do was generate a fake basecall file rather than produce
        an entire normal object table
        
        The object table is parsed as follows since it is a binary file
        
        FIELD            VALUE                    SIZE           DESCRIPTION
        start            -1                       4 bytes        start of next block (1 block per image)
        flowcell_id      {0..FCs_PER_RUN - 1}     4 bytes        the number of flowcells currently on the Polonator (usually 1 or 2)
        array_id         {0..ARRAYS_PER_FC - 1}   4 bytes        the number of arrays the Polonator will image per flowcell (usually 18)
        image_id         {0..IMGS_PER_ARRAY - 1}  4 bytes        the number of images per array the Polonator will acquire (usually 2514)
        num_objects      {0..65535}               4 bytes        maximum number of objects per frame is largest allowable short unsigned int
        beadpos_xcol     {0..NUM_XCOLS - 1}       2 bytes        X image position of centroid for current bead
        beadpos_yrow     {0..NUM_YROWS - 1}       2 bytes        Y image position of centroid for current bead

        The basecall file is parsed as follows since it is a tab delimited text file
        FLOWCELL#    LANE#      IMAGE#    BEAD#    X    X    X    FLAG
        one basecall file per lane
        
        the number of beads flagged per lane in a given basecall file is in the *.hit_summary file
        in a tab delimited fashion
"""
bc = basecall
ot = objectTable

XMAX = 1000 # camera max resolution in X
YMAX = 1000 # camera max resolution in Y

# now look up the base call file
#polonator_data_dir = '/home/polonator/G.007/G.007_acquisition/'
polonator_data_dir = os.path.abspath(os.getcwd())#'Z:/polonator/code/polonator_dmd_C/src-nick/'
base_dir = polonator_data_dir + '\\output_data\\'
the_in_files = glob.glob(base_dir + '*.basecalls') # there is a basecall file for each lane?
the_hit_sum_files = glob.glob(base_dir + '*.hit_summary')
print the_hit_sum_files[0]
in_file_stats = open(the_hit_sum_files[0], 'r') 


struct.calcsize('i')    # i should be 4 bytes
struct.calcsize('h')    # h should be 2 bytes 

def BcToOt():
    print "starting"
    last_image = -1
    
    the_basecall = bc.BaseCall(the_in_files[0])
    the_out_file = open(base_dir+'fake_object.table', 'wb')
    
    while True:
        the_basecall.read_entry()
    
        if (the_basecall.isEOF()):   # breaks the loop when a line length is zero indicating EOF
            #print 'breaking'
            #print 'a'+ base_call_line + 'b'
            print('STATUS: reached end of basecall file, line of zero length')
            break # if no longer reading a line
        else:    
            flowcell = the_basecall.get_flowcell() 
            lane = the_basecall.get_lane() 
            image_number = the_basecall.get_image_num() 
            bead = the_basecall.get_bead_num() 
            flag = the_basecall.get_flag() 

            if last_image != image_number:
                last_image = image_number
                num_objects = int(((in_file_stats.readline()).split())[1])
                # pack the values into a string
                buff = struct.pack('iiiii', -1, flowcell, lane, image_number, num_objects)
                the_out_file.write(buff)
                for i in range(num_objects):
                    buff = struct.pack('hh', random.randint(0,XMAX),random.randint(0, YMAX))
                    the_out_file.write(buff)
                # end for
            # end if
        # end else
    #end while
    the_out_file.close()
    the_basecall.close()
# end def

def OTvalidate():
    """
        Tests an object table file to generate a similar basecall like text version
        assumes at least one entry in the table
    
    """
    theOT = ot.ObjectTable(base_dir+'fake_object.table')    # this opens and reads the first entry
    theValidation = open(base_dir+'validater.txt','w')
    print "validating"
    while True:
        if theOT.isEOF():
            break
        else:
            theValidation.write('Flwcll\t\t%d\n' % theOT.get_flowcell() )
            theValidation.write('Lane\t\t%d\n' % theOT.get_lane())
            theValidation.write('Image\t\t%d\n' % theOT.get_image_num())
            theValidation.write('NumObjs\t\t%d\n' % theOT.get_num_objects())
            for ind in range(theOT.get_num_objects()):
                theOT.read_point()
                theValidation.write('XY\t\t%d\t%d\n' % (theOT.get_x(),theOT.get_y()))
             #end for
        # end else
        theOT.read_entry()  # read the n+1 entry
    # end while 
    theOT.close()
    theValidation.close()
# end def
