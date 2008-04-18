# for preliminary testing of LayerRasterizer


geosconfig=/usr/local/pkg/geos/bin/geos-config
gdalconfig=/usr/local/pkg/gdal/bin/gdal-config

INC=$(shell $(geosconfig) --cflags)
INC+=$(shell $(gdalconfig) --cflags)

LIBS=$(shell $(geosconfig) --libs)
LIBS+=$(shell $(gdalconfig) --libs)

SRC=LayerRasterizer.cc rpolyqt.cc LineRasterizer.cc ../util/Progress.cc \
    ../raster/Raster_gdal.cc
    
OBJ=LayerRasterizer.o  rpolyqt.o  LineRasterizer.o  Progress.o \
    Raster_gdal.o
    
    
layertest: layertest.cc  $(OBJ)
	g++ -Wall -g $(INC) -I.. -I../util -I../raster -I. \
    layertest.cc  $(OBJ) \
    $(LIBS) \
    -o layertest

runlayertest: layertest
	LD_LIBRARY_PATH=/usr/local/pkg/geos/lib/:/usr/local/pkg/gdal/lib/ \
    ./layertest
	
$(OBJ): $(SRC) LayerRasterizer.h rasterizers.h
	g++ -Wall -g $(INC) -I.. -I../util -I../raster -I. -c $(SRC)
    

clean:
	rm -f *.o *~
