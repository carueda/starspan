//
// STARSpan project
// Carlos A. Rueda
// starspan_jtstest - generate JTS test
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include "jts.h"       
#include <geos/io.h>

#include <stdlib.h>
#include <assert.h>


#include "rasterizers.h"


static bool use_polys = false;

/**
  *
  */
struct JtsTestObserver : public Observer {
	JTS_TestGenerator* jtstest;
	int jtstest_count;
	OGRPolygon* raster_poly;
	OGRGeometryCollection* pp;
	double pix_x_size, pix_y_size;
	
	/**
	  *
	  */
	JtsTestObserver(const char* jtstest_filename, double pix_x_size_, double pix_y_size_)
	: pix_x_size(pix_x_size_), pix_y_size(pix_y_size_) {
		jtstest = new JTS_TestGenerator(jtstest_filename);
		jtstest_count = 0;
		raster_poly = NULL;
		pp = NULL;
	}

	/**
	  *
	  */
	~JtsTestObserver() {
		if ( pp ) {
			finish_case();
		}
		delete jtstest;
	}

	/**
	  *
	  */
	void rasterPoly(OGRPolygon* raster_poly_) {
		raster_poly = raster_poly_;
		fprintf(stdout, "raster_poly: ");
		raster_poly->dumpReadable(stdout);
		fprintf(stdout, "\n");
	}

	/**
	  *
	  */
	void intersectionFound(OGRFeature* feature) {
		if ( pp ) {
			finish_case();
		}
		
		OGRGeometry* geom = feature->GetGeometryRef();
		//OGRGeometry* geom = raster_poly;
		
		char case_descr[128];
		sprintf(case_descr, "%s contains multipoint", geom->getGeometryName());
		fprintf(stdout, "CASE: %s\n", case_descr);
		jtstest_count++;
		jtstest->case_init(case_descr);
		// a argument:
		jtstest->case_arg_init("a");
		geom->dumpReadable(jtstest->getFile(), "    ");
		jtstest->case_arg_end("a");
		// b argument:
		if ( use_polys )
			pp = new OGRMultiPolygon();
		else
			pp = new OGRMultiPoint();
	}
	
	/**
	  *
	  */
	void addPixel(TraversalEvent& ev) { 
		double x = ev.pixelLocation.x;
		double y = ev.pixelLocation.y;
		assert(pp);
		if ( use_polys ) {
			double x0, y0, x1, y1;
			if ( pix_x_size < 0 ) {
				x0 = x + pix_x_size;
				x1 = x0 - .95*pix_x_size;
			}
			else {
				x0 = x;
				x1 = x0 + .95*pix_x_size;
			}
			if ( pix_y_size < 0 ) {
				y0 = y + pix_y_size;
				y1 = y0 - .95*pix_y_size;
			}
			else {
				y0 = y;
				y1 = y0 + .95*pix_y_size;
			}
			OGRPolygon poly;
			OGRLinearRing ring;
			ring.addPoint(x0, y0);
			ring.addPoint(x0, y1);
			ring.addPoint(x1, y1);
			ring.addPoint(x1, y0);
			poly.addRing(&ring);
			poly.closeRings();
			pp->addGeometry(&poly);
		}
		else {
			OGRPoint point(x,y);
			pp->addGeometry(&point);
		}
	}

	/**
	  *
	  */
	void finish_case() {
		assert(pp);
		int count = pp->getNumGeometries();
		if ( count > 0 ) {
			jtstest->case_arg_init("b");
			if ( false ) {
				OGRGeometry* g0 = pp->getGeometryRef(0); 
				g0->dumpReadable(jtstest->getFile(), "    ");
			}
			else {
				pp->dumpReadable(jtstest->getFile(), "    ");
			}
			jtstest->case_arg_end("b");
			jtstest->case_end("intersects");
			fprintf(stdout, "%d points in multipoint\n", count);
		}
		else {
			jtstest->case_cancel();
		}
		delete pp;
		pp = NULL;
	}

	/**
	  * returns true.
	  */
	bool isSimple() { 
		return true; 
	}
};



/**
  * implementation
  */
void starspan_jtstest(Raster& rast, Vector& vect, const char* jtstest_filename) {
	double pix_x_size, pix_y_size;
	rast.getPixelSize(&pix_x_size, &pix_y_size);
	JtsTestObserver observer(jtstest_filename, pix_x_size, pix_y_size);
	Traverser traverser(&rast, &vect);
	traverser.setObserver(&observer);
	traverser.traverse();
}


