//
// rasterizers
// Carlos A. Rueda
// $Id$
//

#ifndef rasterizers_h
#define rasterizers_h

/**
  * Rasterizes lines within sub-pixel accuracy.
  * See http://www.antigrain.com/doc/introduction/introduction.agdoc.html
  *
  * A subclass will provide the required functionality
  * via the pixelFound operation.
  */
class LineRasterizer {
public:
	/**
	  * Creates a line rasterizer.
	  * @param pixel_size_x_ pixel size in x direction
	  * @param pixel_size_y_ pixel size in y direction
	  */
	LineRasterizer(double pixel_size_x_, double pixel_size_y_) 
	: pixel_size_x(pixel_size_x_), pixel_size_y(pixel_size_y_) {}

	/**
	  * Rasterizes a line between two given vertices.
	  * Rasterization is accomplished by calling pixelFound
	  * for each pixel in the interpolation connecting
	  * (x1,y1) and (x2,y2).
	  */
	void line(double x1, double y1, double x2, double y2); 
	
	/**
	  * Called when a new pixel location is computed as part
	  * of a rasterization.
	  * (x,y) is given in user coordinates, where x and y are
	  * multiple of corresponding pixel coordinate sizes.
	  */
	virtual void pixelFound(double x, double y) {};

protected:
	double pixel_size_x;
	double pixel_size_y;
};


#endif

