//
// Traverse
// Carlos A. Rueda
// $Id$
//

#ifndef traverser_h
#define traverser_h

#include "Raster.h"
#include "Vector.h"

#include <stdio.h>


/**
  * Any object interested in doing some task as geometries are
  * traversed must extend this interface.
  */
class Observer {
public:
	virtual ~Observer() {}
	
	/** A new intersection has been found */
	virtual void intersection(OGRFeature* feature, OGREnvelope intersection_env) {}
	
	/** A new signature has been extracted */
	virtual void addSignature(double x, double y, void* signature, GDALDataType rasterType, int typeSize) {}
};

/**
  * A Traverser intersects every geometry feature in a vector datasource
  * with a raster dataset. An observer should be registered for the
  * actual work to be done.
  */
class Traverser {
public:
	Traverser(Raster* r, Vector* v);
	~Traverser();
	
	void setObserver(Observer* aObserver) { observer = aObserver; }
	Observer* getObserver(void) { return observer; }
	
	void traverse(void);
	
private:
	Raster* rast;
	Vector* vect;
	Observer* observer;
};



#endif

