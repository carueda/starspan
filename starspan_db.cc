//
// STARSpan project
// Carlos A. Rueda
// starspan_db - generate db ... very incomplete!
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <assert.h>


class DBObserver : public Observer {
public:
	FILE* file;
	DBObserver(FILE* f) { file = f; }

	void intersection(int feature_id, OGREnvelope intersection_env) {
		starspan_print_envelope(file, "intersection_env:", intersection_env);
	}
	
	void addSignature(double x, double y, void* signature, int typeSize) {
		// pending
	}
};

int starspan_db(
	Raster* rast, 
	Vector* vect, 
	const char* db_name,
	bool only_in_feature,
	const char* pszOutputSRS  // see gdal_translate option -a_srs 
	                         // If NULL, projection is taken from input dataset
) {
	// create output file
	char db_filename[1024];
	sprintf(db_filename, "%s", db_name);
	FILE* file = fopen(db_filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", db_filename);
		return 1;
	}

	DBObserver obs(file);	
	Traverser tr(rast, vect);
	tr.setObserver(&obs);
	tr.traverse();
	
	fclose(file);
	fprintf(stdout, "finished.\n");

	return 0;
}
		


