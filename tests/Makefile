#
# Makefile for tests
# $Id$
#

STARSPAN=../starspan

test: init test_miniraster

init:
	mkdir -p _generated/

test_miniraster:
	mkdir -p _generated/miniraster/
	${STARSPAN} \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan2raster.img \
		--mr _generated/miniraster/myprefix_ \
		--in \
		--nodata 1 \
		--fid 3 
	cmp pre_generated/miniraster/myprefix_0003.img \
		   _generated/miniraster/myprefix_0003.img
	${STARSPAN} \
		--vector data/vector/starspan_testply.shp \
		--raster data/raster/starspan2raster.img \
		--mr _generated/miniraster/myprefix_buff2_ \
		--in \
		--buffer 2 \
		--fid 3
	cmp pre_generated/miniraster/myprefix_buff2_0003.img \
		   _generated/miniraster/myprefix_buff2_0003.img

