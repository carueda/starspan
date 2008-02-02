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


/**
 * Gets the center of the geometry. 
 */
static OGRPoint* getGeometryCenter(OGRGeometry* geometry) {
	OGREnvelope env;
	geometry->getEnvelope(&env);
	OGRPoint* center = new OGRPoint();
	center->setX((env.MinX + env.MaxX) / 2);
	center->setY((env.MinY + env.MaxY) / 2);
	return center;
}

////////////////////////
// info for each raster
struct RasterInfo {
    const int ri_idx;
    
	// raster filename
	const char* ri_filename;
	
	// mask filename
	const char* ri_mask_filename;
	
	// mask
	Raster* ri_mask;
	
    
	// bounding box
	OGRPolygon* ri_bb;
	
	// center of ri_bb;
	OGRPoint* ri_center;
	
	// distance: set during evaluation as a candidate
	double distance;
	
	RasterInfo(int idx, const char* raster_filename, const char* mask_filename)
	: ri_idx(idx), ri_filename(raster_filename), ri_mask_filename(mask_filename) {
        
        if ( ri_mask_filename ) {
            ri_mask = new Raster(ri_mask_filename);
        }
        
        // open the raster momentarily to get its bounding box and center:
		Raster ri_raster(ri_filename);
        
		// create a geometry for raster envelope:
		double x0, y0, x1, y1;		
		ri_raster.getCoordinates(&x0, &y0, &x1, &y1);
		OGRLinearRing* raster_ring = new OGRLinearRing();
		raster_ring->addPoint(x0, y0);
		raster_ring->addPoint(x1, y0);
		raster_ring->addPoint(x1, y1);
		raster_ring->addPoint(x0, y1);
		raster_ring->addPoint(x0, y0);
		ri_bb = new OGRPolygon();
		ri_bb->addRingDirectly(raster_ring);
		
		ri_center = getGeometryCenter(ri_bb);
	}
    
	~RasterInfo() {
		delete ri_bb;
		delete ri_center;
        
        if ( ri_mask ) {
            delete ri_mask;
        }
	}
};



/**
  * Checks bands for zero values. If a zero is found, it records its
  * location [col0, row0].
  */
struct MaskObserver : public Observer {
	GlobalInfo* global_info;
	bool OK;
    bool zeroFound;
    
    int col0;
    int row0;
		
	MaskObserver() : global_info(0), zeroFound(false) {
	}
	
	void init(GlobalInfo& info) {
		global_info = &info;

		OK = false;   // but let's see ...
		
		if ( global_info->bands.size() == 0 ) {
			cerr<< "MaskObserver: warning: no bands in raster mask" <<endl;
			return;
		}

		// now, all seems OK to continue processing		
		OK = true;
	}
	
	void addPixel(TraversalEvent& ev) { 
        if ( !OK || zeroFound ) {
            return;
        }
		void* band_values = ev.bandValues;
		
		// check bands for any zero value
		char* ptr = (char*) band_values;
		char value[1024];
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			int typeSize = GDALGetDataTypeSize(bandType) >> 3;
			
			int value = starspan_extract_int_value(bandType, ptr);
            if ( value == 0 ) {
                zeroFound = true;
                col0 = 1 + ev.pixel.col;
                row0 = 1 + ev.pixel.row;
                break;
            }
			
			// move to next piece of data in buffer:
			ptr += typeSize;
		}
	}

};




/**
 * Determines whether the feature is fully contained in the raster
 * according to the mask. 
 */
static bool within_mask(OGRFeature* feature, RasterInfo* rasterInfo) {
    assert( rasterInfo->ri_mask ) ;

	// strategy:
    // - Traverse feature with a MaskObserver to detect if a zero value appears
    
    MaskObserver obs;
    
	Traverser tr;
	tr.addObserver(&obs);

	tr.setVector(vect);
	tr.setLayerNum(layernum);
	tr.setDesiredFID(feature->GetFID());
    
	if ( globalOptions.pix_prop >= 0.0 )
		tr.setPixelProportion(globalOptions.pix_prop);
	tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);
    
    tr.addRaster(rasterInfo->ri_mask);
    
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;

    tr.traverse();
    
    Traverser::_resetReading = prevResetReading;
    
    return obs.OK && !obs.zeroFound;
}
		




/**
 * Processes the given feature extracting pixels from the given raster. 
 */
