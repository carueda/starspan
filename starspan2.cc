//
// STARSpan project
// Carlos A. Rueda
// starspan2 
// $Id$
//

#include "starspan.h"           
#include "traverser.h"           

#include <stdlib.h>

#define VERSION "0.4"


static char rcsid[] = "$Id$";


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
		"  starspan -help\n"
		"      prints this message and exits.\n"
		"\n"
		"  starspan -xhelp\n"
		"      prints help for extended usage and exits.\n"
		"\n"
		"  starspan <inputs/commands/options>...\n"
		"\n"
		"   inputs:\n"
		"      -vector <filename>   An OGR recognized vector file.\n"
		"      -raster <filename>   A GDAL recognized raster file.\n"
		"\n"
		"   commands:\n"
		"      -report              Shows info about given input files\n"
		"      -db <name>           Generates a DBF file\n"
		"      -envisl <name>       Generates an ENVI spectral library\n"
		"      -mr <prefix>         Generates mini rasters\n"
		"      -jtstest <filename>  Generates a JTS test file\n"
		"\n"
		"   options:\n"
		"      -pixprop <value>\n"
		"              Minimum proportion of pixel area in intersection so that\n"
		"              the pixel is included.  <value> in [0.0, 1.0].\n"
		"              Only used in intersections resulting in polygons.\n"
		"              By default, the pixel is included only if the polygon \n"
		"              contains the upper left corner of the pixel.\n"
		"      -in     (Used with -mr)\n"
		"              Only pixels contained in geometry features are retained.\n"
		"              Zero (0) is used to nullify pixels outside features.\n"
		"              By default all pixels in envelope are retained.\n"
		"      -srs <srs>   (Used with -mr)\n"
		"              See gdal_translate option -a_srs.\n"
		"              By default projection is taken from input raster.\n"
		"\n"
		, VERSION, __DATE__, __TIME__
	);
	exit(0);
}

///////////////////////////////////////////////////////////////
// main test program
int main(int argc, char ** argv) {
	if ( argc == 1 || 0==strcmp("-help", argv[1]) ) {
		usage(NULL);
	}
	
	const char* raster_filename = NULL;
	const char* vector_filename = NULL;
	bool do_report = false;
	bool only_in_feature = false;
	const char*  envisl_name = NULL;
	const char*  db_name = NULL;
	const char*  mini_prefix = NULL;
	const char*  mini_srs = NULL;
	const char* jtstest_filename = NULL;
	
	for ( int i = 1; i < argc; i++ ) {
		
		// INPUTS:
		if ( 0==strcmp("-vector", argv[i]) ) {
			if ( ++i == argc )
				usage("-vector: which vector file?");
			vector_filename = argv[i];
			
		}
		else if ( 0==strcmp("-raster", argv[i]) ) {
			if ( ++i == argc )
				usage("-raster: which raster file?");
			raster_filename = argv[i];
		}
		
		// COMMANDS
		else if ( 0==strcmp("-db", argv[i]) ) {
			if ( ++i == argc )
				usage("-db: which name?");
			db_name = argv[i];
		}
		else if ( 0==strcmp("-mr", argv[i]) ) {
			if ( ++i == argc )
				usage("-mr: which prefix?");
			mini_prefix = argv[i];
		}
		else if ( 0==strcmp("-envisl", argv[i]) ) {
			if ( ++i == argc )
				usage("-envisl: which base name?");
			envisl_name = argv[i];
		}
		else if ( 0==strcmp("-jtstest", argv[i]) ) {
			if ( ++i == argc )
				usage("-jtstest: which JTS test file name?");
			jtstest_filename = argv[i];
		}
		else if ( 0==strcmp("-report", argv[i]) ) {
			do_report = true;
		}
		
		// OPTIONS
		else if ( 0==strcmp("-pixprop", argv[i]) ) {
			if ( ++i == argc )
				usage("-pixprop: pixel proportion?");
			double pix_prop = atof(argv[i]);
			if ( pix_prop < 0.0 || pix_prop > 1.0 )
				usage("invalid pixel proportion");
			Traverser::setPixelProportion(pix_prop);
		}
		else if ( 0==strcmp("-in", argv[i]) ) {
			only_in_feature = true;
		}
		else if ( 0==strcmp("-srs", argv[i]) ) {
			if ( ++i == argc )
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
	
	// module initialization
	Raster::init();
	Vector::init();
	
	// create basic objects:
	Raster* rast = NULL;
	Vector* vect = NULL;
	
	if ( raster_filename )
		rast = new Raster(raster_filename);
	if ( vector_filename )
		vect = new Vector(vector_filename);

	if ( db_name     && mini_prefix 
	||   envisl_name && mini_prefix
	||   db_name     && envisl_name ) {
		usage("Only one of -db, -envisl, or -mr, please\n");
	}
	
	CPLPushErrorHandler(starspan_myErrorHandler);
	
	
	// COMMANDS
	if ( db_name ) { 
		if ( !rast || !vect ) {
			usage("-db option requires both a raster and a vector file to proceed\n");
		}
		return starspan_db(rast, vect, db_name);
	}
	else if ( envisl_name ) { 
		if ( !rast || !vect ) {
			usage("-envisl option requires both a raster and a vector file to proceed\n");
		}
		return starspan_gen_envisl(rast, vect, envisl_name, mini_srs);
	}
	else if ( mini_prefix ) {
		if ( !rast || !vect ) {
			usage("-mr option requires both a raster and a vector file to proceed\n");
		}
		return starspan_minirasters(*rast, *vect, mini_prefix, only_in_feature, mini_srs);
	}
	else if ( jtstest_filename ) {
		starspan_jtstest(*rast, *vect, jtstest_filename);
	}
	else if ( do_report ) {
		if ( !rast && !vect ) {
			usage("-report: Please give a raster and/or a vector file to report\n");
		}
		starspan_report(rast, vect);
	}
	else {
		usage("what should I do?\n");
	}
	
	
	return 0;
}

