//
// STARSpan project
// Carlos A. Rueda
// starspan_tuct1 - generates a CSV file
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
int starspan_getTuct1Observer(
	const char* vector_filename,
	vector<const char*> raster_filenames,
	const char* speclib_filename,
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
	calbase_file << "FID,RID,BandNumber,FieldBandValue,ImageBandValue" << endl;
	
	vector<const char*> select_stats(1, "avg");
	
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
		||   csv.getfield(0) != "FID" ) {
			speclib_file.close();
			cerr<< "Unexpected format: " << speclib_filename << endl;
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
			string sFID = csv.getfield(0);
			long FID = atol(sFID.c_str());
			
			// progress message
			cout << "\tFID " << FID << endl;
			
			// get stats for FID
			double** stats = starspan_getFeatureStats(
				FID, vect, rast, select_stats
			); 

			for ( int bandNumber = 1; bandNumber <= bands; bandNumber++ ) {
				double fieldBandValue = atof(csv.getfield(0).c_str());
				double imageBandValue = stats[AVG][bandNumber-1];


				// write record
				calbase_file 
					<< FID             <<","
					<< raster_filename <<","
					<< bandNumber      <<","
					<< fieldBandValue  <<","
					<< imageBandValue  << endl
				;

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
		


