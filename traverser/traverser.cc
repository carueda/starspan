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

// for processPolygon_pixel:
#include <geos.h>


static double pixelProportion = -1.0;   // disabled
static long desired_FID = -1;


void Traverser::setPixelProportion(double pixprop) {
	pixelProportion = pixprop; 
}

void Traverser::setDesiredFID(long FID) {
	desired_FID = FID; 
}


// (null-object pattern)
static Observer null_observer;

//
// Raster type info is taken from first band (assumed valid for all bands).
//
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

//
//
//
Traverser::~Traverser() {
	delete[] signature_buffer;
	delete lineRasterizer;
}

//
// read in signature in (col,row):
//
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

//
// (x,y) to (col,row) conversion
//
inline void Traverser::toColRow(double x, double y, int *col, int *row) {
	*col = (int) floor( (x - x0) / pix_x_size );
	*row = (int) floor( (y - y0) / pix_y_size );
}

//
// (col,row) to (x,y) conversion
//
inline void Traverser::toGridXY(int col, int row, double *x, double *y) {
	*x = x0 + col * pix_x_size;
	*y = y0 + row * pix_y_size;
}

//
// implementation as a LineRasterizerObserver, but also called directly, 
// see processPoint, processMultiPoint, processPolygon
//
void Traverser::pixelFound(double x, double y) {
	// get pixel location:
	int col, row;
	toColRow(x, y, &col, &row);
	TraversalEvent event;
	event.pixelLocation.col = col;
	event.pixelLocation.row = row;
	event.pixelLocation.x = x;
	event.pixelLocation.y = y;
	
	if ( !observer->isSimple() ) {
		// get also signature and raster info:
		getSignature(col, row);
		event.signature = signature_buffer;
		event.rasterType = rasterType;
		event.typeSize = rasterTypeSize;
	}
	
	// notify observer:
	observer->addPixel(event);
}

//
// process a point intersection
//
void Traverser::processPoint(OGRPoint* point) {
	pixelFound(point->getX(), point->getY());
}

//
// process a multi point intersection
//
void Traverser::processMultiPoint(OGRMultiPoint* pp) {
	for ( int i = 0; i < pp->getNumGeometries(); i++ ) {
		OGRPoint* point = (OGRPoint*) pp->getGeometryRef(i);
		pixelFound(point->getX(), point->getY());
	}
}


//
// process a line string intersection
//
void Traverser::processLineString(OGRLineString* linstr) {
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
			//
			// FIXME: We don't want to repeat pixels...
			//
			
			point0 = point;
		}
	}
}

//
// process a multi line string intersection
//
void Traverser::processMultiLineString(OGRMultiLineString* coll) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRLineString* linstr = (OGRLineString*) coll->getGeometryRef(i);
		processLineString(linstr);
	}
}


//
// process a polygon intersection
// If a pixel proportion has been specified via setPixelProportion(), then
// the area of intersections are used to determine if a pixel is to be
// included. Otherwise, the pixel is included only if the upper left corner
// of the pixel is within the polygon. 
//
void Traverser::processPolygon(OGRPolygon* poly) {
	if ( pixelProportion < 0.0 )
		processPolygon_point(poly);
	else
		processPolygon_pixel(poly);
}


