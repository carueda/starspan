//
// STARSpan project
// Traverse
// Carlos A. Rueda
// $Id$
// See traverser.h for public documentation
//

#include "traverser.h"           

#include <cstdlib>
#include <cassert>
#include <cstring>

// for polygon processing:
#include "geos/opPolygonize.h"


Traverser::Traverser() {
	vect = 0;
	pixelProportion = 0.5;
	desired_FID = -1;
	desired_fieldName = "";
	desired_fieldValue = "";
	
	// buffer: note that we won't allocate minimumBandBufferSize
	// bytes, but assume the biggest data type, double.  See below.
	bandValues_buffer = 0;
	minimumBandBufferSize = 0;
	
	// assume observers will be all simple:
	notSimpleObserver = false;
	
	lineRasterizer = 0;
	progress_out = 0;
	verbose = false;
	logstream = 0;
	debug_dump_polys = getenv("STARSPAN_DUMP_POLYS_ON_EXCEPTION") != 0;
	skip_invalid_polys = false;
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
	notSimpleObserver = false;
}


void Traverser::setPixelProportion(double pixprop) {
	pixelProportion = pixprop; 
}

void Traverser::setDesiredFID(long FID) {
	desired_FID = FID; 
}


void Traverser::setDesiredFeatureByField(const char* field_name, const char* field_value) {
	desired_fieldName = field_name;
	desired_fieldValue = field_value;
}


void Traverser::setVector(Vector* vector) {
	if ( vect )
		cerr<< "traverser: Warning: resetting vector\n";
	vect = vector;
}


inline static OGRPolygon* create_rectangle(double x0, double y0, double x1, double y1) { 
	OGRLinearRing ring;
	ring.addPoint(x0, y0);
	ring.addPoint(x1, y0);
	ring.addPoint(x1, y1);
	ring.addPoint(x0, y1);
	OGRPolygon* poly = new OGRPolygon();
	poly->addRing(&ring);
	poly->closeRings();
	return poly;
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
		double pix_x_size, pix_y_size;
		rasts[0]->getPixelSize(&pix_x_size, &pix_y_size);
		grid.setPixelSize(pix_x_size, pix_y_size);
		
		double x0, y0, x1, y1;
		rasts[0]->getCoordinates(&x0, &y0, &x1, &y1);
		OGRPolygon* poly = create_rectangle(x0, y0, x1, y1);
		globalInfo.rastersUnion = poly;
		globalInfo.rasterPolys.addGeometry(poly);
	}
	else {   //  assert( rasts.size() > 1 )
		const int last = rasts.size() -1;

		// update rastersUnion by unioning the new raster.

		// we keep the restriction that pixel size must be equal
		double pix_x_size, pix_y_size;
		rasts[last]->getPixelSize(&pix_x_size, &pix_y_size);
		if ( grid.abs_pix_x_size != fabs(pix_x_size) 
		||   grid.abs_pix_y_size != fabs(pix_y_size) ) {
			cerr<< "Different pixel size\n";
			exit(1);
		}
		
		// update: rastersUnion += new rectangle
		double x0, y0, x1, y1;
		rasts[last]->getCoordinates(&x0, &y0, &x1, &y1);
		OGRPolygon* poly = create_rectangle(x0, y0, x1, y1);
		globalInfo.rasterPolys.addGeometry(poly);
		OGRGeometry* new_rastersUnion = 0;
		try {
			new_rastersUnion = globalInfo.rastersUnion->Union(poly);
		} 
		catch(geos::GEOSException* ex) {
			cerr<< "geos::GEOSException: " << ex->toString()<< endl;
			exit(1);
		}
		delete globalInfo.rastersUnion;
		globalInfo.rastersUnion = new_rastersUnion;
	}
	
	// update grid info
	OGREnvelope env;
	globalInfo.rastersUnion->getEnvelope(&env);
	grid.setOrigin(env);
}

