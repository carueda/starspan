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


Traverser::Traverser() {
	vect = 0;
	pixelProportion = -1.0;   // disabled
	desired_FID = -1;
	
	// buffer: note that we won't allocate minimumBandBufferSize
	// bytes, but assume the biggest data type, double.  Se below.
	bandValues_buffer = 0;
	minimumBandBufferSize = 0;
	
	// assume observers will be all simple:
	notSimpleObserver = false;
	
	lineRasterizer = 0;
	pixset = 0;
}

void Traverser::addObserver(Observer* aObserver) { 
	observers.push_back(aObserver);
	//if at least one observer is not simple...
	if ( !aObserver->isSimple() )
		notSimpleObserver = true;
}


void Traverser::releaseObservers(void) { 
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		delete *obs;
	
	observers.empty();
}


void Traverser::setPixelProportion(double pixprop) {
	pixelProportion = pixprop; 
}

void Traverser::setDesiredFID(long FID) {
	desired_FID = FID; 
}



void Traverser::setVector(Vector* vector) {
	if ( vect )
		fprintf(stderr, "traverser: Warning: resetting vector\n");
	vect = vector;
}


//
//
void Traverser::addRaster(Raster* raster) {
	rasts.push_back(raster);
	
	GDALDataset* dataset = raster->getDataset();
	
	// add band info from given raster
	for ( int i = 0; i < dataset->GetRasterCount(); i++ ) {
		GDALRasterBand* band = dataset->GetRasterBand(i+1);
		globalInfo.bands.push_back(band);
		
		// update minimumBandBufferSize:
		GDALDataType bandType = band->GetRasterDataType();
		int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
		minimumBandBufferSize += bandTypeSize;
	}
	
	if ( rasts.size() == 1 ) {
		// info taken from first raster.
		
		rasts[0]->getSize(&width, &height, NULL);
		rasts[0]->getCoordinates(&x0, &y0, &x1, &y1);
		rasts[0]->getPixelSize(&pix_x_size, &pix_y_size);

		// create a geometry for raster envelope:
		OGRLinearRing raster_ring;
		raster_ring.addPoint(x0, y0);
		raster_ring.addPoint(x1, y0);
		raster_ring.addPoint(x1, y1);
		raster_ring.addPoint(x0, y1);
		globalInfo.rasterPoly.addRing(&raster_ring);
		globalInfo.rasterPoly.closeRings();
		globalInfo.rasterPoly.getEnvelope(&raster_env);
		
		lineRasterizer = new LineRasterizer(x0, y0, pix_x_size, pix_y_size);
		lineRasterizer->setObserver(this);
	}
	
	if ( rasts.size() > 1 ) {
		// check compatibility against first raster:
		// (rather strict for now)
		
		int last = rasts.size() -1;
		int _width, _height;
		double _x0, _y0, _x1, _y1;
		double _pix_x_size, _pix_y_size;
		rasts[last]->getSize(&_width, &_height, NULL);
		rasts[last]->getCoordinates(&_x0, &_y0, &_x1, &_y1);
		rasts[last]->getPixelSize(&_pix_x_size, &_pix_y_size);

		if ( _width != width || _height != height ) {
			fprintf(stderr, "Different number of lines/cols\n");
			exit(1);
		}
		if ( _x0 != x0 || _y0 != y0 || _x1 != x1 || _y1 != y1) {
			fprintf(stderr, "Different geographic location\n");
			exit(1);
		}
		if ( _pix_x_size != pix_x_size || _pix_y_size != pix_y_size ) {
			fprintf(stderr, "Different pixel size\n");
			exit(1);
		}
	}
}

//
// destroys this traverser
//
Traverser::~Traverser() {
	if ( bandValues_buffer )
		delete[] bandValues_buffer;
	if ( lineRasterizer )
		delete lineRasterizer;
}

void* Traverser::getBandValuesForPixel(int col, int row, void* buffer) {
	char* ptr = (char*) buffer;
	for ( unsigned i = 0; i < globalInfo.bands.size(); i++ ) {
		GDALRasterBand* band = globalInfo.bands[i];
		GDALDataType bandType = band->GetRasterDataType();
	
		int status = band->RasterIO(
			GF_Read,
			col, row,
			1, 1,             // nXSize, nYSize
			ptr,              // pData
			1, 1,             // nBufXSize, nBufYSize
			bandType,         // eBufType
			0, 0              // nPixelSpace, nLineSpace
		);
		
		if ( status != CE_None ) {
			fprintf(stdout, "Error reading band value, status= %d\n", status);
			exit(1);
		}
		
		int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
		ptr += bandTypeSize;
	}
	
	return buffer;
}


