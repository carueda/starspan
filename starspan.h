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


#define STARSPAN_VERSION "0.7beta"


/////////////////////////////////////////////////////////////////////////////
// main operations:


/**
  * Gets an observer that computes statistics for each FID
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	const char* filename
);





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
  * @param noColRow if true, no col,row fields will be included
  * @param noXY if true, no x,y fields will be included
  *
  * @return 0 iff OK. 
  */
int starspan_db(
	Traverser& tr,
	const char* select_fields,     // comma-separated field names
	const char* db_filename,
	bool noColRow,
	bool noXY
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
  * @param noColRow if true, no col,row fields will be included
  * @param noXY if true, no x,y fields will be included
  *
  * @return 0 iff OK. 
  */
int starspan_csv(
	Traverser& tr, 
	const char* select_fields,     // comma-separated field names
	const char* csv_filename,
	bool noColRow,
	bool noXY
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
  * Updates a DBF
  * DOC ME!
  */
int starspan_update_dbf(
	const char* in_dbf_filename,
	vector<const char*> raster_filenames,
	const char* out_dbf_filename
);


/**
  * Updates a CSV
  * DOC ME!
  */
int starspan_update_csv(
	const char* in_csv_filename,
	vector<const char*> raster_filenames,
	const char* out_csv_filename
);





/**
  * Generate a JTS test.
  * @param tr Data traverser
  * @param use_polys If true, pixels are represented as polygons;
  *        otherwise as points.
  * @param jtstest_filename output file name
  * @return 0 iff OK.
  */
int starspan_jtstest(
	Traverser& tr,
	bool use_polys,
	const char* jtstest_filename
);

/** 
  * Generate mini rasters
  * @return 0 iff OK.
  */
int starspan_minirasters(
	Traverser& tr,
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


/**
  * Extracts a value from a buffer according to a type and returns it as a double.
  */
inline double starspan_extract_double_value(GDALDataType bandType, void* ptr) {
	double value;
	switch(bandType) {
		case GDT_Byte:
			value = (double) *( (char*) ptr );
			break;
		case GDT_UInt16:
			value = (double) *( (unsigned short*) ptr );
			break;
		case GDT_Int16:
			value = (double) *( (short*) ptr );
			break;
		case GDT_UInt32: 
			value = (double) *( (unsigned int*) ptr );
			break;
		case GDT_Int32:
			value = (double) *( (int*) ptr );
			break;
		case GDT_Float32:
			value = (double) *( (float*) ptr );
			break;
		case GDT_Float64:
			value = (double) *( (double*) ptr );
			break;
		default:
			fprintf(stderr, "Unexpected GDALDataType: %s\n", GDALGetDataTypeName(bandType));
			exit(1);
	}
	return value;
}




#endif

