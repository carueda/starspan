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
	Traverser& tr;
	OGRLayer* layer;
	Raster& rast;
	GlobalInfo* global_info;
	const char* prefix;
	const char* pszOutputSRS;
	GDALDatasetH hOutDS;
	long pixel_count;

	// to help in determination of extrema col,row coordinates
	bool first;
	int mini_col0, mini_row0, mini_col1, mini_row1;
	
	/**
	  * Creates the observer for this operation. 
	  */
	MiniRasterObserver(Traverser& tr, OGRLayer* layer, Raster* rast, const char* prefix, const char* pszOutputSRS)
	: tr(tr), layer(layer), rast(*rast), prefix(prefix), pszOutputSRS(pszOutputSRS)
	{
		global_info = 0;
		pixel_count = 0;
		hOutDS = 0;
	}
	
	/**
	  * simply calls end()
	  */
	~MiniRasterObserver() {
		end();
	}
	
	/**
	  * Nothing done.
	  */
	void end() {
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
	  * Just sets first = true.
	  */
	void intersectionFound(OGRFeature* feature) {
		first = true;
	}

	/**
	  * used to keep track of the extrema
	  */
	void addPixel(TraversalEvent& ev) {
		int col = ev.pixel.col;
		int row = ev.pixel.row;
		if ( first ) {
			first = false;
			mini_col0 = mini_col1 = col;
			mini_row0 = mini_row1 = row;
		}
		else {
			if ( mini_col0 > col )
				mini_col0 = col;
			if ( mini_row0 > row )
				mini_row0 = row;
			if ( mini_col1 < col )
				mini_col1 = col;
			if ( mini_row1 < row )
				mini_row1 = row;
		}
	}

	/**
	  * Proper creation of mini-raster with info gathered during traversal
	  * of the given feature.
	  */
	void intersectionEnd(OGRFeature* feature) {
		set<EPixel> pixels = tr.getPixelSet();
		if ( !pixels.size() )
			return;
		
		int mini_width = mini_col1 - mini_col0 + 1;  
		int mini_height = mini_row1 - mini_row0 + 1;
		
		// create mini raster
		long FID = feature->GetFID();
		char mini_filename[1024];
		sprintf(mini_filename, "%s%04ld.img", prefix, FID);
		
		GDALDatasetH hOutDS = starspan_subset_raster(
			rast.getDataset(),
			mini_col0, mini_row0, mini_width, mini_height,
			mini_filename,
			pszOutputSRS
		);
		
		if ( globalOptions.only_in_feature ) {
			double nodata = globalOptions.nodata;
			
			if ( globalOptions.verbose )
				fprintf(stdout, "nullifying pixels...\n");

			const int nBandCount = GDALGetRasterCount(hOutDS);
			
			int num_points = 0;			
			for ( int row = mini_row0; row <= mini_row1; row++ ) {
				for ( int col = mini_col0; col <= mini_col1 ; col++ ) {
					if ( tr.pixelVisited(col, row) ) {
						num_points++;
					}
					else {   // nullify:
						for(int i = 0; i < nBandCount; i++ ) {
							GDALRasterBand* band = ((GDALDataset *) hOutDS)->GetRasterBand(i+1);
							band->RasterIO(
								GF_Write,
								col - mini_col0, 
								row - mini_row0,
								1, 1,              // nXSize, nYSize
								&nodata,           // pData
								1, 1,              // nBufXSize, nBufYSize
								GDT_Float64,       // eBufType
								0, 0               // nPixelSpace, nLineSpace
							);
							
						}
					}
				}
			}
			if ( globalOptions.verbose )
				fprintf(stdout, " %d points retained\n", num_points);
		}

		GDALClose(hOutDS);
		fprintf(stdout, "\n");

		pixels.clear();
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

	return new MiniRasterObserver(tr, layer, rast, prefix, pszOutputSRS);
}


