# Badass Makefile for the G.007 Polonator
# Nick Conway 12 2010 Wyss Institute
# FYI:
# -L is a root directory for a library, -l is off a root directory
# -I is an include directory
# Edit this file for pointing to locations of
# SWIG, python, numpy, and the phoenix frame grabber libraries

# commented out the absolute path of the base directory
BASE_DIR := $(shell pwd)


# general include and compiler optimizations
CFLAGS = -I$(BASE_DIR) -O3
CPPFLAGS = -I$(BASE_DIR) -O3 -msse -msse2 -fexpensive-optimizations -funroll-loops

# for C SWIG wrapped Python
SHARELIBFLAGS = -fPIC -shared
#PYTHON_VER=2.5
#PYTHON_VER := $(shell `python -V 2>&1 | sed -e 's/Python 2\.\([0-9]*\).*/python2.\1/'`)
PYTHON_VER=2.7
PYINC		= /usr/include/python$(PYTHON_VER)
PYLIB		= /usr/lib/python$(PYTHON_VER)
SWIG_LIB = -I/usr/lib64
PYTHONFLAGS = -I$(PYINC) -I$(PYLIB) -lpython$(PYTHON_VER)

# For including numpy in SWIG
NUMPY_INC1 = /usr/lib64/python$(PYTHON_VER)/site-packages/numpy/core/include
NUMPY_INC2 = /usr/lib64/python$(PYTHON_VER)/site-packages/numpy/numarray
NUMPY_FLAGS = -I$(NUMPY_INC1) -I$(NUMPY_INC2)

# Logging in the polonator code wrapped by python
LOG_DEP = $(BASE_DIR)/logger.c $(BASE_DIR)/logger.h
LOG_SRC = $(BASE_DIR)/logger.c


# Image processing source
IMG_DIR = $(BASE_DIR)/image
FINDOBJ_DEP = $(IMG_DIR)/find_objects.c $(IMG_DIR)/find_objects.h \
				$(IMG_DIR)/img_tools.c $(IMG_DIR)/img_tools.h $(BASE_DIR)/ProcessorParams.h
FINDOBJ_SRC = $(IMG_DIR)/find_objects.c $(IMG_DIR)/img_tools.c
FINDOBJ_INC = -I$(IMG_DIR)

IMGTOOLS_DEP = $(IMG_DIR)/img_tools.c $(IMG_DIR)/img_tools.h $(BASE_DIR)/ProcessorParams.h
IMGTOOLS_SRC = $(IMG_DIR)/img_tools.c
IMGTOOLS_INC = -I$(IMG_DIR)

# DMD source related to the DLi D4000 control board
ILL_DIR = $(BASE_DIR)/illum
ILLUMFUNCTIONS_DEP = $(ILL_DIR)/control_D4000.c $(ILL_DIR)/control_D4000.h \
						$(ILL_DIR)/image_generator.c $(ILL_DIR)/image_generator.h \
						$(IMG_DIR)/alignment_functions.c $(IMG_DIR)/alignment_functions.h \
						$(ILL_DIR)/hardware_D4000.c $(ILL_DIR)/hardware_D4000.h \
						$(ILL_DIR)/illuminate_common.c $(ILL_DIR)/illuminate_common.h \
						$(LOG_DEP)
ILLUMFUNCTIONS_SRC = $(ILL_DIR)/control_D4000.c $(ILL_DIR)/image_generator.c \
						$(IMG_DIR)/alignment_functions.c $(ILL_DIR)/hardware_D4000.c \
						$(ILL_DIR)/illuminate_common.c \
						$(LOG_SRC)
ILLUMFUNCTIONS_INC = -I$(ILL_DIR) -I$(IMG_DIR)

LIB_DMD_FLAGS = -lD4000


# Motion Control Source
MO_DIR = $(BASE_DIR)/motion
MAESTRO_DEP = $(MO_DIR)/maestro_functions.c $(MO_DIR)/maestro_functions.h \
				$(MO_DIR)/config.c $(MO_DIR)/config.h \
				$(MO_DIR)/get_sock.c $(MO_DIR)/get_sock.h \
				$(CAM_DIR)/global_parameters.h
MAESTRO_SRC = $(MO_DIR)/maestro_functions.c $(MO_DIR)/config.c \
				$(MO_DIR)/get_sock.c
MAESTRO_INC = -I$(MO_DIR) -I$(CAM_DIR)

# Network Functions source
NET_DEP = $(BASE_DIR)/network_functions.c $(BASE_DIR)/network_functions.h \
				$(BASE_DIR)/logger.c $(BASE_DIR)/logger.h \
				$(MO_DIR)/get_sock.c $(MO_DIR)/get_sock.h \

