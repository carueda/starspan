#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rasterizers.h"


class MyLineRasterizerObserver : public LineRasterizerObserver {
public:	
	void pixelFound(double x, double y) {
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

	LineRasterizer lr(pix_size_x, pix_size_y);
	MyLineRasterizerObserver observer;
	lr.addObserver(&observer);
	
	fprintf(stdout, "From  %g %g  ->  %g %g:\n", x1, y1, x2, y2);
	lr.line(x1, y1, x2, y2);
	
    return 0;
}

