"""
================================================================================

Polonator G.007 Selective Illuminate Software

Wyss Institute
Written by Nick Conway

mapping.py
handles coordinate-system transformations between camera
and illuminate hardware, as well as helps determine this transformation

Original version -- 07-29-2009 [DL]
Ported to python from c and extended -- 12-12-2009 [Nick Conway]
Updated and renamed 09-20-2010 [Nick Conway]

This software may be modified and re-distributed, but this header must appear
at the top of the file.

1) move to spot with global offset
2) take image
3) send image to processing
4) receive from processing the offset and list of points
5) perform release on offset
    
================================================================================
"""

    
import os
import time # for date stamping
import sys
import glob 
import numpy
import polonator.logger as PL
import polonator.illum.D4000 as PI
import polonator.camera.asPhoenix as PC
import ConfigParser
import polonator.motion.maestro as PM
# from PIL import Image
import Image # for later versions of PIL
import time

ALIGNMENT_PARAM_PADBITS = 3;
ALIGNMENT_PARAM_NUM = 1;

PolonatorPath = os.environ['POLONATOR_PATH']

class MappingFunctions:
    
    def __init__(self):
        """
        This configures the class by loading key parameters from a
        configuration file

        """
        PL.p_log("STATUS:\tMappingFunctions: initializing")

        self.config = cparser = ConfigParser.ConfigParser()
        self.config_dir = PolonatorPath + '/config_files'
        self.config_path = self.config_dir + '/camera_to_DMD_params.config'
        self.A_map = numpy.empty(10,dtype=numpy.float)
        # make sure something is in the file
        if cparser.read(self.config_path) == []:
            self.defaultConfigFile()
            cparser.read(self.config_path)
        #end if
        elif not cparser.has_section('Main'):
            print "No main"
            self.defaultConfigFile()
        # end elif
        self.IlluminateWidth = int(cparser.get( 'Main', 'IlluminateWidth'))
        self.IlluminateHeight = int(cparser.get( 'Main', 'IlluminateHeight'))
        self.CameraWidth = int(cparser.get( 'Main',  'CameraWidth'))
        self.CameraHeight = int(cparser.get( 'Main', 'CameraHeight'))
        self.ImageExposure = float(cparser.get('Main', 'ImageExposure'))
        self.ImageGain = int(cparser.get( 'Main', 'ImageGain'))
        self.ImageColor = cparser.get( 'Main', 'ImageColor')
        self.ImageFilename = cparser.get( 'Main', 'ImageFilename')
        self.ImageCCD_BytesPerPixel = int(cparser.get('Main', 'ImageCCD_BytesPerPixel'))
        self.MaestroF = PM.MaestroFunctions()
        self.mask_number0 = 0
        self.mask_radius0 = 1
        self.mask_number1 = 1
        self.mask_radius1 = 0
        self.mask_number2 = 2
        self.mask_radius2 = 1
        self.mask_number3 = 3
        self.mask_radius3 = 3
        self.expose = 0.25
        self.gain = 200
        self.illumInit()
    # end def

    def parseConfigSplitLine(self,array, field_string, field_var):
        """
        Parses a tab delimited config file that is split into an array 
        """
        ind = 0
        if (array[ind] == field_string):
            ind += 1
            field_var = array[ind]
            return True
        # end if
        else: 
            return False
        # end else
    # end def

    def mapReinit(self):
        """
        This configures the class by loading key parameters from a
        configuration file
        
        """
        # read in the config file.  must be formated correctly
        PL.p_log("STATUS:\tMappingFunctions: initializing")
        
        self.config = cparser = ConfigParser.ConfigParser()
        self.config_dir = os.environ['POLONATOR_PATH'] + '/config_files'
        self.config_path = self.config_dir + '/camera_to_DMD_params.config'

        # make sure something is in the file
        if cparser.read(self.config_path) == []:
            self.defaultConfigFile()
            cparser.read(self.config_path)
        #end if
        elif not cparser.has_section('Main'):
            print "No main"
            self.defaultConfigFile()
        # end elif
        self.IlluminateWidth = int(cparser.get( 'Main', 'IlluminateWidth'))
        self.IlluminateHeight = int(cparser.get('Main', 'IlluminateHeight'))
        self.CameraWidth = int(cparser.get( 'Main', 'CameraWidth'))
        self.CameraHeight = int(cparser.get( 'Main', 'CameraHeight'))
        self.ImageExposure = float(cparser.get( 'Main', 'ImageExposure'))
        self.ImageGain = int(cparser.get( 'Main', 'ImageGain'))
        self.ImageColor = cparser.get( 'Main', 'ImageColor')
        self.ImageFilename = cparser.get( 'Main', 'ImageFilename')
        self.ImageCCD_BytesPerPixel = int(cparser.get('Main', 'ImageCCD_BytesPerPixel'))
    # end def
        
    def generateMapping( self, \
                         points_found_x, \
                         points_found_y, \
                         points_to_illum_x, \
                         points_to_illum_y, \
                         num_coords):
        """
        takes a list of of coordinates and returns the mapping based on the
        penrose inverse

            alignment parameters are enumerated as follows:

        [illuminate_x]     [ A0 ]     [A2  A3]   [Cam_x - A6]     [ A8 ]
        [     ]         =  [    ]  +  [      ] * [          ]  +  [    ]
        [illuminate_y]     [ A1 ]     [A4  A5]   [Cam_y - A7]     [ A9 ]

        ==> R_bar = Q*A_map
        A_map = ([A0, A1, A2, A3, A4, A5]') = Q_inv*R_bar
        
        This parameters A7 to A10 have predetermined values corresponding to:

        (A6, A7) = Center of camera image in x and y, respectively
        (A8, A9) = Center of illuminate image in x and y, respectively

        The additive parameters (A0, A1, A6, A7, A8 & A9) are given in
        sub-bit resolution (defined in Release_common.h)
        The multiplicative parameters (A2, A3, A4 & A5) are in
        1 / (1 << ALIGNMENT_PARAM_PADBITS) units (see below)
        
        We need to solve for A0, A1, A2, A3, A4, and A5
        """
        A_map = self.A_map
        # load A6, A7, A8, and A9 from DMD config file
        # don't need these assignments, but adds clarity
        a8 = 0#(self.IlluminateWidth -1) /2
        a9 = 0#(self.IlluminateHeight-1)/2
        a6 = 0#(self.CameraWidth-1)/2
        a7 = 0#(self.CameraHeight-1)/2
        
        
        # Construct matrices/arrays
        # create empty array, 2x num_coords rows x 6 columns
        #this will be the states
        Q = numpy.empty([2*num_coords, 6])


        # create empty array 2x num_coords rows x 1 column
        R_bar = numpy.empty([2*num_coords, 1])

        for i in range(num_coords):
            """
                c_bar_x = measured_CCD_coords[i]['x'] - a6
                c_bar_y = measured_CCD_coords[i]['y'] - a7
                r_bar_x = illuminated_DMD_coords[i]['x'] - a8
                r_bar_y = illuminated_DMD_coords[i]['y'] - a9
            """
            c_bar_x = float(points_found_x[i] - a6)
            c_bar_y = float(points_found_y[i] - a7)
            r_bar_x = float(points_to_illum_x[i] - a8)
            r_bar_y = float(points_to_illum_y[i] - a9)
            

            Q[i,:] = [1, 0, c_bar_x, c_bar_y, 0, 0]

            # put the y coordinates in the bottom half of the matrix to
            Q[i+num_coords,:] = [0, 1, 0,0, c_bar_x, c_bar_y]  

            R_bar[i,0] = r_bar_x
            R_bar[i+num_coords,0] = r_bar_y
        #end for
        Q_inv = numpy.linalg.pinv(Q)
        print("Qinv \n")
        print(Q_inv)
        print("Qinv is: " + str(Q_inv.shape))
        print("Rbar is: " + str(R_bar.shape))
        temp = numpy.dot(Q_inv,R_bar)   # must use dot with numpy.array
        # transfer values to A_map
        for i in range(6):
            A_map[i] = temp[i][0]
        # end for
        # extend A_map with the camera and DMD parameters
        A_map[6] = a6
        A_map[7] = a7
        A_map[8] = a8
        A_map[9] = a9
        print("Amap \n")
        print(A_map)
    # end def
             
    def writeMappingFile(self):
        """
            This function writes a mapping function after it has been generated
            within the class and is in memory
            To a file
        """
        A_map = self.A_map
        # write mapping to file
        PL.p_log("Status:\tMF: writing camera to DMD map to file")
        mapping_file = open('camera_to_DMD.map', 'w')
        mapping_file.write('#This is the mapping file for the camera to DMD\n')
        mapping_file.write( '#Generated on: ' \
                            +  time.strftime("%Y-%m-%d %H:%M:%S") + '\n')
        mapping_file.write('#A0\tA1\tA2\tA3\tA4\tA5\tA6\tA7\tA8\tA9\n')
        mapping_file.write( str(A_map[0]) + \
                            '\t' + str(A_map[1]) + \
                            '\t' + str(A_map[2]) + \
                            '\t' + str(A_map[3]) + \
                            '\t' + str(A_map[4]) + \
                            '\t' + str(A_map[5]) + \
                            '\t' + str(A_map[6]) + \
                            '\t' + str(A_map[7]) + \
                            '\t' + str(A_map[8]) + \
                            '\t' + str(A_map[9]) + '\n' )
        mapping_file.close()
    # end def
    
    def readMappingFile(self): 
        """
            This reads a mapping data from file into memory.  Assumes the
            mapping file already exists
            TODO: Should add exception in the case the file does not exist
        """
        A_map = self.A_map
        PL.p_log("Status:\tMF: reading camera to DMD map to file")
        mapping_file = open('camera_to_DMD.map', 'r')
        for line in mapping_file:
            # the mapping data starts on the first line without a # to start it
            if line[0] != '#':  
                map_line = line.split()
                for i in range(len(map_line)):
                    A_map[i] = float(map_line[i]);
                    print A_map[i]
                # end for
                break;
            # end if
        # end for
        mapping_file.close()
        
        # load map into SWIGed C Polonator_IlluminateFunctions.c for faster math?
        
        
    # end def
    def ccd_to_dmd(self,x,y):
        """
        Individual point transform
        [illuminate_x]     [ A0 ]     [A2  A3]   [Cam_x - A6]     [ A8 ]
        [     ]         =  [    ]  +  [      ] * [          ]  +  [    ]
        [illuminate_y]     [ A1 ]     [A4  A5]   [Cam_y - A7]     [ A9 ]
        """
        A = self.A_map
        x_out = A[0]+A[8] \
            -A[2]*A[6]-A[3]*A[7] \
            +A[2]*x+A[3]*y
        y_out = A[1]+A[9] \
            -A[4]*A[6]-A[5]*A[7] \
            +A[4]*x+A[5]*y
        out_point = (int(x_out), int(y_out))
        #print(out_point)
        return out_point
    # end def

    def n_ccd_to_dmd(self,array_x,array_y):
        """
        Use 1-D numpy arrays to calculate transforms in parallel
        [illuminate_x]     [ A0 ]     [A2  A3]   [Cam_x - A6]     [ A8 ]
        [     ]         =  [    ]  +  [      ] * [          ]  +  [    ]
        [illuminate_y]     [ A1 ]     [A4  A5]   [Cam_y - A7]     [ A9 ]
        """
        A = self.A_map
        x_out = A[0]+A[8] \
            -A[2]*A[6]-A[3]*A[7] \
            +A[2]*array_x+A[3]*array_y
        y_out = A[1]+A[9] \
            -A[4]*A[6]-A[5]*A[7] \
            +A[4]*array_x+A[5]*array_y
        out_point = (x_out.astype(numpy.int32), y_out.astype(numpy.int32))
        return out_point
    # end def
    
    def getCameraImage(self):
        """
        Causes a camera snap event to happen and returns the pointer to a
        image copied from the frame buffer
        skips loading from a file.
        """
        return PC.snapPtr( self.ImageExposure,\
                            self.ImageGain, \
                            self.ImageColor, \
                            self.ImageFilename)
        # image has a size and byte value
    # end def

    

    
    def mapGen(self, test_me = False):
        """
        This function generates the mapping it:
        1)  loads the points to illuminate
        2)  generates a image for the DMD based on said coordinates
        3)  illuminates the image
        4) takes a picture
        5) copies the picture from the frame buffer
        6) finds the illuminated points in the camera coordinates
        7) generates a mapping to the DMD using a pseudoinverse
        8) writes the mapping to class memory
        9) writes the mapping to file
    
        Polonator_IlluminateFunctions is SWIGed as follows
        Illuminate and Hardware coordinates are mapped using the SWIG 
        This allows you to access an array of Coordinates in C and python
        as follows once the IlluminateCoords_type struct is also interfaced
        in SWIG :

        new_coordArray(int size) -- creates an array of size
        delete_coordArray(IlluminateCoords_type * arry) -- deletes and array
        coordArray_get_x(IlluminateCoords_type *c_xy, int i)  -- gets an value
        coordArray_set_x(IlluminateCoords_type *c_xy, int i, signed short
        int val) -- sets a value
        ptr_coordArray(IlluminateCoords_type *c_xy, int i)  returns a
        pointer to an index
        """
        """
        get points for bitmap and load them SWIG style
        mapping_basis is just a list of points to to illuminate to
        generate a good and sufficient mapping basis coordinates are given
        as a range from -1 to 1 with 0,0 being the image center
        """
        IW, IH = self.IlluminateWidth, self.IlluminateHeight
        print("STATUS:\tMappingFunctions: opening frame buffer\n")
        mapping_basis = open(self.config_dir +'/mapping_basis.coordinates')
        print(  "STATUS:\tMappingFunctions: getting list of points \
                to illuminate from file\n")
        idx = -1
        num_points = 0
        for line in mapping_basis:
            if line[0] != '#':   
                # '#' is reserved for comments on a separate line
                basis_coordinate = line.split()  # tab delimited fields
                print(basis_coordinate)
                if basis_coordinate[0] == 'number':  
                    # this is the number of basis points we need to illuminate
                    num_points = int(basis_coordinate[1])
                    idx = num_points 
                    # this creates the array of coordinates to illuminate
                    points_to_illum_x = numpy.empty(idx, dtype=numpy.int32)
                    # this is the array containing the found points on the CCD
                    points_found_x = numpy.empty(idx, dtype=numpy.int32)
                    points_to_illum_y = numpy.empty(idx, dtype=numpy.int32)
                    points_found_y = numpy.empty(idx, dtype=numpy.int32)
                    # write coordinates in reverse
                    idx = idx - 1
                    print("number of Points to Illuminate: " + str(num_points))
                elif (basis_coordinate[0] == 'XY') and (idx > -1):
                # make sure its tagged as a coordinate and the index is positive
                    # scale to dimensions of the DMD
                    bc_x = int( float(IW)/2 \
                                *float(basis_coordinate[1]) ) \
                                + (float(IW)-1)/2
                    bc_y = int( float(IH)/2 \
                                *float(basis_coordinate[2]) ) \
                                + (float(IH)-1)/2
                    print("read(%d,%d)\n" % (bc_x, bc_y))
                    points_to_illum_x[idx] = bc_x
                    points_to_illum_y[idx] = bc_y  
                    idx = idx - 1
                #end if
            #end if
        #end for

        """
        Set up imaging
        """
        MaestroF = self.MaestroF
        
        MaestroF.darkfield_off()
        MaestroF.filter_home()
        MaestroF.filter_goto("spare")
        time.sleep(1)
        #MaestroF.filter_unlock()
        """
        set up release hardware
        """

        #self.illumInit()
        num_sub = 0 # keeps track of not found points
        data_size = 1000000
        img_array = numpy.empty(data_size, dtype=numpy.uint16)
        #img_array_out = numpy.empty(data_size, dtype=numpy.uint16)
        img_array_float = numpy.zeros(data_size, dtype=numpy.float)
        
        for idx in range(num_points):
            # generate bitmap
            print("STATUS:\tMappingFunctions: clearing frame buffer\n")
            PI.py_clear_framebuffer()
            print("STATUS:\tMappingFunctions: creating mask\n")
            
            print("STATUS:\tMappingFunctions: generating one image of with ' + \
                    'one pixel\n")
            PI.py_illuminate_point( int(points_to_illum_x[idx]),\
                                    int(points_to_illum_y[idx]),\
                                    self.mask_number0)
            # illuminate just ONE spot
            print("STATUS:\tMappingFunctions: illuminating one point\n")
            print("illuminating (%d,%d)\n" % \
                    (int(points_to_illum_x[idx]), int(points_to_illum_y[idx])))
            PI.py_illuminate_expose()    # turn on the frame buffer
            #time.sleep(1)

            # analyze image for centroid
            
            print("STATUS:\tMappingFunctions: taking picture \
                    of one illuminated point\n")

            # take a picture and get a pointer to the picture
            frames = 5
            for i in range(frames): 
                PC.py_snapPtr(img_array, self.expose, self.gain,"spare")
                # sum accumulator
                img_array_float += img_array.astype(numpy.float) 
            # end for
            # average over frames
            img_array_out = (img_array_float/frames).astype(numpy.uint16) 
            img_array_float *= 0 # reset accumulator
            
            self.convertPicPNG(img_array_out, \
                                points_to_illum_x[idx], points_to_illum_y[idx])
            # read in the config file.  must be formated correctly
            print("STATUS:\tMappingFunctions: find illuminated point\n")

            # SWIGged function for finding the centroid
            # doing array[x:x+1] returns a pointer to an index x in an array 
            # essentially in numpy
            a = points_found_x[idx:idx+1]
            b = points_found_y[idx:idx+1]
            max_val = PI.py_illuminate_spot_find(img_array_out,a,b)
            if max_val < 100:
                print("point not found")
                num_sub += 1
            # end if
            else:
                print("found(%d,%d)" % \ 
                        (points_found_x[idx],points_found_y[idx]) )
            # end else
            
            # free up memory
            #unload_camera_image(img_array)
            PI.py_clear_framebuffer()
            PI.py_illuminate_float()
            #PC.cameraClose() # also frees up image buffer memory
        #end for
        num_points -= num_sub
        # perform mapping operation
        print("STATUS:\tMappingFunctions: generating mapping\n")
        PC.cameraClose() # also frees up image buffer memory
        self.generateMapping(points_found_x, points_found_y, \
            points_to_illum_x, points_to_illum_y, num_points)
        self.writeMappingFile()
        for i in range(num_points):
            print("illuminated(%d,%d)" % \
                    (points_to_illum_x[i],points_to_illum_y[i]) )
            print("found(%d,%d)" % (points_found_x[i],points_found_y[i]) )
            print(" ")
        # end for
        print("Finished map generation\n")
        
    # end def
    
    def snapImageAndSave(self):
        """
        A wrapper function that allows MappingFunctions.py to snap and
        save an image
        MappingFunctions hould be instantiated before this gets called by
        calling illumInit()
        
        """
        # snap an image with settings from the config file
        PC.snap(self.ImageExposure, \
                self.ImageGain, \
                self.ImageColor, \
                self.ImageFilename)
    # end def
    
    def snapImageFilename(self):
        """

        A wrapper function that allows file viewing
        """
        # snap an image with settings from the config file
        return self.ImageFilename
    # end def
    
    def snapImage(self):
        """
        A wrapper function that allows MappingFunctions.py to snap and
        save an image
        MappingFunctions should be instantiated before this gets called
        by calling illumInit()
        """
        # snap an image with settings from the config file
        return PC.snapPtr(  self.ImageExposure,\
                            self.ImageGain, \
                            self.ImageColor)
    # end def
    
    def defaultConfigFile(self, date_string = "NC 08/07/2010"):
        """
        Create a default mapping config file

        When adding sections or items, add them in the reverse order of
        how you want them to be displayed in the actual file.
        In addition, please note that using RawConfigParser's and the raw
        mode of ConfigParser's respective set functions, you can assign
        non-string values to keys internally, but will receive an error
        when attempting to write to a file or when you get it in non-raw
        mode. SafeConfigParser does not allow such assignments to
        take place.
        """
        cparser = self.config
        cparser.add_section('Main')
        cparser.set('Main', 'IlluminateWidth', 1920)
        cparser.set('Main', 'IlluminateHeight', 1080)
        cparser.set('Main', 'CameraWidth', 1000)
        cparser.set('Main', 'CameraHeight', 1000)
        cparser.set('Main', 'ImageExposure', 0.001)
        cparser.set('Main', 'ImageGain', 20)
        cparser.set('Main', 'ImageColor', 'spare')
        cparser.set('Main', 'ImageFilename', 'DMD/image.raw')
        cparser.set('Main', 'ImageCCD_BytesPerPixel', 2)

        # Writing our configuration file to 'example.cfg'
        configfile =  open(self.config_dir, 'w')
        #need to create the file
        configfile.write("# Configuration file for the camera to DMD mapping\n") 
        configfile.write("# " + date_string + "\n")
        configfile.write("#\n")
        configfile.write("# A ConfigParser file with # as a comment character\n")
        
        cparser.write(configfile)
        configfile.close()
    #end def

    def convertPicPNG(self, pixels, x,y):
        """
            snaps an image and saves the 14bit image as an 8 bit greyscale PNG
            by shifting out 6 bits
        """
        im = Image.new('L', (1000,1000))
        pix = im.load()
        for i in range(1000):
            for j in range(1000):
                pix[j,i] = PC.py_14to8bit(pixels, 1000*i+j)
            # end for
        # end for
        im.save("map_" + str(x) + "_" + str(y) + ".png", "png")
    # end def
    
    def convertPicPNG2(self, pixels, x,y):
        """
            snaps an image and saves the 14bit image as an 8 bit greyscale PNG
            by shifting out 6 bits
        """
        pix_copy1 = numpy.empty(1000000, dtype=numpy.uint8)
        #print("The size o %d\n" % pixels.size)
        #print("The size n %d\n" % pix_copy1.size)
        PC.py_14to8Image(pixels, pix_copy1);
        pix_copy2 = pix_copy1.reshape((1000,1000))
        im = Image.fromarray(pix_copy2,'L')
        im.save("map_" + str(x) + "_" + str(y) + ".png", "png")
        del pix_copy1
        del pix_copy2
    # end def 
    
    """
    Begin the Illumination interface
    This wraps the illuminate functions such that more than one module 
    calling the illumination functions will always use the same hooks so long 
    as they go through this module  
    """
    def illumInit(self):
        """
            initialize the illumination system for use in the mapping process   
        """
        print "Initializing release system\n"
        PI.py_clear_memory();
        
        # initialize module values
        if PI.py_illuminate_init(int(self.IlluminateWidth),\
                                int(self.IlluminateHeight),\
                                int(self.CameraWidth),\
                                int(self.CameraHeight)) < 0:
            sys.exit(-1)
        #end if

        # load an identity map for alignment parameters
        # CRUCIAL as for now!!!!!!!!!!!!
        PI.py_illuminate_alignment_load_identity()

        # enable release hardware
        PI.py_illuminate_enable()
        
        # Create a group of masks
        PI.py_illum_mask_radius(self.mask_radius0,self.mask_number0)
        PI.py_illum_mask_radius(self.mask_radius1,self.mask_number1)
        PI.py_illum_mask_radius(self.mask_radius2,self.mask_number2)
        PI.py_illum_mask_radius(self.mask_radius3,self.mask_number3)
    # end def
    
    def closeDMD(self):
        """
        Float the DMD and clear the frame buffer
        """
        PI.py_illuminate_float()
        PI.py_clear_framebuffer()
    #end def
    
    def enableDMD(self):
        # enable release hardware
        PI.py_illuminate_enable()
    # end def
    
    def lightAll(self):
        PI.py_light_all()
    # end def    
    
    
    def vector(self,vx,vy, num_points,spot_size=0):
        
        outpoints = self.n_ccd_to_dmd(vx,vy)
        a = outpoints[0]
        b = outpoints[1]
        PI.py_illuminate_enable()
        PI.py_clear_framebuffer()
        if spot_size == 1:
            PI.py_illuminate_vector(a,b, self.mask_number2, num_points)
        elif spot_size == 2:
            PI.py_illuminate_vector(a,b, self.mask_number3, num_points)
        else:
            PI.py_illuminate_vector(a,b, self.mask_number1, num_points)
        
        PI.py_illuminate_expose() # turn on the frame buffer
    # end def

    def pointTransform(self,x,y, check=0):
        """
            illuminates a single point while performing a mapping transform
            function exists primarily for testing

        """
        coord = self.ccd_to_dmd(x,y)
        # TODO:
        # multiplying by 4 because of confusing SUBPIXEL stuff fix in image 
        # generator
        # do all floats in FUTURE
        PI.py_illuminate_enable()
        PI.py_clear_framebuffer()
        PI.py_illuminate_point(coord[0],coord[1], self.mask_number1)
        PI.py_illuminate_expose()                   # turn on the frame buffer
        if check == 1:
            img_array = PC.snapPtr(0.35,180,"spare")
            # just clobber the illumination point
            a = numpy.array([coord[0]])
            b = numpy.array([coord[1]])
            max_val = PI.py_illuminate_spot_find(img_array, a,b)
            if max_val < 100:
                print("point not found")
            # end if
            else:
                print("found(%d,%d)" % (a[0],b[0]))
            # end else
            PC.cameraClose() # also frees up image buffer memory
        # end if
    # end def

    def bitmap(self,transform=1):
        """
        illuminates a single point while performing a mapping transform
        function exists primarily for testing

        """

        #im = Image.open("wyss_1000.bmp")
        #im = Image.open("wyss_1000_i2.bmp")
        #im = Image.open("alignment_DMD2.bmp")
        #im = Image.open("George_Church_04.bmp")
        #im = Image.open("George_Church_05.bmp")
        im = Image.open(PolonatorPath+"/data/TitlePres.bmp")
        #im = Image.open("all_white.bmp")

        pix = numpy.array(im.getdata(), dtype=numpy.uint8)

        pix = pix.reshape((1000,1000))
        #print("array created")
        itemind = numpy.where(pix==0)
        #print("points found")
        PI.py_clear_framebuffer()
        #print("framebuff cleared")
        points_illum = self.n_ccd_to_dmd(itemind[1],itemind[0])
        #print("transformed")

        PI.py_illuminate_enable()
        PI.py_illuminate_vector(points_illum[0],points_illum[1], \
                                self.mask_number1, points_illum[0].size)
        PI.py_illuminate_expose()
    # end def
    
    def wyss(self,inverse=0,decimate=0):
        """
        illuminates a single point while performing a mapping transform
        function exists primarily for testing

        """

        im = Image.open(PolonatorPath+"/data/wyss_1000_i2.bmp")

        pix = numpy.array(im.getdata(), dtype=numpy.uint8)

        # decimate it down
        if decimate > 0: 
            if inverse == 1:
                for i in range(pix.size/decimate):
                    for j in range(decimate-1):
                        pix[decimate*i+j+1] = 0
                    # end for
                # end for
            # end if
            else:
                for i in range(pix.size/decimate):
                    for j in range(decimate-1):
                        pix[decimate*i+j+1] = 1
                    # end for
                # end for
            # end else
        # end if
        pix = pix.reshape((1000,1000))

        itemind = numpy.where(pix==inverse)

        PI.py_clear_framebuffer()

        points_illum = self.n_ccd_to_dmd(itemind[1],itemind[0])

        PI.py_illuminate_enable()
        PI.py_illuminate_vector(points_illum[0],points_illum[1], \
                                self.mask_number1, points_illum[0].size)
        PI.py_illuminate_expose()
    # end def
    
    def george(self,inverse=0,decimate=0):
        """
        illuminates a single point while performing a mapping transform
        function exists primarily for testing

        """

        im = Image.open(PolonatorPath+"/data/George_Church_05.bmp")

        pix = numpy.array(im.getdata(), dtype=numpy.uint8)

        pix = pix.reshape((1000,1000))

        itemind = numpy.where(pix==inverse)

        PI.py_clear_framebuffer()

        points_illum = self.n_ccd_to_dmd(itemind[1],itemind[0])

        PI.py_illuminate_enable()
        PI.py_illuminate_vector(points_illum[0],points_illum[1], \ 
                                self.mask_number1, points_illum[0].size)
        PI.py_illuminate_expose()
    # end def    
    
    def grid(self,grid_spacing=10, square_size=1):
        """
        illuminates a single point while performing a mapping transform
        function exists primarily for testing

        """

        pix = numpy.zeros((1000,1000), dtype=numpy.uint8)
        period = grid_spacing+square_size
        for i in range(1000/period):
            for j in range(1000/period):
                for p in range(square_size):
                    for q in range(square_size):
                        pix[period*i+p,period*j+q] = 1
                    # end for
                #end for
            # end for
        # end for

        itemind = numpy.where(pix==1)

        PI.py_clear_framebuffer()
        
        points_illum = self.n_ccd_to_dmd(itemind[1],itemind[0])

        PI.py_illuminate_enable()
        PI.py_illuminate_vector(points_illum[0],points_illum[1], \
                                self.mask_number1, points_illum[0].size)
        PI.py_illuminate_expose()
    # end def
