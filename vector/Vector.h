/*
	vector - vector interface
	$Id$
*/
#ifndef Vector_h
#define Vector_h

/** Represents a vector. */
class Vector {
public:
	// initializes this module
	static int init(void);
	
	// Creates a vector.
	Vector(const char* filename);
	
	// destroys this vector.
	~Vector();
	
private:
	void* prv_data;
};

#endif
