//
// STARSpan project
// Carlos A. Rueda
// starspan2 
// $Id$
//

#include "starspan.h"           
#include "traverser.h"           

#include <cstdlib>
#include <ctime>


//static char rcsid[] = "$Id$";


GlobalOptions globalOptions;


// prints a help message
static void usage(const char* msg) {
	if ( msg ) { 
		fprintf(stderr, "starspan: %s\n", msg); 
		fprintf(stderr, "Type `starspan -help' for help\n");
		exit(1);
	}
	fprintf(stdout, 
		"\n"
		"starspan %s (%s %s)    UNDER DEVELOPMENT\n"
		"\n"
		"USAGE:\n"
		"  starspan <inputs/commands/options>...\n"
		"\n"
		"   inputs:\n"
		"      -raster <filenames>...\n"
		"      -vector <filename>\n"
		"      -speclib <filename>\n"
		"      -update-csv <filename>\n"
		"\n"
		"   commands:\n"
		"      -raster_field <name>\n"
		"      -raster_directory <directory>\n"
		"      -csv <name>\n"
		"      -envi <name>\n"
		"      -envisl <name> \n"
		"      -stats outfile.csv [avg|mode|stdev|min|max]...\n"
		"      -calbase <link> <filename> [<stats>...]\n"
		"      -report \n"
		"      -dump_geometries <filename>\n"
		"      -mr <prefix> \n"
		"      -jtstest <filename>\n"
		"\n"
		"   options:\n"
		"      -fields field1 field2 ... fieldn\n"
		"      -pixprop <minimum-pixel-proportion>\n"
		"      -noColRow \n"
		"      -noXY\n"
		"      -fid <FID>\n"
		"      -skip_invalid_polys \n"
		"      -progress [value] \n"
		"      -RID_as_given \n"
		"      -verbose \n"
		"      -ppoly \n"
		"      -in   \n"
		"      -srs <srs>\n"
		"\n"
		"Visit http://starspan.casil.ucdavis.edu/?Usage for more details.\n"
		"\n"
		, STARSPAN_VERSION, __DATE__, __TIME__
	);
	exit(0);
}