//
// destroys this traverser
//
Traverser::~Traverser() {
	if ( globalInfo.rastersUnion )
		delete globalInfo.rastersUnion;
	if ( bandValues_buffer )
		delete[] bandValues_buffer;
	if ( lineRasterizer )
		delete lineRasterizer;
}

void* Traverser::getBandValuesForPixel(double x, double y, void* buffer) {
	char* ptr = (char*) buffer;
	for ( unsigned i = 0; i < globalInfo.bands.size(); i++ ) {
		GDALRasterBand* band = globalInfo.bands[i];
		GDALDataType bandType = band->GetRasterDataType();
		int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
		// initialize with NODATA value  (here fixed to zero)  PENDING
		memset(&ptr, 0, bandTypeSize);
		
		GDALDataset* dataset = band->GetDataset();
		int col, row;
		if ( Raster::toColRow(dataset, x, y, &col, &row) ) {
			int width = band->GetXSize();
			int height = band->GetYSize();
			if ( col < 0 || col >= width || row < 0 || row >= height ) {
				// leave NODATA value.
			}
			else {
				// get value from band:
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
					cerr<< "Error reading band value, status= " <<status<< "\n";
					exit(1);
				}
			}
		}
		
		ptr += bandTypeSize;
	}
	
	return buffer;
}


void Traverser::getBandValuesForPixel(double x, double y) {
	assert(bandValues_buffer);
	getBandValuesForPixel(x, y, bandValues_buffer);
}


int Traverser::getPixelIntegerValuesInBand(
	unsigned band_index, 
	vector<CRPixel>* colrows,
	vector<int>& list
) {
	if ( band_index <= 0 || band_index > globalInfo.bands.size() ) {
		cerr<< "Traverser::getPixelDoubleValuesInBand: band_index " <<band_index<< " out of range\n";
		return 1;
	}
	
	GDALRasterBand* band = globalInfo.bands[band_index-1];
	const int width = band->GetXSize();
	const int height = band->GetYSize();
	for ( vector<CRPixel>::const_iterator colrow = colrows->begin(); colrow != colrows->end(); colrow++ ) {
		int col = colrow->col;
		int row = colrow->row;
		int value = 0;
		if ( col < 0 || col >= width || row < 0 || row >= height ) {
			// nothing:  keep the 0 value
		}
		else {
			int status = band->RasterIO(
				GF_Read,
				col, row,
				1, 1,             // nXSize, nYSize
				&value,           // pData
				1, 1,             // nBufXSize, nBufYSize
				GDT_Int32,        // eBufType
				0, 0              // nPixelSpace, nLineSpace
			);
			
			if ( status != CE_None ) {
				cerr<< "Error reading band value, status=" <<status<< "\n";
				exit(1);
			}
		}
		list.push_back(value);
	}
	return 0;
}

int Traverser::getPixelDoubleValuesInBand(
	unsigned band_index, 
	vector<CRPixel>* colrows,
	vector<double>& list
) {
	if ( band_index <= 0 || band_index > globalInfo.bands.size() ) {
		cerr<< "Traverser::getPixelDoubleValuesInBand: band_index " <<band_index<< " out of range\n";
		return 1;
	}
	
	GDALRasterBand* band = globalInfo.bands[band_index-1];
	const int width = band->GetXSize();
	const int height = band->GetYSize();
	for ( vector<CRPixel>::const_iterator colrow = colrows->begin(); colrow != colrows->end(); colrow++ ) {
		int col = colrow->col;
		int row = colrow->row;
		double value = 0.0;
		if ( col < 0 || col >= width || row < 0 || row >= height ) {
			// nothing:  keep the 0.0 value
		}
		else {
			int status = band->RasterIO(
				GF_Read,
				col, row,
				1, 1,             // nXSize, nYSize
				&value,           // pData
				1, 1,             // nBufXSize, nBufYSize
				GDT_Float64,      // eBufType
				0, 0              // nPixelSpace, nLineSpace
			);
			
			if ( status != CE_None ) {
				cerr<< "Error reading band value, status=" <<status<< "\n";
				exit(1);
			}
		}
		list.push_back(value);
	}
	return 0;
}

