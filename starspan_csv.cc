//
// STARSpan project
// Carlos A. Rueda
// starspan_csv - generate a CSV file
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
  * Creates fields and populates the table.
  */
class CSVObserver : public Observer {
public:
	GlobalInfo* global_info;
	Vector* vect;
	FILE* file;
	bool noColRow, noXY;
	OGRFeature* currentFeature;
	vector<const char*>* select_fields;
	
	/**
	  * Creates a csv creator
	  */
	CSVObserver(Traverser& tr, FILE* f, vector<const char*>* select_fields,
		bool noColRow, bool noXY)
	: file(f), noColRow(noColRow), noXY(noXY), select_fields(select_fields)
	{
		vect = tr.getVector();
		global_info = 0;
	}
	
	/**
	  * Creates first line with column headers:
	  *    FID, [col,row,] [x,y,] fields-from-feature, bands-from-raster
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		OGRLayer* poLayer = vect->getLayer(0);
		if ( !poLayer ) {
			fprintf(stderr, "Couldn't fetch layer 0\n");
			exit(1);
		}

		//		
		// write column headers:
		//

		// Create FID field
		fprintf(file, "FID");
		
		// Create (col,row) fields, if so indicated
		if ( !noColRow ) {
			fprintf(file, ",col");
			fprintf(file, ",row");
		}
		
		// Create (x,y) fields, if so indicated
		if ( !noXY ) {
			fprintf(file, ",x");
			fprintf(file, ",y");
		}
		
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
		
		// Create fields for bands
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			fprintf(file, ",Band%d", i+1);
		}
		
		fprintf(file, "\n");
		
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
		
		// add (col,row) fields
		if ( !noColRow ) {
			fprintf(file, ",%d,%d", col, row);
		}
		
		// add (x,y) fields
		if ( !noXY ) {
			fprintf(file, ",%.3f,%.3f", ev.pixel.x, ev.pixel.y);
		}
		
		// add attribute fields from source currentFeature to record:
		if ( select_fields ) {
			for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
				const int i = currentFeature->GetFieldIndex(*fname);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", *fname);
					exit(1);
				}
				const char* str = currentFeature->GetFieldAsString(i);
				fprintf(file, ",%s", str);
			}
		}
		else {
			// all fields
			int feature_field_count = currentFeature->GetFieldCount();
			for ( int i = 0; i < feature_field_count; i++ ) {
				const char* str = currentFeature->GetFieldAsString(i);
				fprintf(file, ",%s", str);
			}
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
	  * closes the file
	  */
	void end() {
		if ( file ) {
			fclose(file);
			fprintf(stdout, "csv: finished.\n");
			file = 0;
		}
	}
};



/**
  * implementation
  */
Observer* starspan_csv(
	Traverser& tr,
	vector<const char*>* select_fields,
	const char* filename,
	bool noColRow,
	bool noXY
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", filename);
		return 0;
	}

	return new CSVObserver(tr, file, select_fields, noColRow, noXY);	
}
		


