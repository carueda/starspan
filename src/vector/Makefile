#
# Makefile to build vector module
# $Id$
#
#
VECTOR_INCLUDE=-I/usr/local/include  -I/usr/local/include/libshp
POLY_INCLUDE=-I../polygon/

CXXFLAGS = \
		   -g -Wall \
		   $(VECTOR_INCLUDE) \
		   $(POLY_INCLUDE)

all: Vector_ogr.o Vector_shapelib.o

.cc.o:
	g++ $(CXXFLAGS) -c $<

