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
	Traverser& tr,
	const char* prefix,
	const char* pszOutputSRS  // see gdal_translate option -a_srs 
	                         // If NULL, projection is taken from input dataset
) {
	if ( tr.getNumRasters() > 1 ) {
		fprintf(stderr, "Only one input raster is expected in this version\n");
		return 1;
	}
	Raster& rast = *tr.getRaster(0);
	Vector& vect = *tr.getVector();

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
			
			assert(intersection_env.MinX >= min_x);
			assert(intersection_env.MinY >= min_y);
			
			// create mini raster
			char mini_filename[1024];
			sprintf(mini_filename, "%s%03d.img", prefix, feature_num);
			
			fprintf(stdout, "\n%s:\n", mini_filename);
			starspan_print_envelope(stdout, "intersection_env:", intersection_env);
			
			// region
			int mini_x0 = (int) ((intersection_env.MinX - min_x) / pix_size_x);
			int mini_y0 = (int) ((intersection_env.MinY - min_y) / pix_size_y);
			int mini_width =  (int) ((intersection_env.MaxX+pix_size_x - intersection_env.MinX) / pix_size_x); 
			int mini_height = (int) ((intersection_env.MaxY+pix_size_y - intersection_env.MinY) / pix_size_y);

			cout<< " mini_x0=" <<mini_x0<< ", mini_y0=" <<mini_y0<< endl 
				<< " mini_width=" <<mini_width<< ", mini_height=" <<mini_height<< endl
			;

			GDALDatasetH hOutDS = starspan_subset_raster(
				rast.getDataset(),
				mini_x0, mini_y0, mini_width, mini_height,
				mini_filename,
				pszOutputSRS
			);
			
			
			if ( globalOptions.only_in_feature ) {
				fprintf(stdout, "nullifying pixels not contained in feature..\n");
				
				// check locations in intersection_env.
				// These locations have to be on the grid defined by the
				// raster coordinates and resolution. Basically we need
				// to determine the starting location (grid_x0,grid_y0)
				// and let the point (x,y) take values on the grid in the
				// intersection_env.
				double grid_x0 = intersection_env.MinX;
				double grid_y0 = intersection_env.MinY;
				
				// FIXME  adjust (grid_x0,grid_y0) as just described !!!
				// ...
				
				int num_points = 0;
				int nYOff = 0;
				for (double y = grid_y0; nYOff < mini_height; y += pix_size_y, nYOff++) {
					int nXOff = 0;
					for (double x = grid_x0; nXOff < mini_width; x += pix_size_x, nXOff++) {
						point->setX(x);
						point->setY(y);
						if ( geom->Contains(point) ) {
							num_points++;
						}
						else {
							// nullify bands in hOutDS for (x,y):
							int nBandCount = GDALGetRasterCount(hOutDS);
							for(int i = 0; i < nBandCount; i++ ) {
								GDALRasterBand* band = ((GDALDataset *) hOutDS)->GetRasterBand(i+1);
								double dfNoData = 0.0;
								
								band->RasterIO(
									GF_Write,
									nXOff, nYOff,
									1, 1,          // nXSize, nYSize
									&dfNoData,     // pData
									1, 1,          // nBufXSize, nBufYSize
									GDT_Float64,   // eBufType
									0, 0           // nPixelSpace, nLineSpace
								);
								
							}
						}
					}
				}
				fprintf(stdout, " %d points retained\n", num_points);
				
				
			}
			
			GDALClose(hOutDS);
			fprintf(stdout, "\n");

			delete intersection;
		}

		delete feature;
	}
	delete point;
	delete raster_ring;
	
	fprintf(stdout, "finished.\n");

	return 0;
}
		


