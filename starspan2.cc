//
// starspan2 - second version
//         uses Vector.h, Raster.h
// $Id$
//

#include "Raster.h"           
#include "Vector.h"       

#include <stdio.h>
#include <stdlib.h>

static char rcsid[] = "$Id$";


static void usage(const char* msg) {
	if ( msg ) { 
		fprintf(stderr, "starspan: %s\n", msg); 
		fprintf(stderr, "Try `starspan -help' for help\n");
		exit(1);
	}
	fprintf(stderr, 
		"\n"
		"USAGE:  starspan [-vector <vectorfile>] [-raster <rasterfile>]\n"
		"  <vectorfile>   vector file to read -- Any OGR recognized format\n"
		"  <rasterfile>   raster file to read -- Any GDAL recognized format\n"
		"\n"
	);
	exit(0);
}

///////////////////////////////////////////////////////////////
// main test program
int main(int argc, char ** argv) {
	if ( argc == 1 || 0==strcmp("-help", argv[1]) ) {
		usage(NULL);
	}
	
	// initialization
	Vector::init();
	Raster::init();
	
	for ( int i = 1; i < argc; i++ ) {
		if ( 0==strcmp("-vector", argv[i]) ) {
			if ( ++i == argc )
				usage("vectorfile missing");
			const char* filename = argv[i];
			new Vector(filename);
		}
		else if ( 0==strcmp("-raster", argv[i]) ) {
			if ( ++i == argc )
				usage("rasterfile missing");
			const char* filename = argv[i];
			new Raster(filename);
		}
		else if ( 0==strcmp("-help", argv[i]) ) {
			usage(NULL);
		}
		else {
			usage("invalid arguments");
		}
	}
	
	return 0;
}

