/*
	raster - raster interface
	$Id$
*/
#ifndef Raster_h
#define Raster_h

#include "gdal.h"           
#include "ogr_srs_api.h"

#include <stdio.h>  // FILE


/** Represents a raster. */
class Raster {
public:
	// initializes this module
	static int init(void);
	
	// finishes this module
	static int end(void);
	
	// Creates a raster.
	Raster(const char* filename);
	
	// gets raster size in pixels
	void getSize(int *width, int *height);
	
	// gets raster coordinates
	void getCoordinates(double *x0, double *y0, double *x1, double *y1);
	
	// destroys this raster.
	~Raster();
	
	// for debugging
	void report(FILE* file);
	
private:
	GDALDatasetH hDataset;
	GDALDriverH hDriver;
    const char* pszProjection;
    double adfGeoTransform[6];
	bool geoTransfOK;
	
	void report_corner(FILE* file,const char*,int,int);
};

#endif
