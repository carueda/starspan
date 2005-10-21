//
// starspan.extractpoints
// Carlos A. Rueda
// $Id$
// :tabSize=4:indentSize=4:noTabs=false:
// :folding=indent:collapseFolds=2:
//

#include "Raster.h"
#include "Vector.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>


#define VERSION "0.1"


static const char* vector_filename = 0;
static vector<const char*> raster_filenames;
static const char* mini_raster_strip_filename = NULL;
static long FID = -1;
static set<long> skipfids;
	


static FILE* img;
static FILE* stdlog;

static string img_filename;
static string hdr_filename;
static string log_filename;

static 	vector<Raster*> rasts;
static int bands = 0;
static unsigned bufsize;
static char* nullbuffer;              


static Vector* vect;
static OGRLayer* layer;

static unsigned num_intersecting_pixels = 0;              
static unsigned num_empty_pixels = 0;              
                             
	
static void _init(string& prefix) {
	Raster::init();
	Vector::init();      
	
	img_filename = prefix + ".img";
	hdr_filename = prefix + ".hdr";
	log_filename = prefix + ".log";
	
	img = fopen(img_filename.c_str(), "wb");
	if ( !img) {
		cerr<< "Couldn't create output image " <<img_filename<< endl;
		exit(1);
	}

	stdlog = fopen(log_filename.c_str(), "w");
	if ( !stdlog) {
		cerr<< "Couldn't create report file " <<log_filename<< endl;
		exit(1);
	}       

	vect = Vector::open(vector_filename);
	if ( !vect ) {
		fprintf(stderr, "Cannot open %s\n", vector_filename);
		exit(1);
	}

	layer = vect->getLayer(0);
	if ( !layer ) {
		cerr<< "Couldn't get layer from " << vect->getName()<< endl;
		exit(1);
	}
	layer->ResetReading();
	
	if ( layer->GetSpatialRef() == NULL ) {
		fprintf(stdout, "# %s: WARNING: UNKNOWN SPATIAL REFERENCE IN VECTOR FILE!\n", vect->getName());
		fprintf(stdlog, "# %s: WARNING: UNKNOWN SPATIAL REFERENCE IN VECTOR FILE!\n", vect->getName());
	}
		
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		Raster* rast = new Raster(raster_filenames[i]);
		rasts.push_back(rast);

		const char* raster_projection = rast->getDataset()->GetProjectionRef();
		if ( !raster_projection || strlen(raster_projection) == 0 ) {
			fprintf(stdout, "# %s: WARNING: UNKNOWN PROJECTION IN RASTER FILE!\n", raster_filenames[i]);
			fprintf(stdlog, "# %s: WARNING: UNKNOWN PROJECTION IN RASTER FILE!\n", raster_filenames[i]);
		}
	
	}
}

static void _end() {
	char msg[1024];
	
	sprintf(msg, "\n"
		"# Summary:\n"
		"# intersecting points     = %4u\n"
		"# non-intersecting points = %4u\n"
		"# total processed points  = %4u\n"
		, num_intersecting_pixels,
		  num_empty_pixels, 
		  num_empty_pixels + num_intersecting_pixels
	); 
	
	fprintf(stdlog, "%s\n", msg); 
	fflush(stdlog);
	
	fprintf(stdout, "%s\n", msg);
	fflush(stdout);

	
	fclose(img);
	fclose(stdlog);
	
	if ( nullbuffer ) {
		delete[] nullbuffer;
	}
	GDALDestroyDriverManager();
	CPLPopErrorHandler();
}



static void process_feature(OGRFeature* feature) {
	OGRGeometry* feature_geometry = feature->GetGeometryRef();
	OGRwkbGeometryType geometry_type = feature_geometry->getGeometryType();

	fprintf(stdout, "%ld ", feature->GetFID());
	fprintf(stdlog, "%ld ", feature->GetFID());

	switch ( geometry_type ) {
		case wkbPoint:
		case wkbPoint25D:
			/////////    OK    //////////////
			break;
	
		default:
			cerr<< OGRGeometryTypeToName(geometry_type)<< " : geometry type not considered"<< endl;
			exit(1);
	}
	
	
	OGRPoint* point = (OGRPoint*) feature_geometry;
	double x = point->getX();
	double y = point->getY();

	fprintf(stdout, "%g  %g", x, y);
	fprintf(stdlog, "%g  %g", x, y);
	

	void* band_values = 0;
	const char* raster_filename = 0;
	int raster_col, raster_row;
	// now get (x,y) from the first intersecting raster:
	for ( unsigned i = 0; i < rasts.size(); i++ ) {
		Raster* rast = rasts[i];
		int col, row;
		rast->toClosestColRow(x, y, &col, &row);
		band_values = rast->getBandValuesForPixel(col, row);
		if ( band_values ) {
			// if at least one value is non-zero, we actually choose
			// the bands from this raster; otherwise, we continue with 
			// the next raster.
			// To avoid doing a type-based check, we just check if any
			// byte in the returned buffer is non-zero:
			if ( memcmp(nullbuffer, band_values, bufsize) ) {
				// OK, non-null pixel; take it:
				raster_filename = raster_filenames[i];
				raster_col = col;
				raster_row = row;
				break;
			}
		}
	}
	
	if ( band_values ) { // there was intersection
		fprintf(stdout, " %d %d %s", raster_col, raster_row, raster_filename);
		fprintf(stdlog, " %d %d %s", raster_col, raster_row, raster_filename);
		// copy value to output raster:
		if ( 1 != fwrite(band_values, bufsize, 1, img) ) {
			cerr<< " Cannot write pixel\n";
			exit(1);
		}
		fflush(img);
		num_intersecting_pixels++;
	}
	else {
		fprintf(stdout, " -");
		fprintf(stdlog, " -");
		// fill with nodata values
		if ( 1 != fwrite(nullbuffer, bufsize, 1, img) ) {
			cerr<< " Cannot write pixel (null buffer)\n";
			exit(1);
		}
		fflush(img);
		num_empty_pixels++;
	}
	
	fprintf(stdlog, " (%u)\n", num_empty_pixels + num_intersecting_pixels); 
	fflush(stdlog);
	
	fprintf(stdout, " (%u)\n", num_empty_pixels + num_intersecting_pixels);
	fflush(stdout);
			
}





