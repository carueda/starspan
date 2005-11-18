LFS test 
Carlos A. Rueda
$Id$ 

lfstest creates a >2Gb-size raster with name BIGRASTER. It skips the
first 2Gb bytes (thus "filling" them with zeroes) and then fills the
trailing cells with ones; then uses GDAL RasterIO to read the two byte
values just on the 2Gb boundary and prints them to stdout. If the
underlying system supports large files and GDAL has been compiled
accordingly, then the printed message should be "01".

To compile and run the test:
	make



