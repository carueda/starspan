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
  * Creates fields and populates the table.
  */
class DBObserver : public Observer {
public:
	GlobalInfo* global_info;
	Vector* vect;
	DBFHandle file;
	bool includePixelLocation;
	OGRFeature* currentFeature;
	int next_record_index;
	const char* select_fields;	

	
	/**
	  * Creates fields: FID, col, row, fields-from-feature, bands-from-raster
	  */
	DBObserver(Traverser& tr, DBFHandle f, const char* select_fields_)
	: file(f), select_fields(select_fields_) 
	{
		// PENDING maybe read this from a parameter
		includePixelLocation = true;
		
		vect = tr.getVector();
		global_info = 0;
	}
	
	
	/**
	  *
	  */
	void init(GlobalInfo& info) { 
		global_info = &info;

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
		if ( select_fields ) {
			char buff[strlen(select_fields) + 1];
			strcpy(buff, select_fields);
			for ( char* fname = strtok(buff, ","); fname; fname = strtok(NULL, ",") ) {
				const int i = poDefn->GetFieldIndex(fname);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", fname);
					exit(1);
				}
				OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
				field_type = fieldtype_2_dbftype(poField->GetType());
				field_width = poField->GetWidth();
				field_precision = poField->GetPrecision();
				fprintf(stdout, "Creating field: %s\n", fname);
				DBFAddField(file, fname, field_type, field_width, field_precision);
				next_field_index++;
			}
		}
		else {
			int feature_field_count = poDefn->GetFieldCount();
			
			for ( int i = 0; i < feature_field_count; i++ ) {
				OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
				const char* fname = poField->GetNameRef();
				
				field_type = fieldtype_2_dbftype(poField->GetType());
				field_width = poField->GetWidth();
				field_precision = poField->GetPrecision();
				fprintf(stdout, "Creating field: %s\n", fname);
				DBFAddField(file, fname, field_type, field_width, field_precision);
				next_field_index++;
			}
		}

		// Create fields for bands
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			field_type = FTDouble;
			field_width = 18;
			field_precision = 3;
			sprintf(field_name, "Band%d", i+1);
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
	

	void write_field(int i, int next_field_index) {
		OGRFieldDefn* poField = currentFeature->GetFieldDefnRef(i);
		OGRFieldType ft = poField->GetType();
		switch(ft) {
			case OFTString: {
				const char* str = currentFeature->GetFieldAsString(i);
				DBFWriteStringAttribute(
					file,
					next_record_index,            // int iShape -- record number
					next_field_index,           // int iField,
					str 
				);
				break;
			}
			case OFTInteger: { 
				int val = currentFeature->GetFieldAsInteger(i);
				DBFWriteIntegerAttribute(
					file,
					next_record_index,            // int iShape -- record number
					next_field_index,           // int iField,
					val
				);
				break;
			}
			case OFTReal: { 
				double val = currentFeature->GetFieldAsDouble(i);
				DBFWriteDoubleAttribute(
					file,
					next_record_index,            // int iShape -- record number
					next_field_index,           // int iField,
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
		if ( select_fields ) {
			char buff[strlen(select_fields) + 1];
			strcpy(buff, select_fields);
			for ( char* fname = strtok(buff, ","); fname; fname = strtok(NULL, ",") ) {
				const int i = currentFeature->GetFieldIndex(fname);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", fname);
					exit(1);
				}
				write_field(i, next_field_index++);
			}
		}
		else {
			// all fields
			int feature_field_count = currentFeature->GetFieldCount();
			for ( int i = 0; i < feature_field_count; i++ ) {
				write_field(i, next_field_index++);
			}
		}		 
		
		
		// add band values to record:
		char* sign = (char*) band_values;
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			int typeSize = GDALGetDataTypeSize(bandType) >> 3;
			double val = starspan_extract_double_value(bandType, sign);
			
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
				sprintf(field_name, "Band%d", i+1);
				fprintf(stderr, "DBFWriteDoubleAttribute returned %d\nfield name %s, record %d\n", 
					ok, field_name, next_record_index
				);
				exit(1);
			}

			// move to next piece of data in buffer:
			sign += typeSize;
		}
		next_record_index++;
	}
};



/**
  * implementation
  */
int starspan_db(
	Traverser& tr,
	const char* select_fields,
	const char* filename
) {
	// create output file
	DBFHandle file = DBFCreate(filename);
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", filename);
		return 1;
	}

	DBObserver obs(tr, file, select_fields);	
	tr.setObserver(&obs);
	tr.traverse();
	
	DBFClose(file);
	fprintf(stdout, "finished.\n");

	return 0;
}
		


