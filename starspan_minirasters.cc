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
#include <assert.h>

int starspan_minirasters(
	Raster& rast, 
	Vector& vect, 
	const char* prefix,
	const char* pszOutputSRS    // see -a_srs option for gdal_translate
	                         // If NULL, projection is taken from input dataset
) {
	//fprintf(stderr, "((mini raster is being implemented right now))\n");
	
	OGRLayer* layer = vect.getLayer(0);
	if ( !layer ) {
		fprintf(stderr, "Couldn't get layer 0 from %s\n", vect.getName());
		return 1;
	}
	
	// get raster size and coordinates
	int width, height, bands;
	rast.getSize(&width, &height, &bands);
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

	
	double min_x = MIN(x0, x1);
	double min_y = MIN(y0, y1);
	
	
    OGRPoint* point = new OGRPoint();

    OGRFeature* feature;
    int feature_num = 0;
	while( (feature = layer->GetNextFeature()) != NULL ) {
		feature_num++;
		OGRGeometry* geom = feature->GetGeometryRef();
		OGREnvelope feature_env;
		geom->getEnvelope(&feature_env);
		OGREnvelope intersection_env;

		// intersect with raster
		OGRGeometry* intersection = geom->Intersection(raster_ring);
		if ( intersection ) {
			intersection->getEnvelope(&intersection_env);
			starspan_print_envelope(stdout, "intersection_env:", intersection_env);
			
			assert(intersection_env.MinX >= min_x);
			assert(intersection_env.MinY >= min_y);
			
			// create mini raster
			char mini_filename[1024];
			sprintf(mini_filename, "%s%03d.img", prefix, feature_num);
			int mini_x0 = (int) ((intersection_env.MinX - min_x) / pix_size_x);
			int mini_y0 = (int) ((intersection_env.MinY - min_y) / pix_size_y);
			int mini_width =  (int) ((intersection_env.MaxX+pix_size_x - intersection_env.MinX) / pix_size_x); 
			int mini_height = (int) ((intersection_env.MaxY+pix_size_y - intersection_env.MinY) / pix_size_y);

			GDALDatasetH hOutDS = starspan_subset_raster(
				rast.getDataset(),
				mini_x0, mini_y0, mini_width, mini_height,
				mini_filename,
				pszOutputSRS
			);
			
			GDALClose(hOutDS);
			fprintf(stdout, "\n");

			if ( true )
				continue;


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
			
			delete intersection;
		}

		delete feature;
	}
	delete point;
	delete raster_ring;
	
	fprintf(stdout, "finished.\n");

	return 0;
}
		


