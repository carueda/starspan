//
// STARSpan project
// Carlos A. Rueda
// starspan_stats - some stats calculation
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <iostream>
#include <cstdlib>
#include <math.h>
#include <cassert>

using namespace std;


/**
  * Creates fields and populates the table.
  */
class StatsObserver : public Observer {
public:
	Traverser& tr;
	GlobalInfo* global_info;
	Vector* vect;
	FILE* file;
	vector<const char*> select_stats;
	vector<const char*>* select_fields;
	// is there a previous feature to be finalized?
	bool previous;
	void* bandValues_buffer;
	double* bandValues;
	
	struct Pixel {
		int col;
		int row;
		Pixel(int col, int row) : col(col), row(row) {}
	};
	vector<Pixel> pixels;
	
	double* result_stats[TOT_RESULTS];
	bool compute[TOT_RESULTS];
	
	bool releaseStats;
	
	// last processed FID
	long last_FID;
	
		
	/**
	  * Creates a stats calculator
	  */
	StatsObserver(Traverser& tr, FILE* f, vector<const char*> select_stats,
		vector<const char*>* select_fields
	) : tr(tr), file(f), select_stats(select_stats), select_fields(select_fields)
	{
		vect = tr.getVector();
		global_info = 0;
		previous = false;
		bandValues_buffer = 0;
		bandValues = 0;
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			result_stats[i] = 0;
			compute[i] = false;
		}
		// by default, result_stats arrays (to be allocated in init())
		// get released in end():
		releaseStats = true;
		
		for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
			if ( 0 == strcmp(*stat, "avg") )
				compute[AVG] = true;
			else if ( 0 == strcmp(*stat, "stdev") )
				compute[STDEV] = true;
			else if ( 0 == strcmp(*stat, "min") )
				compute[MIN] = true;
			else if ( 0 == strcmp(*stat, "max") )
				compute[MAX] = true;
			else {
				cerr<< "Unrecognized stats " << *stat<< endl;
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
		finalizePreviousFeatureIfAny();
		if ( file ) {
			fclose(file);
			cout<< "Stats: finished" << endl;
			file = 0;
		}
		if ( bandValues_buffer ) {
			delete[] (double*) bandValues_buffer;
			bandValues_buffer = 0;
		}
		if ( bandValues ) {
			delete[] bandValues;
			bandValues = 0;
		}
		
		if ( releaseStats ) {
			for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
				if ( result_stats[i] ) {
					delete[] result_stats[i];
					result_stats[i] = 0;
				}
			}
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
			cerr<< "Couldn't fetch layer 0" << endl;
			exit(1);
		}

		//		
		// write column headers:
		//
		if ( file ) {
			// Create FID field
			fprintf(file, "FID");
	
			// Create fields:
			if ( select_fields ) {
				for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
					fprintf(file, ",%s", *fname);
				}
			}
			else {
				// all fields from layer definition
				OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
				int feature_field_count = poDefn->GetFieldCount();
				
				for ( int i = 0; i < feature_field_count; i++ ) {
					OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
					const char* pfield_name = poField->GetNameRef();
					fprintf(file, ",%s", pfield_name);
				}
			}
			
			
			// Create numPixels field
			fprintf(file, ",numPixels");
			
