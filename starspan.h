//
// starspan declarations
// Carlos A. Rueda
// $Id$
//

#ifndef starspan_h
#define starspan_h

#include "Raster.h"           
#include "Vector.h"       

#include <stdio.h> // FILE


/////////////////////////////////////////////////////////////////////////////
// main operations:

/** Generates envi spectral library ... (preliminaries)
    returns 0 iff OK. */
int starspan_gen_envisl(
	Raster* rast, 
	Vector* vect, 
	const char* envisl_name,
	const char* pszOutputSRS  // see gdal_translate option -a_srs 
	                         // If NULL, projection is taken from input dataset
);

	
/** Generates db ... (preliminaries)
    returns 0 iff OK. */
int starspan_db(
	Raster* rast, 
	Vector* vect, 
	const char* db_filename,
	bool only_in_feature
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


/** Generate a JTS test */
void starspan_jtstest(Raster& rast, Vector& vect, const char* jtstest_filename);


/////////////////////////////////////////////////////////////////////////////
// misc and supporting utilities:

/** aux routine for reporting */ 
void starspan_report(Raster* rast, Vector* vect);

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

