//
// STARSpan project
// Carlos A. Rueda
// starspan_csv - generate a CSV file from multiple rasters
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <assert.h>


/**
  * This observer extracts from ONE raster
  */
class CSVObserver : public Observer {
public:
	GlobalInfo* global_info;
	Vector* vect;
	OGRLayer* poLayer;
	OGRFeature* currentFeature;
	vector<const char*>* select_fields;
	const char* raster_filename;
	bool write_header;
	FILE* file;
	
	/**
	  * Creates a csv creator
	  */
	CSVObserver(Vector* vect, vector<const char*>* select_fields, FILE* f)
	: vect(vect), select_fields(select_fields), file(f)
	{
		global_info = 0;
		poLayer = vect->getLayer(0);
		if ( !poLayer ) {
			fprintf(stderr, "Couldn't fetch layer 0\n");
			exit(1);
		}
	}
	
	/**
	  * If write_header is true, it creates first line with 
	  * column headers:
	  *    FID, fields-from-feature, [col,row,] [x,y,] RID, bands-from-raster
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		if ( write_header ) {
			//		
			// write column headers:
			//
	
			// Create FID field
			fprintf(file, "FID");
			
			// Create fields:
			if ( select_fields ) {
				for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
					fprintf(file, ",%s", *fname);
				}
			}
			else {
				// all fields from layer definition
				OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
				int feature_field_count = poDefn->GetFieldCount();
				
				for ( int i = 0; i < feature_field_count; i++ ) {
					OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
					const char* pfield_name = poField->GetNameRef();
					fprintf(file, ",%s", pfield_name);
				}
			}
			
			// RID field header
			fprintf(file, ",RID");
			
			// Create (col,row) fields, if so indicated
			if ( !globalOptions.noColRow ) {
				fprintf(file, ",col");
				fprintf(file, ",row");
			}
			
			// Create (x,y) fields, if so indicated
			if ( !globalOptions.noXY ) {
				fprintf(file, ",x");
				fprintf(file, ",y");
			}
			
			// Create fields for bands
			for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
				fprintf(file, ",Band%d", i+1);
			}
			
			fprintf(file, "\n");
		}
		
		currentFeature = NULL;
	}
	

	/**
	  * Used here to update currentFeature
	  */
	void intersectionFound(OGRFeature* feature) {
		currentFeature = feature;
	}
	
	
	/**
	  * Adds a record to the output file.
	  */
	void addPixel(TraversalEvent& ev) { 
		int col = 1 + ev.pixel.col;
		int row = 1 + ev.pixel.row;
		void* band_values = ev.bandValues;
		
		//
		// Add field values to new record:
		//

		// Add FID value:
		fprintf(file, "%ld", currentFeature->GetFID());
		
		// add attribute fields from source currentFeature to record:
		if ( select_fields ) {
			for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
				const int i = currentFeature->GetFieldIndex(*fname);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", *fname);
					exit(1);
				}
				const char* str = currentFeature->GetFieldAsString(i);
				if ( strchr(str, ',') )
					fprintf(file, ",\"%s\"", str);   // quote string
				else
					fprintf(file, ",%s", str);
			}
		}
		else {
			// all fields
			int feature_field_count = currentFeature->GetFieldCount();
			for ( int i = 0; i < feature_field_count; i++ ) {
				const char* str = currentFeature->GetFieldAsString(i);
				if ( strchr(str, ',') )
					fprintf(file, ",\"%s\"", str);   // quote string
				else
					fprintf(file, ",%s", str);
			}
		}

		// add RID field
		fprintf(file, ",%s", raster_filename);
		
		
		// add (col,row) fields
		if ( !globalOptions.noColRow ) {
			fprintf(file, ",%d,%d", col, row);
		}
		
		// add (x,y) fields
		if ( !globalOptions.noXY ) {
			fprintf(file, ",%.3f,%.3f", ev.pixel.x, ev.pixel.y);
		}
		
		// add band values to record:
		char* ptr = (char*) band_values;
		char value[1024];
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			int typeSize = GDALGetDataTypeSize(bandType) >> 3;
			
			starspan_extract_string_value(bandType, ptr, value);
			fprintf(file, ",%s", value);
			
			// move to next piece of data in buffer:
			ptr += typeSize;
		}
		fprintf(file, "\n");
	}

	/**
	  * does nothing
	  */
	void end() {}
};



////////////////////////////////////////////////////////////////////////////////

//
// Each raster is processed independently
//
int starspan_csv(
	const char* vector_filename,
	vector<const char*> raster_filenames,
	vector<const char*>* select_fields,
	const char* csv_filename
) {
	FILE* file;
	bool new_file = false;
	
	// if file exists, append new rows. Otherwise create file.
	file = fopen(csv_filename, "r+");
	if ( file ) {
		if ( globalOptions.verbose )
			fprintf(stdout, "Appending to existing file %s\n", csv_filename);

		fseek(file, 0, SEEK_END);

		// check that new data will start in a new line:
		// if last character is not '\n', then write a '\n':
		// (This check will make the output more robust in case
		// the previous information is not properly aligned, eg.
		// when the previous generation was killed for some reason.)
		long endpos = ftell(file);
		if ( endpos > 0 ) {
			fseek(file, endpos -1, SEEK_SET);
			char c;
			if ( 1 == fread(&c, sizeof(c), 1, file) ) {
				if ( c != '\n' )
					fputc('\n', file);    // add a new line
			}
		}
	}
	else {
		// create output file
		file = fopen(csv_filename, "w");
		if ( !file) {
			fprintf(stderr, "Cannot create %s\n", csv_filename);
			return 1;
		}
		new_file = true;
	}

	Vector vect(vector_filename);

	CSVObserver obs(&vect, select_fields, file);

	Traverser tr;
	tr.addObserver(&obs);

	tr.setVector(&vect);
	if ( globalOptions.pix_prop >= 0.0 )
		tr.setPixelProportion(globalOptions.pix_prop);
	if ( globalOptions.FID >= 0 )
		tr.setDesiredFID(globalOptions.FID);
	tr.setVerbose(globalOptions.verbose);
	if ( globalOptions.progress ) {
		tr.setProgress(globalOptions.progress_perc, cout);
		cout << "Number of features: ";
		long psize = vect.getLayer(0)->GetFeatureCount();
		if ( psize >= 0 )
			cout << psize;
		else
			cout << "(not known in advance)";
		cout<< endl;
	}
	tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);
	
	Raster* rasters[raster_filenames.size()];
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		rasters[i] = new Raster(raster_filenames[i]);
	}
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		fprintf(stdout, "%3u: Extracting from %s\n", i+1, raster_filenames[i]);
		obs.raster_filename = raster_filenames[i];
		obs.write_header = new_file && i == 0;
		tr.removeRasters();
		tr.addRaster(rasters[i]);
		
		tr.traverse();

		if ( globalOptions.report_summary ) {
			tr.reportSummary();
		}
	}
	
	fclose(file);
	
	/***   PENDING
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		delete rasters[i];
	}
	***/
	
	return 0;
}

