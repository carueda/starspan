//
// Progress - A simple progress indicator
// Carlos A. Rueda
// $Id$
//

#ifndef Progress_h
#define Progress_h

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


/**
  * A simple progress indicator
  */
class Progress {
public:
	/** creates a progress object for percentage.
	  * @param size number of items corresponding to 100%
	  * @param perc_incr increment in percentage, must be > 0
	  * @param out output stream
	  */
	Progress(long size, double perc_incr, ostream& out=cout);
	
	/** creates a progress object.
	  * @param chunksize increment amount
	  * @param out output stream
	  */
	Progress(long chunksize, ostream& out=cout);
	
	/** Updates this progress object */
	void update();

	/** Ends this progress object */
	void end();
	
private:
	long size;
	double perc_incr;
	ostream& out;
	double next_perc;
	double curr_perc;
	long curr;
	string last_msg;
	void writemsg(string msg);
	ostringstream oss;
};


#endif