//
// processPolygon_point:
// process a polygon intersection: checking point inclusion
//
void Traverser::processPolygon_point(OGRPolygon* poly) {
	OGREnvelope intersection_env;
	poly->getEnvelope(&intersection_env);
	
	// extern void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env);
	// starspan_print_envelope(stdout, "poly envolppe: ", intersection_env);
	
	// get envelope corners in pixel coordinates:
	int minCol, minRow, maxCol, maxRow;
	toColRow(intersection_env.MinX, intersection_env.MinY, &minCol, &minRow);
	toColRow(intersection_env.MaxX, intersection_env.MaxY, &maxCol, &maxRow);
	// Note: minCol is not necessarily <= maxCol (idem for *Row)

	fprintf(stdout, " minCol=%d, minRow=%d\n", minCol, minRow); 
	fprintf(stdout, " maxCol=%d, maxRow=%d\n", maxCol, maxRow); 
	
	// get envelope corners in grid coordinates:
	double minX, minY, maxX, maxY;
	toGridXY(minCol, minRow, &minX, &minY);
	toGridXY(maxCol, maxRow, &maxX, &maxY);

	fprintf(stdout, " minX=%g, minY=%g\n", minX, minY); 
	fprintf(stdout, " maxX=%g, maxY=%g\n", maxX, maxY); 
	
	// envelope dimensions:
	int rows_env = abs(maxRow - minRow) +1;
	int cols_env = abs(maxCol - minCol) +1;
	int num_points_in_poly = 0;
	
	fprintf(stdout, " %d cols x %d rows\n", cols_env, rows_env);
	fprintf(stdout, " processing rows: %4d", 0); fflush(stdout);
	
	double abs_pix_y_size = fabs(pix_y_size);
	double abs_pix_x_size = fabs(pix_x_size);
	double y = minY;
	for ( int i = 0; i < rows_env; i++, y += abs_pix_y_size) {
		double x = minX;
		for (int j = 0; j < cols_env; j++, x += abs_pix_x_size) {
			OGRPoint point(x, y);
			if ( poly->Contains(&point) ) {
				num_points_in_poly++;
				pixelFound(x, y);
			}
		}
		fprintf(stdout, "\b\b\b\b%4d", (i+1)); fflush(stdout);
	}
	fprintf(stdout, "\n"); 
	fprintf(stdout, " %d points in poly out of %d points in envelope\n", 
		num_points_in_poly, rows_env * cols_env
	);
}

//
// processPolygon_pixel:
// process a polygon intersection: checking area of pixel intersection
//
// To avoid some OGR overhead, use GEOS directly.
//
static geos::GeometryFactory* global_factory = new geos::GeometryFactory();

// create pixel polygon
inline static geos::Polygon* create_pix_poly(double x0, double y0, double x1, double y1) {
	geos::CoordinateSequence *cl = new geos::DefaultCoordinateSequence();
	cl->add(geos::Coordinate(x0, y0));
	cl->add(geos::Coordinate(x1, y0));
	cl->add(geos::Coordinate(x1, y1));
	cl->add(geos::Coordinate(x0, y1));
	cl->add(geos::Coordinate(x0, y0));
	geos::LinearRing* pixLR = global_factory->createLinearRing(cl);
	vector<geos::Geometry *>* holes = NULL;
	geos::Polygon *poly = global_factory->createPolygon(pixLR, holes);
	return poly;
}

