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
        self.path = QDir.homePath()
        self.connect(self.actionOpen, SIGNAL("triggered()"), self.OpenClicked)
        self.selectedbrowser = self.SelectedTextBrowser
        self.livebrowser = self.LivetextBrowser
        
        self.Green = self.GreenDefault
        self.Red = self.RedDefault
        self.Blue = self.BlueDefault
        
        #self.dotpermission = False
        self.Image = None
        global imageviewer
        imageviewer = self
        global dotpermission
        dotpermission = False
        
        
    def on_LoadComposite_released(self):
        if self.Red.isChecked() == True:
            path2 = "C:/Users/Roger Conturie/Desktop/rawimages/colorsnap-cy5_after.raw"
        else:
            path2 = QFileDialog.getOpenFileName(self,
                    "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg
        self.RedFile.setText(str(path2.split("/")[-1]))        

        if self.Green.isChecked() == True:
            path1 = "C:/Users/Roger Conturie/Desktop/rawimages/colorsnap-cy3_after.raw"
        else:
            path1 = QFileDialog.getOpenFileName(self,
                    "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg
        self.GreenFile.setText(str(path1.split("/")[-1]))

        if self.Blue.isChecked() == True:
            path3 = "C:/Users/Roger Conturie/Desktop/rawimages/colorsnap-txred_after.raw"
        else:
            path3 = QFileDialog.getOpenFileName(self,
                    "Open File", self.path , str("Images (*raw)"))  #*.png *.xpm *.jpg

        self.BlueFile.setText(str(path3.split("/")[-1]))
        
        self.ImageGenerator(path1, path2, path3)

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
            graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, channel1) 
            self.scene.clear()
            self.scene.addItem(graphicitem)
            self.Image = Image
              #  image_array = numpy.array(Image)

            
    def ImageGenerator(self, path1, path2, path3):
        width = 1000
        height = 1000
        shape = (width, height)
        image_file = open(path1)
        # load a 1000000 length array
        image_array_1D1 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
        image_file.close()
        image_array_2D1 = image_array_1D1.reshape(shape)
        image_8bit1 = (image_array_2D1 >> 6).astype(numpy.uint8)
        
        if path2 is not None:
            image_file = open(path2)
            # load a 1000000 length array
            image_array_1D2 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
            image_file.close()
            image_array_2D2 = image_array_1D2.reshape(shape)
            image_8bit2 = (image_array_2D2 >> 6).astype(numpy.uint8)
        else:
            image_8bit2 = image_8bit1
        
        if path3 is not None:
            image_file = open(path3)
            # load a 1000000 length array
            image_array_1D3 = numpy.fromfile(file=image_file, dtype=numpy.uint16)
            image_file.close()
            image_array_2D3 = image_array_1D3.reshape(shape)
            image_8bit3 = (image_array_2D3 >> 6).astype(numpy.uint8)
        else:
            image_8bit3 = image_8bit1
            

        # make the array 2D


        # create the alpha channel array
        alpha_array = 255*(numpy.ones(shape, dtype=numpy.uint8))
 
        # apparently alpha gets stacked last. load BGRA because the byte order in memory for each
        # channel is stored in memory as BGRA 0xBBGGRRAA by little-endian CPU's such as intel processors
        # for future overlay work do 
        # example>>>>> image_ARGB = numpy.dstack([image_Blue,image_Green,image_Red,alpha_array])
        image_ARGB_3D = numpy.dstack([image_8bit1,image_8bit2,image_8bit3,alpha_array])
        # reshape to a 2D array that has 4 bytes per pixel
        image_ARGB_2D = numpy.reshape(image_ARGB_3D,(-1,width*4)) # reshape does not copy, only manipulates data structure holding the data
 
        #Numpy buffer QImage declaration
        Image = QImage(image_ARGB_2D.data, width, height, QImage.Format_ARGB32)
        Image.ndarray = image_ARGB_2D  # necessary to create a persistant reference to the data per QImage class
        target = QRectF(0, 0, Image.width(), Image.height())
        source = QRectF(0, 0, Image.width(), Image.height())
    
        #graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, image_8bit)
        graphicitem = GraphicsItem(target, Image, source, self.selectedbrowser, self.livebrowser, image_array_2D1) 
        self.scene.clear()
        self.scene.addItem(graphicitem)
        self.Image = Image
        


        
class PainterObject(QGraphicsItem):
    def __init__(self, baseimage, overlayimage, imagewithoverlay):
        super(PainterObject, self).__init__(parent = None)
        self.baseimage = baseimage
        self.overlayimage = overlayimage
        self.imagewithoverlay = imagewithoverlay
        
             
    def boundingRect(self):
        return QRectF(0, 0, self.baseimage.width(), self.baseimage.height())

    def paint(self, painter, option, widget=None):
        painter.setCompositionMode(QPainter.CompositionMode_Source)
        painter.fillRect(self.imagewithoverlay.rect(), Qt.transparent)
        
        px_baseImage = 0
        py_baseImage = 0
        px_overlayimage = 0
        py_overlayimage = 0
        
        painter.setCompositionMode(QPainter.CompositionMode_SourceOver)
        painter.drawImage(px_baseImage, py_baseImage, self.baseimage)
        painter.drawImage(px_overlayimage, py_overlayimage, self.overlayimage)
        
        
        
class GraphicsItem(QGraphicsItem):
    def __init__(self, target, Image, source, selectedbrowser, livebrowser, image_8bit, parent=None):
        super(GraphicsItem, self).__init__(parent)
        self.target = target
        self.Image = Image
        self.source = source
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
        self.rect = QRectF(-5, -5, 10, 10)
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
        return self.rect
    
    def paint(self, painter, option, widget=None):
    #    painter.setBrush(QBrush(self.fill))
        painter.setPen(QPen(self.stroke))
        painter.drawEllipse(self.rect)

class DistanceLine(QGraphicsItem):
    def __init__(self, x1, y1, x2, y2):
        super(DistanceLine, self).__init__()
        
        self.x1 = x1
        self.x2 = x2
        self.y1 = y1
        self.y2 = y2
        self.line = QLine(self.x2, self.y2 , self.x1, self.y1)        
        self.stroke = QColor(255, 204, 153)
        self.rect = QRectF(0,0,100,100)
        
        global lastline
        if lastline is not None:
            LLS = lastline.scene()
            LLS.removeItem(lastline)
            
        lastline = self

    def boundingRect(self):
        return self.rect
    
    def paint(self, painter, option, widget=None):
        painter.setPen(QPen(self.stroke))
        painter.drawLine(self.line)
    
    
def main():
    app = QApplication(sys.argv)
    window = STBimageviewer()
    window.show()
    app.exec_()

if __name__ == '__main__':
    main()