static void do_extraction(OGRFeature* feature, RasterInfo* rasterInfo, const char* csv_filename) {

	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Call starspan_csv() 
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(rasterInfo->ri_filename);
	
	// - Call starspan_csv()
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;
	int ret = starspan_csv(
		vect,
		raster_filenames,
		select_fields,
		csv_filename,
		layernum
	);
	Traverser::_resetReading = prevResetReading;
	
	if ( globalOptions.verbose ) {
		cout<< "--duplicate_pixel: do_extraction: starspan_csv returned: " <<ret<< endl;
	}
}
		

/**
 * Gets the angle of the vector p2-p1 in degrees
 */
static inline double getAngle(OGRPoint* p1, OGRPoint* p2) {
	double theta = atan2(p2->getY() - p1->getY(), p2->getX() - p1->getX());  
	return theta * 180 / PI;
}

/**
 * Gets the distance between the given arguments.
 */
static inline double getDistance(OGRPoint* origin, OGRPoint* p) {
	return origin->Distance(p);
}


/**
 * To sort candidates list
 */
static bool comparator(RasterInfo* ri1, RasterInfo* ri2) {
	return ri1->distance < ri2->distance;
}
			    
/**
 * Applies the duplicate pixel modes to the given feature 
 */
static void process_modes_feature(
		vector<DupPixelMode>& dupPixelModes,
		OGRFeature* feature, 
		vector<RasterInfo*>& rastInfos) {
	

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
	
	if ( globalOptions.verbose ) {
        cout<< endl
		    << "___________________________________________________" <<endl
            << "--duplicate_pixel: FID " <<feature->GetFID()<< endl;
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
	OGRPoint* feature_center = getGeometryCenter(feature_geometry);
	
	///////////////////////////////////////////////////////////////////
	// --buffer option given?
	// this operation is IGNORED here -- see traverser.cc
	if ( globalOptions.bufferParams.given ) {
		cout<< "--duplicate_pixel: Warning: buffer operation ignored for selection of raster."<<endl;
		cout<< "     But it will be applied before extraction."<<endl;
	}
	
	
	/////////////////////////////////////////////////////////////////
	// initialize candidates with the rasters containing the feature:
	vector<RasterInfo*> candidates;
	if ( globalOptions.verbose ) {
        cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": Checking " <<rastInfos.size()<< " rasters for containment" <<endl;
	}
	for ( unsigned i = 0, numRasters = rastInfos.size(); i < numRasters; i++ ) {
		RasterInfo* rasterInfo = rastInfos[i];
		bool containsFeature = false;
		if ( rasterInfo->ri_bb->Contains(feature_geometry) ) {
			candidates.push_back(rasterInfo);
            if ( globalOptions.verbose ) {
                cout<< "\t" << "  " <<rasterInfo->ri_filename<< endl;
            }
		}
	}
	
	if ( candidates.size() == 0 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": No raster containing the feature" << endl;
		}
		delete feature_center;
		return;
	}
    
    
	/////////////////////////////////////////////////////////////////
	// if masks are given, then check that all pixels in features contain actual
    // data according to the masks:
	if ( globalOptions.verbose ) {
        cout<< "--duplicate_pixel: Selecting according to masks, if given:" <<endl;
	}
    vector<RasterInfo*> newCandidates;
    for ( unsigned i = 0, numRasters = candidates.size(); i < numRasters; i++ ) {
        RasterInfo* rasterInfo = candidates[i];
        if ( !rasterInfo->ri_mask || within_mask(feature, rasterInfo) ) {
            newCandidates.push_back(rasterInfo);
            if ( globalOptions.verbose ) {
                cout<< "\t" << "  " <<rasterInfo->ri_filename<< endl;
            }
        }
    }
    // update candidates list:
    candidates.clear();
    for (unsigned i = 0; i < newCandidates.size(); i++ ) {
        candidates.push_back(newCandidates[i]);
    }
	
    
	if ( candidates.size() == 0 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": No raster containing the feature" << endl;
		}
		delete feature_center;
		return;
	}
    
    
    ///////
	
	RasterInfo* selectedRasterInfo = 0;
	
	if ( candidates.size() == 1 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": Only one raster contains the feature" << endl;
			cout<< "   No need to apply duplicate pixel mode(s)" << endl;
		}
		selectedRasterInfo = candidates[0];
	}
	else {
		/////////////////////////////////////////////////////////
		// Apply given modes until one candidate is obtained
		
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: Applying duplicate pixel mode(s) to FID " <<feature->GetFID()<< ": " 
			    <<candidates.size()<< " initial candidates..." <<endl;
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
				RasterInfo* candidate = candidates[i];
				
				double distance = getDistance(feature_center, candidate->ri_center);
				
				if ( mode.code == "direction" ) {
					// if the distance is "zero", ie.,  within an epsilon...
					if ( distance <= DISTANCE_EPS ) {
						// ... then, assume the angle is the desired one:
						candidate->distance = 0;
					}
					else {
						double theta = getAngle(feature_center, candidate->ri_center);
						candidate->distance = fabs(theta - mode.arg);
					}
				}
				if ( mode.code == "distance" ) {
					candidate->distance = getDistance(feature_center, candidate->ri_center);
				}
				else if ( mode.code == "ignore_nodata" ) {
					// nothing to do here. Should be handled above.
				}
			}
			
			// sort evaluations in order of increasing distance
			sort(candidates.begin(), candidates.end(), comparator);


			// Get new candidates:			
			newCandidates.clear();
			
			// at least, the first (best) candidate is selected:
			newCandidates.push_back(candidates[0]);
			
			// but, add next ones if almost as good:
			for (unsigned i = 1; i < candidates.size(); i++ ) {
				if ( mode.code == "direction" ) {
					// Take candidate if |difference| <= maxDegreeDifference
                    // (note: no need for fabs())
					if ( candidates[i]->distance - candidates[0]->distance <= maxDegreeDifference ) {
						newCandidates.push_back(candidates[i]);
					}
				}
				else {
					// Take candidate if distance <= acceptable distance:
					if ( candidates[i]->distance <= (maxDistancePercentage + 1) * candidates[0]->distance ) {
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
		
		selectedRasterInfo = candidates[0];
		
		if ( candidates.size() > 1 ) {
			if ( globalOptions.verbose ) {
				cout<< "--duplicate_pixel: FID " <<feature->GetFID()<< ": More than one raster satisfy the conditions." << endl;
				cout<< "   First best candidate will be chosen for extraction." << endl;
			}
		}
	}
	
	assert( selectedRasterInfo != 0 ) ;
	
	delete feature_center;
	
	// we have our selected raster:
	do_extraction(feature, selectedRasterInfo, csv_filename);
}



////////////////////////////////////////////////////////////////////////////////

//
// FR 200337: Duplicate pixel handling.
// Only one raster is chosen (if possible) to get pixel data for the feature.
//
int starspan_csv_dup_pixel(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	const char* _csv_filename,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes
) {
    // Either no masks are given, or the number of rasters and masks are the same:
    assert( mask_filenames == 0 || mask_filenames->size() == raster_filenames.size() );
    
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
	vector<RasterInfo*> rastInfos;
	OGRGeometry* allRasterArea = new OGRPolygon();
	
	int res = 0;
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
        const char* mask_filename = 0;
        if ( mask_filenames ) {
            mask_filename = (*mask_filenames)[i];
        }
		RasterInfo* rasterInfo = new RasterInfo(i, raster_filenames[i], mask_filename);

		OGRGeometry* newAllRasterArea = allRasterArea->Union(rasterInfo->ri_bb);
		delete allRasterArea;
		allRasterArea = newAllRasterArea;
		
		rastInfos.push_back(rasterInfo);
	}
	
	
	// Now, traverse the features:
	OGRFeature* feature;
	
	//
	// Was a specific FID given?
	//
	if ( globalOptions.FID >= 0 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: Only to process FID: " <<globalOptions.FID <<endl;
		}
		feature = layer->GetFeature(globalOptions.FID);
		if ( !feature ) {
			cerr<< "FID " <<globalOptions.FID<< " not found in " <<vect->getName()<< endl;
			res = 2;
			goto end;
		}
		process_modes_feature(dupPixelModes, feature, rastInfos);
		delete feature;
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
		if ( globalOptions.verbose ) {
			cout<< "--duplicate_pixel: Setting spatial filter" << endl;
		}
		layer->SetSpatialFilter(allRasterArea);
		
		while( (feature = layer->GetNextFeature()) != NULL ) {
			process_modes_feature(dupPixelModes, feature, rastInfos);
			delete feature;
		}
	}
	
end:
	if ( allRasterArea ) {
		delete allRasterArea;
	}
	
	        
	// release rasters
	for ( unsigned i = 0; i < rastInfos.size(); i++ ) {
		delete rastInfos[i];
	}
	
	return res;
}

