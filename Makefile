#
# Makefile to build the starspan tool
# $Id$
#
#

# specify the GDAL source directory
GDAL_SRC_DIR := /home/carueda/cstars/GDAL/gdal

# specify where GDAL is installed (--prefix)
GDAL_PREFIX := /usr/local

# specify where GEOS is installed (--prefix)
GEOS_PREFIX := /usr/local

# specify where SHAPELIB is installed
SHAPELIB_PREFIX := /usr/local

# specify where AGG is installed
AGG_PREFIX := /usr/local


INCLUDE := -I/usr/local/include

# starspan_db.cc
INCLUDE += -I${SHAPELIB_PREFIX}/include/libshp

INCLUDE += -I$(GDAL_PREFIX)/include
INCLUDE += -I$(GDAL_SRC_DIR)/frmts 
		 
# See rasterizers/LineRasterizer.cc
INCLUDE += -I${AGG_PREFIX}/include/agg2 
INCLUDE += -Ivector -Iraster -Itraverser -Irasterizers -Ijts		 


SRCS = starspan_db.cc \
       starspan_csv.cc \
       starspan_gen_envisl.cc \
       starspan_minirasters.cc \
       starspan_jtstest.cc \
       starspan_util.cc \
	   jts/jts.cc \
	   traverser/traverser.cc \
	   raster/Raster_gdal.cc \
	   vector/Vector_ogr.cc \
	   rasterizers/LineRasterizer.cc

	   
CXXFLAGS = -g -Wall \
		   $(INCLUDE)

LIBPATH = -L/usr/local/lib

LDFLAGS = -lshp -lgdal -lgeos

.PHONY= all install clean

all: starspan2

starspan2: starspan2.o $(SRCS)
	g++ $(CXXFLAGS) $(OBJ_OPT) \
	    $(LIBPATH) -o starspan2 starspan2.o $(SRCS) $(LDFLAGS)

install:
	cp starspan2 ~/bin/ 

starspan1: starspan1.o $(SRCS)
	g++ $(LIBPATH) -o starspan1 starspan1.o $(SRCS) $(LDFLAGS)


.cc.o:
	g++ -c $(CXXFLAGS) $(OBJ_OPT) $<

clean:
	rm -f *.o starspan1 starspan2
