/*
	vector - vector interface
	$Id$
*/
#ifndef Vector_h
#define Vector_h

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include <stdio.h>  // FILE

/** Represents a vector dataset. */
class Vector {
public:
	// initializes this module
	static int init(void);
	
	// finishes this module
	static int end(void);
	
	// Creates a vector.
	Vector(const char* filename);
	
	// destroys this vector.
	~Vector();
	
	// gets the name of this object
	const char* getName();
	
	// gets a layer
	OGRLayer* getLayer(int layer_num); 
	
	// for debugging
	void report(FILE* file);
	
private:
	OGRSFDriver* poDriver;
	OGRDataSource* poDS;
};

#endif
