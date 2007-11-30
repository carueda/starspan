#
# Makefile for tests
# $Id$
#

STARSPAN=../starspan

.PHONY: test init test_csv test_stats test_miniraster test_miniraster_strip

ALL_TESTS: init test_csv test_stats test_miniraster test_miniraster_strip
	@echo "!!!!!!! $@ : OK !!!!!!!"

init:
	$(shell if [ ! -x ${STARSPAN} ]; then chmod +x ${STARSPAN}; fi)
	mkdir -p generated/

test_csv:
	mkdir -p generated/csv/
	rm -f generated/csv/*.csv
	$(shell if [ -f expected/csv/myoutput.csv.gz ]; then gunzip expected/csv/myoutput.csv.gz; fi)
	${STARSPAN} \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan[1-3]raster.img \
		--csv generated/csv/myoutput.csv \
	diff expected/csv/myoutput.csv generated/csv/myoutput.csv
	@echo "$@ : OK"
	@echo
	
test_stats:
	mkdir -p generated/stats/
	rm -f generated/stats/*.csv
	$(shell if [ -f expected/stats/myoutput.csv.gz ]; then gunzip expected/stats/myoutput.csv.gz; fi)
	${STARSPAN} \
		--verbose \
		--fields none \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan[1-3]raster.img \
		--nodata 0 \
		--stats generated/stats/myoutput.csv avg mode stdev min max sum median nulls
	diff expected/stats/myoutput.csv generated/stats/myoutput.csv
	@echo "$@ : OK"
	@echo
	
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
	
