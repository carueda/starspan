//
// LFS test 
// Carlos A. Rueda
// See README.txt 
// $Id$ 
//

#include "gdal.h"           
#include "gdal_priv.h"
#include "ogr_srs_api.h"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;


static const char* raster_filename = "BIGRASTER";
static const off_t cols = 46341;
static const off_t lins = 46341;
static const off_t size = cols * lins; // == 2147488281
static const off_t position = (off_t) 1 << 31;  // == 2147483648 
static const off_t difference = size - position; 


static long col = 41707;
static long row = 46340;
static long ban = 1;


static void create_large_raster(void) {
	FILE* file = fopen(raster_filename, "w");
	if ( !file ) {
		cerr<< " could not create " << raster_filename << endl;
		exit(1);
	}
	cerr<< " Size = " <<size<< endl;
	
	cerr<< "  seeking to 2^31 position" << endl;
	cerr<< "  which is col,row = " <<((off_t)1 << 31)%46341<< "," <<46340<< endl;
	MY_SEEK(file, position, SEEK_SET); 
	
	cerr<< "Completing size with " <<difference<< " ones" <<endl;
	for ( int i = 0; i < difference; i++ ) {
		fputc(1, file);
	}
	fclose(file);
	
	cerr<< "Creating header" <<endl;
	string headername = raster_filename;
	headername += ".hdr";
	ofstream header;
	header.open(headername.c_str());
	if ( !header ) {
		cerr<< " could not create " << headername << endl;
		exit(1);
	}
	header<< "ENVI\n"
	      << "description = { large raster file for LFS test }\n"
	      << "samples = " <<cols<< "\n"
	      << "lines   = " <<lins<< "\n"
	      << "bands   = 1\n"
	      << "header offset = 0\n"
	      << "file type = ENVI Standard\n"
	      << "data type = 1\n"
	      << "interleave = bip\n"
	      << "byte order = 1\n"
	;
}


static void extract_pixels(void) {
	
	cerr<< "raster: " <<raster_filename<< endl;
	cerr<< "col = " <<col<< endl;
	cerr<< "row = " <<row<< endl;
	cerr<< "ban = " <<ban<< endl;
	
	GDALAllRegister();


	GDALDataset* dataset = (GDALDataset*) GDALOpen(raster_filename, GA_ReadOnly);	
    if( dataset == NULL ) {
        fprintf(stderr, "GDALOpen failed: %s\n", CPLGetLastErrorMsg());
        exit(1);
    }

	GDALRasterBand* band = dataset->GetRasterBand(ban);
    if( band == NULL ) {
        fprintf(stderr, "GDALOpen failed: %s\n", CPLGetLastErrorMsg());
        exit(1);
    }
	
	for ( int i = 0; i < 2; i++ ) {
		unsigned char buf;
		
		int status = band->RasterIO(
			GF_Read,
			col++, row,
			1, 1,             // nXSize, nYSize
			&buf,              // pData
			1, 1,             // nBufXSize, nBufYSize
			GDT_Byte,         // eBufType
			0, 0              // nPixelSpace, nLineSpace
		);
		
		if ( status != CE_None ) {
			fprintf(stdout, "Error reading band value, status= %d\n", status);
			exit(1);
		}
		
		cout<< (int)buf;
	}
	cout<<endl;
	
	GDALDestroyDriverManager();
	CPLPopErrorHandler();
}


int main(int argc, char ** argv) {
	create_large_raster();
	extract_pixels();
	return 0;
}

