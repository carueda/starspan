//
// STARSpan project
// Traverse
// Carlos A. Rueda
// $Id$
// See traverser.h for public documentation
//

#include "traverser.h"           

#include <stdlib.h>
#include <assert.h>


extern bool starspan_intersect_envelopes(OGREnvelope& oEnv1, OGREnvelope& oEnv2, OGREnvelope& envr);

// (null-object pattern)
static Observer null_observer;

Traverser::Traverser(Raster* raster, Vector* vector) {
	rast = raster;
	vect = vector;
	observer = &null_observer;
}

Traverser::~Traverser() {
}


void Traverser::traverse() {
	OGRLayer* layer = vect->getLayer(0);
	if ( !layer ) {
		fprintf(stderr, "Couldn't get layer 0 from %s\n", vect->getName());
		exit(1);
	}
	
	// get raster size and coordinates
	int width, height, bands;
	rast->getSize(&width, &height, &bands);
	double x0, y0, x1, y1;
	rast->getCoordinates(&x0, &y0, &x1, &y1);

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

    int feature_id = 0;
    OGRFeature* feature;
	while( (feature = layer->GetNextFeature()) != NULL ) {
		feature_id++;
		OGRGeometry* feature_geometry = feature->GetGeometryRef();
		OGREnvelope feature_env;
		feature_geometry->getEnvelope(&feature_env);
		OGREnvelope intersection_env;

		// intersect with raster
		
		//OGRGeometry* intersection = feature_geometry->Intersection(raster_ring);
		//if ( intersection ) {
		//	intersection->getEnvelope(&intersection_env);

		if ( starspan_intersect_envelopes(raster_env, feature_env, intersection_env) ) {


			assert(intersection_env.MinX >= min_x);
			assert(intersection_env.MinY >= min_y);
			
			observer->intersection(feature_id, intersection_env);



			// region
			// FIXME: check this calculation
			int mini_x0 = (int) ((intersection_env.MinX - min_x) / pix_size_x);
			int mini_y0 = (int) ((intersection_env.MinY - min_y) / pix_size_y);
			int mini_width =  (int) ((intersection_env.MaxX+pix_size_x - intersection_env.MinX) / pix_size_x); 
			int mini_height = (int) ((intersection_env.MaxY+pix_size_y - intersection_env.MinY) / pix_size_y);

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
			
			GDALDataset* dataset = rast->getDataset();
			
			// some info from the first band (assumed to be valid for all bands)
			GDALRasterBand* band1 = dataset->GetRasterBand(1);
			GDALDataType rasterType = band1->GetRasterDataType(); 
			int rasterTypeSize = GDALGetDataTypeSize(rasterType) >> 3;
			//fprintf(stdout, "rasterTypeSize=%d\n", rasterTypeSize);
			
			int num_points_in_envelope = 0;
			int num_points_in_feature = 0;
			int nYOff = 0;
			
			for (double y = grid_y0; y <= intersection_env.MaxY+pix_size_y; y += pix_size_y) {
				int nXOff = 0;
				for (double x = grid_x0; x <= intersection_env.MaxX+pix_size_x; x += pix_size_x) {
					num_points_in_envelope++;
					point->setX(x);
					point->setY(y);
					if ( feature_geometry->Contains(point) ) {
						num_points_in_feature++;
						
						// read in signature for (x,y):
						double sig_buffer[bands];   // large enough
						char* signature = (char*) sig_buffer;
						for(int i = 0; i < bands; i++ ) {
							GDALRasterBand* band = dataset->GetRasterBand(i+1);
							band->RasterIO(
								GF_Read,
								nXOff + mini_x0, nYOff + mini_y0,
								1, 1,          // nXSize, nYSize
								signature + i*rasterTypeSize, // pData
								1, 1,          // nBufXSize, nBufYSize
								rasterType,    // eBufType
								0, 0           // nPixelSpace, nLineSpace
							);
						}
						
						observer->addSignature(x, y, signature, rasterTypeSize);
					}
				}
			}
			fprintf(stdout, " %d points in feature out of %d points in envelope\n", 
				num_points_in_feature, num_points_in_envelope
			);

			//delete intersection;
		}
		
		
		delete feature;
	}
			
	delete point;
	delete raster_ring;
}




