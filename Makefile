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

INCLUDE := -Isrc -Ivector -Iraster -Itraverser -Irasterizers -Ijts -Icsv -Istats -Iutil

INCLUDE += -I$(GDAL_PREFIX)/include
INCLUDE += -I$(GDAL_SRC_DIR)/frmts
INCLUDE += -I$(GDAL_SRC_DIR)

INCLUDE += -I/usr/local/include


SRCS =	src/starspan_stats.cc \
		src/starspan_update_csv.cc \
		src/starspan_csv.cc \
		src/starspan_csv_raster_field.cc \
		src/starspan_gen_envisl.cc \
		src/starspan_minirasters.cc \
		src/starspan_jtstest.cc \
		src/starspan_util.cc \
		src/starspan_dump.cc \
		src/starspan_tuct2.cc \
		jts/jts.cc \
		traverser/traverser.cc \
		raster/Raster_gdal.cc \
		vector/Vector_ogr.cc \
		rasterizers/LineRasterizer.cc \
		csv/Csv.cc \
		stats/Stats.cc \
		util/Progress.cc


OBJS = $(subst .cc,.o,$(SRCS))

CXXFLAGS = -g -Wall \
		   $(INCLUDE)

PROF = -pg -fprofile-arcs

LIBPATH = \
	 -L$(GDAL_PREFIX)/lib \
	 -L$(GEOS_PREFIX)/lib

LDFLAGS = -lgdal -lgeos

.PHONY= all install tidy clean-test clean prof

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

VERSION=""
cvsroot=":pserver:anonymous@cvs.casil.ucdavis.edu:/cvsroot/starspan"

dist: DISTDIR

DISTDIR:
	$(shell \
	mkdir -p DISTDIR &&\
	cd DISTDIR &&\
	cvs -d$(cvsroot) login && cvs -z3 -d$(cvsroot) co starspan &&\
	(find starspan -name CVS -exec rm -rf {} \; 2>/dev/null || /bin/true) &&\
	mv starspan starspan-$(VERSION) &&\
	tar cf starspan-$(VERSION).tar starspan-$(VERSION) &&\
	gzip -9 starspan-$(VERSION).tar &&\
	rm -rf starspan-$(VERSION) && ls -l )
	echo "Done."

tidy:
	rm -rf *.o *~ DISTDIR/

clean-test:
	rm -rf tests/generated/

clean: clean-test tidy
	rm -f *.da  starspan2 starspan2_prof

