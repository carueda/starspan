//
// STARSpan project
// Carlos A. Rueda
// starspan_minirasters - generate mini rasters
// $Id$
//

#include "starspan.h"           
#include "jts.h"       
#include <geos/io.h>

#include <stdlib.h>

int starspan_minirasters(Raster& rast, Vector& vect) {
	//fprintf(stderr, "((mini raster is being implemented right now))\n");
	
	OGRLayer* layer = vect.getLayer(0);
	if ( !layer ) {
		fprintf(stderr, "Couldn't get layer 0 from %s\n", vect.getName());
		return 1;
	}
	
	// get raster size and coordinates
	int width, height;
	rast.getSize(&width, &height);
	double x0, y0, x1, y1;
	rast.getCoordinates(&x0, &y0, &x1, &y1);

	double pix_size_x = (x1 - x0) / width;
	double pix_size_y = (y1 - y0) / height;

	// take absolute values since intersection_env has increasing
	// coordinates:
	if ( pix_size_x < 0 )
		pix_size_x = -pix_size_x;
	if ( pix_size_y < 0 )
		pix_size_y = -pix_size_y;

	
	// create a geometry for raster envelope:
	OGRLinearRing* raster_ring = new OGRLinearRing();
	raster_ring->addPoint(x0, y0);
	raster_ring->addPoint(x1, y0);
	raster_ring->addPoint(x1, y1);
	raster_ring->addPoint(x0, y1);
	raster_ring->closeRings();
	OGREnvelope raster_env;
	raster_ring->getEnvelope(&raster_env);

	
	
    OGRPoint* point = new OGRPoint();

    OGRFeature* feature;
	while( (feature = layer->GetNextFeature()) != NULL ) {
		OGRGeometry* geom = feature->GetGeometryRef();
		OGREnvelope feature_env;
		geom->getEnvelope(&feature_env);
		OGREnvelope intersection_env;

		/* hmm, Intersection() aborts !  why? */
		OGRGeometry* intersection = geom->Intersection(raster_ring);
		if ( intersection ) {
			intersection->getEnvelope(&intersection_env);
			starspan_print_envelope(stdout, "intersection_env:", intersection_env);
		}
		/**/
		
		if ( starspan_intersect_envelopes(raster_env, feature_env, intersection_env) ) {
			
			
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
			
			fprintf(stdout, "    MULTIPOINT(");
			int num_points = 0;
			for (double y = grid_y0; y <= intersection_env.MaxY+pix_size_y; y += pix_size_y) {
				for (double x = grid_x0; x <= intersection_env.MaxX+pix_size_x; x += pix_size_x) {
					point->setX(x);
					point->setY(y);
					if ( geom->Contains(point) ) {
						if ( num_points > 0 )
							fprintf(stdout, ", ");
						fprintf(stdout, "%.3f  %.3f", x, y);
						num_points++;
					}
				}
			}
			if ( num_points > 0 )
				fprintf(stdout, ")\n");
			else
				fprintf(stdout, "EMPTY)\n");
			fprintf(stdout, "case with %d points\n", num_points);
		}

		delete feature;
	}
	delete point;
	delete raster_ring;
	
	fprintf(stdout, "finished.\n");

	return 0;
}
		


