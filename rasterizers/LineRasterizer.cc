//
// STARSpan project
// Line rasterization using a Bresenham interpolator 
// with subpixel accuracy (AGG)
// Carlos A. Rueda
// $Id$
//

#include "rasterizers.h"           
#include "agg_dda_line.h"   // line_bresenham_interpolator

#include <math.h>

// Implementation is based on agg_renderer_primitives.h


// user location to agg location with 8-bit accuracy
inline int to_int(double dv, double size_v) { 
	int v = (int) floor( 256.0 * (dv / size_v) );
	return v;
}

// agg location to user location
inline double to_double(int iv, double size_v) { 
	double v = iv * size_v;
	return v;
}


void LineRasterizer::line(double dx1, double dy1, double dx2, double dy2)  {
	int x1 = to_int(dx1, pixel_size_x);
	int y1 = to_int(dy1, pixel_size_y);
	int x2 = to_int(dx2, pixel_size_x);
	int y2 = to_int(dy2, pixel_size_y);

	agg::line_bresenham_interpolator li(x1, y1, x2, y2);

	unsigned len = li.len();
	if(len == 0) {
		double x = to_double(li.line_lr(x1),  pixel_size_x);
		double y = to_double(li.line_lr(y1),  pixel_size_y);
		pixelFound(x, y);
		return;
	}

	++len;

	if(li.is_ver()) {
		do {
			double x = to_double(li.x2(), pixel_size_x);
			double y = to_double(li.y1(), pixel_size_y);
			pixelFound(x, y);
			li.vstep();
		}
		while(--len);
	}
	else {
		do {
			double x = to_double(li.x1(), pixel_size_x);
			double y = to_double(li.y2(), pixel_size_y);
			pixelFound(x, y);
			li.hstep();
		}
		while(--len);
	}
}



