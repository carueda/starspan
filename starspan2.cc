//
// STARSpan project
// Carlos A. Rueda
// starspan2 - second version
//         uses Vector.h, Raster.h
// $Id$
//

#include "Raster.h"           
#include "Vector.h"       
#include "jts.h"       

#include <stdio.h>
#include <stdlib.h>

#include <geos/io.h>

#define VERSION "0.2"


static char rcsid[] = "$Id$";

// aux routine for reporting 
static void report(Raster& rast, Vector& vect) {
	fprintf(stdout, "----------------RASTER--------------\n");
	rast.report(stdout);
	fprintf(stdout, "----------------VECTOR--------------\n");
	vect.report(stdout);
}


// my intersect function, adapted from OGRGeometry::Intersect
static bool intersect_envelopes(OGREnvelope& oEnv1, OGREnvelope& oEnv2, OGREnvelope& envr) {
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

/*
static void print_envelope(FILE* file, const char* msg, OGREnvelope& env) {
	fprintf(file, "%s %g %g %g %g\n", msg, env.MinX, env.MinY, env.MaxX, env.MaxY);
}
*/


// central process
static void process(Raster& rast, Vector& vect, FILE* jtstest_file) {
	// get raster size and coordinates
	int width, height;
	rast.getSize(&width, &height);
	double x0, y0, x1, y1;
	rast.getCoordinates(&x0, &y0, &x1, &y1);

	double pix_size_x = (x1 - x0) / width;
	double pix_size_y = (y1 - y0) / height;
	
	if ( false ) {
		fprintf(stderr, "(width,height) = (%d,%d)\n", width, height);
		fprintf(stderr, "(x0,y0) = (%7.1f,%7.1f)\n", x0, y0);
		fprintf(stderr, "(x1,y1) = (%7.1f,%7.1f)\n", x1, y1);
		fprintf(stderr, "pix_size = (%7.1f,%7.1f)\n", pix_size_x, pix_size_y);
	}

	// create a geometry for raster envelope:
	OGRLinearRing* raster_ring = new OGRLinearRing();
	raster_ring->addPoint(x0, y0);
	raster_ring->addPoint(x1, y0);
	raster_ring->addPoint(x1, y1);
	raster_ring->addPoint(x0, y1);
	raster_ring->closeRings();
	OGREnvelope raster_env;
	raster_ring->getEnvelope(&raster_env);
	
	if ( false ) {
		fprintf(stderr, "raster_ring:\n");
		raster_ring->dumpReadable(stderr);
		//print_envelope(stderr, "raster_env:", raster_env);
	}
	
	OGRLayer* layer = vect.getLayer(0);
	if ( !layer )
		return;
	
	// take absolute values since intersection_env has increasing
	// coordinates:
	if ( pix_size_x < 0 )
		pix_size_x = -pix_size_x;
	if ( pix_size_y < 0 )
		pix_size_y = -pix_size_y;
	
	
	jts_test_init(jtstest_file);
	int jtstest_count = 0;
	
    OGRPoint* point = new OGRPoint();

    OGRFeature* feature;
	while( (feature = layer->GetNextFeature()) != NULL ) {
		OGRGeometry* geom = feature->GetGeometryRef();
		OGREnvelope feature_env;
		geom->getEnvelope(&feature_env);
		OGREnvelope intersection_env;

		/* hmm, Intersection() aborts !  why?
		OGRGeometry* intersection = geom->Intersection(raster_ring);
		if ( intersection ) {
			intersection->getEnvelope(&intersection_env);
			print_envelope(stdout, "intersection_env:", intersection_env);
		}
		*/
		
		if ( intersect_envelopes(raster_env, feature_env, intersection_env) ) {
			if ( jtstest_file ) {
				jtstest_count++;
				jts_test_case_init(jtstest_file);
				
				jts_test_case_arg_init(jtstest_file, "a");
				geom->dumpReadable(jtstest_file, "    ");
				jts_test_case_arg_end(jtstest_file, "a");
			}
			
			// check locations in intersection_env.
			// These locations have to be on the grid defined by the
			// raster coordinates and resolution. Basically we need
			// to determine the starting location (grid_x0,grid_y0)
			// and let the point (x,y) take values on the grid in the
			// intersection_env.
			double grid_x0 = intersection_env.MinX;
			double grid_y0 = intersection_env.MinY;
			
			// FIXME  adjust (grid_x0,grid_y0) as described !!!
			// ...
			// I'm now testing the Contains() method first ...
			
			try {
				jts_test_case_arg_init(jtstest_file, "b");

				if ( jtstest_file ) {
					fprintf(jtstest_file, "    MULTIPOINT(");
				}
				int num_points = 0;
				for (double y = grid_y0; y <= intersection_env.MaxY+pix_size_y; y += pix_size_y) {
					for (double x = grid_x0; x <= intersection_env.MaxX+pix_size_x; x += pix_size_x) {
						point->setX(x);
						point->setY(y);
						if ( geom->Contains(point) ) {
							// stdout: very simple for now:
							//fprintf(stdout, "%7.1f  %7.1f\n", x, y);

							// jtstest_file: 
							if ( jtstest_file ) {
								if ( num_points > 0 )
									fprintf(jtstest_file, ", ");
								fprintf(jtstest_file, "%.3f  %.3f", x, y);
							}
							num_points++;
						}
					}
				}
				if ( jtstest_file ) {
					if ( num_points > 0 )
						fprintf(jtstest_file, ")\n");
					else
						fprintf(jtstest_file, "EMPTY)\n");
					fprintf(stdout, "case %d: %d points\n", jtstest_count, num_points);
				}
				jts_test_case_arg_end(jtstest_file, "b");
				jts_test_case_end(jtstest_file);
			}
			catch(geos::ParseException* ex) {
				fprintf(stderr, "geos::ParseException: %s\n", ex->toString().c_str());
			}
		}

		delete feature;
	}
	delete point;
	delete raster_ring;
	
	if ( jtstest_file ) {
		jts_test_end(jtstest_file);
		fprintf(stdout, "%d test cases generated\n", jtstest_count);
	}
}



static void myErrorHandler(CPLErr eErrClass, int err_no, const char *msg) {
	fprintf(stderr, "myError: %s\n", msg);
	fflush(stderr);
	abort();
}


// prints a help message
static void usage(const char* msg) {
	if ( msg ) { 
		fprintf(stderr, "starspan: %s\n", msg); 
		fprintf(stderr, "Type `starspan -help' for help\n");
		exit(1);
	}
	fprintf(stdout, 
		"\n"
		"starspan %s (%s %s)\n"
		"(under development)\n"
		"\n"
		"USAGE:\n"
		"\n"
		"  starspan -help\n"
		"      prints this message and exits\n"
		"\n"
		"  starspan -xhelp\n"
		"      prints help for extended usage\n"
		"\n"
		"\n"
		"  starspan args...\n"
		"\n"
		"   Inputs:\n"
		"      -vector <filename>   An OGR recognized vector file\n"
		"      -raster <filename>   A GDAL recognized raster file\n"
		"\n"
		"   Options:\n"
		"      -jtstest <filename>  generates a JTS test file\n"
		"\n"
		"\n"
		, VERSION, __DATE__, __TIME__
	);
	exit(0);
}

///////////////////////////////////////////////////////////////
// main test program
int main(int argc, char ** argv) {
	if ( argc == 1 || 0==strcmp("-help", argv[1]) ) {
		usage(NULL);
	}
	
	const char* raster_filename = NULL;
	const char* vector_filename = NULL;
	bool do_report = false;
	const char* jtstest_filename = NULL;
	
	for ( int i = 1; i < argc; i++ ) {
		if ( 0==strcmp("-vector", argv[i]) ) {
			if ( ++i == argc )
				usage("missing vector file");
			vector_filename = argv[i];
			
		}
		else if ( 0==strcmp("-raster", argv[i]) ) {
			if ( ++i == argc )
				usage("missing raster file");
			raster_filename = argv[i];
		}
		else if ( 0==strcmp("-help", argv[i]) ) {
			usage(NULL);
		}
		else if ( 0==strcmp("-jtstest", argv[i]) ) {
			if ( ++i == argc )
				usage("missing JTS test filename");
			jtstest_filename = argv[i];
		}
		else if ( 0==strcmp("-report", argv[i]) ) {
			do_report = true;
		}
		else {
			usage("invalid arguments");
		}
	}
	
	if ( !vector_filename )
		usage("missing -vector argument");
	if ( !raster_filename )
		usage("missing -raster argument");
	
	// module initialization
	Raster::init();
	Vector::init();
	
	// create basic objects:
	Raster rast(raster_filename);
	Vector vect(vector_filename);

	CPLPushErrorHandler(myErrorHandler);
	
	
	if ( do_report ) {
		report(rast, vect);
	}
	else {
		FILE* jtstest_file = NULL;
		if ( jtstest_filename ) {
			jtstest_file = fopen(jtstest_filename, "w");
			if ( !jtstest_file ) {
				fprintf(stderr, "Could not create %s\n", jtstest_filename);
				return 1;
			}
		}
		process(rast, vect, jtstest_file);
		if ( jtstest_file )
			fclose(jtstest_file);
	}
	
	return 0;
}