void Traverser::processPolygon_pixel(OGRPolygon* poly) {
	OGREnvelope intersection_env;
	poly->getEnvelope(&intersection_env);
	
	// get envelope corners in pixel coordinates:
	int minCol, minRow, maxCol, maxRow;
	toColRow(intersection_env.MinX, intersection_env.MinY, &minCol, &minRow);
	toColRow(intersection_env.MaxX, intersection_env.MaxY, &maxCol, &maxRow);
	// Note: minCol is not necessarily <= maxCol (idem for *Row)

	fprintf(stdout, " minCol=%d, minRow=%d\n", minCol, minRow); 
	fprintf(stdout, " maxCol=%d, maxRow=%d\n", maxCol, maxRow); 
	
	// get envelope corners in grid coordinates:
	double minX, minY, maxX, maxY;
	toGridXY(minCol, minRow, &minX, &minY);
	toGridXY(maxCol, maxRow, &maxX, &maxY);

	fprintf(stdout, " minX=%g, minY=%g\n", minX, minY); 
	fprintf(stdout, " maxX=%g, maxY=%g\n", maxX, maxY); 
	
	// envelope dimensions:
	int rows_env = abs(maxRow - minRow) +1;
	int cols_env = abs(maxCol - minCol) +1;
	
	// number of pixels found in polygon:
	int num_pixels_in_poly = 0;
	
	fprintf(stdout, " %d cols x %d rows\n", cols_env, rows_env);
	fprintf(stdout, " processing rows: %4d", 0); fflush(stdout);

	geos::Polygon* geos_poly = (geos::Polygon*) poly->exportToGEOS();
	const double pix_area = fabs(pix_x_size*pix_y_size);
	
	//geos::WKTWriter wktWriter;
	
	double abs_pix_y_size = fabs(pix_y_size);
	double abs_pix_x_size = fabs(pix_x_size);
	double y = minY;
	for ( int i = 0; i < rows_env; i++, y += abs_pix_y_size) {
		double x = minX;
		for (int j = 0; j < cols_env; j++, x += abs_pix_x_size) {
			// create pixel polygon for (x,y) grid location:
			geos::Polygon* pix_poly = create_pix_poly(x, y, x + pix_x_size, y + pix_y_size);
			// intersect
			geos::Geometry* pix_inters = geos_poly->intersection(pix_poly);
			if ( pix_inters ) {
				double area = pix_inters->getArea();
				//cout << wktWriter.write(pix_inters) << " area=" << area << endl;
				//cout " area=" << area << endl;
				if ( area >= pixelProportion * pix_area ) { 
					num_pixels_in_poly++;
					pixelFound(x, y);
				}
				delete pix_inters;
			}
			delete pix_poly;
		}
		fprintf(stdout, "\b\b\b\b%4d", (i+1)); fflush(stdout);
	}
	fprintf(stdout, "\n"); 
	fprintf(stdout, " %d pixels in poly out of %d pixels in envelope\n", 
		num_pixels_in_poly, rows_env * cols_env
	);
}




//
// main method for traversal
//
void Traverser::process_feature(OGRFeature* feature) {
	//
	// get geometry
	//
	OGRGeometry* feature_geometry = feature->GetGeometryRef();

	//
	// intersect this feature with raster (raster ring)
	//
	OGRGeometry* intersection_geometry = feature_geometry->Intersection(&raster_poly);
	if ( !intersection_geometry ) {
		return;
	}

	fprintf(stdout, 
		"\n\nFID: %ld  INTERSECTION %s\n",
		feature->GetFID(),
		intersection_geometry->getGeometryName()
	);			
	
	//
	// notify observer about this feature
	// 
	observer->intersectionFound(feature);

	//
	// get intersection type and process accordingly
	//
	OGRwkbGeometryType intersection_type = intersection_geometry->getGeometryType();
	switch ( intersection_type ) {
		case wkbPoint:
			fputc('.', stdout); fflush(stdout);
			processPoint((OGRPoint*) intersection_geometry);
			break;
	
		case wkbMultiPoint:
			fputc(':', stdout); fflush(stdout);
			processMultiPoint((OGRMultiPoint*) intersection_geometry);
			break;
	
		case wkbLineString:
			fputc('|', stdout); fflush(stdout);
			processLineString((OGRLineString*) intersection_geometry);
			break;
	
		case wkbMultiLineString:
			fputc('!', stdout); fflush(stdout);
			processMultiLineString((OGRMultiLineString*) intersection_geometry);
			break;
			
		case wkbPolygon:
			fputc('@', stdout); fflush(stdout);
			processPolygon((OGRPolygon*) intersection_geometry);
			break;
			
		default:
			fprintf(stdout, "?: intersection type not considered\n");
	}
	
	delete intersection_geometry;
}


//
// main method for traversal
//
void Traverser::traverse() {
	//
	// Only first layer (0) is processed (which assumes only one layer exists)
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

    OGRFeature* feature;
	
	//
	// Was a specific FID given?
	//
	if ( desired_FID >= 0 ) {
		feature = layer->GetFeature(desired_FID);
		if ( !feature ) {
			fprintf(stderr, "FID %ld not found in %s\n", desired_FID, vect->getName());
			exit(1);
		}
		process_feature(feature);
		delete feature;
	}
	else {
		//
		// For each feature in vector datasource:
		//
		while( (feature = layer->GetNextFeature()) != NULL ) {
			process_feature(feature);
			delete feature;
		}
	}
}


