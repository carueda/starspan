#
# Makefile to build the starspan tool
# $Id$
#
#

VECTOR_INCLUDE=-Ivector -I/usr/local/include  -I/usr/local/include/libshp
RASTER_INCLUDE=-Iraster -I/usr/local/include

OBJS = jts.o raster/Raster_gdal.o vector/Vector_ogr.o

CXXFLAGS = -g -Wall \
		   $(VECTOR_INCLUDE) \
		   $(RASTER_INCLUDE)

LIBPATH = -L/usr/local/lib

LDFLAGS = -lshp -lgdal -lgeos

.PHONY= all install

all: starspan2

starspan2: starspan2.o $(OBJS)
	g++ $(LIBPATH) -o starspan2 starspan2.o $(OBJS) $(LDFLAGS)

install:
	cp starspan2 ~/bin/ 

starspan1: starspan1.o $(OBJS)
	g++ $(LIBPATH) -o starspan1 starspan1.o $(OBJS) $(LDFLAGS)


.cc.o:
	g++ -c $(CXXFLAGS) $(OBJ_OPT) $<

