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

#include <set>
#include <list>
#include <vector>
#include <string>
#include <cstdio>

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
  * Note that all pixel locations are 0-based, with (0,0) denoting
  * the upper left corner pixel.
  */
struct TraversalEvent {
	/**
	  * Info about the location of pixels.
	  */
	struct {
		/** 0-based (col,row) location relative to raster image.  */
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

	/**
	  * Called only once at the end of a traversal processing.
	  */
	virtual void end(void) {}
	
};

/**
  * A Traverser intersects every geometry feature in a vector datasource
  * with a raster dataset. Observer should be registered for the
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
  *		// add observers:  
  *		tr.addObserver(observer1);
  *		tr.addObserver(observer2);
  *		...
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
	  *
	  * @param FID  a FID.
	  */
	void setDesiredFID(long FID);

	/**
	  * Only the feature whose given field is equal to the given value
	  * FID will be processed.
	  * This method will have no effect if setDesiredFID(FID) is called with
	  * a valid FID.
	  *
	  * @param field_name
	  * @param field_value
	  */
	void setDesiredFeatureByField(const char* field_name, const char* field_value);
	
	
	/**
	  * Adds an observer to this traverser.
	  */
	void addObserver(Observer* aObserver);

	/**
	  * Gets the number of observers associated to this traverser.
	  */
	unsigned getNumObservers(void) { return observers.size(); } 

	/**
	  * convenience method to delete the observers associated to this traverser.
	  */
	void releaseObservers(void);

	
	/**
	  * Gets the (minimum) size in bytes for a buffer to store 
	  * all band values. The result of this call varies as more
	  * rasters are added to this traverser.
	  *
	  * @return size in bytes.
	  */
	size_t getBandBufferSize(void) {
		return minimumBandBufferSize;
	}
	
	/**
	  * Returns a list of values corresponding to a given list of pixel
	  * locations from a given band.
	  * @param band_index Desired band. Note that 1 corresponds to the first band
	  *              in the first raster.
	  * @param colrows Desired locations.
	  * @return the list of corresponding values. Note that a 0.0 will be
	  *          put in the list where (col,row) is not valid.
	  */
	vector<double>* getPixelValuesInBand(unsigned band_index, vector<CRPixel>* colrows);
	
	/**
	  * Reads in values from all bands (all given rasters) at pixel in (col,row).
	  * Values are stored in bandValues_buffer.
	  *
	  * @param buffer where values are copied.  
	  *      Assumed to have at least getBandBufferSize() bytes allocated.
	  * @return buffer
	  */
	void* getBandValuesForPixel(int col, int row, void* buffer);
	
	/**
	  * Executes the traversal.
	  * This traverser should not be modified while
	  * the traversal is performed. Otherwise undefined
	  * behaviour may occur.
	  */
	void traverse(void);
	
private:
	Vector* vect;
	vector<Raster*> rasts;
	vector<Observer*> observers;
	bool notSimpleObserver;

	double pixelProportion;
	long desired_FID;
	string desired_fieldName;
	string desired_fieldValue;
	
	GlobalInfo globalInfo;
	int width, height;
	double x0, y0, x1, y1;
	double pix_x_size, pix_y_size;
	OGREnvelope raster_env;
	size_t minimumBandBufferSize;
	double* bandValues_buffer;
	LineRasterizer* lineRasterizer;
	void notifyObservers(void);
	void getBandValuesForPixel(int col, int row);
	void toColRow(double x, double y, int *col, int *row);
	void toGridXY(int col, int row, double *x, double *y);
	void processPoint(OGRPoint*);
	void processMultiPoint(OGRMultiPoint*);
	void processLineString(OGRLineString* linstr);
	void processMultiLineString(OGRMultiLineString* coll);
	void processPolygon(OGRPolygon* poly);

	void process_feature(OGRFeature* feature);
	
	// LineRasterizerObserver	
	void pixelFound(double x, double y);

	//
	// Element info for set of visited pixels
	//
	class EPixel {
		int col, row;
		public:
		EPixel(int col, int row) : col(col), row(row) {}
		EPixel(const EPixel& p) : col(p.col), row(p.row) {}
		bool operator<(EPixel const &right) const {
			if ( col < right.col )
				return true;
			else if ( col == right.col )
				return row < right.row;
			else
				return false;
		}
	};
	set<EPixel>* pixset;


};



#endif

