//
// STARSpan project
// Carlos A. Rueda
// starspan_tuct2 - second version (based on tuct1) according to new scheme
// $Id$
//


#include "starspan.h"           

#include "Csv.h"       
#include <fstream>       
#include <cstdlib>
#include <cassert>

/**
  * implementation
  */
int starspan_tuct_2(
	const char* vector_filename,
	vector<const char*> raster_filenames,
	const char* speclib_filename,
	double pix_prop,
	const char* link_name,
	vector<const char*> select_stats,
	const char* calbase_filename
) {
	// open input file
	ifstream speclib_file(speclib_filename);
	if ( !speclib_file ) {
		cerr<< "Couldn't open " << speclib_filename << endl;
		return 1;
	}
	
	// create output file
	ofstream calbase_file(calbase_filename, ios::out);
	if ( !calbase_file ) {
		speclib_file.close();
		cerr<< "Couldn't create " << calbase_filename << endl;
		return 1;
	}

	// open vector datasource:
	Vector* vect = new Vector(vector_filename);

	//
	// Let's get to work
	//
	
	int ret = 0;   // for return value.
	
	cout << "processing ..." << endl;
	
	//
	// write header:
	//
	calbase_file<< "FID," <<link_name<< ",RID,BandNumber,FieldBandValue";
	for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
		calbase_file<< "," << *stat << "_ImageBandValue";
	}
	calbase_file<< endl;
	
	//
	// for each raster...
	//	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		const char* raster_filename = raster_filenames[i];
		Raster* rast = new Raster(raster_filename);

		int bands;
		rast->getSize(NULL, NULL, &bands);

		//
		// reset speclib input
		// (repeated for each image, but doesn't matter)
		//
		speclib_file.seekg(0);
		Csv csv(speclib_file);
		string line;
		if ( !csv.getline(line) ) {
			speclib_file.close();
			cerr<< "Couldn't get header line from " << speclib_filename << endl;
			ret = 1;
			break;
		}
		const unsigned num_speclib_fields = csv.getnfield();
		if ( num_speclib_fields < 2 
		||   csv.getfield(0) != link_name ) {
			speclib_file.close();
			cerr<< "Unexpected format: " << speclib_filename << endl;
			cerr<< "There bust be more than one field and the "
			    << "first one is expected to be named " << link_name << endl;
			ret = 1;
			break;
		}
		
		if ( num_speclib_fields-1 != (unsigned) bands ) {
			speclib_file.close();
			cerr<< "Different number of bands:" << endl
			    << "  " << speclib_filename << ": " << (num_speclib_fields-1) << endl
			    << "  " << raster_filename << ": " << bands << endl
			;
			ret = 1;
			break;
		}
				
		//
		// for each feature
		//
		cout << "processing features..." << endl;
		for ( int record = 0; csv.getline(line); record++ ) {
			string link_val = csv.getfield(0);

			// progress message
			cout << "\n\t processing " <<link_name<< ": " << link_val << endl;
			
			// get stats for link_val
			long FID;
			double** stats = starspan_getFeatureStatsByField(
				link_name, link_val.c_str(), 
				vect, rast,
				pix_prop,
				select_stats,
				&FID
			); 
			if ( stats ) {
				if ( FID >= 0 ) {
					for ( int bandNumber = 1; bandNumber <= bands; bandNumber++ ) {
						double fieldBandValue = atof(csv.getfield(bandNumber).c_str());

						// write record
						calbase_file 
							<< FID             <<","
							<< link_val        <<","
							<< raster_filename <<","
							<< bandNumber      <<","
							<< fieldBandValue
						;
						
						for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
							double imageBandValue = 0.0;
							if ( 0 == strcmp(*stat, "avg") )
								imageBandValue = stats[AVG][bandNumber-1];
							else if ( 0 == strcmp(*stat, "mode") )
								imageBandValue = stats[MODE][bandNumber-1];
							else if ( 0 == strcmp(*stat, "stdev") )
								imageBandValue = stats[STDEV][bandNumber-1];
							else if ( 0 == strcmp(*stat, "min") )
								imageBandValue = stats[MIN][bandNumber-1];
							else if ( 0 == strcmp(*stat, "max") )
								imageBandValue = stats[MAX][bandNumber-1];
							
							calbase_file<< "," << imageBandValue;
						}
						calbase_file<< endl;
					}
	
				}
				// release stats:
				for ( unsigned i = 0; i < TOT_RESULTS; i++ )
					delete[] stats[i];
			}
		}
		delete rast;
	}
	delete vect;

	calbase_file.close();    
	speclib_file.close();
	cout << "finished." << endl;
	return ret;	
}
		


