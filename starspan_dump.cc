//
// STARSpan project
// Carlos A. Rueda
// starspan_dump - dump geometries (testing routine)
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include <iostream>
#include <cstdlib>

#include "unistd.h"   // unlink           
#include <string>
#include <stdlib.h>
#include <stdio.h>


using namespace std;



/**
  * Writes geometries and grid to a ptplot file.
  */
class DumpObserver : public Observer {
public:
	OGRLayer* layer;
	Raster* rast;
	GlobalInfo* global_info;
	FILE* file;
	long pixel_count;
	bool use_polys;
	double pix_x_size, pix_y_size;

	/**
	  * Creates a dump object
	  */
	DumpObserver(OGRLayer* layer, Raster* rast, bool use_polys, FILE* f)
	: layer(layer), rast(rast), file(f), use_polys(use_polys)
	{
		global_info = 0;
		pixel_count = 0;
	}
	
	/**
	  * simply calls end()
	  */
	~DumpObserver() {
		end();
	}
	
	/**
	  * finalizes current feature if any; closes the file
	  */
	void end() {
		if ( file ) {
			fclose(file);
			fprintf(stdout, "dump: finished.\n");
			file = 0;
		}
	}
	
	/**
	  * returns true.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * Creates grid according to raster
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		double x0, y0, x1, y1;
		rast->getCoordinates(&x0, &y0, &x1, &y1);
		rast->getPixelSize(&pix_x_size, &pix_y_size);
		if ( x0 > x1 ) {
			double tmp = x0;
			x0 = x1;
			x1 = tmp;
		}
		if ( y0 > y1 ) {
			double tmp = y0;
			y0 = y1;
			y1 = tmp;
		}
		double abs_pix_x_size = pix_x_size;
		double abs_pix_y_size = pix_y_size;
		if ( abs_pix_x_size < 0.0 )
			abs_pix_x_size = -abs_pix_x_size;
		if ( abs_pix_y_size < 0.0 )
			abs_pix_y_size = -abs_pix_y_size;
		
		fprintf(file, "XTicks:");
		for ( double x = x0; x <= x1; x += abs_pix_x_size )
			fprintf(file, " %10.3f %10.3f,", x, x);
		fprintf(file, "\n");
		fprintf(file, "YTicks:");
		for ( double y = y0; y <= y1; y += abs_pix_y_size )
			fprintf(file, " %10.3f %10.3f,", y, y);
		fprintf(file, "\n");


		fprintf(file, "DataSet: grid_envelope\n");
		fprintf(file, "%f , %f\n", x0, y0);
		fprintf(file, "%f , %f\n", x1, y0);
		fprintf(file, "%f , %f\n", x1, y1);
		fprintf(file, "%f , %f\n", x0, y1);
		fprintf(file, "%f , %f\n", x0, y0);
	}

	void processPoint(OGRPoint* point) {
		fprintf(file, "DataSet:\n");
		fprintf(file, "%10.3f , %10.3f\n", point->getX(), point->getY());
	}
	void processMultiPoint(OGRMultiPoint* pp) {
		for ( int i = 0; i < pp->getNumGeometries(); i++ ) {
			OGRPoint* point = (OGRPoint*) pp->getGeometryRef(i);
			processPoint(point);
		}
	}
	void processLineString(OGRLineString* linstr) {
		fprintf(file, "DataSet:\n");
		int num_points = linstr->getNumPoints();
		for ( int i = 0; i < num_points; i++ ) {
			OGRPoint point;
			linstr->getPoint(i, &point);
			fprintf(file, "%10.3f , %10.3f\n", point.getX(), point.getY());
		}
	}
	void processMultiLineString(OGRMultiLineString* coll) {
		for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
			OGRLineString* linstr = (OGRLineString*) coll->getGeometryRef(i);
			processLineString(linstr);
		}
	}
	void processPolygon(OGRPolygon* poly) {
		OGRLinearRing* ring = poly->getExteriorRing();
		processLineString(ring);
	}


	/**
	  * 
	  */
	void writeFeature(OGRFeature* feature) {
		fprintf(file, "DataSet: FID=%ld\n", feature->GetFID());

		OGRGeometry* geometry = feature->GetGeometryRef();
		OGRwkbGeometryType type = geometry->getGeometryType();
		switch ( type ) {
			case wkbPoint:
			case wkbPoint25D:
				processPoint((OGRPoint*) geometry);
				break;
		
			case wkbMultiPoint:
			case wkbMultiPoint25D:
				processMultiPoint((OGRMultiPoint*) geometry);
				break;
		
			case wkbLineString:
			case wkbLineString25D:
				processLineString((OGRLineString*) geometry);
				break;
		
			case wkbMultiLineString:
			case wkbMultiLineString25D:
				processMultiLineString((OGRMultiLineString*) geometry);
				break;
				
			case wkbPolygon:
			case wkbPolygon25D:
				processPolygon((OGRPolygon*) geometry);
				break;
				
			default:
				fprintf(stdout, "%s: intersection type not considered\n",
					OGRGeometryTypeToName(type)
				);
		}
	}
	
	
	/**
	  * Inits creation of datasets corresponding to new feature
	  */
	void intersectionFound(OGRFeature* feature) {
		writeFeature(feature);
	}

	/**
	  * writes out the point in the given event.
	  */
	void addPixel(TraversalEvent& ev) {
		fprintf(file, "DataSet:\n");
		if ( use_polys ) {
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y);
			fprintf(file, "%f , %f\n", ev.pixel.x + pix_x_size, ev.pixel.y);
			fprintf(file, "%f , %f\n", ev.pixel.x + pix_x_size, ev.pixel.y + pix_y_size);
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y + pix_y_size);
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y);
		}
		else {
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y);
		}
	}
};



Observer* starspan_dump(
	Traverser& tr,
	bool use_polys,
	const char* filename
) {	
	if ( !tr.getVector() ) {
		fprintf(stderr, "vector datasource expected\n");
		return 0;
	}
	OGRLayer* layer = tr.getVector()->getLayer(0);
	if ( !layer ) {
		fprintf(stdout, "warning: No layer 0 found\n");
		return 0;
	}

	if ( tr.getNumRasters() == 0 ) {
		fprintf(stderr, "raster expected\n");
		return 0;
	}
	Raster* rast = tr.getRaster(0);

	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr << "Couldn't create " << filename << endl;
		return 0;
	}
	return new DumpObserver(layer, rast, use_polys, file);
}


