#
# Makefile to build the starspan tool
# $Id$
#
#

INCLUDE= -Ivector -Iraster -Ijts \
         -I/usr/local/include  \
         -I/home/carueda/cstars/GDAL/gdal/frmts  \
		 -I/usr/local/include/libshp

SRCS = starspan_minirasters.cc \
       starspan_jtstest.cc \
       starspan_util.cc \
	   jts/jts.cc \
	   raster/Raster_gdal.cc \
	   vector/Vector_ogr.cc

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
