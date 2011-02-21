#!/usr/bin/python
  
import numpy as np
from PIL import Image
import polonator.image.findObjects as FO
import polonator.illum.mapping as PM
from polonator.motion import maestro
import polonator.camera.asPhoenix as PC
import polonator.illum.D4000 as PI
import time
import sys

MapFunc = PM.MappingFunctions()
MapFunc.read_mapping_file()

MaestroF = maestro.MaestroFunctions()

#my_color = "spare"
my_color = "cy3"
LEN = 1000000
data_size = LEN#+4
MAX_BEADS_PERFRAME = 1000000

img_array = np.empty(data_size, dtype=np.uint16)
img_array2 = np.empty(data_size, dtype=np.uint16)
beadpos_xcol = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
beadpos_yrow = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
segmask_image =  np.empty(data_size, dtype=np.uint16)


MaestroF.darkfield_off()
MaestroF.filter_home()
MaestroF.filter_goto(my_color)
time.sleep(1)

MapFunc.init_illum()
MapFunc.light_all()

PC.py_snapPtr(img_array, .35,80,my_color)
#invert for images where beads are lighter than background
img_array = 16383 - img_array # invert the 14 bit image
num_beads = FO.find_objects(1000, 1000, img_array, beadpos_xcol, beadpos_yrow, segmask_image)
print("The number of beads found: %d" % num_beads)
MapFunc.convertPicPNG2(16383-img_array,0,1)
MapFunc.convertPicPNG2(segmask_image*65535, 0,2)
#MapFunc.convertPicPNG(segmask_image*65535+img_array, 0,3)



print("phase ONE complete!!!!!!!!!!!!!")
PC.cameraClose() # also frees up image buffer memory

#MapFunc.illum_bitmap()
MapFunc.illum_vector(beadpos_yrow,beadpos_xcol)

time.sleep(1)
PC.py_snapPtr(img_array2,.35,80,my_color)
MapFunc.convertPicPNG2(img_array2,0,3)

PC.cameraClose() # also frees up image buffer memory

imR = Image.open("map_0_1" +".png") # just beads
imG = Image.open("map_0_3" +".png") # the selected illuminated beads
imB = Image.open("map_0_2" +".png") # the found beads
im = Image.new('RGB', (1000,1000))
pix = im.load()
pixR = imR.load()
pixB = imB.load()
pixG = imG.load()


for i in range(1000):
    for j in range(1000):
        pix[i,j] = (pixR[i,j],0,pixB[i,j])
    # end for
# end for
im.save("map_0_4" +".png", "png")
for i in range(1000):
    for j in range(1000):
        pix[i,j] = (0,pixG[i,j],pixB[i,j])
    # end for
# end for
im.save("map_0_5" +".png", "png")
