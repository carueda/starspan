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

	bufferParams.given = false;
}


void Traverser::setBufferParameters(BufferParams _bufferParams) {
	bufferParams = _bufferParams;
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
	
	observers.clear();
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
			cerr<< "Different number of lines/cols\n";
			exit(1);
		}
		if ( _x0 != x0 || _y0 != y0 || _x1 != x1 || _y1 != y1) {
			cerr<< "Different geographic location\n";
			exit(1);
		}
		if ( _pix_x_size != pix_x_size || _pix_y_size != pix_y_size ) {
			cerr<< "Different pixel size\n";
			exit(1);
		}
	}
}

void Traverser::removeRasters() {
	rasts.clear();
	globalInfo.bands.clear();
	// make sure we have a an empty rasterPoly:
	globalInfo.rasterPoly.empty();
	memset(&summary, 0, sizeof(summary));
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
			cerr<< "Error reading band value, status= " <<status<< "\n";
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


int Traverser::getPixelIntegerValuesInBand(
	unsigned band_index, 
	vector<int>& list
) {
	if ( band_index <= 0 || band_index > globalInfo.bands.size() ) {
		cerr<< "Traverser::getPixelIntegerValuesInBand: band_index " <<band_index<< " out of range\n";
		return 1;
	}
	
	GDALRasterBand* band = globalInfo.bands[band_index-1];
	for ( set<EPixel>::iterator colrow = pixset.begin(); colrow != pixset.end(); colrow++ ) {
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
	vector<double>& list
) {
	if ( band_index <= 0 || band_index > globalInfo.bands.size() ) {
		cerr<< "Traverser::getPixelDoubleValuesInBand: band_index " <<band_index<< " out of range\n";
		return 1;
	}
	
	GDALRasterBand* band = globalInfo.bands[band_index-1];
	for ( set<EPixel>::iterator colrow = pixset.begin(); colrow != pixset.end(); colrow++ ) {
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
// Implementation as a LineRasterizerObserver, but also called directly. 
// Checks for duplicate pixel.
//
void Traverser::pixelFound(double x, double y) {
	// get pixel location:
	int col, row;
	toColRow(x, y, &col, &row);
	
	if ( col < 0 || col >= width 
	||   row < 0 || row >= height ) {
		return;
	}
	
	// check this location has not been processed
	EPixel colrow(col, row);
	if ( pixset.find(colrow) != pixset.end() ) {
		return;
	}
	
	toGridXY(col, row, &x, &y);

	TraversalEvent event(col, row, x, y);
	summary.num_processed_pixels++;
	
	// if at least one observer is not simple...
	if ( notSimpleObserver ) {
		// get also band values
		getBandValuesForPixel(col, row);
		event.bandValues = bandValues_buffer;
	}
	
	// notify observers:
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->addPixel(event);
	
	// keep track of processed pixels
	pixset.insert(colrow);
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
	
	// get envelope corners in pixel coordinates:
	int minCol, minRow, maxCol, maxRow;
	toColRow(intersection_env->getMinX(), intersection_env->getMinY(), &minCol, &minRow);
	toColRow(intersection_env->getMaxX(), intersection_env->getMaxY(), &maxCol, &maxRow);
	// Note: minCol is not necessarily <= maxCol (idem for *Row)

	// get envelope corners in grid coordinates:
	double minX, minY, maxX, maxY;
	toGridXY(minCol, minRow, &minX, &minY);
	toGridXY(maxCol, maxRow, &maxX, &maxY);

	if ( logstream ) {
		(*logstream)
		   << " minCol=" <<minCol<< ", minRow=" <<minRow<< endl 
		   << " maxCol=" <<maxCol<< ", maxRow=" <<maxRow<< endl
		   << " minX=" <<minX<< ", minY=" <<minY<< endl 
		   << " maxX=" <<maxX<< ", maxY=" <<maxY<< endl;
	}
	
	// envelope dimensions:
	int rows_env = abs(maxRow - minRow) +1;
	int cols_env = abs(maxCol - minCol) +1;
	
	// number of pixels found in polygon:
	int num_pixels_in_poly = 0;
	
	if ( verbose ) {
		fprintf(stdout, " %d cols x %d rows\n", cols_env, rows_env);
		fprintf(stdout, " processing rows: %4d", 0); fflush(stdout);
	}

	const double pix_area = fabs(pix_x_size*pix_y_size);
	
	double abs_pix_y_size = fabs(pix_y_size);
	double abs_pix_x_size = fabs(pix_x_size);
	double y = minY;
	for ( int i = 0; i < rows_env; i++, y += abs_pix_y_size) {
		double x = minX;
		for (int j = 0; j < cols_env; j++, x += abs_pix_x_size) {
			// create pixel polygon for (x,y) grid location:
			geos::Polygon* pix_poly = create_pix_poly(x, y, x + pix_x_size, y + pix_y_size);
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
				if ( area >= pixelProportion * pix_area ) { 
					num_pixels_in_poly++;
					
					// we now check for pixel duplication always
					pixelFound(x, y);
					//pixelFoundInPolygon(x, y);  <-  no any more
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
					geos::Polygonizer polygonizer;
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

	///////////////////////////////////////////////////////////////////
	//
	// apply buffer operation if so indicated:
	//
	if ( bufferParams.given ) {
		OGRGeometry* buffered_geometry = 0;
		
		// get distance:
		double distance; 
		if ( bufferParams.distance[0] == '@' ) {
			const char* attr = bufferParams.distance.c_str() + 1;
			int index = feature->GetFieldIndex(attr);
			if ( index < 0 ) {
				cerr<< "\n\tField `" <<attr<< "' not found\n";
				return;
			}
			distance = feature->GetFieldAsInteger(index);
		}
		else {
			distance = atoi(bufferParams.distance.c_str());
		}
		
		// get quadrantSegments:
		int quadrantSegments;
		if ( bufferParams.quadrantSegments[0] == '@' ) {
			const char* attr = bufferParams.quadrantSegments.c_str() + 1;
			int index = feature->GetFieldIndex(attr);
			if ( index < 0 ) {
				cerr<< "\n\tField `" <<attr<< "' not found\n";
				return;
			}
			quadrantSegments = feature->GetFieldAsInteger(index);
		}
		else {
			quadrantSegments = atoi(bufferParams.quadrantSegments.c_str());
		}
		
		
		
		try {
			buffered_geometry = feature_geometry->Buffer(distance, quadrantSegments);
		}
		catch(geos::GEOSException* ex) {
			cerr<< ">>>>> FID: " << feature->GetFID()
				<< "  GEOSException: " << ex->toString()<< endl;
			return;
		}
		if ( !buffered_geometry ) {
			cout<< ">>>>> FID: " << feature->GetFID()
				<< "  null buffered result!" << endl;
			return;
		}
		
		feature_geometry = buffered_geometry;
		// NOTE that this feature_geometry must be deleted
		// since it's a new created object. See below for destruction
	}
	
	
	//
	// intersect this feature with raster (raster ring)
	//
	OGRGeometry* intersection_geometry = 0;
	
	try {
		intersection_geometry = globalInfo.rasterPoly.Intersection(feature_geometry);
	}
	catch(geos::GEOSException* ex) {
		cerr<< ">>>>> FID: " << feature->GetFID()
		    << "  GEOSException: " << ex->toString()<< endl;
		goto done;
	}

	if ( !intersection_geometry ) {
		if ( verbose ) {
			cout<< " NO INTERSECTION:\n";
		}
		goto done;
	}
	summary.num_intersecting_features++;

	if ( verbose ) {
		cout<< " Type of intersection: "
		    << intersection_geometry->getGeometryName()<< endl;
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

	//
	// notify observers processing of this feature has finished
	// 
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->intersectionEnd(feature);

done:
	delete intersection_geometry;
	if ( bufferParams.given ) {
		delete feature_geometry;
	}
}


//
// main method for traversal
//
void Traverser::traverse() {
	// don't allow a recursive call
	if ( bandValues_buffer ) {
		cerr<< "traverser.traverse: recursive call!\n";
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

	lineRasterizer = new LineRasterizer(x0, y0, pix_x_size, pix_y_size);
	lineRasterizer->setObserver(this);
	
	memset(&summary, 0, sizeof(summary));
	
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
			long psize = layer->GetFeatureCount();
			if ( psize >= 0 ) {
				progress = new Progress(psize, progress_perc, *progress_out);
			}
			else {
				progress = new Progress((long)progress_perc, *progress_out);
			}
			*progress_out << "\t";
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

	delete[] bandValues_buffer;
	bandValues_buffer = 0;
	delete lineRasterizer;
	lineRasterizer = 0;
}


void Traverser::reportSummary() {
	cout<< "Summary:" <<endl;
	cout<< "  Intersecting features: " << summary.num_intersecting_features<< endl;
	if ( summary.num_point_features )
		cout<< "      Points: " <<summary.num_point_features<< endl;
	if ( summary.num_multipoint_features )
		cout<< "      MultiPoints: " <<summary.num_multipoint_features<< endl;
	if ( summary.num_linestring_features )
		cout<< "      LineStrings: " <<summary.num_linestring_features<< endl;
	if ( summary.num_multilinestring_features )
		cout<< "      MultiLineStrings: " <<summary.num_multilinestring_features<< endl;
	if ( summary.num_polygon_features )
		cout<< "      Polygons: " <<summary.num_polygon_features<< endl;
	if ( summary.num_invalid_polys )
		cout<< "          invalid: " <<summary.num_invalid_polys<< endl;
	if ( summary.num_polys_with_internal_ring )
		cout<< "          with internalring: " <<summary.num_polys_with_internal_ring<< endl;
	if ( summary.num_polys_exploded )
		cout<< "          exploded: " <<summary.num_polys_exploded<< endl;
	if ( summary.num_sub_polys )
		cout<< "          sub polys: " <<summary.num_sub_polys<< endl;
	if ( summary.num_multipolygon_features )
		cout<< "      MultiPolygons: " <<summary.num_multipolygon_features<< endl;
	if ( summary.num_geometrycollection_features )
		cout<< "      GeometryCollections: " <<summary.num_geometrycollection_features<< endl;
	cout<< endl;
	cout<< "  Processed pixels: " <<summary.num_processed_pixels<< endl;
}