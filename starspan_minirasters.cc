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
#include <vector>

#include "unistd.h"   // unlink           
#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

struct MRBasicInfo {
	// corresponding FID from which this the miniraster was extracted
	long FID;
	
	// dimensions of this miniraster
	int width;
	int height;
	
	MRBasicInfo(long FID, int width, int height) : 
		FID(FID), width(width), height(height)
	{}
};


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
	
	// If not null, basic info is added for each created miniraster
	vector<MRBasicInfo>* mrbi_list;
	
	
	/**
	  * Creates the observer for this operation. 
	  */
	MiniRasterObserver(Traverser& tr, OGRLayer* layer, Raster* rast, const char* prefix, const char* pszOutputSRS)
	: tr(tr), layer(layer), rast(*rast), prefix(prefix), pszOutputSRS(pszOutputSRS)
	{
		global_info = 0;
		pixel_count = 0;
		hOutDS = 0;
		mrbi_list = 0;
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
	virtual void end() {
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

	/** aux to create image filename */
	static void create_filename(char* filename, const char* prefix, long FID) {
		sprintf(filename, "%s%04ld.img", prefix, FID);
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
		create_filename(mini_filename, prefix, FID);
		
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

		if ( mrbi_list ) {
			mrbi_list->push_back(MRBasicInfo(FID, mini_width, mini_height));
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


/////////////////////
// miniraster strip:

/**
  * Uses mrbi_list and overrides end() to create the strip
  * output files.
  */
class MiniRasterStripObserver : public MiniRasterObserver {
	static const char* const prefix = "TMP_PREFX_";
	char basefilename[1024];
	
public:
	MiniRasterStripObserver(Traverser& tr, OGRLayer* layer, Raster* rast, 
		const char* bfilename)
	: MiniRasterObserver(tr, layer, rast, prefix, 0)
	{
		strncpy(basefilename, bfilename, sizeof(basefilename) -1);
		mrbi_list = new vector<MRBasicInfo>();
	}

	/**
	  * calls create_strip()
	  */
	virtual void end() {
		if ( mrbi_list ) {
			create_strip();
			delete mrbi_list;
			mrbi_list = 0;
		}
	}
	  
	/**
	  * Creates strip
	  */
	void create_strip() {
		if ( globalOptions.verbose )
			fprintf(stdout, "Creating miniraster strip...\n");

		// get dimension for strip image:
		int strip_width = 0;		
		int strip_height = 0;		
		int max_height = 0;		
		for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); mrbi != mrbi_list->end(); mrbi++ ) {
			// width will be the maximum miniraster width:
			if ( strip_width < mrbi->width )
				strip_width = mrbi->width;
			// height will be the sum of the miniraster heights:
			strip_height += mrbi->height;

			// to allocate common buffer for all transfers
			if ( max_height < mrbi->height )
				max_height = mrbi->height;
		}
		if ( globalOptions.verbose )
			fprintf(stdout, "strip: width x height: %d x %d\n", strip_width, strip_height);

		int strip_bands;
		rast.getSize(NULL, NULL, &strip_bands);

		// allocate transfer buffer:		
		const long doubles = (long)strip_width * strip_height * strip_bands;
		if ( globalOptions.verbose ) {
			fprintf(stdout, "Allocating buffer: width x height x bands: %d x %d x %d", 
				strip_width, max_height, strip_bands
			);
			fprintf(stdout, " = %ld doubles\n", doubles);
		}
		double* buffer = new double[doubles];
		if ( !buffer ) {
			fprintf(stderr, " Cannot allocate buffer!\n");
			return;
		}
		
		GDALDataType band_type = rast.getDataset()->GetRasterBand(1)->GetRasterDataType();

		// create strip image:
		char strip_filename[1024];
		sprintf(strip_filename, "%s.img", basefilename);
		const char *pszFormat = "ENVI";
		GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
		if( hDriver == NULL ) {
			delete buffer;
			fprintf(stderr, "Couldn't get driver %s\n", pszFormat);
			return;
		}
		char **papszOptions = NULL;
		GDALDataset* strip_ds = hDriver->Create(
			strip_filename, strip_width, strip_height, strip_bands, 
			band_type, 
			papszOptions 
		);
		if ( !strip_ds ) {
			delete buffer;
			fprintf(stderr, "Couldn't create strip dataset %s\n", strip_filename);
			return;
		}
		
		
		int next_row = 0;
		
		// now transfer data from minirasters to strip:		
		for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); mrbi != mrbi_list->end(); mrbi++ ) {
			if ( globalOptions.verbose )
				fprintf(stdout, "  adding FID=%ld to strip...\n", mrbi->FID);

			// get the filename:
			char mini_filename[1024];
			create_filename(mini_filename, prefix, mrbi->FID);
			
			// open miniraster
			GDALDataset* min_ds = (GDALDataset*) GDALOpen(mini_filename, GA_ReadOnly);
			if ( !min_ds ) {
				fprintf(stderr, " Unexpected: couldn't read %s\n", mini_filename);
				continue;
			}
			
			// read data from raster into buffer
			min_ds->RasterIO(GF_Read,
					0,   	       //nXOff,
					0,  	       //nYOff,
					min_ds->GetRasterXSize(),   //nXSize,
					min_ds->GetRasterYSize(),  //nYSize,
					buffer,        //pData,
					min_ds->GetRasterXSize(),   //nBufXSize,
					min_ds->GetRasterYSize(),  //nBufYSize,
					band_type,     //eBufType,
					strip_bands,   //nBandCount,
					NULL,          //panBandMap,
					0,             //nPixelSpace,
					0,             //nLineSpace,
					0              //nBandSpace
			);  	
			
			// write buffer in strip image
			strip_ds->RasterIO(GF_Write,
					0,   	       //nXOff,
					next_row,      //nYOff,    <<--  NOTE
					min_ds->GetRasterXSize(),   //nXSize,
					min_ds->GetRasterYSize(),  //nYSize,
					buffer,        //pData,
					min_ds->GetRasterXSize(),   //nBufXSize,
					min_ds->GetRasterYSize(),  //nBufYSize,
					band_type,     //eBufType,
					strip_bands,   //nBandCount,
					NULL,          //panBandMap,
					0,             //nPixelSpace,
					0,             //nLineSpace,
					0              //nBandSpace
			);  	
			
			// close miniraster
			delete min_ds;
			
			// advance to next row in strip image 
			next_row += mrbi->height;

		}
		
		delete strip_ds;
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
	MiniRasterStripObserver* obs = new MiniRasterStripObserver(tr, layer, rast, filename);
	return obs;
}

