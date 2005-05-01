#
# Makefile for tests
# $Id$
#

STARSPAN=../starspan

.PHONY: test init test_miniraster test_miniraster_strip

ALL_TESTS: init test_miniraster test_miniraster_strip
	@echo "!!!!!!! $@ : OK !!!!!!!"

init:
	chmod +x $STARSPAN
	mkdir -p generated/

test_miniraster:
	mkdir -p generated/miniraster/
	${STARSPAN} \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan2raster.img \
		--mini_rasters generated/miniraster/myprefix_ \
		--in \
		--nodata 1 \
		--fid 3 
	cmp expected/miniraster/myprefix_0003.img \
	   generated/miniraster/myprefix_0003.img
	${STARSPAN} \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan2raster.img \
		--mini_rasters generated/miniraster/myprefix_buff2_ \
		--in \
		--buffer 2 \
		--fid 3
	cmp expected/miniraster/myprefix_buff2_0003.img \
	   generated/miniraster/myprefix_buff2_0003.img
	@echo "$@ : OK"
	@echo
	
test_miniraster_strip:
	mkdir -p generated/mrstrip/
	${STARSPAN} \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan2raster.img \
		--mini_raster_strip generated/mrstrip/myoutput \
		--in
	cmp expected/mrstrip/myoutput_mr.img \
	   generated/mrstrip/myoutput_mr.img
	cmp expected/mrstrip/myoutput_mrid.img \
	   generated/mrstrip/myoutput_mrid.img
	cmp expected/mrstrip/myoutput_mrloc.glt \
	   generated/mrstrip/myoutput_mrloc.glt
	@echo "$@ : OK"
	@echo
	
