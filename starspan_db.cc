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

///////////////////////////////////////////////////////////////////////
class DBObserver : public Observer {
public:
	Raster* rast; 
	Vector* vect;
	DBFHandle file;
	int first_band_field_index;
	int numBands;
	int no_records;	
	
	
	DBObserver(Raster* r, Vector* v, DBFHandle f);
	
	
	void intersection(int feature_id, OGREnvelope intersection_env) {
		// ?
	}
	
	void addSignature(double x, double y, void* signature, GDALDataType rasterType, int typeSize);
};



///////////////////////////////////////////////////////////////////////
DBObserver::DBObserver(Raster* r, Vector* v, DBFHandle f) {
	rast = r;
	vect = v;
	file = f;
	
	OGRLayer* poLayer = vect->getLayer(0);
	if ( !poLayer ) {
		fprintf(stderr, "Couldn't fetch layer 0\n");
		exit(1);
	}
	
	
	// Create desired fields:
	
	// first, create fields from definition 
	OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
	int field_count = poDefn->GetFieldCount();
	
	field_count = 0;  // for a tesnting only with bands, see below
	
	first_band_field_index = field_count;  // for next section

	printf("Field Count in layer 0: %d\n", field_count);
	for ( int i = 0; i < field_count; i++ ) {
		OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
		const char* field_name = poField->GetNameRef();
		const DBFFieldType field_type = fieldtype_2_dbftype(poField->GetType());
		const int field_width = poField->GetWidth();
		const int field_precision = poField->GetPrecision();
		
		fprintf(stdout, "Creating field: %s\n", field_name);
		
		if ( true ) { // field_name is to be included-- PENDING
			DBFAddField(
				file,
				field_name,
				field_type,
				field_width,
				field_precision
			);
		}
	}
	
	// then, create fields for bands
	rast->getSize(NULL, NULL, &numBands);
	for ( int i = 0; i < numBands; i++ ) {
		char field_name[64];
		const DBFFieldType field_type = FTDouble;
		const int field_width = 18;
		const int field_precision = 3;
		
		sprintf(field_name, "Band%d", i);
		
		fprintf(stdout, "Creating field: %s\n", field_name);
		DBFAddField(
			file,
			field_name,
			field_type,
			field_width,
			field_precision
		);
	}
	
	no_records = 0;
}


///////////////////////////////////////////////////////////////////////
void DBObserver::addSignature(double x, double y, void* signature, GDALDataType rasterType, int typeSize) {
	fprintf(stdout, "signature %d\n", no_records);
	char* sign = (char*) signature;
	for ( int i = 0; i < numBands; i++ ) {
		
		// FIXME:  rasterType has to be used to extract values accordingly
		double dFieldValue = (double) *( (double*) (sign + i*typeSize) );
		
		
		int ok = DBFWriteDoubleAttribute(
			file,
			no_records,                   // int iShape -- record number
			first_band_field_index + i,   // int iField,
			dFieldValue 
		);
		
		// false: I'm skipping this test for now
		// FIXME: recheck DBFWriteDoubleAttribute documentation/source code
		if ( false && !ok ) {
			char field_name[64];
			sprintf(field_name, "Band%d", i);
			fprintf(stderr, "DBFWriteDoubleAttribute returned %d\nfield name %s, record %d\n", 
				ok, field_name, no_records+1
			);
			exit(1);
		}
	}
	no_records++;
}



///////////////////////////////////////////////////////////////////////////
int starspan_db(
	Raster* rast, 
	Vector* vect, 
	const char* db_filename,
	bool only_in_feature
) {
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
		


