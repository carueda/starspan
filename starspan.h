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
#include "Stats.h"       

#include <stdio.h> // FILE


#define STARSPAN_VERSION "0.84beta"


/////////////////////////////////////////////////////////////////////////////
// some services:

/**
  * Gets statistics for a given feature in a raster.
  * RESULT[s][b] = statistic s on band b
  * where:
  *    MIN <= s <= STDDEV 
  *    0 <= b < #bands in raster
  * @param FID
  * @param vect
  * @param rast
  * @param select_stats List of desired statistics (avg, mode, stdev, min, max)
  */
double** starspan_getFeatureStats(
	long FID, Vector* vect, Raster* rast,
	vector<const char*> select_stats
); 


/**
  * Gets statistics for a feature in a raster.
  * RESULT[s][b] = statistic s on band b
  * where:
  *    MIN <= s <= STDDEV 
  *    0 <= b < #bands in raster
  * @param field_name    Field name
  * @param field_value   Field value
  * @param vect
  * @param rast
  * @param select_stats List of desired statistics (avg, mode, stdev, min, max)
  * @param FID  Output: If not null, it'll have the corresponding FID.
  */
double** starspan_getFeatureStatsByField(
	const char* field_name, 
	const char* field_value, 
	Vector* vect, Raster* rast,
	double pix_prop,
	vector<const char*> select_stats,
	long *FID
); 



/////////////////////////////////////////////////////////////////////////////
// main operations:


/**
  * Generates a CSV file with the following columns:
  *     FID, link, RID, BandNumber, FieldBandValue, <s1>_ImageBandValue, <s2>_ImageBandValue, ...
  * where:
  *     FID:          (informative.  link is given by next field)
  *     link:         link field
  *     RID           raster filename
  *     BandNumber    1 .. N
  *     FieldBandValue  value from input speclib
  *     <s1>_ImageBandValue  
  *                   selected statistics for BandNumber in RID
  *                   example, avg_ImageBandValue.
  *
  * @param vector_filename Vector datasource
  * @param raster_filenames rasters
  * @param speclib_filename spectral library file name
  * @param pixprop A value assumed to be in [0.0, 1.0].
  * @param link_name Name of field to be used as link vector-speclib
  * @param select_stats List of desired statistics (avg, mode, stdev, min, max)
  * @param calbase_filename output file name
  *
  * @return 0 iff OK 
  */
int starspan_tuct_2(
	const char* vector_filename,
	vector<const char*> raster_filenames,
	const char* speclib_filename,
	double pix_prop,
	const char* link_name,
	vector<const char*> select_stats,
	const char* calbase_filename
);


/**
  * Generates a CSV file with the following columns:
  *     FID, RID, BandNumber, FieldBandValue, ImageBandValue
  * where:
  *     FID:          feature ID as given by OGRFeature#GetFID()
  *     RID           raster filename
  *     BandNumber    1 .. N
  *     FieldBandValue  value from input speclib
  *     ImageBandValue  MEAN value for BandNumber in RID
  *
  * @param vector_filename Vector datasource
  * @param raster_filenames rasters
  * @param speclib_filename spectral library file name
  * @param calbase_filename output file name
  *
  * @return 0 iff OK 
  */
int starspan_tuct_1(
	const char* vector_filename,
	vector<const char*> raster_filenames,
	const char* speclib_filename,
	const char* calbase_filename
);



/**
  * Gets an observer that computes statistics for each FID.
  *
  * @param tr Data traverser
  * @param select_stats List of desired statistics (avg, stdev, min, max)
  * @param select_fields desired fields
  * @param filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	vector<const char*> select_stats,
	vector<const char*>* select_fields,
	const char* filename
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
  * @param select_fields desired fields
  * @param csv_filename output file name
  * @param noColRow if true, no col,row fields will be included
  * @param noXY if true, no x,y fields will be included
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_csv(
	Traverser& tr, 
	vector<const char*>* select_fields,
	const char* csv_filename,
	bool noColRow,
	bool noXY
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
  * @param select_fields desired fields
  * @param db_filename output file name
  * @param noColRow if true, no col,row fields will be included
  * @param noXY if true, no x,y fields will be included
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_db(
	Traverser& tr,
	vector<const char*>* select_fields,
	const char* db_filename,
	bool noColRow,
	bool noXY
);



/** Generates ENVI output
  * @param tr Data traverser
  * @param select_fields desired fields
  * @param envisl_filename output file name
  * @param envi_image  true for image, false for spectral library
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_gen_envisl(
	Traverser& tr,
	vector<const char*>* select_fields,
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
  * Creates a ptplot-readable file with geometries and
  * grid
  * @param tr Data traverser
  * @param filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_dump(
	Traverser& tr,
	bool use_polys,
	const char* filename
);



/**
  * Generate a JTS test.
  * @param tr Data traverser
  * @param use_polys If true, pixels are represented as polygons;
  *        otherwise as points.
  * @param jtstest_filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_jtstest(
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

