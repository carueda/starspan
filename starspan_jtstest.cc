//
// STARSpan project
// Carlos A. Rueda
// starspan_jtstest - generate JTS test
// $Id$
//

#include "starspan.h"           
#include "jts.h"       
#include <geos/io.h>

#include <stdlib.h>
#include <assert.h>


void starspan_jtstest(Raster& rast, Vector& vect, const char* jtstest_filename) {
	// get raster size and coordinates
	int width, height;
	rast.getSize(&width, &height, NULL);
	double x0, y0, x1, y1;
	rast.getCoordinates(&x0, &y0, &x1, &y1);

	double pix_size_x = (x1 - x0) / width;
	double pix_size_y = (y1 - y0) / height;
	
	if ( false ) {   // verbose?--PENDING
		fprintf(stdout, "(width,height) = (%d,%d)\n", width, height);
		fprintf(stdout, "(x0,y0) = (%7.1f,%7.1f)\n", x0, y0);
		fprintf(stdout, "(x1,y1) = (%7.1f,%7.1f)\n", x1, y1);
		fprintf(stdout, "pix_size = (%7.1f,%7.1f)\n", pix_size_x, pix_size_y);
	}

	// create a geometry for raster envelope:
	OGRLinearRing* raster_ring = new OGRLinearRing();
	raster_ring->addPoint(x0, y0);
	raster_ring->addPoint(x1, y0);
	raster_ring->addPoint(x1, y1);
	raster_ring->addPoint(x0, y1);
	raster_ring->closeRings();
	OGREnvelope raster_env;
	raster_ring->getEnvelope(&raster_env);
	
	if ( false ) {
		fprintf(stderr, "raster_ring:\n");
		raster_ring->dumpReadable(stderr);
		//print_envelope(stderr, "raster_env:", raster_env);
	}
	
	OGRLayer* layer = vect.getLayer(0);
	if ( !layer ) {
		fprintf(stdout, "No layer 0 found\n");
		return;
	}
	
	// check type of geometries
	OGRFeatureDefn* poDefn = layer->GetLayerDefn();
	OGRwkbGeometryType gtype = poDefn->GetGeomType();
	if ( gtype != wkbPolygon
	&&   gtype != wkbPoint
	&&   gtype != wkbLineString ) {
		fprintf(stdout, 
			"Geometry type is %s. Currently accepting: polygon, point, line-string.\n",
			OGRGeometryTypeToName(gtype)
		);
		return;
	}
	
	
	
	// take absolute values since intersection_env has increasing
	// coordinates:
	if ( pix_size_x < 0 )
		pix_size_x = -pix_size_x;
	if ( pix_size_y < 0 )
		pix_size_y = -pix_size_y;
	
	
	JTS_TestGenerator jtstest(jtstest_filename);
	int jtstest_count = 0;
	
	OGRMultiPoint* grid = new OGRMultiPoint();
    OGRPoint* point = new OGRPoint();

    OGRFeature* feature;
	while( (feature = layer->GetNextFeature()) != NULL ) {
		OGRGeometry* geom = feature->GetGeometryRef();
		

		OGREnvelope feature_env;
		geom->getEnvelope(&feature_env);
		OGREnvelope intersection_env;

		// first, intersect envelopes:
		if ( !starspan_intersect_envelopes(raster_env, feature_env, intersection_env) ) {
			fputc('.', stdout); fflush(stdout);
			continue;
		}
		
		
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
		


		////////////////////////////////////////////
		// Point
		if ( gtype == wkbPoint ) {
			fputc('!', stdout); fflush(stdout);
			
			point->setX(grid_x0);
			point->setY(grid_y0);
			
			// create test case:
			jtstest_count++;
			jtstest.case_init("point contains point");
				jtstest.case_arg_init("a");
					geom->dumpReadable(jtstest.getFile(), "    ");
				jtstest.case_arg_end("a");
				jtstest.case_arg_init("b");
					point->dumpReadable(jtstest.getFile(), "    ");
				jtstest.case_arg_end("b");
			jtstest.case_end();
			fprintf(stdout, "case %d: point contains point\n", 
				jtstest_count
			);
		}
		
		////////////////////////////////////////////
		// Polygon
		else if ( gtype == wkbPolygon ) {
			// create grid (multipoint):
			grid->empty();
			int grid_lines = 0;
			int grid_points = 0;
			for (double y = grid_y0; y <= intersection_env.MaxY+pix_size_y; y += pix_size_y, grid_lines++) {
				for (double x = grid_x0; x <= intersection_env.MaxX+pix_size_x; x += pix_size_x) {
					OGRPoint* grid_point = new OGRPoint();
					grid_point->setX(x);
					grid_point->setY(y);
					grid->addGeometryDirectly(grid_point);
					grid_points++;
				}
			}
			int grid_cols = grid_points / grid_lines;
			fprintf(stdout, "Grid dimensions %d x %d = %d points.  Computing intersection\n",
				grid_lines, grid_cols, grid_points
			);
	
			// intersect grid with geom
			OGRGeometry* intersection = geom->Intersection(grid);
			if ( !intersection ) {
				fprintf(stdout, "No grid points obtained in intersection\n");
				continue;
			}
			if ( intersection->getGeometryType() != wkbMultiPoint ) {
				fprintf(stdout, "Expecting a MultiPoint as intersection!\n");
				continue;
			}
			OGRMultiPoint* mpintersection = (OGRMultiPoint*) intersection;
	
			// intersection is non-empty
			fputc('!', stdout); fflush(stdout);
			
			// create test case:
			jtstest_count++;
			jtstest.case_init("geometry contains points");
				jtstest.case_arg_init("a");
					geom->dumpReadable(jtstest.getFile(), "    ");
				jtstest.case_arg_end("a");
				jtstest.case_arg_init("b");
					mpintersection->dumpReadable(jtstest.getFile(), "    ");
				jtstest.case_arg_end("b");
			jtstest.case_end();
			fprintf(stdout, "case %d: %d points\n", 
				jtstest_count,
				mpintersection->getNumGeometries()
			);
		}
		
		////////////////////////////////////////////
		// Line String
		else if ( gtype == wkbLineString ) {
			OGRLineString* linstr = (OGRLineString*) geom;
			
			// PENDING
			OGRMultiPoint* pp = new OGRMultiPoint();
			
			
			int num_points = linstr->getNumPoints();
			fprintf(stdout, "      num_points = %d\n", num_points);
			for ( int i = 0; i < num_points; i++ ) {
				linstr->getPoint(i, point);
				if ( raster_ring->Contains(point) )
					pp->addGeometry(point);
			}
			
			// create test case:
			jtstest_count++;
			jtstest.case_init("line-string contains points");
				jtstest.case_arg_init("a");
					linstr->dumpReadable(jtstest.getFile(), "    ");
				jtstest.case_arg_end("a");
				jtstest.case_arg_init("b");
					pp->dumpReadable(jtstest.getFile(), "    ");
				jtstest.case_arg_end("b");
			jtstest.case_end();
			fprintf(stdout, "case %d: line-string contains points %d\n",
				num_points,
				jtstest_count
			);
			
			delete pp;
		}

		////////////////////////////////////////////
		// unexpected type
		else {
			assert(false && "should not happen.  please, notify this bug.");
		}

		delete feature;
	}
	delete point;
	delete grid;
	delete raster_ring;
	
	int no_cases = jtstest.end();
	fprintf(stdout, "%d test cases generated\n", no_cases);
}