NET_SRC = $(BASE_DIR)/network_functions.c \
				$(BASE_DIR)/logger.c \
				$(MO_DIR)/get_sock.c
NET_INC = -I$(MO_DIR)

# Camera/Framegrabber Source
# Active Silicon Phoenix Framegrabber
CAM_DIR = $(BASE_DIR)/camera
CAMERA1_DEP = $(CAM_DIR)/as_phoenix_functions.c $(CAM_DIR)/as_phoenix_functions.h \
                $(CAM_DIR)/as_phoenix_live_functions.c $(CAM_DIR)/as_phoenix_live_functions.h \
                $(CAM_DIR)/common/common.c $(CAM_DIR)/common/common.h \
				$(CAM_DIR)/global_parameters.h
CAMERA1_SRC = $(CAM_DIR)/as_phoenix_functions.c $(CAM_DIR)/as_phoenix_live_functions.c \
                $(CAM_DIR)/common/common.c
CAMERA1_INC = -I$(CAM_DIR) -I$(MO_DIR) -I$(CAM_DIR)/common

CAMERA2_DEP = $(CAM_DIR)/as_phoenix.c $(CAM_DIR)/as_phoenix.h \
				$(CAM_DIR)/as_phoenix_functions.c $(CAM_DIR)/as_phoenix_functions.h \
				$(CAM_DIR)/as_phoenix_live_functions.c $(CAM_DIR)/as_phoenix_live_functions.h \
				$(CAM_DIR)/common/common.c $(CAM_DIR)/common/common.h \
				$(CAM_DIR)/global_parameters.h
CAMERA2_SRC = $(CAM_DIR)/as_phoenix.c $(CAM_DIR)/as_phoenix_functions.c \
				$(CAM_DIR)/as_phoenix_live_functions.c $(CAM_DIR)/common/common.c
CAMERA2_INC = -I$(CAM_DIR) -I$(MO_DIR) -I$(CAM_DIR)/common

CAMERA3_DEP =  $(CAM_DIR)/as_phoenix.c $(CAM_DIR)/as_phoenix_functions.c \
                $(CAM_DIR)/as_phoenix_live.c $(CAM_DIR)/as_phoenix_live.h \
				$(CAM_DIR)/common/common.c $(CAM_DIR)/common/common.h \
				$(CAM_DIR)/global_parameters.h
CAMERA3_SRC =  $(CAM_DIR)/as_phoenix_live.c  \
                $(CAM_DIR)/as_phoenix_functions.c \
				$(CAM_DIR)/common/common.c
CAMERA3_INC = -I$(CAM_DIR) -I$(MO_DIR) -I$(CAM_DIR)/common

#/////////////////////////////////////////////////////
# includes for the Active Silicon Phoenix FrameGrabber
# adapted from PHXlive Makefile
#PHX_VER=5.59
PHX_VER=5.74
PHX_INCDIR     = /usr/local/active_silicon/phx_sdk-$(PHX_VER)/include
COMMON_DIR  = $(BASE_DIR)/common
COMMON_SRC  = $(BASE_DIR)/common/common.c
SUBARCH := $(shell uname -m)
ifeq ($(SUBARCH),x86_64)
PHX_LIBDIR = /usr/local/active_silicon/phx_sdk-$(PHX_VER)/lib/linux64
X11_LIBDIR = /usr/X11R6/lib64
else
PHX_LIBDIR = /usr/local/active_silicon/phx_sdk-$(PHX_VER)/lib/linux
X11_LIBDIR = /usr/X11R6/lib
endif

# general includes
PHX_LIBS = -L$(PHX_LIBDIR) -lpfw -lphx -lpbu -lpdl -lX11 -lm -Xlinker -rpath $(PHX_LIBDIR)
PHX_INCLUDE = -I$(PHX_INCDIR) -I$(COMMON_DIR) -I/usr/X11R6/include
PHX_CFLAGS  = -D_GNU_SOURCE -D_PHX_LINUX -D_CDA_LINUX -D_REENTRANT
# end PHXlive section
#////////////////////////////////////////////////////

# this is for the pipeline integrated with selective release
PROCDMD_DEP = $(BASE_DIR)/polonator_proc_DMD.c \
			    $(BASE_DIR)/network_functions.c $(BASE_DIR)/network_functions.h \
			    $(MO_DIR)/get_sock.c $(MO_DIR)/get_sock.h \
				$(LOG_DEP)
PROCDMD_SRC = $(BASE_DIR)/polonator_proc_DMD.c \
			    $(BASE_DIR)/network_functions.c \
			    $(MO_DIR)/get_sock.c \
				$(LOG_SRC)
PROCDMD_INC = -I$(ILL_DIR) -I$(MO_DIR)

