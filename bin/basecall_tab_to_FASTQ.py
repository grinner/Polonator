import struct
import random
import glob
"""
        Nick Conway Wyss Institute  02-02-2010
        Parses a Polonator table file and makes a FASTQ file.   TODO: Actually make this work
        
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

XMAX = 1000 # camera max resolution in X
YMAX = 1000 # camera max resolution in Y

# now look up the base call file
#polonator_data_dir = '/home/polonator/G.007/G.007_acquisition/'
polonator_data_dir = 'Z:/polonator/code/polonator_dmd_C/src-nick/'
base_dir = polonator_data_dir + 'output_data/'
the_in_files = glob.glob(base_dir + '*.basecalls') # there is a basecall file for each lane?
the_hit_sum_files = glob.glob(base_dir + '*.hit_summary')

the_in_file = open(the_in_files[0],'r')

the_out_file = open(base_dir+'fake_object.table', 'wb')

struct.calcsize('i')    # i should be 4 bytes
struct.calcsize('h')    # h should be 2 bytes 

def BcToFASTQ(filename_in, hit_sum_file_in, filename_out):
    """
        @SEQ-ID:[LANE]:[IMAGE_NUM]:[X_COORD]:[Y_COORD]#[NNNNN][/member of a pair]
        LANE = flowcell lane
        IMAGE_NUM = image number of a tile within a given flowcell lane
        X_COORD = X coordinate of an image in pixels
        Y_COORD = Y coordinate of an image in pixels
        NNNNNN = the multiplexed tag
        /member of pair = /1 or /2 for paired end or mate pair reads only
    """
    print "starting"
    last_image = -1
    
    the_in_file = open(filename_in,'r')     # our basecall file
    the_out_file = open(filename_out, 'w')  # the FASTQ file
    in_file_stats = open(hit_sum_file_in, 'r') 
    while True:
        base_call_line = the_in_file.readline()
        base_call_columns = base_call_line.split()
        #if ((base_call_line == '\n') or (base_call_line == 0)):
        flag_index = len(base_call_columns)
        if (flag_index == 0):   # breaks the loop when a line length is zero indicating EOF
            #print 'breaking'
            #print 'a'+ base_call_line + 'b'
            print('STATUS: reached end of basecall file, line of zero length')
            break # if no longer reading a line
        else:    
            #base_call_columns = base_call_line.split()
            #print base_call_columns
            flowcell = int(base_call_columns[0]) 
            lane = int(base_call_columns[1])
            image_number = int(base_call_columns[2])
            bead = int(base_call_columns[3])
            flag = int(base_call_columns[flag_index-1])

            
            
            
            if last_image != image_number:
                last_image = image_number
                num_objects = int(((in_file_stats.readline()).split())[1])
                #print num_objects
                # pack the values into a string
                buff = 
                the_out_file.write(buff)
                for i in range(num_objects):
                    buff = struct.pack('hh', random.randint(0,XMAX),random.randint(0, YMAX))
                    the_out_file.write(buff)
                # end for
            # end if
        # end else
    #end while
    the_out_file.close()
    the_in_file.close()
# end def

def OTvalidate():
    """
        Tests an object table file to generate a similar basecall like text version
    
    """
    theOT = open(base_dir+'fake_object.table','rb')
    theValidation = open(base_dir+'validater.txt','w')
    while True:
        raw = theOT.read(4)
        if len(raw) == 0:
            break
        else:
            val = struct.unpack('i',raw)
            if (int(val[0]) == -1):
                #print 'something works!'
                raw = theOT.read(16)
                val = struct.unpack('iiii',raw)
                #print val
                theValidation.write('Flwcll\t\t%d\n' % val[0] )
                theValidation.write('Lane\t\t%d\n' % val[1])
                theValidation.write('Image\t\t%d\n' % val[2])
                theValidation.write('NumObjs\t\t%d\n' % val[3])
                for ind in range(val[3]):
                    raw = theOT.read(4)
                    val = struct.unpack('hh',raw)
                    #theValidation.write('XY\t\t'+str(val[0])+'\t'+str(val[1])+'\n')
                    theValidation.write('XY\t\t%d\t%d\n' % (val[0],val[1]))
                #end for
            #end if
        # end else
    # end while 
    theOT.close()
    theValidation.close()
# end def
