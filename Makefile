#
# Makefile to build the starspan tool
# $Id$
#

# NOTE: This file is not intended to be changed by users.
# Instead, create and edit a text file Makefile.props to define 
# the required properties before building and installing STARSPAN. 
# You can rename or copy the provided template Makefile.props.templ 
# as Makefile.props and make the necessary changes. 
# MacOS X users may want to use Makefile.props.fink as the template. 


# Makefile.props must define the required build properties.
# See Makefile.props.templ for a template.  
include Makefile.props

#------------------------------------------------------------

# my include directories:
INCLUDE := -Isrc -Isrc/csv -Isrc/jts -Isrc/raster -Isrc/rasterizers -Isrc/stats \
           -Isrc/traverser -Isrc/util -Isrc/vector 

# external include directories:
INCLUDE += -I$(GDAL_PREFIX)/include
INCLUDE += -I$(GEOS_PREFIX)/include


SRCS =	src/starspan_stats.cc \
		src/starspan_countbyclass.cc \
		src/starspan_update_csv.cc \
		src/starspan_csv.cc \
		src/starspan_csv_raster_field.cc \
		src/starspan_gen_envisl.cc \
		src/starspan_minirasters.cc \
		src/starspan_jtstest.cc \
		src/starspan_util.cc \
		src/starspan_dump.cc \
		src/starspan_tuct2.cc \
		src/csv/Csv.cc \
		src/jts/jts.cc \
		src/raster/Raster_gdal.cc \
		src/rasterizers/LineRasterizer.cc \
		src/stats/Stats.cc \
		src/traverser/traverser.cc \
		src/util/Progress.cc \
		src/vector/Vector_ogr.cc


OBJS = $(subst .cc,.o,$(SRCS))

CXXFLAGS = -g -Wall \
		   $(INCLUDE)

PROF = -pg -fprofile-arcs

# for external libraries:
LIBPATH = \
	 -L$(GDAL_PREFIX)/lib \
	 -L$(GEOS_PREFIX)/lib

LDFLAGS = -lgdal -lgeos

.PHONY= all install tidy clean-test clean prof dist

all: starspan2

# yet to be used
objs : $(OBJS)

starspan2: src/starspan2.cc $(SRCS)
	g++ $(CXXFLAGS) $(OBJ_OPT) \
	    $(LIBPATH) -o starspan2 src/starspan2.cc $(SRCS) $(LDFLAGS)

test:
	(cd tests/ && make)

install:
	install -p starspan starspan2 $(STARSPAN_PREFIX)/bin/

prof: src/starspan2.cc $(SRCS)
	g++ $(PROF) \
	    $(CXXFLAGS) $(OBJ_OPT) \
	    $(LIBPATH) -o starspan2_prof src/starspan2.cc $(SRCS) $(LDFLAGS)


.cc.o:
	g++ -c $(CXXFLAGS) $(OBJ_OPT) $<

dist:
	./mksrcdist.sh $(VERSION)

tidy:
	rm -rf *.o *~ DISTDIR/

clean-test:
	rm -rf tests/generated/

clean: clean-test tidy
	rm -f *.da  starspan2 starspan2_prof

