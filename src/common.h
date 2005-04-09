//
// starspan common declarations
// Carlos A. Rueda
// $Id$
//

#ifndef starspan_common_h
#define starspan_common_h

#include <string>

using namespace std;


/** buffer parameters
  */
struct BufferParams {
	/** were given? */
	bool given;
	
	/** Distance to be applied.
	  * If it starts with '@' then it indicates the name of the attribute 
	  * from which this value will be obtained.
	  */
	string distance;

	/** Number of segments to use to approximate a* quadrant of a circle.
	  * If it starts with '@' then it indicates the name of the attribute 
	  * from which this value will be obtained.
	  */
	string quadrantSegments;
};


/** Options that might be used by different services.
  * This comes in handy while the tool gets more stabilized.
  */
struct GlobalOptions {
	bool use_pixpolys;
	bool skip_invalid_polys;

	double pix_prop;
	
	/** desired FID */
	long FID;
	
	bool verbose;
	
	bool progress;
	double progress_perc;

	/** param noColRow if true, no col,row fields will be included */
	bool noColRow;
	
	/** if true, no x,y fields will be included */
  	bool noXY;

	bool only_in_feature;
	
	/** Make RID exactly as given in command line? 
	  * If not, the simple filename is set as RID. */
	bool RID_as_given;
	
	bool report_summary;
	
	/** value used as nodata */
	double nodata;  
	
	// buffer parameters
	BufferParams bufferParams;
	
	// miniraster parity
	string mini_raster_parity;
	
	/** separation in pixels between minirasters in strip */
	int mini_raster_separation;
};

extern GlobalOptions globalOptions;


#endif
