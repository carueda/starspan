//
// STARSpan project
// Carlos A. Rueda
// starspan_jtstest - generate JTS test
// $Id$
//

#include "starspan.h"           
#include "jts.h"       
#include <geos/io.h>

#include <stdlib.h>


void starspan_jtstest(Raster& rast, Vector& vect, FILE* jtstest_file) {
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
		
		if ( starspan_intersect_envelopes(raster_env, feature_env, intersection_env) ) {
			jtstest_count++;
			jts_test_case_init(jtstest_file);
			
			jts_test_case_arg_init(jtstest_file, "a");
			geom->dumpReadable(jtstest_file, "    ");
			jts_test_case_arg_end(jtstest_file, "a");
			
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
				fprintf(jtstest_file, "    MULTIPOINT(");
				int num_points = 0;
				for (double y = grid_y0; y <= intersection_env.MaxY+pix_size_y; y += pix_size_y) {
					for (double x = grid_x0; x <= intersection_env.MaxX+pix_size_x; x += pix_size_x) {
						point->setX(x);
						point->setY(y);
						if ( geom->Contains(point) ) {
							if ( num_points > 0 )
								fprintf(jtstest_file, ", ");
							fprintf(jtstest_file, "%.3f  %.3f", x, y);
							num_points++;
						}
					}
				}
				if ( num_points > 0 )
					fprintf(jtstest_file, ")\n");
				else
					fprintf(jtstest_file, "EMPTY)\n");
				fprintf(stdout, "case %d: %d points\n", jtstest_count, num_points);
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
	
	jts_test_end(jtstest_file);
	fprintf(stdout, "%d test cases generated\n", jtstest_count);
}


