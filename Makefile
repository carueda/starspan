#
# Makefile to build the starspan tool
# $Id$
#
#

INCLUDE := -Ivector -Iraster -Itraverser -Ijts \
         -I/usr/local/include
		 
# starspan_db.cc
INCLUDE += -I/usr/local/include/libshp

# starspan_utilcc: vrt/vrtdataset.h not in /usr/local/include/  ;-(
INCLUDE += -I/home/carueda/cstars/GDAL/gdal/frmts 
		 
# See rasterizers/LineRasterizer.cc
INCLUDE += -I/usr/local/include/agg2 
INCLUDE += -Irasterizers 
		 


SRCS = starspan_db.cc \
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
