//
// STARSpan project
// Carlos A. Rueda
// starspan_stats - some stats calculation
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <assert.h>



/**
  * Creates fields and populates the table.
  */
class StatsObserver : public Observer {
public:
	GlobalInfo* global_info;
	Vector* vect;
	FILE* file;
	long currentFeatureID;
	double* aggregated;
	int count;  // number of pixel in current feature
	
	/**
	  * Creates a stats calculator
	  */
	StatsObserver(Traverser& tr, FILE* f)
	: file(f)
	{
		vect = tr.getVector();
		global_info = 0;
		currentFeatureID = -1;
		aggregated = 0;
		count = 0;
	}
	
	/**
	  * simply calls end()
	  */
	~StatsObserver() {
		end();
	}
	
	/**
	  * Creates first line with column headers:
	  *    FID, count, bands
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		OGRLayer* poLayer = vect->getLayer(0);
		if ( !poLayer ) {
			fprintf(stderr, "Couldn't fetch layer 0\n");
			exit(1);
		}

		//		
		// write column headers:
		//

		// Create FID field
		fprintf(file, "FID");
		
		// Create counter field
		fprintf(file, ",count");
		
		// Create fields for bands
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			fprintf(file, ",Band%d", i+1);
		}
		
		fprintf(file, "\n");
		
		// allocate buffer for aggregation values:
		aggregated = new double[global_info->bands.size()];
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			aggregated[i] = 0.0;
		}
		count = 0;
	}
	

	/**
	  * dispatches finalization of current feature
	  */
	void finalizeCurrentFeatureIfAny(void) {
		if ( currentFeatureID >= 0 ) {
			// Add FID value:
			fprintf(file, "%ld", currentFeatureID);
			
			// Add count value:
			fprintf(file, ",%d", count);
	 
			// add aggregated band values:
			for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
				fprintf(file, ",%g", aggregated[i]);
			}
			fprintf(file, "\n");
			currentFeatureID = -1;
		}
	}

	/**
	  * dispatches aggregation of current feature and prepares for next
	  */
	void intersectionFound(OGRFeature* feature) {
		finalizeCurrentFeatureIfAny();
		
		// be ready for new feature:
		currentFeatureID = feature->GetFID();
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			aggregated[i] = 0.0;
		}
		count = 0;
	}
	
	
	/**
	  * Adds a new pixel to aggregation
	  */
	void addPixel(TraversalEvent& ev) { 
		// add band values to aggregated:
		char* ptr = (char*) ev.bandValues;
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			int typeSize = GDALGetDataTypeSize(bandType) >> 3;
			aggregated[i] += starspan_extract_double_value(bandType, ptr);
			
			// move to next piece of data in buffer:
			ptr += typeSize;
		}
		count++;
	}

	/**
	  * finalizes current feature if any; closes the file
	  */
	void end() {
		finalizeCurrentFeatureIfAny();
		if ( file ) {
			fclose(file);
			fprintf(stdout, "Stats: finished.\n");
			file = 0;
		}
		if ( aggregated )
			delete[] aggregated;
	}
};



/**
  * implementation
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	const char* filename
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", filename);
		return 0;
	}

	return new StatsObserver(tr, file);	
}
		


