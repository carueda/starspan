//
// StarSpan project
// Carlos A. Rueda
// starspan_dup_pixel - generate a CSV file with handling of duplicate pixels
// $Id$
//

#include "starspan.h"
#include "traverser.h"
#include "Csv.h"

#include <stdlib.h>
#include <assert.h>
#include <algorithm>


#define PI 3.14159265
#define DISTANCE_EPS 10e-5


/**
 * Maximum difference in direction to consider a new candidate
 * w.r.t. the best candidate
 */
// TODO global option
static const double maxDegreeDifference = 5;

/**
 * Maximum difference in % distance to consider a new candidate w.r.t. the best candidate, ie.,
 *  if candidate.distance < (maxDistancePercentage + 1) * best_candidate.distance, then
 *	 choose also candidate
 */
// TODO global option
static const double maxDistancePercentage = 5;




// obtained in starspan_csv_dup_pixel() and used in do_extraction():
static Vector* vect;
static vector<const char*>* select_fields;
static const char* csv_filename;
static int layernum;


////////////////////////
// info for each raster
struct RasterInfo {
	// filename
	const char* raster_filename;
	
	// raster
	Raster* rast;
	
	// bounding box
	OGRPolygon bb;
	
	// center of bb;
	OGRPoint center;
	
	// distance: set during evaluation as a candidate
	double distance;
};



/**
 * Gets the center of the geometry. 
 */
static OGRPoint getGeometryCenter(OGRGeometry* geometry) {
	OGREnvelope env;
	geometry->getEnvelope(&env);
	OGRPoint center;
	center.setX((env.MinX + env.MaxX) / 2);
	center.setY((env.MinY + env.MaxY) / 2);
	return center;
}

/**
 * Processes the given feature extracting pixels from the figen raster. 
 */
static void do_extraction(OGRFeature* feature, RasterInfo& rasterInfo) {

	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Call starspan_csv() 
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(rasterInfo.raster_filename);
	
	// - Call starspan_csv() 
	int ret = starspan_csv(
		vect,
		raster_filenames,
		select_fields,
		csv_filename,
		layernum
	);
	
	if ( globalOptions.verbose ) {
		cout<< "--duplicate_pixel: do_extraction: starspan_csv returned: " <<ret<< endl;
	}
}
		

/**
 * Gets the angle of the vector p2-p1 in degrees
 */
static inline double getAngle(OGRPoint& p1, OGRPoint& p2) {
	double theta = atan2(p2.getY() - p1.getY(), p2.getX() - p1.getX());  
	return theta * 180 / PI;
}

/**
 * Gets the distance between the given arguments.
 */
static inline double getDistance(OGRPoint& origin, OGRPoint& p) {
	return origin.Distance(&p);
}


/**
 * To sort candidates list
 */
static bool comparator(RasterInfo ri1, RasterInfo ri2) {
	return ri1.distance < ri2.distance;
}
			    
/**
 * Applies the duplicate pixel modes to the given feature 
 */