// prints a help message
static void usage(const char* msg) {
	if ( msg ) { 
		fprintf(stderr, "starspan.extractpoints: %s\n", msg); 
		fprintf(stderr, "Type `starspan.extractpoints --help' for help\n");
		exit(1);
	}
	// header:
	fprintf(stdout, 
		"\n"
		"starspan.extractpoints %s (%s %s)\n"
		"\n"
		, VERSION, __DATE__, __TIME__
	);
	
	// main body:
	fprintf(stdout, 
	"USAGE:\n"
	"  starspan.extractpoints --vector V \\\n"
	"                         --raster R1 R2 ... \\\n"
	"                         --mini_raster_strip prefix \\\n"
	"                         [ --fid FID ]\n"
	"                         [ --skipfids FID FID ... ]\n"
	"\n"
	);
	
	exit(0);
}

///////////////////////////////////////////////////////////////
// main program
int main(int argc, char ** argv) {
	if ( argc == 1 ) {
		usage(NULL);
	}

	for ( int i = 1; i < argc; i++ ) {
		
		//
		// INPUTS:
		//
		if ( 0==strcmp("--vector", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--vector: which vector file?");
			if ( vector_filename )
				usage("--vector specified twice");
			vector_filename = argv[i];
		}
		
		else if ( 0==strcmp("--raster", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* raster_filename = argv[i];
				raster_filenames.push_back(raster_filename);
			}
			if ( raster_filenames.size() == 0 )
				usage("--raster: which raster files?");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
		
		else if ( 0==strcmp("--mini_raster_strip", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mini_raster_strip: filename?");
			mini_raster_strip_filename = argv[i];
		}
		
		else if ( 0==strcmp("--fid", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--fid: desired FID?");
			FID = atol(argv[i]);
		}

		else if ( 0==strcmp("--skipfids", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				long skipfid = atol(argv[i]);
				skipfids.insert(skipfid);
			}
			if ( skipfids.size() == 0 )
				usage("--skipfid: which FIDs?");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
	}

	if ( !vector_filename ) {
		usage("No vector file was specified\n");
	}
	if ( raster_filenames.size() == 0 ) {
		usage("No raster files were specified\n");
	}

	string prefix(mini_raster_strip_filename);

	_init(prefix);		

	

	fprintf(stdlog, "# Command line:");
	for ( int i = 0; i < argc; i++ ) {
		fprintf(stdlog, " %s", argv[i]);
	}
	fprintf(stdlog, "\n");
	
	//
	// Taken from the first raster:
	//
	// number of bands in output image;  
	bands = 0;
	rasts[0]->getSize(0, 0, &bands);
	
	// size on bytes of the array of bands for one pixel
	bufsize = rasts[0]->getBandValuesBufferSize();
	
	// a null buffer:
	nullbuffer = new char[bufsize];
	bzero(nullbuffer, bufsize);
	
	
	fprintf(stdout, "# Bands per pixel = %d\n\n", bands);
	fprintf(stdlog, "# Bands per pixel = %d\n\n", bands);
	
	fprintf(stdout, "# FID  x  y  col row rasterfile  (count)\n");
	fprintf(stdlog, "# FID  x  y  col row rasterfile  (count)\n");
	
	
	// now, traverse the points in vect:
	
	OGRFeature*	feature;

	if ( FID >= 0 ) {
		feature = layer->GetFeature(FID);
		if ( !feature ) {
			cerr<< "FID " <<FID<< " not found in " <<vect->getName()<< endl;
			exit(1);
		}
		process_feature(feature);
		delete feature;
	}
	else{
		//long psize = layer->GetFeatureCount();	
		while( (feature = layer->GetNextFeature()) != NULL ) {
			long fid = feature->GetFID();
			if ( !skipfids.count(fid) ) { 
				process_feature(feature);
			}
			delete feature;
		}
	}
	
	_end();
	
	
	return 0;
}

