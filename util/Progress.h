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
	/** creates a progress object */
	Progress(long size, long perc_incr=0, ostream& out=cout);
	
	/** Updates this progress object */
	void update();

	/** Ends this progress object */
	void end();
	
private:
	long size;
	long perc_incr;
	ostream& out;
	double next_perc;
	double curr_perc;
	long curr;
	string last_msg;
	void writemsg(string msg);
};


#endif

