//
// STARSpan project
// Carlos A. Rueda
// starspan_minirasters - generate mini rasters
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include <iostream>
#include <cstdlib>

#include "unistd.h"   // unlink           
#include <string>
#include <stdlib.h>
#include <stdio.h>


using namespace std;

/**
  * 
  */
class MiniRasterObserver : public Observer {
public:
	OGRLayer* layer;
	Raster& rast;
	GlobalInfo* global_info;
	const char* prefix;
	const char* pszOutputSRS;
	GDALDatasetH hOutDS;
	long pixel_count;

	vector<CRPixel> pixels;
	
	// last processed FID
	long last_FID;

	/**
	  * 
	  */
	MiniRasterObserver(OGRLayer* layer, Raster* rast, const char* prefix, const char* pszOutputSRS)
	: layer(layer), rast(*rast), prefix(prefix), pszOutputSRS(pszOutputSRS)
	{
		global_info = 0;
		pixel_count = 0;
		hOutDS = 0;
		last_FID = -1;
	}
	
	/**
	  * simply calls end()
	  */
	~MiniRasterObserver() {
		end();
	}
	
	/**
	  * finalizes current feature if any; closes the file
	  */
	void end() {
		finalizePreviousFeatureIfAny();
	}
	
	/**
	  * returns true:  we subset the raster directly for
	  *  each intersecting feature.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * 
	  */
	void init(GlobalInfo& info) {
		global_info = &info;
	}


	/**
	  * dispatches finalization of previous feature
	  */
	void finalizePreviousFeatureIfAny(void) {
		if ( !pixels.size() )
			return;
		
		int mini_x0, mini_y0, mini_x1, mini_y1;
		bool first = true;
		for ( vector<CRPixel>::const_iterator colrow = pixels.begin(); colrow != pixels.end(); colrow++ ) {
			int col = colrow->col;
			int row = colrow->row;
			if ( first ) {
				first = false;
				mini_x0 = mini_x1 = col;
				mini_y0 = mini_y1 = row;
			}
			else {
				if ( mini_x0 > col )
					mini_x0 = col;
				if ( mini_y0 > row )
					mini_y0 = row;
				if ( mini_x1 < col )
					mini_x1 = col;
				if ( mini_y1 < row )
					mini_y1 = row;
			}
		}		
		int mini_width = mini_x1 - mini_x0 + 1;  
		int mini_height = mini_y1 - mini_y0 + 1;
		
		// create mini raster
		char mini_filename[1024];
		sprintf(mini_filename, "%s%04ld.img", prefix, last_FID);
		
		GDALDatasetH hOutDS = starspan_subset_raster(
			rast.getDataset(),
			mini_x0, mini_y0, mini_width, mini_height,
			mini_filename,
			pszOutputSRS
		);
		
		if ( globalOptions.only_in_feature ) {
			fprintf(stdout, "nullifying pixels not contained in feature..\n");
			fprintf(stdout, "   PENDING\n");
			
			// FIXME: nullify pixels outside feature
			// ...
		}
		fprintf(stdout, "all points retained\n");
		//fprintf(stdout, " %d points retained\n", num_points);

		GDALClose(hOutDS);
		fprintf(stdout, "\n");

		pixels.clear();
	}

	/**
	  * Inits creation of mini-raster corresponding to new feature
	  */
	void intersectionFound(OGRFeature* feature) {
		finalizePreviousFeatureIfAny();
		
		// keep track of last FID processed:
		last_FID = feature->GetFID();
	}

	/**
	  * 
	  */
	void addPixel(TraversalEvent& ev) {
		pixels.push_back(CRPixel(ev.pixel.col, ev.pixel.row));
	}
};



Observer* starspan_getMiniRasterObserver(
	Traverser& tr,
	const char* prefix,
	const char* pszOutputSRS 
) {	
	if ( !tr.getVector() ) {
		cerr<< "vector datasource expected\n";
		return 0;
	}
	OGRLayer* layer = tr.getVector()->getLayer(0);
	if ( !layer ) {
		cerr<< "warning: No layer 0 found\n";
		return 0;
	}

	if ( tr.getNumRasters() == 0 ) {
		cerr<< "raster expected\n";
		return 0;
	}
	Raster* rast = tr.getRaster(0);

	return new MiniRasterObserver(layer, rast, prefix, pszOutputSRS);
}