ACQ_DEP = $(BASE_DIR)/Polonator-acquirer.c \
			$(MAESTRO_DEP) $(CAMERA1_DEP) \
			$(BASE_DIR)/network_functions.c $(BASE_DIR)/network_functions.h \
			$(LOG_DEP)
ACQ_SRC = $(BASE_DIR)/Polonator-acquirer.c \
			$(MAESTRO_SRC) $(CAMERA1_SRC) $(BASE_DIR)/network_functions.c \
			$(LOG_SRC)
ACQ_INC = -I$(CAM_DIR) -I$(MO_DIR) -I$(CAM_DIR)/common

# Command line runs the stage alignment
STG_DEP = $(BASE_DIR)/Polonator-stagealign.c $(BASE_DIR)/Polonator-stagealign.h \
		$(MAESTRO_DEP) \
		$(CAMERA2_DEP) \
		$(BASE_DIR)/network_functions.c $(BASE_DIR)/network_functions.h \
		$(BASE_DIR)/logger.c $(BASE_DIR)/logger.h
STG_SRC = $(BASE_DIR)/Polonator-stagealign.c \
	$(CAMERA2_SRC) \
	$(MAESTRO_SRC) \
	$(BASE_DIR)/network_functions.c \
	$(BASE_DIR)/logger.c
STG_INC = -I$(CAM_DIR) -I$(MO_DIR) -I$(CAM_DIR)/common

# Command line utilities such a camera operation
UTIL_DEP = $(BASE_DIR)/PolonatorUtils.c $(BASE_DIR)/PolonatorUtils.h \
		$(MAESTRO_DEP) \
		$(CAMERA1_DEP) \
		$(BASE_DIR)/network_functions.c $(BASE_DIR)/network_functions.h \
		$(BASE_DIR)/logger.c $(BASE_DIR)/logger.h
UTIL_SRC = $(BASE_DIR)/PolonatorUtils.c \
	$(CAMERA1_SRC) \
	$(MAESTRO_SRC)\
	$(BASE_DIR)/network_functions.c \
	$(BASE_DIR)/logger.c
UTIL_INC = -I$(CAM_DIR) -I$(MO_DIR) -I$(CAM_DIR)/common

# Explicit rules for building objects go here.

all : directory_check \
	python_camA python_camB python_maestro \
	python_log \
	python_img_tools python_find_obj \
	Polonator-stagealign PolonatorUtils \
	PolonatorAcq \
	python_network

directory_check:
	echo nachos $(BASE_DIR)


python_illum: $(ILLUMFUNCTIONS_DEP) $(ILL_DIR)/control_D4000.i
	swig -python $(ILL_DIR)/control_D4000.i
	gcc $(ILLUMFUNCTIONS_SRC) $(ILL_DIR)/control_D4000_wrap.c $(NUMPY_FLAGS) $(PYTHONFLAGS) \
		$(CFLAGS) $(ILLUMFUNCTIONS_INC) -o $(ILL_DIR)/_D4000.so $(LIB_DMD_FLAGS) $(SHARELIBFLAGS)
    #rm *.o

python_camA: $(CAMERA2_DEP) $(LOG_DEP) $(CAM_DIR)/as_phoenix.i
	swig -python $(CAM_DIR)/as_phoenix.i
	gcc $(CAMERA2_SRC) $(MAESTRO_SRC) $(LOG_SRC) $(CAM_DIR)/as_phoenix_wrap.c $(NUMPY_FLAGS) $(PYTHONFLAGS) \
	    $(CFLAGS) $(CAMERA2_INC) -o $(CAM_DIR)/_asPhoenix.so \
	    $(PHX_CFLAGS) $(PHX_INCLUDE) $(PHX_LIBS) $(SHARELIBFLAGS) \
     #rm *.o

python_camB: $(CAMERA2_DEP) $(LOG_DEP) $(CAM_DIR)/as_phoenix_functions.i
	swig -python $(CAM_DIR)/as_phoenix_functions.i
	gcc $(CAMERA2_SRC) $(MAESTRO_SRC) $(LOG_SRC) $(CAM_DIR)/as_phoenix_wrap.c $(NUMPY_FLAGS) $(PYTHONFLAGS) \
	    $(CFLAGS) $(CAMERA2_INC) -o $(CAM_DIR)/_asPhoenixFunctions.so \
	    $(PHX_CFLAGS) $(PHX_INCLUDE) $(PHX_LIBS) $(SHARELIBFLAGS) \
     #rm *.o

