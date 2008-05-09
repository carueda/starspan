//
// starspan common declarations
// Carlos A. Rueda
// $Id$
//

#ifndef starspan_common_h
#define starspan_common_h

#include <string>
#include <sstream>
#include <vector>

using namespace std;


/** buffer parameters.
 * These parameters are applied on geometry features
 * before computing the intersection.
 * By default, no buffer operation will be applied.
  */
struct BufferParams {
	/** were given? */
	bool given;
	
	/** Distance to be applied.
	  * If it starts with '@' then it indicates the name of the attribute 
	  * from which this value will be obtained.
	  */
	string distance;

	/** Number of segments to use to approximate a quadrant of a circle.
	  * If it starts with '@' then it indicates the name of the attribute 
	  * from which this value will be obtained.
	  */
	string quadrantSegments;
};


/** Box parameters.
 * Sets a fixed box centered according to bounding box
 * of the geometry features before computing the intersection.
 * By default, no box is used.
 */
struct BoxParams {
	/** were given? */
	bool given;
	
	double width;
	double height;
	
};


/** Class for duplicate pixel modes.  */
struct DupPixelMode {
	string code;
	double arg;
	
	DupPixelMode(string code, double arg) :
		code(code), arg(arg), withArg(true) {
	}
	
	DupPixelMode(string code) :
		code(code), withArg(false) {
	}
	
	string toString() {
		ostringstream ostr;
		ostr << code;
        if ( withArg ) {
            ostr << " " << arg;
        }
		return ostr.str();
	}
    
    private:
        bool withArg;
};


/** 
 * vector selection parameters
 */
struct VectorSelectionParams {
    /** sql statement */
    string sql;
    
    /** where statement */
    string where;
    
    /** dialect */
    string dialect;
    
    VectorSelectionParams() : sql(""), where(""), dialect("") {}
    
};


/** Options that might be used by different services.
  */
struct GlobalOptions {
	bool use_pixpolys;
    
    /** should invalid polygons be skipped?
	 * By default all polygons are processed. 
     */
	bool skip_invalid_polys;

	/**
	 * The proportion of intersected area required for a pixel to be included.
	 * This parameter is only used during processing of polygons.
     * Value assumed to be in [0.0, 1.0].
     */
	double pix_prop;
	
	/** vector selection parameters */
	VectorSelectionParams vSelParams;
	
	/** If non-negative, it will be the desired FID to be extracted */
	long FID;
	
	bool verbose;
	
	bool progress;
	double progress_perc;

	/** param noColRow if true, no col,row fields will be included */
	bool noColRow;
	
	/** if true, no x,y fields will be included */
  	bool noXY;

	bool only_in_feature;
	
	/** Style for RID values in output.
	  * "file" : the simple filename without path 
	  * "path" : exactly as obtained from command line or field in vector 
	  * "none" : no RID is included in output
	  */
	string RID;
	
	bool report_summary;
	
	/** value used as nodata */
	double nodata;  
	
	/** buffer parameters */
	BufferParams bufferParams;
	
	/** box parameters */
	BoxParams boxParams;
	
	/** miniraster parity */
	string mini_raster_parity;
	
	/** separation in pixels between minirasters in strip */
	int mini_raster_separation;
	
    
	/** separator for CSV files */
	string delimiter;
	
	
	/** Given duplicate pixel modes. Empty if not given. */
	vector<DupPixelMode> dupPixelModes;
};

extern GlobalOptions globalOptions;


#endif