//
// Implementation as a LineRasterizerObserver, but also called directly. 
// Checks for duplicate pixel.
//
void Traverser::pixelFound(double x, double y) {
	// get pixel location in the global grid:
	int col, row;
	grid.toColRow(x, y, &col, &row);
	
	assert ( col >= 0 );
	assert ( row >= 0 );
	
	// check this location has not been processed
	EPixel colrow(col, row);
	if ( pixset.find(colrow) != pixset.end() ) {
		return;
	}
	pixset.insert(colrow);
	
	grid.toGridXY(col, row, &x, &y);

	TraversalEvent event(col, row, x, y);
	summary.num_processed_pixels++;
	
	// if at least one observer is not simple...
	if ( notSimpleObserver ) {
		// get also all values for this pixel from the bands.
		// Use point at center of the pixel:
		getBandValuesForPixel(x + grid.abs_pix_x_size/2, y + grid.abs_pix_y_size/2);
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
	//cout<< "      num_points = " <<num_points<< endl;
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
// To avoid some OGR overhead, I use GEOS directly.
//
static geos::GeometryFactory* global_factory = new geos::GeometryFactory();
static const geos::CoordinateSequenceFactory* global_cs_factory = global_factory->getCoordinateSequenceFactory();

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

//
// Process a valid polygon.
//
inline void Traverser::processValidPolygon(geos::Polygon* geos_poly) {
	const geos::Envelope* intersection_env = geos_poly->getEnvelopeInternal();
	
	double actual_min_x, actual_max_x;
	if ( intersection_env->getMinX() < intersection_env->getMaxX() ) {
		actual_min_x = intersection_env->getMinX();
		actual_max_x = intersection_env->getMaxX();
	}
	else {
		actual_min_x = intersection_env->getMaxX();
		actual_max_x = intersection_env->getMinX();
	}
	double actual_min_y, actual_max_y;
	if ( intersection_env->getMinY() < intersection_env->getMaxY() ) {
		actual_min_y = intersection_env->getMinY();
		actual_max_y = intersection_env->getMaxY();
	}
	else {
		actual_min_y = intersection_env->getMaxY();
		actual_max_y = intersection_env->getMinY();
	}
	
	
	// get envelope corners in pixel coordinates:
	int minCol, minRow, maxCol, maxRow;
	grid.toColRow(actual_min_x, actual_min_y, &minCol, &minRow);
	grid.toColRow(actual_max_x, actual_max_y, &maxCol, &maxRow);

	// get envelope corners in grid coordinates:
	double minX, minY, maxX, maxY;
	grid.toGridXY(minCol, minRow, &minX, &minY);
	grid.toGridXY(maxCol, maxRow, &maxX, &maxY);

	assert( minRow <= maxRow );
	assert( minCol <= maxCol );
	
	if ( logstream ) {
		(*logstream)
		   << " minCol=" <<minCol<< ", minRow=" <<minRow<< endl 
		   << " maxCol=" <<maxCol<< ", maxRow=" <<maxRow<< endl
		   << " minX=" <<minX<< ", minY=" <<minY<< endl 
		   << " maxX=" <<maxX<< ", maxY=" <<maxY<< endl;
	}
	
	// envelope dimensions:
	int rows_env = maxRow - minRow +1;
	int cols_env = maxCol - minCol +1;
	
	// number of pixels found in polygon:
	int num_pixels_in_poly = 0;
	
	if ( verbose ) {
		fprintf(stdout, " %d cols x %d rows\n", cols_env, rows_env);
		fprintf(stdout, " processing rows: %4d", 0); fflush(stdout);
	}

	double y = minY;
	for ( int i = 0; i < rows_env; i++, y += grid.abs_pix_y_size) {
		double x = minX;
		for (int j = 0; j < cols_env; j++, x += grid.abs_pix_x_size) {
			// create pixel polygon for (x,y) grid location:
			geos::Polygon* pix_poly = create_pix_poly(x, y, x + grid.abs_pix_x_size, y + grid.abs_pix_y_size);
			// intersect
			geos::Geometry* pix_inters = 0;
			try {
				pix_inters = geos_poly->intersection(pix_poly);
			}
			catch(geos::TopologyException* ex) {
				cerr<< "TopologyException: " << ex->toString()<< endl;
				if ( debug_dump_polys ) {
					cerr<< "pix_poly = " << wktWriter.write(pix_poly) << endl;
					cerr<< "geos_poly = " << wktWriter.write(geos_poly) << endl;
				}
			}
			catch(geos::GEOSException* ex) {
				cerr<< "geos::GEOSException: " << ex->toString()<< endl;
				if ( debug_dump_polys ) {
					geos::WKTWriter wktWriter;
					cerr<< "pix_poly = " << wktWriter.write(pix_poly) << endl;
					cerr<< "geos_poly = " << wktWriter.write(geos_poly) << endl;
				}
			}
			if ( pix_inters ) {
				double area = pix_inters->getArea();
				//cout << wktWriter.write(pix_inters) << " area=" << area << endl;
				//cout " area=" << area << endl;
				if ( area >= pixelProportion * grid.pix_area ) { 
					num_pixels_in_poly++;
					
					// we now check for pixel duplication always
					pixelFound(x, y);
				}
				delete pix_inters;
			}
			delete pix_poly;
		}
		if ( verbose ) {
			fprintf(stdout, "\b\b\b\b%4d", (i+1)); fflush(stdout);
		}
	}
	
	if ( verbose ) {
		fprintf(stdout, "\n"); 
		fprintf(stdout, " %d pixels in poly out of %d pixels in envelope\n", 
			num_pixels_in_poly, rows_env * cols_env
		);
	}
}

class MyPolygonizer : public geos::Polygonizer {
public:
	MyPolygonizer() : geos::Polygonizer() {}
	
/***
	~MyPolygonizer() {
		for ( unsigned i = 0; i < holeList->size(); i++ ) {
			delete (*holeList)[i];
		}
		
		for ( unsigned i = 0; i < shellList->size(); i++ ) {
			delete (*shellList)[i];
		}
	}
***/	
};

//
// process a polygon intersection.
// The area of intersections are used to determine if a pixel is to be
// included.  
//
void Traverser::processPolygon(OGRPolygon* poly) {
	geos::Polygon* geos_poly = (geos::Polygon*) poly->exportToGEOS();
	if ( geos_poly->isValid() ) {
		processValidPolygon(geos_poly);
	}
	else {
		summary.num_invalid_polys++;
		
		if ( skip_invalid_polys ) {
			//cerr<< "--skipping invalid polygon--"<< endl;
			if ( debug_dump_polys ) {
				cerr<< "geos_poly = " << wktWriter.write(geos_poly) << endl;
			}
		}
		else {
			// try to explode this poly into smaller ones:
			if ( geos_poly->getNumInteriorRing() > 0 ) {
				//cerr<< "--Invalid polygon has interior rings: cannot explode it--" << endl;
				summary.num_polys_with_internal_ring++;
			} 
			else {
				const geos::LineString* lines = geos_poly->getExteriorRing();
				// get noded linestring:
				geos::Geometry* noded = 0;
				const int num_points = lines->getNumPoints();
				const geos::CoordinateSequence* coordinates = lines->getCoordinatesRO();
				for ( int i = 1; i < num_points; i++ ) {
					vector<geos::Coordinate>* subcoordinates = new vector<geos::Coordinate>();
					subcoordinates->push_back(coordinates->getAt(i-1));
					subcoordinates->push_back(coordinates->getAt(i));
					geos::CoordinateSequence* cs = global_cs_factory->create(subcoordinates);
					geos::LineString* ln = global_factory->createLineString(cs);
					
					if ( !noded )
						noded = ln;
					else {
						noded = noded->Union(ln);
						delete ln;
					}
				}
				
				if ( noded ) {
					// now, polygonize:
					MyPolygonizer polygonizer;
					polygonizer.add(noded);
					
					// and process generated sub-polygons:
					vector<geos::Polygon*>* polys = polygonizer.getPolygons();
					if ( polys ) {
						summary.num_polys_exploded++;
						summary.num_sub_polys += polys->size();
						if ( verbose )
							cout << polys->size() << " sub-polys obtained\n";
						for ( unsigned i = 0; i < polys->size(); i++ ) {
							processValidPolygon((*polys)[i]);
						}
					}
					else {
						cerr << "could not explode polygon\n";
					}
					
					delete noded;
				}
			}
		}
	}
	delete geos_poly;
}


//
// process a multi-polygon intersection.
//
void Traverser::processMultiPolygon(OGRMultiPolygon* mpoly) {
	for ( int i = 0; i < mpoly->getNumGeometries(); i++ ) {
		OGRPolygon* poly = (OGRPolygon*) mpoly->getGeometryRef(i);
		processPolygon(poly);
	}
}

//
// process a geometry collection intersection.
//
void Traverser::processGeometryCollection(OGRGeometryCollection* coll) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRGeometry* geo = (OGRGeometry*) coll->getGeometryRef(i);
		processGeometry(geo, false);
	}
}

