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
  * Info about the location of pixels.
  */
struct PixelLocation {
	/** (col,row) location relative to raster image. */
	int col;
	int row;
	
	/** (x,y) location in geographic coordinates. */
	double x;
	double y;
};

/**
  * Event sent to traversal observers.
  */
struct TraversalEvent {
	PixelLocation pixelLocation;
	
	// data from the scanned raster:
	void* signature;
	GDALDataType rasterType;
	int typeSize;
};

/**
  * Any object interested in doing some task as geometries are
  * traversed must extend this interface.
  */
class Observer {
public:
	virtual ~Observer() {}
	
	/**
	  * Returns true if this observer is only interested in the location of
	  * intersecting pixels, in which case traversal event objects will
	  * be filled with only info about the pixel location.
	  */
	virtual bool isSimple(void) { return false; }
	
	/**
	  * Called only once at the beginning of a raster processing.
	  * @param rasterPoly A simple, 4-vertex polygon covering the
	  * raster extension.
	  */
	virtual void rasterPoly(OGRPolygon* rasterPoly) {}
	
	/**
	  * A new intersecting feature has been found 
	  */
	virtual void intersectionFound(OGRFeature* feature) {}
	
	/**
	  * A new pixel location has been computed.
	  * @param ev Associated event. Pixel location is always provided
	  * but raster info is given only if isSimple() returns false.
	  */
	virtual void addPixel(TraversalEvent& ev) {}

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

	//
	// these class members might be converted to instance members..
	//
	
	/**
	  * Sets the proportion of intersected area required for a pixel to be included.
	  * This parameter is only used during processing of polygons.
	  * By default, a point-in-poly criterion is used: if the polygon contains 
	  * the upper left corner of the pixel, then the pixel is included.
	  *
	  * @param pixprop A value assumed to be in [0.0, 1.0].
	  */
	static void setPixelProportion(double pixprop);

	/**
	  * Only the given FID will be processed.
	  * Useful for debugging.
	  *
	  * @param FID  a FID.
	  */
	static void setDesiredFID(long FID);

	/**
	  * Creates a traverser.
	  * Then you will call setObserver().
	  */
	Traverser(Raster* r, Vector* v);
	
	~Traverser();
	
	/**
	  * Sets the observer for this traverser.
	  */
	void setObserver(Observer* aObserver) { observer = aObserver; }

	/**
	  * Gets the observer associated to this traverser.
	  */
	Observer* getObserver(void) { return observer; }

	/**
	  * Executes the traversal.
	  */
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
	void toGridXY(int col, int row, double *x, double *y);
	void processPoint(OGRPoint*);
	void processMultiPoint(OGRMultiPoint*);
	void processLineString(OGRLineString* linstr);
	void processMultiLineString(OGRMultiLineString* coll);
	void processPolygon(OGRPolygon* poly);
	void processPolygon_point(OGRPolygon* poly);
	void processPolygon_pixel(OGRPolygon* poly);

	void process_feature(OGRFeature* feature);
	
	// LineRasterizerObserver	
	void pixelFound(double x, double y);
};



#endif

