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
		"      -vector <filename>       An OGR recognized vector file.\n"
		"      -raster <filenames>...   GDAL recognized raster files.\n"
		"\n"
		"   commands:\n"
		"      -report              Shows info about given input files\n"
		"      -dbf <name>          Generates a DBF file\n"
		"      -csv <name>          Generates a CSV file\n"
		"      -envi <name>         Generates an ENVI image\n"
		"      -envisl <name>       Generates an ENVI spectral library\n"
		"      -mr <prefix>         Generates mini rasters\n"
		"      -jtstest <filename>  Generates a JTS test file\n"
		"\n"
		"   options:\n"
		"      -fields field1,field2,...,fieldn\n"
		"      -pixprop <pixel-proportion-value>\n"
		"      -fid <FID>\n"
		"      -ppoly\n"
		"      -in\n"
		"      -srs <srs>\n"
		"\n"
		"Example:\n"
		"   starspan -vector V.shp  -raster R.img  -dbf D.dbf  -fields species,foo,baz\n"
		"\n"
		" creates D.dbf with pixels extracted from R.img that intersect\n"
		" features in V.shp; only the given fields from V.shp are written to D.dbf.\n"
		"\n"
		"More details at http://starspan.casil.ucdavis.edu/?Usage\n"
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
	const char*  select_fields = NULL;
	const char*  dbf_name = NULL;
	const char*  csv_name = NULL;
	const char*  mini_prefix = NULL;
	const char*  mini_srs = NULL;
	const char* jtstest_filename = NULL;
	bool update = false;
	
	bool an_output_given = false;
	
	const char* vector_filename = 0;
	vector<const char*> raster_filenames;
	double pix_prop = -1.0;
	long FID = -1;

	//
	// collect arguments  -- FIXME: use getopt later on
	//
	for ( int i = 1; i < argc; i++ ) {
		
		// INPUTS:
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
			if ( argv[i][0] == '-' ) 
				--i;
		}
		
		// COMMANDS
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
		
		// OPTIONS
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
		
		else if ( 0==strcmp("-ppoly", argv[i]) ) {
			use_polys = true;
		}
		
		else if ( 0==strcmp("-fields", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("-fields: which fields to select?");
			select_fields = argv[i];
		}

		else if ( 0==strcmp("-update", argv[i]) ) {
			update = true;
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
	
	// COMMANDS
	if ( csv_name ) { 
		return starspan_csv(tr, select_fields, csv_name);
	}
	else if ( dbf_name ) { 
		return starspan_db(tr, select_fields, dbf_name);
	}
	else if ( envi_name ) {
		return starspan_gen_envisl(tr, select_fields, envi_name, envi_image);
	}
	else if ( mini_prefix ) {
		return starspan_minirasters(tr, mini_prefix, only_in_feature, mini_srs);
	}
	else if ( jtstest_filename ) {
		starspan_jtstest(tr, use_polys, jtstest_filename);
	}
	else if ( do_report ) {
		if ( tr.getNumRasters() == 0 && !tr.getVector() ) {
			usage("-report: Please give at least one input file to report\n");
		}
		starspan_report(tr);
	}
	else {
		usage("what should I do?\n");
	}
	
	return 0;
}

