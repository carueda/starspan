//
// starspan2 - second version
//         uses Vector.h, Raster.h
// $Id$
//

#include "Raster.h"           
#include "Vector.h"       

#include <stdio.h>
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
		"starspan %s (%s %s)\n"
		"(under development)\n"
		"\n"
		"USAGE:\n"
		"  starspan -help\n"
		"      prints this message and exits\n"
		"  starspan options...\n"
		"   Options:\n"
		"    -vector <filename>   An OGR recognized vector file\n"
		"    -raster <filename>   A GDAL recognized raster file\n"
		"\n", VERSION, __DATE__, __TIME__
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
	
	// initialization
	Vector::init();
	Raster::init();
	
	for ( int i = 1; i < argc; i++ ) {
		if ( 0==strcmp("-vector", argv[i]) ) {
			if ( ++i == argc )
				usage("missing vector file");
			vector_filename = argv[i];
			
		}
		else if ( 0==strcmp("-raster", argv[i]) ) {
			if ( ++i == argc )
				usage("missing raster file");
			raster_filename = argv[i];
		}
		else if ( 0==strcmp("-help", argv[i]) ) {
			usage(NULL);
		}
		else {
			usage("invalid arguments");
		}
	}
	
	if ( vector_filename ) {
		new Vector(vector_filename);
	}
	
	if ( raster_filename ) {
		new Raster(raster_filename);
	}
	
	return 0;
}