//
// get intersection type and process accordingly
//
void Traverser::processGeometry(OGRGeometry* intersection_geometry, bool count) {
	OGRwkbGeometryType intersection_type = intersection_geometry->getGeometryType();
	switch ( intersection_type ) {
		case wkbPoint:
		case wkbPoint25D:
			if ( count ) 
				summary.num_point_features++;
			processPoint((OGRPoint*) intersection_geometry);
			break;
	
		case wkbMultiPoint:
		case wkbMultiPoint25D:
			if ( count ) 
				summary.num_multipoint_features++;
			processMultiPoint((OGRMultiPoint*) intersection_geometry);
			break;
	
		case wkbLineString:
		case wkbLineString25D:
			if ( count ) 
				summary.num_linestring_features++;
			processLineString((OGRLineString*) intersection_geometry);
			break;
	
		case wkbMultiLineString:
		case wkbMultiLineString25D:
			if ( count ) 
				summary.num_multilinestring_features++;
			processMultiLineString((OGRMultiLineString*) intersection_geometry);
			break;
			
		case wkbPolygon:
		case wkbPolygon25D:
			if ( count ) 
				summary.num_polygon_features++;
			processPolygon((OGRPolygon*) intersection_geometry);
			break;
			
		case wkbMultiPolygon:
		case wkbMultiPolygon25D:
			if ( count ) 
				summary.num_multipolygon_features++;
			processMultiPolygon((OGRMultiPolygon*) intersection_geometry);
			break;
			
		case wkbGeometryCollection:
		case wkbGeometryCollection25D:
			if ( count ) 
				summary.num_geometrycollection_features++;
			processGeometryCollection((OGRGeometryCollection*) intersection_geometry);
			break;
			
		default:
			throw (string(OGRGeometryTypeToName(intersection_type))
			    + ": intersection type not considered."
			);
	}
}


