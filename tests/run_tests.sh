#!/bin/sh
#
# STARSpan project
# Carlos A. Rueda
# run_tests - script to run tests
# $Id$
#

if [ "$1" == "" ]; then
	echo "ERROR: expecting path of starspan executable"
	exit 1
fi

STARSPAN=$1
if [ ! -x $STARSPAN ]; then
	echo "ERROR: cannot execute $STARSPAN"
	exit 1
fi

if [ ! -d data/ ]; then
	echo "ERROR: cannot find data/ directory"
	exit 1
fi

if [ ! -d pre_generated/ ]; then
	echo "ERROR: cannot find pre_generated/ directory"
	exit 1
fi


mkdir -p _generated/

############################################################################
# Mini-raster tests:
mkdir -p _generated/miniraster/

$STARSPAN \
	--vector data/vector/starspan_testply.shp \
	--raster data/raster/starspan2raster.img \
	--mr _generated/miniraster/myprefix_ \
	--in \
	--nodata 1 \
	--fid 3 \
&&
cmp pre_generated/miniraster/myprefix_0003.img \
       _generated/miniraster/myprefix_0003.img




# All OK!
exit 0
