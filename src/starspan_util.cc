//
// STARSpan project
// Carlos A. Rueda
// starspan_util - misc utilities
// $Id$
//

#include "starspan.h"           
#include "vrtdataset.h"
#include "jts.h"       

#include <stdlib.h>

// aux routine for reporting 
void starspan_report(Traverser& tr) {
	Vector* vect = tr.getVector();
	if ( vect ) {
		fprintf(stdout, "\n----------------VECTOR--------------\n");
		vect->report(stdout);
	}

	for ( int i = 0; i < tr.getNumRasters(); i++ ) {
		Raster* rast = tr.getRaster(i);
		fprintf(stdout, "\n----------------RASTER--------------\n");
		rast->report(stdout);
	}
}

// aux routine for reporting 
void starspan_report_vector(Vector* vect) {
	if ( vect ) {
		fprintf(stdout, "\n----------------VECTOR--------------\n");
		vect->report(stdout);
	}
}

// aux routine for reporting 
void starspan_report_raster(Raster* rast) {
	if ( rast ) {
		fprintf(stdout, "\n----------------RASTER--------------\n");
		rast->report(stdout);
	}
}





void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env) {
	fprintf(file, "%s %.3f %.3f %.3f %.3f\n", msg, env.MinX, env.MinY, env.MaxX, env.MaxY);
}




//////////////////////////////////////////////////////////////////////////////
// Adapted from gdal_translate.cpp
//
GDALDatasetH starspan_subset_raster(
	GDALDatasetH hDataset,        // input dataset
	int          xoff,
	int          yoff,            // xoff,yoff being the ulx, uly in pixel/line
	int          xsize, 
	int          ysize,
	
	const char*  pszDest,         // output dataset name
	const char*  pszOutputSRS,    // see -a_srs option for gdal_translate
	                              // If NULL, projection is taken from input dataset

	int          xsize_incr,      // used to create the raster 
	int          ysize_incr,      // used to create the raster
	double		 *nodata          // used if not null
) {
	// input vars:
    int*              panBandList = NULL;
	int               nBandCount = 0;
    double		      adfGeoTransform[6];
    GDALProgressFunc  pfnProgress = GDALTermProgress;
    int               bStrict = TRUE;

	// output vars:
    const char*     pszFormat = "ENVI";
    GDALDriverH		hDriver;
	GDALDatasetH	hOutDS;
	int             anSrcWin[4] = { xoff, yoff, xsize, ysize };
	int             nOXSize = anSrcWin[2];
	int             nOYSize = anSrcWin[3];
	char**          papszCreateOptions = NULL;

	
	
/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName(pszFormat);
    if( hDriver == NULL ) {
        fprintf(stderr, "Output driver `%s' not recognised.\n", pszFormat);
		return NULL;
	}



/* -------------------------------------------------------------------- */
/*	Build band list to translate					*/
/* -------------------------------------------------------------------- */
	nBandCount = GDALGetRasterCount( hDataset );
	panBandList = (int *) CPLMalloc(sizeof(int)*nBandCount);
	for(int i = 0; i < nBandCount; i++ )
		panBandList[i] = i+1;
	
	
	
/* ==================================================================== */
/*      Create a virtual dataset.                                       */
/* ==================================================================== */
    VRTDataset *poVDS;
        
/* -------------------------------------------------------------------- */
/*      Make a virtual clone.                                           */
/* -------------------------------------------------------------------- */
    poVDS = new VRTDataset( nOXSize + xsize_incr, nOYSize + ysize_incr );

	// set projection:
	if ( pszOutputSRS ) {
		OGRSpatialReference oOutputSRS;

		if( oOutputSRS.SetFromUserInput(pszOutputSRS) != OGRERR_NONE )
		{
			fprintf( stderr, "Failed to process SRS definition: %s\n", 
					 pszOutputSRS);
			GDALDestroyDriverManager();
			exit( 1 );
		}

		fprintf(stdout, "---------exportToWkt\n"); fflush(stdout);
		char* wkt;
		oOutputSRS.exportToWkt(&wkt);
		fprintf(stdout, "---------Setting given projection (wkt)\n");
		//fprintf(stdout, "%s\n", wkt);
		poVDS->SetProjection(wkt);
		CPLFree(wkt);
	}
	else {
		const char* pszProjection = GDALGetProjectionRef( hDataset );
		if( pszProjection != NULL && strlen(pszProjection) > 0 ) {
			fprintf(stdout, "---------Setting projection from input dataset\n");
			//fprintf(stdout, "%s\n", pszProjection);
			poVDS->SetProjection(pszProjection);
		}
	}
	
	
    if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None ) {
        adfGeoTransform[0] += anSrcWin[0] * adfGeoTransform[1]
            + anSrcWin[1] * adfGeoTransform[2];
        adfGeoTransform[3] += anSrcWin[0] * adfGeoTransform[4]
            + anSrcWin[1] * adfGeoTransform[5];
        
        adfGeoTransform[1] *= anSrcWin[2] / (double) nOXSize;
        adfGeoTransform[2] *= anSrcWin[3] / (double) nOYSize;
        adfGeoTransform[4] *= anSrcWin[2] / (double) nOXSize;
        adfGeoTransform[5] *= anSrcWin[3] / (double) nOYSize;
        
        poVDS->SetGeoTransform( adfGeoTransform );
    }
	
    poVDS->SetMetadata( ((GDALDataset*)hDataset)->GetMetadata() );
	
	
    for(int i = 0; i < nBandCount; i++ )
    {
        VRTSourcedRasterBand   *poVRTBand;
        GDALRasterBand  *poSrcBand;
        GDALDataType    eBandType;

        poSrcBand = ((GDALDataset *) hDataset)->GetRasterBand(panBandList[i]);
		
/* -------------------------------------------------------------------- */
/*      Select output data type to match source.                        */
/* -------------------------------------------------------------------- */
		eBandType = poSrcBand->GetRasterDataType();
		
		
/* -------------------------------------------------------------------- */
/*      Create this band.                                               */
/* -------------------------------------------------------------------- */
        poVDS->AddBand( eBandType, NULL );
        poVRTBand = (VRTSourcedRasterBand *) poVDS->GetRasterBand( i+1 );
            
/* -------------------------------------------------------------------- */
/*      Create a simple data source depending on the         */
/*      translation type required.                                      */
/* -------------------------------------------------------------------- */
		poVRTBand->AddSimpleSource( poSrcBand,
									anSrcWin[0], anSrcWin[1], 
									anSrcWin[2], anSrcWin[3], 
									0, 0, nOXSize, nOYSize );
		
/* -------------------------------------------------------------------- */
/*      copy over some other information of interest.                   */
/* -------------------------------------------------------------------- */
        poVRTBand->SetMetadata( poSrcBand->GetMetadata() );
        poVRTBand->SetColorTable( poSrcBand->GetColorTable() );
        poVRTBand->SetColorInterpretation(
            poSrcBand->GetColorInterpretation());
        if( strlen(poSrcBand->GetDescription()) > 0 )
            poVRTBand->SetDescription( poSrcBand->GetDescription() );
		
		if ( nodata ) 
            poVRTBand->SetNoDataValue(*nodata);
		else {
			int bSuccess;
			double dfNoData = poSrcBand->GetNoDataValue( &bSuccess );
			if ( bSuccess )
				poVRTBand->SetNoDataValue( dfNoData );
		}
    }

