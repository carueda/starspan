starspan readme
$Id$

Contents

traverser/    Observer-based processing mechanism.
              This is the core of the starspan library.
            
starspan.h    Declaration of main operations.
            
starspan2.cc  Contains the main() function.

starspan_db.cc            DBF creation
starspan_gen_envisl.cc    Envi spectral library creation
starspan_minirasters.cc   miniraster creation
starspan_jtstest.cc       JTS test suit creation
starspan_util.cc          misc supporting utilities

Interfaces aimed to have some abstraction on raster and
vector objects, although not completely transparent for now:
	vector/       Vector interface implemented on OGR
	raster/       Raster interface implemented on GDAL

jts/              JTS_TestGenerator: generates a test suite suitable for
                  the JTS Test runner




Previous approaches for the starspan tool:
	java/         Geotools/JTS based approach: quite interesting but
	              limitations in format support made me decide for
				  a GDAL/OGR/GEOS approach.
	polygon/      tests on CGAL
	libtests/     basic library tests

	starspan1.cc  preliminary version of the tool.

