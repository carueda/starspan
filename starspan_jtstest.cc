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



///////////////////////////////////////////////////////////////////////
struct JtsTestObserver : public Observer {
	JTS_TestGenerator* jtstest;
	int jtstest_count;
	OGRPolygon* raster_poly;
	OGRMultiPoint* pp;
	
	JtsTestObserver(const char* jtstest_filename) {
		jtstest = new JTS_TestGenerator(jtstest_filename);
		jtstest_count = 0;
		raster_poly = NULL;
		pp = NULL;
	}
	~JtsTestObserver() {
		if ( pp ) {
			finish_case();
		}
		delete jtstest;
	}

	void rasterPoly(OGRPolygon* raster_poly_) {
		raster_poly = raster_poly_;
		fprintf(stdout, "raster_poly: ");
		raster_poly->dumpReadable(stdout);
		fprintf(stdout, "\n");
	}

	void intersectionFound(OGRFeature* feature) {
		if ( pp ) {
			finish_case();
		}
		
		//OGRGeometry* geom = feature->GetGeometryRef();
		OGRGeometry* geom = raster_poly;
		
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
		pp = new OGRMultiPoint();
	}
	
	void addPixel(double x, double y) {
		assert(pp);
		OGRPoint p(x,y);
		pp->addGeometry(&p);
	}

	void finish_case() {
		assert(pp);
		jtstest->case_arg_init("b");
		pp->dumpReadable(jtstest->getFile(), "    ");
		jtstest->case_arg_end("b");
		jtstest->case_end();
		fprintf(stdout, "%d points in multipoint\n", pp->getNumGeometries());
		delete pp;
		pp = NULL;
	}

	bool isSimple() { 
		return true; 
	}
};



void starspan_jtstest(Raster& rast, Vector& vect, const char* jtstest_filename) {
	JtsTestObserver observer(jtstest_filename);
	Traverser traverser(&rast, &vect);
	traverser.setObserver(&observer);
	traverser.traverse();
}


