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

#include <list>
#include <vector>
#include <stdio.h>

using namespace std;


/**
  * Info passed in observer#init(info)
  */
struct GlobalInfo {
	
	/** info about all bands in given rasters */
	vector<GDALRasterBand*> bands;
	
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
  *
  * synopsis of usage:
  *
  * <pre>
  *		// create traverser:
  *		Traverser tr;
  *
  *		// give inputs:
  *		tr.setVector(v);
  *		tr.addRaster(r1);
  *		tr.addRaster(r2);
  *		...
  *		
  *		// optionally
  *		tr.setPixelProportion(pp);
  *		tr.setDesiredFID(fid);
  *
  *		// set the observer:  
  *		tr.setObserver(observer);
  *
  *		// run processing:
  *		tr.traverse();
  * </pre>
  */
class Traverser : LineRasterizerObserver {
public:

	/**
	  * Creates a traverser.
	  */
	Traverser(void);
	
	~Traverser();
	
	/**
	  * Sets the vector datasource.
	  * (eventually this would become addVector)
	  * @param v vector datadource
	  */
	  void setVector(Vector* vector);

	/**
	  * Gets the vector associated to this traverser.
	  */
	Vector* getVector(void) { return vect; }
	
	/**
	  * Adds a raster.
	  * -- NOTE: only first one added is processed until complete impl is done.
	  * @param r the raster dataset
	  */
	void addRaster(Raster* raster);
	
	/**
	  * Gets the number of rasters.
	  */
	int getNumRasters(void) { return rasts.size(); }

	/**
	  * Gets a raster from the list of rasters.
	  */
	Raster* getRaster(int i) { return rasts[i]; }
	
	
	/**
	  * Sets the proportion of intersected area required for a pixel to be included.
	  * This parameter is only used during processing of polygons.
	  * By default, a point-in-poly criterion is used: if the polygon contains 
	  * the upper left corner of the pixel, then the pixel is included.
	  *
	  * @param pixprop A value assumed to be in [0.0, 1.0].
	  */
	void setPixelProportion(double pixprop);

	/**
	  * Only the given FID will be processed.
	  * Useful for debugging.
	  *
	  * @param FID  a FID.
	  */
	void setDesiredFID(long FID);

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
	Vector* vect;
	vector<Raster*> rasts;
	Observer* observer;

	double pixelProportion;
	long desired_FID;
	
	GlobalInfo globalInfo;
	int width, height;
	double x0, y0, x1, y1;
	double pix_x_size, pix_y_size;
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