//
// processes a given feature
//
void Traverser::process_feature(OGRFeature* feature) {
	if ( verbose ) {
		fprintf(stdout, "\n\nFID: %ld", feature->GetFID());
	}
	
	//
	// get geometry
	//
	OGRGeometry* feature_geometry = feature->GetGeometryRef();

	//
	// intersect this feature with raster (raster ring)
	//
	OGRGeometry* intersection_geometry = 0;
	
	try {
		intersection_geometry = feature_geometry->Intersection(globalInfo.rastersUnion);
	}
	catch(geos::GEOSException* ex) {
		cerr<< ">>>>> FID: " << feature->GetFID()
		    << "  GEOSException: " << ex->toString()<< endl;
		return;
	}

	if ( !intersection_geometry ) {
		if ( verbose ) {
			fprintf(stdout, " NO intersection:\n");
		}
		return;
	}
	summary.num_intersecting_features++;

	if ( verbose ) {
		fprintf(stdout, " Type of intersection: %s\n",
			intersection_geometry->getGeometryName()
		);		
	}
	
	//
	// notify observers about this feature
	// 
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->intersectionFound(feature);

	pixset.clear();
	try {
		processGeometry(intersection_geometry, true);
	}
	catch(string err) {
		cerr<< "starspan: FID=" <<feature->GetFID()
		    << ", " << OGRGeometryTypeToName(feature_geometry->getGeometryType())
		    << endl << err << endl;
	}

	delete intersection_geometry;
}


