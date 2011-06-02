# The MIT License
# 
# Copyright (c) 2010 Wyss Institute
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
# http://www.opensource.org/licenses/mit-license.php

"""
16bitimageviewer.py

Created by Roger Conturie and Nick Conway on 2010-10-09.
"""

import os, sys, math, numpy, ui_16bitimagewindow #,png, itertools
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import QtCore, QtGui, QtOpenGL
import matplotlib.image as mpimg
import matplotlib.pyplot as plt

global lastcircle
lastcircle = []
global lastline
lastline = None
global compinstanceL
compinstanceL = []
global zoom
zoom = "+"
global mag
mag = False
"""
Many global variables still exist for coding simplicity, they will be removed in the future once all object relationships are finalized
"""

class STBimageviewer(QMainWindow, ui_16bitimagewindow.Ui_MainWindow):
    def __init__(self, parent=None):
        super(STBimageviewer, self).__init__(parent)
        self.setupUi(self)
        self.view = self.graphicsView
        self.scene = QGraphicsScene()
        self.view.setScene(self.scene)
        self.comboBox = self.comboBox
        
        self.compositeview1 = self.graphicsView_2
        self.compositescene1 = QGraphicsScene()
        self.compositeview1.setScene(self.compositescene1)

        self.compositeview2 = self.graphicsView_3
        self.compositescene2 = QGraphicsScene()
        self.compositeview2.setScene(self.compositescene2)

        self.compositeview3 = self.graphicsView_4
        self.compositescene3 = QGraphicsScene()
        self.compositeview3.setScene(self.compositescene3)

        self.compositeview4 = self.graphicsView_5
        self.compositescene4 = QGraphicsScene()
        self.compositeview4.setScene(self.compositescene4)
        
        self.process = None
        self.videoview = self.VideoStreamWindow
        self.videoscene = QGraphicsScene()
        self.videoview.setScene(self.videoscene)
        
        self.count = 0
        self.path = QDir.homePath()
        self.connect(self.actionOpen, SIGNAL("triggered()"), self.OpenClicked)
        
        self.selectedbrowser = self.SelectedTextBrowser
        self.livebrowser = self.LivetextBrowser

        self.pic = None
        
        self.C1Def = self.C1Def
        self.C2Def = self.C2Def
        self.C3Def = self.C3Def
        self.C4Def = self.C4Def
        self.path1 = None
        self.path2 = None
        self.path3 = None
        self.path4 = None
        
        #self.dotpermission = False
        self.Image = None
        global imageviewer
        imageviewer = self
        global dotpermission
        dotpermission = False

        self.connect(self.comboBox, SIGNAL( "currentIndexChanged(int)" ), self.comboChange)
    
  #  def vstream(self):
   #     freq = 100
   #     while (freq > 0):
   #         print freq
   #         freq = freq - 1
    def on_pushButton_released(self):
        if self.process == None:
