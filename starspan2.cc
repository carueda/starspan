//
// STARSpan project
// Carlos A. Rueda
// starspan2 
// $Id$
//

#include "starspan.h"           
#include "traverser.h"           

#include <stdlib.h>


//static char rcsid[] = "$Id$";


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
		"http://starspan.casil.ucdavis.edu\n"
		"\n"
		"USAGE:\n"
		"  starspan <inputs/commands/options>...\n"
		"\n"
		"   inputs:\n"
		"      -raster <filenames>...   GDAL recognized raster files.\n"
		"      -vector <filename>       An OGR recognized vector file.\n"
		"      -speclib <filename>      A CSV file with scheme: ID,band1,band2,...,bandN\n"
		"      -update-csv <filename>   A CSV file with pixel locations.\n"
		"      -update-dbf <filename>   A DBF file with pixel locations.\n"
		"\n"
		"   commands:\n"
		"      -report              Show info about given input files\n"
		"      -dbf <name>          Generate a DBF file\n"
		"      -csv <name>          Generate a CSV file\n"
		"      -envi <name>         Generate an ENVI image\n"
		"      -envisl <name>       Generate an ENVI spectral library\n"
		"      -stats outfile.csv [avg|stdev|min|max]...\n"
		"                           Compute statistics\n"
		"      -dump_geometries <filename>  Save geometries for vizualization\n"
		"      -calbase <link> <filename>  Create base table for calibration\n"
		"      -mr <prefix>         Generate mini rasters\n"
		"      -jtstest <filename>  Generate a JTS test file\n"
		"\n"
		"   options:\n"
		"      -fields field1,...,fieldn\n"
		"                   Only these fields from vector.  - for none\n"
		"      -pixprop <minimum-pixel-proportion>\n"
		"      -noColRow    do not include (col,row) fields\n"
		"      -noXY        do not include (x,y) fields\n"
		"      -fid <FID>   only this FID\n"
		"      -ppoly       write pixels as rectangles for visualization\n"
		"      -in          only pixels contained in polygons\n"
		"      -srs <srs>\n"
		"\n"
		"Example:\n"
		"   starspan -vector V.shp  -raster R.img  -csv D.csv  -fields species,foo,baz\n"
		" creates D.csv with pixels extracted from R.img that intersect\n"
		" features in V.shp; only the given fields from V.shp are written to D.csv.\n"
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
	
	bool do_report = false;
	bool only_in_feature = false;
	bool use_polys = false;
	const char*  envi_name = NULL;
	bool envi_image = true;
	vector<const char*>* select_fields = NULL;
	const char*  dbf_name = NULL;
	const char*  update_dbf_name = NULL;
	const char*  csv_name = NULL;
	const char*  stats_name = NULL;
	vector<const char*> select_stats;
	const char*  update_csv_name = NULL;
	const char*  mini_prefix = NULL;
	const char*  mini_srs = NULL;
	const char* jtstest_filename = NULL;
	bool noColRow = false;
	bool noXY = false;
	
	bool an_output_given = false;
	
	const char* vector_filename = 0;
	vector<const char*> raster_filenames;
	
	const char* speclib_filename = 0;
	const char* callink_name = 0;
	const char* calbase_filename = 0;
	
	double pix_prop = -1.0;
	long FID = -1;
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
			speclib_filename = argv[i];
		}
		
		else if ( 0==strcmp("-update-csv", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-update-csv: which CSV file?");
			update_csv_name = argv[i];
		}
		
		else if ( 0==strcmp("-update-dbf", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-update-dbf: which DBF file?");
			update_dbf_name = argv[i];
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
		}
		
		else if ( 0==strcmp("-dbf", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-dbf: which name?");
			if ( an_output_given )
				usage("Only one output format, please\n");
			an_output_given = true;
			dbf_name = argv[i];
		}
		
		else if ( 0==strcmp("-csv", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-csv: which name?");
			if ( an_output_given )
				usage("Only one output format, please\n");
			an_output_given = true;
			csv_name = argv[i];
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
			if ( an_output_given )
				usage("Only one output format, please\n");
			an_output_given = true;
			mini_prefix = argv[i];
		}
		
		else if ( 0==strcmp("-envi", argv[i]) || 0==strcmp("-envisl", argv[i]) ) {
			envi_image = 0==strcmp("-envi", argv[i]);
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-envi, -envisl: which base name?");
			if ( an_output_given )
				usage("Only one output format, please\n");
			an_output_given = true;
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
			if ( an_output_given )
				usage("Only one output format, please\n");
			an_output_given = true;
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
			pix_prop = atof(argv[i]);
			if ( pix_prop < 0.0 || pix_prop > 1.0 )
				usage("invalid pixel proportion");
		}
		
		else if ( 0==strcmp("-fid", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-fid: desired FID?");
			FID = atol(argv[i]);
			if ( FID < 0 )
				usage("invalid FID");
		}
		
		else if ( 0==strcmp("-noColRow", argv[i]) ) {
			noColRow = true;
		}
		
		else if ( 0==strcmp("-noXY", argv[i]) ) {
			noXY = true;
		}
		
		else if ( 0==strcmp("-ppoly", argv[i]) ) {
			use_polys = true;
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
			only_in_feature = true;
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

	// module initialization
	Raster::init();
	Vector::init();

	
	//
	// dispatch commands that don't follow traverser pattern:
	//
	if ( calbase_filename ) {
		if ( !vector_filename ) {
			usage("-calbase expects a vector input (use -vector)");
		}
		if ( raster_filenames.size() == 0 ) {
			usage("-calbase requires at least a raster input (use -raster)");
		}
		if ( !speclib_filename ) {
			usage("-calbase expects a speclib input (use -speclib)");
		}
		return starspan_tuct_2(
			vector_filename,  
			raster_filenames,
			speclib_filename,
			callink_name,
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
		return starspan_update_csv(update_csv_name, raster_filenames, csv_name);
	}
	else if ( update_dbf_name ) {
		if ( !dbf_name ) {
			usage("-update-dbf works with -dbf. Please specify an existing DBF");
		}
		if ( vector_filename ) {
			usage("-update-dbf does not expect a vector input");
		}
		if ( raster_filenames.size() == 0 ) {
			usage("-update-dbf requires at least a raster input");
		}
		return starspan_update_dbf(update_dbf_name, raster_filenames, dbf_name);
	}
	
	//
	// traverser-based commands
	//
	
	// the traverser object	
	Traverser tr;

	if ( vector_filename )
		tr.setVector(new Vector(vector_filename));
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		tr.addRaster(new Raster(raster_filenames[i]));
	}


	if ( pix_prop >= 0.0 )
		tr.setPixelProportion(pix_prop);

	if ( FID >= 0 )
		tr.setDesiredFID(FID);

	if ( dbf_name || csv_name || envi_name || mini_prefix || jtstest_filename) { 
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
	
	if ( csv_name ) { 
		Observer* obs = starspan_csv(tr, select_fields, csv_name, noColRow, noXY);
		if ( obs )
			tr.addObserver(obs);
	}
	
	if ( dbf_name ) { 
		Observer* obs = starspan_db(tr, select_fields, dbf_name, noColRow, noXY);
		if ( obs )
			tr.addObserver(obs);
	}
	
	if ( envi_name ) {
		Observer* obs = starspan_gen_envisl(tr, select_fields, envi_name, envi_image);
		if ( obs )
			tr.addObserver(obs);
	}
	
	if ( dump_geometries_filename ) {
		Observer* obs = starspan_dump(tr, use_polys, dump_geometries_filename);
		if ( obs )
			tr.addObserver(obs);
	}

	if ( jtstest_filename ) {
		Observer* obs = starspan_jtstest(tr, use_polys, jtstest_filename);
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
		starspan_minirasters(tr, mini_prefix, only_in_feature, mini_srs);
	}
	
	//
	// now observer-based processing:
	//
	
	if ( tr.getNumObservers() > 0 ) {
		// let's get to work!	
		tr.traverse();
	
		// release observers:
		tr.releaseObservers();
	}
	else {
		if ( !do_report && !mini_prefix )
			usage("please specify a command\n");
	}
	
	return 0;
}