//
// main method for traversal
//
void Traverser::traverse() {
	// don't allow a second call, for now
	if ( bandValues_buffer ) {
		cerr<< "traverser.traverse: second call!\n";
		return;
	}

	// do some checks:
	if ( observers.size() == 0 ) {
		cerr<< "traverser: No observers registered!\n";
		return;
	}

	if ( !vect ) {
		cerr<< "traverser: Vector datasource not specified!\n";
		return;
	}
	if ( rasts.size() == 0 ) {
		cerr<< "traverser: No raster datasets were specified!\n";
		return;
	}
	//
	// Only first layer (0) is processed (which assumes only one layer exists)
	//
	if ( vect->getLayerCount() > 1 ) {
		cerr<< "Vector datasource with more than one layer: "
		    << vect->getName()
			<< "\nOnly one layer expected.\n"
		;
		return;
	}
	OGRLayer* layer = vect->getLayer(0);
	if ( !layer ) {
		cerr<< "Couldn't get layer from " << vect->getName()<< endl;
		return;
	}
	layer->ResetReading();

	
	memset(&summary, 0, sizeof(summary));
	
	// assuming biggest data type we assign enough memory:
	bandValues_buffer = new double[globalInfo.bands.size()];

	// prepare line rasterizer in case we need it:       
	lineRasterizer = new LineRasterizer(
		grid.x0, 
		grid.y0, 
		grid.abs_pix_x_size, 
		grid.abs_pix_y_size
	);
	lineRasterizer->setObserver(this);

	if ( getenv("STARSPAN_DUMP_RASTER_UNION") ) {
		fprintf(stdout, "RASTER_UNION = ");
		globalInfo.rasterPolys.dumpReadable(stdout);
		fflush(stdout);
	}
		
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
			cerr<< "FID " <<desired_FID<< " not found in " <<vect->getName()<< endl;
			exit(1);
		}
		process_feature(feature);
		delete feature;
	}
	//
	// Was a specific field name/value given?
	//
	else if ( desired_fieldName.size() > 0 ) {
		//
		// search for corresponding feature in vector datasource:
		//
		bool finished = false;
		bool found = false;
		while( !finished && (feature = layer->GetNextFeature()) != NULL ) {
			const int i = feature->GetFieldIndex(desired_fieldName.c_str());
			if ( i < 0 ) {
				finished = true;
			}
			else {
				const char* str = feature->GetFieldAsString(i);
				assert(str);
				if ( desired_fieldValue == string(str) ) {
					process_feature(feature);
					finished = true;
					found = true;
				}
			}
			delete feature;
		}
		// cout<< "   found=" <<found << endl;
	}
	//
	// else: process each feature in vector datasource:
	//
	else {
		Progress* progress = 0;
		if ( progress_out ) {
			*progress_out << "Number of features: ";
			long psize = layer->GetFeatureCount();
			if ( psize >= 0 ) {
				*progress_out << psize << "\n\t";
				progress = new Progress(psize, progress_perc, *progress_out);
			}
			else {
				*progress_out << "(not known in advance)\n\t";
				progress = new Progress((long)progress_perc, *progress_out);
			}
			progress->start();
		}
		while( (feature = layer->GetNextFeature()) != NULL ) {
			process_feature(feature);
			delete feature;
			if ( progress )
				progress->update();
		}
		if ( progress ) {
			progress->complete();
			delete progress;
			progress = 0;
			*progress_out << endl;
		}
	}
	
	//
	// notify observers about finalization of process
	//
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->end();
}