void Traverser::getBandValuesForPixel(int col, int row) {
	assert(bandValues_buffer);
	getBandValuesForPixel(col, row, bandValues_buffer);
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
// see processPoint, processMultiPoint, processPolygon.
// If pixset is not null, it checks for duplicate pixel.
//
void Traverser::pixelFound(double x, double y) {
	// get pixel location:
	int col, row;
	toColRow(x, y, &col, &row);
	
	if ( col < 0 || col >= width 
	||   row < 0 || row >= height ) {
		return;
	}
	
	if ( pixset ) {
		// check this location has not been processed
		EPixel colrow(col, row);
		if ( pixset->find(colrow) != pixset->end() ) {
			return;
		}
		pixset->insert(colrow);
	}
	
	toGridXY(col, row, &x, &y);
	
	TraversalEvent event;
	event.pixel.col = col;
	event.pixel.row = row;
	event.pixel.x = x;
	event.pixel.y = y;
	
	// if at leat one observer is not simple...
	if ( notSimpleObserver ) {
		// get also band values
		getBandValuesForPixel(col, row);
		event.bandValues = bandValues_buffer;
	}
	
	// notify observers:
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->addPixel(event);
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
		//
		// Note that connecting pixels between lines are not repeated.
		//
		for ( int i = 1; i < num_points; i++ ) {
			OGRPoint point;
			linstr->getPoint(i, &point);

			//
			// Traverse line from point0 to point:
			//
			bool last = i == num_points - 1;
			lineRasterizer->line(point0.getX(), point0.getY(), point.getX(), point.getY(), last);
			
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
// processes a given feature
//
void Traverser::process_feature(OGRFeature* feature) {
	//
	// get geometry
	//
	OGRGeometry* feature_geometry = feature->GetGeometryRef();

	//
	// intersect this feature with raster (raster ring)
	//
	OGRGeometry* intersection_geometry = feature_geometry->Intersection(&globalInfo.rasterPoly);
	if ( !intersection_geometry ) {
		return;
	}

	fprintf(stdout, 
		"\n\nFID: %ld  INTERSECTION %s\n",
		feature->GetFID(),
		intersection_geometry->getGeometryName()
	);			
	
	//
	// notify observers about this feature
	// 
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->intersectionFound(feature);

	assert(!pixset);
	
	//
	// get intersection type and process accordingly
	// Note that pixset is created where duplicate pixel control is required.
	//
	OGRwkbGeometryType intersection_type = intersection_geometry->getGeometryType();
	switch ( intersection_type ) {
		case wkbPoint:
		case wkbPoint25D:
			fputc('.', stdout); fflush(stdout);
			processPoint((OGRPoint*) intersection_geometry);
			break;
	
		case wkbMultiPoint:
		case wkbMultiPoint25D:
			fputc(':', stdout); fflush(stdout);
			pixset = new set<EPixel>();
			processMultiPoint((OGRMultiPoint*) intersection_geometry);
			break;
	
		case wkbLineString:
		case wkbLineString25D:
			fputc('|', stdout); fflush(stdout);
			pixset = new set<EPixel>();
			processLineString((OGRLineString*) intersection_geometry);
			break;
	
		case wkbMultiLineString:
		case wkbMultiLineString25D:
			fputc('!', stdout); fflush(stdout);
			pixset = new set<EPixel>();
			processMultiLineString((OGRMultiLineString*) intersection_geometry);
			break;
			
		case wkbPolygon:
		case wkbPolygon25D:
			fputc('@', stdout); fflush(stdout);
			processPolygon((OGRPolygon*) intersection_geometry);
			break;
			
		default:
			fprintf(stdout, "%s: intersection type not considered\n",
				OGRGeometryTypeToName(intersection_type)
			);
	}

	if ( pixset ) {
		delete pixset;
		pixset = 0;
	}
	delete intersection_geometry;
}


//
// main method for traversal
//
void Traverser::traverse() {
	// don't allow a second call, for now
	if ( bandValues_buffer ) {
		fprintf(stderr, "traverser.traverse: second call!\n");
		return;
	}

	// do some checks:
	if ( observers.size() == 0 ) {
		fprintf(stderr, "traverser: No observers registered!\n");
		return;
	}

	if ( !vect ) {
		fprintf(stderr, "traverser: Vector datasource not specified!\n");
		return;
	}
	if ( rasts.size() == 0 ) {
		fprintf(stderr, "traverser: No raster datasets were specified!\n");
		return;
	}
	//
	// Only first layer (0) is processed (which assumes only one layer exists)
	//
	if ( vect->getLayerCount() > 1 ) {
		fprintf(stderr, 
			"Vector datasource with more than one layer: %s\n"
			"Only one layer expected.\n",
			vect->getName()
		);
		return;
	}
	OGRLayer* layer = vect->getLayer(0);
	if ( !layer ) {
		fprintf(stderr, "Couldn't get layer from %s\n", vect->getName());
		return;
	}

	
	// assuming biggest data type we assign enough memory:
	bandValues_buffer = new double[globalInfo.bands.size()];
		
	//
	// notify observers about initialization of process
	//
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->init(globalInfo);

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
	
	//
	// notify observers about finalization of process
	//
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->end();
}


