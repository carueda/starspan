/*
	Vector - vector interface implemented on OGR
	$Id$
	See Vector.h for public doc.
*/

#include "Vector.h"

// implementation is based on OGR
#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"



int Vector::init() {
    OGRRegisterAll();
	return 0;
}

static void ReportOnLayer( OGRLayer * poLayer) {
    OGRFeatureDefn      *poDefn = poLayer->GetLayerDefn();

    printf( "\n" );
    printf( "Layer name: %s\n", poDefn->GetName() );
	printf( "Geometry: %s\n", 
			OGRGeometryTypeToName( poDefn->GetGeomType() ) );
	printf( "Feature Count: %d\n", poLayer->GetFeatureCount() );
	
	OGREnvelope oExt;
	if (poLayer->GetExtent(&oExt, TRUE) == OGRERR_NONE)
	{
		printf("Extent: (%f, %f) - (%f, %f)\n", 
			   oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
	}

	char    *pszWKT;
	
	if( poLayer->GetSpatialRef() == NULL )
		pszWKT = CPLStrdup( "(unknown)" );
	else
	{
		poLayer->GetSpatialRef()->exportToPrettyWkt( &pszWKT );
	}            

	printf( "Layer SRS WKT:\n%s\n", pszWKT );
	CPLFree( pszWKT );

	for( int iAttr = 0; iAttr < poDefn->GetFieldCount(); iAttr++ )
	{
		OGRFieldDefn    *poField = poDefn->GetFieldDefn( iAttr );
		
		printf( "%s: %s (%d.%d)\n",
				poField->GetNameRef(),
				poField->GetFieldTypeName( poField->GetType() ),
				poField->GetWidth(),
				poField->GetPrecision() );
	}

    OGRFeature  *poFeature;

	int     nFetchFID = OGRNullFID;
	int     bSummaryOnly = FALSE;	
	
    if( nFetchFID == OGRNullFID && !bSummaryOnly )
    {
        while( (poFeature = poLayer->GetNextFeature()) != NULL )
        {
            poFeature->DumpReadable( stdout );
            delete poFeature;
        }
    }
    else if( nFetchFID != OGRNullFID )
    {
        poFeature = poLayer->GetFeature( nFetchFID );
        if( poFeature == NULL )
        {
            printf( "Unable to locate feature id %d on this layer.\n", 
                    nFetchFID );
        }
        else
        {
            poFeature->DumpReadable( stdout );
            delete poFeature;
        }
    }
}


Vector::Vector(const char  *pszDataSource) {
	OGRSFDriver* poDriver;
	OGRDataSource* poDS = OGRSFDriverRegistrar::Open(pszDataSource, true, &poDriver);
    if( poDS == NULL ) {
        fprintf(stderr, "Unable to open datasource `%s'\n", pszDataSource);
		exit(1);
	}
	fprintf(stdout, "poDS->GetLayerCount() = %d\n", poDS->GetLayerCount());
	
	for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ )
	{
		OGRLayer        *poLayer = poDS->GetLayer(iLayer);

		if( poLayer == NULL )
		{
			printf( "FAILURE: Couldn't fetch advertised layer %d!\n",
					iLayer );
			exit( 1 );
		}

		ReportOnLayer(poLayer);
	}

    delete poDS;
    delete OGRSFDriverRegistrar::GetRegistrar();
    CPLFinderClean();
}

