/*
	Raster - raster interface implemented on GDAL
	$Id$
	See Raster.h for public doc.
*/

#include "Raster.h"

#include <assert.h>



int Raster::init() {
    GDALAllRegister();
	return 0;
}

int Raster::end() {
	return 0;
}


Raster::Raster(const char* filename, int width, int height, int bands) {
	const char *pszFormat = "ENVI";
    GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
	if( hDriver == NULL ) {
		fprintf(stderr, "Couldn't get driver %s\n", pszFormat);
        exit(1);
	}
	
    char **papszOptions = NULL;

	hDataset = hDriver->Create(
		filename, 
		width, height, bands, 
		GDT_Byte, 
        papszOptions 
	);
	
	if( hDataset == NULL ) {
		fprintf(stderr, "Couldn't create dataset: %s\n", CPLGetLastErrorMsg());
        exit(1);
	}

	
	
	geoTransfOK = GDALGetGeoTransform(hDataset, adfGeoTransform) == CE_None; 
    if( geoTransfOK ) {
        pszProjection = GDALGetProjectionRef(hDataset);
    }
	
	bandValues_buffer = new double[bands];
}

	
Raster::Raster(const char* rastfilename) {
	hDataset = (GDALDataset*) GDALOpen(rastfilename, GA_ReadOnly);
    
    if( hDataset == NULL ) {
        fprintf(stderr, "GDALOpen failed: %s\n", CPLGetLastErrorMsg());
        exit(1);
    }

	geoTransfOK = GDALGetGeoTransform(hDataset, adfGeoTransform) == CE_None; 
    if( geoTransfOK ) {
        pszProjection = GDALGetProjectionRef(hDataset);
    }
	bandValues_buffer = new double[GDALGetRasterCount(hDataset)];
}


void Raster::getSize(int *width, int *height, int *bands) {
	if ( width )  *width = GDALGetRasterXSize(hDataset);
	if ( height ) *height = GDALGetRasterYSize(hDataset);
	if ( bands )  *bands = GDALGetRasterCount(hDataset);
}


inline static void geo_corner(double adfGeoTransform[6], int ix, int iy, double *x, double *y) {
	*x = adfGeoTransform[0] + adfGeoTransform[1] * ix + adfGeoTransform[2] * iy;
	*y = adfGeoTransform[3] + adfGeoTransform[4] * ix + adfGeoTransform[5] * iy;
}


void Raster::getCoordinates(double *x0, double *y0, double *x1, double *y1) {
    if( !geoTransfOK )
		return;
	int width, height;
	getSize(&width, &height, NULL);
	geo_corner(adfGeoTransform,0, 0, x0, y0);
	geo_corner(adfGeoTransform, width-1, height-1, x1, y1);
}

void Raster::getPixelSize(double *pix_x_size, double *pix_y_size) {
    if( !geoTransfOK )
		return;
	if ( pix_x_size ) *pix_x_size = adfGeoTransform[1];
	if ( pix_y_size ) *pix_y_size = adfGeoTransform[5];
}

void Raster::toColRow(double x, double y, int *col, int *row) {
	double pix_x_size = adfGeoTransform[1];
	double pix_y_size = adfGeoTransform[5];
	double x0, y0;
	geo_corner(adfGeoTransform,0, 0, &x0, &y0);
	*col = (int) floor( (x - x0) / pix_x_size );
	*row = (int) floor( (y - y0) / pix_y_size );
}


void* Raster::getBandValuesForPixel(int col, int row) {
	assert(bandValues_buffer);
	
	int width, height, bands;
	getSize(&width, &height, &bands);
	if ( col < 0 || col >= width || row < 0 || row >= height ) {
		return NULL;
	}
	
	char* ptr = (char*) bandValues_buffer;
	for ( int i = 0; i < bands; i++ ) {
		GDALRasterBand* band = (GDALRasterBand*) GDALGetRasterBand(hDataset, i+1);
		GDALDataType bandType = band->GetRasterDataType();
	
		int status = band->RasterIO(
			GF_Read,
			col, row,
			1, 1,             // nXSize, nYSize
			ptr,              // pData
			1, 1,             // nBufXSize, nBufYSize
			bandType,         // eBufType
			0, 0              // nPixelSpace, nLineSpace
		);
		
		if ( status != CE_None ) {
			fprintf(stdout, "Error reading band value, status= %d\n", status);
			exit(1);
		}
		
		int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
		ptr += bandTypeSize;
	}
	
	return bandValues_buffer;
}


