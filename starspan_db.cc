//
// STARSpan project
// Carlos A. Rueda
// starspan_db - generate a DBF db
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include "shapefil.h"       

#include <stdlib.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////
/** maps an OGR field type to a corresponding DBF field type
  * I got this mapping by looking at the headers ogr_core.h and
  * shapefil.h, and also the OGRShapeLayer::CreateField method
  * in ogrshapelayer.cpp .
  */
static DBFFieldType fieldtype_2_dbftype(OGRFieldType ft) {
	switch(ft) {
		case OFTString: 
			return FTString;
		case OFTInteger: 
			return FTInteger;
		case OFTReal: 
			return FTDouble;
		default:
			fprintf(stderr, "fieldtype_2_dbftype: expecting: "
					"OFTString, OFTInteger, or OFTReal \n");
			exit(2);
	}
}

/**
  * Gets a value from a signature band as a double.
  */
static double extract_double_value(GDALDataType rasterType, char* sign) {
	double value;
	switch(rasterType) {
		case GDT_Byte:
			value = (double) *( (char*) sign );
			break;
		case GDT_UInt16:
			value = (double) *( (unsigned short*) sign );
			break;
		case GDT_Int16:
			value = (double) *( (short*) sign );
			break;
		case GDT_UInt32: 
			value = (double) *( (unsigned int*) sign );
			break;
		case GDT_Int32:
			value = (double) *( (int*) sign );
			break;
		case GDT_Float32:
			value = (double) *( (float*) sign );
			break;
		case GDT_Float64:
			value = (double) *( (double*) sign );
			break;
		default:
			fprintf(stderr, "Unexpected GDALDataType: %s\n", GDALGetDataTypeName(rasterType));
			exit(1);
	}
	return value;
}


/**
  *
  */
