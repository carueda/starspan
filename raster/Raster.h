/*
	raster - raster interface
	$Id$
*/
#ifndef Raster_h
#define Raster_h

/** Represents a raster. */
class Raster {
public:
	// initializes this module
	static int init(void);
	
	// Creates a raster.
	Raster(const char* filename);
	
	// destroys this raster.
	~Raster();
	
private:
	void* prv_data;
};

#endif
