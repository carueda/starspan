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


// (null-object pattern)
static Observer null_observer;

Traverser::Traverser(Raster* raster, Vector* vector) {
	rast = raster;
	vect = vector;
	observer = &null_observer;

	dataset = rast->getDataset();
	// some info from the first band (assumed to be valid for all bands)
	band1 = dataset->GetRasterBand(1);
	rasterType = band1->GetRasterDataType(); 
	rasterTypeSize = GDALGetDataTypeSize(rasterType) >> 3;
	//fprintf(stdout, "rasterTypeSize=%d\n", rasterTypeSize);

	// get raster size and coordinates
	rast->getSize(&width, &height, &bands);
	rast->getCoordinates(&x0, &y0, &x1, &y1);
	rast->getPixelSize(&pix_x_size, &pix_y_size);

	// create a geometry for raster envelope:
	OGRLinearRing raster_ring;
	raster_ring.addPoint(x0, y0);
	raster_ring.addPoint(x1, y0);
	raster_ring.addPoint(x1, y1);
	raster_ring.addPoint(x0, y1);
	raster_poly.addRing(&raster_ring);
	raster_poly.closeRings();
	raster_poly.getEnvelope(&raster_env);
	
	signature_buffer = new double[bands];   // large enough
	
	lineRasterizer = new LineRasterizer(pix_x_size, pix_y_size);
	lineRasterizer->setObserver(this);
}


Traverser::~Traverser() {
	delete[] signature_buffer;
	delete lineRasterizer;
}




// read in signature for (x,y):
void Traverser::getSignature(int col, int row) {
	char* signature = (char*) signature_buffer;
	for(int i = 0; i < bands; i++ ) {
		GDALRasterBand* band = dataset->GetRasterBand(i+1);
		band->RasterIO(
			GF_Read,
			col, row,
			1, 1,                         // nXSize, nYSize
			signature + i*rasterTypeSize, // pData
			1, 1,                         // nBufXSize, nBufYSize
			rasterType,                   // eBufType
			0, 0                          // nPixelSpace, nLineSpace
		);
	}
}

// (x,y) to (col,row) conversion
void Traverser::toColRow(double x, double y, int *col, int *row) {
	*col = (int) floor( (x - x0) / pix_x_size );
	*row = (int) floor( (y - y0) / pix_y_size );
}

// as a LineRasterizerObserver, but also called directly	
void Traverser::pixelFound(double x, double y) {
	if ( observer->isSimple() )
		observer->addPixel(x, y);
	else {
		int col, row;
		toColRow(x, y, &col, &row);
		getSignature(col, row);
		observer->addSignature(x, y, signature_buffer, rasterType, rasterTypeSize);
	}
}

// process a line string
void Traverser::lineString(OGRLineString* linstr) {
	int num_points = linstr->getNumPoints();
	//fprintf(stdout, "      num_points = %d\n", num_points);
	if ( num_points > 0 ) {
		OGRPoint point0;
		linstr->getPoint(0, &point0);
		for ( int i = 1; i < num_points; i++ ) {
			OGRPoint point;
			linstr->getPoint(i, &point);

			//traverse line from point0 to point:
			lineRasterizer->line(point0.getX(), point0.getY(), point.getX(), point.getY());
			
			point0 = point;
		}
	}
}