class DBObserver : public Observer {
public:
	Raster* rast; 
	Vector* vect;
	DBFHandle file;
	bool includePixelLocation;
	int numBands;
	OGRFeature* currentFeature;
	int next_record_index;	

	
	/**
	  * Creates fields: FID, col, row, fields-from-feature, bands-from-raster
	  */
	DBObserver(Raster* r, Vector* v, DBFHandle f) {
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
		// Create desired fields:
		//
		int next_field_index = 0;
		
		char field_name[64];
		DBFFieldType field_type;
		int field_width;
		int field_precision;


		// Create FID field
		field_type = FTInteger;
		field_width = 6;
		field_precision = 0;
		sprintf(field_name, "FID");
		fprintf(stdout, "Creating field: %s\n", field_name);
		DBFAddField(file, field_name, field_type, field_width, field_precision);
		next_field_index++;
		
		// Create (col,row) fields, if so indicated
		if ( includePixelLocation ) {
			field_type = FTInteger;
			field_width = 6;
			field_precision = 0;
			sprintf(field_name, "col");
			fprintf(stdout, "Creating field: %s\n", field_name);
			DBFAddField(file, field_name, field_type, field_width, field_precision);
			next_field_index++;
	
			sprintf(field_name, "row");
			fprintf(stdout, "Creating field: %s\n", field_name);
			DBFAddField(file, field_name, field_type, field_width, field_precision);
			next_field_index++;
		}
		
		// Create fields from layer definition 
		OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
		int field_count = poDefn->GetFieldCount();
		
		for ( int i = 0; i < field_count; i++ ) {
			OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
			const char* pfield_name = poField->GetNameRef();
			
			// is this field to be included?  
			// FIXME: now all fields are included.
			if ( true ) {
				field_type = fieldtype_2_dbftype(poField->GetType());
				field_width = poField->GetWidth();
				field_precision = poField->GetPrecision();
				fprintf(stdout, "Creating field: %s\n", pfield_name);
				DBFAddField(file, pfield_name, field_type, field_width, field_precision);
				next_field_index++;
			}
		}
		
		// Create fields for bands
		rast->getSize(NULL, NULL, &numBands);
		for ( int i = 0; i < numBands; i++ ) {
			field_type = FTDouble;
			field_width = 18;
			field_precision = 3;
			sprintf(field_name, "Band%d", i);
			fprintf(stdout, "Creating field: %s\n", field_name);
			DBFAddField(file, field_name, field_type, field_width, field_precision);
			next_field_index++;
		}
		
		next_record_index = 0;
		currentFeature = NULL;
		
		fprintf(stdout, "   %d fields created\n", next_field_index);
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
		int col = ev.pixelLocation.col;
		int row = ev.pixelLocation.row;
		void* signature = ev.signature;
		GDALDataType rasterType = ev.rasterType;
		int typeSize = ev.typeSize;
		
		//	fprintf(stdout, "signature %d\n", next_record_index);
		
		//
		// Add field values to new record:
		//
		int next_field_index = 0;

		// Add FID value:
		DBFWriteIntegerAttribute(
			file,
			next_record_index,            // int iShape -- record number
			next_field_index++,           // int iField,
			currentFeature->GetFID()
		);
		
		// add (col,row) fields
		if ( includePixelLocation ) {
			DBFWriteIntegerAttribute(
				file,
				next_record_index,            // int iShape -- record number
				next_field_index++,           // int iField,
				col
			);
			DBFWriteIntegerAttribute(
				file,
				next_record_index,            // int iShape -- record number
				next_field_index++,           // int iField,
				row
			);
		}
		
		// add attribute fields from source currentFeature to record:
		int field_count = currentFeature->GetFieldCount();
		for ( int i = 0; i < field_count; i++ ) {
			OGRFieldDefn* poField = currentFeature->GetFieldDefnRef(i);
			OGRFieldType ft = poField->GetType();
			switch(ft) {
				case OFTString: {
					const char* str = currentFeature->GetFieldAsString(i);
					DBFWriteStringAttribute(
						file,
						next_record_index,            // int iShape -- record number
						next_field_index++,           // int iField,
						str 
					);
					break;
				}
				case OFTInteger: { 
					int val = currentFeature->GetFieldAsInteger(i);
					DBFWriteIntegerAttribute(
						file,
						next_record_index,            // int iShape -- record number
						next_field_index++,           // int iField,
						val
					);
					break;
				}
				case OFTReal: { 
					double val = currentFeature->GetFieldAsDouble(i);
					DBFWriteDoubleAttribute(
						file,
						next_record_index,            // int iShape -- record number
						next_field_index++,           // int iField,
						val
					);
					break;
				}
				default:
					fprintf(stderr, "fieldtype_2_dbftype: expecting: "
							"OFTString, OFTInteger, or OFTReal \n");
					exit(2);
			}
		}
		 
		
		
		// add signature values to record:
		char* sign = (char*) signature;
		for ( int i = 0; i < numBands; i++, sign += typeSize ) {
			// extract value:
			double val = extract_double_value(rasterType, sign);
			
			int ok = DBFWriteDoubleAttribute(
				file,
				next_record_index,            // int iShape -- record number
				next_field_index++,           // int iField,
				val 
			);
			
			// false: I'm skipping this test for now.
			// FIXME: recheck DBFWriteDoubleAttribute documentation/source code
			//        so ok should be always true.
			if ( false && !ok ) {
				char field_name[64];
				sprintf(field_name, "Band%d", i);
				fprintf(stderr, "DBFWriteDoubleAttribute returned %d\nfield name %s, record %d\n", 
					ok, field_name, next_record_index
				);
				exit(1);
			}
		}
		next_record_index++;
	}
};



/**
  * implementation
  */
int starspan_db(Raster* rast, Vector* vect, const char* db_filename) {
	// create output file
	DBFHandle file = DBFCreate(db_filename);
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", db_filename);
		return 1;
	}

	DBObserver obs(rast, vect, file);	
	Traverser tr(rast, vect);
	tr.setObserver(&obs);
	tr.traverse();
	
	DBFClose(file);
	fprintf(stdout, "finished.\n");

	return 0;
}
		


