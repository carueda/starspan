//
// rasterizers
// Carlos A. Rueda
// $Id$
//

#ifndef rasterizers_h
#define rasterizers_h

/**
  * Any object interested in doing some task as pixels are
  * found must extend this interface.
  */
class LineRasterizerObserver {
public:
	virtual ~LineRasterizerObserver() {}
	
	/**
	  * Called when a new pixel location is computed as part
	  * of a rasterization.
	  * (x,y) is given in user coordinates, where x and y are
	  * multiple of corresponding pixel coordinate sizes.
	  */
	virtual void pixelFound(double x, double y) {};
};

	
/**
  * Rasterizes lines within sub-pixel accuracy.
  * See http://www.antigrain.com/doc/introduction/introduction.agdoc.html
  *
  * An observer should be registered for the
  * actual work to be done.
  */
class LineRasterizer {
public:
	/**
	  * Creates a line rasterizer.
	  * @param pixel_size_x_ pixel size in x direction
	  * @param pixel_size_y_ pixel size in y direction
	  */
	LineRasterizer(double pixel_size_x_, double pixel_size_y_); 

	/**
	  * Set the observer for this rasterizer.
	  */
	void setObserver(LineRasterizerObserver* aObserver) { observer = aObserver; }
	LineRasterizerObserver* getObserver(void) { return observer; }
	
	/**
	  * Rasterizes a line between two given vertices.
	  * Rasterization is accomplished by calling observer->pixelFound
	  * for each pixel in the interpolation connecting
	  * (x1,y1) and (x2,y2).
	  *
	  * @param last true to include last pixel corresponding to end point
	  *         (x2,y2).  false by default.
	  */
	void line(double x1, double y1, double x2, double y2, bool last=false); 
	
protected:
	double pixel_size_x;
	double pixel_size_y;
	LineRasterizerObserver* observer;
};


#endif

