//
// STARSpan project
// Traverse::processValidPolygon_QT
// Carlos A. Rueda
// $Id$
// Quadtree algorithm for polygon rasterization 
//

#include "traverser.h"           

#include <cstdlib>
#include <cassert>
#include <cstring>

// for polygon processing:
#include "geos/opPolygonize.h"


inline void swap_if_greater(int& a, int&b) {
	if ( a > b ) {
		int tmp = a;
		a = b;
		b = tmp;
	}
}



// processValidPolygon_QT: Quadtree algorithm
void Traverser::processValidPolygon_QT(geos::Polygon* geos_poly) {
	const geos::Envelope* intersection_env = geos_poly->getEnvelopeInternal();
	
	// get envelope corners in pixel coordinates:
	int minCol, minRow, maxCol, maxRow;
	toColRow(intersection_env->getMinX(), intersection_env->getMinY(), &minCol, &minRow);
	toColRow(intersection_env->getMaxX(), intersection_env->getMaxY(), &maxCol, &maxRow);
	
	// since minCol is not necessarily <= maxCol (ditto for *Row):
	swap_if_greater(minCol, maxCol);
	swap_if_greater(minRow, maxRow);
	
	double x, y;
	toGridXY(minCol, minRow, &x, &y);
	_Rect env(this, x, y, maxCol - minCol + 1, maxRow - minRow +1);
	
	rasterize_poly_QT(env, geos_poly);
}

// implicitily does a quadtree-like scan:
void Traverser::rasterize_poly_QT(_Rect& e, geos::Polygon* i) {
	if ( i == NULL || e.empty() )
		return;
	
	const double pix_area = fabs(pix_x_size*pix_y_size);
	const double pixelProportion_times_pix_area = pixelProportion * pix_area;


	// compare envelope and intersection areas: 
	double area_e = e.area();
	double area_i = i->getArea();
	
	// Sometimes I get that, say, 182 > 182 is true, so put
	// here a small epsilon:
	if ( area_i > area_e + 10e-6 ) {
		cerr<< "--Internal error: area_i=" <<area_i<< " > area_e=" <<area_e<< endl;
		return;
	}

	if ( area_i >= area_e - pixelProportion_times_pix_area ) {
		// all pixels in e are to be reported:
		dispatchRect_QT(e);
		return;
	}
	
	if ( area_i < pixelProportion_times_pix_area ) { 
		// do nothing (we can safely discard the whole rectangle).
		return;
	}
	
	_Rect e_ul = e.upperLeft();
	geos::Geometry* i_ul = e_ul.intersect(i);
	if ( i_ul ) {		
		rasterize_geometry_QT(e_ul, i_ul);
		delete i_ul;
	}
	
	_Rect e_ur = e.upperRight();
	geos::Geometry* i_ur = e_ur.intersect(i);
	if ( i_ur ) {		
		rasterize_geometry_QT(e_ur, i_ur);
		delete i_ur;
	}

	_Rect e_ll = e.lowerLeft();
	geos::Geometry* i_ll = e_ll.intersect(i);
	if ( i_ll ) {		
		rasterize_geometry_QT(e_ll, i_ll);
		delete i_ll;
	}

	_Rect e_lr = e.lowerRight();
	geos::Geometry* i_lr = e_lr.intersect(i);
	if ( i_lr ) {		
		rasterize_geometry_QT(e_lr, i_lr);
		delete i_lr;
	}
}

// implicitily does a quadtree-like scan:
void Traverser::rasterize_geometry_QT(_Rect& e, geos::Geometry* i) {
	if ( i == NULL || e.empty() )
		return;
	geos::GeometryTypeId type = i->getGeometryTypeId();
	switch ( type ) {
		case geos::GEOS_POLYGON:
			rasterize_poly_QT(e, (geos::Polygon*) i);
			break;
			
		case geos::GEOS_MULTIPOLYGON:
		case geos::GEOS_GEOMETRYCOLLECTION: {
			geos::GeometryCollection* gc = (geos::GeometryCollection*) i; 
			for ( int j = 0; j < gc->getNumGeometries(); j++ ) {
				geos::Geometry* g = (geos::Geometry*) gc->getGeometryN(j);
				rasterize_geometry_QT(e, (geos::Polygon*) g);
			}
			break;
		}
	
		default:
			if ( logstream ) {
				(*logstream)
					<< "Warning: rasterize_geometry_QT: "
					<< "intersection ignored: "
					<< i->getGeometryType() <<endl
					//<< wktWriter.write(i) <<endl
				;
			}
	}
	
}

void Traverser::dispatchRect_QT(_Rect& r) {
	int col, row;
	toColRow(r.x, r.y, &col, &row);
	for ( int i = 0; i < r.rows; i++ ) {
		double y = r.y + i * pix_y_size; 
		for ( int j = 0; j < r.cols; j++ ) {
			double x = r.x + j * pix_x_size;
			EPixel colrow(col + j, row + i);
			dispatchPixel(colrow, x, y);
		}
	}
}

