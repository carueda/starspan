starspan readme
$Id$

Contents

starspan2.cc      Contains the main() function.
starspan_*.cc     With implementation of main operations.
starspan.h        Declaration of main operations.

Interfaces aimed to have some abstraction on raster and
vector objects, although not completely transparent for now:
	vector/       Vector interface implemented on OGR
	raster/       Raster interface implemented on GDAL

traverser/        Observer-based processing mechanism.
                  See starspan_gen_envisl.cc for a client.

jts/              JTS_TestGenerator: generate a test suite suitable for
                  the JTS Test runner




Preliminary approaches for the starspan tool:
	java/         Geotools/JTS based approach: quite interesting but
	              limitations in format support made me decide for
				  a GDAL/OGR/GEOS approach.
	polygon/      tests on CGAL
	libtests/     basic library tests

starspan1.cc  preliminary version of the tool.