///////////////////////////////////////////////////////////////
// main test program
int main(int argc, char ** argv) {
	if ( argc == 1 || 0==strcmp("-help", argv[1]) ) {
		usage(NULL);
	}
	
	globalOptions.use_pixpolys = false;
	globalOptions.skip_invalid_polys = false;
	globalOptions.pix_prop = -1.0;
	globalOptions.FID = -1;
	globalOptions.verbose = false;
	globalOptions.progress = false;
	globalOptions.progress_perc = 1;
	globalOptions.noColRow = false;
	globalOptions.noXY = false;
	globalOptions.only_in_feature = false;
	globalOptions.RID_as_given = false;
	globalOptions.report_summary = true;
	
	
	bool report_elapsed_time = true;
	bool do_report = false;
	const char*  envi_name = NULL;
	bool envi_image = true;
	vector<const char*>* select_fields = NULL;
	const char*  csv_name = NULL;
	const char*  stats_name = NULL;
	vector<const char*> select_stats;
	const char*  update_csv_name = NULL;
	const char*  mini_prefix = NULL;
	const char*  mini_srs = NULL;
	const char* jtstest_filename = NULL;
	
	const char* vector_filename = 0;
	vector<const char*> raster_filenames;
	
	const char* speclib_filename = 0;
	const char* callink_name = 0;
	const char* calbase_filename = 0;
	
	const char* raster_field_name = 0;
	const char* raster_directory = ".";

	const char* dump_geometries_filename = NULL;
	
	//
	// collect arguments  -- TODO: use getopt later on
	//
	for ( int i = 1; i < argc; i++ ) {
		
		//
		// INPUTS:
		//
		if ( 0==strcmp("-vector", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-vector: which vector file?");
			if ( vector_filename )
				usage("-vector specified twice");
			vector_filename = argv[i];
		}
		else if ( 0==strcmp("-raster", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				raster_filenames.push_back(argv[i]);
			}
			if ( raster_filenames.size() == 0 )
				usage("-raster: which raster files?");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
		else if ( 0==strcmp("-speclib", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-speclib: which CSV file?");
			if ( speclib_filename )
				usage("-speclib specified twice");
			speclib_filename = argv[i];
		}
		
		else if ( 0==strcmp("-update-csv", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-update-csv: which CSV file?");
			if ( update_csv_name )
				usage("-update-csv specified twice");
			update_csv_name = argv[i];
		}
		
		//
		// COMMANDS
		//
		else if ( 0==strcmp("-calbase", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-calbase: which field to use as link?");
			callink_name = argv[i];
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-calbase: which output file name?");
			calbase_filename = argv[i];
			while ( ++i < argc && argv[i][0] != '-' ) {
				select_stats.push_back(argv[i]);
			}
			if ( select_stats.size() == 0 )
				select_stats.push_back("avg");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
		
		else if ( 0==strcmp("-csv", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-csv: which name?");
			csv_name = argv[i];
		}
		
		else if ( 0==strcmp("-raster_field", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-raster_field: which field name?");
			raster_field_name = argv[i];
		}
		
		else if ( 0==strcmp("-raster_directory", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-raster_directory: which raster directory?");
			raster_directory = argv[i];
		}
		
		else if ( 0==strcmp("-stats", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-stats: which output name?");
			stats_name = argv[i];
			while ( ++i < argc && argv[i][0] != '-' ) {
				select_stats.push_back(argv[i]);
			}
			if ( select_stats.size() == 0 )
				usage("-stats: which statistics?");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}

		else if ( 0==strcmp("-mr", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-mr: which prefix?");
			mini_prefix = argv[i];
		}
		
		else if ( 0==strcmp("-envi", argv[i]) || 0==strcmp("-envisl", argv[i]) ) {
			envi_image = 0==strcmp("-envi", argv[i]);
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-envi, -envisl: which base name?");
			envi_name = argv[i];
		}
		
		else if ( 0==strcmp("-dump_geometries", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-dump_geometries: which name?");
			dump_geometries_filename = argv[i];
		}
		
		else if ( 0==strcmp("-jtstest", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-jtstest: which JTS test file name?");
			jtstest_filename = argv[i];
		}
		else if ( 0==strcmp("-report", argv[i]) ) {
			do_report = true;
		}
		
		//
		// OPTIONS
		//                                                        
		else if ( 0==strcmp("-pixprop", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-pixprop: pixel proportion?");
			globalOptions.pix_prop = atof(argv[i]);
			if ( globalOptions.pix_prop < 0.0 || globalOptions.pix_prop > 1.0 )
				usage("invalid pixel proportion");
		}
		
		else if ( 0==strcmp("-fid", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-fid: desired FID?");
			globalOptions.FID = atol(argv[i]);
			if ( globalOptions.FID < 0 )
				usage("invalid FID");
		}
		
		else if ( 0==strcmp("-noColRow", argv[i]) ) {
			globalOptions.noColRow = true;
		}
		
		else if ( 0==strcmp("-noXY", argv[i]) ) {
			globalOptions.noXY = true;
		}
		
		else if ( 0==strcmp("-ppoly", argv[i]) ) {
			globalOptions.use_pixpolys = true;
		}
		
		else if ( 0==strcmp("-skip_invalid_polys", argv[i]) ) {
			globalOptions.skip_invalid_polys = true;
		}
		
		else if ( 0==strcmp("-fields", argv[i]) ) {
			if ( !select_fields )
				select_fields = new vector<const char*>();
			while ( ++i < argc && argv[i][0] != '-' ) {
				select_fields->push_back(argv[i]);
			}
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}

		else if ( 0==strcmp("-in", argv[i]) ) {
			globalOptions.only_in_feature = true;
		}
		
		else if ( 0==strcmp("-RID_as_given", argv[i]) ) {
			globalOptions.RID_as_given = true;
		}
		
		else if ( 0==strcmp("-progress", argv[i]) ) {
			if ( i+1 < argc && argv[i+1][0] != '-' )
				globalOptions.progress_perc = atof(argv[++i]);
			globalOptions.progress = true;
		}
		
		else if ( 0==strcmp("-verbose", argv[i]) ) {
			globalOptions.verbose = true;
		}
		
		else if ( 0==strcmp("-srs", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-srs: which srs?");
			mini_srs = argv[i];
		}

		// HELP
		else if ( 0==strcmp("-help", argv[i]) ) {
			usage(NULL);
		}
		else {
			usage("invalid arguments");
		}
	}
	
	CPLPushErrorHandler(starspan_myErrorHandler);

	time_t time_start = time(NULL);
	
	// module initialization
	Raster::init();
	Vector::init();

	int res = 0;
	
	//
	// dispatch commands that don't follow traverser pattern:
	//
	if ( csv_name ) { 
		if ( !vector_filename ) {
			usage("-csv expects a vector input (use -vector)");
		}
		if ( raster_field_name && raster_filenames.size() > 0 ) {
			usage("Only one of -raster_field or -raster please");
		}
		if ( raster_field_name ) {
			res = starspan_csv_raster_field(
				vector_filename,  
				raster_field_name,
				raster_directory,
				select_fields, 
				csv_name
			);
		}
		else if ( raster_filenames.size() > 0 ) {
			res = starspan_csv(
				vector_filename,  
				raster_filenames,
				select_fields, 
				csv_name
			);
		}
		else {
			usage("-csv expects at least a raster input (use -raster)");
		}
	}
	else if ( calbase_filename ) {
		if ( !vector_filename ) {
			usage("-calbase expects a vector input (use -vector)");
		}
		if ( raster_filenames.size() == 0 ) {
			usage("-calbase requires at least a raster input (use -raster)");
		}
		if ( !speclib_filename ) {
			usage("-calbase expects a speclib input (use -speclib)");
		}
		res = starspan_tuct_2(
			vector_filename,  
			raster_filenames,
			speclib_filename,
			callink_name,
			select_stats,
			calbase_filename
		);
	}
	else if ( update_csv_name ) {
		if ( !csv_name ) {
			usage("-update-csv works with -csv. Please specify an existing CSV");
		}
		if ( vector_filename ) {
			usage("-update-csv does not expect a vector input");
		}
		if ( raster_filenames.size() == 0 ) {
			usage("-update-csv requires at least a raster input");
		}
		res = starspan_update_csv(update_csv_name, raster_filenames, csv_name);
	}
	else {
		//
		// traverser-based commands
		//
		
		// the traverser object	
		Traverser tr;
	
		if ( globalOptions.pix_prop >= 0.0 )
			tr.setPixelProportion(globalOptions.pix_prop);
	
		if ( globalOptions.FID >= 0 )
			tr.setDesiredFID(globalOptions.FID);
		
		tr.setVerbose(globalOptions.verbose);
		if ( globalOptions.progress )
			tr.setProgress(globalOptions.progress_perc, cout);
	
		tr.setSkipInvalidPolygons(globalOptions.skip_invalid_polys);
	
		if ( vector_filename )
			tr.setVector(new Vector(vector_filename));
		
		for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {    
			tr.addRaster(new Raster(raster_filenames[i]));
		}
	
	
		if ( csv_name || envi_name || mini_prefix || jtstest_filename) { 
			if ( tr.getNumRasters() == 0 || !tr.getVector() ) {
				usage("Specified output option requires both a raster and a vector to proceed\n");
			}
		}
	
	
		//	
		// COMMANDS
		//
		
		if ( stats_name ) {
			Observer* obs = starspan_getStatsObserver(tr, select_stats, select_fields, stats_name);
			if ( obs )
				tr.addObserver(obs);
		}
		
		if ( envi_name ) {
			Observer* obs = starspan_gen_envisl(tr, select_fields, envi_name, envi_image);
			if ( obs )
				tr.addObserver(obs);
		}
		
		if ( dump_geometries_filename ) {
			if ( tr.getNumRasters() == 0 && tr.getVector() && globalOptions.FID >= 0 ) {
				dumpFeature(tr.getVector(), globalOptions.FID, dump_geometries_filename);
			}
			else {
				Observer* obs = starspan_dump(tr, dump_geometries_filename);
				if ( obs )
					tr.addObserver(obs);
			}
		}
	
		if ( jtstest_filename ) {
			Observer* obs = starspan_jtstest(tr, jtstest_filename);
			if ( obs )
				tr.addObserver(obs);
		}
	
	
		// report and minirasters don't use Observer scheme;  
		// just call corresponding functions:	
		if ( do_report ) {
			if ( tr.getNumRasters() == 0 && !tr.getVector() ) {
				usage("-report: Please give at least one input file to report\n");
			}
			starspan_report(tr);
		}
		if ( mini_prefix ) {
			starspan_minirasters(tr, mini_prefix, mini_srs);
		}
		
		//
		// now observer-based processing:
		//
		
		if ( tr.getNumObservers() > 0 ) {
			// let's get to work!	
			tr.traverse();
	
			if ( globalOptions.report_summary ) {
				tr.reportSummary();
			}
			
			// release observers:
			tr.releaseObservers();
		}
	}
	
	Vector::end();
	Raster::end();
	
	if ( report_elapsed_time ) {
		cout<< "Elapsed time: ";
		// report elapsed time:
		time_t secs = time(NULL) - time_start;
		if ( secs < 60 )
			cout<< secs << "s\n";
		else {
			time_t mins = secs / 60; 
			secs = secs % 60; 
			if ( mins < 60 ) 
				cout<< mins << "m:" << secs << "s\n";
			else {
				time_t hours = mins / 60; 
				mins = mins % 60; 
				if ( hours < 24 ) 
					cout<< hours << "h:" << mins << "m:" << secs << "s\n";
				else {
					time_t days = hours / 24; 
					hours = hours % 24; 
					cout<< days << "d:" <<hours << "h:" << mins << "m:" << secs << "s\n";
				}
			}
		}
	}

	return res;
}

