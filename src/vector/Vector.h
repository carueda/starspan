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
	
	// Creates a vector object representing an existing file.
	Vector(const char* filename);
	
	// destroys this vector.
	~Vector();
	
	// gets the name of this object
	const char* getName();
	
	// gets the number of layers
	int getLayerCount(void) { return poDS->GetLayerCount(); } 
	
	// gets a layer
	OGRLayer* getLayer(int layer_num); 
	
	// for debugging
	void report(FILE* file);
	
	// report
	void showFields(FILE* file);
	
private:
	OGRDataSource* poDS;
};

#endif
