//
// Progress - A simple progress indicator
// Carlos A. Rueda
// $Id$
// See Progress.h for public doc.
//

#include "Progress.h"

Progress::Progress(long size, long perc_incr, ostream& out)
: size(size), perc_incr(perc_incr), out(out) {
	curr = 1;
	if ( perc_incr > 0 ) {
		next_perc = perc_incr;
		writemsg(string("0% "));
	}
	else
		writemsg(string("0 "));
}

void Progress::writemsg(string msg) {
	if ( msg != last_msg ) {
		out << msg;
		out.flush();
		last_msg = msg;
	}
}

void Progress::update() {
	if ( perc_incr > 0 ) {
		curr_perc = 100.0 * curr / size;
		if ( curr_perc >= next_perc ) {
			ostringstream ostr;
			ostr << next_perc << "% ";
			writemsg(ostr.str());
			next_perc += perc_incr;
		}
	}
	else if ( curr % size == 0 ) {
		ostringstream ostr;
		ostr << curr << " ";
		writemsg(ostr.str());
	}
	curr += 1;
}

void Progress::end() {
	if ( perc_incr > 0 )
		writemsg(string("100% "));
	else {
		ostringstream ostr;
		ostr << curr-1 << " ";
		writemsg(ostr.str());
	}
}
	

