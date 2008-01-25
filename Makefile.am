#########################################################
# starspan Makefile.am 
# $Id$
#########################################################

bin_PROGRAMS = starspan2
bin_SCRIPTS  = starspan
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST   = starspan

starspan2_SOURCES =	\
	src/starspan2.cc \
	src/starspan_csv_dup_pixel.cc \
	src/starspan_stats.cc \
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
	src/csv/CsvOutput.cc \
	src/jts/jts.cc \
	src/raster/Raster_gdal.cc \
	src/rasterizers/LineRasterizer.cc \
	src/stats/Stats.cc \
	src/traverser/traverser.cc \
	src/traverser/polyqt.cc \
	src/traverser/pixset.cc \
	src/util/Progress.cc \
	src/vector/Vector_ogr.cc

AM_CPPFLAGS = -g @GEOS_INC@  @GDAL_INC@

INCLUDES = \
	-I$(srcdir)/src \
	-I$(srcdir)/src/csv \
	-I$(srcdir)/src/jts \
	-I$(srcdir)/src/raster \
	-I$(srcdir)/src/rasterizers \
	-I$(srcdir)/src/stats \
	-I$(srcdir)/src/traverser \
	-I$(srcdir)/src/util \
	-I$(srcdir)/src/vector

