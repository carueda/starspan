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


void Stats::compute(vector<int>& values) {
	//
	// Note that stats that require only a first pass are always computed.
	//
	
	
	// initialize result:
	for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
		result[i] = 0.0;
	}
	
	const unsigned num_values = values.size();
	if ( num_values == 0 )
		return;
	
	result[MIN] = result[MAX] = values[0];

	for ( unsigned i = 0; i < num_values; i++ ) {
		const int value = values[i];

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
	if ( num_values > 0 ) {
		result[AVG] = result[SUM] / num_values;
	}

	if ( include[VAR] || include[STDEV] ) {
		
		// standard deviation defined as sqrt of the sample variance.
		// So we need at least 2 values
		
		if ( num_values > 1 ) {
			// initialize sum of square variances:
			double aux_CUM = 0.0;
			
			// take values again
			for ( unsigned i = 0; i < num_values; i++ ) {
				const int value = values[i];
	
				// cumulate square variance:
				double h = value - result[AVG];
				aux_CUM += h * h; 
			}
			
			// finally take var and stdev:
			result[VAR] = aux_CUM / (num_values - 1);
			result[STDEV] = sqrt(result[VAR]);
		}
	}


	if ( include[MODE] ) {
		map<int,int> count;
		
		// take values again
		for ( unsigned i = 0; i < num_values; i++ ) {
			const int value = values[i];
			map<int, int>::iterator it = count.find(value); 
			if ( it == count.end() )
				count.insert(map<int,int>::value_type(value, 1));
			else
				count[value]++;
		}
		int best_value;
		int best_count = 0;
		for ( map<int, int>::iterator it = count.begin(); it != count.end(); it++ ) {
			pair<int,int> p = *it;
			if ( it == count.begin()  ||  best_count < p.second ) {
				best_value = p.first;
				best_count = p.second;
			}
		}
		
		result[MODE] = best_value;
	}
}

void Stats::compute(vector<double>& values) {
	//
	// Note that stats that require only a first pass are always computed.
	//
	
	
	// initialize result:
	for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
		result[i] = 0.0;
	}
	
	const unsigned num_values = values.size();
	if ( num_values == 0 )
		return;
	
	result[MIN] = result[MAX] = values[0];
	
	for ( unsigned i = 0; i < num_values; i++ ) {
		const double value = values[i];

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
	if ( num_values > 0 ) {
		result[AVG] = result[SUM] / num_values;
	}

	if ( include[VAR] || include[STDEV] ) {
		
		// standard deviation defined as sqrt of the sample variance.
		// So we need at least 2 values
		
		if ( num_values > 1 ) {
			// initialize sum of square variances:
			double aux_CUM = 0.0;
			
			// take values again
			for ( unsigned i = 0; i < num_values; i++ ) {
				const double value = values[i];
	
				// cumulate square variance:
				double h = value - result[AVG];
				aux_CUM += h * h; 
			}
			
			// finally take var and stdev:
			result[VAR] = aux_CUM / (num_values - 1);
			result[STDEV] = sqrt(result[VAR]);
		}
	}


	if ( include[MODE] ) {
		map<string,int> count;
		
		// take values again
		for ( unsigned i = 0; i < num_values; i++ ) {
			const double value = values[i];
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


