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

SHP_INCLUDE=/usr/local/include/libshp
POLY_INCLUDE=polygon/

#---------------------------------------------------------------------#
#                    compiler flags
#---------------------------------------------------------------------#

CXXFLAGS = \
           $(CGAL_CXXFLAGS) \
           $(LONG_NAME_PROBLEM_CXXFLAGS) \
           $(DEBUG_OPT) \
		   -g -I$(SHP_INCLUDE) -I$(POLY_INCLUDE)

#---------------------------------------------------------------------#
#                    linker flags
#---------------------------------------------------------------------#

LIBPATH = \
          $(CGAL_LIBPATH) \
		  -L/usr/local/lib

LDFLAGS = -lshp \
          $(LONG_NAME_PROBLEM_LDFLAGS) \
          $(CGAL_LDFLAGS) 
		  

#---------------------------------------------------------------------#
#                    target entries
#---------------------------------------------------------------------#

all:            \
                starspan$(EXE_EXT) \

starspan$(EXE_EXT): tool0$(OBJ_EXT) polygon/Polygon.o
	$(CGAL_CXX) $(LIBPATH) $(EXE_OPT)starspan tool0$(OBJ_EXT) polygon/Polygon.o $(LDFLAGS)


#---------------------------------------------------------------------#
#                    suffix rules
#---------------------------------------------------------------------#

.cc$(OBJ_EXT):
	$(CGAL_CXX) $(CXXFLAGS) $(OBJ_OPT) $<

