//
// starspan declarations
// Carlos A. Rueda
// $Id$
//

#ifndef starspan_h
#define starspan_h

#include "Raster.h"           
#include "Vector.h"       

#include <stdio.h> // FILE


// Generate mini rasters
int starspan_minirasters(Raster& rast, Vector& vect);

// Generate a JTS test
void starspan_jtstest(Raster& rast, Vector& vect, FILE* jtstest_file);


//////////////////////////
// misc utilities:

// aux routine for reporting 
void starspan_report(Raster* rast, Vector* vect);

// intersect two envelopes
bool starspan_intersect_envelopes(OGREnvelope& oEnv1, OGREnvelope& oEnv2, OGREnvelope& envr);
void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env);
void starspan_myErrorHandler(CPLErr eErrClass, int err_no, const char *msg);


#endif

