//
//	Stats - basic stats calculator
//	$Id$
//	See Stats.h for public doc.
//

#include "Stats.h"

#include <string>
#include <map>
#include <cstdio>
#include <cstdlib>  // atof
#include <cmath>  // sqrt


void Stats::compute(vector<double>* values) {
	//
	// Note that stats that require only a first pass are always computed.
	//
	
	
	// initialize result:
	for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
		result[i] = 0.0;
	}
	
	if ( values->size() == 0 )
		return;
	
	result[MIN] = result[MAX] = (*values)[0];
	
	for ( vector<double>::const_iterator pval = values->begin(); pval != values->end(); pval++ ) {
		double value = *pval;

		// cumulate
		result[SUM] += value;
		
		// min and max:
		// min
		if ( result[MIN] > value ) 
			result[MIN] = value; 
		
		// max
		if ( result[MAX] < value ) 
			result[MAX] = value; 
	}
	
	// average
	if ( values->size() > 0 ) {
		result[AVG] = result[SUM] / values->size();
	}

	if ( include[VAR] || include[STDEV] ) {
		
		// standard deviation defined as sqrt of the sample variance.
		// So we need at least 2 values
		
		if ( values->size() > 1 ) {
			// initialize sum of square variances:
			double aux_CUM = 0.0;
			
			// take values again
			for ( vector<double>::const_iterator pval = values->begin(); pval != values->end(); pval++ ) {
				double value = *pval;
	
				// cumulate square variance:
				double h = value - result[AVG];
				aux_CUM += h * h; 
			}
			
			// finally take var and stdev:
			result[VAR] = aux_CUM / (values->size() - 1);
			result[STDEV] = sqrt(result[VAR]);
		}
	}


	if ( include[MODE] ) {
		map<string,int> count;
		
		// take values again
		for ( vector<double>::const_iterator pval = values->begin(); pval != values->end(); pval++ ) {
			double value = *pval;
			char str[1024];
			sprintf(str, "%.3f", value);
			string key = str;
			map<string, int>::iterator it = count.find(str); 
			if ( it == count.end() )
				count.insert(map<string,int>::value_type(str, 1));
			else
				count[str]++;
		}
		string best_str;
		int best_count = 0;
		for ( map<string, int>::iterator it = count.begin(); it != count.end(); it++ ) {
			pair<string,int> p = *it;
			if ( it == count.begin()  ||  best_count < p.second ) {
				best_str = p.first;
				best_count = p.second;
			}
		}
		
		result[MODE] = atof(best_str.c_str());
	}
}


