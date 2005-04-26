//
// STARSpan project
// Carlos A. Rueda
// starspan_countbyclass - count by class
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       
#include "Stats.h"       

#include <iostream>
#include <cstdlib>
#include <math.h>
#include <cassert>

using namespace std;


/**
  * Creates fields and populates the table.
  */
class CountByClassObserver : public Observer {
public:
	Traverser& tr;
	GlobalInfo* global_info;
	Vector* vect;
	FILE* file;
	bool OK;
		
	/**
	  * Creates a stats calculator
	  */
	CountByClassObserver(Traverser& tr, FILE* f) : tr(tr), file(f) {
		vect = tr.getVector();
		global_info = 0;
		OK = false;
		assert(file);
	}
	
	/**
	  * simply calls end()
	  */
	~CountByClassObserver() {
		end();
	}
	
	/**
	  * closes the file
	  */
	void end() {
		if ( file ) {
			fclose(file);
			cout<< "CountByClass: finished" << endl;
			file = 0;
		}
	}


	/**
	  * returns true. We ask traverser to get values for
	  * visited pixels in each feature.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * Creates first line with column headers:
	  *    FID, class, count
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		OK = false;   // but let's see ...
		
		OGRLayer* poLayer = vect->getLayer(0);
		if ( !poLayer ) {
			cerr<< "Couldn't fetch layer 0" << endl;
			return;
		}
		
		//		
		// write column headers:
		//
		fprintf(file, "FID,class,count\n");
		
		const unsigned num_bands = global_info->bands.size();
		
		if ( num_bands > 1 ) {
			cerr<< "CountByClass: warning: multiband raster data;" <<endl;
			cerr<< "              Only the first band will be processed." <<endl;
		}
		else if ( num_bands == 0 ) {
			cerr<< "CountByClass: warning: no bands in raster data;" <<endl;
			return;
		}

		// check first band is of integral type:
		GDALDataType bandType = global_info->bands[0]->GetRasterDataType();
		if ( bandType == GDT_Float64 || bandType == GDT_Float32 ) {
			cerr<< "CountByClass: warning: first band in raster data is not of integral type" <<endl;
			return;
		}

		// now, all seems OK to continue processing		
		OK = true;
	}
	
	/**
	  * gets the counts and writes news records accordingly.
	  */
	void intersectionEnd(OGRFeature* feature) {
		if ( !OK )
			return;

		const long FID = feature->GetFID();
		
		// get values of pixels corresponding to first band:		
		vector<int> values;
		tr.getPixelIntegerValuesInBand(1, values);
		
		// get the counts:
		map<int,int> counters;
		Stats::computeCounts(values, counters);

		// report the counts:
		for ( map<int, int>::iterator it = counters.begin(); it != counters.end(); it++ ) {
			const int class_ = it->first;
			const int count = it->second;
			
			// add record to file:
			fprintf(file, "%ld,%d,%d\n", FID, class_, count);
		}
	}
};



/**
  * starspan_getCountByClassObserver: implementation
  */
Observer* starspan_getCountByClassObserver(
	Traverser& tr,
	const char* filename
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr<< "Couldn't create "<< filename << endl;
		return 0;
	}

	return new CountByClassObserver(tr, file);	
}
		


