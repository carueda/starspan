//
// starspan declarations
// Carlos A. Rueda
// $Id$
//

#ifndef starspan_h
#define starspan_h

#include "Raster.h"           
#include "Vector.h"       
#include "traverser.h"       

#include <stdio.h> // FILE


#define STARSPAN_VERSION "0.5"


/////////////////////////////////////////////////////////////////////////////
// main operations:

/**
  * Generates a DBF table with the following structure:
  *     FID, col,row {vect-attrs}, {bands-from-raster}
  * where:
  *     FID:          feature ID as given by OGRFeature#GetFID()
  *     col,row:      pixel location relative to (0,0) in raster
  *     {vect-attrs}: attributes from vector
  *     {rast-bands}: bands from raster at corresponding location
  *
  * @param tr Data traverser
  * @param select_fields comma-separated field names
  * @param db_filename output file name
  *
  * @return 0 iff OK. 
  */
int starspan_db(
	Traverser& tr,
	const char* select_fields,     // comma-separated field names
	const char* db_filename
);

/**
  * Generates a CSV file with the following columns:
  *     FID, col,row {vect-attrs}, {bands-from-raster}
  * where:
  *     FID:          feature ID as given by OGRFeature#GetFID()
  *     col,row:      pixel location relative to (0,0) in raster
  *     {vect-attrs}: attributes from vector
  *     {rast-bands}: bands from raster at corresponding location
  *
  * @param tr Data traverser
  * @param select_fields comma-separated field names
  * @param csv_filename output file name
  *
  * @return 0 iff OK. 
  */
int starspan_csv(
	Traverser& tr, 
	const char* select_fields,     // comma-separated field names
	const char* csv_filename
);

/** Generates ENVI output
  * @param tr Data traverser
  * @param select_fields comma-separated field names
  * @param envisl_filename output file name
  * @param envi_image  true for image, false for spectral library
  *
    returns 0 iff OK. */
int starspan_gen_envisl(
	Traverser& tr,
	const char* select_fields,
	const char* envisl_name,
	bool envi_image
);

	

/**
  * Generate a JTS test.
  * @param tr Data traverser
  * @param use_polys If true, pixels are represented as polygons;
  *        otherwise as points.
  * @param jtstest_filename output file name
  */
void starspan_jtstest(
	Traverser& tr,
	bool use_polys,
	const char* jtstest_filename
);

/** Generate mini rasters
    returns 0 iff OK. */
int starspan_minirasters(
	Raster& rast, 
	Vector& vect, 
	const char* prefix,
	bool only_in_feature,
	const char* pszOutputSRS  // see gdal_translate option -a_srs 
	                         // If NULL, projection is taken from input dataset
);



/////////////////////////////////////////////////////////////////////////////
// misc and supporting utilities:

/** aux routine for reporting */ 
void starspan_report(Traverser& tr);

/** intersect two envelopes */
bool starspan_intersect_envelopes(OGREnvelope& oEnv1, OGREnvelope& oEnv2, OGREnvelope& envr);

void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env);

void starspan_myErrorHandler(CPLErr eErrClass, int err_no, const char *msg);

/** 
  * Creates a raster by subsetting a given raster
  */
GDALDatasetH starspan_subset_raster(
	GDALDatasetH hDataset,        // input dataset
	int          xoff,
	int          yoff,            // xoff,yoff being the ulx, uly in pixel/line
	int          xsize, 
	int          ysize,
	
	const char*  pszDest,         // output dataset name
	const char*  pszOutputSRS     // see -a_srs option for gdal_translate.
	                              // If NULL, projection is taken from input dataset
);


#endif

