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

#include <list.h>   // should be <list>
#include <stdio.h>


/**
  * Info passed in observer#init(info)
  */
struct GlobalInfo {
	
	/** Band info in raster */
	struct {
		GDALDataType type;
		int typeSize;
	} band;
	
	/** A rectangle polygon covering the raster extension. */
	OGRPolygon rasterPoly;
};
	
	
/**
  * Event sent to traversal observers every time an intersecting
  * pixel is found.
  */
struct TraversalEvent {
	/**
	  * Info about the location of pixels.
	  */
	struct {
		/** (col,row) location relative to raster image. 
		  * Upper left corner is (1,1)
		  */
		int col;
		int row;
		
		/** (x,y) location in geographic coordinates. */
		double x;
		double y;
	} pixel;
	
	
	/**
	  * data from pixel in scanned raster.
	  * Only assigned if observer#isSimple() is false.
	  */
	void* bandValues;
};

/**
  * Any object interested in doing some task as geometries are
  * traversed must implement this interface.
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
	  * Called only once at the beginning of a traversal processing.
	  */
	virtual void init(GlobalInfo& info) {}
	
	/**
	  * A new intersecting feature has been found 
	  */
	virtual void intersectionFound(OGRFeature* feature) {}
	
	/**
	  * A new pixel location has been computed.
	  * @param ev Associated event. Pixel location is always provided
	  * but raster bands are given only if isSimple() returns false.
	  */
	virtual void addPixel(TraversalEvent& ev) {}
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
	  *
	  * @param r rasters -- NOTE: only first one used while complete impl is done
	  * @param v vector
	  */
	Traverser(list<Raster*>* r, Vector* v);
	
	~Traverser();
	
	/**
	  * Sets the observer for this traverser.
	  */
	void setObserver(Observer* aObserver) { observer = aObserver; }

	/**
	  * Gets the observer associated to this traverser.
	  */
	Observer* getObserver(void) { return observer; }

	Raster* getRaster(void) { return rast; }
	Vector* getVector(void) { return vect; }
	
	
	/**
	  * Executes the traversal.
	  */
	void traverse(void);
	
private:
	list<Raster*>* rasts;
	Raster* rast;  // transitional member
	Vector* vect;
	Observer* observer;
	
	GDALDataset* dataset;
	GDALRasterBand* band1;
	GDALDataType bandType; 
	int bandTypeSize;
	int width, height, bands;
	double x0, y0, x1, y1;
	double pix_x_size, pix_y_size;
	
	GlobalInfo globalInfo;
	OGREnvelope raster_env;
	double* bandValues_buffer;
	LineRasterizer* lineRasterizer;
	void getBandValues(int col, int row);
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

