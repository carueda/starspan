//
// STARSpan project
// Carlos A. Rueda
// starspan_stats - some stats calculation
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <math.h>
#include <assert.h>



/**
  * Creates fields and populates the table.
  */
class StatsObserver : public Observer {
public:
	Traverser& tr;
	GlobalInfo* global_info;
	Vector* vect;
	FILE* file;
	vector<const char*> desired;
	long currentFeatureID;
	void* bandValues_buffer;
	double* bandValues;
	
	struct Pixel {
		int col;
		int row;
		Pixel(int col, int row) : col(col), row(row) {}
	};
	vector<Pixel> pixels;
	
	// indices into result_stats:
	enum {
		CUM,     // cumulation
		MIN,     // minimum
		MAX,     // maximum
		AVG,     // average
		STDEV,   // std deviation--requires second pass
		TOT_RESULTS
	};
	double* result_stats[TOT_RESULTS];
	bool compute[TOT_RESULTS];
	
	
		
	/**
	  * Creates a stats calculator
	  */
	StatsObserver(Traverser& tr, FILE* f, vector<const char*> desired)
	: tr(tr), file(f), desired(desired)
	{
		vect = tr.getVector();
		global_info = 0;
		currentFeatureID = -1;
		bandValues_buffer = 0;
		bandValues = 0;
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			result_stats[i] = 0;
			compute[i] = false;
		}
		
