/*
	Polygon - basic operations
	$Id$
	See Polygon.h for public doc.
*/

#include "Polygon.h"

// implementation is based on CGAL
#include <CGAL/Cartesian.h>
#include <CGAL/Polygon_2.h>

typedef CGAL::Cartesian<double> K;
typedef K::Point_2 Point;
typedef CGAL::Polygon_2<K> CGALPolygon;



Polygon::Polygon() {
	prv_data = new CGALPolygon();
}

void Polygon::addPoint(double x, double y) {
	CGALPolygon* pgn = (CGALPolygon*) prv_data;
	pgn->push_back(Point(x,y));
}

int Polygon::getSize() {
	CGALPolygon* pgn = (CGALPolygon*) prv_data;
	return pgn->size();
}

bool Polygon::isSimple() {
	CGALPolygon* pgn = (CGALPolygon*) prv_data;
	return pgn->is_simple();
}

bool Polygon::isConvex() {
	CGALPolygon* pgn = (CGALPolygon*) prv_data;
	return pgn->is_convex();
}

bool Polygon::containsPoint(double x, double y) {
	CGALPolygon* pgn = (CGALPolygon*) prv_data;
	return pgn->bounded_side(Point(x,y)) == CGAL::ON_BOUNDED_SIDE;
}

Polygon::~Polygon() {
	if ( !prv_data )
		return;
	delete ((CGALPolygon*) prv_data);
	prv_data = 0;
}


