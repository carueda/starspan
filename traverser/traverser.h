//
// Traverse
// Carlos A. Rueda
// $Id$
//

#ifndef traverser_h
#define traverser_h

#include "Raster.h"
#include "Vector.h"
#include "rasterizers.h"

#include <stdio.h>


/**
  * Any object interested in doing some task as geometries are
  * traversed must extend this interface.
  */
class Observer {
public:
	virtual ~Observer() {}
	
	/**
	  * Returns true if this observer is only interested in the location of
	  * intersecting pixels, in which case addPixel(x,y) will be called 
	  * instead of addSignature(x,y,signature,...).
	  */
	virtual bool isSimple(void) { return false; }
	
	/**
	  * Called only once at the beginning of a raster processing. 
	  */
	virtual void rasterPoly(OGRPolygon* rasterPoly) {}
	
	/**
	  * A new intersecting feature has been found 
	  */
	virtual void intersectionFound(OGRFeature* feature) {}
	
	/**
	  * A new pixel location has been computed.
	  * This is called only if isSimple() returns true.
	  */
	virtual void addPixel(double x, double y) {}

	/**
	  * A new signature has been extracted. 
	  * This is called only if isSimple() returns false.
	  */
	virtual void addSignature(double x, double y, void* signature, GDALDataType rasterType, int typeSize) {}
};

/**
  * A Traverser intersects every geometry feature in a vector datasource
  * with a raster dataset. An observer should be registered for the
  * actual work to be done.
  */
class Traverser : LineRasterizerObserver {
public:
	Traverser(Raster* r, Vector* v);
	~Traverser();
	
	/**
	  * Set the observer for this traverser.
	  */
	void setObserver(Observer* aObserver) { observer = aObserver; }
	Observer* getObserver(void) { return observer; }
	
	void traverse(void);
	
private:
	Raster* rast;
	Vector* vect;
	Observer* observer;
	
	GDALDataset* dataset;
	GDALRasterBand* band1;
	GDALDataType rasterType; 
	int rasterTypeSize;
	int width, height, bands;
	double x0, y0, x1, y1;
	double pix_x_size, pix_y_size;
	OGRPolygon raster_poly;
	OGREnvelope raster_env;
	double* signature_buffer;
	LineRasterizer* lineRasterizer;
	void getSignature(int col, int row);
	void toColRow(double x, double y, int *col, int *row);
	void lineString(OGRLineString* linstr);

	// LineRasterizerObserver	
	void pixelFound(double x, double y);
};



#endif

