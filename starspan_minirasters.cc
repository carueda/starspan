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

#include <unistd.h>   // unlink           
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

struct MRBasicInfo {
	// corresponding FID from which the miniraster was extracted
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
	string prefix;
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
	MiniRasterObserver(Traverser& tr, OGRLayer* layer, Raster* rast, const char* aprefix, const char* pszOutputSRS)
	: tr(tr), layer(layer), rast(*rast), prefix(aprefix), pszOutputSRS(pszOutputSRS)
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
	static string create_filename(string prefix, long FID) {
		ostringstream ostr;
		ostr << prefix << FID << ".img";
		return ostr.str();
	}
	static string create_filename_hdr(string prefix, long FID) {
		ostringstream ostr;
		ostr << prefix << FID << ".hdr";
		return ostr.str();
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
		string mini_filename = create_filename(prefix, FID);
		
		GDALDatasetH hOutDS = starspan_subset_raster(
			rast.getDataset(),
			mini_col0, mini_row0, mini_width, mini_height,
			mini_filename.c_str(),
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
	string basefilename;
	
public:
	MiniRasterStripObserver(Traverser& tr, OGRLayer* layer, Raster* rast, 
		const char* bfilename)
	: MiniRasterObserver(tr, layer, rast, "dummy", 0)
	{
		basefilename = bfilename;
		prefix = basefilename;
		prefix += "_TMP_PRFX_";
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

		const char * const pszFormat = "ENVI";
		GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
		if( hDriver == NULL ) {
			fprintf(stderr, "Couldn't get driver %s to create output rasters.\n", pszFormat);
			return;
		}

		// get dimensions for output images:
		int strip_width = 0;		
		int strip_height = 0;		
		for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); mrbi != mrbi_list->end(); mrbi++ ) {
			// width will be the maximum miniraster width:
			if ( strip_width < mrbi->width )
				strip_width = mrbi->width;
			// height will be the sum of the miniraster heights:
			strip_height += mrbi->height;
		}
		int strip_bands;
		rast.getSize(NULL, NULL, &strip_bands);

		if ( globalOptions.verbose ) {
			cout<< "strip: width x height x bands: " 
			    <<strip_width<< " x " <<strip_height<< " x " <<strip_bands<< endl;
		}

		// allocate transfer buffer with enough space for the longest row	
		const long doubles = (long)strip_width * strip_bands;
		if ( globalOptions.verbose ) {
			cout<< "Allocating " <<doubles<< " doubles for buffer\n";
		}
		double* buffer = new double[doubles];
		if ( !buffer ) {
			cerr<< " Cannot allocate " <<doubles<< " doubles for buffer\n";
			return;
		}
		
		const GDALDataType strip_band_type = rast.getDataset()->GetRasterBand(1)->GetRasterDataType();
		const GDALDataType fid_band_type = GDT_Int32; 
		const GDALDataType loc_band_type = GDT_Float32; 

		////////////////////////////////////////////////////////////////
		// create output rasters

		char **papszOptions = NULL;
		
		// create strip image:
		string strip_filename = basefilename + "_mr.img";
		GDALDataset* strip_ds = hDriver->Create(
			strip_filename.c_str(), strip_width, strip_height, strip_bands, 
			strip_band_type, 
			papszOptions 
		);
		if ( !strip_ds ) {
			delete buffer;
			cerr<< "Couldn't create " <<strip_filename<< endl;
			return;
		}
		// fill with globalOptions.nodata
		for ( int k = 0; k < strip_bands; k++ ) {
			strip_ds->GetRasterBand(k+1)->Fill(globalOptions.nodata);
		}
		
		// create FID 1-band image:
		string fid_filename = basefilename + "_mrid.img";
		GDALDataset* fid_ds = hDriver->Create(
			fid_filename.c_str(), strip_width, strip_height, 1, 
			fid_band_type, 
			papszOptions 
		);
		if ( !fid_ds ) {
			delete strip_ds;
			hDriver->Delete(strip_filename.c_str());
			delete buffer;
			cerr<< "Couldn't create " <<fid_filename<< endl;
			return;
		}
		// fill with -1 as "background" value. 
		// -1 is chosen as normally FIDs starts from zero.
		fid_ds->GetRasterBand(1)->Fill(-1);
		
		// create loc 2-band image:
		string loc_filename = basefilename + "_mrloc.glt";
		GDALDataset* loc_ds = hDriver->Create(
			loc_filename.c_str(), strip_width, strip_height, 2, 
			loc_band_type, 
			papszOptions 
		);
		if ( !loc_ds ) {
			delete fid_ds;
			hDriver->Delete(fid_filename.c_str());
			delete strip_ds;
			hDriver->Delete(strip_filename.c_str());
			delete buffer;
			cerr<< "Couldn't create " <<loc_filename<< endl;
			return;
		}
		// fill with 0 as "background" value.
		// 0 is arbitrarely chosen.
		loc_ds->GetRasterBand(1)->Fill(0);
		loc_ds->GetRasterBand(2)->Fill(0);
		
		////////////////////////////////////////////////////////////////
		// transfer data, fid, and loc from minirasters to output strips
		int next_row = 0;
		for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); mrbi != mrbi_list->end(); mrbi++ ) {
			if ( globalOptions.verbose )
				cout<< "  adding miniraster FID=" <<mrbi->FID<< " to strip...\n";

			// get miniraster filename:
			string mini_filename = create_filename(prefix, mrbi->FID);
			
			// open miniraster
			GDALDataset* mini_ds = (GDALDataset*) GDALOpen(mini_filename.c_str(), GA_ReadOnly);
			if ( !mini_ds ) {
				cerr<< " Unexpected: couldn't read " <<mini_filename<< endl;
				hDriver->Delete(mini_filename.c_str());
				unlink(create_filename_hdr(prefix, mrbi->FID).c_str()); // hack
				continue;
			}

			///////////////////////////////////////////////////////////////
			// transfer data (row by row):			
			for ( int i = 0; i < mini_ds->GetRasterYSize(); i++ ) {
				
				// read data from raster into buffer
				mini_ds->RasterIO(GF_Read,
					0,   	                     //nXOff,
					0 + i,  	                 //nYOff,
					mini_ds->GetRasterXSize(),   //nXSize,
					1,                           //nYSize,
					buffer,                      //pData,
					mini_ds->GetRasterXSize(),   //nBufXSize,
					1,                           //nBufYSize,
					strip_band_type,             //eBufType,
					strip_bands,                 //nBandCount,
					NULL,                        //panBandMap,
					0,                           //nPixelSpace,
					0,                           //nLineSpace,
					0                            //nBandSpace
				);  	
				
				// write buffer in strip image
				strip_ds->RasterIO(GF_Write,
					0,   	                     //nXOff,
					next_row + i,                //nYOff,
					mini_ds->GetRasterXSize(),   //nXSize,
					1,                           //nYSize,
					buffer,                      //pData,
					mini_ds->GetRasterXSize(),   //nBufXSize,
					1,                           //nBufYSize,
					strip_band_type,             //eBufType,
					strip_bands,                 //nBandCount,
					NULL,                        //panBandMap,
					0,                           //nPixelSpace,
					0,                           //nLineSpace,
					0                            //nBandSpace
				);  	
			}
			
			///////////////////////////////////////////////////////////////
			// write FID chunck by replication
			long fid_datum = mrbi->FID;
			if ( false ) {  // should work but seems to be a RasterIO bug
				fid_ds->RasterIO(GF_Write,
					0,   	                   //nXOff,
					next_row,                  //nYOff,
					mini_ds->GetRasterXSize(), //nXSize,
					mini_ds->GetRasterYSize(), //nYSize,
					&fid_datum,                //pData,
					1,                         //nBufXSize,
					1,                         //nBufYSize,
					fid_band_type,             //eBufType,
					1,                         //nBandCount,
					NULL,                      //panBandMap,
					0,                         //nPixelSpace,
					0,                         //nLineSpace,
					0                          //nBandSpace
				);
			}
			else { // workaround
				// replicate FID in buffer
				int* fids = (int*) buffer;
				for ( int j = 0; j < mini_ds->GetRasterXSize(); j++ ) {
					fids[j] = (int) mrbi->FID;
				}
				for ( int i = 0; i < mini_ds->GetRasterYSize(); i++ ) {
					fid_ds->RasterIO(GF_Write,
						0,   	                    //nXOff,
						next_row + i,               //nYOff,
						mini_ds->GetRasterXSize(),  //nXSize,
						1,                          //nYSize,
						fids,                       //pData,
						mini_ds->GetRasterXSize(),  //nBufXSize,
						1,                          //nBufYSize,
						fid_band_type,              //eBufType,
						1,                          //nBandCount,
						NULL,                       //panBandMap,
						0,                          //nPixelSpace,
						0,                          //nLineSpace,
						0                           //nBandSpace
					);
				}
			}
			
			///////////////////////////////////////////////////////////////
			// write loc data
			double adfGeoTransform[6];
			mini_ds->GetGeoTransform(adfGeoTransform);
			float pix_x_size = (float) adfGeoTransform[1];
			float pix_y_size = (float) adfGeoTransform[5];
			float x0 = (float) adfGeoTransform[0];
			float y0 = (float) adfGeoTransform[3];
			float xy[2] = { x0, y0 };
			for ( int i = 0; i < mini_ds->GetRasterYSize(); i++, xy[1] += pix_y_size ) {
				xy[0] = x0;
				for ( int j = 0; j < mini_ds->GetRasterXSize(); j++, xy[0] += pix_x_size ) {
					loc_ds->RasterIO(GF_Write,
						j,   	                    //nXOff,
						next_row + i,               //nYOff,
						1,                          //nXSize,
						1,                          //nYSize,
						xy,                         //pData,
						1,                          //nBufXSize,
						1,                          //nBufYSize,
						loc_band_type,              //eBufType,
						2,                          //nBandCount,
						NULL,                       //panBandMap,
						0,                          //nPixelSpace,
						0,                          //nLineSpace,
						0                           //nBandSpace
					);
				}
			}
			
			
			///////////////////////////////////////////////////////////////
			strip_ds->FlushCache();
			fid_ds->FlushCache();
			loc_ds->FlushCache();
			
			// close and delete miniraster
			delete mini_ds;
			hDriver->Delete(mini_filename.c_str());
			unlink(create_filename_hdr(prefix, mrbi->FID).c_str()); // hack
			
			// advance to next row in strip images 
			next_row += mrbi->height;

		}
		
		// close outputs
		delete strip_ds;
		delete fid_ds;
		delete loc_ds;
		
		// release buffer
		delete buffer;
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