static void process_modes_feature(
		vector<DupPixelMode>& dupPixelModes,
		OGRFeature* feature, 
		vector<RasterInfo>& rastInfos) {
	

	bool ignore_nodata = false;
	
	// a pre-check of given modes:
	for (int k = 0, count = dupPixelModes.size(); k < count; k++ ) {
		string& code = dupPixelModes[k].code;
		
		assert( code == "ignore_nodata" || code == "direction" || code == "distance" );
		
		if ( code == "ignore_nodata" ) {
			ignore_nodata = true;
			break;
		}
	}
	
	///////////////////////////////////////////////////////////////////
	// ignore_nodata?
	// TODO Not yet implemented.  Mode IGNORED at this time.
	if ( ignore_nodata ) {
		cout<< "--duplicate_pixel: Warning: ignore_nodata ignored, not yet implemented."<<endl;
	}

	//
	// get geometry
	//
	OGRGeometry* feature_geometry = feature->GetGeometryRef();
	
	// get center of feature's bounding box:
	OGRPoint feature_center = getGeometryCenter(feature_geometry);
	
	///////////////////////////////////////////////////////////////////
	// --buffer option given?
	// this operation is IGNORED here -- see traverser.cc
	if ( globalOptions.bufferParams.given ) {
		cout<< "--duplicate_pixel: Warning: buffer operation ignored for selection of raster."<<endl;
		cout<< "     But it will be applied before extraction."<<endl;
	}
	
	
	/////////////////////////////////////////////////////////////////
	// initialize candidates with the rasters containing the feature:
	vector<RasterInfo> candidates;
	for ( unsigned i = 0; i < rastInfos.size(); i++ ) {
		RasterInfo& rasterInfo = rastInfos[i];
		if ( rasterInfo.bb.Contains(feature_geometry) ) {
			candidates.push_back(rasterInfo);
		}
	}
	
	if ( candidates.size() == 0 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": No raster containing the feature" << endl;
		}
		return;
	}
	
	RasterInfo* selectedRasterInfo = 0;
	
	if ( candidates.size() == 1 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": Only one raster contains the feature" << endl;
			cout<< "   No need to apply duplicate pixel mode(s)" << endl;
		}
		selectedRasterInfo = &candidates[0];
	}
	else {
		/////////////////////////////////////////////////////////
		// Apply given modes until one candidate is obtained
		
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: Applying duplicate pixel mode(s) to FID " <<feature->GetFID()<< "..." << endl;
		}
		
		// apply each mode in order while there are more than one candidate:
		for ( unsigned m = 0; m < dupPixelModes.size(); m++ ) {
			DupPixelMode& mode = dupPixelModes[m];
			
			if ( candidates.size() <= 1 ) {
				break;
			}
			
			if ( globalOptions.verbose ) {
				cout<< "--duplicate_pixel: Applying duplicate pixel mode: " <<mode.code<< " " <<mode.arg<< endl;
			}
				
			// evaluate candidates according to mode:
			for (unsigned i = 0; i < candidates.size(); i++ ) {
				RasterInfo& candidate = candidates[i];
				
				double distance = getDistance(feature_center, candidate.center);
				
				if ( mode.code == "direction" ) {
					// if the distance is "zero", ie.,  within an epsilon...
					if ( distance <= DISTANCE_EPS ) {
						// ... then, assume the angle is the desired one:
						candidate.distance = 0;
					}
					else {
						double theta = getAngle(feature_center, candidate.center);
						candidate.distance = fabs(theta - mode.arg);
					}
				}
				if ( mode.code == "distance" ) {
					candidate.distance = getDistance(feature_center, candidate.center);
				}
				else if ( mode.code == "ignore_nodata" ) {
					// nothing to do here. Should be handled above.
				}
			}
			
			// sort evaluations in order of increasing distance
			sort(candidates.begin(), candidates.end(), comparator);


			// Get new candidates:			
			vector<RasterInfo> newCandidates;
			
			// at least, the first (best) candidate is selected:
			newCandidates.push_back(candidates[0]);
			
			// but, add next ones if almost as good:
			for (unsigned i = 1; i < candidates.size(); i++ ) {
				if ( mode.code == "direction" ) {
					// Take candidate if |difference| <= maxDegreeDifference
					if ( fabs(candidates[i].distance - candidates[0].distance) <= maxDegreeDifference ) {
						newCandidates.push_back(candidates[i]);
					}
				}
				else {
					// Take candidate if distance <= acceptable distance:
					if ( candidates[i].distance <= (maxDistancePercentage + 1) * candidates[0].distance ) {
						newCandidates.push_back(candidates[i]);
					}
				}
			}
			
			// update candidates list:
			candidates.clear();
			for (unsigned i = 0; i < newCandidates.size(); i++ ) {
				candidates.push_back(newCandidates[i]);
			}
		}
		
		assert( candidates.size() > 0 );
		
		selectedRasterInfo = &candidates[0];
		
		if ( candidates.size() > 1 ) {
			if ( globalOptions.verbose ) {
				cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": More than one raster satisfy the conditions." << endl;
				cout<< "   First best candidate will be chosen for extraction." << endl;
			}
		}
	}
	
	assert( selectedRasterInfo != 0 ) ;
	
	// we have our selected raster:
	do_extraction(feature, *selectedRasterInfo);
}



////////////////////////////////////////////////////////////////////////////////

//
// FR 350112: Duplicate pixel handling.
// Only one raster is chosen (if possible) to get pixel data for the feature.
//
int starspan_csv_dup_pixel(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*>* _select_fields,
	const char* _csv_filename,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes
) {
	vect = _vect;
	select_fields = _select_fields;
	csv_filename = _csv_filename;
	layernum = _layernum;
	
	
	////////////
	// Get layer	
	OGRLayer* layer = vect->getLayer(layernum);
	if ( !layer ) {
		cerr<< "Couldn't get layer " <<layernum<< " from " << vect->getName()<< endl;
		return 1;
	}
	layer->ResetReading();
	
	
	// Open rasters, corresponding bounding boxes, and union of all boxes:
	vector<RasterInfo> rastInfos;
	OGRGeometry* allRasterArea = new OGRPolygon();
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		RasterInfo rasterInfo;
		rastInfos.push_back(rasterInfo);

		rasterInfo.raster_filename = raster_filenames[i];
		rasterInfo.rast = new Raster(rasterInfo.raster_filename);

		// create a geometry for raster envelope:
		double x0, y0, x1, y1;		
		rasterInfo.rast->getCoordinates(&x0, &y0, &x1, &y1);
		OGRLinearRing raster_ring;
		raster_ring.addPoint(x0, y0);
		raster_ring.addPoint(x1, y0);
		raster_ring.addPoint(x1, y1);
		raster_ring.addPoint(x0, y1);
		rasterInfo.bb.addRing(&raster_ring);
		rasterInfo.bb.closeRings();
		rasterInfo.center = getGeometryCenter(&rasterInfo.bb);
		
		OGRGeometry* newAllRasterArea = allRasterArea->Union(&rasterInfo.bb);
		delete allRasterArea;
		allRasterArea = newAllRasterArea;
	}
	
	if ( globalOptions.verbose ) {
		cout<< "--duplicate_pixel: Setting spatial filter" << endl;
	}
	layer->SetSpatialFilter(allRasterArea);
	
	
	// Now, traverse the features:
	OGRFeature* feature;
	
	//
	// Was a specific FID given?
	//
	if ( globalOptions.FID >= 0 ) {
		// TODO -- IGNORED here -- see traverser.cc
		cout<< "--duplicate_pixel: Sorry: --fid option not yet implemented" <<endl;
		return 2;
	}
	
	//
	// Was a specific field name/value given?
	// TODO -- IGNORED here -- see traverser.cc
	//
	else if ( false ) {
	}
	
	//
	// else: process each feature in vector datasource:
	//
	else {
		while( (feature = layer->GetNextFeature()) != NULL ) {
			process_modes_feature(dupPixelModes, feature, rastInfos);
			delete feature;
		}
	}
	
	// release rasters
	for ( unsigned i = 0; i < rastInfos.size(); i++ ) {
		delete rastInfos[i].rast;
	}
	
	return 0;
}