		for ( vector<const char*>::const_iterator stat = desired.begin(); stat != desired.end(); stat++ ) {
			if ( 0 == strcmp(*stat, "avg") )
				compute[AVG] = true;
			else if ( 0 == strcmp(*stat, "stdev") )
				compute[STDEV] = true;
			else if ( 0 == strcmp(*stat, "min") )
				compute[MIN] = true;
			else if ( 0 == strcmp(*stat, "max") )
				compute[MAX] = true;
			else {
				fprintf(stderr, "Unrecognized stats %s\n", *stat);
				exit(1);
			}
		}
	}
	
	/**
	  * simply calls end()
	  */
	~StatsObserver() {
		end();
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
		if ( bandValues_buffer )
			delete[] (double*) bandValues_buffer;
		if ( bandValues )
			delete[] bandValues;
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			if ( result_stats[i] )
				delete[] result_stats[i];
		}
	}


	/**
	  * returns true. We collect (col,row) locations and then
	  * ask for band values explicitely.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * Creates first line with column headers:
	  *    FID, numPixels, S1_Band1, S1_Band2 ..., S2_Band1, S2_Band2 ...
	  * where S# is each desired statistics
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
		
		// Create numPixels field
		fprintf(file, ",numPixels");
		
		// Create fields for bands
		for ( vector<const char*>::const_iterator stat = desired.begin(); stat != desired.end(); stat++ ) {
			for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
				fprintf(file, ",%s_Band%d", *stat, i+1);
			}
		}		
		fprintf(file, "\n");
		
		// allocate buffer for band values--large enough
		bandValues_buffer = new double[global_info->bands.size()];
		
		// and this is the double array as such
		bandValues = new double[global_info->bands.size()];
		
		// allocate space for all possible results
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			result_stats[i] = new double[global_info->bands.size()];
		}
	}
	

	/**
	  * compute all results from current list of pixels.
	  * Desired results are reported by finalizeCurrentFeatureIfAny.
	  */
	void computeResults(void) {
		// initialize results:
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			for ( unsigned j = 0; j < global_info->bands.size(); j++ )
				result_stats[i][j] = 0.0;
		}

		unsigned num_pixels = 0;
		for ( vector<Pixel>::const_iterator pixel = pixels.begin(); pixel != pixels.end(); pixel++, num_pixels++ ) {
			// first get buffer with all bands
			tr.getBandValues(pixel->col, pixel->row, bandValues_buffer);

			// now get those values in double format:
			char* ptr = (char*) bandValues_buffer;
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				GDALDataType bandType = global_info->bands[j]->GetRasterDataType();
				int typeSize = GDALGetDataTypeSize(bandType) >> 3;
				bandValues[j] += starspan_extract_double_value(bandType, ptr);
				
				// move to next piece of data in buffer:
				ptr += typeSize;
			}

			// cumulate:
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				result_stats[CUM][j] += bandValues[j]; 
			}

			// min and max:
			if ( num_pixels == 0 ) {    
				// first pixel: just take values
				for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
					result_stats[MIN][j] = bandValues[j]; 
					result_stats[MAX][j] = bandValues[j]; 
				}
			}
			else {
				// compare:
				for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
					// min
					if ( result_stats[MIN][j] > bandValues[j] ) 
						result_stats[MIN][j] = bandValues[j];
					
					// max
					if ( result_stats[MAX][j] < bandValues[j] ) 
						result_stats[MAX][j] = bandValues[j]; 
				}
			}
		}
		
		// average
		if ( num_pixels > 0 ) {
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				result_stats[AVG][j] = result_stats[CUM][j] / num_pixels;
			}
		}
		
		// Is a second pass required?
		if ( compute[STDEV] ) {
			
			// standard deviation defined as sqrt of the sample variance.
			// So we need at least 2 pixels.
			
			if ( num_pixels > 1 ) {
				// initialize CUM:
				for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
					result_stats[CUM][j] = 0.0;
				}
				
				// take pixels again
				for ( vector<Pixel>::const_iterator pixel = pixels.begin(); pixel != pixels.end(); pixel++, num_pixels++ ) {
					// first get buffer with all bands
					tr.getBandValues(pixel->col, pixel->row, bandValues_buffer);
		
					// now get those values in double format:
					char* ptr = (char*) bandValues_buffer;
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						GDALDataType bandType = global_info->bands[j]->GetRasterDataType();
						int typeSize = GDALGetDataTypeSize(bandType) >> 3;
						bandValues[j] += starspan_extract_double_value(bandType, ptr);
						
						// move to next piece of data in buffer:
						ptr += typeSize;
					}
				
					// cumulate square variance:
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						double h = bandValues[j] - result_stats[AVG][j];
						result_stats[CUM][j] += h * h; 
					}
				}
				
				// finally take std dev:
				for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
					double sd = sqrt(result_stats[CUM][j] / (num_pixels - 1));
					result_stats[STDEV][j] = sd;
				}
			}
		}
	}

	/**
	  * dispatches finalization of current feature
	  */
	void finalizeCurrentFeatureIfAny(void) {
		if ( currentFeatureID >= 0 ) {
			// Add FID value:
			fprintf(file, "%ld", currentFeatureID);
			
			// Add numPixels value:
			fprintf(file, ",%d", pixels.size());
			
			computeResults();
	 
			// report desired results:
			// (desired list is traversed to keep order according to column headers)
			for ( vector<const char*>::const_iterator stat = desired.begin(); stat != desired.end(); stat++ ) {
				if ( 0 == strcmp(*stat, "avg") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%g", result_stats[AVG][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "stdev") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%g", result_stats[STDEV][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "min") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%g", result_stats[MIN][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "max") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%g", result_stats[MAX][j]);
					}
				}
				else {
					fprintf(stderr, "Unrecognized stats %s\n", *stat);
					exit(1);
				}
			}
			fprintf(file, "\n");
			currentFeatureID = -1;
			pixels.empty();
		}
	}

	/**
	  * dispatches aggregation of current feature and prepares for next
	  */
	void intersectionFound(OGRFeature* feature) {
		finalizeCurrentFeatureIfAny();
		
		// be ready for new feature:
		currentFeatureID = feature->GetFID();
	}
	
	
	/**
	  * Adds a new pixel to aggregation
	  */
	void addPixel(TraversalEvent& ev) {
		pixels.push_back(Pixel(ev.pixel.col, ev.pixel.row));
	}

};



/**
  * implementation
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	const char* filename,
	vector<const char*> desired
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		fprintf(stderr, "Couldn't create %s\n", filename);
		return 0;
	}

	return new StatsObserver(tr, file, desired);	
}
		


