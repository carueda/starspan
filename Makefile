#
# Makefile to build the starspan tool
# $Id$
#

# Use Makefile.props.templ as a template to create Makefile.props
# according to your system.

include Makefile.props

#------------------------------------------------------------

INCLUDE := -Ivector -Iraster -Itraverser -Irasterizers -Ijts -Icsv -Istats		 

# starspan_db.cc
INCLUDE += -I${SHAPELIB_PREFIX}/include/libshp

INCLUDE += -I$(GDAL_PREFIX)/include
INCLUDE += -I$(GDAL_SRC_DIR)/frmts 
		 
# See rasterizers/LineRasterizer.cc
INCLUDE += -I${AGG_PREFIX}/include/agg2 

INCLUDE += -I/usr/local/include


SRCS =	starspan_stats.cc \
		starspan_db.cc \
		starspan_update_csv.cc \
		starspan_update_dbf.cc \
		starspan_csv.cc \
		starspan_gen_envisl.cc \
		starspan_minirasters.cc \
		starspan_jtstest.cc \
		starspan_util.cc \
		jts/jts.cc \
		traverser/traverser.cc \
		raster/Raster_gdal.cc \
		vector/Vector_ogr.cc \
		rasterizers/LineRasterizer.cc \
		csv/Csv.cc \
		stats/Stats.cc \
		starspan_dump.cc \
		starspan_tuct2.cc


OBJS = $(subst .cc,.o,$(SRCS))

CXXFLAGS = -g -Wall \
		   $(INCLUDE)

LIBPATH = -L$(AGG_PREFIX)/lib \
	 -L$(GDAL_PREFIX)/lib \
	 -L$(GEOS_PREFIX)/lib \
	 -L$(SHAPELIB_PREFIX)/lib \

LDFLAGS = -lgdal -lshp -lgeos

.PHONY= all install clean

all: starspan2

objs : $(OBJS)
	   
starspan2: starspan2.o $(SRCS)
	g++ $(CXXFLAGS) $(OBJ_OPT) \
	    $(LIBPATH) -o starspan2 starspan2.o $(SRCS) $(LDFLAGS)

install:
	install -p starspan starspan2 $(STARSPAN_PREFIX)/bin/ 

starspan1: starspan1.o $(SRCS)
	g++ $(LIBPATH) -o starspan1 starspan1.o $(SRCS) $(LDFLAGS)


.cc.o:
	g++ -c $(CXXFLAGS) $(OBJ_OPT) $<

clean:
	rm -f *.o *~ starspan1 starspan2