#            currentdir = str(os.getcwd())
            cmd = "python " + "videostream.py"#"videostream.py " + str(whilelength) #+ currentdir.replace("\\" , "/") 
    #    cmd = "python " + cmd.replace("C:", "")
            self.process_start(cmd)
        else:
            self.process.kill()
            print "killing process"
            self.process = None
        
     #   print cmd
      #  os.system(cmd)
        #vprocess = QProcess()
        #vprocess.start(cmd)

    def process_start(self, cmd):
        self.process = QProcess()
        self.process.start(cmd)


        self.connect(self.process, SIGNAL("readyRead()"), self.process_readyRead)
        self.connect(self.process, SIGNAL("finished(int)"), self.process_finished)
        
    def process_readyRead(self):

        self.count = self.count + 1
        try:
            out = self.process.readAll() #readAllStandardOutput()
            self.pic = numpy.fromstring(out, dtype = numpy.uint8)
            #print self.pic
            if len(self.pic) == 1000000:
                self.pic.resize((1000, 1000))
                stream = VideoStream(self.pic, self.count)
                
                self.videoscene.clear()
                self.videoscene.addItem(stream)
            
        except:
            print "Empty channel"
           # print self.pic[0]
    #     self.process_start(cmd, self.polonatorTextArea, ['pass'], "self.process_pass()") 

    def process_finished(self):
        print "FINISHED!"
        self.count = 0
        
    def on_distancebutton_released(self):

        ##########################################
        ##########################################
        ###         ####   ####    #####    ######
        ####### ###### #### ### ### ### #### #####
        ####### ###### #### ### #### ## #### #####
        ####### ###### #### ### #### ## #### #####
        ####### ###### #### ### ### ### #### #####
        ####### #######    ####    #####    ######
        ##########################################
        ##########################################
        print "To Do: Write Distance Function"
    
    def on_SNRbutton_released(self):
        pathfour = "C:/Users/Roger Conturie/Desktop/Image Viewer Resources/rawimages/testImageDMD.raw"
        shape = (1000,1000)
        image_file = open(pathfour)
        # load a 1000000 length array
        image_array_1D5 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
        image_file.close()
        image_array_2D5 = image_array_1D5.reshape(shape)
        image_8bit = (image_array_2D5 >> 6)
        

        fourierarray = image_array_1D5
        fourier = numpy.fft.fft(fourierarray, 1000000)
        t = numpy.arange(1000000)
        sp = fourier
        freq = numpy.fft.fftfreq(t.shape[-1])
       # print max(sp.real)
        #print len(freq)
        plt.plot(freq, abs(sp.real))#, freq, sp.imag)
        plt.show()

        
        ##########################################
        ##########################################
        ###         ####   ####    #####    ######
        ####### ###### #### ### ### ### #### #####
        ####### ###### #### ### #### ## #### #####
        ####### ###### #### ### #### ## #### #####
        ####### ###### #### ### ### ### #### #####
        ####### #######    ####    #####    ######
        ##########################################
        ##########################################
        print "To Do: Write SNR Function"
    
    def on_MagnifyPushButton_released(self):
        global mag
        if mag == False:
            self.MagnifyPushButton.setText("Turn Off Magnifying Glass")
            mag = True
            return
        if mag == True:
            self.MagnifyPushButton.setText("Turn On Magnifying Glass")
            mag = False
            return
        
    def comboChange(self):
        if str(self.comboBox.currentText()) == "":
            self.ClearButton.setEnabled(False)
        else:
            self.ClearButton.setEnabled(True)
            
    def on_ClearButton_released(self):
        self.scene.clear()
        if self.comboBox.currentText() == "All":
            self.loadinit("A", "Clear")
        if self.comboBox.currentText() == "3 Channels":
            self.loadinit("3C", "Clear")
        if self.comboBox.currentText() == "Channel 1":
            self.loadinit("1", "Clear")
        if self.comboBox.currentText() == "Channel 2":
            self.loadinit("2", "Clear")
        if self.comboBox.currentText() == "Channel 3":
            self.loadinit("3", "Clear")
        if self.comboBox.currentText() == "Channel 4":
            self.loadinit("4", "Clear")
                     
        
    def keyPressEvent(self, event):
        if event.key() == 16777248:
            global zoom
            zoom = "-"
    def keyReleaseEvent(self, event):
        if event.key() == 16777248:
            global zoom
            zoom = "+"
            
    def on_LoadComposite_released(self):
        if self.comboBox.currentText() == "All":
            self.loadinit("A", "Load")
        if self.comboBox.currentText() == "3 Channels":
            self.loadinit("3C", "Load")
        if self.comboBox.currentText() == "Channel 1":
            self.loadinit("1", "Load")
        if self.comboBox.currentText() == "Channel 2":
            self.loadinit("2", "Load")
        if self.comboBox.currentText() == "Channel 3":
            self.loadinit("3", "Load")
        if self.comboBox.currentText() == "Channel 4":
            self.loadinit("4", "Load")
        
    def loadinit(self, type, mode):
        if mode == "Load":
            if type == "1" or type == "A" or type == "3C":
                if self.C1Def.isChecked() == True:
                    self.path1 = "C:/Users/Roger Conturie/Desktop/Image Viewer Resources/rawimages/colorsnap-txred_after.raw"
                else:
                    self.path1 = QFileDialog.getOpenFileName(self,
                            "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg
                self.c1label.setText(str(self.path1.split("/")[-1]))
            
            if type == "2" or type == "A" or type == "3C":
                if self.C2Def.isChecked() == True:
                    self.path2 = "C:/Users/Roger Conturie/Desktop/Image Viewer Resources/rawimages/colorsnap-cy5_after.raw"
                else:     
                    self.path2 = QFileDialog.getOpenFileName(self,
                            "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg
                self.c2label.setText(str(self.path2.split("/")[-1]))
            
            if type == "3" or type == "A" or type == "3C":
                if self.C3Def.isChecked() == True:
                    self.path3 = "C:/Users/Roger Conturie/Desktop/Image Viewer Resources/rawimages/colorsnap-cy3_after.raw"
                else:
                    self.path3 = QFileDialog.getOpenFileName(self,
                            "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg
                self.c3label.setText(str(self.path3.split("/")[-1]))
            
            if type == "4" or type == "A":
                if self.C4Def.isChecked() == True:
                    self.path4 = "C:/Users/Roger Conturie/Desktop/Image Viewer Resources/rawimages/testImageDMD.raw"
                else:
                    self.path4 = QFileDialog.getOpenFileName(self,
                            "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg
                self.c4label.setText(str(self.path4.split("/")[-1]))

        if mode == "Clear":
            if type == '1' or type == 'A' or type == '3C':
                self.path1 = None
                self.c1label.setText('')
            if type == '2' or type == 'A' or type == '3C':
                self.path2 = None
                self.c2label.setText('')
            if type == '3' or type == 'A' or type == '3C':
                self.path3 = None
                self.c3label.setText('')
            if type == '4' or type == 'A':
                self.path4 = None
                self.c4label.setText('')

        self.ImageGenerator(self.path1, self.path2, self.path3, self.path4)

    def on_DotOn_released(self):
        global dotpermission
        dotpermission = True
        self.DotOff.setEnabled(True)
        self.DotOn.setEnabled(False)

    def on_DotOff_released(self):
        global dotpermission
        dotpermission = False
        self.DotOff.setEnabled(False)
        self.DotOn.setEnabled(True)

    def on_ClearDots_released(self):
        imageitems = self.scene.items()
        for item in imageitems:
            if str(item.__class__.__name__) == "DistanceLine" or str(item.__class__.__name__) == "CircleMarker":
                self.scene.removeItem(item)
        self.scene.update()
        global lastline
        lastline = None
        global lastcircle
        lastcircle = []
        
    def on_SearchButton_released(self):
        x = self.xedit.text()
        y = self.yedit.text()
        
        itemlist = self.view.items()
        for item in itemlist:
            if str(item.__class__.__name__) == "GraphicsItem":
                item.searchExecuted(x, y)

    def on_SearchButton2_released(self):
        x = self.xedit2.text()
        y = self.yedit2.text()
        itemlist = self.compositeview1.items() + self.compositeview2.items() + self.compositeview3.items() + self.compositeview4.items()
        if x == '' or y == '':
            pass
        else:
            for item in itemlist:
                if str(item.__class__.__name__) == "QuadView":
                    try:
                        item.searchExecuted(x, y)
                    except:
                        print "Invalid Type"
                        break
    
    def OpenClicked(self):

        path = QFileDialog.getOpenFileName(self,
            "Open File", self.path , str("Images (*raw *png)"))  #*.png *.xpm *.jpg
        
        ftype = str(path.split(".")[-1])        
        
        if ftype == "raw":
            self.ImageGenerator(path, None, None)
      #  image_file = open(path)

        if ftype == "png":
            #import png as numpy array
            Image = mpimg.imread(str(path))
            #scale pixel values which are originally between 0 and 1 so that their values fall between 0 and 255 
            scaledimage = 255*Image
            #reformat pixel data type from float to numpy.uint8
            scaled_num_array = numpy.array(scaledimage,dtype=numpy.uint8)

            shape = (scaled_num_array.shape[0], scaled_num_array.shape[1])
            channel1 = scaled_num_array[:,:,2]
            channel2 = scaled_num_array[:,:,1]
            channel3 = scaled_num_array[:,:,0]
            alpha_array = 255*(numpy.ones(shape, dtype=numpy.uint8))

            # apparently alpha gets stacked last. load BGRA because the byte order in memory for each
            # channel is stored in memory as BGRA 0xBBGGRRAA by little-endian CPU's such as intel processors
            # for future overlay work do 
            # example>>>>> image_ARGB = numpy.dstack([image_Blue,image_Green,image_Red,alpha_array])
            image_ARGB_3D = numpy.dstack([channel1,channel2,channel3,alpha_array])

            # reshape to a 2D array that has 4 bytes per pixel
            image_ARGB_2D = numpy.reshape(image_ARGB_3D,(-1,scaled_num_array.shape[1]*4)) # reshape does not copy, only manipulates data structure holding the data
            #Numpy buffer QImage declaration
            Image = QImage(image_ARGB_2D.data, scaled_num_array.shape[1], scaled_num_array.shape[0], QImage.Format_ARGB32)
            Image.ndarray = image_ARGB_2D  # necessary to create a persistant reference to the data per QImage class
            target = QRectF(0, 0, Image.width(), Image.height())
            source = QRectF(0, 0, Image.width(), Image.height())
            #graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, image_8bit)
            graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, self.label_15, channel1) 
            self.scene.clear()
            self.scene.addItem(graphicitem)
            self.Image = Image
              #  image_array = numpy.array(Image)

    def ImageGenerator(self, path1, path2, path3, path4):
        width = 1000
        height = 1000
        shape = (width, height)
        alpha_array = 255*(numpy.ones(shape, dtype=numpy.uint8))
        zero_array = numpy.zeros(shape, dtype=numpy.uint8)
        
        Red = zero_array
        Blue = zero_array
        Green = zero_array
        Yellow = zero_array
        Cyan = zero_array
        Magenta = zero_array
        White = zero_array
    #    if path1 is not None and path2 is None and path3 is None:
     #       path2 = path1
      #      path3 = path1
        #Red
        
        image_array_2D1 = zero_array
        
        #Blue
        try:
            image_file = open(path1)
            # load a 1000000 length array
            image_array_1D1 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
            image_file.close()
            image_array_2D1 = image_array_1D1.reshape(shape)
            image_8bit1 = (image_array_2D1 >> 6)

        except:
            image_8bit1 = 0*alpha_array
        
        #Green
        try:
            image_file = open(path2)
            # load a 1000000 length array
            image_array_1D2 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
            image_file.close()
            image_array_2D2 = image_array_1D2.reshape(shape)
            image_8bit2 = (image_array_2D2 >> 6)
        except:
            image_8bit2 = 0*alpha_array         
        
        try:
            image_file = open(path3)
            image_array_1D3 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
            image_file.close()
            image_array_2D3 = image_array_1D3.reshape(shape)
            image_8bit3 = (image_array_2D3 >> 6)
        except:
            image_8bit3 = 0*alpha_array
            image_array_2D3 = alpha_array.reshape(shape)
        
            
        #George
 #       if self.channel1combobox.currentText() == "Red":

        try:
            image_file = open(path4)
            # load a 1000000 length array
            image_array_1D4 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
            image_file.close()
            image_array_2D4 = image_array_1D4.reshape(shape)
            image_8bit4 = (image_array_2D4 >> 6)
        except:
            image_8bit4 = 0*alpha_array


        channelcombolist = [self.channel1combobox, self.channel2combobox, self.channel3combobox, self.channel4combobox]
        imagelist = [image_8bit1, image_8bit2, image_8bit3, image_8bit4]
        for item in range(4):
            if channelcombolist[item].currentText() == "Red":
                Red = Red + imagelist[item]
            elif channelcombolist[item].currentText() == "Blue":
                Blue = Blue + imagelist[item]
            elif channelcombolist[item].currentText() == "Green":
                Green = Green + imagelist[item]
            elif channelcombolist[item].currentText() == "Yellow":
                Yellow = Yellow + imagelist[item]
            elif channelcombolist[item].currentText() == "Cyan":
                Cyan = Cyan + imagelist[item]
            elif channelcombolist[item].currentText() == "Magenta":
                Magenta = Magenta + imagelist[item]
            elif channelcombolist[item].currentText() == "White":
                White = White + imagelist[item]
        
        # apparently alpha gets stacked last. load BGRA because the byte order in memory for each
        # channel is stored in memory as BGRA 0xBBGGRRAA by little-endian CPU's such as intel processors
        # for future overlay work do 
        # example>>>>> image_ARGB = numpy.dstack([image_Blue,image_Green,image_Red,alpha_array])
#        max = numpy.max(image_8bit4)
        
        
                    
        
   #     test = (Yellow + Magenta).clip(min=None,max=255).astype(numpy.uint8)
    #    for i in range(1000):
     #       for j in range(1000):
      #          if Yellow[i, j] + Magenta[i, j] > 255:
       #             print test[i, j]
      #  min = numpy.min(image_8bit4)
        image_ARGB_3D = numpy.dstack([(Blue + Cyan + Magenta + White).clip(0,255).astype(numpy.uint8),(Green + Cyan + Yellow + White).clip(0,255).astype(numpy.uint8), (Red + Magenta + Yellow + White).clip(0,255).astype(numpy.uint8), alpha_array])
 #       image_ARGB_3D = numpy.dstack([(image_8bit4).clip(min=None,max=255).astype(numpy.uint8),(image_8bit4).clip(min=None,max=255).astype(numpy.uint8), (zero_array).clip(min=None,max=255).astype(numpy.uint8), alpha_array])
        
        # reshape to a 2D array that has 4 bytes per pixel
        image_ARGB_2D = numpy.reshape(image_ARGB_3D,(-1,width*4)) # reshape does not copy, only manipulates data structure holding the data
 
        #Numpy buffer QImage declaration
        Image = QImage(image_ARGB_2D.data, width, height, QImage.Format_ARGB32)
        Image.ndarray = image_ARGB_2D  # necessary to create a persistant reference to the data per QImage class
        target = QRectF(0, 0, Image.width(), Image.height())
        source = QRectF(0, 0, Image.width(), Image.height())
    
        #graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, image_8bit)
        graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, self.label_15, image_array_2D1) 
        self.scene.clear()
        self.scene.addItem(graphicitem)
        self.Image = Image

        global compinstanceL
        compinstanceL = []
        
        QV1 = QuadView(image_8bit1, zero_array, alpha_array, self.compositeview1, self.label11, self.SelectedTextBrowser_2, self.LivetextBrowser_2)
        self.compositescene1.clear()
        self.compositescene1.addItem(QV1)
        
        self.compositeview1.resetMatrix()
        matrix = self.compositeview1.matrix()

        horizontal = 0.5
        vertical = 0.5
        matrix.scale(horizontal, vertical)
        self.compositeview1.setMatrix(matrix)
        
        QV2 = QuadView(image_8bit2, zero_array, alpha_array, self.compositeview2, self.label12, self.SelectedTextBrowser_2, self.LivetextBrowser_2)
        self.compositescene2.clear()
        self.compositescene2.addItem(QV2)
        self.compositeview2.setMatrix(matrix)
        

        QV3 = QuadView(image_8bit3, zero_array, alpha_array, self.compositeview3, self.label21, self.SelectedTextBrowser_2, self.LivetextBrowser_2)
        self.compositescene3.clear()
        self.compositescene3.addItem(QV3)
        self.compositeview3.setMatrix(matrix)
        

        QV4 = QuadView(image_8bit4, zero_array, alpha_array, self.compositeview4, self.label22, self.SelectedTextBrowser_2, self.LivetextBrowser_2)
        self.compositescene4.clear()
        self.compositescene4.addItem(QV4)
        self.compositeview4.setMatrix(matrix)








class VideoStream(QGraphicsItem):
    def __init__(self, Image, count, parent=None):
        super(VideoStream, self).__init__(parent)
        self.count = count
        self.pic = Image
        self.source = QRectF(0, 0, 1000, 1000)

    def boundingRect(self):
        return self.source

    def paint(self, painter, option, widget=None):

        shape = (1000, 1000)
        
        alpha_array = 255*(numpy.ones(shape, dtype=numpy.uint8))        
        Red = 0*alpha_array
        Green = 0*alpha_array
        Blue = 0*alpha_array
        if self.count%3 == 1:
            Green = self.pic
        elif self.count%3 == 2: 
            Blue = self.pic
        elif self.count%3 == 0:
            Red = self.pic 
        width = 1000
        

        image_ARGB_3D = numpy.dstack([Blue, Red, Green, alpha_array])
        image_ARGB_2D = numpy.reshape(image_ARGB_3D,(-1,width*4))

        ImageQ = QImage(image_ARGB_2D.data, 1000, 1000, QImage.Format_ARGB32)
        painter.drawImage(self.source, ImageQ, self.source)        





















class QuadView(QGraphicsItem):
    def __init__(self, image, zero, alpha, view, channel, selectedbrowser, livebrowser):
        super(QuadView, self).__init__(parent = None)
        self.image = image
        self.channel = channel
        self.zero = zero
        self.alpha = alpha
        self.view = view
        self.livebrowser = livebrowser
        self.selectedbrowser = selectedbrowser
        self.rect = QRectF(0, 0, 1000, 1000)
        global compinstanceL
        self.COL = compinstanceL
        self.COL.append(self)
        self.setAcceptHoverEvents(True)

    def hoverEnterEvent(self, event):
        self.setCursor(QCursor(Qt.CrossCursor))

    def hoverMoveEvent(self, event):
        
        x = int(event.pos().x())
        y = int(event.pos().y())
        pixloc = x + y*1000
        self.livebrowser.setText(QString("X..." + str(x) + "    Y..." + str(y) + "    Pixel Value..." + str(self.image[y, x])))
        
        
    def mousePressEvent(self, event):
        
        x = event.pos().x()
        y = event.pos().y()
        global mag
        if mag == True:   
            for i in self.COL:
                i.matrixresize(x, y)
        self.selectedbrowser.append(QString(str(x) + "\t" + str(y) + "\t" + str(self.image[y, x])))
        
    def matrixresize(self, x, y):
        matrix = self.view.matrix()
        scale = 2.0        
        global zoom
        if matrix.m11() == 256:
            self.view.resetMatrix()
            scale = 0.5**9
        if zoom == "-":
            if matrix.m11() == 0.5:
                scale = 1
            else:
                scale = 0.5
        matrix.scale(scale, scale)
        self.view.setMatrix(matrix)
        self.view.centerOn(x, y)
     #   limit = 256/matrix.m11()
      #  log = math.log(limit, 2)
       # adjustlim = 1000/(2**(9-log))
        #self.view.ensureVisible(x, y, adjustlim, adjustlim)

    def searchExecuted(self, x, y):
        self.channel.setText(QString(str(self.image[y, x]))) 

 #       self.searchbrowser.setText(QString("Pixel Value..." + str(self.image[y, x])))
    
    def boundingRect(self):
        return self.rect

    def paint(self, painter, option, widget=None):
      #  painter.setCompositionMode(QPainter.CompositionMode_SourceOver)
       # painter.fillRect(self.rect, Qt.transparent)
        image_ARGB_3D1 = numpy.dstack([self.image.astype(numpy.uint8), self.image.astype(numpy.uint8), self.image.astype(numpy.uint8), self.alpha])
        image_ARGB_2D1 = numpy.reshape(image_ARGB_3D1,(-1, 1000*4))
        Image1 = QImage(image_ARGB_2D1.data, 1000, 1000, QImage.Format_ARGB32)
        painter.drawImage(self.rect, Image1, self.rect)

        
class GraphicsItem(QGraphicsItem):
    def __init__(self, target, Image, source, selectedbrowser, livebrowser, searchbrowser, image_8bit, parent=None):
        super(GraphicsItem, self).__init__(parent)
        self.target = target
        self.Image = Image
        self.source = source
        self.searchbrowser = searchbrowser
        self.rawimage = image_8bit
        self.selectedbrowser = selectedbrowser
        self.livebrowser = livebrowser
        self.setAcceptHoverEvents(True)

    def hoverEnterEvent(self, event):
        self.setCursor(QCursor(Qt.CrossCursor))
        
    def hoverMoveEvent(self, event):
        x = int(event.pos().x())
        y = int(event.pos().y())
        pixloc = x + y*1000
        self.livebrowser.setText(QString("X..." + str(x) + "    Y..." + str(y) + "    Pixel Value..." + str(self.rawimage[y, x])))
        
        for i in range(3):
            for j in range(3):
                print self.rawimage[y + i-1, x + j-1].astype(numpy.uint8)
            
    
    def searchExecuted(self, x, y):
        self.searchbrowser.setText(QString("Pixel Value..." + str(self.rawimage[y, x])))
        
    def mousePressEvent(self, event):
        x = event.pos().x()
        y = event.pos().y()      
        self.selectedbrowser.append(QString(str(x) + "\t" + str(y) + "\t" + str(self.rawimage[y, x])))

        global dotpermission
        if dotpermission == True:
            cm = CircleMarker(QPointF(x, y), self)
            cm.setParentItem(self)
        
    def boundingRect(self):
        return self.source

    def paint(self, painter, option, widget=None):
        painter.drawImage(self.target, self.Image, self.source)
        painter.drawRect(self.target)
    

class CircleMarker(QGraphicsItem):
    def __init__(self, position, futureparent):
        super(CircleMarker, self).__init__()
        
        self.futureparent = futureparent
        self.fill = QColor(100, 204, 150)
        self.stroke = QColor(153, 204, 255)
        self.setPos(position)
        imageviewer.ClearDots.setEnabled(True)
        
        global lastcircle
        if len(lastcircle) > 0:
            DL = DistanceLine(lastcircle[0], lastcircle[1], position.x(), position.y())
            DL.setParentItem(self.futureparent)        
        
        self.futureparent.update()
        lastcircle = []
        lastcircle.append(position.x())
        lastcircle.append(position.y())
        
    def boundingRect(self):
        return QRectF(-6,-6,12,12)
    
    def paint(self, painter, option, widget=None):
    #    painter.setBrush(QBrush(self.fill))
        painter.setPen(QPen(self.stroke))
        painter.drawEllipse(QRectF(-5, -5, 10, 10))
        painter.drawEllipse(QRectF(-6,-6,12,12))

class DistanceLine(QGraphicsItem):
    def __init__(self, x1, y1, x2, y2):
        super(DistanceLine, self).__init__()
        
        self.x1 = x1
        self.x2 = x2
        self.y1 = y1
        self.y2 = y2
        
        self.stroke = QColor(255, 204, 153)
        self.rect = QRectF(0,0,100,100)

        global lastline
        if lastline is not None:
            try:
                LLS = lastline.scene()
                LLS.removeItem(lastline)
            except:
                return
        lastline = self

    def boundingRect(self):
        return self.rect
    
    def paint(self, painter, option, widget=None):
        painter.setPen(QPen(self.stroke))
        for i in range(9):
            x = (i+1)%3 -1
            y = (i+1)/3 -1
            line = QLine(self.x2 + x , self.y2 + y, self.x1 + x, self.y1 + y)
            painter.drawLine(line)



def main():
    app = QApplication(sys.argv)
    window = STBimageviewer()
    window.show()
    app.exec_()

if __name__ == '__main__':
    main()