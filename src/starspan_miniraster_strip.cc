//
// STARSpan project
// Carlos A. Rueda
// starspan_miniraster_strip - generates strip of mini-rasters
// (based on starspan_miniraster.cc)
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include <iostream>
#include <cstdlib>

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <cassert>

using namespace std;

/**
  * 
  */
class MiniRasterStripObserver : public Observer {
public:
	Traverser& tr;
	OGRLayer* layer;
	Raster& rast;
	GlobalInfo* global_info;
	FILE* img_file;
	FILE* fid_file;
	FILE* glt_file;
	long pixel_count;
	long FID;

	/**
	  * Creates the observer for this operation. 
	  */
	MiniRasterStripObserver(Traverser& tr, OGRLayer* layer, Raster* rast, const char* filename)
	: tr(tr), layer(layer), rast(*rast)
	{
		global_info = 0;
		pixel_count = 0;
		char out_filename[1024];
		sprintf(out_filename, "%s.img", filename);
		if ( 0 == (img_file = fopen(out_filename, "w")) ) {
			cerr<< "cannot create " <<out_filename<< endl;
		}
		else {
			sprintf(out_filename, "%s.fid", filename);
			if ( 0 == (fid_file = fopen(out_filename, "w")) ) {
				cerr<< "cannot create " <<out_filename<< endl;
				fclose(img_file);
				img_file = 0;
			}
			sprintf(out_filename, "%s.glt", filename);
			if ( 0 == (glt_file = fopen(out_filename, "w")) ) {
				cerr<< "cannot create " <<out_filename<< endl;
				fclose(fid_file);
				fclose(img_file);
				img_file = 0;
			}
		}
	}
	
	/**
	  * simply calls end()
	  */
	~MiniRasterStripObserver() {
		end();
	}
	
	/**
	  * Closed files
	  */
	void end() {
		if ( img_file ) {
			fclose(img_file);
			fclose(fid_file);
			fclose(glt_file);
			img_file = 0;
		}
	}
	
	/**
	  * returns false:  we dispatch every pixel found
	  */
	bool isSimple() { 
		return false; 
	}

	/**
	  * 
	  */
	void init(GlobalInfo& info) {
		global_info = &info;
	}

	/**
	  * Sets the current FID
	  */
	void intersectionFound(OGRFeature* feature) {
		FID = feature->GetFID();
	}

	/**
	  * used to keep track of the extrema
	  */
	void addPixel(TraversalEvent& ev) {
		double x = ev.pixel.x;
		double y = ev.pixel.y;
		
		assert(ev.bandValues);
		
		// write the bands to img_file ...
		fwrite(ev.bandValues, 1, tr.getBandBufferSize(), img_file);
		
		// write coordinate to glt_file:
		fwrite(&x, sizeof(x), 1, glt_file);
		fwrite(&y, sizeof(y), 1, glt_file);
		
		// write FID to fid_file:
		fwrite(&FID, sizeof(FID), 1, fid_file);
		
		pixel_count++;
	}

	/**
	  * nothing needs to be done.
	  */
	void intersectionEnd(OGRFeature* feature) {
	}
};


Observer* starspan_getMiniRasterStripObserver(
	Traverser& tr,
	const char* filename
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

	return new MiniRasterStripObserver(tr, layer, rast, filename);
}


