#
# Makefile to build the starspan tool
# $Id$
#
#

#---------------------------------------------------------------------#
#                    include platform specific settings
#---------------------------------------------------------------------#
# Choose the right include file from the <cgalroot>/make directory.

CGAL_MAKEFILE=/home/carueda/cstars/CGAL/CGAL-3.0.1/make/makefile_i686_Linux-2.4.20_g++-2.95
include $(CGAL_MAKEFILE)

VECTOR_INCLUDE=-Ivector -I/usr/local/include  -I/usr/local/include/libshp
RASTER_INCLUDE=-Iraster -I/usr/local/include
POLY_INCLUDE=-Ipolygon/

# using vector_ogr
OBJS=polygon/Polygon.o raster/Raster_gdal.o vector/Vector_ogr.o

#---------------------------------------------------------------------#
#                    compiler flags
#---------------------------------------------------------------------#

CXXFLAGS = \
           $(CGAL_CXXFLAGS) \
           $(LONG_NAME_PROBLEM_CXXFLAGS) \
           $(DEBUG_OPT) \
		   -g \
		   $(VECTOR_INCLUDE) \
		   $(RASTER_INCLUDE) \
		   $(POLY_INCLUDE)

#---------------------------------------------------------------------#
#                    linker flags
#---------------------------------------------------------------------#

LIBPATH = \
          $(CGAL_LIBPATH) \
		  -L/usr/local/lib

LDFLAGS = -lshp -lgdal \
          $(LONG_NAME_PROBLEM_LDFLAGS) \
          $(CGAL_LDFLAGS) 
		  

#---------------------------------------------------------------------#
#                    target entries
#---------------------------------------------------------------------#

.PHONY= all install

all:            \
                starspan$(EXE_EXT) \

starspan$(EXE_EXT): starspan2$(OBJ_EXT) $(OBJS)
	$(CGAL_CXX) $(LIBPATH) $(EXE_OPT)starspan starspan2$(OBJ_EXT) $(OBJS) $(LDFLAGS)

install:
	cp starspan$(EXE_EXT) ~/bin/ 

#---------------------------------------------------------------------#
#                    suffix rules
#---------------------------------------------------------------------#

.cc$(OBJ_EXT):
	$(CGAL_CXX) $(CXXFLAGS) $(OBJ_OPT) $<

