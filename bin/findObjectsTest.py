#!/usr/bin/python
"""
    The Python Imaging Library (PIL) is

    Copyright © 1997-2006 by Secret Labs AB
    Copyright © 1995-2006 by Fredrik Lundh 
"""  
import numpy as np
from PIL import Image
import polonator.image.findObjects as FO
LEN = 1000000
data_size = LEN#+4
MAX_BEADS_PERFRAME = 1000000
#inputBuffer = np.empty(data_size, dtype=np.uint16)
#image = np.empty(data_size, dtype=np.uint16)
beadpos_xcol = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
beadpos_yrow = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
segmask_image =  np.empty(data_size, dtype=np.uint16)

im = np.fromfile("09162009D_00_1000.raw", dtype=np.uint16)
# the file format is a 6MB file on uint16s so 
# 1st 2MB is raw image
# 2nd 2MB is flattened image
# 3rd 2MB is binary object mask
raw_image = im[0:1000000]
binary_object_mask = im[2000000:3000000]
print("The size is %d" % binary_object_mask.size)
objects_found_old = np.where(binary_object_mask==1)[0].size
print("Chao found %d objects" % objects_found_old)
num_beads = FO.find_objects(1000, 1000, raw_image, beadpos_xcol, beadpos_yrow, segmask_image)
print("I found %d objects" % num_beads)

binary_object_mask = binary_object_mask*65535
segmask_image = segmask_image*65535
binary_object_mask = binary_object_mask.astype(np.uint8)
segmask_image = segmask_image.astype(np.uint8)

imChao = Image.fromarray(np.reshape(binary_object_mask, (1000, 1000) ))
imNick = Image.fromarray(np.reshape(segmask_image, (1000, 1000) ))

imChao.show(command="display")
imNick.show(command="display")
#imChao.show(command="eog")
#imNick.show(command="eog")
