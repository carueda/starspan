/*
	Vector - vector interface implemented on shapelib
	$Id$
	See Vector.h for public doc.
*/

#include "Vector.h"

// implementation is based on Shapefile
#include "shapefil.h"       

// my polygon layer
#include "Polygon.h"        


static void process_shape(int i, SHPObject* psShape) {
	printf( "\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
			"  Bounds:(%12.3f,%12.3f) to (%12.3f,%12.3f)\n",
			i, SHPTypeName(psShape->nSHPType),
			psShape->nVertices, psShape->nParts,
			psShape->dfXMin, psShape->dfYMin,
			psShape->dfXMax, psShape->dfYMax
	);

	Polygon* polygon = new Polygon();
	for( int j = 0, iPart = 1; j < psShape->nVertices; j++ ) {
		const char	*pszPartType = "";

		if( j == 0 && psShape->nParts > 0 )
			pszPartType = SHPPartTypeName( psShape->panPartType[0] );

		const char 	*pszPlus;
		
		if( iPart < psShape->nParts
		&& psShape->panPartStart[iPart] == j ) {
			pszPartType = SHPPartTypeName( psShape->panPartType[iPart] );
			iPart++;
			pszPlus = "+";
		}
		else
			pszPlus = " ";

		printf("   %s (%12.3f,%12.3f) %s \n",
			   pszPlus,
			   psShape->padfX[j],
			   psShape->padfY[j],
			   pszPartType 
		);
		
		polygon->addPoint(psShape->padfX[j], psShape->padfY[j]);
	}
	SHPDestroyObject( psShape );

	printf(
		"Polygon info:\n"
		"  size     = %d\n"
		"  isSimple = %s\n"
		"  isConvex = %s\n"
		, 
			polygon->getSize(),
			polygon->isSimple() ? "yes" : "no",
			polygon->isConvex() ? "yes" : "no"
	);
}




int Vector::init() {
    // nothing to do.
	return 0;
}


Vector::Vector(const char* shpfilename) {
	SHPHandle hSHP = SHPOpen(shpfilename, "rb" );
	if( hSHP == NULL ) {
		fprintf(stderr, "Unable to open: %s\n", shpfilename);
		exit(1);
	}
	int nShapeType, nEntities;
	double adfMinBound[4], adfMaxBound[4];

	SHPGetInfo( hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound );

	if ( 0 ) {
		printf("Shapefile Type: %s   # of Shapes: %d\n\n",
			   SHPTypeName( nShapeType ), nEntities
		);
		printf("(x0,y0) -> (x1,y1) = (%12.3f,%12.3f) -> (%12.3f,%12.3f)\n",
			   adfMinBound[0],
			   adfMinBound[1],
			   adfMaxBound[0],
			   adfMaxBound[1]
		);
		printf("(x1-x0, y1-y0) = (%12.3f,%12.3f)\n",
			   adfMaxBound[0] - adfMinBound[0],
			   adfMaxBound[1] - adfMinBound[1]
		);
	}

	// process all shapes:
	for( int i = 0; i < nEntities; i++ ) {
		SHPObject* psShape = SHPReadObject( hSHP, i);
		process_shape(i, psShape);
	}

	SHPClose( hSHP );
	
}

