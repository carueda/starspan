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

	
	/**
	  * Creates first line with column headers:
	  *    FID, col, row, fields-from-feature, bands-from-raster
	  */
	CSVObserver(Raster* r, Vector* v, FILE* f) {
		rast = r;
		vect = v;
		file = f;
		
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
		
		// Create fields from layer definition 
		OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
		int feature_field_count = poDefn->GetFieldCount();
		
		for ( int i = 0; i < feature_field_count; i++ ) {
			OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
			const char* pfield_name = poField->GetNameRef();
			
			// is this field to be included?  
			// FIXME: now all fields are included.
			if ( true ) {
				fprintf(file, ",%s", pfield_name);
			}
		}
		
		// Create fields for bands
		rast->getSize(NULL, NULL, &numBands);
		for ( int i = 0; i < numBands; i++ ) {
			fprintf(file, ",Band%d", i);
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
		int feature_field_count = currentFeature->GetFieldCount();
		for ( int i = 0; i < feature_field_count; i++ ) {
			OGRFieldDefn* poField = currentFeature->GetFieldDefnRef(i);
			OGRFieldType ft = poField->GetType();
			switch(ft) {
				case OFTString: {
					const char* str = currentFeature->GetFieldAsString(i);
					fprintf(file, ",%s", str);
					break;
				}
				case OFTInteger: { 
					int val = currentFeature->GetFieldAsInteger(i);
					fprintf(file, ",%d", val);
					break;
				}
				case OFTReal: { 
					double val = currentFeature->GetFieldAsDouble(i);
					fprintf(file, ",%f", val);
					break;
				}
				default:
					fprintf(stderr, "CSVObserver: expecting: "
							"OFTString, OFTInteger, or OFTReal \n");
					exit(2);
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
int starspan_csv(Raster* rast, Vector* vect, const char* filename) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", filename);
		return 1;
	}

	CSVObserver obs(rast, vect, file);	
	Traverser tr(rast, vect);
	tr.setObserver(&obs);
	tr.traverse();
	
	fclose(file);
	fprintf(stdout, "finished.\n");

	return 0;
}
		


