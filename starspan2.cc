//
// STARSpan project
// Carlos A. Rueda
// starspan2 - second version
//         uses Vector.h, Raster.h
// $Id$
//

#include "starspan.h"           

#include <stdlib.h>

#define VERSION "0.2"


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
		"\n"
		"USAGE:\n"
		"\n"
		"  starspan -help\n"
		"      prints this message and exits\n"
		"\n"
		"  starspan -xhelp\n"
		"      prints help for extended usage\n"
		"\n"
		"\n"
		"  starspan args...\n"
		"\n"
		"   Inputs:\n"
		"      -vector <filename>   An OGR recognized vector file\n"
		"      -raster <filename>   A GDAL recognized raster file\n"
		"\n"
		"   Options:\n"
		"      -report              shows info about given input files\n"
		"                           similar to gdalinfo and ogrinfo.\n"
		"      -mr <prefix> <srs>   generates a mini raster for each intersecting feature\n"
		"                           <prefix> : used for created raster names\n"
		"                           <srs>    : See gdal_translate option -a_srs\n"
		"                                      use - to take projection from input raster\n"
		"      -jtstest <filename>  generates a JTS test file with given name\n"
		"                           -xhelp explains what to do next.\n"
		"\n"
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
	const char*  mini_prefix = NULL;
	const char*  mini_srs = NULL;
	const char* jtstest_filename = NULL;
	
	for ( int i = 1; i < argc; i++ ) {
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
		else if ( 0==strcmp("-help", argv[i]) ) {
			usage(NULL);
		}
		else if ( 0==strcmp("-mr", argv[i]) ) {
			if ( ++i == argc )
				usage("-mr: which prefix?");
			mini_prefix = argv[i];
			if ( ++i == argc )
				usage("-mr: which srs?");
			if ( strcmp("-", argv[i]) )
				mini_srs = argv[i];
		}
		else if ( 0==strcmp("-jtstest", argv[i]) ) {
			if ( ++i == argc )
				usage("-jtstest: which JTS test file name?");
			jtstest_filename = argv[i];
		}
		else if ( 0==strcmp("-report", argv[i]) ) {
			do_report = true;
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

	CPLPushErrorHandler(starspan_myErrorHandler);
	
	
	if ( mini_prefix ) {
		// this option takes precedence.
		if ( !rast && !vect ) {
			usage("-mr option requires both a raster and a vector file to process\n");
		}
		return starspan_minirasters(*rast, *vect, mini_prefix, mini_srs);
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

