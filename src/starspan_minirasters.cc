//
// STARSpan project
// Carlos A. Rueda
// starspan_minirasters - generate mini rasters including strip
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <vector>

#include <unistd.h>   // unlink           
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

using namespace std;

///////////////////////////////////////////////////
// mini raster basic information; A list of these elements
// is gathered by the main miniraster generator and then used by
// the strip generator to properly locate the minirasters within the strip.
//
struct MRBasicInfo {
	// corresponding FID from which the miniraster was extracted
	long FID;
	
	// dimensions of this miniraster
	int width;
	int height;
    
    // row to locate this miniraster in strip:
    int mrs_row;
	
	MRBasicInfo(long FID, int width, int height, int mrs_row) : 
		FID(FID), width(width), height(height), mrs_row(mrs_row)
	{}
};


/**
  * Creates a miniraster for each traversed feature.
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

	// to help in determination of extrema col,row coordinates
	bool first;
	int mini_col0, mini_row0, mini_col1, mini_row1;
	
	// If not null, basic info is added for each created miniraster
	vector<MRBasicInfo>* mrbi_list;
    
    // used to update MRBasicInfo::mrs_row in mrbi_list:
    int next_row;
	
	
	/**
	  * Creates the observer for this operation. 
	  */
	MiniRasterObserver(Traverser& tr, OGRLayer* layer, Raster* rast, const char* aprefix, const char* pszOutputSRS)
	: tr(tr), layer(layer), rast(*rast), prefix(aprefix), pszOutputSRS(pszOutputSRS)
	{
		global_info = 0;
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
	  * Nothing done in this class.
	  */
	virtual void end() {
	}
	
	/**
	  * returns true:  we subset the raster directly for
	  *  each intersecting feature in intersectionEnd()
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * 
	  */
	virtual void init(GlobalInfo& info) {
		global_info = &info;
        next_row = 0;
	}

	/**
	  * Just sets first = true.
	  */
	void intersectionFound(IntersectionInfo& intersInfo) {
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
		ostr << prefix << setfill('0') << setw(4) << FID << ".img";
		return ostr.str();
	}
	static string create_filename_hdr(string prefix, long FID) {
		ostringstream ostr;
		ostr << prefix << setfill('0') << setw(4) << FID << ".hdr";
		return ostr.str();
	}

	/**
	  * Proper creation of mini-raster with info gathered during traversal
	  * of the given feature.
	  */
	virtual void intersectionEnd(IntersectionInfo& intersInfo) {
        OGRFeature* feature = intersInfo.feature;
        OGRGeometry* intersection_geometry = intersInfo.intersection_geometry;
        
		if ( tr.getPixelSetSize() == 0 )
			return;
		
        
        // (width,height) of proper mini raster according to feature geometry
		int mini_width = mini_col1 - mini_col0 + 1;  
		int mini_height = mini_row1 - mini_row0 + 1;
		
		// create mini raster
		long FID = feature->GetFID();
		string mini_filename = create_filename(prefix, FID);
		
		// for parity; by default no parity adjustments:
		int xsize_incr = 0; 
		int ysize_incr = 0;
		
		if ( globalOptions.mini_raster_parity.length() > 0 ) {
            //
            // make necessary parity adjustments (on xsize_incr and/or ysize_incr) 
            //
			const char* mini_raster_parity = 0;
			if ( globalOptions.mini_raster_parity[0] == '@' ) {
				const char* attr = globalOptions.mini_raster_parity.c_str() + 1;
				int index = feature->GetFieldIndex(attr);
				if ( index < 0 ) {
					cerr<< "\n\tField `" <<attr<< "' not found";
					cerr<< "\n\tParity not performed\n";
				}
				else {
					mini_raster_parity = feature->GetFieldAsString(index);
				}
			}
			else {
				// just take the given parameter
				mini_raster_parity = globalOptions.mini_raster_parity.c_str();
			}

			int parity = 0;
			if ( 0 == strcmp(mini_raster_parity, "odd") )
				parity = 1;
			else if ( 0 == strcmp(mini_raster_parity, "even") )
				parity = 2;
			else {
				cerr<< "\n\tunrecognized value `" <<mini_raster_parity<< "' for parity";
				cerr<< "\n\tParity not performed\n";
			}
			
			if ( (parity == 1 && mini_width % 2 == 0)
			||   (parity == 2 && mini_width % 2 == 1) ) {
				xsize_incr = 1;
			}
			if ( (parity == 1 && mini_height % 2 == 0)
			||   (parity == 2 && mini_height % 2 == 1) ) {
				ysize_incr = 1;
			}
		}
        
		
		GDALDatasetH hOutDS = starspan_subset_raster(
			rast.getDataset(),
			mini_col0, mini_row0, mini_width, mini_height,
			mini_filename.c_str(),
			pszOutputSRS,
			xsize_incr, ysize_incr,
			NULL // nodata -- PENDING
		);
		
		if ( globalOptions.only_in_feature ) {
			double nodata = globalOptions.nodata;
			
			if ( globalOptions.verbose )
				cout<< "nullifying pixels...\n";

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
				cout<< " " <<num_points<< " points retained\n";
		}

		if ( mrbi_list ) {
			mrbi_list->push_back(MRBasicInfo(FID, mini_width, mini_height, next_row));
            next_row += mini_height + globalOptions.mini_raster_separation;
		}
		
		GDALClose(hOutDS);
		cout<< endl;
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

	int layernum = tr.getLayerNum();
	OGRLayer* layer = tr.getVector()->getLayer(layernum);
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


/////////////////////////////////////////////////////////////////////
/////////// miniraster strip:
/////////////////////////////////////////////////////////////////////

static void translateGeometry(OGRGeometry* geometry, double x0, double y0);


/**
  * Extends MiniRasterObserver to override a couple of methods,
  * use mrbi_list, and create the strip output files.
  */
class MiniRasterStripObserver : public MiniRasterObserver {
	string basefilename;
    
    OGRLayer* inLayer;  // layer being traversed;
    
    const char* shpfilename;
    Vector* outVector;
    OGRLayer* outLayer;
	
public:
	MiniRasterStripObserver(Traverser& tr, OGRLayer* layer, Raster* rast, 
                            const char* bfilename, const char* bshpfilename)
	: MiniRasterObserver(tr, layer, rast, "dummy", 0)
	{
		basefilename = bfilename;
        shpfilename = bshpfilename;
		prefix = basefilename;
		prefix += "_TMP_PRFX_";
		mrbi_list = new vector<MRBasicInfo>();
        
        outVector = NULL;
	}
    
    
    /**
     * Takes a handle on the layer being traversed
     * and, if indicated, creates the output shapefile with corresponding 
     * field definitions.
     */
    void init(GlobalInfo& info) {
        MiniRasterObserver::init(info);
        
        // remember layer being traversed:
        inLayer = info.layer;

        if ( shpfilename == NULL ) {
            return;
        }
        
		if ( globalOptions.verbose ) {
			cout<< "mini_raster_strip: starting creation of output vector " <<shpfilename<< " ...\n";
        }

        outVector = Vector::create(shpfilename); 
        if ( outVector == NULL ) {
            // errors should have been written
            exit(1);  // TODO: the observer interface should allow for early termination
        }                

        OGRDataSource *poDS = outVector->getDataSource();
        
        // get layer definition:
        OGRFeatureDefn* inDefn = inLayer->GetLayerDefn();

        // create layer in output vector:
        outLayer = poDS->CreateLayer(
                "mini_raster_strip", 
                inLayer->GetSpatialRef(),
                inDefn->GetGeomType(),
                NULL
        );
        if ( outLayer == NULL ) {
            delete outVector;
            outVector = NULL;
            cerr<< "Layer creation failed.\n";
            exit(1);  // TODO: the observer interface should allow for early termination
        }
        
        // TODO Add new fields (eg. RID)
        // ...
        
        // create field definitions from originating layer:
        for( int iAttr = 0; iAttr < inDefn->GetFieldCount(); iAttr++ ) {
            OGRFieldDefn* inField = inDefn->GetFieldDefn(iAttr);
            
            OGRFieldDefn outField(inField->GetNameRef(), inField->GetType());
            outField.SetWidth(inField->GetWidth());
            outField.SetPrecision(inField->GetPrecision());
            if ( outLayer->CreateField(&outField) != OGRERR_NONE ) {
                cerr<< "Creating Name field failed.\n";
                exit(1);  // TODO: the observer interface should allow for early termination
            }
        }
    }
    
    
    /**
     * Calls super.intersectionEnd(); if output vector was indicated, then
     * it copies fields and translates the feature data to the output vector.
     */
	virtual void intersectionEnd(IntersectionInfo& intersInfo) {
        MiniRasterObserver::intersectionEnd(intersInfo);

        OGRFeature* feature = intersInfo.feature;
        OGRGeometry* geometryToIntersect = intersInfo.geometryToIntersect;
        OGRGeometry* intersection_geometry = intersInfo.intersection_geometry;

		if ( outVector == 0 ) {
			return;  // not output vector creation
        }
        
		if ( tr.getPixelSetSize() == 0 ) {
			return;   // no pixels were actually intersected
        }
        
        // create feature in output vector:
        OGRFeature *outFeature = OGRFeature::CreateFeature(outLayer->GetLayerDefn());
        
        // TODO Add values for new fields:
        // ...
        
        // copy everything from incoming feature:
        // (TODO handled selected fields as in other commands) 
        // (TODO do not copy geometry, but set the corresponding geometry)
        outFeature->SetFrom(feature);
        
        //  make sure we release the copied geometry:
        outFeature->SetGeometryDirectly(NULL);

        // depending on the associated geometry (see below), these offsets
        // will help in locating the geometry in the strip:
        double offsetX = 0;
        double offsetY = 0;
        
        double pix_x_size, pix_y_size;
        rast.getPixelSize(&pix_x_size, &pix_y_size);
        
        // Associate the required geometry.
        // This will depend on what geometry was actually used for interesection:
        OGRGeometry* feature_geometry = feature->GetGeometryRef();
        if ( geometryToIntersect == feature_geometry ) {
            // if it was the original feature's geometry, 
            // then associate intersection_geometry:
            outFeature->SetGeometry(intersection_geometry);
            // (offsetX,offsetY) will remain == (0,0)
        }
        else {
            // else, associate the intersection between original feature's 
            // geometry and intersection_geometry:
            try {
                OGRGeometry* featInters = feature_geometry->Intersection(intersection_geometry);
                outFeature->SetGeometryDirectly(featInters);
                // (note: featInters is a new object so we can use SetGeometryDirectly)
                
                // In this case, we need to take into account a possible offset
                // between featInters and intersection_geometry:
                
                OGREnvelope env1, env2;
                featInters->getEnvelope(&env1);
                intersection_geometry->getEnvelope(&env2);
                
                // the sign of the pixel size will indicate the way to get the offset: 
                offsetX = -(pix_x_size < 0 ? env2.MaxX - env1.MaxX : env2.MinX - env1.MinX);
                offsetY = -(pix_y_size < 0 ? env2.MaxY - env1.MaxY : env2.MinY - env1.MinY);
            }
            catch(GEOSException* ex) {
                cerr<< "min_raster_strip: GEOSException: " << EXC_STRING(ex) << endl;
                
                // should not happen, but well, assign intersection_geometry:
                outFeature->SetGeometry(intersection_geometry);
            }
        }
        
        
        //  grab a ref to the associated geometry for modification:
        OGRGeometry* outGeometry = outFeature->GetGeometryRef();

        
        // Adjust outGeometry to be relative to the strip so it can be overlayed:
        
        // get mrbi just pushed:
        MRBasicInfo mrbi = mrbi_list->back();
        int next_row = mrbi.mrs_row;   // base row for this miniraster in strip:

        // origin of outGeometry relative to:
        //      * origin of miniraster (0,       pix_y_size*next_row) 
        //      * and offset above     (offsetX, offsetY)
        double x0 = 0                   + offsetX;
        double y0 = pix_y_size*next_row + offsetY;
        
        // translate outGeometry:
        translateGeometry(outGeometry, x0, y0);

        // add outFeature to outLayer:
        if ( outLayer->CreateFeature(outFeature) != OGRERR_NONE ) {
           cerr<< "*** WARNING: Failed to create feature in output layer.\n";
           return;
        }
        OGRFeature::DestroyFeature(outFeature);
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
	  * Creates the output strips according to the minirasters registered
      * in list mrbi_list.
	  */
	void create_strip() {
		if ( globalOptions.verbose ) {
			cout<< "Creating miniraster strip...\n";
        }

        ///////////////////////
        // get ENVI driver
		const char * const pszFormat = "ENVI";
		GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
		if( hDriver == NULL ) {
			cerr<< "Couldn't get driver " <<pszFormat<< " to create output rasters.\n";
			return;
		}
		
        // the number of minirasters:
		const int num_minis = mrbi_list->size();

        /////////////////////////////////////////////////////////////
		// the dimensions for output strip images (obtained below):
		int strip_width = 0;		
		int strip_height = 0;
        
        
        // separation between minirasters:
		const int mr_separation = globalOptions.mini_raster_separation;
        
        ////////////////////////////////////////////////////////
		// get dimensions for output strip images:
        
        // strip_width will be the maximum miniraster width:
        // strip_height will be the sum of the miniraster heights, plus separation pixels (see below):
        for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); mrbi != mrbi_list->end(); mrbi++ ) {
            if ( strip_width < mrbi->width ) {
                strip_width = mrbi->width;
            }
            strip_height += mrbi->height;
        }
        
		// add pixels to height according to desired separation between minirasters:
		strip_height += mr_separation * (num_minis - 1);
		
        
        ///////////////////////
        // get number off bands
		int strip_bands;
		rast.getSize(NULL, NULL, &strip_bands);

		if ( globalOptions.verbose ) {
			cout<< "strip: width x height x bands: " 
			    <<strip_width<< " x " <<strip_height<< " x " <<strip_bands<< endl;
		}

        /////////////////////////////////////////////////////////////////////
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
		
        //////////////////////////////////////////////
        // the band types for the strips: 
		const GDALDataType strip_band_type = rast.getDataset()->GetRasterBand(1)->GetRasterDataType();
		const GDALDataType fid_band_type = GDT_Int32; 
		const GDALDataType loc_band_type = GDT_Float32; 

        
		////////////////////////////////////////////////////////////////
		// create output rasters
        ///////////////////////////////////////////////////////////////

		char **papszOptions = NULL;
		
        /////////////////////////////////////////////////////////////////////
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
		
        /////////////////////////////////////////////////////////////////////
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
		
        /////////////////////////////////////////////////////////////////////
		// create 2-band loc image:
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
		
        
		/////////////////////////////////////////////////////////////////////
		// transfer data, fid, and loc from minirasters to output strips
        /////////////////////////////////////////////////////////////////////
        
		int processed_minirasters = 0;
        
        /////////////////////////////////////////////////////////////////////
        // for each miniraster (according to mrbi_list):
		for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); 
		mrbi != mrbi_list->end(); mrbi++, processed_minirasters++ ) {
        
            if ( globalOptions.verbose ) {
				cout<< "  adding miniraster FID=" <<mrbi->FID<< " to strip...\n";
            }

			// get miniraster filename:
			string mini_filename = create_filename(prefix, mrbi->FID);
			
            ///////////////////////
			// open miniraster
			GDALDataset* mini_ds = (GDALDataset*) GDALOpen(mini_filename.c_str(), GA_ReadOnly);
			if ( !mini_ds ) {
				cerr<< " Unexpected: couldn't read " <<mini_filename<< endl;
				hDriver->Delete(mini_filename.c_str());
				unlink(create_filename_hdr(prefix, mrbi->FID).c_str()); // hack
				continue;
			}
            
            // row to position miniraster in strip:
            int next_row = mrbi->mrs_row;

            // column to position miniraster in strip:
            int next_col = 0;
            

			///////////////////////////////////////////////////////////////
			// transfer data to image strip (row by row):			
			for ( int i = 0; i < mini_ds->GetRasterYSize(); i++ ) {
				
				// read data from raster into buffer
                // (note: reading is always (0,0)-relative in source miniraster)
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
                // (note: wiritn is (next_col,next_row)-based in destination strip)
				strip_ds->RasterIO(GF_Write,
					next_col,                    //nXOff,
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
            
			if ( false ) {  
                // ----- should work but seems to be a RasterIO bug  -----
                // NOTE: The arguments in the following call need to be reviewed
                // since I haven't paid much attention to keep it up to date.
				fid_ds->RasterIO(GF_Write,
					next_col,                  //nXOff,
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
			else { 
               // ------- workaround  -------------
				// replicate FID in buffer
				int* fids = (int*) buffer;
				for ( int j = 0; j < mini_ds->GetRasterXSize(); j++ ) {
					fids[j] = (int) mrbi->FID;
				}
				for ( int i = 0; i < mini_ds->GetRasterYSize(); i++ ) {
					fid_ds->RasterIO(GF_Write,
						next_col,                   //nXOff,
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
						next_col + j,               //nXOff,
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
			if ( processed_minirasters == 0 ) { // is this the first miniraster?
				// set projection for generated strips using the info from
				// this (arbitrarely chosen) first miniraster:
				const char* projection = mini_ds->GetProjectionRef();
				if ( projection && strlen(projection) > 0 ) {
					strip_ds->SetProjection(projection);
					fid_ds->  SetProjection(projection);                   
					loc_ds->  SetProjection(projection);
				}
			}
			
			///////////////////////////////////////////////////////////////
			if ( false ) {  
				// GDAL bug: ENVIDataset::FlushCache is not idempotent
				// for header file (repeated items would be generated)
				strip_ds->FlushCache();
				fid_ds->FlushCache();
				loc_ds->FlushCache();
			}
			else {
				// workaround: don't call FlushCache; it seems
				// we don't need these calls anyway.
			}
			
			// close and delete miniraster
			delete mini_ds;
			hDriver->Delete(mini_filename.c_str());
			unlink(create_filename_hdr(prefix, mrbi->FID).c_str()); // hack
		}
		
		// close outputs
		delete strip_ds;
		delete fid_ds;
		delete loc_ds;
        
        if ( outVector ) {
            delete outVector;
        }
		
		// release buffer
		delete buffer;
	}
	
};

Observer* starspan_getMiniRasterStripObserver(
	Traverser& tr,
	const char* filename,
	const char* shpfilename
) {	
	if ( !tr.getVector() ) {
		cerr<< "vector datasource expected\n";
		return 0;
	}

	int layernum = tr.getLayerNum();
	OGRLayer* layer = tr.getVector()->getLayer(layernum);
	if ( !layer ) {
		cerr<< "warning: No layer 0 found\n";
		return 0;
	}

	if ( tr.getNumRasters() == 0 ) {
		cerr<< "raster expected\n";
		return 0;
	}
	Raster* rast = tr.getRaster(0);
	MiniRasterStripObserver* obs = new MiniRasterStripObserver(tr, layer, rast, filename, shpfilename);
	return obs;
}


static void translateLineString(OGRLineString* gg, double deltaX, double deltaY) {
    int numPoints = gg->getNumPoints();
    for ( int i = 0; i < numPoints; i++ ) {
        OGRPoint pp;
        gg->getPoint(i, &pp);
        pp.setX(pp.getX() + deltaX);
        pp.setY(pp.getY() + deltaY);
        gg->setPoint(i, &pp);
    }
}

/** does the translation */
static void traverseTranslate(OGRGeometry* geometry, double deltaX, double deltaY) {
	OGRwkbGeometryType type = geometry->getGeometryType();
	switch ( type ) {
		case wkbPoint:
		case wkbPoint25D: {
			OGRPoint* gg = (OGRPoint*) geometry;
            gg->setX(gg->getX() + deltaX);
            gg->setY(gg->getY() + deltaY);
        }
			break;
	
		case wkbLineString:
		case wkbLineString25D: {
			OGRLineString* gg = (OGRLineString*) geometry;
            translateLineString(gg, deltaX, deltaY);
        }
			break;
	
		case wkbPolygon:
		case wkbPolygon25D: {
			OGRPolygon* gg = (OGRPolygon*) geometry;
            OGRLinearRing *ring = gg->getExteriorRing();
            translateLineString(ring, deltaX, deltaY);
           	for ( int i = 0; i < gg->getNumInteriorRings(); i++ ) {
           		ring = gg->getInteriorRing(i);
           		translateLineString(ring, deltaX, deltaY);
           	}
        }
			break;
			
		case wkbGeometryCollection:
		case wkbGeometryCollection25D: {
			OGRGeometryCollection* gg = (OGRGeometryCollection*) geometry;
           	for ( int i = 0; i < gg->getNumGeometries(); i++ ) {
           		OGRGeometry* geo = (OGRGeometry*) gg->getGeometryRef(i);
           		traverseTranslate(geo, deltaX, deltaY);
           	}
        }            
			break;
			
		default:
			cerr<< "***WARNING: mini_raster_strip: " <<OGRGeometryTypeToName(type)
                << ": geometry type not considered.\n"
			;
	}
}


/**
 * Translates the geometry such that it's located relative to (x0,y0).
 */
static void translateGeometry(OGRGeometry* geometry, double x0, double y0) {
    OGREnvelope bbox;
    geometry->getEnvelope(&bbox);
    
    //
    // FIXME: What follows is still rather a hack
    //
    
    double baseX = //x0 <= 0 ? bbox.MaxX : bbox.MinX;
                   bbox.MinX;
    double baseY = y0 <= 0 ? bbox.MaxY : bbox.MinY;
    
    // deltas to adjust all points in geometry:
    // first term: to move to origin;
    // second term: to move to requested (x0,y0) origin:
    double deltaX = -baseX +x0;
    double deltaY = -baseY +y0;

    traverseTranslate(geometry, deltaX, deltaY);    
}