			// Create fields for bands
			for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
				for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
					fprintf(file, ",%s_Band%d", *stat, i+1);
				}
			}		
			fprintf(file, "\n");
		}
		
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
	  * Desired results are reported by finalizePreviousFeatureIfAny.
	  */
	void computeResults(void) {
		// initialize results:
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			for ( unsigned j = 0; j < global_info->bands.size(); j++ )
				result_stats[i][j] = 0.0;
		}

		for ( vector<Pixel>::const_iterator pixel = pixels.begin(); pixel != pixels.end(); pixel++ ) {
			// first get buffer with all bands
			tr.getBandValuesForPixel(pixel->col, pixel->row, bandValues_buffer);

			// now get those values in double format:
			char* ptr = (char*) bandValues_buffer;
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				GDALDataType bandType = global_info->bands[j]->GetRasterDataType();
				int typeSize = GDALGetDataTypeSize(bandType) >> 3;
				bandValues[j] = starspan_extract_double_value(bandType, ptr);
				
				// move to next piece of data in buffer:
				ptr += typeSize;
			}

			// cumulate:
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				result_stats[CUM][j] += bandValues[j]; 
			}

			// min and max:
			if ( pixel == pixels.begin() ) {    
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
		if ( pixels.size() > 0 ) {
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				result_stats[AVG][j] = result_stats[CUM][j] / pixels.size();
			}
		}
		
		// Is a second pass required?
		if ( compute[STDEV] ) {
			
			// standard deviation defined as sqrt of the sample variance.
			// So we need at least 2 pixels.
			
			if ( pixels.size() > 1 ) {
				// initialize CUM:
				for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
					result_stats[CUM][j] = 0.0;
				}
				
				// take pixels again
				for ( vector<Pixel>::const_iterator pixel = pixels.begin(); pixel != pixels.end(); pixel++ ) {
					// first get buffer with all bands
					tr.getBandValuesForPixel(pixel->col, pixel->row, bandValues_buffer);
		
					// now get those values in double format:
					char* ptr = (char*) bandValues_buffer;
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						GDALDataType bandType = global_info->bands[j]->GetRasterDataType();
						int typeSize = GDALGetDataTypeSize(bandType) >> 3;
						bandValues[j] = starspan_extract_double_value(bandType, ptr);
						
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
					double sd = sqrt(result_stats[CUM][j] / (pixels.size() - 1));
					result_stats[STDEV][j] = sd;
				}
			}
		}
	}

	/**
	  * dispatches finalization of previous feature
	  */
	void finalizePreviousFeatureIfAny(void) {
		if ( !previous )
			return;
		
		computeResults();
 
		if ( file ) {
			// Add numPixels value:
			fprintf(file, ",%d", pixels.size());
			
			// report desired results:
			// (desired list is traversed to keep order according to column headers)
			for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
				if ( 0 == strcmp(*stat, "avg") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%f", result_stats[AVG][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "stdev") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%f", result_stats[STDEV][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "min") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%f", result_stats[MIN][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "max") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						fprintf(file, ",%f", result_stats[MAX][j]);
					}
				}
				else {
					cerr<< "Unrecognized stats "<< *stat << endl;
					exit(1);
				}
			}
			fprintf(file, "\n");
		}
		pixels.clear();
		previous = false;
	}

	/**
	  * dispatches aggregation of current feature and prepares for next
	  */
	void intersectionFound(OGRFeature* feature) {
		finalizePreviousFeatureIfAny();
		
		//
		// start info for new feature:
		//
		
		if ( file ) {
			// Add FID value:
			fprintf(file, "%ld", feature->GetFID());
			last_FID = feature->GetFID();
	
			// add attribute fields from source feature to record:
			if ( select_fields ) {
				for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
					const int i = feature->GetFieldIndex(*fname);
					if ( i < 0 ) {
						cerr<< endl << "\tField `" <<*fname<< "' not found" << endl;
						exit(1);
					}
					const char* str = feature->GetFieldAsString(i);
					fprintf(file, ",%s", str);
				}
			}
			else {
				// all fields
				int feature_field_count = feature->GetFieldCount();
				for ( int i = 0; i < feature_field_count; i++ ) {
					const char* str = feature->GetFieldAsString(i);
					fprintf(file, ",%s", str);
				}
			}
		}
		
		// rest of record will be done by finalizePreviousFeatureIfAny
		previous = true;
	}
	
	
	/**
	  * Adds a new pixel to aggregation
	  */
	void addPixel(TraversalEvent& ev) {
		pixels.push_back(Pixel(ev.pixel.col, ev.pixel.row));
	}

};



/**
  * starspan_getStatsObserver: implementation
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	vector<const char*> select_stats,
	vector<const char*>* select_fields,
	const char* filename
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr<< "Couldn't create "<< filename << endl;
		return 0;
	}

	return new StatsObserver(tr, file, select_stats, select_fields);	
}
		

/**
  * starspan_getFeatureStats: implementation
  */
double** starspan_getFeatureStats(
	long FID, Vector* vect, Raster* rast,
	vector<const char*> select_stats
) {
	Traverser tr;
	tr.setVector(vect);
	tr.addRaster(rast);
	tr.setDesiredFID(FID);

	FILE* file = 0;
	vector<const char*> select_fields;
	StatsObserver* statsObs = new StatsObserver(tr, file, select_stats, &select_fields);
	if ( !statsObs )
		return 0;
	
	// I want to keep the resulting stats arrays for the client
	statsObs->releaseStats = false;
	
	tr.addObserver(statsObs);
	tr.traverse();
	
	// take results:
	double** result_stats = statsObs->result_stats;
	
	tr.releaseObservers();
	
	return result_stats;
}


double** starspan_getFeatureStatsByField(
	const char* field_name, 
	const char* field_value, 
	Vector* vect, Raster* rast,
	vector<const char*> select_stats,
	long *FID
) {
	Traverser tr;
	tr.setVector(vect);
	tr.addRaster(rast);
	tr.setDesiredFeatureByField(field_name, field_value);

	FILE* file = 0;
	vector<const char*> select_fields;
	StatsObserver* statsObs = new StatsObserver(tr, file, select_stats, &select_fields);
	if ( !statsObs )
		return 0;
	
	// I want to keep the resulting stats arrays for the client
	statsObs->releaseStats = false;
	
	tr.addObserver(statsObs);
	tr.traverse();
	
	// take results:
	double** result_stats = statsObs->result_stats;
	if ( FID )
		*FID = statsObs->last_FID;
	
	tr.releaseObservers();
	
	return result_stats;
}

