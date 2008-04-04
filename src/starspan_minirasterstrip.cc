//
// STARSpan project
// Carlos A. Rueda
// starspan_minirasterstrip - mini raster strip with duplicate pixel handling
// $Id$
//

#include "starspan.h"
#include "traverser.h"
#include "Csv.h"

#include <stdlib.h>
#include <assert.h>


static Vector* vect;
static vector<const char*>* select_fields;
static int layernum;

static const char* mini_raster_strip_filename = 0;



static void extractFunction(ExtractionItem* item) {
    
	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Create and initialize a traverser
	// - Create and register MiniRasterStripObserver
    // - traverse
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = item->feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(item->rasterFilename);
	
	// - Call starspan_csv()
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;
    
    
	Traverser tr;
    
	tr.setVector(vect);
	tr.setLayerNum(layernum);

    Raster* raster = new Raster(item->rasterFilename);  
    tr.addRaster(raster);

    if ( globalOptions.pix_prop >= 0.0 )
		tr.setPixelProportion(globalOptions.pix_prop);
    
    tr.setVectorSelectionParams(globalOptions.vSelParams);
    
	if ( globalOptions.FID >= 0 )
		tr.setDesiredFID(globalOptions.FID);
	tr.setVerbose(globalOptions.verbose);
    tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);
    

    Observer* obs = starspan_getMiniRasterStripObserver(tr, mini_raster_strip_filename);
	tr.addObserver(obs);

    tr.traverse();
    
    tr.releaseObservers();
    
	Traverser::_resetReading = prevResetReading;
    
    delete raster;
	
	if ( globalOptions.verbose ) {
		cout<< "--starspan_miniraster_strip: completed." << endl;
	}
}

//
//
int starspan_miniraster_strip(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
    const char*  _mini_raster_strip_filename
) {
    vect = _vect;
    select_fields = _select_fields;
    layernum = _layernum;
    
    mini_raster_strip_filename = _mini_raster_strip_filename;
    
    return starspan_dup_pixel(
        vect,
        raster_filenames,
        mask_filenames,
        select_fields,
        layernum,
        dupPixelModes,
        extractFunction
    );
}

