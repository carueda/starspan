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
	Raster* rast; 
	Vector* vect;
	GDALDataType bandType;
	int typeSize;
	FILE* file;
	bool includePixelLocation;
	int numBands;
	OGRFeature* currentFeature;
	const char* select_fields;
	
	/**
	  * Creates first line with column headers:
	  *    FID, col, row, fields-from-feature, bands-from-raster
	  */
	CSVObserver(Traverser& tr, FILE* f, const char* select_fields_)
	: file(f), select_fields(select_fields_) 
	{
		rast = tr.getRaster();
		vect = tr.getVector();
		
		// PENDING maybe read this from a parameter
		includePixelLocation = true;
		
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
		if ( includePixelLocation ) {
			fprintf(file, ",col");
			fprintf(file, ",row");
		}
		
		// Create fields:
		if ( select_fields ) {
			char buff[strlen(select_fields) + 1];
			strcpy(buff, select_fields);
			for ( char* fname = strtok(buff, ","); fname; fname = strtok(NULL, ",") ) {
				fprintf(file, ",%s", fname);
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
		rast->getSize(NULL, NULL, &numBands);
		for ( int i = 0; i < numBands; i++ ) {
			fprintf(file, ",Band%d", i+1);
		}
		
		fprintf(file, "\n");
		
		currentFeature = NULL;
	}
	
	/**
	  *
	  */
	void init(GlobalInfo& info) { 
		bandType = info.band.type;
		typeSize = info.band.typeSize;
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
		int col = ev.pixel.col;
		int row = ev.pixel.row;
		void* band_values = ev.bandValues;
		
		//
		// Add field values to new record:
		//

		// Add FID value:
		fprintf(file, "%ld", currentFeature->GetFID());
		
		// add (col,row) fields
		if ( includePixelLocation ) {
			fprintf(file, ",%d,%d", col, row);
		}
		
		// add attribute fields from source currentFeature to record:
		if ( select_fields ) {
			char buff[strlen(select_fields) + 1];
			strcpy(buff, select_fields);
			for ( char* fname = strtok(buff, ","); fname; fname = strtok(NULL, ",") ) {
				const int i = currentFeature->GetFieldIndex(fname);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", fname);
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
		for ( int i = 0; i < numBands; i++, sign += typeSize ) {
			// extract value:
			fprintf(file, ",%s", extract_value(bandType, sign));
		}
		fprintf(file, "\n");
	}
};



/**
  * implementation
  */
int starspan_csv(
	Traverser& tr,
	const char* select_fields,     // comma-separated field names
	const char* filename
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", filename);
		return 1;
	}

	CSVObserver obs(tr, file, select_fields);	
	tr.setObserver(&obs);
	tr.traverse();
	
	fclose(file);
	fprintf(stdout, "finished.\n");

	return 0;
}
		


