//
// STARSpan project
// Carlos A. Rueda
// starspan_csv_raster_field - generate a CSV file from rasters given in a
// vector field
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <assert.h>

static const char* vector_filename;
static const char* raster_field_name;
static const char* raster_directory;
static string raster_filename;
static vector<const char*>* select_fields;
static const char* output_filename;
static FILE* output_file;
static bool new_file = false;
static OGRFeature* currentFeature;
static Raster* raster;



static void open_output_file() {
	// if file exists, append new rows. Otherwise create file.
	output_file = fopen(output_filename, "r+");
	if ( output_file ) {
		fseek(output_file, 0, SEEK_END);
		if ( globalOptions.verbose )
			fprintf(stdout, "Appending to existing file %s\n", output_filename);
	}
	else {
		// create output file
		output_file = fopen(output_filename, "w");
		if ( !output_file) {
			fprintf(stderr, "Cannot create %s\n", output_filename);
			exit(1);
		}
		new_file = true;
	}
}


static void processPoint(OGRPoint* point) {
	double x = point->getX();
	double y = point->getY();
	int col, row;
	raster->toColRow(x, y, &col, &row);
	
	void* band_values = raster->getBandValuesForPixel(col, row);
	if ( !band_values ) {
		// means NO intersection.
		return;
	}
	
	bool RID_already_included = false;  // could be factored out
	
	//
	// Add field values to new record:
	//

	// Add FID value:
	fprintf(output_file, "%ld", currentFeature->GetFID());
	
	// add attribute fields from source currentFeature to record:
	if ( select_fields ) {
		for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
			const int i = currentFeature->GetFieldIndex(*fname);
			if ( i < 0 ) {
				fprintf(stderr, "\n\tField `%s' not found\n", *fname);
				exit(1);
			}
			
			if ( raster_field_name == *fname ) {
				RID_already_included = true;
			}

			const char* str = currentFeature->GetFieldAsString(i);
			if ( strchr(str, ',') )
				fprintf(output_file, ",\"%s\"", str);   // quote string
			else
				fprintf(output_file, ",%s", str);
		}
	}
	else {
		// all fields
		int feature_field_count = currentFeature->GetFieldCount();
		for ( int i = 0; i < feature_field_count; i++ ) {
			const char* str = currentFeature->GetFieldAsString(i);
			if ( strchr(str, ',') )
				fprintf(output_file, ",\"%s\"", str);   // quote string
			else
				fprintf(output_file, ",%s", str);
		}
		RID_already_included = true;
	}

	if ( ! RID_already_included ) {
		// add RID field
		fprintf(output_file, ",%s", raster_filename.c_str());
	}
	
	
	// add (col,row) fields
	if ( !globalOptions.noColRow ) {
		fprintf(output_file, ",%d,%d", col, row);
	}
	
	// add (x,y) fields
	if ( !globalOptions.noXY ) {
		fprintf(output_file, ",%.3f,%.3f", x, y);
	}
	
	// add band values to record:
	char* ptr = (char*) band_values;
	char value[1024];
	GDALDataset* dataset = raster->getDataset();
	for ( int i = 0; i < dataset->GetRasterCount(); i++ ) {
		GDALRasterBand* band = dataset->GetRasterBand(i+1);
		GDALDataType bandType = band->GetRasterDataType();
		int typeSize = GDALGetDataTypeSize(bandType) >> 3;
		starspan_extract_string_value(bandType, ptr, value);
		fprintf(output_file, ",%s", value);
		
		// move to next piece of data in buffer:
		ptr += typeSize;
	}
	fprintf(output_file, "\n");
	
}


//
// only Point type is processed now
//
static void processGeometry(OGRGeometry* geometry) {
	OGRwkbGeometryType type = geometry->getGeometryType();
	switch ( type ) {
		case wkbPoint:
		case wkbPoint25D:
			processPoint((OGRPoint*) geometry);
			break;
			
		default:
			cerr<< (string(OGRGeometryTypeToName(type))
			    + ": intersection type not considered."
			);
	}
}



static void extract_pixels() {
	OGRGeometry* feature_geometry = currentFeature->GetGeometryRef();
	processGeometry(feature_geometry);
}



static void process_feature() {
	if ( globalOptions.verbose ) {
		fprintf(stdout, "\n\nFID: %ld", currentFeature->GetFID());
	}
	
	const int i = currentFeature->GetFieldIndex(raster_field_name);
	if ( i < 0 ) {
		fprintf(stderr, "\n\tField `%s' not found\n", raster_field_name);
		exit(1);
	}
	raster_filename = raster_directory;
	raster_filename += "/";
	raster_filename += currentFeature->GetFieldAsString(i);
	
	raster = new Raster(raster_filename.c_str());
	
	extract_pixels();
	
	delete raster;
}





////////////////////////////////////////////////////////////////////////////////

int starspan_csv_raster_field(
	const char*          _vector_filename,
	const char*          _raster_field_name,
	const char*          _raster_directory,
	vector<const char*>* _select_fields,
	const char*          _output_filename
) {
	vector_filename =   _vector_filename;
	raster_field_name = _raster_field_name;
	raster_directory =  _raster_directory;
	select_fields =     _select_fields;
	output_filename =   _output_filename;



	Vector vect(vector_filename);
	if ( vect.getLayerCount() > 1 ) {
		cerr<< "Vector datasource with more than one layer: "
		    << vect.getName()
			<< "\nOnly one layer expected.\n"
		;
		return 1;
	}
	OGRLayer* layer = vect.getLayer(0);
	if ( !layer ) {
		cerr<< "Couldn't get layer from " << vect.getName()<< endl;
		return 1;
	}
	layer->ResetReading();

	open_output_file();
	
	//
	// Was a specific FID given?
	//
	if ( globalOptions.FID >= 0 ) {
		currentFeature = layer->GetFeature(globalOptions.FID);
		if ( !currentFeature ) {
			cerr<< "FID " <<globalOptions.FID<< " not found in " <<vect.getName()<< endl;
			exit(1);
		}
		process_feature();
		delete currentFeature;
	}
	//
	// else: process each feature in vector datasource:
	//
	else {
		while( (currentFeature = layer->GetNextFeature()) != NULL ) {
			process_feature();
			delete currentFeature;
		}
	}	
	return 0;
}

