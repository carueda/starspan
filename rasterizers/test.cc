#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rasterizers.h"


class LineRasterizerTest : public LineRasterizer {
public:	
	LineRasterizerTest(double pixel_size_x_, double pixel_size_y_) 
	: LineRasterizer(pixel_size_x_, pixel_size_y_) {}
	
	
	virtual void pixelFound(double x, double y) {
		fprintf(stdout, "\t%g %g \n", x, y);
	}
};

int main(int argc, char** argv) {
	if ( argc != 7 ) {
		fprintf(stderr, "test pix_size_x pix_size_y x1 y1 x2 y2\n");
		return 0;
	}
	int arg = 1;
	double pix_size_x = atof(argv[arg++]);
	double pix_size_y = atof(argv[arg++]);
	double x1 = atof(argv[arg++]);
	double y1 = atof(argv[arg++]);
	double x2 = atof(argv[arg++]);
	double y2 = atof(argv[arg++]);

	LineRasterizerTest lr(pix_size_x, pix_size_y);
	
	fprintf(stdout, "From  %g %g  ->  %g %g:\n", x1, y1, x2, y2);
	lr.line(x1, y1, x2, y2);
	
    return 0;
}

