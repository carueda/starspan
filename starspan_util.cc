//
// STARSpan project
// Carlos A. Rueda
// starspan_util - misc utilities
// $Id$
//

#include "starspan.h"           
#include "jts.h"       
#include <geos/io.h>

#include <stdlib.h>

// aux routine for reporting 
void starspan_report(Raster* rast, Vector* vect) {
	if ( rast ) {
		fprintf(stdout, "----------------RASTER--------------\n");
		rast->report(stdout);
	}
	if ( vect ) {
		fprintf(stdout, "----------------VECTOR--------------\n");
		vect->report(stdout);
	}
}


// my intersect function, adapted from OGRGeometry::Intersect
bool starspan_intersect_envelopes(OGREnvelope& oEnv1, OGREnvelope& oEnv2, OGREnvelope& envr) {
    if( oEnv1.MaxX < oEnv2.MinX
        || oEnv1.MaxY < oEnv2.MinY
        || oEnv2.MaxX < oEnv1.MinX
        || oEnv2.MaxY < oEnv1.MinY )
        return false;
		
	envr.MinX = MAX(oEnv1.MinX, oEnv2.MinX);
	envr.MaxX = MIN(oEnv1.MaxX, oEnv2.MaxX);
	envr.MinY = MAX(oEnv1.MinY, oEnv2.MinY);
	envr.MaxY = MIN(oEnv1.MaxY, oEnv2.MaxY);
		
	return true;
}


void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env) {
	fprintf(file, "%s %.3f %.3f %.3f %.3f\n", msg, env.MinX, env.MinY, env.MaxX, env.MaxY);
}


void starspan_myErrorHandler(CPLErr eErrClass, int err_no, const char *msg) {
	fprintf(stderr, "myError: %s\n", msg);
	fflush(stderr);
	abort();
}


