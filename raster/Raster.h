/*
	raster - raster interface
	$Id$
*/
#ifndef Raster_h
#define Raster_h

#include "gdal.h"           
#include "gdal_priv.h"
#include "ogr_srs_api.h"

#include <stdio.h>  // FILE


/** Represents a raster. */
class Raster {
public:
	// initializes this module
	static int init(void);
	
	// finishes this module
	static int end(void);
	
	// Creates a raster object representing an existing file.
	Raster(const char* filename);
	
	// Creates a raster object representing a new raster file.
	Raster(const char* filename, int width, int height, int bands);
	
	
	GDALDataset* getDataset(void) { return hDataset; }
	
	// gets raster size in pixels and number of bands
	void getSize(int *width, int *height, int *bands);
	
	// gets raster coordinates
	void getCoordinates(double *x0, double *y0, double *x1, double *y1);
	
	// closes this raster.
	~Raster();
	
	// for debugging
	void report(FILE* file);
	
private:
	GDALDataset* hDataset;
    const char* pszProjection;
    double adfGeoTransform[6];
	bool geoTransfOK;
	
	void report_corner(FILE* file,const char*,int,int);
};

#endif
