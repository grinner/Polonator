
"""
Nick Conway Wyss Institute  02-02-2010

Parses a basecall file

The basecall file is parsed as follows since it is a tab delimited text 
file
FLOWCELL#    LANE#      IMAGE#    BEAD#    N    Xtab        FLAG
0            1            2        3                        length-1

one basecall file per lane

N is the read sequence of a bead containing X bases
Xtab indicates that X tabbed fields exist for a given read

the number of beads flagged per lane in a given basecall file is in the 
*.hit_summary file
in a tab delimited fashion

Since each basecall file is for a given lane of a given flowcell, 
searching is simple
"""

class BaseCall:
    def __init__(self,filename):
        """
        initializes the class opens the file and 
        loads the first entry of the class
        
        expects the file "filename" to be tab delimited
        """
        self.file = open(filename, 'r')
        
        # the current entry read from the basecall file
        self._entry = 0
        
        self._EOF = False
        
        # read the first entry
        self.read_entry()
        
        # flag_index is the index of the last field in a line
        self._flag_index = len(self._entry)-1
        
        # 
        self._base_count = self._flag_index-6+1 # 6 fields minimum
    #end def
    
    def open(self,filename):
        """
        Open the basecall file and get the file descriptor
        """ 
        self.file = open(filename, 'r')
    # end def
    
    def close(self):
        """
        Close the basecall file descriptor pointer
        """ 
        self.file.close()
    #end def
    
    def base_count(self):
        """
        returns the number of bases read per bead in a basecall file
        this is a reflection on the line length
        """
        return self._base_count
    # end def
            
    def read_entry(self):   
        """
        return a tuple of data from an entry in the basecall file 
        loads data into the class variable
        """
        basecall_line = self.file.readline()
        # split the line by fields
        basecall_columns = basecall_line.split()
        self._EOF = not (len(basecall_columns))
        if (self._EOF):   # breaks the loop when a line length is zero 
                         # indicating EOF
            #print 'breaking'
            #print 'a'+ base_call_line + 'b'
            print('STATUS: reached end of basescall file, line of zero length')
            self._entry =  [] # if no longer reading a line
        # end if
        else:
            
            self._entry = basecall_columns
        # end else
    # end def
    
    def isEOF(self):
        return self._EOF
    #end def
        
    def get_entry(self):
        return self._entry
    # end def
    
    def get_flowcell(self):
        return int(self._entry[0])
    #end
    
    def get_lane(self):
        return int(self._entry[1])
    # end def
    
    def get_image_num(self):
        return int(self._entry[2])
    # end def
    
    def get_bead_num(self):
        return int(self._entry[3])
    # end def
    
    def get_flag(self):
        """
        Return 1 if flagged
        0 otherwise
        
        A line is flagged as a "1" if the bead is to be released or a match
        in some sense.
        
        Return the flag of current entry      
        """
        return int(self._entry[self._flag_index])
    # end def
    
    def seek_image(self,image_num):
        """
        Read lines until we get a line that has something in it
        
        This is a little weak as far as exceptions go
        """
        while True:
            self.read_entry()
            if (len(self._entry) == 0):
                break
            # end if
            elif self.get_image_num() == image_num:
                break
            # end elif
        # end while
    # end def