python_camC: $(CAMERA3_DEP) $(LOG_DEP) $(CAM_DIR)/as_phoenix_live.i
	swig -python $(CAM_DIR)/as_phoenix_live.i
	gcc $(CAMERA3_SRC) $(MAESTRO_SRC) $(LOG_SRC) $(CAM_DIR)/as_phoenix_live_wrap.c $(NUMPY_FLAGS) $(PYTHONFLAGS) \
	    $(CFLAGS) $(CAMERA3_INC) -o $(CAM_DIR)/_asPhoenixLive.so \
	    $(PHX_CFLAGS) $(PHX_INCLUDE) $(PHX_LIBS) $(SHARELIBFLAGS)
     #rm *.o

python_maestro: $(MAESTRO_DEP) $(LOG_DEP) $(MO_DIR)/maestro_functions.i
	swig -python -lcpointer.i $(MO_DIR)/maestro_functions.i
	gcc $(MAESTRO_SRC) $(LOG_SRC) $(MO_DIR)/maestro_functions_wrap.c $(PYTHONFLAGS) \
	$(CFLAGS) $(MAESTRO_INC) -o $(MO_DIR)/_maestroFunctions.so  $(SHARELIBFLAGS)
	#rm *.o

python_proc_dmd: $(PROCDMD_DEP) $(BASE_DIR)/polonator_proc_DMD.i
	swig -python -lcpointer.i $(BASE_DIR)/polonator_proc_DMD.i
	gcc $(PROCDMD_SRC) $(BASE_DIR)/polonator_proc_DMD_wrap.c $(PYTHONFLAGS) \
	$(CFLAGS) $(PROCDMD_INC) -o $(BASE_DIR)/_polProcDMD.so  \
	$(SHARELIBFLAGS)
	#rm *.o

python_log: $(BASE_DIR)/logger.c $(BASE_DIR)/logger.h $(BASE_DIR)/logger.i
	swig -python -lcpointer.i $(BASE_DIR)/logger.i
	gcc $(BASE_DIR)/logger.c $(BASE_DIR)/logger_wrap.c $(PYTHONFLAGS) \
	$(CFLAGS) -o $(BASE_DIR)/_logger.so  $(SHARELIBFLAGS)
	#rm *.o

python_network: $(NET_DEP)  $(BASE_DIR)/network_functions.i
	swig -python $(BASE_DIR)/network_functions.i
	gcc $(NET_SRC) $(BASE_DIR)/network_functions_wrap.c $(NUMPY_FLAGS) $(PYTHONFLAGS) \
	$(CFLAGS) $(NET_INC) -o $(BASE_DIR)/_networkG007.so  $(SHARELIBFLAGS)

Polonator-stagealign: $(STG_DEP)
	gcc -Wall $(STG_SRC) $(CFLAGS) $(STG_INC) -o  ../bin/Polonator-stagealign $(PHX_CFLAGS) $(PHX_INCLUDE) $(PHX_LIBS)

PolonatorUtils: $(UTIL_DEP)
	gcc -Wall $(UTIL_SRC) $(CFLAGS) $(UTIL_INC) -o ../bin/PolonatorUtils $(PHX_CFLAGS) $(PHX_INCLUDE) $(PHX_LIBS)

PolonatorAcq: $(ACQ_DEP)
	gcc -Wall $(ACQ_SRC) $(CFLAGS) $(ACQ_INC) -o ../bin/Polonator-acquirer $(PHX_CFLAGS) $(PHX_INCLUDE) $(PHX_LIBS)

python_find_obj: $(FINDOBJ_DEP) $(IMG_DIR)/find_objects.i
	swig -c++ -python $(IMG_DIR)/find_objects.i
	g++ $(FINDOBJ_SRC) $(IMG_DIR)/find_objects_wrap.cxx $(NUMPY_FLAGS) $(PYTHONFLAGS) \
		$(CPPFLAGS) $(FINDOBJ_INC) -o $(IMG_DIR)/_findObjects.so $(LIBFLAGS) $(SHARELIBFLAGS)
    #rm *.o

python_img_tools: $(IMGTOOLS_DEP) $(IMG_DIR)/img_tools.i
	swig -c++ -python $(IMG_DIR)/img_tools.i
	g++ $(IMGTOOLS_SRC) $(IMG_DIR)/img_tools_wrap.cxx $(NUMPY_FLAGS) $(PYTHONFLAGS) \
		$(CPPFLAGS) $(IMGTOOLS_INC) -o $(IMG_DIR)/_imgTools.so $(LIBFLAGS) $(SHARELIBFLAGS)
    #rm *.o

clean :
	-rm -r -f *.o *_wrap.c *.so *.pyc
	-rm -f ../bin/Polonator-stagealign
	-rm -f ../bin/PolonatorUtils
	-rm -f ../bin/Polonator-acquirer
# EOF

