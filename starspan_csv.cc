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
  * Gets a value from a band as a string.
  */
static char* extract_value(GDALDataType bandType, char* sign) {
	static char value[1024];
	
	switch(bandType) {
		case GDT_Byte:
			sprintf(value, "%d", (int) *( (char*) sign ));
			break;
		case GDT_UInt16:
			sprintf(value, "%u", *( (unsigned short*) sign ));
			break;
		case GDT_Int16:
			sprintf(value, "%d", *( (short*) sign ));
			break;
		case GDT_UInt32:
			sprintf(value, "%u", *( (unsigned int*) sign ));
			break;
		case GDT_Int32:
			sprintf(value, "%u", *( (int*) sign ));
			break;
		case GDT_Float32:
			sprintf(value, "%f", *( (float*) sign ));
			break;
		case GDT_Float64:
			sprintf(value, "%f", *( (double*) sign ));
			break;
		default:
			fprintf(stderr, "Unexpected GDALDataType: %s\n", GDALGetDataTypeName(bandType));
			exit(1);
	}
	return value;
}


/**
  * This observer extracts from ONE raster
  */
class CSVObserver : public Observer {
public:
	GlobalInfo* global_info;
	Vector* vect;
	OGRFeature* currentFeature;
	vector<const char*>* select_fields;
	bool write_header;
	FILE* file;
	
	/**
	  * Creates a csv creator
	  */
	CSVObserver(Traverser& tr, vector<const char*>* select_fields, bool write_header, FILE* f)
	: select_fields(select_fields), write_header(write_header), file(f)
	{
		vect = tr.getVector();
		global_info = 0;
	}
	
	/**
	  * Creates first line with column headers:
	  *    FID, [col,row,] [x,y,] fields-from-feature, RID, bands-from-raster
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		OGRLayer* poLayer = vect->getLayer(0);
		if ( !poLayer ) {
			fprintf(stderr, "Couldn't fetch layer 0\n");
			exit(1);
		}

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
		fprintf(file, ",<<RID-PENDING>>");
		
		
		// add (col,row) fields
		if ( !globalOptions.noColRow ) {
			fprintf(file, ",%d,%d", col, row);
		}
		
		// add (x,y) fields
		if ( !globalOptions.noXY ) {
			fprintf(file, ",%.3f,%.3f", ev.pixel.x, ev.pixel.y);
		}
		
		// add band values to record:
		char* sign = (char*) band_values;
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			int typeSize = GDALGetDataTypeSize(bandType) >> 3;
			fprintf(file, ",%s", extract_value(bandType, sign));
			
			// move to next piece of data in buffer:
			sign += typeSize;
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
	// create output file
	FILE* file = fopen(csv_filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", csv_filename);
		return 1;
	}

	Vector vect(vector_filename);
	Traverser tr;
	tr.setVector(&vect);
	if ( globalOptions.pix_prop >= 0.0 )
		tr.setPixelProportion(globalOptions.pix_prop);
	if ( globalOptions.FID >= 0 )
		tr.setDesiredFID(globalOptions.FID);
	tr.setVerbose(globalOptions.verbose);
	if ( globalOptions.progress )
		tr.setProgress(globalOptions.progress_perc, cout);
	tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);
	
	Raster* rasters[raster_filenames.size()];
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		rasters[i] = new Raster(raster_filenames[i]);
	}
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		bool write_header = i == 0;
		tr.removeRasters();
		tr.addRaster(rasters[i]);
		
		Observer* obs = new CSVObserver(tr, select_fields, write_header, file);
		tr.addObserver(obs);
		
		tr.traverse();

		if ( globalOptions.report_summary ) {
			tr.reportSummary();
		}
		tr.releaseObservers();
	}
	
	fclose(file);
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		delete rasters[i];
	}
	
	return 0;
}