void Raster::report(FILE* file) {
	fprintf(file, "%s\n", hDataset->GetDescription());
    GDALDriver* hDriver = hDataset->GetDriver();
    fprintf(file, "Driver: %s/%s\n",
		GDALGetDriverShortName(hDriver),
		GDALGetDriverLongName(hDriver) 
	);
	int width, height, bands;
	getSize(&width, &height, &bands);
    fprintf(file, "Size is %d, %d;  bands = %d\n", width, height, bands);
	
	double pix_x_size, pix_y_size;
	getPixelSize(&pix_x_size, &pix_y_size);
	fprintf(file, "Pixel size: %g x %g\n", pix_x_size, pix_y_size);
	
    fprintf(file, "Corner Coordinates:\n");
    report_corner(file, "Upper Left", 0, 0);
    report_corner(file, "Lower Left", 0, height);
    report_corner(file, "Upper Right", width, 0);
    report_corner(file, "Lower Right", width, height);
	
	fprintf(file, "Metadata:\n");
	char** md = GDALGetMetadata(hDataset, NULL);
	if ( md ) {
		for ( int i = 0; md[i] != NULL; i++ ) {
			fprintf(file, "md[%d] = [%s]\n", i, md[i]);
		}
	}
	else {
		fprintf(file, "\tNo metadata is given by the driver.\n");
	}
}

void Raster::report_corner(FILE* file, const char* corner_name, int ix, int iy) {
    OGRCoordinateTransformationH hTransform = NULL;
        
    fprintf(file, "%-11s ", corner_name );
    
    if( !geoTransfOK ) {
        fprintf(file, "(%7.1f,%7.1f)\n", (double) ix, (double) iy);
        return;
    }
	double dfGeoX, dfGeoY;
	geo_corner(adfGeoTransform, ix, iy, &dfGeoX, &dfGeoY);

	/* Report the georeferenced coordinates.                           */
    if( ABS(dfGeoX) < 181 && ABS(dfGeoY) < 91 ) {
        fprintf(file, "(%12.7f,%12.7f) ", dfGeoX, dfGeoY );
    }
    else {
        fprintf(file, "(%12.3f,%12.3f) ", dfGeoX, dfGeoY );
    }

	/* Setup transformation to lat/long.                               */
    if( pszProjection != NULL && strlen(pszProjection) > 0 ) {
        OGRSpatialReferenceH hProj, hLatLong = NULL;

        hProj = OSRNewSpatialReference( pszProjection );
        if( hProj != NULL )
            hLatLong = OSRCloneGeogCS( hProj );

        if( hLatLong != NULL ) {
            CPLPushErrorHandler( CPLQuietErrorHandler );
            hTransform = OCTNewCoordinateTransformation( hProj, hLatLong );
            CPLPopErrorHandler();
            
            OSRDestroySpatialReference( hLatLong );
        }
        if( hProj != NULL )
            OSRDestroySpatialReference( hProj );
    }

	/* Transform to latlong and report.                                */
    if( hTransform != NULL && OCTTransform(hTransform,1,&dfGeoX,&dfGeoY,NULL) ) {
        fprintf(file, "(%s,", GDALDecToDMS( dfGeoX, "Long", 2 ) );
        fprintf(file, "%s)", GDALDecToDMS( dfGeoY, "Lat", 2 ) );
    }
    if( hTransform != NULL )
        OCTDestroyCoordinateTransformation( hTransform );
    
    fprintf(file, "\n" );
}




Raster::~Raster() {
	if ( bandValues_buffer )
		delete[] bandValues_buffer;
	delete hDataset;
}

