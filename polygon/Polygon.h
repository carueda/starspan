/*
	Polygon - basic operations
	$Id$
*/
#ifndef Polygon_h
#define Polygon_h

/**
 Represents a polygon.
*/
class Polygon {
public:
	// Creates a polygon.
	Polygon();
	
	// destroys this polygon.
	~Polygon();
	
	// adds a point to this polygon.
	void addPoint(double x, double y);
	
	//  Returns the number of vertices of this polygon
	int getSize();
	
	// determines if this polygon is simple.
	bool isSimple();
	
	// determines if this polygon is convex.
	bool isConvex();
	
	// checks if a point lays inside this polygon.
	bool containsPoint(double x, double y);
	
private:
	void* prv_data;
};

#endif
