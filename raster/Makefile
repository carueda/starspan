#
# Makefile to build raster module
# $Id$
#
#
RASTER_INCLUDE=-I/usr/local/include

CXXFLAGS = \
		   -g \
		   $(RASTER_INCLUDE)

all: Raster_gdal.o

.cc.o:
	g++ $(CXXFLAGS) -c $<

