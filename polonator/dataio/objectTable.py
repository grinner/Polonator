
"""
Nick Conway Wyss Institute  02-02-2010

This parses a Polonator G.007 object table file
      
The object table is parsed as follows since it is a binary file
        
FIELD            VALUE                    SIZE       DESCRIPTION
start            -1                       4 bytes    start of next block 
                                                     (1 block per image)
flowcell_id      {0..FCs_PER_RUN - 1}     4 bytes    the number of flowcells 
                                                     currently on the Polonator 
                                                     (usually 1 or 2)
array_id         {0..ARRAYS_PER_FC - 1}   4 bytes    the number of arrays the 
                                                     Polonator will image per 
                                                     flowcell (usually 18)
image_id         {0..IMGS_PER_ARRAY - 1}  4 bytes    the number of images per 
                                                     array the Polonator will 
                                                     acquire (usually 2514)
num_objects      {0..65535}               4 bytes    maximum number of objects 
                                                     per frame is largest 
                                                     allowable short unsigned int
beadpos_xcol     {0..NUM_XCOLS - 1}       2 bytes    X image position of 
                                                     centroid for current bead
beadpos_yrow     {0..NUM_YROWS - 1}       2 bytes    Y image position of 
                                                     centroid for current bead

TODO add skip lane and skip flowcell functions
"""

import struct   # struct is used for unpacking binary data from a file

class ObjectTable:
    def __init__(self,filename):
        """
        initializes the class opens the file and 
        loads the first entry of the class
        """
        self._file = self.open(filename)
        self._entry = 0
        self._point = 0
        self._EOF = False
        self.read_entry()
        self._flag_index = len(self._entry) - 1
        self._base_count = self._flag_index-6 + 1   # 6 fields minimum
        
        """
        this is reset every for every image read and keeps track of the 
        number of objects ('num_objects' above) in an image to make sure 
        we read the appropriate number of bytes.
        a bead number index of the point currently read into 
        the class variable self._point
        """
        self._bead_num = -1;                        
        self._lane_max = 7                       # index of maximum lane number
        self._image_num_max = 2180               # maximum image tile index
    #end def
    
    def open(self,filename): 
        return open(filename, 'rb')
    # end def
    
    def close(self):
         self._file.close()
        #end def
            
    def read_entry(self):   
        """
        return a tuple of data from an entry in the object table binary file 
        loads header data into the class variable
        """
        raw = self._file.read(4)
        self._EOF = not len(raw)
        if self._EOF:   # if EOF
            self._entry = []
            print('STATUS: reached end of object table file, line of zero length')
            return 0
        # end if
        else:
            # use the struct module to unpack the binary data according to 
            # the file type listed above
            val = struct.unpack('i',raw)
            if (int(val[0]) == -1): 
                raw = self._file.read(16)
                self._entry = struct.unpack('iiii',raw)
                self._bead_num = -1;     # reset bead number index
            # end if
            else:
              print('ERROR: objectTable: expected header start of -1')  
              return 0
        # end else
    # end def
    
    def read_point(self):
        """
        no EOF checking is this function since points should match the 
        number of objects
        assumes a header has just been read to save the operation
        """
        raw = self._file.read(4)
        self._point = struct.unpack('hh',raw)
        self._bead_num += 1
    # end def
    
    def get_x(self):
        """
        Get the x pixel location of the object location of the entry
        """
        return self._point[0]
    # end def
    
    def get_y(self):
        """
        Get the y pixel location of the object location of the entry
        """
        return self._point[1]
    # end def
    
    def isEOF(self):
        return self._EOF
    #end def
        
    
    def get_entry(self):
        """
        Return an entire 
        """
        return self._entry
    # end def
    
    def get_flowcell(self):
        """
        Return the integer Flowcell number of the entry, 0 or 1 for the G.007
        """
        return int(self._entry[0])
    #end
    
    def get_lane(self):
        """
        Return the integer Flowcell number of the entry, 0 to 7 for the G.007
        """
        return int(self._entry[1])
    # end def
    
    def get_image_num(self):
        """
        Return the integer image number of the entry, 0 to 2179 for the G.007
        """
        return int(self._entry[2])
    # end def
    
    def get_num_objects(self):
        """
        Return the integer number of objects in the image of the entry, 
        0 to 65535 for the G.007
        """
        return int(self._entry[3])
    # end def
    
    def get_bead_num(self):
        return self._bead_num
    #end def
        
    def seek_bead_num(self, bead):
        """ 
        assumes the seek is in the same image and that the header for the 
        current image has been read
        does not read the point, just puts the pointer to right before the 
        entry desired
        """
        
        self._file.seek( 4*(bead - self._bead_num - 1) )
        self._bead_num = bead - self._bead_num - 1
    # end def
    
    def skip_to_next_image(self):
        """ 
        assumes that the header for the current image has been read
        does not read the entry, just puts the pointer to right before the 
        next entry desired
        """
        self.seek_bead_num(self.get_num_objects())
    # end def

    def skip_N_images(self,N):
        """
        skip to the end of least the current image for N = 1
        """
        self.skip_to_next_image()       # current image
        ind = N - 1
        if (ind > 0):
            for i in range(ind):
                self.read_entry()
                if self._EOF:
                    print("EOF hit not enough images")
                    break
                # end if
                self.skip_to_next_image() 
            # end for
        # end if
    # end def
    
    def seek_image(self,image_num):
        """ 
        assumes a header has been read
        no checking occurs to verify the object table is in order or has 
        no errors
        """
        seek_num = image_num - self.get_image_num()
        self.skip_N_images(seek_num)

    # end def
    
    def skip_to_next_lane(self):
        """
        Skips to the end of the last image data set of the current lane
        It does so by seeking an image +1 out of range of the maximum
        image count of a lane
        """
        self.seek_image(self._image_num_max + 1)
    # end def
    
    def skip_to_lane_N(self,N):
        """
         skips at least to the end of the current lane
         need to read_entry in order to get the next image data
        """
        ind = N - self.get_lane()
        if ind > 0:
            for i in range(ind):
                self.skip_to_next_lane()
            # end for
        #end if
    # end def
    
    def skip_to_next_flowcell(self):
        """
        Assumes a maximum count of two flowcells in a polonator
        It does so by seeking a lane +1 out of range of the maximum
        lane count of a flowcell
        """
        self.skip_to_lane_N(self._lane_max + 1)
        
# end class
        
        
        