// main method
void Traverser::traverse() {
	//
	// Only first layer (0) is processed (which assumes only one later exists)
	//
	if ( vect->getLayerCount() > 1 ) {
		fprintf(stderr, 
			"Vector datasource with more than one layer: %s\n"
			"Only one layer expected.  Exiting",
			vect->getName()
		);
		exit(1);
	}
	OGRLayer* layer = vect->getLayer(0);
	if ( !layer ) {
		fprintf(stderr, "Couldn't get layer from %s\n", vect->getName());
		exit(1);
	}

	//
	// notify observer about raster ring
	//
	observer->rasterPoly(&raster_poly);
	
	//
	// For each feature in vector datasource:
	//
    OGRFeature* feature;
	while( (feature = layer->GetNextFeature()) != NULL ) {
		//
		// get geometry
		//
		OGRGeometry* feature_geometry = feature->GetGeometryRef();

		//
		// intersect this feature with raster (raster ring)
		//
		OGRGeometry* intersection_geometry = feature_geometry->Intersection(&raster_poly);
		if ( !intersection_geometry ) {
			delete feature;
			continue;
		}

		fprintf(stdout, "\n\nINTERSECTION %s\n", intersection_geometry->getGeometryName());			
		
		//
		// notify observer about this feature
		// 
		observer->intersectionFound(feature);

		//
		// get intersection type and process accordingly
		//
		OGRwkbGeometryType intersection_type = intersection_geometry->getGeometryType();
		
		
		////////////////////////////////////////////
		// Point
		if ( intersection_type == wkbPoint ) {
			fputc('.', stdout); fflush(stdout);
			OGRPoint* point = (OGRPoint*) intersection_geometry;
			pixelFound(point->getX(), point->getY());
		}
		
		////////////////////////////////////////////
		// MultiPoint
		if ( intersection_type == wkbMultiPoint ) {
			fputc(':', stdout); fflush(stdout);
			OGRMultiPoint* pp = (OGRMultiPoint*) intersection_geometry;
			for ( int i = 0; i < pp->getNumGeometries(); i++ ) {
				OGRPoint* point = (OGRPoint*) pp->getGeometryRef(i);
				pixelFound(point->getX(), point->getY());
			}
		}
		
		////////////////////////////////////////////
		// Line String
		else if ( intersection_type == wkbLineString ) {
			fputc('|', stdout); fflush(stdout);
			OGRLineString* linstr = (OGRLineString*) intersection_geometry;
			lineString(linstr);
		}
		
		////////////////////////////////////////////
		// MultiLineString
		if ( intersection_type == wkbMultiLineString ) {
			fputc('!', stdout); fflush(stdout);
			OGRMultiLineString* coll = (OGRMultiLineString*) intersection_geometry;
			for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
				OGRLineString* linstr = (OGRLineString*) coll->getGeometryRef(i);
				lineString(linstr);
			}
		}
		
		////////////////////////////////////////////
		// Polygon
		else if ( intersection_type == wkbPolygon ) {
			fputc('@', stdout); fflush(stdout);
			OGRPolygon* poly = (OGRPolygon*) intersection_geometry;
			OGREnvelope intersection_env;
			poly->getEnvelope(&intersection_env);
			
			extern void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env);
			starspan_print_envelope(stdout, "poly envolppe: ", intersection_env);
			
			int minCol, minRow, maxCol, maxRow;
			toColRow(intersection_env.MinX, intersection_env.MinY, &minCol, &minRow);
			toColRow(intersection_env.MaxX, intersection_env.MaxY, &maxCol, &maxRow);
			// Note: minCol is not necessarily <= maxCol (idem for *Row)

			fprintf(stdout, " minCol=%d, minRow=%d\n", minCol, minRow); 
			fprintf(stdout, " maxCol=%d, maxRow=%d\n", maxCol, maxRow); 
			
			int rows_env = abs(maxRow - minRow +1);
			int cols_env = abs(maxCol - minCol +1);
			int num_points_in_poly = 0;
			
			fprintf(stdout, " %d cols\n", cols_env);
			fprintf(stdout, " %d rows: ", rows_env); fflush(stdout);
			double abs_pix_y_size = fabs(pix_y_size);
			double abs_pix_x_size = fabs(pix_x_size);
			double y = intersection_env.MinY;
			for ( int i = 0; i < rows_env; i++, y += abs_pix_y_size) {
				double x = intersection_env.MinX;
				for (int j = 0; j < cols_env; j++, x += abs_pix_x_size) {
					OGRPoint point(x, y);
					if ( poly->Contains(&point) ) {
						num_points_in_poly++;
						pixelFound(x, y);
					}
				}
				fprintf(stdout, "%d ", (i+1)); fflush(stdout);
			}
			fprintf(stdout, "\n"); 
			fprintf(stdout, " %d points in poly out of %d points in envelope\n", 
				num_points_in_poly, rows_env * cols_env
			);
		}
		
		delete intersection_geometry;
		delete feature;
	}
}




