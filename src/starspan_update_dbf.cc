//
// STARSpan project
// Carlos A. Rueda
// starspan_update_dbf - update a DBF db
// $Id$
//
//	NOTE: As of 2005/01/23 this file is no longer used.

#include "starspan.h"           
#include "shapefil.h"       

#include <stdlib.h>
#include <assert.h>


/**
  * implementation
  */
int starspan_update_dbf(
	const char* in_dbf_filename,
	vector<const char*> raster_filenames,
	const char* out_dbf_filename
) {
	// open input file
	DBFHandle in_file = DBFOpen(in_dbf_filename, "r");
	if ( !in_file ) {
		fprintf(stderr, "Couldn't open %s\n", in_dbf_filename);
		return 1;
	}
	
	// first attempt is to use x,y fields
	bool use_xy = true;

	// check fields x,y in dbf file:
	int x_field_index = DBFGetFieldIndex(in_file, "x");
	int y_field_index = DBFGetFieldIndex(in_file, "y");
	int col_field_index = DBFGetFieldIndex(in_file, "col");
	int row_field_index = DBFGetFieldIndex(in_file, "row");
	
	if ( x_field_index < 0 || y_field_index < 0 ) {
		fprintf(stdout, "Warning: No fields 'x' and/or 'y' not present in %s\n", in_dbf_filename);
		fprintf(stdout, "         Will try with col,row fields ...\n");
		use_xy = false;
		if ( col_field_index < 0 || row_field_index < 0 ) {
			DBFClose(in_file);
			fprintf(stdout, "No fields 'col' and/or 'row' present in %s\n", in_dbf_filename);
			return 1;
		}
	}

	// open raster datasets:
	vector<Raster*> rasts;
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		rasts.push_back(new Raster(raster_filenames[i]));
	}

	// create output file
	DBFHandle out_file = DBFCreate(out_dbf_filename);
	if ( !out_file ) {
		DBFClose(in_file);
		fprintf(stderr, "Couldn't create %s\n", out_dbf_filename);
		return 1;
	}


	//
	// copy existing field definitions
	//
	const int num_existing_fields = DBFGetFieldCount(in_file);
	for ( int i = 0; i < num_existing_fields; i++ ) {
		char field_name[1024];
		DBFFieldType field_type;
		int field_width;
		int field_precision;
		field_type = DBFGetFieldInfo(in_file, i, field_name, &field_width, &field_precision);
		
		fprintf(stdout, "Creating field: %s\n", field_name);
		DBFAddField(out_file, field_name, field_type, field_width, field_precision);
	}


	// create new field definitions in dbf:	
	int next_field_index = num_existing_fields;
	for ( unsigned r = 0; r < raster_filenames.size(); r++ ) {
		const char* rast_name = raster_filenames[r];
		Raster* rast = rasts[r];
		int bands;
		rast->getSize(NULL, NULL, &bands);
		for ( int b = 0; b < bands; b++ ) {
			char field_name[1024];
			DBFFieldType field_type = FTDouble;
			int field_width = 18;
			int field_precision = 3;
			sprintf(field_name, "Band_%d_%s", b+1, rast_name);
			fprintf(stdout, "Creating field: %s\n", field_name);
			int actual_index = DBFAddField(out_file, field_name, field_type, field_width, field_precision);
			if ( actual_index < 0 ) {
				DBFClose(in_file);
				DBFClose(out_file);
				fprintf(stderr, "\tCould not create field\n");
				return 1;
			}
			assert(actual_index == next_field_index); 	
			next_field_index++;
		}
	}
	
	
	//
	// main body of processing
	//
	
	// for each point point in input dbf...
	int num_records = DBFGetRecordCount(in_file);
	fprintf(stdout, "processing %d records...\n", num_records);
	for ( int record = 0; record < num_records; record++ ) {
		fprintf(stdout, "record %d  ", record);
		//
		// copy existing field values
		//
		for ( int i = 0; i < num_existing_fields; i++ ) {
			DBFFieldType field_type;
			field_type = DBFGetFieldInfo(in_file, i, NULL, NULL, NULL);
			switch ( field_type ) {
				case FTString: {
					const char* str = DBFReadStringAttribute(in_file, record, i);
					DBFWriteStringAttribute(out_file, record, i, str);
					break;
				}
				case FTInteger: {
					int val = DBFReadIntegerAttribute(in_file, record, i);
					DBFWriteIntegerAttribute(out_file, record, i, val);
					break;
				}
				case FTDouble: {
					double val = DBFReadDoubleAttribute(in_file, record, i);
					DBFWriteDoubleAttribute(out_file, record, i, val);
					break;
				}
				default:
					fprintf(stderr, "Unexpected field type: %d\n", field_type);
					exit(1);
			}
		}
		
		
		//
		// now extract desired pixel from given rasters
		//
		
		double x = 0, y = 0;
		int col = 0, row = 0;
		
		if ( use_xy ) {
			x = DBFReadDoubleAttribute(in_file, record, x_field_index);
			y = DBFReadDoubleAttribute(in_file, record, y_field_index);
			fprintf(stdout, "x , y = %g , %g\n", x, y);
		}
		else {
			col = DBFReadIntegerAttribute(in_file, record, col_field_index);
			row = DBFReadIntegerAttribute(in_file, record, row_field_index);
			fprintf(stdout, "col , row = %d , %d\n", col, row);
			// make col and row 0-based:
			--col;
			--row;
		}
		
		
		// for each raster...
		next_field_index = num_existing_fields;
		for ( unsigned r = 0; r < rasts.size(); r++ ) {
			Raster* rast = rasts[r];
			int bands;
			rast->getSize(NULL, NULL, &bands);
			GDALDataset* dataset = rast->getDataset();

			if ( use_xy ) {
				// convert from (x,y) to (col,row) in this rast
				rast->toColRow(x, y, &col, &row);
				//fprintf(stdout, "x,y = %g , %g \n", x, y);
			}
			// else: (col,row) already given above.
			
			
			//
			// extract pixel at (col,row) from rast
			char* ptr = (char*) rast->getBandValuesForPixel(col, row);
			if ( ptr ) {
				// add these bands to DBF
				for ( int b = 0; b < bands; b++ ) {
					GDALRasterBand* band = dataset->GetRasterBand(b+1);
					GDALDataType bandType = band->GetRasterDataType();

					double val = starspan_extract_double_value(bandType, ptr);
					DBFWriteDoubleAttribute(out_file, record, next_field_index + b, val);

					int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
					ptr += bandTypeSize;
				}
			}
		}
	}

	// close files:
	for ( unsigned r = 0; r < rasts.size(); r++ ) {
		delete rasts[r];
	}
	DBFClose(in_file);
	DBFClose(out_file);

	fprintf(stdout, "finished.\n");
	return 0;
}
		