/* -------------------------------------------------------------------- */
/*      Write to the output file using CopyCreate().                    */
/* -------------------------------------------------------------------- */
    hOutDS = GDALCreateCopy( hDriver, pszDest, (GDALDatasetH) poVDS,
                             bStrict, papszCreateOptions, 
                             pfnProgress, NULL );

    GDALClose((GDALDatasetH) poVDS);
	CPLFree( panBandList );
  	
	
	return hOutDS;
}


/** 
 * Creates a layer for a given vector.
 * It copies the definition of all fields from input layer.
 *
 * @return The created layer.
 */
OGRLayer* starspan_createLayer(
    OGRLayer* inLayer,           // source layer
    Vector* outVector,           // vector where layer will be created
    const char *outLayerName     // name for created layer
) {
    if ( globalOptions.verbose ) {
        cout<< "starspan_createLayer: starting creation of output layer for vector " <<outVector->getName()<< " ...\n";
    }

    // get datasource to create layer:
    OGRDataSource *poDS = outVector->getDataSource();
    
    // get layer definition from the input layer:
    OGRFeatureDefn* inDefn = inLayer->GetLayerDefn();

    // create layer in output vector:
    OGRLayer* outLayer = poDS->CreateLayer(
            outLayerName, // "mini_raster_strip", 
            inLayer->GetSpatialRef(),
            inDefn->GetGeomType(),
            NULL
    );
    if ( !outLayer ) {
        cerr<< "Layer creation failed.\n";
        return 0;
    }
    
    // create field definitions from originating layer:
    for( int iAttr = 0; iAttr < inDefn->GetFieldCount(); iAttr++ ) {
        OGRFieldDefn* inField = inDefn->GetFieldDefn(iAttr);
        
        OGRFieldDefn outField(inField->GetNameRef(), inField->GetType());
        outField.SetWidth(inField->GetWidth());
        outField.SetPrecision(inField->GetPrecision());
        if ( outLayer->CreateField(&outField) != OGRERR_NONE ) {
            cerr<< "Creation of field failed: " <<inField->GetNameRef()<< "\n";
            return 0;
        }
    }
    
    return outLayer;
}

