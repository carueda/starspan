/*
	Raster - raster interface implemented on GDAL
	$Id$
	See Raster.h for public doc.
*/

#include "Raster.h"

// implementation is based on GDAL
#include "gdal.h"           
#include "ogr_srs_api.h"


int Raster::init() {
    GDALAllRegister();
	return 0;
}


static int 
GDALInfoReportCorner( GDALDatasetH hDataset, 
                      const char * corner_name,
                      double x, double y )

{
    double	dfGeoX, dfGeoY;
    const char  *pszProjection;
    double	adfGeoTransform[6];
    OGRCoordinateTransformationH hTransform = NULL;
        
    printf( "%-11s ", corner_name );
    
/* -------------------------------------------------------------------- */
/*      Transform the point into georeferenced coordinates.             */
/* -------------------------------------------------------------------- */
    if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None )
    {
        pszProjection = GDALGetProjectionRef(hDataset);

        dfGeoX = adfGeoTransform[0] + adfGeoTransform[1] * x
            + adfGeoTransform[2] * y;
        dfGeoY = adfGeoTransform[3] + adfGeoTransform[4] * x
            + adfGeoTransform[5] * y;
    }

    else
    {
        printf( "(%7.1f,%7.1f)\n", x, y );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Report the georeferenced coordinates.                           */
/* -------------------------------------------------------------------- */
    if( ABS(dfGeoX) < 181 && ABS(dfGeoY) < 91 )
    {
        printf( "(%12.7f,%12.7f) ", dfGeoX, dfGeoY );

    }
    else
    {
        printf( "(%12.3f,%12.3f) ", dfGeoX, dfGeoY );
    }

/* -------------------------------------------------------------------- */
/*      Setup transformation to lat/long.                               */
/* -------------------------------------------------------------------- */
    if( pszProjection != NULL && strlen(pszProjection) > 0 )
    {
        OGRSpatialReferenceH hProj, hLatLong = NULL;

        hProj = OSRNewSpatialReference( pszProjection );
        if( hProj != NULL )
            hLatLong = OSRCloneGeogCS( hProj );

        if( hLatLong != NULL )
        {
            CPLPushErrorHandler( CPLQuietErrorHandler );
            hTransform = OCTNewCoordinateTransformation( hProj, hLatLong );
            CPLPopErrorHandler();
            
            OSRDestroySpatialReference( hLatLong );
        }

        if( hProj != NULL )
            OSRDestroySpatialReference( hProj );
    }

/* -------------------------------------------------------------------- */
/*      Transform to latlong and report.                                */
/* -------------------------------------------------------------------- */
    if( hTransform != NULL 
        && OCTTransform(hTransform,1,&dfGeoX,&dfGeoY,NULL) )
    {
        
        printf( "(%s,", GDALDecToDMS( dfGeoX, "Long", 2 ) );
        printf( "%s)", GDALDecToDMS( dfGeoY, "Lat", 2 ) );
    }

    if( hTransform != NULL )
        OCTDestroyCoordinateTransformation( hTransform );
    
    printf( "\n" );

    return TRUE;
}




Raster::Raster(const char* rastfilename) {
	GDALDatasetH hDataset = GDALOpen(rastfilename, GA_ReadOnly);
    
    if( hDataset == NULL ) {
        fprintf( stderr,
                 "GDALOpen failed - %d\n%s\n",
                 CPLGetLastErrorNo(), CPLGetLastErrorMsg() );
        exit(1);
    }
	
    GDALDriverH hDriver = GDALGetDatasetDriver( hDataset );
    printf( "Driver: %s/%s\n",
            GDALGetDriverShortName( hDriver ),
            GDALGetDriverLongName( hDriver ) );

    printf( "Size is %d, %d\n",
            GDALGetRasterXSize( hDataset ), 
            GDALGetRasterYSize( hDataset ) );
	
    printf( "Corner Coordinates:\n" );
    GDALInfoReportCorner( hDataset, "Upper Left", 
                          0.0, 0.0 );
    GDALInfoReportCorner( hDataset, "Lower Left", 
                          0.0, GDALGetRasterYSize(hDataset));
    GDALInfoReportCorner( hDataset, "Upper Right", 
                          GDALGetRasterXSize(hDataset), 0.0 );
    GDALInfoReportCorner( hDataset, "Lower Right", 
                          GDALGetRasterXSize(hDataset), 
                          GDALGetRasterYSize(hDataset) );
    GDALInfoReportCorner( hDataset, "Center", 
                          GDALGetRasterXSize(hDataset)/2.0, 
                          GDALGetRasterYSize(hDataset)/2.0 );

						  
	GDALClose( hDataset );
	
}

